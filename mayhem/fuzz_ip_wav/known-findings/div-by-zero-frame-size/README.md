# wav_open(): division by zero when the channel count does not fit the sample-format field

- **Where:** `ip/wav.c:217`, `priv->pcm_size % sf_get_frame_size(ip_data->sf)`.
- **Reproducer:** `reproducer.wav`; `fuzz_ip_wav-standalone reproducer.wav` stops with UBSan `division by zero`
  (SIGFPE without sanitizers).
- **Cause:** the `fmt ` chunk's channel count is stored into the packed `sample_format_t`, whose channel field
  is narrower than the 16-bit header field. The reproducer's count, 38400 (`0x9600`), is `>= 1` so it passes the
  `channels < 1` check, but its low 8 bits are 0, so `sf` holds 0 channels and `sf_get_frame_size()` is zero.
- **Impact:** a crafted .wav kills the player when it is opened or added to the library.
- **Fix:** validate `sf_get_channels(ip_data->sf) > 0` (or reject counts above 255) after building `sf`.
