#define _GNU_SOURCE
#include <stdlib.h>
#include "id3.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	int fd = memfd_create("id3", 0);
	if (fd < 0)
		return -1;
	if (write(fd, data, size) != (ssize_t)size) {
		close(fd);
		return -1;
	}
	struct id3tag id3;
	id3_init(&id3);
	int rc = id3_read_tags(&id3, fd, ID3_V1 | ID3_V2);
	if (rc == 0) {
		for (int k = 0; k < NUM_ID3_KEYS; k++)
			free(id3_get_comment(&id3, k));
	}
	id3_free(&id3);
	close(fd);
	return rc ? -1 : 0;
}
