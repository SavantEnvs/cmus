# cue_parser_to_sheet(): `tp->index0 - prev_tp->index1` overflows `int`

- **Where:** `cue.c:436`, `s->tracks[i - 1].length = (tp->index0 - prev_tp->index1) / 75.0;`.
- **Reproducer:** `reproducer.cue`; `fuzz_cue-standalone reproducer.cue` stops with UBSan
  `signed integer overflow: 0 - -2147483648 cannot be represented in type 'int'`.
- **Cause:** `index0`/`index1` are `int` frame counts parsed from `INDEX nn mm:ss:ff` lines with no range
  check, so a hostile sheet makes the subtraction leave the `int` range.
- **Impact:** undefined behaviour in the arithmetic; the result only feeds a `double` track length, so no
  memory-safety effect.
- **Fix:** subtract in `int64_t` (or `double`), or bound the parsed index values.
