# wav_open(): allocation sized by an unchecked chunk length

- **Where:** `ip/wav.c:129`, `fmt = xnew(char, fmt_size)`.
- **Reproducer:** `reproducer.wav` (193 bytes): `fuzz_ip_wav reproducer.wav` reports libFuzzer
  `out-of-memory (malloc(3690987536))`. The standalone build has no malloc limit and just returns.
- **Cause:** `fmt_size` is the 32-bit length of the `fmt ` chunk straight from the file; only the lower
  bound (`>= 16`) is checked, and the buffer is allocated before the data is read.
- **Impact:** a 193-byte file makes the player allocate up to 4 GiB (abort on OOM through `xnew`).
- **Fix:** reject `fmt_size` larger than a few hundred bytes (a valid chunk is 16 to 40) before allocating.
