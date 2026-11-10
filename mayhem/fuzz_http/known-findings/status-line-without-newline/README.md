# http_parse_response(): `strchr(str, '\n')` returns NULL for a status line with no newline

- **Where:** `http.c:355-357`, `end = strchr(str, '\n'); hg->reason = xstrndup(str, end - str); str = end + 1;`.
- **Reproducer:** `reproducer.bin` (URI, NUL, then `HTTP/1.1 2...` whose first line holds no `\n` before the
  terminating NUL); `fuzz_http-standalone reproducer.bin` stops with UBSan
  `http.c:357:12: applying non-zero offset 1 to null pointer`.
- **Cause:** the result of `strchr` is not checked. `end - str` is then `NULL - str` (a huge `size_t` for
  `xstrndup`, which happens to stop at the NUL) and `str = end + 1` is `(char *)1`, which the following
  `while (*str)` would dereference once UBSan's pointer check no longer stops first.
- **Impact:** a truncated status line from a server makes the client do NULL-pointer arithmetic and then read
  address 1 (a crash, by reading the code).
- **Fix:** `if (!end) return -2;` right after the `strchr`.
