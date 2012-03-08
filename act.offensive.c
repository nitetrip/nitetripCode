/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"

/* extern variables */
extern int pk_allowed;

/* extern functions */
void raw_kill(struct char_data *ch, struct char_data * killer);
void check_killer(struct char_data *ch, struct char_data *vict);
int compute_armor_class(struct char_data *ch);
bool trap_check(struct char_data *ch, int location);
void skill_gain(struct char_data *ch, int skill_num);

/* local functions */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_backstab);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_bash);
ACMD(do_rescue);
ACMD(do_kick);
ACMD(do_fury);
ACMD(do_disarm);
ACMD(do_circle);

// working on these - 3/7/2012
ACMD(do_parry);
ACMD(do_rage);

/*ACMD(do_rescue);
ACMD(do_retreat);
ACMD(do_shieldpunch);
ACMD(do_shieldrush);
ACMD(do_turn);
ACMD(do_whirlwind); will add later */


ACMD(do_assist)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char(ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
    else
      for (opponent = world[IN_ROOM(ch)].people;
	   opponent && (FIGHTING(opponent) != helpee);
	   opponent = opponent->next_in_room)
		;

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if ((pk_allowed ) && !IS_NPC(opponent))	/* prevent accidental pkill */
      act("Use 'murder' if you really want to attack $N.", FALSE,
	  ch, 0, opponent, TO_CHAR);
    else {
      send_to_char(ch, "You join the fight!\r\n");
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}


ACMD(do_hit)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Hit whom?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "They don't seem to be here.\r\n");
  else if (vict == ch) {
    send_to_char(ch, "You hit yourself...OUCH!\r\n");
    act("$n hits $mself, and says 'OUCH!'", FALSE, ch, 0, vict, TO_ROOM);
  }
  else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.",
        FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (pk_allowed) {
      if (!IS_NPC(vict) && !IS_NPC(ch)) {
	if (subcmd != SCMD_MURDER) {
	  send_to_char(ch, "Use 'murder' to hit another player.\r\n");
	  return;
	} else {
	  check_killer(ch, vict);
	}
      }
      if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
	return;			/* you can't order a charmed pet to attack a
				 * player - Oh..?  FRENZY */
    }

    /* start the fight */
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
        if( (IS_NPC(vict)) || (PLR_FLAGGED(ch, PLR_KILLER))
            || (PLR_FLAGGED(vict, PLR_KILLER)) )
	{
	    hit(ch, vict, TYPE_UNDEFINED);
            WAIT_STATE(ch, PULSE_VIOLENCE + 2);
	}
    }
    else
        send_to_char(ch, "You do the best you can!\r\n");
  }
}



ACMD(do_kill)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (GET_LEVEL(ch) < LVL_IMPL || IS_NPC(ch)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Kill who?\r\n");
  } else {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "They aren't here.\r\n");
    else if (ch == vict)
      send_to_char(ch, "Your mother would be so sad.  :(\r\n");
   else if ((GET_LEVEL(vict) == LVL_IMPL) && !IS_NPC(vict)) {
      send_to_char(vict, "%s tried to slay you!\r\n", GET_NAME(ch));
      send_to_char(ch, "That was a bad idea!!\r\n");
      raw_kill(ch, vict);
      return;
    }
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict, ch);
    }
  }
}



ACMD(do_backstab)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, prob;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  one_argument(argument, buf);

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Backstab who?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "How can you sneak up on yourself?\r\n");
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
    return;
  }
  if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT &&
      GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STING - TYPE_HIT &&
      GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STAB - TYPE_HIT) {
    send_to_char(ch, "Only piercing weapons can be used for backstabbing.\r\n");
    return;
  }
  if (FIGHTING(vict)) {
    send_to_char(ch, "You can't backstab a fighting person -- they're too alert!\r\n");
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(ch, vict, TYPE_UNDEFINED);
    return;
  }

  percent = rand_number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BACKSTAB);

  if (AWAKE(vict) && (percent > prob))
    damage(ch, vict, 0, SKILL_BACKSTAB);
  else
    hit(ch, vict, SKILL_BACKSTAB);

  WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
}
ACMD(do_circle) 
{ 
  char buf[MAX_INPUT_LENGTH]; 
  struct char_data *victim; 
  int percent, prob; 

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_CIRCLE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "You cannot mar the peace of this place with your murderous ways.");
    return;
  }

  one_argument(argument, buf); 

/*  if (!FIGHTING(ch)) {
    send_to_char(ch, "You must be fighting in order t
    '[hdr
    
    
  }
*/
  if ((!*buf) && !FIGHTING(ch)) {
    send_to_char(ch, "Circle behind who?\r\n");
    return;
  }
  else {
  //allow player to circle without a target when fighting
    if (!*buf) {
      victim = FIGHTING(ch);
    }
    else if(!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Circle behind who?\r\n");
      return;
    }
    if (victim == ch) {
      send_to_char(ch, "How do you think you are going to circle behind yourself?\r\n");
      return;
    }
/*    if (PC_VS_PC_NOT_ALLOWED(ch, victim)) {
      send_to_char(ch, "The circle of your treachery is broken! You cannot circle %s,\r\n", GET_NAME(victim));
      return;
    } */
    if (FIGHTING(victim) == ch) {
      send_to_char(ch, "You cannot circle behind %s since %s is directly fighting you!\r\n", GET_NAME(victim), HSSH(victim));
      return;
    }
    if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
    return;
  }
    if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT &&
      GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STING - TYPE_HIT &&
      GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STAB - TYPE_HIT) {
    send_to_char(ch, "Only piercing weapons can be used for circling.\r\n");
    return;
  }
  }

  percent = rand_number(1, 101); /* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_CIRCLE);

  if (AWAKE(victim) && (percent > prob))
    damage(ch, victim, 0, SKILL_CIRCLE);
  else
     hit(ch, victim, SKILL_CIRCLE);

    
/*  if (FIGHTING(victim) == tmp_ch)
    stop_fighting(victim);
  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);
*/
  GET_WAIT_STATE(ch) = 3 * PULSE_VIOLENCE;
}

ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char(ch, "Order who to do what?\r\n");
  else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
    send_to_char(ch, "That person isn't here.\r\n");
  else if (ch == vict)
    send_to_char(ch, "You obviously suffer from skitzofrenia.\r\n");
  else {
    if (AFF_FLAGGED(ch, AFF_CHARM)) {
      send_to_char(ch, "Your superior would not aprove of you giving orders.\r\n");
      return;
    }
    if (vict) {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(ch, "%s", OK);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      for (k = ch->followers; k; k = k->next) {
	if (IN_ROOM(ch) == IN_ROOM(k->follower))
	  if (AFF_FLAGGED(k->follower, AFF_CHARM)) {
	    found = TRUE;
	    command_interpreter(k->follower, message);
	  }
      }
      if (found)
	send_to_char(ch, "%s", OK);
      else
	send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
    }
  }
}



ACMD(do_flee)
{
  int i, attempt, loss;
  struct char_data *was_fighting;

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_FURY)) {
    send_to_char(ch, "Your FURY prevents YOU from fleeing!!\r\n");
    return;
  }

  for (i = 0; i < 6; i++) {
    attempt = rand_number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
    if (CAN_GO(ch, attempt) &&
	!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      was_fighting = FIGHTING(ch);      
      if (!trap_check(ch, attempt) && do_simple_move(ch, attempt, TRUE) ) {
	
	send_to_char(ch, "You flee head over heels to the %s!\r\n", dirs[attempt]);
	if (was_fighting && !IS_NPC(ch)) {
          /* FRENZY - want it like this ? */
	  loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
	  loss *= GET_LEVEL(was_fighting);
	  gain_exp(ch, -loss);
	}
      } else {
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
}


ACMD(do_bash)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BASH)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Bash who?\r\n");
      return;
    }
  }
  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  percent = rand_number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BASH);

  if (MOB_FLAGGED(vict, MOB_NOBASH))
    percent = 101;

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
  } else {
    /*
     * If we bash a player and they wimp out, they will move to the previous
     * room before we set them sitting.  If we try to set the victim sitting
     * first to make sure they don't flee, then we can't bash them!  So now
     * we only set them sitting if they didn't flee. -gg 9/21/98
     */
    if (damage(ch, vict, 1, SKILL_BASH) > 0) {	/* -1 = dead, 0 = miss */
      WAIT_STATE(vict, PULSE_VIOLENCE);
      if (IN_ROOM(ch) == IN_ROOM(vict))
        GET_POS(vict) = POS_SITTING;
    }
  }
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}


ACMD(do_rescue)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict, *tmp_ch;
  int percent, prob;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_RESCUE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Whom do you want to rescue?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "What about fleeing instead?\r\n");
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char(ch, "How can you rescue someone you are trying to kill?\r\n");
    return;
  }
  for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  /* FRENZY - only fail 1 in 101 ... no matter what ? */
  percent = rand_number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_RESCUE);

  if (percent > prob) {
    send_to_char(ch, "You fail the rescue!\r\n");
    return;
  }
  send_to_char(ch, "Banzai!  To the rescue...\r\n");
  act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
    stop_fighting(vict);
  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);

  WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
}



ACMD(do_kick)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, prob;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_KICK)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Kick who?\r\n");
      return;
    }
  }
  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  /* 101% is a complete failure */
  percent = ((10 - (compute_armor_class(vict) / 10)) * 2) + rand_number(1, 101);
  prob = GET_SKILL(ch, SKILL_KICK);

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_KICK);
  } else
    damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_KICK);

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_fury)
{

int prob, conadd = 0, stradd = 0, percent;

if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_FURY)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
    }

 if (GET_POS(ch) != POS_FIGHTING) {
    send_to_char(ch, "You can only harness your fury while in battle!\r\n");
    return;
    }
    
/* FRENZY - Needs to adjust on max abils */

   if (GET_CON(ch) <= 5) 				/* 0(1?)-5 Con */
      conadd = -20;
   if ((GET_CON(ch) >= 6) && (GET_CON(ch) <= 10)) 	/* 6-10 Con */
      conadd = -10;
   if ((GET_CON(ch) >= 11) && (GET_CON(ch) <= 15))  	/* 11-15 Con */
      conadd = 0;
   if (GET_CON(ch) >= 16) 				/* 16+ Up to ..? Con */
      conadd = 10;

   if (GET_STR(ch) <= 5) 				/* 0(1?)-5 Str */
      stradd = -20;
   if ((GET_STR(ch) >= 6) && (GET_STR(ch) <= 10)) 	/* 6-10 Str */
      stradd = -10;
   if ((GET_STR(ch) >= 11) && (GET_STR(ch) <= 15))  	/* 11-15 Str */
      stradd = 0;
   if (GET_STR(ch) >= 16) 				/* 16+ Up to ..? Str */
      stradd = 10;

  percent = conadd + stradd + rand_number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_FURY);

  if (percent > prob) {
    send_to_char(ch, "You cannot concentrate on your foe!\r\n");
    return;
  }

  SET_BIT(AFF_FLAGS(ch), AFF_FURY);
  send_to_char(ch, "Your eyes narrow, your muscles tighten, and you let loose your fury upon your foe!\r\n");
  act("You notice $n's muscles tighten as $e gains a sadistic glaze in $s eyes.", FALSE, ch, 0, 0, TO_NOTVICT);

}

ACMD(do_disarm)
{
  struct obj_data *obj;
  struct char_data *vict;
  char buf[MAX_INPUT_LENGTH];

  one_argument(argument, buf);

  if (!*buf) {
        send_to_char(ch, "Whom do you want to disarm?\r\n");
 return;
  }
  else if (!(vict = get_char_room_vis(ch, buf, NULL))) {
        send_to_char(ch, NOPERSON);
 return;
  }
  else if (!GET_SKILL(ch, SKILL_DISARM)) {
        send_to_char(ch, "You have no idea how.\r\n");
   return;
  }
  else if (vict == ch) {
        send_to_char(ch, "That would be funny to watch.\r\n");
   return;
  }
  else if ((GET_LEVEL(vict) > 40)){
	send_to_char(ch, "Your skills are not adequate to disarm such a foe!\r\n");
   return;
  }
  else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict)) {
        send_to_char(ch, "The thought of disarming your master makes you weep.\r\n");
   return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  else if (!(obj = GET_EQ(vict, WEAR_WIELD)))
        act("$N is unarmed!", FALSE, ch, 0, vict, TO_CHAR);
  else if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_NO_DISARM) ||
      MOB_FLAGGED(vict, MOB_NOBASH) ||
           (rand_number(1, 101) > (!IS_NPC(ch) ?
             GET_SKILL(ch, SKILL_DISARM) : rand_number(0, 100)))) {
        act("You failed to disarm $N and make $M rather angry!", FALSE, ch, 0, vict, TO_CHAR);
        damage(vict, ch, rand_number(1, GET_LEVEL(vict)), TYPE_HIT);
    }
  else if (dice(2, GET_STR(ch)) + GET_LEVEL(ch) <= dice(2, GET_STR(vict)) + GET_LEVEL(vict)) {
        act("Your hand just misses $N's weapon, failing to disarm $M/", FALSE, ch, 0, vict, TO_CHAR);
        act("$N has tried and failed to disarm you!", FALSE, vict, 0, ch, TO_CHAR);
        damage(vict, ch, rand_number(1, GET_LEVEL(vict) / 2), TYPE_HIT);
  } else {
        obj_to_room(unequip_char(vict, WEAR_WIELD), vict->in_room);
        act("You rip $N's weapon from $S hands and it falls the ground!", FALSE, ch, 0, vict, TO_CHAR);
        act("$N smashes your weapon from your hand!", FALSE, vict, obj, ch, TO_CHAR);
        act("$n disarms $N, $p drops to the ground.", FALSE, ch, obj, vict, TO_ROOM);
  }
  hit(ch, vict, TYPE_UNDEFINED);
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_parry)
{
  if (!GET_TOT_SKILL(ch, SKILL_PARRY)) {
    send_to_char(ch, "%s", NOPROFICIENCY);
    return;
  }
  if IS_SET(MSC_FLAGS(ch), MSC_ACTIVE_SKILL_PARRY) {
    send_to_char(ch, "You will no longer parry in combat.\r\n");
    REMOVE_BIT(MSC_FLAGS(ch), MSC_ACTIVE_SKILL_PARRY);
    return;
  }
  else {
    send_to_char(ch, "You will now attempt to parry in combat!\r\n");
    SET_BIT(MSC_FLAGS(ch), MSC_ACTIVE_SKILL_PARRY);
    return;
  }
}

ACMD(do_rage)
{
  int percent, prob;

  if (!GET_TOT_SKILL(ch, SKILL_RAGE)) {
    send_to_char(ch, "%s", NOPROFICIENCY);
    return;
  }
  else if (AFF_FLAGGED(ch, AFF_RAGE) || IS_SET(MSC_FLAGS(ch), MSC_ACTIVE_SKILL_RAGE)) {
    send_to_char(ch, "You're already in a rage!\r\n");
    return;
  }
  if (IS_FIGHTING(ch)) {
    percent = rand_number(1, 101);
    prob = GET_TOT_SKILL(ch, SKILL_RAGE);
    if (percent < prob) {
      SET_BIT(MSC_FLAGS(ch), MSC_ACTIVE_SKILL_RAGE);
      send_to_char(ch, "You draw upon your battle lust and go into a RAGE!\r\n");
      act("$n draws upon $s battle lust and go into a RAGE!", FALSE, ch, 0, ch, TO_ROOM);
    }
    else
      send_to_char(ch, "You try to enter a rage, but can't get excited enough.\r\n");
    if (percent < SG_MIN)
      skill_gain(ch, SKILL_RAGE);
      GET_WAIT_STATE(ch) = PULSE_VIOLENCE; //wait 1 round a failed rage can be attempted again
  }
  else {
    send_to_char(ch, "But you aren't fighting anyone?\r\n");
    return;
  }
}

ACMD(do_whirlwind)
{
  struct char_data *vict, *next_vict;
  int percent, prob;
  int w_wielded_type, dam;
  struct obj_data *wielded = WIELDED_WEAPON(ch);
  int snum_wielded, wskill_wielded;
  /* If player is a mob or skill isn't learned, we can't do this. */
  if (!GET_TOT_SKILL(ch, SKILL_WHIRLWIND)) {
    send_to_char(ch, "%s", NOPROFICIENCY);
    return;
  }
  /* Neither can we do this in a peaceful room */
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "%s", PEACE_ROOM_WARNING);
    return;
  }
  /* And finally, the show costs 30 moves, so the player must be able to pay */
  if (GET_MANA(ch) < BASE_MANA_WHIRLWIND) {
    send_to_char(ch, "You don't have the energy to do that just now!\r\n");
    return;
  }
  /* Now we just need to calculate the chance for sucess before we begin. */
  percent = rand_number(1, 101);                    /* 101% is a complete failure */
  prob = GET_TOT_SKILL(ch, SKILL_WHIRLWIND);
  if (percent > prob) {
    send_to_char(ch, "You fail to complete your whirlwind!\r\n");
    act("$n fails to complete $s whirlwind attack!", FALSE, ch, 0, NULL, TO_NOTVICT);
    return;
  }
 else {
    if (percent < SG_MIN)
      skill_gain(ch, SKILL_WHIRLWIND);
    /* Find the weapon type (for display purposes only) */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
      w_wielded_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
    else {
      if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
        w_wielded_type = ch->mob_specials.attack_type + TYPE_HIT;
      else if (IS_MONK(ch))
        if (rand_number(0,1))
          w_wielded_type = TYPE_PUNCH;
        else
          w_wielded_type = TYPE_KICK;
      else
        w_wielded_type = TYPE_HIT;
    }
    if (wielded)
      snum_wielded = (GET_OBJ_VAL(wielded, 0));
    else
      snum_wielded = SKILL_WP_UNARMED;
    wskill_wielded = GET_TOT_SKILL(ch, snum_wielded);
    /* Find first target, hit it, then move on to next one */
    act("You make a whirlwind attack, wounding ALL of your opponents!", FALSE, ch, 0, NULL, TO_CHAR);
    // act("$n attacks widely about, hitting everyone!", FALSE, ch, 0, NULL, TO_NOTVICT);
    act("$n explodes into a whirlwind of attacking frenzy.", FALSE, ch, 0, NULL, TO_NOTVICT);
    for (vict = world[ch->in_room].people; vict; vict = next_vict) {
      next_vict = vict->next_in_room;
    /*  We'll leave out immortals, players (for !pk muds) and pets from the
        hit list  */
     if (vict == ch)
        continue;
      if (!IS_NPC(vict) && GET_LEVEL(vict) >= LVL_SAINT)
        continue;
      if (!pk_allowed)
        continue;
      dam = get_max_damage_per_hit(ch, FALSE);
      act("You are struck by $N's horrible whirlwind-like attack!", FALSE, vict, 0, ch, TO_CHAR);
//      damage(ch, vict, dam, w_wielded_type, DEATH_MSG_ATTACKER);
	damage(ch,vict, dam, w_wielded_type);
    }
    GET_MANA(ch)=GET_MANA(ch) - BASE_MANA_WHIRLWIND;
  }
  if (IS_NPC(ch)) SET_BIT(MSC_FLAGS(ch), MSC_JUST_ATTACKED);
  GET_WAIT_STATE(ch) = 2 * PULSE_VIOLENCE;
}




