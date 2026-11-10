#include <stdarg.h>
/* Definitions of the few globals that the fuzzed cmus modules read but upstream defines in
 * ui_curses.c / options.c (which pull in the whole curses UI). Values are cmus' own defaults. */
char *charset = "UTF-8";
int using_utf8 = 1;
char *id3_default_charset = "ISO-8859-1";
char *clipped_text_internal = "...";

/* The UI's error line; nothing to show it to when fuzzing. */
void error_msg(const char *format, ...)
{
	(void)format;
}
