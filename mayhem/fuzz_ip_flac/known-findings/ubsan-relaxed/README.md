# ip/flac.c: UB that the build relaxes (no reproducer is kept: the shipped binary no longer stops on it)

`mayhem/build.sh` compiles `ip/flac.c` with `-fno-sanitize=shift-base,shift-exponent,float-cast-overflow`.
Without it every valid FLAC with a negative sample and every header with an odd sample rate halts UBSan
before the decoder is reached.

- **`ip/flac.c:184`** `buf[ch % nch][i] << (bits - depth)` shifts negative samples left (shift-base) and
  shifts by a negative amount when the stream's bits-per-sample exceeds the output depth (shift-exponent,
  seen as `shift exponent -8 is negative`). Fix: multiply by `1 << (bits - depth)` as `int32_t`/`uint32_t`,
  and reject `depth > bits`.
- **`ip/flac.c:482`** `flac_duration()` returns the `double` `total_samples / sample_rate` as `int`; a
  `sample_rate` of 0 gives `inf` (`inf is outside the range of representable values of type 'int'`).
  Fix: reject `sample_rate == 0` in `metadata_cb()`.

To see them, remove that line from `relax.mk` in `mayhem/build.sh` and run `fuzz_ip_flac` on `testsuite/`.
