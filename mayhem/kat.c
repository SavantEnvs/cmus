/* Known-answer probe for mayhem/test.sh: drives cmus' filter-expression, CUE, ID3, APE and WAV code
 * and prints what they computed, so the test can compare it to values worked out by hand. Built with
 * the project's normal flags against the objects its own Makefile produced. */
#define _GNU_SOURCE
#include "expr.h"
#include "track_info.h"
#include "keyval.h"
#include "cue.h"
#include "id3.h"
#include "ape.h"
#include "ip.h"
#include "sf.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int cmd_expr(const char *src)
{
	struct expr *e = expr_parse(src);
	if (!e) {
		puts("parse-error");
		return 0;
	}
	struct track_info *ti = track_info_new("/music/Some Artist/song.flac");
	struct keyval *kv = keyvals_new(5);
	kv[0].key = strdup("artist");
	kv[0].val = strdup("Some Artist");
	kv[1].key = strdup("title");
	kv[1].val = strdup("Some Song");
	kv[2].key = strdup("genre");
	kv[2].val = strdup("Rock");
	kv[3].key = strdup("date");
	kv[3].val = strdup("1999");
	kv[4].key = NULL;
	track_info_set_comments(ti, kv);
	ti->duration = 215;
	ti->bitrate = 320000;
	printf("%d\n", expr_eval(e, ti) ? 1 : 0);
	track_info_unref(ti);
	expr_free(e);
	return 0;
}

static int cmd_cue(const char *path)
{
	struct cue_sheet *s = cue_from_file(path);
	if (!s) {
		puts("cue-error");
		return 1;
	}
	printf("tracks=%zu album=%s performer=%s\n", s->num_tracks,
			s->meta.title ? s->meta.title : "-",
			s->meta.performer ? s->meta.performer : "-");
	for (size_t i = 0; i < s->num_tracks; i++) {
		struct cue_track *t = cue_get_track(s, i + 1);
		printf("track %zu title=%s offset=%.3f\n", t->number,
				t->meta.title ? t->meta.title : "-", t->offset);
	}
	cue_free(s);
	return 0;
}

static int cmd_id3(const char *path)
{
	int fd = open(path, O_RDONLY);
	struct id3tag id3;
	id3_init(&id3);
	int rc = id3_read_tags(&id3, fd, ID3_V1 | ID3_V2);
	if (rc) {
		puts("id3-error");
		return 1;
	}
	static const enum id3_key keys[] = { ID3_ARTIST, ID3_TITLE, ID3_ALBUM, ID3_DATE, ID3_TRACK, ID3_GENRE };
	for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
		char *v = id3_get_comment(&id3, keys[i]);
		printf("%s=%s\n", id3_key_names[keys[i]], v ? v : "");
		free(v);
	}
	id3_free(&id3);
	close(fd);
	return 0;
}

static int cmd_ape(const char *path)
{
	int fd = open(path, O_RDONLY);
	APETAG(ape);
	int rc = ape_read_tags(&ape, fd, 1);
	printf("items=%d\n", rc);
	char *val, *key;
	while ((key = ape_get_comment(&ape, &val))) {
		printf("%s=%s\n", key, val);
		free(key);
		free(val);
	}
	ape_free(&ape);
	close(fd);
	return 0;
}

static int cmd_wav(const char *path)
{
	struct input_plugin_data ip;
	memset(&ip, 0, sizeof(ip));
	ip.filename = (char *)path;
	ip.fd = open(path, O_RDONLY);
	if (ip_ops.open(&ip) != 0) {
		puts("wav-error");
		return 1;
	}
	char buf[512];
	size_t total = 0;
	unsigned sum = 0;
	int n;
	while ((n = ip_ops.read(&ip, buf, sizeof(buf))) > 0) {
		for (int i = 0; i < n; i++)
			sum = sum * 31 + (unsigned char)buf[i];
		total += n;
	}
	printf("rate=%d channels=%d bits=%d bytes=%zu sum=%u\n", sf_get_rate(ip.sf),
			sf_get_channels(ip.sf), sf_get_bits(ip.sf), total, sum);
	ip_ops.close(&ip);
	close(ip.fd);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 3)
		return 2;
	if (!strcmp(argv[1], "expr"))
		return cmd_expr(argv[2]);
	if (!strcmp(argv[1], "cue"))
		return cmd_cue(argv[2]);
	if (!strcmp(argv[1], "id3"))
		return cmd_id3(argv[2]);
	if (!strcmp(argv[1], "ape"))
		return cmd_ape(argv[2]);
	if (!strcmp(argv[1], "wav"))
		return cmd_wav(argv[2]);
	return 2;
}
