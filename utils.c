/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"


/* external globals */
extern struct time_info_data time_info;

/* local functions */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void prune_crlf(char *txt);

void clanlog(struct char_data *ch, const char *str, ...);

int get_outside_light()
//the outside light should correspond to DAWN_HOUR and DUSK_HOUR
{
  int exterior_light = 0;
  switch(time_info.hours) {
    case 0: exterior_light = -3; break;
    case 1: exterior_light = -4; break;
    case 2: exterior_light = -3; break;
    case 3: exterior_light = -2; break;
    case 4: exterior_light = -1; break;
    case 5: exterior_light = 0; break;
    case 6: exterior_light = 1; break;
    case 7: exterior_light = 2; break;
    case 8: exterior_light = 3; break;
    case 9: exterior_light = 4; break;
    case 10: exterior_light = 5; break;
    case 11: exterior_light = 6; break;
    case 12: exterior_light = 8; break;
    case 13: exterior_light = 10; break;
    case 14: exterior_light = 8; break;
    case 15: exterior_light = 6; break;
    case 16: exterior_light = 5; break;
    case 17: exterior_light = 4; break;
    case 18: exterior_light = 3; break;
    case 19: exterior_light = 2; break;
    case 20: exterior_light = 1; break;
    case 21: exterior_light = 0; break;
    case 22: exterior_light = -1; break;
    case 23: exterior_light = -2; break;
    default: exterior_light = 0; break;
  }
  if (weather_info.sky == SKY_CLOUDY) exterior_light -=1; /* considered fog */
  if (weather_info.sky == SKY_RAINING) exterior_light -=2; /* slightly more obscurity */
  if (weather_info.sky == SKY_LIGHTNING) exterior_light -=2;
  if (!IS_DAYTIME) exterior_light+=MOONLIGHT_SCALOR;
  return exterior_light;
}

int get_room_light(room_rnum room)
{ 
  if (room != NOWHERE)
    return MIN(MAX_ROOM_LIGHT, MAX(MIN_ROOM_LIGHT, (world[room].nat_light + world[room].mod_light + (OUTSIDER(room) ? get_outside_light() : DEFAULT_NORM_ROOM_LIGHT))));
  else return DEFAULT_NORM_ROOM_LIGHT;
} 
/* creates a random number in interval [from;to] */ 
int rand_number(int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
    log("SYSERR: rand_number() should be called with lowest, then highest. (%d, %d), not (%d, %d).", from, to, to, from);
  }

  /*
   * This should always be of the form:
   *
   *	((float)(to - from + 1) * rand() / (float)(RAND_MAX + from) + from);
   *
   * if you are using rand() due to historical non-randomness of the
   * lower bits in older implementations.  We always use circle_random()
   * though, which shouldn't have that problem. Mean and standard
   * deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken.
   */
  return ((circle_random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(1, size);

  return (sum);
}


/* Be wary of sign issues with this. */
int MIN(int a, int b)
{
  return (a < b ? a : b);
}

/* Be wary of sign issues with this. */
int MAX(int a, int b)
{
  return (a > b ? a : b);
}


char *CAP(char *txt)
{
  *txt = UPPER(*txt);
  return (txt);
}


#if !defined(HAVE_STRLCPY)
/*
 * A 'strlcpy' function in the same fashion as 'strdup' below.
 *
 * This copies up to totalsize - 1 bytes from the source string, placing
 * them and a trailing NUL into the destination string.
 *
 * Returns the total length of the string it tried to copy, not including
 * the trailing NUL.  So a '>= totalsize' test says it was truncated.
 * (Note that you may have _expected_ truncation because you only wanted
 * a few characters from the source string.)
 */
size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
  strncpy(dest, source, totalsize - 1);	/* strncpy: OK (we must assume 'totalsize' is correct) */
  dest[totalsize - 1] = '\0';
  return strlen(source);
}
#endif


#if !defined(HAVE_STRDUP)
/* Create a duplicate of a string */
char *strdup(const char *source)
{
  char *new_z;

  CREATE(new_z, char, strlen(source) + 1);
  return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif


/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}


#ifndef str_cmp
/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; arg1[i] || arg2[i]; i++)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif


#ifndef strn_cmp
/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif


/* log a death trap hit */
void log_death_trap(struct char_data *ch)
{
  mudlog(BRF, LVL_SAINT, TRUE, "%s hit a Death Trap at [%5d]", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)));
 }

/*Average Dam function - Seymour ripped from odin*/

int get_weapon_dam(struct obj_data *obj)
{
  int dam = 0;
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
dam = (int)((float)(GET_OBJ_VAL(obj, 1)*((float)(1+GET_OBJ_VAL(obj, 2))/2.0)));
  return dam;
}


/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_vlog(const char *format, va_list args)
{
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}


/* So mudlog() can use the same function. */
void basic_mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  basic_mud_vlog(format, args);
  va_end(args);
}


/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
    return;	/* eh, oh well. */

  if (file) {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }

  if (level < 0)
    return;

  strcpy(buf, "[ ");	/* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n");	/* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_LEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "%s%s%s", CCCYN(i->character, C_NRM), buf, CCNRM(i->character, C_NRM));
  }
}



/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
  size_t len = 0;
  int nlen;
  long nr;

  *result = '\0';

  for (nr = 0; bitvector && len < reslen; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
      nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
      if (len + nlen >= reslen || nlen < 0)
        break;
      len += nlen;
    }

    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    len = strlcpy(result, "NOBITS ", reslen);

  return (len);
}


size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

  now.month = -1;
  now.year = -1;

  return (&now);
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35;	/* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17;	/* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return (&now);
}


time_t mud_time_to_secs(struct time_info_data *now)
{
  time_t when = 0;

  when += now->year  * SECS_PER_MUD_YEAR;
  when += now->month * SECS_PER_MUD_MONTH;
  when += now->day   * SECS_PER_MUD_DAY;
  when += now->hours * SECS_PER_MUD_HOUR;

  return (time(NULL) - when);
}

struct time_info_data *age(struct char_data *ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->player.time.birth);
  switch (GET_RACE(ch))
  {
          case RACE_DWARF:
                  player_age.year += 58;
                  break;

          case RACE_ELF:
                  player_age.year += 118;
                  break;

          case RACE_GNOME:
                  player_age.year += 80;
                  break;

	  case RACE_HALFLING:
		  player_age.year += 100;
		  break;

	  case RACE_MINOTAUR:
		  player_age.year += 30;

	  case RACE_PIXIE:
		  player_age.year += 67;

  	  case RACE_ULDRA:
		  player_age.year += 24;

	  case RACE_OGRE:
		  player_age.year += 145;

	  case RACE_VAMPIRE:
		  player_age.year += 1000;

	  case RACE_SHINTARI:
		  player_age.year += 30;

	  case RACE_KARADAL:
		  player_age.year += 24;

	  case RACE_VISRAEL:
		  player_age.year += 98;

          case RACE_HUMAN:
          default:
                  player_age.year += 18;
                  break;
  }

  return (&player_age);
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master == NULL) {
    core_dump();
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
    act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT);
    if (affected_by_spell(ch, SPELL_CHARM))
      affect_from_char(ch, SPELL_CHARM);
  } else {
    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
    act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
  }

  if (ch->master->followers->follower == ch) {	/* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  } else {			/* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next);

    j = k->next;
    k->next = j->next;
    free(j);
  }

  ch->master = NULL;
  REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP);
}


int num_followers_charmed(struct char_data *ch)
{
  struct follow_type *lackey;
  int total = 0;

  for (lackey = ch->followers; lackey; lackey = lackey->next)
    if (AFF_FLAGGED(lackey->follower, AFF_CHARM) && lackey->follower->master == ch)
      total++;

  return (total);
}


/* Called when a character that follows/is followed dies */
void die_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) {
    j = k->next;
    stop_follower(k->follower);
  }
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data *ch, struct char_data *leader)
{
  struct follow_type *k;

  if (ch->master) {
    core_dump();
    return;
  }

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (CAN_SEE(leader, ch))
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}


/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file. Buffer given must
 * be at least READ_SIZE (256) characters large.
 */
int get_line(FILE *fl, char *buf)
{
  char temp[READ_SIZE];
  int lines = 0;
  int sl;

  do {
    if (!fgets(temp, READ_SIZE, fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  /* Last line of file doesn't always have a \n, but it should. */
  sl = strlen(temp);
  while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
    temp[--sl] = '\0';

  strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
  return (lines);
}


int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
  const char *prefix, *middle, *suffix;
  char name[PATH_MAX], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
		orig_name, filename);
    return (0);
  }

  switch (mode) {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case ALIAS_FILE:
    prefix = LIB_PLRALIAS;
    suffix = SUF_ALIAS;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  default:
    return (0);
  }

  strlcpy(name, orig_name, sizeof(name));
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  snprintf(filename, fbufsize, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
  return (1);
}


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return (i);
}

/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
extern FILE *player_fl;
void core_dump_real(const char *who, int line)
{
  log("SYSERR: Assertion failed at %s:%d!", who, line);

#if 0	/* By default, let's not litter. */
#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise...make very sure. */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);
  fflush(player_fl);
  /* Everything, just in case, for the systems that support it. */
  fflush(NULL);

  /*
   * Kill the child so the debugger or script doesn't think the MUD
   * crashed.  The 'autorun' script would otherwise run it again.
   */
  if (fork() == 0)
    abort();
#endif
#endif
}

int can_see_room(struct char_data *ch, room_rnum room)
{
  if (!VALID_ROOM_RNUM(room)) {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)) return TRUE;
  /* rooms that are flagged 'DARK' are ALWAYS dark regardless of room lighting */
  if (ROOM_FLAGGED(room, ROOM_DARK) || IS_BLIND(ch)) return FALSE;
  if (ROOM_FLAGGED(room, ROOM_LIT) || (SECT(room) == SECT_CITY)) return TRUE;
  if (get_room_light(room) >= DEFAULT_NORM_ROOM_LIGHT) return TRUE;
  if ((get_room_light(room) >= MIN_ROOM_BRIGHTNESS_FOR_CLOAK) && HAS_INFRAVISION(ch)) return TRUE;
  if (AFF_FLAGGED(ch, AFF_BAT_SONAR)) return TRUE;
  return FALSE;
}

/*
 * Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.
 */
int room_is_dark(room_rnum room)
{
  if (!VALID_ROOM_RNUM(room)) {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  if (world[room].light)
    return (FALSE);

  if (ROOM_FLAGGED(room, ROOM_DARK))
    return (TRUE);

  if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY)
    return (FALSE);

  if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
    return (TRUE);

  return (FALSE);
}


int get_ave_dam(struct obj_data *obj)
{
int ave_dam, fdie, sdie ;
fdie = GET_OBJ_VAL(obj, 1);
sdie = GET_OBJ_VAL(obj, 2);

ave_dam = (fdie * sdie  + fdie) / 2;

return ave_dam;
}


void infochan(const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;

  if (str == NULL)
    return;     /* eh, oh well. */

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    send_to_char(i->character, "%s[INFO]%s%s", CCBCN(i->character, C_NRM), buf, CCNRM(i->character, C_NRM));
  }
}

int convert_damage_type_to_resistance(int attacktype)
{
/*   switch (attacktype) {
     case TYPE_HIT:
     case TYPE_BLUDGEON:
     case TYPE_CRUSH:
     case TYPE_POUND:
     case TYPE_PUNCH:
     case TYPE_KICK:
     case SPELL_CLENCHED_FIST:
     case SPELL_EARTHQUAKE:
     case SPELL_FLAILING_FISTS:
     case SKILL_BASH:
     case SKILL_KICK:
     case SKILL_SHIELDPUNCH:
     case SKILL_SHIELDRUSH:
     case SKILL_WP_UNARMED:
     case SKILL_WP_BLUDGEON:
       return RESIST_TYPE_BLDG;
     case TYPE_SLASH:
     case TYPE_WHIP:
     case TYPE_CLAW:
     case TYPE_MAUL:
     case TYPE_THRASH:
     case SKILL_WP_SLASH:
       return RESIST_TYPE_SLSH;
     case TYPE_PIERCE:
     case TYPE_BITE:
     case TYPE_BLAST:
     case TYPE_STAB:
     case SPELL_BOLT_OF_STEEL:
     case SPELL_HAIL_OF_ARROWS:
     case SPELL_HORNETS_DART:
     case SPELL_MAGIC_MISSILE:
     case SKILL_WP_PIERCE:
       return RESIST_TYPE_PIER;

     case TYPE_FIRE:
     case SPELL_BREATH_FIRE:
     case SPELL_BURNING_HANDS:
     case SPELL_FIREBALL:
     case SPELL_FIREBOLT:
     case SPELL_FLAMESTRIKE:
     case SPELL_FLAMING_ARROW:
       return RESIST_TYPE_FIRE;
     case TYPE_ELEC:
     case SPELL_BALL_LIGHTNING:
     case SPELL_CALL_LIGHTNING:
     case SPELL_CHAIN_LIGHTNING:
     case SPELL_LIGHTNING_BOLT:
     case SPELL_BREATH_LIGHTNING:
     case SPELL_SHOCKING_GRASP:
       return RESIST_TYPE_ELEC;
     case TYPE_COLD:
     case SPELL_BREATH_FROST:
     case SPELL_CHILL_TOUCH:
     case SPELL_ICE_LANCE:
     case SPELL_ICE_STORM:
       return RESIST_TYPE_COLD;
     case TYPE_STING:
     case TYPE_POIS:
     case SPELL_POISON:
     case SKILL_ENVENOM:
       return RESIST_TYPE_POIS;
     case TYPE_SONC:
     case SPELL_ROAR:
     case SPELL_SONIC_BLAST:
     case SPELL_WAIL_OF_THE_BANSHEE:
       return RESIST_TYPE_SONC;
     case TYPE_ACID:
     case SPELL_ACID_ARROW:
     case SPELL_BREATH_ACID:
       return RESIST_TYPE_ACID;

     case TYPE_GAS:
     case SPELL_BREATH_GAS:
     case SPELL_ASPHYXIATE:
       return RESIST_TYPE_GAS;
     case TYPE_LGHT:
     case SPELL_COLOR_SPRAY:
     case SPELL_MOON_MOTE:
     case SPELL_PRISMATIC_SPRAY:
     case SPELL_SUNRAY:
     case SPELL_SUNBURST:
     case SPELL_SEARING_ORB:
       return RESIST_TYPE_LGHT;
     case TYPE_DIVN:
     case SPELL_DISPEL_EVIL:
     case SPELL_DISPEL_GOOD:
     case SPELL_SMITE_EVIL:
     case SPELL_SMITE_GOOD:
     case SPELL_HOLY_WORD:
     case SPELL_UNHOLY_WORD:
     case SPELL_HARM:
     case SPELL_CAUSE_MINOR:
     case SPELL_CAUSE_MAJOR:
     case SPELL_PROTECTION_FROM_EVIL:
     case SPELL_PROTECTION_FROM_GOOD:
     case SPELL_SHIELD_AGAINST_EVIL:
     case SPELL_SHIELD_AGAINST_GOOD:
     case SKILL_TURNING:
       return RESIST_TYPE_DIVN;
     case SPELL_SUMMON_LESSER:
     case SPELL_SUMMON_GREATER:
     case TYPE_SUMN:
       return RESIST_TYPE_SUMN;
     case TYPE_LIFE:
     case SPELL_LIFE_LEECH:
     case SPELL_VAMPIRIC_TOUCH:
     case SPELL_ENERGY_DRAIN:
 return RESIST_TYPE_LIFE;
     case TYPE_FEAR:
     case SPELL_SPOOK:
       return RESIST_TYPE_FEAR;
     case SKILL_WP_SPECIAL:
     case SPELL_REFLECT_DAMAGE:
     case TYPE_MISC:
       return RESIST_TYPE_MISC;
     default: return RESIST_TYPE_MISC;*/
//   }
}

