#!/usr/bin/env bash
#
# mayhem/build.sh — build cmus' fuzz harnesses, their -standalone reproducers and the test oracle.
#
# cmus configures in-tree, so two private copies of the source are staged under mayhem/build/ (which
# mayhem/.gitignore covers, so `git clean -ffdX` removes it) and each is configured and built with the
# project's own ./configure + Makefile:
#   mayhem/build/oracle  normal flags      -> the KAT probes mayhem/test.sh runs
#   mayhem/build/fuzz    sanitizers + DWARF 3 -> the project objects every harness links against
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

: "${SRC:=/mayhem}"
cd "$SRC"
B="$SRC/mayhem/build"
rm -rf "$B"
mkdir -p "$B"

# Plugins we fuzz are pinned on; everything else (output drivers, mp4/ffmpeg/cdio/...) off, so the
# image builds the same thing wherever apt happens to have more or fewer libraries.
CONF="CONFIG_AAC=y CONFIG_CUE=y CONFIG_FLAC=y CONFIG_MAD=y CONFIG_MODPLUG=y CONFIG_MPC=y CONFIG_OPUS=y
 CONFIG_VORBIS=y CONFIG_WAVPACK=y CONFIG_MP4=n CONFIG_FFMPEG=n CONFIG_MIKMOD=n CONFIG_CDIO=n
 CONFIG_CDDB=n CONFIG_DISCID=n CONFIG_MPRIS=n CONFIG_ALSA=n CONFIG_AO=n CONFIG_ARTS=n CONFIG_JACK=n
 CONFIG_PULSE=n CONFIG_SNDIO=n CONFIG_ROAR=n CONFIG_AAUDIO=n"

stage() {
	mkdir -p "$1"
	tar -C "$SRC" --exclude=./mayhem --exclude=./.git -cf - . | tar -C "$1" -xf -
	cp "$SRC"/mayhem/*.c "$SRC"/mayhem/*.cc "$1"/
}

# Project make variable, e.g. `mkvar FLAC_LIBS`.
mkvar() {
	make -s --no-print-directory -f config.mk -f /dev/stdin _p <<<"_p: ; @echo \$($1)"
}

# Objects of the project that the harnesses and probes need. Linked from an archive so each binary
# pulls in only what it references.
CORE="xmalloc.o debug.o file.o glob.o uchar.o comment.o keyval.o convert.o gbuf.o prog.o xstrjoin.o
 id3.o ape.o read_wrapper.o channelmap.o cue.o expr.o track_info.o http.o misc.o path.o u_collate.o"

# ---------------------------------------------------------------- oracle (normal flags)
O="$B/oracle"
stage "$O"
cd "$O"
env -u CFLAGS -u LDFLAGS ./configure CC="$CC" CFLAGS="-g -O2 $COVERAGE_FLAGS" LDFLAGS="$COVERAGE_FLAGS" $CONF >configure.log
make -j"$MAYHEM_JOBS" main plugins >make.log
make -j"$MAYHEM_JOBS" kat.o globals.o ip/wav.lo $CORE >/dev/null
ar rcs libcore.a $CORE
# kat.c's `wav` subcommand calls ip_ops, so the probe is linked with the wav plugin.
$CC $COVERAGE_FLAGS kat.o globals.o ip/wav.lo libcore.a -lm -o "$B/kat"
cd "$SRC"

# ---------------------------------------------------------------- fuzz (sanitized, DWARF 3)
F="$B/fuzz"
stage "$F"
cd "$F"
CF="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS -fdebug-prefix-map=$F=$SRC -O1"
env -u CFLAGS -u LDFLAGS ./configure CC="$CC" CFLAGS="$CF" LDFLAGS="$SANITIZER_FLAGS" $CONF >configure.log

# UBSan checks relaxed on the compile units where they fire on well-formed input or on garbage metadata
# with no memory effect, so they do not end every fuzzing process before the decoder is reached
# (everything else, ASan and the rest of UBSan, stays halting; see mayhem/<target>/known-findings/):
#   shift-base            read_le32() (utils.h) shifts a promoted unsigned char into the sign bit: ape.c, ip/wav.c
#   shift-base/-exponent  ip/flac.c scales PCM samples with `buf << (bits - depth)` (negative values / depth > bits)
#   float-cast-overflow   a double duration/bitrate stored in an int: ip/flac.c (flac_duration), ip/nomad.c
cat >relax.mk <<'MK'
ape.o ip/wav.lo: CFLAGS += -fno-sanitize=shift-base
ip/flac.lo: CFLAGS += -fno-sanitize=shift-base,shift-exponent,float-cast-overflow
ip/nomad.lo: CFLAGS += -fno-sanitize=float-cast-overflow
MK
MK="make -f Makefile -f relax.mk -j$MAYHEM_JOBS"
HARNESSES="fuzz_cue fuzz_expr fuzz_id3 fuzz_ape fuzz_http harness_ip globals"
PLUGINS="wav flac vorbis mad opus mpc wavpack modplug aac"
$MK main plugins >make.log
$MK $(printf '%s.o ' $HARNESSES) $CORE >/dev/null
for p in $PLUGINS; do $MK ip/$p.lo >/dev/null; done
$MK ip/nomad.lo >/dev/null
ar rcs libcore.a $CORE
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c lsan_off.cc -o lsan_off.o

libs_for() {
	case $1 in
		wav) ;;
		flac) mkvar FLAC_LIBS ;;
		vorbis) mkvar VORBIS_LIBS; echo -lvorbis ;;
		mad) mkvar MAD_LIBS ;;
		opus) mkvar OPUS_LIBS; echo -lopus ;;
		mpc) mkvar MPC_LIBS ;;
		wavpack) mkvar WAVPACK_LIBS ;;
		modplug) mkvar MODPLUG_LIBS ;;
		aac) mkvar AAC_LIBS ;;
	esac
}

# link <name> <objects/libs...>: one libFuzzer binary and one standalone reproducer.
link() {
	local name=$1; shift
	$CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$@" lsan_off.o globals.o libcore.a -lm -o "$B/$name"
	$CC $SANITIZER_FLAGS $DEBUG_FLAGS "$STANDALONE_FUZZ_MAIN" "$@" lsan_off.o globals.o libcore.a -lm -o "$B/$name-standalone"
}
for t in cue expr id3 ape http; do link fuzz_$t fuzz_$t.o; done
for p in $PLUGINS; do
	extra=""
	[ "$p" = mad ] && extra=ip/nomad.lo
	link fuzz_ip_$p harness_ip.o ip/$p.lo $extra $(libs_for $p)
done
cd "$SRC"
