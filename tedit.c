/*
 * Originally written by: Michael Scott -- Manx.
 * Last known e-mail address: scottm@workcomm.net
 *
 * XXX: This needs Oasis-ifying.
 *///

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "tedit.h"

extern const char *credits;
extern const char *news;
extern const char *motd;
extern const char *imotd;
extern const char *help;
extern const char *info;
extern const char *background;
extern const char *handbook;
extern const char *policies;
extern const char *zonelist;

void tedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  FILE *fl;
  char *storage = (char *)d->olc;

  if (!storage)
    terminator = STRINGADD_ABORT;

  switch (terminator) {
  case STRINGADD_SAVE:
    if (!(fl = fopen(storage, "w")))
      mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Can't write file '%s'.", storage);
    else {
      if (*d->str) {
        strip_cr(*d->str);
        fputs(*d->str, fl);
      }
      fclose(fl);
      mudlog(CMP, LVL_DEITY, TRUE, "OLC: %s saves '%s'.", GET_NAME(d->character), storage);
      write_to_output(d, "Saved.\r\n");
    }
    break;
  case STRINGADD_ABORT:
    write_to_output(d, "Edit aborted.\r\n");
    act("$n stops editing some scrolls.", TRUE, d->character, 0, 0, TO_ROOM);
    break;
  default:
    log("SYSERR: tedit_string_cleanup: Unknown terminator status.");
    break;
  }

  /* Common cleanup code. */
  if (d->olc) {
    free(d->olc);
    d->olc = NULL;
  }
  STATE(d) = CON_PLAYING;
}

ACMD(do_tedit)
{
  int l, i;
  char field[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  char *backstr = NULL;
   
  struct {
    char *cmd;
    char level;
    const char **buffer;
    int  size;
    char *filename;
  } fields[] = {
	/* edit the lvls to your own needs */
	{ "credits",	LVL_IMPL,	&credits,	2400,	CREDITS_FILE},
	{ "news",	LVL_GOD,	&news,		8192,	NEWS_FILE},
	{ "motd",	LVL_GOD,	&motd,		2400,	MOTD_FILE},
	{ "imotd",	LVL_IMPL,	&imotd,		2400,	IMOTD_FILE},
	{ "help",       LVL_GOD,	&help,		2400,	HELP_PAGE_FILE},
	{ "info",	LVL_GOD,	&info,		8192,	INFO_FILE},
	{ "background",	LVL_IMPL,	&background,	8192,	BACKGROUND_FILE},
	{ "handbook",   LVL_IMPL,	&handbook,	8192,   HANDBOOK_FILE},
	{ "policies",	LVL_IMPL,	&policies,	8192,	POLICIES_FILE},
    { "zone info",  LVL_GOD,    &zonelist,  8192,   ZONELIST_FILE},
	{ "\n",		0,		NULL,		0,	NULL }
  };

  if (ch->desc == NULL)
    return;
   
  half_chop(argument, field, buf);

  if (!*field) {
    strcpy(buf, "Files available to be edited:\r\n");
    i = 1;
    for (l = 0; *fields[l].cmd != '\n'; l++) {
      if (GET_LEVEL(ch) >= fields[l].level) {
	sprintf(buf, "%s%-11.11s", buf, fields[l].cmd);
	if (!(i % 7))
	  strcat(buf, "\r\n");
	i++;
      }
    }
    if (--i % 7)
      strcat(buf, "\r\n");
    if (i == 0)
      strcat(buf, "None.\r\n");
    send_to_char(ch, "%s", buf);
    return;
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;
   
  if (*fields[l].cmd == '\n') {
    send_to_char(ch, "Invalid text editor option.\r\n");
    return;
  }
   
  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return;
  }

  /* set up editor stats */
  clear_screen(ch->desc);
  send_editor_help(ch->desc);
  send_to_char(ch, "Edit file below:\r\n\r\n");

  if (*fields[l].buffer) {
    send_to_char(ch, "%s", *fields[l].buffer);
    backstr = strdup(*fields[l].buffer);
  }

  ch->desc->olc = strdup(fields[l].filename);
  string_write(ch->desc, (char **)fields[l].buffer, fields[l].size, 0, backstr);

  act("$n begins editing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
  STATE(ch->desc) = CON_TEDIT;
}
