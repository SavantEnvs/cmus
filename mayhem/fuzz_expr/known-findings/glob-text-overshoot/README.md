# do_glob_match(): matching past the end of the text (`glob.c:203`, `uchar.c:303`)

- **Where:** `u_get_char()` at `uchar.c:303` reads `s[0]` of the text; it is called from the `?` branch of
  `do_glob_match` (`glob.c:203`), reached recursively from the `*` branch (`glob.c:232`) through `expr_eval`.
- **Reproducer:** `reproducer.bin` (a filter expression with a glob containing non-ASCII text);
  `fuzz_expr-standalone reproducer.bin` stops with ASan `heap-buffer-overflow ... READ of size 1`,
  0 bytes after the 12-byte track string.
- **Cause (from reading the code; the stop is at the `pos + tlen` call, `glob.c:232`):** the `*` branch continues matching at `pos + tlen`, and the text branch advances by
  `strlen(gitem->text)`, both measured in bytes of the *pattern*. The comparison is case/accent-insensitive
  (`u_strcasestr_base`, `u_strncase_equal_base`), so the matched region of the *text* can be shorter in
  bytes than the pattern text, and the cursor lands beyond the terminating NUL. (`do_u_strcasestr` also
  compares `strlen(haystack)` in bytes against `u_strlen(needle)` in characters.)
- **Impact:** out-of-bounds heap read in application code, reachable from any filter or search expression.
- **Fix:** advance by the number of bytes actually matched in the text (have the `u_str*_base` helpers
  report the match end) instead of the pattern's byte length.
