# ape_read_tags(): `-((int)h->size)` overflows for sizes >= 2^31 (reached through the mpc plugin)

The same defect as `mayhem/fuzz_ape/known-findings/negated-int-min` (`ape.c:223`,
`lseek(fd, -((int)h->size), SEEK_CUR)`), hit here because `ip/mpc.c` reads APE tags with the same code.

- **Reproducer:** `reproducer.mpc`; `fuzz_ip_mpc-standalone reproducer.mpc` stops with UBSan
  `negation of -2147483648 cannot be represented in type 'int'`.
- **Cause, impact, fix:** see the `fuzz_ape` write-up (use `-(off_t)h->size`).
