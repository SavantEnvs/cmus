# http_parse_response(): the status-code digit loop overflows `int`

- **Where:** `http.c:346`, `hg->code *= 10;` inside `while (*str >= '0' && *str <= '9')`.
- **Reproducer:** `reproducer.bin` (URI, NUL, then a response whose status line carries a long run of
  digits); `fuzz_http-standalone reproducer.bin` stops with UBSan
  `signed integer overflow: 444444444 * 10 cannot be represented in type 'int'`.
- **Cause:** the digit loop is unbounded and `hg->code` is a plain `int`, so a status line with more than
  nine digits wraps it.
- **Impact:** undefined behaviour on a server-controlled response line; the wrapped value is only compared
  and stored, so no memory-safety effect.
- **Fix:** stop after three digits (an HTTP status code) or reject the line once `code > 999`.
