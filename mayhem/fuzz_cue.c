#include "cue.h"
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct cue_sheet *s = cue_parse((const char *)data, size);
	if (!s)
		return -1;
	for (size_t i = 0; i <= s->num_tracks + 1; i++)
		(void)cue_get_track(s, i);
	cue_free(s);
	return 0;
}
