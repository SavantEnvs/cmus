# ape_read_tags(): `-((int)h->size)` overflows for sizes >= 2^31

- **Where:** `ape.c:223`, `lseek(fd, -((int)h->size), SEEK_CUR)`.
- **Reproducer:** `reproducer.bin` (an MPC file with a hostile APE footer); `fuzz_ape-standalone reproducer.bin`
  stops with UBSan `negation of -2147483648 cannot be represented in type 'int'`. It was found
  through `fuzz_ip_mpc`, which reads APE tags with the same code.
- **Cause:** the tag size is a file-controlled `uint32_t`; cast to `int` and negated, `0x80000000` is `INT_MIN`.
- **Impact:** undefined behaviour on a crafted tag (in practice a bogus seek offset that fails and rejects
  the tag); not a memory-safety bug.
- **Fix:** `lseek(fd, -(off_t)h->size, SEEK_CUR)`.
