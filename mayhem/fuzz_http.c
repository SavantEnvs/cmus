#include "http.h"
#include "keyval.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

/* Splits the input at its first NUL: the head is a URI for http_parse_uri, the tail is the raw
 * server response read back through http_get/http_read_body over a socketpair. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size > 32768)
		return -1;

	const uint8_t *nul = memchr(data, 0, size);
	size_t ulen = nul ? (size_t)(nul - data) : size;
	const uint8_t *resp = nul ? nul + 1 : data + size;
	size_t rlen = size - (resp - data);

	char *uri = malloc(ulen + 1);
	memcpy(uri, data, ulen);
	uri[ulen] = '\0';

	struct http_uri u;
	int parsed = http_parse_uri(uri, &u) == 0;
	free(uri);
	if (parsed)
		http_free_uri(&u);

	int sv[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0)
		return -1;
	if (write(sv[1], resp, rlen) != (ssize_t)rlen) {
		close(sv[0]);
		close(sv[1]);
		return -1;
	}
	shutdown(sv[1], SHUT_WR);

	struct http_get hg;
	memset(&hg, 0, sizeof(hg));
	hg.fd = sv[0];
	hg.uri.path = strdup("/stream");
	struct keyval headers[] = { { "Host", "example.org" }, { NULL, NULL } };
	int rc = http_get(&hg, headers, 1000);
	if (rc == 0) {
		size_t bsize;
		free(http_read_body(hg.fd, &bsize, 1000));
	}
	http_get_free(&hg);
	close(sv[0]);
	close(sv[1]);
	return rc == 0 ? 0 : -1;
}
