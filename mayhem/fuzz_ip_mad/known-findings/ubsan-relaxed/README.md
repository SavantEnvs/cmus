# ip/nomad.c: UB that the build relaxes (no reproducer is kept: the shipped binary no longer stops on it)

`mayhem/build.sh` compiles `ip/nomad.c` with `-fno-sanitize=float-cast-overflow`.

- **`ip/nomad.c:425`** `calc_bitrate_fast()` stores `(xing.bytes * 8.0) / info.duration` (a `double`) in the
  `int avg_bitrate`; a Xing header with a large byte count and a tiny or zero duration overflows the
  conversion (`-1.27258e+12 is outside the range of representable values of type 'int'`). It is hit by
  most fuzzed MP3s with a Xing header, and with the check on it ends every fuzzing process at once.
  Fix: clamp the quotient before the assignment.

To see it, remove that line from `relax.mk` in `mayhem/build.sh` and run `fuzz_ip_mad` on `testsuite/`.
