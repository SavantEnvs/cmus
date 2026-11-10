/* Generic harness for cmus input plugins (ip/*.c). Each plugin exports the same `ip_ops`, so one
 * binary is built per plugin by linking this file with that plugin's object. The plugin is driven
 * the way input.c drives it: open, read_comments, metadata getters, a bounded decode, seek. */
#define _GNU_SOURCE
#include "ip.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	int fd = memfd_create("ip", 0);
	if (fd < 0)
		return -1;
	if (write(fd, data, size) != (ssize_t)size) {
		close(fd);
		return -1;
	}
	lseek(fd, 0, SEEK_SET);

	struct input_plugin_data ip;
	memset(&ip, 0, sizeof(ip));
	ip.filename = "fuzz";
	ip.fd = fd;

	if (ip_ops.open(&ip) != 0) {
		close(fd);
		return -1;
	}

	struct keyval *comments = NULL;
	if (ip_ops.read_comments && ip_ops.read_comments(&ip, &comments) == 0 && comments)
		keyvals_free(comments);
	if (ip_ops.duration)
		(void)ip_ops.duration(&ip);
	if (ip_ops.bitrate)
		(void)ip_ops.bitrate(&ip);
	if (ip_ops.bitrate_current)
		(void)ip_ops.bitrate_current(&ip);
	if (ip_ops.codec)
		free(ip_ops.codec(&ip));
	if (ip_ops.codec_profile)
		free(ip_ops.codec_profile(&ip));

	char buf[4096];
	size_t total = 0;
	int n;
	while (total < (4u << 20) && (n = ip_ops.read(&ip, buf, sizeof(buf))) > 0)
		total += n;

	if (ip_ops.seek && ip_ops.seek(&ip, 0.5) == 0) {
		for (int i = 0; i < 8 && (n = ip_ops.read(&ip, buf, sizeof(buf))) > 0; i++)
			;
	}

	ip_ops.close(&ip);
	close(fd);
	return 0;
}
