# expand_short_expr() writes past the end of its output buffer

- **Where:** `expr.c:677` in `expand_short_expr()`, called from `expr_parse_i()` (`expr.c:761`).
- **Reproducer:** `reproducer.bin` (`~A\0\a~a.`, 6 bytes). `fuzz_expr-standalone reproducer.bin` aborts with an
  ASan `heap-buffer-overflow` (write).
- **Cause:** the short-expression syntax (`~a...`, `~A...`) is expanded into a buffer of `len * 7` bytes
  (`expr.c:523`), but one input character can expand to more than seven output bytes (the key name, `"*`, `*"`,
  `&key<=num` and parentheses), so a short input made of such sequences outgrows it.
- **Impact:** heap corruption from a user-controlled filter string (`:filter`, `:fset`, `cmus-remote -C`,
  and filters loaded from the `autosave` file).
- **Fix:** size `out` for the worst-case expansion of every state, or grow it before each append.
