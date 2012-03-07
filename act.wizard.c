/* 
************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                           ase           *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "clan.h"
//for goto/at real_zone_by_thing
#include "genzon.h"

/* For NO_PURGE */
struct obj_data *j, *l;

/*   external vars  */
extern FILE *player_fl;
extern struct attack_hit_type attack_hit_text[];
extern char *class_abbrevs[];
extern char *race_abbrevs[];
extern time_t boot_time;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern int top_of_p_table;
extern struct clan_type *clan_info;
extern int cnum;
//goto_at - mak
extern struct zone_data *zone_table;

/* for chars */
extern const char *pc_class_types[];
extern const char *pc_race_types[];
extern const char *mob_race_types[];
extern const char *mob_class_types[];

/* extern functions */
extern const char *sizes[];
int level_exp(int chclass, int level);
void show_shops(struct char_data *ch, char *value);
void hcontrol_list_houses(struct char_data *ch);
void do_start(struct char_data *ch);
void appear(struct char_data *ch);
void reset_zone(zone_rnum zone);
void roll_real_abils(struct char_data *ch);
int parse_full_class(char *arg);
int parse_race(char arg);
void run_autowiz(void);
int save_all(void);
struct char_data *find_char(int n);

/* local functions */
int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
void perform_immort_invis(struct char_data *ch, int level);
ACMD(do_echo);
ACMD(do_send);
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
void do_stat_room(struct char_data *ch);
void do_stat_object(struct char_data *ch, struct obj_data *j);
void do_stat_character(struct char_data *ch, struct char_data *k);
ACMD(do_stat);
ACMD(do_shutdown);
void stop_snooping(struct char_data *ch);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
void perform_immort_vis(struct char_data *ch);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone);
ACMD(do_show);
ACMD(do_set);
void snoop_check(struct char_data *ch);
ACMD(do_saveall);
ACMD(do_peace);
ACMD(do_seize);
ACMD(do_jail);
ACMD(do_release);
ACMD(do_impnet);
ACMD(do_godnet);
ACMD(do_deitynet);
ACMD(do_countnet);
ACMD(do_liegenet);


int find_attack_type(char *arg)
{
  int nr;

  for (nr=0; nr < NUM_ATTACK_TYPES; nr++) {
    if (is_abbrev(arg, attack_hit_text[nr].singular))
      break;
  }

  return (nr);
}


ACMD(do_jail)
{

/* 
 * Wanna move the guy to 4298, freeze him, and set his startroom as 505.
*/

 struct char_data *victim;
 char buf[MAX_INPUT_LENGTH];
 room_rnum jail;

 skip_spaces(&argument);
 one_argument(argument, buf);

 /* Does the JAIL exist in room 4298? */
 if ((jail = find_target_room(ch, "4298")) == NOWHERE)
	{
	send_to_char(ch, "Now where did that jail go? (Psst.. go get an IMP :))\r\n");
	return;
	}

/* No arg? */
 if (!*buf)
    {
    send_to_char(ch, "Jail whom?\r\n");
    return;
    }

/* Does the vict exist? */
 if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
	{
    send_to_char(ch, "%s", NOPERSON);
    return;
	}

/* Is vict a NPC? */
 if (IS_NPC(victim))
	{
  send_to_char(ch, "That's just dumb.\r\n");
  return;
    }

/* Jailing yourself eh? */
  if (victim == ch)
	{
	 send_to_char(ch, "That doesn't make much sense, does it?\r\n");
     return;
    }

/* Jailing someone of higher level, or a fellow IMP ? */ 
  if ((((GET_LEVEL(ch) < GET_LEVEL(victim))) || (GET_LEVEL(victim) == LVL_IMPL)) && !IS_NPC(victim))
	 {
 	 send_to_char(ch, "Try and jail someone your own size.\r\n");
     return;
     }

    /* Move the char */
    send_to_char(victim, "Uh oh.. Whatever you did, it WAS NOT a good idea.\r\n");
	char_from_room(victim);
 	char_to_room(victim, jail);
    look_at_room(IN_ROOM(victim), victim, 0);

	send_to_char(ch, "%s has been jailed.\r\n", victim->player.name);

    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s was jailed by %s.", GET_NAME(victim), GET_NAME(ch));

    /* Set the loadroom as JAIL */
	SET_BIT(PLR_FLAGS(victim), PLR_LOADROOM); 
	GET_LOADROOM(victim) = 4298;

    /* No title them */
	set_title(victim, "* is in JAIL.");
    if (!PLR_FLAGGED(victim, PLR_NOTITLE))
    SET_BIT(PLR_FLAGS(victim), PLR_NOTITLE);
    
	/* Finally, OFFICIALY jail his ass */
    SET_BIT(PLR_FLAGS(victim), PLR_JAIL);
	}

ACMD(do_release)
{
 struct char_data *victim;
 char buf[MAX_INPUT_LENGTH];
 room_rnum jail, return_room;
 

 skip_spaces(&argument);
 one_argument(argument, buf);

 /* Does the JAIL exist in room 4298? */
 if ((jail = find_target_room(ch, "4298")) == NOWHERE)
	{
	send_to_char(ch, "Now where did that jail go? (Psst.. go get an IMP :))\r\n");
	return;
}

  if ((return_room = find_target_room(ch, "10103")) == NOWHERE)
	{
	send_to_char(ch, "Now where did that return room go? (Psst.. go get an IMP :))\r\n");
	return;
	}
   
   /* No arg? */
 if (!*buf)
    {
    send_to_char(ch, "Free whom?\r\n");
    return;
    }

	/* Does the vict exist? */
 if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
	{
    send_to_char(ch, "%s", NOPERSON);
    return;
	}
/* Is vict a NPC? */
 if (IS_NPC(victim))
	{
  send_to_char(ch, "That's just dumb.\r\n");
  return;
    }

/* Vict in Jail? MAKE THIS A FLAG.. LAZY FOR NOW */
  if (!PLR_FLAGGED(victim, PLR_JAIL))
	{
	send_to_char(ch, "That person is not in jail.\r\n");
	return;
	}

    /* Move the char */
    send_to_char(victim, "You have been released .. now be good.\r\n");
	char_from_room(victim);
 	char_to_room(victim, return_room);
    look_at_room(IN_ROOM(victim), victim, 0);

	send_to_char(ch, "%s has been released from jail.\r\n", victim->player.name);

    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s was released from jail by %s.", GET_NAME(victim), GET_NAME(ch));

    /* Set new loadroom to the norm start room */
	GET_LOADROOM(victim) = 10103;
	REMOVE_BIT(PLR_FLAGS(victim), PLR_LOADROOM);

    /* Fix his title */
	set_title(victim, "* has been released from JAIL.");
    if (PLR_FLAGGED(victim, PLR_NOTITLE))
    REMOVE_BIT(PLR_FLAGS(victim), PLR_NOTITLE);

	/* Ok, you can let him out ... */
    REMOVE_BIT(PLR_FLAGS(victim), PLR_JAIL);
}

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes.. but what?\r\n");
  else {
    char buf[MAX_INPUT_LENGTH + 4];

    if (subcmd == SCMD_EMOTE)
      snprintf(buf, sizeof(buf), "$n %s", argument);
    else
      strlcpy(buf, argument, sizeof(buf));

    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    if (!IS_NPC(ch)) {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", OK);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
  }
}


ACMD(do_send)
{
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char(ch, "Send what to who?\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "%s", NOPERSON);
    return;
  }
  send_to_char(vict, "%s\r\n", buf);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "Sent.\r\n");
  else
    send_to_char(ch, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data *ch, char *rawroomstr)
{
  room_rnum location = NOWHERE;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char(ch, "You must supply a room number or name.\r\n");
    return (NOWHERE);
  }

  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    if ((location = real_room((room_vnum)atoi(roomstr))) == NOWHERE) {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (NOWHERE);
    }
  } else {
    struct char_data *target_mob;
    struct obj_data *target_obj;
    char *mobobjstr = roomstr;
    int num;

    num = get_number(&mobobjstr);
    if ((target_mob = get_char_vis(ch, mobobjstr, &num, FIND_CHAR_WORLD)) != NULL) {
      if ((location = IN_ROOM(target_mob)) == NOWHERE) {
        send_to_char(ch, "That character is currently lost.\r\n");
        return (NOWHERE);
      }
    } else if ((target_obj = get_obj_vis(ch, mobobjstr, &num)) != NULL) {
      if (IN_ROOM(target_obj) != NOWHERE)
        location = IN_ROOM(target_obj);
      else if (target_obj->carried_by && IN_ROOM(target_obj->carried_by) != NOWHERE)
        location = IN_ROOM(target_obj->carried_by);
      else if (target_obj->worn_by && IN_ROOM(target_obj->worn_by) != NOWHERE)
        location = IN_ROOM(target_obj->worn_by);

      if (location == NOWHERE) {
        send_to_char(ch, "That object is currently not in a room.\r\n");
        return (NOWHERE);
      }
    }

    if (location == NOWHERE) {
      send_to_char(ch, "Nothing exists by that name.\r\n");
      return (NOWHERE);
    }
  }

  
   
  /* a location has been found -- if you're >= GRGOD, no restrictions. */
  if (GET_LEVEL(ch) >= LVL_GOD)
    {
    if (GET_LEVEL(ch) == LVL_GOD)
    {
     if (ROOM_FLAGGED(location, ROOM_IMPROOM))
     {
      send_to_char(ch, "You dare not disturb the Implementors!\r\n"); 
     return (NOWHERE);
     }
     else
     return (location);
    }
    else
    return (location);
 }
 /* irrelevant because of lin/max lvl rooms - mak 8.21.05 - reinstated 2.9.06 */  
//  if (ROOM_FLAGGED(location, ROOM_GODROOM))
//    send_to_char(ch, "You are not godly enough to use that room!\r\n");
   if (ROOM_FLAGGED(location, ROOM_IMPROOM))
   send_to_char(ch, "You dare not disturb the Implementors!\r\n"); 
  else if (ROOM_FLAGGED(location, ROOM_PRIVATE) && world[location].people && world[location].people->next_in_room)
    send_to_char(ch, "There's a private conversation going on in that room.\r\n");
  else if (ROOM_FLAGGED(location, ROOM_HOUSE) && !House_can_enter(ch, GET_ROOM_VNUM(location)))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else
    return (location);

  return (NOWHERE);
}



ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  room_rnum location, original_loc;
  room_vnum vnumloc;
  zone_rnum rzone;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char(ch, "You must supply a room number or a name.\r\n");
    return;
  }

  if (!*command) {
    send_to_char(ch, "What do you want to do there?\r\n");
    return;
  }

  if ((location = find_target_room(ch, buf)) == NOWHERE)
    return;

  /* a location has been found. */
  /* security - mak */
   vnumloc = GET_ROOM_VNUM(location);
   rzone = real_zone_by_thing(location);
   if (GET_LEVEL(ch) <= LVL_DEITY ){
        mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s did %s AT  %d, zone %d (OLC %d)",  
            GET_NAME(ch), command, vnumloc, rzone, GET_OLC_ZONE(ch));
        }
  
  original_loc = IN_ROOM(ch);
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);
  
  /* check if the char is still there */
  if (IN_ROOM(ch) == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}


ACMD(do_goto)
{
  struct char_data *i;
  char buf[MAX_STRING_LENGTH];
  room_rnum location;
  room_vnum vnumloc;
//  extern zone_vnum zone;
  zone_rnum rzone;

  if ((location = find_target_room(ch, argument)) == NOWHERE)
    return;
    
/*security - mak */
   vnumloc = GET_ROOM_VNUM(location);
   rzone = real_zone_by_thing(location);
//   zone = (zone_table[rzone].number);
   if (GET_LEVEL(ch) <= LVL_DEITY ){
        mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s GOTOed %d, zone %d (OLC %d)",
		   GET_NAME(ch), vnumloc, rzone, GET_OLC_ZONE(ch));        
        }
  
/* Start of IMM poofouts */

if (isname(ch->player.name, "Frenzy"))  
snprintf(buf, sizeof(buf), "\r\nIn a %sflash%s of %sb%sl%su%se%s a%sn%sd%s w%sh%si%st%se%s light Frenzy disappears ...\r\n", CCBYL(ch, C_SPR), CCNRM(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Seymour")) 
snprintf(buf, sizeof(buf), "\r\nWithout warning the ground beneath your feet parts...\r\nThe %sf%si%sr%se%ss%s of Hell leap out toward you and slowly Seymour recedes into the depths and returns from whence he came.\r\n", CCBRD(ch, C_SPR), CCBYL(ch, C_SPR), CCBRD(ch, C_SPR), CCBYL(ch, C_SPR), CCBRD(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Artek")) 
snprintf(buf, sizeof(buf), "\r\nWith a sigh, Artek turns around and fades into the %ss%sh%sa%sd%so%sw%ss%s dancing around you.  Leaving the wind to calm down and the sky to lighten.\r\n", CCWHT(ch, C_SPR), CCCYN(ch, C_SPR), CCWHT(ch, C_SPR), CCCYN(ch, C_SPR), CCWHT(ch, C_SPR), CCCYN(ch, C_SPR), CCWHT(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Makhno"))
snprintf(buf, sizeof(buf), "\r\nMakhno's eyes glaze over, and his flesh turns a fungal %swhite%s.\r\nHis head swells into a giant puffball, while his body grows \r\ninto a dry and brittle stalk. Not sure what to do, you tap him \r\non the shoulder to see if he is still conscious. With your tap, \r\nhis body crumbles. The head cracks open, releasing a cloud \r\nof black spores which are carried away on the wind.\r\n", CCWHT(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Valek"))
snprintf(buf, sizeof(buf), "\r\nWith a quick scent of a %sskank%s in the air. Valek searches for another Spice Girl to dispatch.\r\n", CCBRD(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Chasm"))
snprintf(buf, sizeof(buf), "\r\nChasm sinks back into the %shumus%s.\r\n", CCRED(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Taggert"))
snprintf(buf, sizeof(buf), "\r\nTaggert picks up his buttnugget and goes home. Screw you, hippie.\r\n");
else
snprintf(buf, sizeof(buf), "%s disappears in a puff of smoke.\n", GET_NAME(ch));  /* Grab char name */

  
/* End of IMM poofouts */


  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
  {
    	  if (IN_ROOM(i) == IN_ROOM(ch))
		  {
	  if (CAN_SEE(i, ch) && (ch != i))
	  send_to_char(i, buf);
		  }
  }


  char_from_room(ch);
  char_to_room(ch, location);
   
/* Start of IMM poofins */

if (isname(ch->player.name, "Frenzy")) 
snprintf(buf, sizeof(buf), "\r\nOut of a %sflash%s of %sb%sl%su%se%s a%sn%sd%s w%sh%si%st%se%s light Frenzy appears, he %ssmiles%s warmly and looks around ...\r\n", CCBYL(ch, C_SPR), CCNRM(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCBBL(ch, C_SPR), CCWHT(ch, C_SPR), CCNRM(ch, C_SPR), CCBYL(ch, C_SPR), CCNRM(ch, C_SPR));
else if (isname(ch->player.name, "Seymour")) 
snprintf(buf, sizeof(buf), "\r\nAs the sky darkens, the ground begins to tremble...\r\n%sFLAMES%s erupt from the depths of Hell casting brimstone around the area, Seymour has come for you!\r\n", CCBRD(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Artek")) 
snprintf(buf, sizeof(buf), "\r\nAs the sky begins to darken, the wind howls fiercly.  Artek emerges from the %ss%sh%sa%sd%so%sw%ss%s that dance wildly around you.\r\n", CCWHT(ch, C_SPR), CCCYN(ch, C_SPR), CCWHT(ch, C_SPR), CCCYN(ch, C_SPR), CCWHT(ch, C_SPR), CCCYN(ch, C_SPR), CCWHT(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Makhno"))
snprintf(buf, sizeof(buf), "\r\nThe fertile smell of decay wafts into the room. \r\n%sFuzzy white mycelia%s begin to spread across the ground, \r\nfeeding on rancid flesh. %sWhite mold-like fuzz%s grows \r\nparticularly thick on a corpse directly in front of you. \r\nA stalk shoots up from the fuzz with amazing speed, \r\ngrowing into the shape of Makhno.\r\n", CCGRY(ch, C_SPR), CCNRM(ch, C_SPR), CCWHT(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Valek"))
snprintf(buf, sizeof(buf), "\r\nWith a malicious smirk, Valek %scalmly disembowels%s another member of the Spice Girls!\r\n", CCBRD(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Chasm"))
snprintf(buf, sizeof(buf), "\r\nChasm does a few magic tricks, then %smoonwalks%s while raising the roof with a dagger in his mouth.\r\n", CCWHT(ch, C_SPR), CCNRM(ch, C_SPR));
else if(isname(ch->player.name, "Taggert"))
snprintf(buf, sizeof(buf), "\r\nTaggert casually tosses a %sflaming buttnugget%s into the room.\r\n", CCBRD(ch, C_SPR), CCNRM(ch, C_SPR));
else snprintf(buf, sizeof(buf), "%s appears with an ear-splitting bang.\n", GET_NAME(ch));  /* Grab char name */

/* End of IMM poofins */
  
  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
  {
    	  if (IN_ROOM(i) == IN_ROOM(ch))
		  {
	  if (CAN_SEE(i, ch) && (ch != i))
	  send_to_char(i, buf);
		  }
  }
  
  look_at_room(IN_ROOM(ch), ch, 0);
}



ACMD(do_trans)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to transfer?\r\n");
  else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", NOPERSON);
    else if (PLR_FLAGGED(victim, PLR_JAIL))
	  {
      snprintf(buf2, sizeof(buf2), "%s is taking a timeout right now...\r\n", GET_NAME(victim));
	  send_to_char(ch, buf2);
      return;
	  }
	else if (victim == ch)
      send_to_char(ch, "That doesn't make much sense, does it?\r\n");
    else {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char(ch, "Go transfer someone your own size.\r\n");
	return;
      }
      mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s transfered %s to room [%5d].",
		   GET_NAME(ch), GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(ch)) );	
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, IN_ROOM(ch));
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(IN_ROOM(victim), victim, 0);
    }
  } else {			/* Trans All */
    if (GET_LEVEL(ch) < LVL_GOD) {
      send_to_char(ch, "I think not.\r\n");
      return;
    }
     
    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
	victim = i->character;
	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
	  continue;
	else if (PLR_FLAGGED(i->character, PLR_JAIL))
	  {
      snprintf(buf2, sizeof(buf2), "%s is taking a timeout right now...\r\n", GET_NAME(victim));
	  send_to_char(ch, buf2);
      continue;
	  }   
	mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s transfered %s to room [%5d].",
		   GET_NAME(ch), GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(ch)) );	
	act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, IN_ROOM(ch));
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(IN_ROOM(victim), victim, 0);
      }
    send_to_char(ch, "%s", OK);
  }
}



ACMD(do_teleport)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char(ch, "Whom do you wish to teleport?\r\n");
  else if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", NOPERSON);
  else if (PLR_FLAGGED(victim, PLR_JAIL))
	  {
      snprintf(buf2, sizeof(buf2), "%s is taking a timeout right now...\r\n", GET_NAME(victim));
	  send_to_char(ch, buf2);
      return;
	  }
  else if (victim == ch)
    send_to_char(ch, "Use 'goto' to teleport yourself.\r\n");
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char(ch, "Maybe you shouldn't do that.\r\n");
  else if (!*buf2)
    send_to_char(ch, "Where do you wish to send this person?\r\n");
  else if ((target = find_target_room(ch, buf2)) != NOWHERE) {    
    send_to_char(ch, "%s", OK);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(IN_ROOM(victim), victim, 0);
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s teleported %s to room [%5d].",
		   GET_NAME(ch), GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)) );	
  }
}



ACMD(do_vnum)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj") && !is_abbrev(buf, "weapon"))) {
    send_to_char(ch, "Usage: vnum { obj | mob | weapon } <name | attack-type>\r\n");
     return;

  }
  if (is_abbrev(buf, "mob"))
    if (!vnum_mobile(buf2, ch))
      send_to_char(ch, "No mobiles by that name.\r\n");

  if (is_abbrev(buf, "obj"))
    if (!vnum_object(buf2, ch))
      send_to_char(ch, "No objects by that name.\r\n");

  if (is_abbrev(buf, "weapon"))
    if (!vnum_weapon(find_attack_type(buf2), ch))
      send_to_char(ch, "No weapons with that attack-type.\r\n");
   if (GET_LEVEL(ch) <= LVL_GOD){
   mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s used VNUM to search %s for %s.",
		 GET_NAME(ch), buf, buf2); }
}



void do_stat_room(struct char_data *ch)
{
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  struct extra_descr_data *desc;
  struct room_data *rm = &world[IN_ROOM(ch)];
  int i, found, column;
  struct obj_data *j;
  struct char_data *k;

  sprinttype(rm->size, sizes, buf3, sizeof(buf3));

  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s statted room [%5d].",
		 GET_NAME(ch), rm->number);
  send_to_char(ch, "Room name: %s%s%s\r\n", CCBCN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM));

  sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
  send_to_char(ch, "Zone: [%3d], VNum: [%s%5d%s], RNum: [%5d], Type: %s\r\n",
	  zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number,
	  CCNRM(ch, C_NRM), IN_ROOM(ch), buf2);

  sprintbit(rm->room_flags, room_bits, buf2, sizeof(buf2));
  send_to_char(ch, "SpecProc: %s, Flags: %s\r\n", rm->func == NULL ? "None" : "Exists", buf2);

  send_to_char(ch, "Description:\r\n%s", rm->description ? rm->description : "  None.\r\n");

  if (rm->ex_description) {
    send_to_char(ch, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = rm->ex_description; desc; desc = desc->next)
      send_to_char(ch, " %s", desc->keyword);
    send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
  }

  send_to_char(ch, "Chars present:%s", CCYEL(ch, C_NRM));
  column = 14;	/* ^^^ strlen ^^^ */
  for (found = FALSE, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;

    column += send_to_char(ch, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
		!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB"));
    if (column >= 62) {
      send_to_char(ch, "%s\r\n", k->next_in_room ? "," : "");
      found = FALSE;
      column = 0;
    }
  }
  send_to_char(ch, "%s", CCNRM(ch, C_NRM));

  if (rm->contents) {
    send_to_char(ch, "Contents:%s", CCGRN(ch, C_NRM));
    column = 9;	/* ^^^ strlen ^^^ */

    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;

      column += send_to_char(ch, "%s %s", found++ ? "," : "", j->short_description);
      if (column >= 62) {
	send_to_char(ch, "%s\r\n", j->next_content ? "," : "");
	found = FALSE;
        column = 0;
      }
    }
    send_to_char(ch, "%s", CCNRM(ch, C_NRM));
  }

  for (i = 0; i < NUM_OF_DIRS; i++) {
    char buf1[128];

    if (!rm->dir_option[i])
      continue;

    if (rm->dir_option[i]->to_room == NOWHERE)
      snprintf(buf1, sizeof(buf1), " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    else
      snprintf(buf1, sizeof(buf1), "%s%5d%s", CCCYN(ch, C_NRM), GET_ROOM_VNUM(rm->dir_option[i]->to_room), CCNRM(ch, C_NRM));

    sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2, sizeof(buf2));

    send_to_char(ch, "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n%s",
	CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1, rm->dir_option[i]->key,
	rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None", buf2,
	rm->dir_option[i]->general_description ? rm->dir_option[i]->general_description : "  No exit description.\r\n");
  }
  /* check the room for a script */
  do_sstat_room(ch);
}



void do_stat_object(struct char_data *ch, struct obj_data *j)
{
  int i, found, i3;
  obj_vnum vnum;
  struct obj_data *j2;
  struct extra_descr_data *desc;
  char buf[MAX_STRING_LENGTH];
  bool resists = FALSE;

  vnum = GET_OBJ_VNUM(j);
  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s statted object [%5d] %s .",
		   GET_NAME(ch), vnum, j->name);	
  send_to_char(ch, "%sName: %s%s %sAliases: %s%s\r\n", 
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), j->short_description ? j->short_description : "<None>",
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), j->name);

  sprinttype(GET_OBJ_TYPE(j), item_types, buf, sizeof(buf));
  send_to_char(ch, "%sVNum: %s%5d %sRNum: %s%5d %sType: %s%s %sSpecProc: %s%s\r\n",
	CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), vnum, 
        CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_RNUM(j), 
        CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf,
        CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_SPEC(j) ? "Exists" : "None");
	
  send_to_char(ch, "%sMin Level: %s%d %sMax Level: %s%d\r\n",
                    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_LEVEL(j),
                    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), 
GET_OBJ_MAX_LEVEL(j));
  if (j->ex_description) {
    send_to_char(ch, "%sExtra descs:%s ", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM));
    for (desc = j->ex_description; desc; desc = desc->next)
      send_to_char(ch, " %s", desc->keyword);
    send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
  }

 
  sprintbit(GET_OBJ_WEAR(j), wear_bits, buf, sizeof(buf));
  send_to_char(ch, "%sCan be worn on: %s %s \r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),  buf);

  sprintbit(GET_OBJ_AFFECT(j), affected_bits, buf, sizeof(buf));
  send_to_char(ch, "%sSet char bits : %s %s \r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),  buf );

  sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf, sizeof(buf));
  send_to_char(ch, "%sExtra flags   : %s %s \r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf);

  sprintbit(GET_OBJ_CLASS(j), class_bits, buf, sizeof(buf));
  send_to_char(ch, "%sClass Restr   : %s %s\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),  buf);

  sprinttype(j->size, sizes, buf, sizeof(buf));
  send_to_char(ch, "%sSize          : %s %s \r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf);

  send_to_char(ch, "%sWeight: %s%d %sValue: %s%d %sCost/day: %s%d %sTimer: %s%d \r\n",
     CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_WEIGHT(j), 
     CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_COST(j), 
     CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_RENT(j), 
     CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_TIMER(j));

  send_to_char(ch, "%sIn room: %s%d (%s) ", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_ROOM_VNUM(IN_ROOM(j)),
	IN_ROOM(j) == NOWHERE ? "Nowhere" : world[IN_ROOM(j)].name);

  /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
  send_to_char(ch, "%sIn object: %s%s ", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), j->in_obj ? j->in_obj->short_description : 
"None");
  send_to_char(ch, "%sCarried by: %s%s, ", CCCYN(ch, C_NRM), CCGRN(ch, 
C_NRM), j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
  send_to_char(ch, "%sWorn by: %s%s\r\n", CCCYN(ch, C_NRM), CCGRN(ch, 
C_NRM), j->worn_by ? GET_NAME(j->worn_by) : "Nobody");

  switch (GET_OBJ_TYPE(j)) {
  case ITEM_LIGHT:
    if (GET_OBJ_VAL(j, 2) == -1)
      send_to_char(ch, "%sHours left: %sInfinite\r\n", CCCYN(ch, C_NRM), CCGRN(ch, 
C_NRM));
    else
      send_to_char(ch, "%sHours left: %s%d\r\n", CCCYN(ch, C_NRM), 
CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 2));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    send_to_char(ch, "%sSpells: %sLevel %d %s, %s, %s\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 0),
	    skill_name(GET_OBJ_VAL(j, 1)), skill_name(GET_OBJ_VAL(j, 2)),
	    skill_name(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    send_to_char(ch, "%sSpell: %s%s at level %d, %d (of %d) charges remaining\r\n",
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), skill_name(GET_OBJ_VAL(j, 3)), 
GET_OBJ_VAL(j, 0),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_WEAPON:
    send_to_char(ch, "%sTodam: %s%dd%d	 %sAverage dam: %s%d	%sMessage type:%s %d\r\n",
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), get_ave_dam(j), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_ARMOR:
    send_to_char(ch, "%sAC-apply: %s%d\r\n", CCCYN(ch, C_NRM), 
CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 0));
    break;
  case ITEM_TRAP:
    send_to_char(ch, "%sSpell:%s %d  %sHitpoints: %s%d\r\n", 
        CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 0), 
        CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_CONTAINER:
    sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf, sizeof(buf));
    send_to_char(ch, "%sWeight capacity: %s %d   %sLock Type: %s%s %sKey Num: %s%d %sCorpse: %s%s\r\n",
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 0), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf, 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 2),
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    sprinttype(GET_OBJ_VAL(j, 2), drinks, buf, sizeof(buf));
    send_to_char(ch, "%sCapacity: %s%d %sContains: %s%d %sPoisoned: %s%s %sLiquid:%s %s\r\n",
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 0), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 1), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), YESNO(GET_OBJ_VAL(j, 3)), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf);
    break;
  case ITEM_NOTE:
    send_to_char(ch, "%sTongue: %s%d\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 0));
    break;
  case ITEM_KEY:
    /* Nothing */
    break;
  case ITEM_FOOD:
    send_to_char(ch, "%sMakes full: %s%d %sPoisoned: %s%s\r\n", 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 0), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_MONEY:
    send_to_char(ch, "%sCoins: %s%d\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 0));
    break;
  default:
    send_to_char(ch, "%sValues 0-3: %s%d %d %d %d\r\n",
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OBJ_VAL(j, 0), 
            GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  }

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (j->contains) {
    int column;

    send_to_char(ch, "\r\nContents:%s", CCGRN(ch, C_NRM));
    column = 9;	/* ^^^ strlen ^^^ */

    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      column += send_to_char(ch, "%s %s", found++ ? "," : "", j2->short_description);
      if (column >= 62) {
	send_to_char(ch, "%s\r\n", j2->next_content ? "," : "");
	found = FALSE;
        column = 0;
      }
    }
    send_to_char(ch, "%s", CCNRM(ch, C_NRM));
  }

  found = FALSE;
  send_to_char(ch, "%sAffections%s:", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM));
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier) {
      sprinttype(j->affected[i].location, apply_types, buf, sizeof(buf));
      send_to_char(ch, "%s %+d to %s", found++ ? "," : "", j->affected[i].modifier, buf);
    }
  if (!found)
    send_to_char(ch, " None");

  send_to_char(ch, "\r\n");
  
   /* show resistances etc...*/
  send_to_char(ch, "%sImmunities: %s", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM));
	  for(i3 = 0;i3 < MAX_ATTACK_TYPES; i3++) {
	    if (j->immune[i3] == 1) {
	      send_to_char(ch, "%s ", res_types[i3]);
	      resists = TRUE;
	    }
	  }
	if (resists == FALSE) 
		  send_to_char(ch, "None %s\r\n", CCNRM(ch, C_NRM));
  else {
      send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
      resists = FALSE;
  }
		
	send_to_char(ch, "Resists: %s", CCCYN(ch, C_NRM));
	  for(i3 = 0;i3 < MAX_ATTACK_TYPES; i3++) {
	    if (j->resist[i3] == 1) {
	      send_to_char(ch, "%s ", res_types[i3]);
	       resists = TRUE;
	    }
	  }
	if (resists == FALSE) 
		  send_to_char(ch, "None %s\r\n", CCNRM(ch, C_NRM));
  else {
      send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
      resists = FALSE;
  }
	
	send_to_char(ch, "Vulnerabilities: %s", CCCYN(ch, C_NRM));
	  for(i3 = 0;i3 < MAX_ATTACK_TYPES; i3++) {
	    if (j->vulnerable[i3] == 1) {
	      send_to_char(ch, "%s ", res_types[i3]);
	       resists = TRUE;
	    }
	  }
	if (resists == FALSE) 
		  send_to_char(ch, "None %s\r\n", CCNRM(ch, C_NRM));
  else {
      send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
      resists = FALSE;
  }

  /* check the object for a script */
  do_sstat_object(ch, j);
}


void do_stat_character(struct char_data *ch, struct char_data *k)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  int i, i2, i3,column, found = FALSE;
  int played_time;
  bool resists = FALSE;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;

  sprinttype(GET_SIZE(k), sizes, buf3, sizeof(buf3));

  if (IS_NPC(k) )
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s statted mob [%5d] %s.",
		   GET_NAME(ch), GET_MOB_VNUM(k), GET_NAME(k));
  else
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s statted player %s.",
		   GET_NAME(ch), GET_NAME(k));

  sprinttype(GET_SEX(k), genders, buf, sizeof(buf));
  send_to_char(ch, "%s%s Stats\r\n", CCYEL(ch, C_NRM), (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
  send_to_char(ch, "%sCharacter:%s %s %sIDNum:%s %5ld %sIn room:%s %5d %sSize:%s %s\r\n",
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_NAME(k), 
          CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_IDNUM(k), 
          CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_ROOM_VNUM(IN_ROOM(k)), 
          CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf3);

  if (IS_MOB(k))
    send_to_char(ch, "%sAlias:%s %s %sVNum:%s %5d %sRNum:%s %5d\r\n", 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), k->player.name, 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOB_VNUM(k), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOB_RNUM(k));

  send_to_char(ch, "%sTitle:%s %s\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), k->player.title ? k->player.title : 
"<None>");

  send_to_char(ch, "%sL-Des:%s %s", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), k->player.long_descr ? 
k->player.long_descr : "<None>\r\n");

//dan clan system
  send_to_char(ch, "%sClan:%s %s %sClan Rank:%s %s\r\n",
    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),  
    get_clan_name(GET_CLAN(k)), CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), 
    get_rank_name(GET_CLAN(k), GET_CLAN_RANK(k)));

  sprinttype(k->player.chclass, IS_NPC(k) ? mob_class_types : pc_class_types, buf, sizeof(buf));
  sprinttype(k->player.race, IS_NPC(k) ? mob_race_types: pc_race_types, buf2, sizeof(buf2));
  send_to_char(ch, "%s%sClass:%s %s %sRace:%s %s %sLev:%s %2d %sXP:%s %7d "
                   "%sAlign:%s %4d\r\n", CCCYN(ch, C_NRM), IS_NPC(k) ? "Monster " : "", CCGRN(ch, C_NRM),
                   buf, CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf2, CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), 
                   GET_LEVEL(k),CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),
                   GET_EXP(k), CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),
                   GET_ALIGNMENT(k));

  if (!IS_NPC(k)) {
    char buf1[64], buf2[64];

    /* TODO: replace asctime() calls with strftime() calls */
    strlcpy(buf1, asctime(localtime(&(k->player.time.birth))), sizeof(buf1));
    strlcpy(buf2, asctime(localtime(&(k->player.time.logon))), sizeof(buf2));
    played_time = k->player.time.played + (time(0) - k->player.time.logon);
    buf1[10] = buf2[10] = '\0';

    send_to_char(ch, "%sCreated:%s %s %sLast Logon:%s %s %sPlayed:%s %dd %dh %dm %sAge:%s %d\r\n",
	    CCCYN(k, C_NRM), CCGRN(k, C_NRM),buf1, CCCYN(k, C_NRM), CCGRN(k, C_NRM),buf2,CCCYN(k, C_NRM), 
            CCGRN(k, C_NRM), played_time / 86400, (played_time % 86400) / 3600, ((played_time % 86400) % 3600) / 60, 
            CCCYN(k, C_NRM), CCGRN(k, C_NRM), age(k)->year);

    send_to_char(ch, "%sHometown:%s %d %sSpeaks:%s %d/%d/%d %sSTL:%s%d %sPER:%s%d %sNSTL:%s%d",
	   CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), k->player.hometown, CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_TALK(k, 
0), GET_TALK(k, 1), GET_TALK(k, 2),
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_PRACTICES(k), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), int_app[GET_INT(k)].learn,
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), wis_app[GET_WIS(k)].bonus);
    /*. Display OLC zone for immorts .*/
    if (GET_LEVEL(k) >= LVL_SAINT)
      send_to_char(ch, " %sOLC:%s%d", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_OLC_ZONE(k));
    send_to_char(ch, "\r\n");
  }
  send_to_char(ch, "%sStr:%s %d/%d  %sInt:%s %d  %sWis:%s %d  "
	  "%sDex:%s %d  %sCon:%s %d  %sCha:%s %d\r\n",
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_STR(k), GET_ADD(k),
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_INT(k), 
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_WIS(k), 
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_DEX(k),
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_CON(k),
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_CHA(k));

  send_to_char(ch, "%sHP:%s %d/%d+%d  %sMana%s:%d/%d+%d  %sMove:%s%d/%d+%d\r\n",
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k), 
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k), mana_gain(k),
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k));

  send_to_char(ch, "%sCoins:%s %9d %sBank:%s %9d %sTotal:%s %d\r\n",
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_GOLD(k), 
          CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_BANK_GOLD(k), 
          CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_GOLD(k) + GET_BANK_GOLD(k));

  send_to_char(ch, "%sAC:%s %d(%+d) %sHitroll:%s %2d %sDamroll:%s %2d %sSaving throws:%s %d/%d/%d/%d/%d\r\n",
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_AC(k), dex_app[GET_DEX(k)].defensive, 
          CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), k->points.hitroll,
	  CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), k->points.damroll, 
          CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2),
	       GET_SAVE(k, 3), GET_SAVE(k, 4));

  sprinttype(GET_POS(k), position_types, buf, sizeof(buf));
  send_to_char(ch, "%sPos:%s %s %sFighting:%s %s", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf, 
       CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody");

  if (IS_NPC(k))
    send_to_char(ch, " %sAttack type:%s %s", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), attack_hit_text[(int) 
k->mob_specials.attack_type].singular);

  if (k->desc) {
    sprinttype(STATE(k->desc), connected_types, buf, sizeof(buf));
    send_to_char(ch, " %sConnected:%s %s", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf);
  }

  if (IS_NPC(k)) {
    sprinttype(k->mob_specials.default_pos, position_types, buf, sizeof(buf));
    send_to_char(ch, ", %sDefault position:%s %s\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), buf);
    sprintbit(MOB_FLAGS(k), action_bits, buf, sizeof(buf));
    send_to_char(ch, "%sNPC flags:%s %s\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),  buf);
  } else {
    send_to_char(ch, " %sIdle Timer%s %d tics\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), k->char_specials.timer);

    sprintbit(PLR_FLAGS(k), player_bits, buf, sizeof(buf));
    send_to_char(ch, "%sPlayer Flags:%s %s\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),  buf);

    sprintbit(PRF_FLAGS(k), preference_bits, buf, sizeof(buf));
    send_to_char(ch, "%sPref:%s %s\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),  buf);
  }

  if (IS_MOB(k))
    send_to_char(ch, "%sMob Spec-Proc:%s %s %sNPC Bare Hand Dam:%s %dd%d\r\n",
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"),
	    CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), k->mob_specials.damnodice, k->mob_specials.damsizedice);

  for (i = 0, j = k->carrying; j; j = j->next_content, i++);
  send_to_char(ch, "%sCarried weight:%s %d %sItems:%s %d %sItems in inventory:%s %d ", 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), IS_CARRYING_W(k), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), IS_CARRYING_N(k), 
            CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i))
      i2++;
  send_to_char(ch, "%sEQ:%s %d\r\n", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), i2);

  if (!IS_NPC(k))
    send_to_char(ch, "%sHunger:%s %d %sThirst:%s %d %sDrunk:%s %d\r\n", 
         CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_COND(k, FULL), 
         CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_COND(k, THIRST),
         CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), GET_COND(k, DRUNK));

  column = send_to_char(ch, "%sMaster:%s %s %sFollowers are:%s", 
         CCCYN(ch, C_NRM), CCGRN(ch, C_NRM), k->master ? GET_NAME(k->master) : "<none>",
         CCCYN(ch, C_NRM), CCGRN(ch, C_NRM) );
  if (!k->followers)
    send_to_char(ch, " <none>\r\n");
  else {
    for (fol = k->followers; fol; fol = fol->next) {
      column += send_to_char(ch, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
      if (column >= 62) {
        send_to_char(ch, "%s\r\n", fol->next ? "," : "");
        found = FALSE;
        column = 0;
      }
    }
    if (column != 0)
      send_to_char(ch, "\r\n");
  }
  
  /* show resistances etc...*/
  send_to_char(ch, "%sImmunities:%s ", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM) );
	  for(i3 = 0;i3 < MAX_ATTACK_TYPES; i3++) {
	    if (k->char_specials.immune[i3] > 0) {
	      send_to_char(ch, "%s ", res_types[i3]);
	      resists = TRUE;
	    }
	  }
	if (resists == FALSE) 
		  send_to_char(ch, "None %s\r\n", CCNRM(ch, C_NRM));
  else {
      send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
      resists = FALSE;
  }
		
	send_to_char(ch, "%sResists: %s", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM) );
	  for(i3 = 0;i3 < MAX_ATTACK_TYPES; i3++) {
	    if (k->char_specials.resist[i3] > 0) {
	      send_to_char(ch, "%s ", res_types[i3]);
	       resists = TRUE;
	    }
	  }
	if (resists == FALSE) 
		  send_to_char(ch, "None %s\r\n", CCNRM(ch, C_NRM));
  else {
      send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
      resists = FALSE;
  }
	
	send_to_char(ch, "%sVulnerabilities: %s", CCCYN(ch, C_NRM), CCGRN(ch, C_NRM) );
	  for(i3 = 0;i3 < MAX_ATTACK_TYPES; i3++) {
	    if (k->char_specials.vulnerable[i3] > 0) {
	      send_to_char(ch, "%s ", res_types[i3]);
	       resists = TRUE;
	    }
	  }
	if (resists == FALSE) 
		  send_to_char(ch, "None %s\r\n", CCNRM(ch, C_NRM));
  else {
      send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
      resists = FALSE;
  }

  /* Showing the bitvector */
  sprintbit(AFF_FLAGS(k), affected_bits, buf, sizeof(buf));
  send_to_char(ch, "%sAFF: %s%s%s\r\n", CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), buf, CCNRM(ch, C_NRM));
  
  /* Showing the bitvector for second aff */
  sprintbit(AFF2_FLAGS(k), affected2_bits, buf, sizeof(buf));
  send_to_char(ch, "%sAFF2: %s%s%s\r\n", CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), buf, CCNRM(ch, C_NRM));

  /* Routine to show what spells a char is affected by */
  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      /*                                             What's with this + 1?                                                        */
      /*send_to_char(ch, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1, CCCYN(ch, C_NRM), skill_name(aff->type), CCNRM(ch, C_NRM));*/
        send_to_char(ch, "SPL: (%3dhr) %s%-21s%s ", aff->duration, CCCYN(ch, C_NRM), skill_name(aff->type), CCNRM(ch, C_NRM));
      if (aff->modifier)
	send_to_char(ch, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);

      if (aff->bitvector) {
	if (aff->modifier)
	  send_to_char(ch, ", ");

	sprintbit(aff->bitvector, affected_bits, buf, sizeof(buf));
        send_to_char(ch, "sets %s", buf);
      }
      send_to_char(ch, "\r\n");
    }
  }
  
  /* Routine to show what spells a char is affected2 by */
  if (k->affected2) {
    for (aff = k->affected2; aff; aff = aff->next) {
      send_to_char(ch, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1, CCCYN(ch, C_NRM), skill_name(aff->type), CCNRM(ch, C_NRM));

      if (aff->modifier)
	send_to_char(ch, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);

      if (aff->bitvector) {
	if (aff->modifier)
	  send_to_char(ch, ", ");

	sprintbit(aff->bitvector, affected2_bits, buf, sizeof(buf));
        send_to_char(ch, "sets %s", buf);
      }
      send_to_char(ch, "\r\n");
    }
  }

  /* check mobiles for a script */
  if (IS_NPC(k)) {
    do_sstat_character(ch, k);
    if (SCRIPT_MEM(k)) {
      struct script_memory *mem = SCRIPT_MEM(k);
      send_to_char(ch, "Script memory:\r\n  Remember             Command\r\n");
      while (mem) {
        struct char_data *mc = find_char(mem->id);
        if (!mc)
          send_to_char(ch, "  ** Corrupted!\r\n");
        else {
          if (mem->cmd) 
            send_to_char(ch, "  %-20.20s%s\r\n",GET_NAME(mc),mem->cmd);
          else 
            send_to_char(ch, "  %-20.20s <default>\r\n",GET_NAME(mc));
        }
      mem = mem->next;
      }
    }
  } else {
    /* this is a PC, display their global variables */
    if (k->script && k->script->global_vars) {
      struct trig_var_data *tv;
      char uname[MAX_INPUT_LENGTH];
      void find_uid_name(char *uid, char *name);

      send_to_char(ch, "Global Variables:\r\n");

      /* currently, variable context for players is always 0, so it is */
      /* not displayed here. in the future, this might change */
      for (tv = k->script->global_vars; tv; tv = tv->next) {
        if (*(tv->value) == UID_CHAR) {
          find_uid_name(tv->value, uname);
          send_to_char(ch, "    %10s:  [UID]: %s\r\n", tv->name, uname);
        } else
          send_to_char(ch, "    %10s:  %s\r\n", tv->name, tv->value);
      }
    }
  }
}


ACMD(do_stat)
{
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  struct obj_data *object;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(ch, "Stats on who or what?\r\n");
    return;
  } else if (is_abbrev(buf1, "room")) {
    do_stat_room(ch);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which mobile?\r\n");
    else {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such mobile around.\r\n");
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char(ch, "Stats on which player?\r\n");
    } else {
      if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such player around.\r\n");
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which player?\r\n");
    else if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
    else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      CREATE(victim->player_specials, struct player_special_data, 1);
      if (load_char(buf2, victim) >= 0) {
	char_to_room(victim, 0);
	if (GET_LEVEL(victim) > GET_LEVEL(ch))
	  send_to_char(ch, "Sorry, you can't do that.\r\n");
	else
	  do_stat_character(ch, victim);
	extract_char_final(victim);
      } else {
	send_to_char(ch, "There is no such player.\r\n");
	free_char(victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which object?\r\n");
    else {
      if ((object = get_obj_vis(ch, buf2, NULL)) != NULL)
	do_stat_object(ch, object);
      else
	send_to_char(ch, "No such object around.\r\n");
    }
  } else {
    char *name = buf1;
    int number = get_number(&name);

    if ((object = get_obj_in_equip_vis(ch, name, &number, ch->equipment)) != NULL)
      do_stat_object(ch, object);
    else if ((object = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_ROOM)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_WORLD)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, name, &number)) != NULL)
      do_stat_object(ch, object);
    else
      send_to_char(ch, "Nothing around by that name.\r\n");
  }
}


ACMD(do_shutdown)
{
  char arg[MAX_INPUT_LENGTH];

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char(ch, "If you want to shut something down, say so!\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "now")) {
    log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    circle_shutdown = 1;
    circle_reboot = 2;
  } else if (!str_cmp(arg, "reboot")) {
    log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    touch(FASTBOOT_FILE);
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "die")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "pause")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(PAUSE_FILE);
    circle_shutdown = 1;
  } else
    send_to_char(ch, "Unknown shutdown option.\r\n");
}


void snoop_check(struct char_data *ch)
{
  /*  This short routine is to ensure that characters that happen
   *  to be snooping (or snooped) and get advanced/demoted will
   *  not be snooping/snooped someone of a higher/lower level (and
   *  thus, not entitled to be snooping.
   */
  if (!ch || !ch->desc)
    return;
  if (ch->desc->snooping &&
     (GET_LEVEL(ch->desc->snooping->character) >= GET_LEVEL(ch))) {
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }

  if (ch->desc->snoop_by &&
     (GET_LEVEL(ch) >= GET_LEVEL(ch->desc->snoop_by->character))) {
    ch->desc->snoop_by->snooping = NULL;
    ch->desc->snoop_by = NULL;
  }
}

void stop_snooping(struct char_data *ch)
{
  if (!ch->desc->snooping)
    send_to_char(ch, "You aren't snooping anyone.\r\n");
  else {
    send_to_char(ch, "You stop snooping.\r\n");
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}


ACMD(do_snoop)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such person around.\r\n");
  else if (!victim->desc)
    send_to_char(ch, "There's no link.. nothing to snoop.\r\n");
  else if (victim == ch)
    stop_snooping(ch);
  else if (victim->desc->snoop_by)
    send_to_char(ch, "Busy already. \r\n");
  else if (victim->desc->snooping == ch->desc)
    send_to_char(ch, "Don't be stupid.\r\n");
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
      send_to_char(ch, "You can't.\r\n");
      return;
    }
    mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "(GC) %s snooped %s.",
		 GET_NAME(ch), GET_NAME(victim));
    send_to_char(ch, "%s", OK);

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}



ACMD(do_switch)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  one_argument(argument, arg);

  if (ch->desc->original)
    send_to_char(ch, "You're already switched.\r\n");
  else if (!*arg)
    send_to_char(ch, "Switch with who?\r\n");
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such character.\r\n");
  else if (ch == victim)
    send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
  else if (victim->desc)
    send_to_char(ch, "You can't do that, the body is already in use!\r\n");
  else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
    send_to_char(ch, "You aren't holy enough to use a mortal's body.\r\n");
    /* cut by mak 8.21.05 
  else if (GET_LEVEL(ch) < LVL_GOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n"); */
  else if (GET_LEVEL(ch) < LVL_IMPL && ROOM_FLAGGED(IN_ROOM(victim), ROOM_IMPROOM))
  send_to_char(ch, "You dare not disturb the Implementors!\r\n");
  else if (GET_LEVEL(ch) < LVL_GOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE)
		&& !House_can_enter(ch, GET_ROOM_VNUM(IN_ROOM(victim))))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else {
    send_to_char(ch, "%s", OK);
    
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s switched into %s.",
		   GET_NAME(ch), victim->player.short_descr );	

    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}


ACMD(do_return)
{
  if (ch->desc && ch->desc->original) {
    send_to_char(ch, "You return to your original body.\r\n");

    /*
     * If someone switched into your original body, disconnect them.
     *   - JE 2/22/95
     *
     * Zmey: here we put someone switched in our body to disconnect state
     * but we must also NULL his pointer to our character, otherwise
     * close_socket() will damage our character's pointer to our descriptor
     * (which is assigned below in this function). 12/17/99
     */
    if (ch->desc->original->desc) {
      ch->desc->original->desc->character = NULL;
      STATE(ch->desc->original->desc) = CON_DISCONNECT;
    }

    /* Now our descriptor points to our original body. */
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    /* And our body's pointer to descriptor now points to our descriptor. */
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}



ACMD(do_load)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: load { obj | mob } <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(ch, "That is not a number.\r\n");
    return;
  }
  
  if (is_abbrev(buf, "mob")) {
    struct char_data *mob;
    mob_rnum r_num;
   
    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, IN_ROOM(ch));
    
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s loaded mob [%5d].",
		 GET_NAME(ch), GET_MOB_VNUM(mob));

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
	0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
    load_mtrigger(mob);
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(r_num, REAL);
    if (load_into_inventory)
      obj_to_char(obj, ch);
    else    
     obj_to_room(obj, IN_ROOM(ch));
     mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s loaded obj [%5d].",
		     GET_NAME(ch), GET_OBJ_VNUM(obj));
     act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
      act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
      act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
     load_otrigger(obj);
  } else
    send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}



ACMD(do_vstat)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: vstat { obj | mob } <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(ch, "That's not a valid number.\r\n");
    return;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob;
    mob_rnum r_num;

    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj);
    extract_obj(obj);
  } else
    send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}




/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct obj_data *obj;

  one_argument(argument, buf);

  /* argument supplied. destroy single object or char */
  if (*buf) {
	if ((vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)) != NULL) {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
	send_to_char(ch, "Fuuuuuuuuu!\r\n");
	return;
      }
     if (PLR_FLAGGED(vict, PLR_JAIL))
	  {
      send_to_char(ch, "That player is in JAIL and will remain there.\r\n");
	  return;
	  }	
	  act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict)) {
	mudlog(BRF, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
	if (vict->desc) {
	  STATE(vict->desc) = CON_CLOSE;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = get_obj_in_list_vis(ch, buf, NULL, world[IN_ROOM(ch)].contents)) != NULL) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char(ch, "Nothing here by that name.\r\n");
      return;
    }

    send_to_char(ch, "%s", OK);
  } else {			/* no argument. clean out the room */
    int i;

    act("$n gestures... You are surrounded by scorching flames!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room(IN_ROOM(ch), "The world seems a little cleaner.\r\n");

    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
      if (!IS_NPC(vict))
        continue;

      /* Dump inventory. */
      while (vict->carrying)
        extract_obj(vict->carrying);

      /* Dump equipment. */
      for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(vict, i))
          extract_obj(GET_EQ(vict, i));

      /* Dump character. */
      extract_char(vict);
    }

    /* Clear the ground but not NO_PURGE items. */


j = world[IN_ROOM(ch)].contents;
if ( j == NULL ) 
  {
 	return;
  }
else 
   {
j = world[IN_ROOM(ch)].contents;
while ( j != NULL )   
	{	
	if (OBJ_FLAGGED((j), ITEM_NO_PURGE)) 
		{
		if ( j-> next_content == NULL )
         {
		 return;
		 }
     	 else
		 {
		 j = j->next_content;
         }
		}
	else
		{
		l = j->next_content;	
		extract_obj(j);  
		if (l == NULL)
		    {
			return;
			}
		else{
			j = l;
		    }
         }
    }  
   } 

}
}

const char *logtypes[] = {
  "off", "brief", "normal", "complete", "\n"
};

ACMD(do_syslog)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "Your syslog is currently %s.\r\n",
	logtypes[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)]);
    return;
  }
  if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
    send_to_char(ch, "Usage: syslog { Off | Brief | Normal | Complete }\r\n");
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
  SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

  send_to_char(ch, "Your syslog is now %s.\r\n", logtypes[tp]);
}



ACMD(do_advance)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
  int newlevel, oldlevel;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, "That player is not here.\r\n");
      return;
    }
  } else {
    send_to_char(ch, "Advance who?\r\n");
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char(ch, "Maybe that's not such a great idea.\r\n");
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char(ch, "NO!  Not on NPC's.\r\n");
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char(ch, "That's not a level!\r\n");
    return;
  }
  if (newlevel > LVL_IMPL) {
    send_to_char(ch, "%d is the highest possible level.\r\n", LVL_IMPL);
    return;
  }
  if (newlevel > GET_LEVEL(ch)) {
    send_to_char(ch, "Yeah, right.\r\n");
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    send_to_char(ch, "They are already at that level.\r\n");
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    do_start(victim);
    GET_LEVEL(victim) = newlevel;
    send_to_char(victim, "You are momentarily enveloped by darkness!\r\nYou feel somewhat diminished.\r\n");
  } else {
    act("$n makes some strange gestures.\r\n"
	"A strange feeling comes upon you,\r\n"
	"Like a giant hand, light comes down\r\n"
	"from above, grabbing your body, that\r\n"
	"begins to pulse with colored lights\r\n"
	"from inside.\r\n\r\n"
	"Your head seems to be filled with demons\r\n"
	"from another plane as your body dissolves\r\n"
	"to the elements of time and space itself.\r\n"
	"Suddenly a silent explosion of light\r\n"
	"snaps you back to reality.\r\n\r\n"
	"You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(ch, "%s", OK);

  if (newlevel < oldlevel)
    log("(GC) %s demoted %s from level %d to %d.",
		GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
  else
    log("(GC) %s has advanced %s to level %d (from %d)",
		GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  if (oldlevel >= LVL_SAINT && newlevel < LVL_SAINT) {
    /* If they are no longer an immortal, let's remove some of the
     * nice immortal only flags, shall we?
     */
    REMOVE_BIT(PRF_FLAGS(victim), PRF_LOG1 | PRF_LOG2);
    REMOVE_BIT(PRF_FLAGS(victim), PRF_NOHASSLE | PRF_HOLYLIGHT);
    run_autowiz();
    perform_immort_vis(victim); /* no more invis 47 morts */
  }

  gain_exp_regardless(victim,
	 level_exp(GET_CLASS(victim), newlevel) - GET_EXP(victim));
  save_char(victim, GET_LOADROOM(ch));
}

ACMD(do_restore)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to restore?\r\n");
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", NOPERSON);
  else if (!IS_NPC(vict) && ch != vict && GET_LEVEL(vict) >= GET_LEVEL(ch))
    send_to_char(ch, "They don't need your help.\r\n");
  else {
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s restored %s.",
		 GET_NAME(ch), GET_NAME(vict));
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);

    if (!IS_NPC(vict) && GET_LEVEL(ch) >= LVL_GOD) {
      if (GET_LEVEL(vict) >= LVL_SAINT)
        for (i = 1; i <= MAX_SKILLS; i++)
          SET_SKILL(vict, i, 100);

      if (GET_LEVEL(vict) >= LVL_GOD) {
	vict->real_abils.str_add = 100;
	vict->real_abils.intel = 25;
	vict->real_abils.wis = 25;
	vict->real_abils.dex = 25;
	vict->real_abils.str = 25;
	vict->real_abils.con = 25;
	vict->real_abils.cha = 25;
      }
    }
    update_pos(vict);
    affect_total(vict);
    send_to_char(ch, "%s", OK);
    act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
  }
}


void perform_immort_vis(struct char_data *ch)
{
  if (GET_INVIS_LEV(ch) == 0 && !AFF_FLAGGED(ch, AFF_HIDE | AFF_INVISIBLE)) {
    send_to_char(ch, "You are already fully visible.\r\n");
    return;
  }
   
  GET_INVIS_LEV(ch) = 0;
  appear(ch);
  send_to_char(ch, "You are now fully visible.\r\n");
}


void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
 /*   if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
	  tch, TO_VICT);
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
	  tch, TO_VICT); */
  }

  GET_INVIS_LEV(ch) = level;
  send_to_char(ch, "Your invisibility level is %d.\r\n", level);
   mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "(GC) %s went to INVIS LEVEL %d",  
            GET_NAME(ch), level);
}
  

ACMD(do_invis)
{
  char arg[MAX_INPUT_LENGTH];
  int level;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You can't do that!\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else {
    level = atoi(arg);
    if (level > GET_LEVEL(ch))
      send_to_char(ch, "You can't go invisible above your own level.\r\n");
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}


ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char(ch, "That must be a mistake...\r\n");
  else {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (STATE(pt) == CON_PLAYING && pt->character && pt->character != ch)
	send_to_char(pt->character, "%s\r\n", argument);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", OK);
    else
      send_to_char(ch, "%s\r\n", argument);
  }
}


ACMD(do_poofset)
{
  char **msg;

  switch (subcmd) {
  case SCMD_POOFIN:    msg = &(POOFIN(ch));    break;
  case SCMD_POOFOUT:   msg = &(POOFOUT(ch));   break;
  default:    return;
  }

  skip_spaces(&argument);

  if (*msg)
    free(*msg);

  if (!*argument)
    *msg = NULL;
  else
    *msg = strdup(argument);

  send_to_char(ch, "%s", OK);
}



ACMD(do_dc)
{
  char arg[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char(ch, "Usage: DC <user number> (type USERS for a list)\r\n");
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char(ch, "No such connection.\r\n");
    return;
  }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
    if (!CAN_SEE(ch, d->character))
      send_to_char(ch, "No such connection.\r\n");
    else
      send_to_char(ch, "Umm.. maybe that's not such a good idea...\r\n");
    return;
  }

  /* We used to just close the socket here using close_socket(), but
   * various people pointed out this could cause a crash if you're
   * closing the person below you on the descriptor list.  Just setting
   * to CON_CLOSE leaves things in a massively inconsistent state so I
   * had to add this new flag to the descriptor. -je
   *
   * It is a much more logical extension for a CON_DISCONNECT to be used
   * for in-game socket closes and CON_CLOSE for out of game closings.
   * This will retain the stability of the close_me hack while being
   * neater in appearance. -gg 12/1/97
   *
   * For those unlucky souls who actually manage to get disconnected
   * by two different immortals in the same 1/10th of a second, we have
   * the below 'if' check. -gg 12/17/99
   */
  if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
    send_to_char(ch, "They're already being disconnected.\r\n");
  else {
    /*
     * Remember that we can disconnect people not in the game and
     * that rather confuses the code when it expected there to be
     * a character context.
     */
    if (STATE(d) == CON_PLAYING)
      STATE(d) = CON_DISCONNECT;
    else
      STATE(d) = CON_CLOSE;

    send_to_char(ch, "Connection #%d closed.\r\n", num_to_dc);
    log("(GC) Connection closed by %s.", GET_NAME(ch));
  }
}



ACMD(do_wizlock)
{
  char arg[MAX_INPUT_LENGTH];
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char(ch, "Invalid wizlock value.\r\n");
      return;
    }
    circle_restrict = value;
    when = "now";
  } else
    when = "currently";

  switch (circle_restrict) {
  case 0:
    send_to_char(ch, "The game is %s completely open.\r\n", when);
    break;
  case 1:
    send_to_char(ch, "The game is %s closed to new players.\r\n", when);
    break;
  default:
    send_to_char(ch, "Only level %d and above may enter the game %s.\r\n", circle_restrict, when);
    break;
  }
}


ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
    send_to_char(ch, "Current machine time: %s\r\n", tmstr);
  else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    send_to_char(ch, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, d == 1 ? "" : "s", h, m);
  }
}



ACMD(do_last)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict = NULL;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "For whom do you wish to search?\r\n");
    return;
  }
  CREATE(vict, struct char_data, 1);
  clear_char(vict);
  CREATE(vict->player_specials, struct player_special_data, 1);
  if (load_char(arg, vict) <  0) {
    send_to_char(ch, "There is no such player.\r\n");
    free_char(vict);
    return;
  }
  if ((GET_LEVEL(vict) > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_IMPL)) {
    send_to_char(ch, "You are not sufficiently godly for that!\r\n");
    return;
  }

  send_to_char(ch, "[%5ld] [%2d %s %s] %-12s : %-18s : %-20s\r\n",
    GET_IDNUM(vict), (int) GET_LEVEL(vict),
    race_abbrevs[(int) GET_RACE(vict)], class_abbrevs[(int) GET_CLASS(vict)],
    GET_NAME(vict), vict->player_specials->host && *vict->player_specials->host
    ? vict->player_specials->host : "(NOHOST)",
    ctime(&vict->player.time.logon));
  free_char(vict);
}


ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char arg[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH];

  half_chop(argument, arg, to_force);

  if (!*arg || !*to_force)
    send_to_char(ch, "Whom do you wish to force do what?\r\n");
  else if ((GET_LEVEL(ch) < LVL_GOD) || (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", NOPERSON);
    else if (  (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict)))
      send_to_char(ch, "No, no, no!\r\n");
    else {
      send_to_char(ch, "%s", OK);
      mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(ch, "%s", OK);
    mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);

    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(ch, "%s", OK);
    mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced all to %s", GET_NAME(ch), to_force);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (STATE(i) != CON_PLAYING || !(vict = i->character) || (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch)))
	continue;
      command_interpreter(vict, to_force);
    }
  }
}



ACMD(do_wiznet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
	buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
  struct descriptor_data *d;
  char any = FALSE;
  int level = LVL_SAINT;
    char *tmstr;
  time_t mytime;
  
   mytime = time(0);

  skip_spaces(&argument);
  delete_doubledollar(argument);
tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  
  
  if (!*argument) {
    send_to_char(ch, "Usage: wiznet <text> | wiz @\r\n");
    return;
  }
  switch (*argument) {
  case '@':
    send_to_char(ch, "God channel status:\r\n");
    for (any = 0, d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || GET_LEVEL(d->character) < LVL_SAINT)
        continue;
      if (!CAN_SEE(ch, d->character))
        continue;

      send_to_char(ch, "  %-*s%s%s%s\r\n", MAX_NAME_LENGTH, GET_NAME(d->character),
		PLR_FLAGGED(d->character, PLR_WRITING) ? " (Writing)" : "",
		PLR_FLAGGED(d->character, PLR_MAILING) ? " (Writing mail)" : "",
		PRF_FLAGGED(d->character, PRF_NOWIZ) ? " (Offline)" : "");
    }
    return;

  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char(ch, "You are offline!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Don't bother the gods like that!\r\n");
    return;
  }
    snprintf(buf1, sizeof(buf1), "[Wiz] %s: %s    <%s>\r\n", GET_NAME(ch), argument, tmstr);
    snprintf(buf2, sizeof(buf1), "[Wiz] Someone: %s\r\n<%s>", argument, tmstr);
  

  for (d = descriptor_list; d; d = d->next) {
    if ((IS_PLAYING(d)) && (GET_LEVEL(d->character) >= level) &&
	(!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
	(!PLR_FLAGGED(d->character, PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(d->character, "%s", CCBGR(d->character, C_NRM));
      if (CAN_SEE(d->character, ch))
	send_to_char(d->character, "%s", buf1);
      else
	send_to_char(d->character, "%s", buf2);
      send_to_char(d->character, "%s", CCNRM(d->character, C_NRM));
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", OK);
}

ACMD(do_impnet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
	buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
  struct descriptor_data *d;
  int level = LVL_IMPL;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "Usage: impnet <text>\r\n");
    return;
  }
  
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Don't bother the Imps like that!\r\n");
    return;
  }
    snprintf(buf1, sizeof(buf1), "[Imp] %s: %s\r\n", GET_NAME(ch), argument);
    snprintf(buf2, sizeof(buf1), "[Imp] Someone: %s\r\n", argument);


  for (d = descriptor_list; d; d = d->next) {
    if ((IS_PLAYING(d)) && (GET_LEVEL(d->character) >= level) && (!PLR_FLAGGED(d->character, PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(d->character, "%s", CCBRD(d->character, C_NRM));
      if (CAN_SEE(d->character, ch))
	send_to_char(d->character, "%s", buf1);
      else
	send_to_char(d->character, "%s", buf2);
      send_to_char(d->character, "%s", CCNRM(d->character, C_NRM));
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", OK);
}

ACMD(do_godnet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
	buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
  struct descriptor_data *d;
  int level = LVL_GOD;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "Usage: godnet <text>\r\n");
    return;
  }
  
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Don't bother the Gods like that!\r\n");
    return;
  }
    snprintf(buf1, sizeof(buf1), "[God] %s: %s\r\n", GET_NAME(ch), argument);
    snprintf(buf2, sizeof(buf1), "[God] Someone: %s\r\n", argument);


  for (d = descriptor_list; d; d = d->next) {
    if ((IS_PLAYING(d)) && (GET_LEVEL(d->character) >= level) && (!PLR_FLAGGED(d->character, PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(d->character, "%s", CCBCN(d->character, C_NRM));
      if (CAN_SEE(d->character, ch))
	send_to_char(d->character, "%s", buf1);
      else
	send_to_char(d->character, "%s", buf2);
      send_to_char(d->character, "%s", CCNRM(d->character, C_NRM));
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", OK);
}

ACMD(do_zreset)
{
  char arg[MAX_INPUT_LENGTH];
  zone_rnum i;
  zone_vnum j;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "You must specify a zone.\r\n");
    return;
  }
  if (*arg == '*') {
    if (GET_LEVEL(ch) != LVL_IMPL)
     {
     mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "(GC) %s tried to reset entire world.", GET_NAME(ch));
     send_to_char(ch, "You do not have permission to do that.\r\n");
     return;
     }
    for (i = 0; i <= top_of_zone_table; i++)
      reset_zone(i);
    send_to_char(ch, "Reset world.\r\n");
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset entire world.", GET_NAME(ch));
    return;
  } else if (*arg == '.')
    i = world[IN_ROOM(ch)].zone;
  else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
	break;
  }
  if (i <= top_of_zone_table) {
    if (GET_LEVEL(ch) <= LVL_BUILDER)
    {
    if (i != GET_OLC_ZONE(ch))
    {
    send_to_char(ch, "You do not have permission to reset that zone.\r\n");
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s tried to reset zone %d (%s) allowed zone %d (%s)", GET_NAME(ch), i, zone_table[i].name, GET_OLC_ZONE(ch), zone_table[GET_OLC_ZONE(ch)].name);
    return;
    }
    }
    reset_zone(i);
    send_to_char(ch, "Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number, zone_table[i].name);
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
  } else
    send_to_char(ch, "Invalid zone number.\r\n");
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */
ACMD(do_wizutil)
{
  char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  long result;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Yes, but for whom?!?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "There is no such player.\r\n");
  else if (IS_NPC(vict))
    send_to_char(ch, "You can't do that to a mob!\r\n");
  else if (GET_LEVEL(vict) > GET_LEVEL(ch))
    send_to_char(ch, "Hmmm...you'd better not.\r\n");
  else {
    switch (subcmd) {
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER)) {
	send_to_char(ch, "Your victim is not flagged.\r\n");
	return;
      }
      REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
      send_to_char(ch, "Pardoned.\r\n");
      send_to_char(vict, "You have been pardoned by the Gods!\r\n");
      mudlog(BRF, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_NOTITLE:
     if (PLR_FLAGGED(vict, PLR_JAIL))
		{
      snprintf(buf, sizeof(buf), "%s is in JAIL.  No title changing.\r\n", GET_NAME(vict));
	  send_to_char(ch, buf);
      return;
		}
 	result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) Notitle %s for %s by %s.",
		ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Notitle %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      mudlog(BRF, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) Squelch %s for %s by %s.",
		ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Squelch %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
	return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Your victim is already pretty cold.\r\n");
	return;
      }
      SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char(vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
      send_to_char(ch, "Frozen.\r\n");
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      mudlog(BRF, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
	send_to_char(ch, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
		GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	return;
      }
      mudlog(BRF, MAX(LVL_DEITY, GET_INVIS_LEV(ch)), TRUE, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char(vict, "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
      send_to_char(ch, "Thawed.\r\n");
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected || AFF_FLAGS(vict)) {
         mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s removed affects from %s.",
		       GET_NAME(ch), GET_NAME(vict));
	while (vict->affected)
	  affect_remove(vict, vict->affected);
	AFF_FLAGS(vict) = 0;
	send_to_char(vict, "There is a brief flash of light!\r\nYou feel slightly different.\r\n");
	send_to_char(ch, "All spells removed.\r\n");
      } else {
	send_to_char(ch, "Your victim does not have any affections!\r\n");
	return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      break;
    }
    save_char(vict, GET_LOADROOM(ch));
  }
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

/* FIXME: overflow possible */
size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone)
{
  return snprintf(bufptr, left,
	"%3d %-30.30s Age: %3d; Reset: %3d (%1d); Range: %5d-%5d\r\n",
	zone_table[zone].number, zone_table[zone].name,
	zone_table[zone].age, zone_table[zone].lifespan,
	zone_table[zone].reset_mode,
	zone_table[zone].bot, zone_table[zone].top);
}


ACMD(do_show)
{
  int i, j, k, l, con, nlen;		/* i, j, k to specifics? */
  size_t len;
  zone_rnum zrn;
  zone_vnum zvn;
  byte self = FALSE;
  struct char_data *vict = NULL;
  struct obj_data *obj;
  struct descriptor_data *d;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH],
	arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];

  struct show_struct {
    const char *cmd;
    const char level;
  } fields[] = {
    { "nothing",	0  },				/* 0 */
    { "zones",		LVL_LIEGE },			/* 1 */
    { "player",		LVL_DEITY },
    { "rent",		LVL_DEITY },
    { "stats",		LVL_DEITY },
    { "errors",		LVL_IMPL },			/* 5 */
    { "death",		LVL_DEITY },
    { "godrooms",	LVL_DEITY },
    { "shops",		LVL_DEITY },
    { "houses",		LVL_DEITY },
    { "snoop",		LVL_GOD },			/* 10 */
	{ "improoms",   LVL_IMPL },
    { "dump",		LVL_DEITY },
    { "tickdump",	LVL_DEITY },
    { "rewarddump",	LVL_DEITY },
	{ "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Show options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
	send_to_char(ch, "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    send_to_char(ch, "\r\n");
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));	/* strcpy: OK (argument <= MAX_INPUT_LENGTH == arg) */

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return;
  }
  if (!strcmp(value, "."))
    self = TRUE;
  buf[0] = '\0';

  switch (l) {
  /* show zone */
  case 1:
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, sizeof(buf), world[IN_ROOM(ch)].zone);
    else if (*value && is_number(value)) {
      for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
      if (zrn <= top_of_zone_table)
	print_zone_to_buf(buf, sizeof(buf), zrn);
      else {
	send_to_char(ch, "That is not a valid zone.\r\n");
	return;
      }
    } else
      for (len = zrn = 0; zrn <= top_of_zone_table; zrn++) {
	nlen = print_zone_to_buf(buf + len, sizeof(buf) - len, zrn);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show player */
  case 2:
    if (!*value) {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }

    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    if (load_char(value, vict) < 0) {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(vict);
      return;
    }
    send_to_char(ch, "Player: %-12s (%s) [%2d %s %s]\r\n", GET_NAME(vict),
      genders[(int) GET_SEX(vict)], GET_LEVEL(vict), class_abbrevs[(int)
      GET_CLASS(vict)], race_abbrevs[(int) GET_RACE(vict)]);
    send_to_char(ch, "Au: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Lessons: %-3d\r\n",
      GET_GOLD(vict), GET_BANK_GOLD(vict), GET_EXP(vict),
      GET_ALIGNMENT(vict), GET_PRACTICES(vict));

    /* ctime() uses static buffer: do not combine. */
    send_to_char(ch, "Started: %-20.16s  ", ctime(&vict->player.time.birth));
    send_to_char(ch, "Last: %-20.16s  Played: %3dh %2dm\r\n",
      ctime(&vict->player.time.logon),
      (int) (vict->player.time.played / 3600),
      (int) (vict->player.time.played / 60 % 60));
    free_char(vict);
    break;

  /* show rent */
  case 3:
    if (!*value) {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }
    Crash_listrent(ch, value);
    break;

  /* show stats */
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;
    send_to_char(ch,
	"Current stats:\r\n"
	"  %5d players in game  %5d connected\r\n"
	"  %5d registered\r\n"
	"  %5d mobiles          %5d prototypes\r\n"
	"  %5d objects          %5d prototypes\r\n"
	"  %5d rooms            %5d zones\r\n"
	"  %5d large bufs\r\n"
	"  %5d buf switches     %5d overflows\r\n",
	i, con,
	top_of_p_table + 1,
	j, top_of_mobt + 1,
	k, top_of_objt + 1,
	top_of_world + 1, top_of_zone_table + 1,
	buf_largecount,
	buf_switches, buf_overflows
	);
    break;

  /* show errors */
  case 5:
    len = strlcpy(buf, "Errant Rooms\r\n------------\r\n", sizeof(buf));
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < NUM_OF_DIRS; j++)
	if (world[i].dir_option[j] && world[i].dir_option[j]->to_room == 0) {
	  nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++k, GET_ROOM_VNUM(i), world[i].name);
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show death */
  case 6:
    len = strlcpy(buf, "Death Traps\r\n-----------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show godrooms */
  case 7:
    len = strlcpy(buf, "Godrooms\r\n--------------------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_GODROOM)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show shops */
  case 8:
    show_shops(ch, value);
    break;

  /* show houses */
  case 9:
    hcontrol_list_houses(ch);
    break;

  /* show snoop */
  case 10:
    i = 0;
    send_to_char(ch, "People currently snooping:\r\n--------------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (d->snooping == NULL || d->character == NULL)
	continue;
      if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
	continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	continue;
      i++;
      send_to_char(ch, "%-10s - snooped by %s.\r\n", GET_NAME(d->snooping->character), GET_NAME(d->character));
    }
    if (i == 0)
      send_to_char(ch, "No one is currently snooping.\r\n");
    break;

 /* show improoms */
  case 11:
    len = strlcpy(buf, "Improoms\r\n--------------------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_IMPROOM)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

 /* show dump */
  case 12:
    len = strlcpy(buf, "Dump Rooms\r\n------------------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DUMP)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

 /* show dump on tick */
  case 13:
    len = strlcpy(buf, "Dump on Tick Rooms\r\n-------------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DUMPONTICK)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

 /* show reward on dump */
  case 14:
    len = strlcpy(buf, "Reward on Dump Rooms\r\n-----------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_REWARDDUMP)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

	/* show what? */
  default:
    send_to_char(ch, "Sorry, I don't understand that.\r\n");
    break;
  }
}


/***************** The do_set function ***********************************/

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT(flagset, flags); \
	else if (off) REMOVE_BIT(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

char buf[MAX_INPUT_LENGTH];

/* The set options available */
  struct set_struct {
    const char *cmd;
    const char level;
    const char pcnpc;
    const char type;
  } set_fields[] = {
   { "brief",		LVL_DEITY, 	PC, 	BINARY },  /* 0 */
   { "invstart", 	LVL_DEITY, 	PC, 	BINARY },  /* 1 */
   { "title",		LVL_DEITY, 	PC, 	MISC },
   { "nosummon", 	LVL_DEITY, 	PC, 	BINARY },
   { "maxhit",		LVL_DEITY, 	BOTH, 	NUMBER },
   { "maxmana", 	LVL_DEITY, 	BOTH, 	NUMBER },  /* 5 */
   { "maxmove", 	LVL_DEITY, 	BOTH, 	NUMBER },
   { "hit", 		LVL_DEITY, 	BOTH, 	NUMBER },
   { "mana",		LVL_DEITY, 	BOTH, 	NUMBER },
   { "move",		LVL_DEITY, 	BOTH, 	NUMBER },
   { "align",		LVL_DEITY, 	BOTH, 	NUMBER },  /* 10 */
   { "str",		LVL_DEITY, 	BOTH, 	NUMBER },
   { "stradd",		LVL_DEITY, 	BOTH, 	NUMBER },
   { "int", 		LVL_DEITY, 	BOTH, 	NUMBER },
   { "wis", 		LVL_DEITY, 	BOTH, 	NUMBER },
   { "dex", 		LVL_DEITY, 	BOTH, 	NUMBER },  /* 15 */
   { "con", 		LVL_DEITY, 	BOTH, 	NUMBER },
   { "cha",		LVL_DEITY, 	BOTH, 	NUMBER },
   { "ac", 		LVL_DEITY, 	BOTH, 	NUMBER },
   { "gold",		LVL_DEITY, 	BOTH, 	NUMBER },
   { "bank",		LVL_DEITY, 	PC, 	NUMBER },  /* 20 */
   { "exp", 		LVL_DEITY, 	BOTH, 	NUMBER },
   { "hitroll", 	LVL_DEITY, 	BOTH, 	NUMBER },
   { "damroll", 	LVL_DEITY, 	BOTH, 	NUMBER },
   { "invis",		LVL_IMPL, 	PC, 	NUMBER },
   { "nohassle", 	LVL_DEITY, 	PC, 	BINARY },  /* 25 */
   { "frozen",		LVL_FREEZE, 	PC, 	BINARY },
   { "practices", 	LVL_DEITY, 	PC, 	NUMBER },
   { "lessons", 	LVL_DEITY, 	PC, 	NUMBER },
   { "drunk",		LVL_DEITY, 	BOTH, 	MISC },
   { "hunger",		LVL_DEITY, 	BOTH, 	MISC },    /* 30 */
   { "thirst",		LVL_DEITY, 	BOTH, 	MISC },
   { "killer",		LVL_DEITY, 	PC, 	BINARY },
   { "thief",		LVL_DEITY, 	PC, 	BINARY },
   { "level",		LVL_GOD, 	BOTH, 	NUMBER },
   { "room",		LVL_IMPL, 	BOTH, 	NUMBER },  /* 35 */
   { "roomflag", 	LVL_GOD, 	PC, 	BINARY },
   { "siteok",		LVL_GOD, 	PC, 	BINARY },
   { "deleted", 	LVL_IMPL, 	PC, 	BINARY },
   { "class",		LVL_GOD, 	BOTH, 	MISC },
   { "nowizlist", 	LVL_DEITY, 	PC, 	BINARY },  /* 40 */
   { "quest",		LVL_DEITY, 	PC, 	BINARY },
   { "loadroom", 	LVL_DEITY, 	PC, 	MISC },
   { "color",		LVL_DEITY, 	PC, 	BINARY },
   { "idnum",		LVL_IMPL, 	PC, 	NUMBER },
   { "passwd",		LVL_GOD, 	PC, 	MISC },    /* 45 */
   { "nodelete", 	LVL_DEITY, 	PC, 	BINARY },
   { "sex", 		LVL_DEITY, 	BOTH, 	MISC },
   { "age",		LVL_DEITY,	BOTH,	NUMBER },
   { "height",		LVL_DEITY,	BOTH,	NUMBER },
   { "weight",		LVL_DEITY,	BOTH,	NUMBER },  /* 50 */
   { "olc",		LVL_DEITY,	PC,	NUMBER },
   { "race",            LVL_DEITY,        BOTH,     MISC },
   { "size",            LVL_DEITY,        BOTH,   NUMBER },
//dan clan system
   { "clan",            LVL_DEITY,        PC,     NUMBER },  
   { "rank",            LVL_DEITY,        PC,     NUMBER }, /*55*/
   { "clan_gold",       LVL_IMPL,       PC,     BINARY },
   { "hometown",        LVL_DEITY,      PC,     NUMBER }, 
   { "\n", 0, BOTH, MISC }
  };


int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg)
{
  char buf[MAX_STRING_LENGTH];
  int i, on = 0, off = 0, value = 0;
  room_rnum rnum;
  room_vnum rvnum;
  int oldlevel;
  /* Check to make sure all the levels are correct */
  if (GET_LEVEL(ch) != LVL_IMPL) {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
      send_to_char(ch, "Maybe that's not such a great idea...\r\n");
      return (0);
    }
  }
  if (GET_LEVEL(ch) < set_fields[mode].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return (0);
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
    send_to_char(ch, "You can't do that to a beast!\r\n");
    return (0);
  } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
    send_to_char(ch, "That can only be done to a beast!\r\n");
    return (0);
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char(ch, "Value must be 'on' or 'off'.\r\n");
      return (0);
    }
    send_to_char(ch, "%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), GET_NAME(vict));
  } else if (set_fields[mode].type == NUMBER) {
    value = atoi(val_arg);
    send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
  } else
  if (mode == 2)
	{
  if (PLR_FLAGGED(vict, PLR_JAIL))
		{
	  snprintf(buf, sizeof(buf), "%s is in JAIL.  No title changing.\r\n", GET_NAME(vict));
	  send_to_char(ch, buf);
      return (0);
	    }
  else
  send_to_char(ch, "%s", OK);
	}
  else
  send_to_char(ch, "%s", OK);

  switch (mode) {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 1:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
    break;
  case 2:
     sprintf(buf, "%c", '*');
     if (strchr(val_arg, *buf) == NULL)
     {
	 send_to_char(ch, "Title has to contain the (*) asterisk sign to be valid.\r\nTitle unchanged\r\n");
        return(0);
       }
    set_title(vict, val_arg);
    send_to_char(ch, "%s's title is now: %s\r\n", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    send_to_char(ch, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 4:
    vict->points.max_hit = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 5:
    vict->points.max_mana = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 6:
    vict->points.max_move = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 7:
    vict->points.hit = RANGE(-9, vict->points.max_hit);
    affect_total(vict);
    break;
  case 8:
    vict->points.mana = RANGE(0, vict->points.max_mana);
    affect_total(vict);
    break;
  case 9:
    vict->points.move = RANGE(0, vict->points.max_move);
    affect_total(vict);
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;
  case 11:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.str = value;
    vict->real_abils.str_add = 0;
    affect_total(vict);
    break;
  case 12:
    vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
    break;
  case 13:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.intel = value;
    affect_total(vict);
    break;
  case 14:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.wis = value;
    affect_total(vict);
    break;
  case 15:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.dex = value;
    affect_total(vict);
    break;
  case 16:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.con = value;
    affect_total(vict);
    break;
  case 17:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.cha = value;
    affect_total(vict);
    break;
  case 18:
    vict->points.armor = RANGE(-100, 100);
    affect_total(vict);
    break;
  case 19:
    GET_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 21:
    vict->points.exp = RANGE(0, 50000000);
    break;
  case 22:
    vict->points.hitroll = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 23:
    vict->points.damroll = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 24:
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
    break;
  case 25:
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict && on) {
      send_to_char(ch, "Better not -- could be a long winter!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 27:
  case 28:
    GET_PRACTICES(vict) = RANGE(0, 100);
    break;
  case 29:
  case 30:
  case 31:
    if (!str_cmp(val_arg, "off")) {
      GET_COND(vict, (mode - 29)) = -1; /* warning: magic number here */
      send_to_char(ch, "%s's %s now off.\r\n", GET_NAME(vict), set_fields[mode].cmd);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, (mode - 29)) = value; /* and here too */
      send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
    } else {
      send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
      return (0);
    }
    break;
  case 32:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 34:
    oldlevel = GET_LEVEL(vict);
    if (value > GET_LEVEL(ch) || value > LVL_IMPL) {
      send_to_char(ch, "You can't do that.\r\n");
      return (0);
    }
    
    if (oldlevel >= LVL_SAINT && value < LVL_SAINT) {
        /* If they are no longer an immortal, let's remove some of the
        * nice immortal only flags, shall we?
        */
        REMOVE_BIT(PRF_FLAGS(vict), PRF_LOG1 | PRF_LOG2);
        REMOVE_BIT(PRF_FLAGS(vict), PRF_NOHASSLE | PRF_HOLYLIGHT);
        run_autowiz();
        perform_immort_vis(vict); /* no more invis 47 morts */
  }

    RANGE(0, LVL_IMPL);
    vict->player.level = value;
    break;
  case 35:
    if ((rnum = real_room(value)) == NOWHERE) {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (0);
    }
    if (IN_ROOM(vict) != NOWHERE)	/* Another Eric Green special. */
      char_from_room(vict);
    char_to_room(vict, rnum);
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 38:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 39:
    if ((i = parse_full_class(val_arg)) == CLASS_UNDEFINED) {
      send_to_char(ch, "That is not a class.\r\n");
      return (0);
    }
    GET_CLASS(vict) = i;
    break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 42:
    if (!str_cmp(val_arg, "off")) {
      REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
    } else if (is_number(val_arg)) {
      rvnum = atoi(val_arg);
      if (real_room(rvnum) != NOWHERE) {
        SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
	GET_LOADROOM(vict) = rvnum;
	send_to_char(ch, "%s will enter at room #%d.", GET_NAME(vict), GET_LOADROOM(vict));
      } else {
	send_to_char(ch, "That room does not exist!\r\n");
	return (0);
      }
    } else {
      send_to_char(ch, "Must be 'off' or a room's virtual number.\r\n");
      return (0);
    }
    break;
  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
    break;
  case 44:
    if (GET_IDNUM(ch) != 1 || !IS_NPC(vict))
      return (0);
    GET_IDNUM(vict) = value;
    break;
  case 45:
    if (GET_IDNUM(ch) > 1) {
      send_to_char(ch, "Please don't use this command, yet.\r\n");
      return (0);
    }
    if (GET_LEVEL(vict) >= LVL_GOD) {
      send_to_char(ch, "You cannot change that.\r\n");
      return (0);
    }
    strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);	/* strncpy: OK (G_P:MAX_PWD_LENGTH) */
    *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
    send_to_char(ch, "Password changed to '%s'.\r\n", val_arg);
    break;
  case 46:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 47:
    if ((i = search_block(val_arg, genders, FALSE)) < 0) {
      send_to_char(ch, "Must be 'male', 'female', or 'neutral'.\r\n");
      return (0);
    }
    GET_SEX(vict) = i;
    break;
  case 48:	/* set age */
    if (value < 2 || value > 200) {	/* Arbitrary limits. */
      send_to_char(ch, "Ages 2 to 200 accepted.\r\n");
      return (0);
    }
    /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
    vict->player.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
    break;

  case 49:	/* Blame/Thank Rick Glover. :) */
    GET_HEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 50:
    GET_WEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 51:
    GET_OLC_ZONE(vict) = value;
    break;

  case 52:
    if ((i = parse_race(*val_arg)) == RACE_UNDEFINED) {
      send_to_char(ch, "That is not a race.\r\n");
      return (0);
    }
    GET_RACE(vict) = i;
    break;

  case 53:
   vict->player.size = RANGE(0, 6);
    break;

//dan clan system
  case 54:
    RANGE(0, cnum);
    if (value == 0)
      GET_CLAN_RANK(vict) = CLAN_NONE;
    GET_CLAN(vict) = value;
    break;

  case 55:
    RANGE(0, CLAN_LEADER);
    if (GET_CLAN(vict) == CLAN_NONE) {
      send_to_char(ch, "Target must be in a clan first.\r\n");
      return (0);
    }
    GET_CLAN_RANK(vict) = value;
    break;
  case 56:
    send_to_char(ch, "Set clan_gold disabled for now.\r\n");
    return (0);
    break;
                                                                                
  case 57:
//    show_hometowns(ch);
    RANGE(0, NUM_STARTROOMS);
    GET_HOME(vict) = value;
    break;

  default:
    send_to_char(ch, "Can't set that!\r\n");
    return (0);
  }

  return (1);
}


ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop(argument, name, buf);
  
  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s set - %s %s.",
		   GET_NAME(ch), name, buf);	

  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob"))
    half_chop(buf, name, buf);

  half_chop(buf, field, buf);

  if (!*name || !*field) {
    send_to_char(ch, "Usage: set <victim> <field> <value>\r\n");
    return;
  }

  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such player.\r\n");
	return;
      }
    } else { /* is_mob */
      if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such creature.\r\n");
	return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    CREATE(cbuf->player_specials, struct player_special_data, 1);
    if ((player_i = load_char(name, cbuf)) > -1) {
      if (GET_LEVEL(cbuf) >= GET_LEVEL(ch)) {
	free_char(cbuf);
	send_to_char(ch, "Sorry, you can't do that.\r\n");
	return;
      }
      vict = cbuf;
    } else {
      free_char(cbuf);
      send_to_char(ch, "There is no such player.\r\n");
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strncmp(field, set_fields[mode].cmd, len))
      break;

  /* perform the set */
  retval = perform_set(ch, vict, mode, buf);

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !IS_NPC(vict))
      save_char(vict, GET_LOADROOM(ch));
    if (is_file) {
      GET_PFILEPOS(cbuf) = player_i;
      save_char(cbuf, GET_LOADROOM(cbuf));
      send_to_char(ch, "Saved in file.\r\n");
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}



ACMD(do_saveall)
{
 if (GET_LEVEL(ch) < LVL_BUILDER)
    send_to_char (ch, "You are not holy enough to use this privelege.\n\r");
 else {
    save_all();
    send_to_char(ch, "World files saved.\n\r");
 }
}

ACMD(do_seize)
{
  struct char_data *victim;
  struct obj_data *obj;
  char buf[MAX_STRING_LENGTH];
  char buf2[80];
  char buf3[80];
  int i, k = 0;

  two_arguments(argument, buf2, buf3);

  if (!*buf2)
    send_to_char(ch, "Syntax: seize <object> <character>.\r\n");
  else if (!(victim = get_char_vis(ch, buf3, NULL, FIND_CHAR_ROOM )))
    send_to_char(ch, "No one by that name here.\r\n");
  else if (victim == ch)
    send_to_char(ch, "Are you sure you're feeling ok?\r\n");
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char(ch, "That's really not such a good idea.\r\n");
  else if (!*buf3)
    send_to_char(ch, "Syntax: seize <object> <character>.\r\n");
  else {
    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(victim, i) && CAN_SEE_OBJ(ch, GET_EQ(victim, i)) &&
         isname(buf2, GET_EQ(victim, i)->name)) {
        obj_to_char(unequip_char(victim, i), victim);
        k = 1;
      }
    }

  if (!(obj = get_obj_in_list_vis(victim, buf2, NULL, victim->carrying))) {
    if (!k && !(obj = get_obj_in_list_vis(victim, buf2, NULL, victim->carrying))) {
      sprintf(buf, "%s does not appear to have the %s.\r\n", GET_NAME(victim),buf2);
      send_to_char(ch, buf);
      return;
    }
  }

  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s seized %s from %s.",
		 GET_NAME(ch), obj->short_description, GET_NAME(victim));

  obj_from_char(obj);
  obj_to_char(obj, ch);
  save_char(ch, GET_LOADROOM(ch));
  save_char(victim, GET_LOADROOM(victim));
  }

}


ACMD(do_peace)
{
  struct char_data *vict, *next_v;
        
  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s used peace in room [%5d].",
		 GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)));
  act ("$n breaks up the fighting.",
     FALSE,ch,0,0,TO_ROOM);
  send_to_char(ch, "Order is restored.\r\n");
  for(vict=world[ch->in_room].people;vict;vict=next_v)
    {
    next_v=vict->next_in_room;
    if (FIGHTING(vict))
    {
      if(FIGHTING(FIGHTING(vict))==vict)
        stop_fighting(FIGHTING(vict));
        stop_fighting(vict);
      }
    }
}

ACMD(do_deitynet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
	buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
  struct descriptor_data *d;
  int level = LVL_DEITY;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "Usage: deitynet <text>\r\n");
    return;
  }
  
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Don't bother the deities like that!\r\n");
    return;
  }
    snprintf(buf1, sizeof(buf1), "[Deity] %s: %s\r\n", GET_NAME(ch), argument);
    snprintf(buf2, sizeof(buf1), "[Deity] Someone: %s\r\n", argument);


  for (d = descriptor_list; d; d = d->next) {
    if ((IS_PLAYING(d)) && (GET_LEVEL(d->character) >= level) && (!PLR_FLAGGED(d->character, PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(d->character, "%s", CCBMG(d->character, C_NRM));
      if (CAN_SEE(d->character, ch))
	send_to_char(d->character, "%s", buf1);
      else
	send_to_char(d->character, "%s", buf2);
      send_to_char(d->character, "%s", CCNRM(d->character, C_NRM));
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", OK);
}

ACMD(do_countnet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
	buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
  struct descriptor_data *d;
  int level = LVL_COUNT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "Usage: countnet <text>\r\n");
    return;
  }
  
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Don't bother the counts like that!\r\n");
    return;
  }
    snprintf(buf1, sizeof(buf1), "[Count] %s: %s\r\n", GET_NAME(ch), argument);
    snprintf(buf2, sizeof(buf1), "[Count] Someone: %s\r\n", argument);


  for (d = descriptor_list; d; d = d->next) {
    if ((IS_PLAYING(d)) && (GET_LEVEL(d->character) >= level) && (!PLR_FLAGGED(d->character, PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(d->character, "%s", CCGRN(d->character, C_NRM));
      if (CAN_SEE(d->character, ch))
	send_to_char(d->character, "%s", buf1);
      else
	send_to_char(d->character, "%s", buf2);
      send_to_char(d->character, "%s", CCNRM(d->character, C_NRM));
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", OK);
}

ACMD(do_liegenet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
	buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
  struct descriptor_data *d;
  int level = LVL_LIEGE;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "Usage: liegenet <text>\r\n");
    return;
  }
  
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Don't bother the landed gentry like that!\r\n");
    return;
  }
    snprintf(buf1, sizeof(buf1), "[Liege] %s: %s\r\n", GET_NAME(ch), argument);
    snprintf(buf2, sizeof(buf1), "[Liege] Someone: %s\r\n", argument);


  for (d = descriptor_list; d; d = d->next) {
    if ((IS_PLAYING(d)) && (GET_LEVEL(d->character) >= level) && (!PLR_FLAGGED(d->character, PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(d->character, "%s", CCBYL(d->character, C_NRM));
      if (CAN_SEE(d->character, ch))
	send_to_char(d->character, "%s", buf1);
      else
	send_to_char(d->character, "%s", buf2);
      send_to_char(d->character, "%s", CCNRM(d->character, C_NRM));
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", OK);
}
