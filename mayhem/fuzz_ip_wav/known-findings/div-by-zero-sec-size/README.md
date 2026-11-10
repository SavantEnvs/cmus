# wav_duration(): division by zero when the sample rate does not fit the sample-format field

- **Where:** `ip/wav.c:374`, `priv->pcm_size / priv->sec_size` (also `ip/wav.c:268`, `offset % priv->frame_size`, on seek).
- **Reproducer:** `reproducer.wav`; `fuzz_ip_wav-standalone reproducer.wav` stops with UBSan `division by zero`
  (SIGFPE without sanitizers).
- **Cause:** the 32-bit sample rate from the `fmt ` chunk (here `0x66000000`) is packed into `sample_format_t`,
  whose rate field is 19 bits (`SF_RATE_MASK 0x001ffffc`); the value is truncated to 0, so
  `sec_size = rate * frame_size` is 0. Nothing validates the rate.
- **Impact:** a crafted .wav kills the player when its duration is read.
- **Fix:** after building `sf`, reject the file if `sf_get_rate(sf) == 0` (or if the header rate exceeds the field's maximum).
