#!/usr/bin/env bash
#
# mayhem/test.sh: known-answer tests of cmus' parsers. mayhem/build.sh already built mayhem/build/kat from
# the objects of cmus' own normal-flag build; this only runs it and compares what it prints with values
# worked out from the inputs (a patch that stubs a parser out or exits 0 fails). exit 0 = pass.
# Emits a CTRF summary (file + `CTRF {...}` line).
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
: "${SRC:=/mayhem}"
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}


KAT="$SRC/mayhem/build/kat"
SEEDS="$SRC/mayhem"
if [ ! -x "$KAT" ]; then
  echo "missing $KAT: mayhem/build.sh did not build it" >&2
  emit_ctrf "cmus-kat" 0 1 0
  exit 1
fi

passed=0; failed=0
# check <name> <expected> <subcommand> <argument>: runs the probe and compares its whole stdout.
check() {
  local name="$1" want="$2" got
  got="$("$KAT" "$3" "$4" 2>&1)"
  if [ "$got" == "$want" ]; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    printf 'FAIL %s\n  want: %s\n  got:  %s\n' "$name" "${want//$'\n'/ | }" "${got//$'\n'/ | }" >&2
  fi
}

# Filter expressions against one track: artist "Some Artist", title "Some Song", genre Rock, date 1999,
# 215 s, 320 kbit/s (bitrate is compared in kbit/s).
check expr-and        1 expr 'artist="Some Artist" & duration>200'
check expr-no-match   0 expr 'genre="Jazz"'
check expr-glob-int   1 expr 'title="*song*" & date<2000'
check expr-or-true    1 expr '!(genre="Rock") | bitrate>=300'
check expr-or-false   0 expr '!(genre="Rock") | bitrate>=400'
check expr-bad-syntax parse-error expr 'artist="a'

check cue-sheet 'tracks=2 album=Some Album performer=Some Artist
track 1 title=First offset=0.000
track 2 title=Second offset=202.000' cue "$SEEDS/fuzz_cue/testsuite/album.cue"

check id3v2 'artist=Band
title=Song
album=
date=
tracknumber=
genre=' id3 "$SEEDS/fuzz_id3/testsuite/v2.mp3"
check id3v1 'artist=Artist
title=Title
album=Album
date=2001
tracknumber=5
genre=Rock' id3 "$SEEDS/fuzz_id3/testsuite/v1.mp3"

check ape-tag 'items=2
Title=Fuzz
Artist=Seed' ape "$SEEDS/fuzz_ape/testsuite/tagged.wv"

# 8 kHz mono 16-bit and 11.025 kHz stereo 8-bit; `bytes` is the PCM length (0.05 s * 8000 * 2 and
# 0.03 s * 11025 * 2 rounded to whole frames), `sum` a rolling hash of the decoded bytes.
check wav-mono16 'rate=8000 channels=1 bits=16 bytes=800 sum=2199680654' wav "$SEEDS/fuzz_ip_wav/testsuite/mono16.wav"
check wav-stereo8 'rate=11025 channels=2 bits=8 bytes=662 sum=657783449' wav "$SEEDS/fuzz_ip_wav/testsuite/stereo8.wav"

emit_ctrf "cmus-kat" "$passed" "$failed" 0
