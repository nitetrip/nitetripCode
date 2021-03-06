/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"

/* extern variables */
extern struct spell_info_type spell_info[];
extern const char *class_abbrevs[];
extern const char *race_abbrevs[];
extern const char *clan_names[];
extern const int max_clans;
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern int nameserver_is_slow;
extern int auto_save;
extern int track_through_doors;

/* extern procedures */
void list_skills(struct char_data *ch);
void appear(struct char_data *ch);
void write_aliases(struct char_data *ch);
void perform_immort_vis(struct char_data *ch);
SPECIAL(shop_keeper);
ACMD(do_gen_comm);
void die(struct char_data *ch, struct char_data * killer);
void Crash_rentsave(struct char_data *ch, int cost);
void clanlog(struct char_data *ch, const char *str, ...);

/* local functions */
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_sneak);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_proficiencies);
ACMD(do_visible);
ACMD(do_title);
int perform_group(struct char_data *ch, struct char_data *vict);
void print_group(struct char_data *ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_file);
ACMD(do_detect_traps);
ACMD(do_clan);
ACMD(do_unclan);
ACMD(do_loner);
ACMD(do_promote);
ACMD(do_distant_sight);

ACMD(do_quit)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_SAINT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  } else {
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_SAINT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.", GET_NAME(ch));
    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");
    clanlog(ch, "%s has left the game.", GET_NAME(ch));
    /*  We used to check here for duping attempts, but we may as well
     *  do it right in extract_char(), since there is no check if a
     *  player rents out and it can leave them in an equally screwy
     *  situation.
     */

    if (free_rent)
      Crash_rentsave(ch, 0);

    /* If someone is quitting in their house, let them load back here. */
    if (!PLR_FLAGGED(ch, PLR_LOADROOM) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
      GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    extract_char(ch);		/* Char is saved before extracting. */
  }
}



ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled. This code assumes
     * that guest immortals aren't trustworthy. If you've disabled guest
     * immortal advances from mortality, you may want < instead of <=.
     */
    if (auto_save && GET_LEVEL(ch) <= LVL_SAINT) {
      send_to_char(ch, "Saving aliases.\r\n");
      write_aliases(ch);
      return;
    }
    send_to_char(ch, "Saving %s and aliases.\r\n", GET_NAME(ch));
  }

  write_aliases(ch);
  save_char(ch, GET_LOADROOM(ch));
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}



ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  send_to_char(ch, "Okay, you'll try to move silently for a while.\r\n");
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  percent = rand_number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak + race_app_skill[GET_RACE(ch)].sneak)
    return;

  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  affect_to_char(ch, &af);
}



ACMD(do_hide)
{
  struct affected_type af;
  byte percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  send_to_char(ch, "You attempt to hide yourself.\r\n");

  if (affected_by_spell(ch, SKILL_HIDE))
    affect_from_char(ch, SKILL_HIDE); 

  percent = rand_number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide + race_app_skill[GET_RACE(ch)].hide)
    return;

  af.type = SKILL_HIDE;
  af.duration = GET_LEVEL(ch);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_HIDE;
  affect_to_char(ch, &af);
}




ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Steal what from who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }

  /* 101% is a complete failure */
  percent = rand_number(1, 101) - (dex_app_skill[GET_DEX(ch)].p_pocket + race_app_skill[GET_RACE(ch)].p_pocket);

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS, unless heavy object. */

  if (!pt_allowed && !IS_NPC(vict))
    pcsteal = 1;

  if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
    percent -= 50;

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (GET_LEVEL(vict) >= LVL_SAINT || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char(ch, "Steal the equipment now?  Impossible!\r\n");
	  return;
	} else {
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (percent > GET_SKILL(ch, SKILL_STEAL)) {
	ohoh = TRUE;
	send_to_char(ch, "Oops..\r\n");
	act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {			/* Steal the item */
	if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
	  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char(ch, "Got it!\r\n");
	  }
	} else
	  send_to_char(ch, "You cannot carry that much.\r\n");
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > (GET_SKILL(ch, SKILL_STEAL) + dex_app_skill[GET_DEX(ch)].p_pocket + race_app_skill[GET_RACE(ch)].p_pocket))) {
      ohoh = TRUE;
      send_to_char(ch, "Oops..\r\n");
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some gold coins */
      gold = (GET_GOLD(vict) * rand_number(1, 10)) / 100;
      gold = MIN(1782, gold);
      if (gold > 0) {
	GET_GOLD(ch) += gold;
	GET_GOLD(vict) -= gold;
        if (gold > 1)
	  send_to_char(ch, "Bingo!  You got %d gold coins.\r\n", gold);
	else
	  send_to_char(ch, "You manage to swipe a solitary gold coin.\r\n");
      } else {
	send_to_char(ch, "You couldn't get any gold...\r\n");
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
        hit(vict, ch, TYPE_UNDEFINED);
}

/* For Class Branching */
ACMD(do_progress)
{
  if (IS_NPC(ch))
    return;

  if (!PLR_FLAGGED(ch, PLR_PROGRESS))
	{
  if (GET_LEVEL(ch) == 10)
		{
    send_to_char(ch, "You have already progressed at this level.\r\n");
	return;
		}
  if (GET_LEVEL(ch) == 25)
		{
	send_to_char(ch, "You have already progressed at this level.\r\n");
	return;
		}
  if (GET_LEVEL(ch) < 10)
		{
	send_to_char(ch, "You are not quite ready for that yet.\r\n");
	return;
		}
  if ((GET_LEVEL(ch) > 10) && (GET_LEVEL(ch) < 25))
		{
	send_to_char(ch, "You are not quite ready for that yet.\r\n");
	return;
		}
  if (GET_LEVEL(ch) > 25)
		{
	send_to_char(ch, "You have already achieved your last class.\r\n");
	return;
		}
   }
  else
   send_to_char(ch, "You can only progress at your guild.\r\n");

}

ACMD(do_practice)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only practice skills in your guild.\r\n");
  else
    list_skills(ch);
}

ACMD(do_proficiencies)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "Skill detail not available yet.\r\n");
  else
    list_skills(ch);
}

ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_SAINT) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  } else
    send_to_char(ch, "You are already visible.\r\n");
}



ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch)) {
    send_to_char(ch, "Your title is fine... go away.\r\n");
    return;
    }
  else if (PLR_FLAGGED(ch, PLR_NOTITLE)) {
    send_to_char(ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
    return;
    }
  else if (strstr(argument, "(") || strstr(argument, ")")) {
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
    return;
    }
//  else if (!strstr(argument, "*")){
//       send_to_char(ch, "You must use * for your name in your title.\r\n");
//    return;
//    }

   else if (strlen(argument) > MAX_TITLE_LENGTH) {
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
    return;
    }
  else if (!*argument) {
    send_to_char(ch, "Sorry, set your title to what?\r\n");
    return;
    }
  else
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're title is now: %s.\r\n", GET_TITLE(ch));
 }


int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return (0);

  SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return (1);
}


void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    send_to_char(ch, "But you are not the member of a group!\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    send_to_char(ch, "Your group consists of:\r\n");

    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP)) {
      snprintf(buf, sizeof(buf), "%18s <%s (%s%d) - %d(%d)H %d(%d)M %d(%d)V> (Leader)",
	                         GET_NAME(k), (!IS_NPC(k) ? " PC" : "NPC"), CLASS_ABBR(k), 
                                 GET_LEVEL(k), GET_HIT(k), GET_MAX_HIT(k), 
                                 GET_MANA(k), GET_MAX_MANA(k), GET_MOVE(k), GET_MAX_MOVE(k));
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
	continue;

      snprintf(buf, sizeof(buf), "%18s <%s (%s%d) - %d(%d)H %d(%d)M %d(%d)V>", 
                                 GET_NAME(f->follower), (!IS_NPC(f->follower) ? " PC" : "NPC"), 
                                 CLASS_ABBR(f->follower), GET_LEVEL(f->follower), 
                                 GET_HIT(f->follower), GET_MAX_HIT(f->follower), GET_MANA(f->follower), 
                                 GET_MAX_MANA(f->follower), GET_MOVE(f->follower), GET_MAX_MOVE(f->follower));
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
  }
}



ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You can not enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next)
      found += perform_group(ch, f->follower);
    if (!found)
      send_to_char(ch, "Everyone following you is already in your group.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", NOPERSON);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else if (GET_LEVEL(vict) >= LVL_SAINT)
	{
    act("You can not group immortals.",	FALSE, ch, 0, 0, TO_CHAR);
	return;
	}

  else {
    if (!AFF_FLAGGED(vict, AFF_GROUP))
      perform_group(ch, vict);
    else {
      if (ch != vict)
	act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}



ACMD(do_ungroup)
{
  char buf[MAX_INPUT_LENGTH];
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
      send_to_char(ch, "But you lead no group!\r\n");
      return;
    }

    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
	REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
        act("$N has disbanded the group.", TRUE, f->follower, NULL, ch, TO_CHAR);
        if (!AFF_FLAGGED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char(ch, "You disband the group.\r\n");
    return;
  }
  if (!(tch = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There is no such person!\r\n");
    return;
  }
  if (tch->master != ch) {
    send_to_char(ch, "That person is not following you!\r\n");
    return;
  }

  if (!AFF_FLAGGED(tch, AFF_GROUP)) {
    send_to_char(ch, "That person isn't in your group.\r\n");
    return;
  }

  REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
 
  if (!AFF_FLAGGED(tch, AFF_CHARM))
    stop_follower(tch);
}




ACMD(do_report)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "$n reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      act(buf, TRUE, ch, NULL, f->follower, TO_VICT);

  if (k != ch)
    act(buf, TRUE, ch, NULL, k, TO_VICT);

  send_to_char(ch, "You report to the group.\r\n");
}



ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num, share, rest;
  size_t len;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char(ch, "You don't seem to have that much gold to split.\r\n");
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)))
	num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your gold?\r\n");
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d coins; you receive %d.\r\n",
		GET_NAME(ch), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
		"%d coin%s %s not splitable, so %s keeps the money.\r\n", rest,
		(rest == 1) ? "" : "s", (rest == 1) ? "was" : "were", GET_NAME(ch));
    }
    if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch) &&
		!IS_NPC(k) && k != ch) {
      GET_GOLD(k) += share;
      send_to_char(k, "%s", buf);
    }

    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)) &&
	  f->follower != ch) {

	GET_GOLD(f->follower) += share;
	send_to_char(f->follower, "%s", buf);
      }
    }
    send_to_char(ch, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);

    if (rest) {
      send_to_char(ch, "%d coin%s %s not splitable, so you keep the money.\r\n",
		rest, (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were");
      GET_GOLD(ch) += rest;
    }
  } else {
    send_to_char(ch, "How many coins do you wish to split with your group?\r\n");
    return;
  }
}



ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	return;
      }
      break;
    case SCMD_USE:
      send_to_char(ch, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_wimpy)
{
  float wimp, maxhit, wimpval;
  char wimpbuf[MAX_INPUT_LENGTH];
  int wimpval2;

  char arg[MAX_INPUT_LENGTH];
  int wimp_lev;

  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (IS_NPC(ch))
    return;

  /*
   * Calculations using floats
   */

	one_argument(argument, arg);

  if (!*arg) {
    if (GET_WIMP_LEV(ch)) {

	wimp = GET_WIMP_LEV(ch);
 	maxhit = GET_MAX_HIT(ch);
	wimpval = ((wimp / 100) * maxhit);
    snprintf(wimpbuf, sizeof(wimpbuf), "%f", wimpval);
	wimpval2 = atoi(wimpbuf);

	  send_to_char(ch, "Your current wimpy level is %d%% (%d hps).\r\n", GET_WIMP_LEV(ch), wimpval2);
      return;
    } else {
      send_to_char(ch, "You have not set a wimpy value.\r\n");
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((wimp_lev = atoi(arg)) != 0) {
      if (wimp_lev < 0)
	send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
      else if (wimp_lev > 20)
	send_to_char(ch, "You can't set wimpy above 20%%\r\n");
        else {
	GET_WIMP_LEV(ch) = wimp_lev;
	wimp = GET_WIMP_LEV(ch);
 	maxhit = GET_MAX_HIT(ch);
	wimpval = ((wimp / 100) * maxhit);
    snprintf(wimpbuf, sizeof(wimpbuf), "%f", wimpval);
	wimpval2 = atoi(wimpbuf);
	send_to_char(ch, "Wimpy set to %d%% (%d hps).\r\n", wimp_lev, wimpval2);
      }
    } else {
      send_to_char(ch, "You will no longer wimpy flee.\r\n");
      GET_WIMP_LEV(ch) = 0;
    }
  } else
    send_to_char(ch, "Specify at what percentage of your hit points at which you would like to wimpy flee(0 to disable, max of 20%%).\r\n");
}


ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Mosters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: prompt { { H | M | V | X | T } | all | auto | none }\r\n");
    return;
  }

  if (!str_cmp(argument, "auto")) {
    TOGGLE_BIT(PRF_FLAGS(ch), PRF_DISPAUTO);
    send_to_char(ch, "Auto prompt %sabled.\r\n", PRF_FLAGGED(ch, PRF_DISPAUTO) ? "en" : "dis");
    return;
  }

  if (!str_cmp(argument, "on") || !str_cmp(argument, "all"))
    SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_DISPEXP | PRF_DISPTARGET);
  else if (!str_cmp(argument, "off") || !str_cmp(argument, "none"))
    REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_DISPEXP | PRF_DISPEXP | PRF_DISPTARGET);
  else {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_DISPEXP | PRF_DISPTARGET);

    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'h':
	SET_BIT(PRF_FLAGS(ch), PRF_DISPHP);
	break;
      case 'm':
	SET_BIT(PRF_FLAGS(ch), PRF_DISPMANA);
	break;
      case 'v':
	SET_BIT(PRF_FLAGS(ch), PRF_DISPMOVE);
	break;
      case 't':
    SET_BIT(PRF_FLAGS(ch), PRF_DISPTARGET);
    break;
     case 'x':
    SET_BIT(PRF_FLAGS(ch), PRF_DISPEXP);
    break;
	  default:
	send_to_char(ch, "Usage: prompt { { H | M | V | X | T } | all | auto | none }\r\n");
	return;
      }
    }
  }

  send_to_char(ch, "%s", OK);
}



ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp;
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters can't have ideas - Go away.\r\n");
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "That must be a mistake...\r\n");
    return;
  }
  mudlog(CMP, LVL_SAINT, FALSE, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    return;
  }
  if (fbuf.st_size >= max_filesize) {
    send_to_char(ch, "Sorry, the file is full right now.. try again later.\r\n");
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: do_gen_write");
    send_to_char(ch, "Could not open the file.  Sorry.\r\n");
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	  GET_ROOM_VNUM(IN_ROOM(ch)), argument);
  fclose(fl);
  send_to_char(ch, "Okay.  Thanks!\r\n");
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
  long /* bitvector_t */ result;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
    "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
    "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
    "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"Will no longer track through doors.\r\n",
    "Will now track through doors.\r\n"},
    {"Buildwalk Off.\r\n",
    "Buildwalk On.\r\n"},
    {"AFK flag is now off.\r\n",
    "AFK flag is now on.\r\n"},
    {"You are now deaf to the clan channel.\r\n",
    "You can now hear the clan channel.\r\n"},
    {"You will now hear all clan channels.\r\n",
    "You will no longer see all clan channels.\r\n"},
    {"Autosplit OFF.\r\n",
    "Autosplit ON.\r\n"},


  };


  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (nameserver_is_slow = !nameserver_is_slow);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_TRACK:
    result = (track_through_doors = !track_through_doors);
    break;
  case SCMD_BUILDWALK:
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    break;
  case SCMD_CLANTALK:
    if (!GET_CLAN(ch) && (GET_LEVEL(ch) < LVL_DEITY)) {
      send_to_char(ch, "You don't belong to any clan.\r\n");
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_CLANTALK);
    break;
  case SCMD_ALLCTELL:
    if (GET_LEVEL(ch) < LVL_IMPL) {
      send_to_char(ch, "You are not high enough to listen to all clan channels.\r\n");
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_ALLCTELL);
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
      act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
    else
      act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}

ACMD(do_file)
{
  FILE *req_file;
  int cur_line = 0,
  num_lines = 0,
  req_lines = 0,
  i,
  j;
  int l;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], line[READ_SIZE];
  char buf[MAX_STRING_LENGTH];

  struct file_struct {
    char *cmd;
    char level;
    char *file;
 } fields[] = {
     { "none",           LVL_IMPL,    "Does Nothing" },
     { "bug",            LVL_IMPL,    "../lib/misc/bugs"},
     { "typo",           LVL_IMPL ,   "../lib/misc/typos"},
     { "ideas",          LVL_IMPL,    "../lib/misc/ideas"},
     { "xnames",         LVL_IMPL,     "../lib/misc/xnames"},
     { "levels",         LVL_IMPL,    "../log/levels" },
     { "rip",            LVL_IMPL,    "../log/rip" },
     { "players",        LVL_IMPL,    "../log/newplayers" },
     { "rentgone",       LVL_IMPL,    "../log/rentgone" },
     { "errors",         LVL_IMPL,    "../log/errors" },
     { "godcmds",        LVL_IMPL,    "../log/godcmds" },
     { "syslog",         LVL_IMPL,    "../syslog" },
     { "crash",          LVL_IMPL,    "../syslog.CRASH" },
     { "\n", 0, "\n" }
};

   skip_spaces(&argument);

   if (!*argument) {
     strcpy(buf, "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n");
     for (j = 0, i = 1; fields[i].level; i++)
       if (fields[i].level <= GET_LEVEL(ch))
         sprintf(buf+strlen(buf), "%-15s%s\r\n", fields[i].cmd, fields[i].file);
     send_to_char(ch, buf);
     return;
   }

   two_arguments(argument, field, value);

   for (l = 0; *(fields[l].cmd) != '\n'; l++)
     if (!strncmp(field, fields[l].cmd, strlen(field)))
     break;

   if(*(fields[l].cmd) == '\n') {
     send_to_char(ch, "That is not a valid option!\r\n");
     return;
   }

   if (GET_LEVEL(ch) < fields[l].level) {
     send_to_char(ch, "You are not godly enough to view that file!\r\n");
     return;
   }

   if(!*value)
     req_lines = 15; /* default is the last 15 lines */
   else
     req_lines = atoi(value);

   if (!(req_file=fopen(fields[l].file,"r"))) {
     mudlog(BRF, LVL_IMPL, TRUE,
            "SYSERR: Error opening file %s using 'file' command.",
            fields[l].file);
     return;
   }

   get_line(req_file,line);
   while (!feof(req_file)) {
     num_lines++;
     get_line(req_file,line);
   }
   rewind(req_file);

   req_lines = MIN(MIN(req_lines, num_lines),150);

   buf[0] = '\0';

   get_line(req_file,line);
   while (!feof(req_file)) {
     cur_line++;
     if(cur_line > (num_lines - req_lines))
       sprintf(buf+strlen(buf),"%s\r\n", line);

     get_line(req_file,line);
   }
   fclose(req_file);

   page_string(ch->desc, buf, 1);

}

ACMD(do_detect_traps)
{
  struct obj_data *k;
  int percent;
  bool found_trap = FALSE;

  percent = rand_number(1, 101);
  if (percent <= GET_SKILL(ch, SKILL_DETECT_TRAPS)) {
    for (k = world[IN_ROOM(ch)].contents; k; k = k->next_content) {
      if(GET_OBJ_TYPE(k) == ITEM_TRAP && GET_LEVEL(ch) >= GET_OBJ_LEVEL(k) ) {
        send_to_char(ch, "You spot %s!\r\n", k->short_description);
        return;
        }
    }
   }
  if (found_trap == FALSE)
    send_to_char(ch, "You don't see any traps here.\r\n");
}


ACMD(do_distant_sight)
{

/* This will currently turn off dist sight on eq as well */

if (!GET_SKILL(ch, SKILL_DIST_SIGHT)) {
   send_to_char(ch, "You have no idea how.\r\n");
   return;
  }

if (AFF_FLAGGED(ch, AFF_DIST_SIGHT)) {
    send_to_char(ch, "Your eyesight dulls as you let them rest.\r\n");
    REMOVE_BIT(AFF_FLAGS(ch), AFF_DIST_SIGHT);
    }
else {
SET_BIT(AFF_FLAGS(ch), AFF_DIST_SIGHT);
send_to_char(ch, "Your eyesight sharpens and vision becomes more keen.\r\n");
}

}


void skill_gain(struct char_data *ch, int skill_num)
{
  int gain, temp;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    /* Immortals and the untrained can't improve their skills */
    return;
  temp = (GET_NAT_SKILL(ch, skill_num));
  if ((temp >= MAX_PROFICIENCY) || (temp <= MIN_PROFICIENCY)) return;
  gain = (rand_number(1, GET_NAT_INT(ch)));
  temp = temp + gain;
  if (IS_NPC(ch))
    SET_NAT_MSKILL(ch, skill_num, MIN(MAX_PROFICIENCY, temp));
  else
    SET_NAT_PSKILL(ch, skill_num, MIN(MAX_PROFICIENCY, temp));
//  send_to_char(ch,"Your skill in %s improves and you gain %dXP!\r\n", spell_info[skill_num].name, gain_exp(ch, gain*(GET_LEVEL(ch)+1)));
}

void check_progression(struct char_data *ch)
{
 if (GET_LEVEL(ch) == 10)
                        {
                   send_to_char(ch, "\r\n");
                   send_to_char(ch, "With the sound of the death cry\nringing in your ears you are suddenly\nstruck by a vision of your future. You\nappear stronger, more experienced, and\nmore powerful.  It is time to make an\nimpportant decision, you must meet with\nyour guildmaster before continuing on\nyour quest...\r\n\r\n");

           SET_BIT(PLR_FLAGS(ch), PLR_NOEXPGAIN);
                   SET_BIT(PLR_FLAGS(ch), PLR_PROGRESS);
                        }
           if (GET_LEVEL(ch) == 25)
                        {
                   send_to_char(ch, "\r\n");
                   send_to_char(ch, "With the sound of the death cry\nringing in your ears you are suddenly\nstruck by a vision of your future. You\nappear stronger, more experienced, and\nmore powerful.  It is time to make an\nimpportant decision, you must meet with\nyour guildmaster before continuing on\nyour quest...\r\n\r\n");

           SET_BIT(PLR_FLAGS(ch), PLR_NOEXPGAIN);
                   SET_BIT(PLR_FLAGS(ch), PLR_PROGRESS);
                        }
}


int list_profinciencies(struct char_data *ch, char *argument)
{
  
}
