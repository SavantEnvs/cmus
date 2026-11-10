#define _GNU_SOURCE
#include <stdlib.h>
#include "ape.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	int fd = memfd_create("ape", 0);
	if (fd < 0)
		return -1;
	if (write(fd, data, size) != (ssize_t)size) {
		close(fd);
		return -1;
	}
	APETAG(ape);
	int rc = ape_read_tags(&ape, fd, 1);
	if (rc > 0) {
		char *val;
		char *key;
		while ((key = ape_get_comment(&ape, &val))) {
			free(key);
			free(val);
		}
	}
	ape_free(&ape);
	close(fd);
	return rc < 0 ? -1 : 0;
}
