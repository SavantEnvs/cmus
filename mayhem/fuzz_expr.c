#include "expr.h"
#include "track_info.h"
#include "keyval.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	char *str = malloc(size + 1);
	memcpy(str, data, size);
	str[size] = '\0';

	struct expr *e = expr_parse(str);
	free(str);
	if (!e)
		return -1;

	struct track_info *ti = track_info_new("/music/Some Artist/song.flac");
	struct keyval *kv = keyvals_new(6);
	kv[0].key = strdup("artist");
	kv[0].val = strdup("Some Artist");
	kv[1].key = strdup("album");
	kv[1].val = strdup("Some Album");
	kv[2].key = strdup("title");
	kv[2].val = strdup("Some Song");
	kv[3].key = strdup("genre");
	kv[3].val = strdup("Rock");
	kv[4].key = strdup("date");
	kv[4].val = strdup("1999");
	kv[5].key = NULL;
	kv[5].val = NULL;
	track_info_set_comments(ti, kv);
	ti->duration = 215;
	ti->bitrate = 320000;

	(void)expr_eval(e, ti);
	(void)expr_get_match_type(e);
	(void)expr_is_harmless(e);

	track_info_unref(ti);
	expr_free(e);
	return 0;
}
