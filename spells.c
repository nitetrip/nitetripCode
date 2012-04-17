/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "genolc.h"
#include "screen.h"

/* external variables */
extern room_rnum recall_room;
extern room_rnum sorin_recall_room; /* mak 5.14.05 */
extern room_rnum arcane_library_room;/* rnum of tp zones              */
extern room_rnum first_forest_room;  /*                               */
extern room_rnum ethereal_plane_room;/*        superfluous? oh well   */
extern room_rnum astral_plane_room;  /*                               */
extern room_rnum shadow_plane_room;  /*                               */ 
extern room_rnum r_mortal_start_room;
extern int mini_mud;
extern int pk_allowed;
extern struct clan_type *clan_info; //dan clan system
// local functions
int adjust_charm_duration(struct char_data *ch, struct char_data *victim, int duration);

/* external functions */
byte saving_throws_nat(int class_num, int type, int level);
void clearMemory(struct char_data *ch);
void weight_change_object(struct obj_data *obj, int weight);
int mag_savingthrow(struct char_data *ch, struct char_data *victim, int type, int modifier);
void name_to_drinkcon(struct obj_data *obj, int type);
void name_from_drinkcon(struct obj_data *obj);
int compute_armor_class(struct char_data *ch);
void weather_change(int change);
void death_cry(struct char_data *ch);
ACMD(do_flee);
ACMD(do_peace);
void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);

/*
 * Special spells appear below.
 */
int adjust_charm_duration(struct char_data *ch, struct char_data *victim, int duration)
{
  if (IS_NECROMANCER(ch)) duration *= (MAX(1, MAX_STAT_ATTRIBUTE - GET_TOT_CHA(ch)));
  else duration *= (MAX(1, GET_TOT_CHA(ch)));
  duration /= MAX(1, (GET_TOT_INT(victim)));
  return duration;
}


ASPELL(spell_blood_quench)
{
  if (IS_NPC(ch) || (GET_LEVEL(ch) >= LVL_IMMORT))      /* Cannot use GET_COND() on mobs. */
    return;
  if (!IS_CORPSE(obj)) {
    send_to_char(ch, "But %s is not a corpse!", GET_OBJ_NAME(obj));
    return;
  }
  if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0)) {
    send_to_char(ch, "Your stomach can't contain anymore!\r\n");
    return;
  }
  act("You drain all the blood from $p, leaving only dust behind.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n drains all the blood from $p, leaving only dust behind.", TRUE, ch, obj, 0, TO_ROOM);
  gain_condition(ch, THIRST, GET_OBJ_VAL(obj, 0));
  empty_container_to_room_then_destroy_it(obj);
}

ASPELL(spell_calm)
{
  do_peace(ch, NULL, 0, 0);
}


ASPELL(spell_cannibalize)
{
  if (IS_NPC(ch) || (GET_LEVEL(ch) >= LVL_IMMORT))      /* Cannot use GET_COND() on mobs. */
    return;
  if (!IS_CORPSE(obj)) {
    send_to_char(ch, "But %s is not a corpse!", GET_OBJ_NAME(obj));
    return;
  }
  if (GET_COND(ch, FULL) > 20) {/* Stomach full */
    send_to_char(ch, "You are too full to eat more!\r\n");
    return;
  }
  act("You shred the corpse to pieces, devouring the meat.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n shreds a corpse to pieces, devouring its meat.", TRUE, ch, obj, 0, TO_ROOM);
  gain_condition(ch, FULL, GET_OBJ_VAL(obj, 0));
  empty_container_to_room_then_destroy_it(obj);
}


ASPELL(spell_control_weather)
{
  int change = 0;
  //if param1 ==1 then make the weather worse, otherwise make it better
  switch(weather_info.sky) {
    case SKY_CLOUDLESS: change = param1 ? 1:0; break;
    case SKY_CLOUDY: change = param1 ? 2:3; break;
    case SKY_RAINING: change = param1 ? 4:5; break;
    case SKY_LIGHTNING: change = param1 ? 0:6; break;
  }
  weather_change(change);
}

ASPELL(spell_create_water)
{
  int water;
  if (ch == NULL || obj == NULL){
     mudlog(BRF, LVL_IMPL, TRUE, "Spell Create Water did not detect an argument");
    return;}
  /* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
      if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	if (GET_OBJ_VAL(obj, 1) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	name_to_drinkcon(obj, LIQ_WATER);
	weight_change_object(obj, water);
	act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}



ASPELL(spell_clan_recall)
{
  struct clan_type *cptr;
  if (victim == NULL)
    return;
  if (IS_NPC(victim))
    if (!(victim->master == ch))
      return;
  for (cptr = clan_info; cptr->number != GET_CLAN(victim); cptr = cptr->next);
  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  if(!IS_NPC(victim)) {
    if(GET_CLAN(victim) > 0)
      char_to_room(victim, real_room(cptr->clan_recall));
    else
      char_to_room(victim, r_mortal_start_room);
  } 

  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
 
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);

  return;
}



ASPELL(spell_recall)
{


  if (victim == NULL || IS_NPC(victim))
    return;

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, real_room(recall_room));
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

ASPELL(spell_sorin_recall)
{


  if (victim == NULL || IS_NPC(victim))
    return;

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, real_room(sorin_recall_room));
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

ASPELL(spell_recharge)
{
  int restored_charges = 0, explode = 0;
  char buf[256];

  if (ch == NULL || obj == NULL)
    return;
  if IS_SET(GET_OBJ_EXTRA(obj), ITEM_NO_RECHARGE) {
    send_to_char(ch, "This item cannot be recharged.\r\n");
    return;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_WAND) {
    if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1)) {
      send_to_char(ch, "You attempt to recharge the wand.\r\n");
      restored_charges = rand_number(1, 2);
      GET_OBJ_VAL(obj, 2) += restored_charges;
      if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1)) {
        send_to_char(ch, "The wand is overcharged and explodes!\r\n");
        sprintf(buf, "%s overcharges %s and it explodes!\r\n", GET_NAME(ch), obj->name);
        act(buf, TRUE, 0, 0, 0, TO_NOTVICT);
        explode = dice(40, 5);
        GET_HIT(ch) -= explode;
        update_pos(ch);
         extract_obj(obj);
        return;
      }
      else {
        sprintf(buf, "You restore %d charges to the wand.\r\n", restored_charges);
        send_to_char(ch, buf);
        return;
      }
    }
    else {
      send_to_char(ch, "That item is already at full charges!\r\n");
      return;
    }
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_STAFF) {
    if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1)) {
      send_to_char(ch, "You attempt to recharge the staff.\r\n");
      restored_charges = rand_number(1, 3);
      GET_OBJ_VAL(obj, 2) += restored_charges;
      if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1)) {
        send_to_char(ch, "The staff is overcharged and explodes!\r\n");
        sprintf(buf, "%s overcharges %s and it explodes!\r\n", GET_NAME(ch), obj->name);
        act(buf, TRUE, 0, 0, 0, TO_NOTVICT);
        explode = dice(40, 5);
        GET_HIT(ch) -= explode;
        update_pos(ch);
        extract_obj(obj);
        return;
      }
  else
      {
        sprintf(buf, "You restore %d charges to the staff.\r\n", restored_charges);
        send_to_char(ch, buf);
        return;
      }
    }
    else {
      send_to_char(ch, "That item is already at full charges!\r\n");
      return;
    }
  }
  else {
    send_to_char(ch, "That item cannot be recharged.\r\n");
    return;
  }
}

ASPELL(spell_arcane_lore)
{


  if (victim == NULL || IS_NPC(victim))
    return;

  act("$n delves deeply into arcane lore.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, real_room(arcane_library_room));
  act("$n has come in search of arcane lore.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}
ASPELL(spell_arboreal_form)
{


  if (victim == NULL || IS_NPC(victim))
    return;

  act("$n's flesh turns green as $e becomes a plant, falling over, spreading vines and leaves, rooting in the ground and becoming the underbrush.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, real_room(first_forest_room));
  act("$n roots $mself in the soil, sending a thick stalk towards the sun, which turns back into flesh.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}
ASPELL(spell_ethereal_projection)
{


  if (victim == NULL || IS_NPC(victim))
    return;

  act("$n grows thin and murky, slowly disappearing into the ether", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, real_room(ethereal_plane_room));
  act("The hazy form of $n condenses in the ether.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

ASPELL(spell_feign_death)
{
  struct char_data *vict, *next_v;
  int mod_save = 0;

  if (GET_POS(ch) == POS_FAKEDEAD) {
    send_to_char(ch, "You are already feigning death!\r\n");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_HASTE)) {
    send_to_char(ch, "You cannot feign death while you're affected by haste!\r\n");
    return;
  }
  act("You let out a cry as you fall to the ground, apparently dead.", FALSE, ch, NULL, 0, TO_CHAR);
  death_cry(ch);
  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v) {
    next_v = vict->next_in_room;
    if (FIGHTING(vict) == ch) {
      mod_save = -((GET_TOT_INT(vict)>>1) + (GET_TOT_WIS(vict)>>1));
      if (mag_savingthrow(ch, vict, SAVING_PETRI, mod_save)) {
        act("$N continues to pound at the dead body of $n!", FALSE, ch, 0, vict, TO_ROOM);
        continue;
      }
      else stop_fighting(vict);
    }
  }
  stop_fighting(ch);
  GET_POS(ch) = POS_FAKEDEAD;
}


ASPELL(spell_fumble)
{
  struct obj_data *wielded_weapon = GET_EQ(victim, WEAR_WIELD);
  struct obj_data *held_weapon = GET_EQ(victim, WEAR_HOLD);

  if (victim == NULL)
    return;
  if (!pk_allowed) {
    send_to_char(ch, "%s", NOEFFECT_PC);
    return;
  }
  if ((GET_LEVEL(victim) >= (LVL_IMMORT - 1)) || mag_savingthrow(ch, victim, SAVING_SPELL, 0)) {
    send_to_char(ch, "%s", NOEFFECT);
    return;
  }

  if (held_weapon && GET_OBJ_TYPE(held_weapon) == ITEM_WEAPON) {
    if (IS_SET(GET_OBJ_EXTRA(held_weapon), ITEM_NO_DISARM) || IS_SET(GET_OBJ_EXTRA(held_weapon), ITEM_NODROP))
      send_to_char(ch, "%s", NOEFFECT);
    else {
      act("$N fumbles and drops $s $p!", FALSE, ch, held_weapon, victim, TO_CHAR);
      act("$n causes you to fumble and drop your $p!", FALSE, ch, held_weapon, victim, TO_VICT);
      act("$N fumbles and drops $s $p!", FALSE, ch, held_weapon, victim, TO_NOTVICT);
      obj_to_room(unequip_char(victim, WEAR_HOLD), IN_ROOM(victim));
    }
  }
  else if (wielded_weapon && GET_OBJ_TYPE(wielded_weapon) == ITEM_WEAPON) {
    if (IS_SET(GET_OBJ_EXTRA(wielded_weapon), ITEM_NO_DISARM) || IS_SET(GET_OBJ_EXTRA(wielded_weapon), ITEM_NODROP))
      send_to_char(ch, "%s", NOEFFECT);
    else {
      act("$N fumbles and drops $s $p!", FALSE, ch, wielded_weapon, victim, TO_CHAR);
      act("$n causes you to fumble and drop your $p!", FALSE, ch, wielded_weapon, victim, TO_VICT);
      act("$N fumbles and drops $s $p!", FALSE, ch, wielded_weapon, victim, TO_NOTVICT);
      obj_to_room(unequip_char(victim, WEAR_WIELD), IN_ROOM(victim));
    }
  }
  else
    send_to_char(ch, "%s is not using any weapons.\r\n", CAP(GET_NAME(victim)));
}


ASPELL(spell_astral_projection)
{


  if (victim == NULL || IS_NPC(victim))
    return;

  act("$n gets a starry look in $s eyes, as a white light ascends from $m to the heavens.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, real_room(astral_plane_room));
  act("A beam of white light fixes in the firmament, and the thin outline of $n emerges.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

ASPELL(spell_hang)
{


  if (victim == NULL || IS_NPC(victim))
    return;

  act("$n slips $s head into a secure noose, and leaps into eternity. $n's eyes pop out of the sockets, $s face turns a sickly purple, and there is a snap like a branch wrapped in a wet towel breaking. With a jerk $n's body goes stiff as $s soul is liberated. ", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, real_room(shadow_plane_room));
  act("A gamey scent turns your nose as $n shudders into the body of a dead soul.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}


ASPELL(spell_phase_door)
{
  if ((param1 != NOWHERE) && IS_VALID_EXIT(ch, param1) && EXIT_FLAGGED(EXIT(ch, param1), EX_CLOSED) && (EXIT(ch, param1)->to_room != NOWHERE)) {
    int from_room = IN_ROOM(ch);
    int to_room = EXIT(ch, param1)->to_room;
    const char *doorname = EXIT(ch, param1)->keyword;
    send_to_char(ch, "You phase %s through the %s.\r\n", dirs[param1], doorname);
    char_from_room(ch);
    char_to_room(ch, to_room);
    send_to_room(from_room, "%s phases %s through the %s.\r\n", GET_NAME(ch), dirs[param1], doorname);
    act("$n's body becomes substantial as $e phases into the room.", FALSE, ch, obj, 0, TO_ROOM);
    look_at_room(IN_ROOM(ch), ch, 0);
    entry_memory_mtrigger(ch);
    greet_mtrigger(ch, -1);
    greet_memory_mtrigger(ch);
  }
  else
    send_to_char(ch, "You can't phase through that.\r\n");
}


ASPELL(spell_teleport)
{
  room_rnum to_room;

  if (victim == NULL || IS_NPC(victim))
    return;

  do {
    to_room = rand_number(0, top_of_world);
  } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM));

  act("$n slowly fades out of existence and is gone.",
      FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, to_room);
  act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
  
}

ASPELL(spell_teleportm)
{
  room_rnum to_room;

  if (victim == NULL) victim = ch;
  if ((IN_ROOM(victim) == IN_ROOM(ch)) && !IS_NPC(victim)) {
    do {
      to_room = rand_number(0, top_of_world);
    } while (!CAN_USE_ROOM(victim, to_room));
  }
  else to_room = IN_ROOM(victim);
  if (!CAN_USE_ROOM(ch, to_room) || !CAN_USE_ROOM(victim, to_room)) //(this is for plane support) || !is_on_same_plane(IN_ROOM(ch), to_room)) 
  {
    send_to_char(ch, "The room is protected by powerful magic.\r\n");
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char(ch, "You cannot involve NPCs in teleportation.\r\n");
    return;
  }
  if ((IN_ROOM(victim) == IN_ROOM(ch)) && !IS_NPC(victim)) {
 // act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
    if (FIGHTING(victim)) {
      send_to_char(ch, "You cannot teleport %s while %s busy fighting!\r\n", GET_NAME(victim), HSSH(victim));
      return;
    }
    else
      ch = victim;
  }
  act("$n slowly fades out of existence and is gone.", FALSE, ch, 0, victim, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, to_room);
  act("$n arrives suddenly accompanied by the smell of ozone.", FALSE, ch, 0, 0, TO_ROOM);
  look_at_room(IN_ROOM(ch), ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}


ASPELL(spell_teleview)
{
  send_to_char(ch, "You close your eyes and project your thoughts...\r\n");
  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_PRIVATE | ROOM_GODROOM))
    send_to_char(ch, "Powerful magic confines your thoughts.\r\n");
  else if (IN_ROOM(ch) == IN_ROOM(victim)) send_to_char(ch, "You are already there!!!\r\n");
  else if (victim == NULL) send_to_char(ch, "You cannot sense anything.\r\n");
  else look_at_room(IN_ROOM(victim), ch, 0);
  send_to_char(ch, "You come out of your trance momentarily lost.\r\n");
}

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
{
  if (ch == NULL || victim == NULL)
    return;
 if (GET_LEVEL(ch) < LVL_GOD){
    if (GET_LEVEL(victim) > MIN(LVL_SAINT - 1, level + 3)) {
      send_to_char(ch, "%s", SUMMON_FAIL);
      return; }

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOMAGIC)) {
    send_to_char(ch, "Powerful magic prevents your summon.\r\n");
    return; }

  if (!pk_allowed) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and $N travels\r\n"
	  "through time and space towards you, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely send $M back.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return; }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) && !PLR_FLAGGED(victim, PLR_KILLER)) {
      send_to_char(victim, "%s just tried to summon you to: %s.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to summon you.\r\n",
	      GET_NAME(ch), world[IN_ROOM(ch)].name,
	      (ch->player.sex == SEX_MALE) ? "He" : "She");

      send_to_char(ch, "You failed because %s has summon protection on.\r\n", GET_NAME(victim));
      mudlog(BRF, LVL_SAINT, TRUE, "%s failed summoning %s to %s.", GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
      return; }
  }

  if (MOB_FLAGGED(victim, MOB_NOSUMMON) || (IS_NPC(victim) && mag_savingthrow(ch, victim, SAVING_SPELL, 0))) {
    send_to_char(ch, "%s", SUMMON_FAIL);
    return; }
  if (ROOM_MIN_LEVEL(IN_ROOM(ch)) != 0 && GET_LEVEL(victim) < ROOM_MIN_LEVEL(IN_ROOM(ch)) ) { // min 0 means no min level
     act("$N does not belong in this room!", FALSE, ch, 0, victim, TO_CHAR); // Victim does not meet min level requirements
     return; }

  if (ROOM_MAX_LEVEL(IN_ROOM(ch)) !=0 &&  GET_LEVEL(victim) > ROOM_MAX_LEVEL(IN_ROOM(ch))){ // max 0 means no max level
    act("$N does not belong in this room!", FALSE, ch, 0, victim, TO_CHAR); // Victim does not meet max level requirements
    return; }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GODROOM) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_IMPROOM)){ // To make it easy, no summoning into GODROOMs and IMPROOMs
    send_to_char(ch, "This room is too godly to play around with that kind of magic!!\r\n");
    return; }
 }
  act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, IN_ROOM(ch));

  act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);

}



ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH];
  int j;
  int itemnum = 1;

  /* 
   * FIXME: This is broken.  The spell parser routines took the argument
   * the player gave to the spell and located an object with that keyword.
   * Since we're passed the object and not the keyword we can only guess
   * at what the player originally meant to search for. -gg
   */

 /* use the temporary object that do_cast created */
  if (!obj->name) {
    send_to_char(ch, "What object would you like to find?\r\n");
    free(obj);
    return;
  }
  strcpy(name, obj->name);
  free(obj->name);
  free(obj);

  j = level / 2;

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname(name, i->name))
      continue;

    send_to_char(ch, "[%d.]  ", itemnum);
    send_to_char(ch, "%c%s", UPPER(*i->short_description), i->short_description + 1);

    if (i->carried_by)
      send_to_char(ch, " is being carried by %s.\r\n", PERS(i->carried_by, ch));
    else if (IN_ROOM(i) != NOWHERE)
      send_to_char(ch, " is in %s.\r\n", world[IN_ROOM(i)].name);
    else if (i->in_obj)
      send_to_char(ch, " is in %s.\r\n", i->in_obj->short_description);
    else if (i->worn_by)
      send_to_char(ch, " is in use somewhere in this world.\r\n");
    else
      send_to_char(ch, "'s location is uncertain.\r\n");

    j--;
    itemnum++;
  }

  if (j == level / 2)
    send_to_char(ch, "You sense nothing.\r\n");
}



ASPELL(spell_charm)
{
  struct affected_type af;

  if (victim == NULL || ch == NULL)
    return;

  if(!IS_NPC(victim)) /* Don't charm an NPC */
   {
   send_to_char(ch, "You cannot charm PCs!\r\n");
   return;
   }
  if (victim == ch)
    send_to_char(ch, "You like yourself even better!\r\n");
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char(ch, "Your victim resists!\r\n");
  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "You can't have any followers of your own!\r\n");
  else if (AFF_FLAGGED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
    send_to_char(ch, "You fail.\r\n");
  else if (IS_ELF(victim) && rand_number(1, 100) <= 90)
    send_to_char(ch, "Your victim resists!\r\n");
  else if (circle_follow(victim, ch))
    send_to_char(ch, "Sorry, following in circles can not be allowed.\r\n");
  else if (mag_savingthrow(ch, victim, SAVING_PARA, 0))
    send_to_char(ch, "Your victim resists!\r\n");
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);

    af.type = SPELL_CHARM;
    af.duration = 24 * 2;
    if (GET_CHA(ch))
      af.duration *= GET_CHA(ch);
    if (GET_INT(victim))
      af.duration /= GET_INT(victim);
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim)){
      REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
      SET_BIT(MOB_FLAGS(victim), MOB_SENTINEL);
    }
  }
}



ASPELL(spell_identify)
{
  int i, found;
  size_t len;

  if (obj) {
    char bitbuf[MAX_STRING_LENGTH];

    sprinttype(GET_OBJ_TYPE(obj), item_types, bitbuf, sizeof(bitbuf));
    send_to_char(ch, "You feel informed:\r\nObject '%s', Item type: %s\r\n", obj->short_description, bitbuf);

    if (GET_OBJ_AFFECT(obj)) {
      sprintbit(GET_OBJ_AFFECT(obj), affected_bits, bitbuf, sizeof(bitbuf));
      send_to_char(ch, "Item will give you following abilities:  %s\r\n", bitbuf);
    }

    sprintbit(GET_OBJ_EXTRA(obj), extra_bits, bitbuf, sizeof(bitbuf));
    send_to_char(ch, "Item is: %s\r\n", bitbuf);

    send_to_char(ch, "Weight: %d, Value: %d, Rent: %d, Min Level: %d\r\n", GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_LEVEL(obj));

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      len = i = 0;

      if (GET_OBJ_VAL(obj, 1) >= 1) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, 1)));
        if (i >= 0)
          len += i;
      }

      if (GET_OBJ_VAL(obj, 2) >= 1 && len < sizeof(bitbuf)) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, 2)));
        if (i >= 0)
          len += i;
      }

      if (GET_OBJ_VAL(obj, 3) >= 1 && len < sizeof(bitbuf)) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, 3)));
        if (i >= 0)
          len += i;
      }

      send_to_char(ch, "This %s casts: %s\r\n", item_types[(int) GET_OBJ_TYPE(obj)], bitbuf);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      send_to_char(ch, "This %s casts: %s\r\nIt has %d maximum charge%s and %d remaining.\r\n",
		item_types[(int) GET_OBJ_TYPE(obj)], skill_name(GET_OBJ_VAL(obj, 3)),
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s", GET_OBJ_VAL(obj, 2));
      break;
    case ITEM_WEAPON:
      send_to_char(ch, "Damage Dice is '%dD%d' for an average per-round damage of %.1f.\r\n",
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), ((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1));
      break;
    case ITEM_ARMOR:
      send_to_char(ch, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
      break;
    }
    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
	  (obj->affected[i].modifier != 0)) {
	if (!found) {
	  send_to_char(ch, "Can affect you as :\r\n");
	  found = TRUE;
	}
	sprinttype(obj->affected[i].location, apply_types, bitbuf, sizeof(bitbuf));
	send_to_char(ch, "   Affects: %s By %d\r\n", bitbuf, obj->affected[i].modifier);
      }
    }
  } else if (victim) {		/* victim */
    send_to_char(ch, "Name: %s\r\n", GET_NAME(victim));
    if (!IS_NPC(victim))
      send_to_char(ch, "%s is %d years, %d months, %d days and %d hours old.\r\n",
	      GET_NAME(victim), age(victim)->year, age(victim)->month,
	      age(victim)->day, age(victim)->hours);
    send_to_char(ch, "Height %d cm, Weight %d pounds\r\n", GET_HEIGHT(victim), GET_WEIGHT(victim));
    send_to_char(ch, "Level: %d, Hits: %d, Mana: %d\r\n", GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
    send_to_char(ch, "AC: %d, Hitroll: %d, Damroll: %d\r\n", compute_armor_class(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
    send_to_char(ch, "Str: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
	GET_STR(victim), GET_ADD(victim), GET_INT(victim),
	GET_WIS(victim), GET_DEX(victim), GET_CON(victim), GET_CHA(victim));
  }
}



/*
 * Cannot use this spell on an equipped object or it will mess up the
 * wielding character's hit/dam totals.
 */
ASPELL(spell_enchant_weapon)
{ // FIXME - let's make a wow - type enchanting system, possibly need reagents
  int i;
  if (ch == NULL || obj == NULL) {
     return;}

  /* Either already enchanted or not a weapon. */
  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
     send_to_char(ch, "You can only enchant weapons!\r\n");
     return; } 

  if (OBJ_FLAGGED(obj, ITEM_MAGIC)) {
     send_to_char(ch, "Magic already flows through this weapon.\r\n");
     return; }

   // Make sure there are no other affections.
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj->affected[i].location != APPLY_NONE){
     send_to_char(ch, "This weapon has been enchanted by other magic!\r\n");
     return;}

  SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj->affected[0].location = APPLY_HITROLL;
  obj->affected[0].modifier = 1 + (level >= 18);

  obj->affected[1].location = APPLY_DAMROLL;
  obj->affected[1].modifier = 1 + (level >= 20);

  if (IS_GOOD(ch)) {
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
    act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
  } else if (IS_EVIL(ch)) {
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
    act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
  } else
    act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
}


ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (AFF_FLAGGED(victim, AFF_POISON))
        send_to_char(ch, "You can sense poison in your blood.\r\n");
      else
        send_to_char(ch, "You feel healthy.\r\n");
    } else {
      if (AFF_FLAGGED(victim, AFF_POISON))
        act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
      else
        act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    }
  }

  if (obj) {
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
      else
	act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
	    TO_CHAR);
      break;
    default:
      send_to_char(ch, "You sense that it should not be consumed.\r\n");
    }
  }
}

ASPELL(spell_knock)
{
  do_doorcmd(ch, obj, param1, SCMD_UNLOCK);
}



ASPELL(spell_rest_in_peace)
{
  if (AFF_FLAGGED(ch, AFF_HASTE)) {
    send_to_char(ch, "You cannot rest in peace while you're affected by haste!\r\n");
    return;
  }
  if (is_on_water(ch) && !AFF_FLAGGED(ch, AFF_WATERBREATH)) {
    send_to_char(ch, "You cannot rest in peace underwater unless you can breath it!\r\n");
    return;
  }
  act("You dig a shallow grave and lay down in it.", FALSE, ch, NULL, 0, TO_CHAR);
  act("$n digs a shallow grave and lays down in it.", FALSE, ch, NULL, 0, TO_ROOM);
  GET_POS(ch) = POS_BURIED;
}

ASPELL(spell_scry)
{
  switch (spellnum) {
    case SPELL_SCRY_LESSER:
      act("\\c0lBYou open up a shimmering blue portal and gaze into it...\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
      act("\\c0lBA shimmering blue portal appears out of nowhere briefly before your eyes.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case SPELL_SCRY_GREATER:
      act("\\c0lRYou open up a shimmering red portal and gaze into it...\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
      act("\\c0lRA shimmering red portal appears out of nowhere briefly before your eyes.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
      break;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOSCRY) || ROOM_FLAGGED(param1, ROOM_NOSCRY) || !CAN_USE_ROOM(ch, param1) /*|| !is_on_same_plane(IN_ROOM(ch), param1)*/ || (param1 == NOWHERE))
    send_to_char(ch, "Your eyes swim in images of color and light.  You try with all your\r\n"
                     "intelligence to make something out of the images, but your mind screams\r\n"
                     "out in agony as the portal disappears.\r\n");
  else if (IN_ROOM(ch) == param1) send_to_char(ch, "You are already there!!!\r\n");
  else look_at_room(param1, ch, 0);
}

ASPELL(spell_spook)
{

 /*
  * I don't know about all this ...
  * For spooking PCs it depends on
  * The spookees INT and SAV_SPELL
  *
  * But for spooking NPCs it depends
  * on Skill % of the spooker
  *
  * In short, spooking PCs has nothing
  * to do with how well the spooker has
  * practiced spook but when spooking NPCs
  * it does.
  *
  *  this is very different from cwe, will compar at a later date - march 12, 2012
  */

 if (ch == NULL || victim == NULL)
    return;

 int percent, prob, intadd=0;

 percent = rand_number(1, 101);

 if (!IS_NPC(ch)) { /* PCs and NPCs have different Savs */


   
   if (GET_INT(victim) <= 5) 				        /* 0(1?)-5 Int */
      intadd = -20;
   if ((GET_INT(victim) >= 6) && (GET_INT(victim) <= 10)) 	/* 6-10 Int */
      intadd = -10;
   if ((GET_INT(victim) >= 11) && (GET_INT(victim) <= 15))  	/* 11-15 Int */
      intadd = 5;
   if (GET_INT(victim) >= 16) 				        /* 16+ Up to ..? Int */
      intadd = 10;


   if (((saving_throws_nat(GET_CLASS(victim), SAVING_SPELL, GET_LEVEL(victim)) + GET_SAVE(victim, 4)) - intadd) > percent) {
 act("$n places fear in the heart of $N and $E RUNS!", FALSE, ch, 0, victim, TO_NOTVICT);
 act("$n's gazes meets yours and fear strikes your heart, you RUN!", FALSE, ch, 0, victim, TO_VICT);
 act("Your gaze meets $N's and fear strikes $S heart, $E RUNS!", FALSE, ch, 0, victim, TO_CHAR);
 do_flee(victim, NULL, 0, 0);
 }
 else  {
 act("$n attempts to place fear in the heart of $N but fails!", FALSE, ch, 0, victim, TO_NOTVICT);
 act("$n's gaze meets yours and $e attempts to frighten you, but you are not afraid!", FALSE, ch, 0, victim, TO_VICT);
 act("Your gaze meets $N's and $E sneers back defiantly!", FALSE, ch, 0, victim, TO_CHAR);
 }

 } /* End of PC */
 else { /* NPCs have Savs of 0 by default .. we can't let 'em never get spooked, can we? */

 prob = GET_SKILL(ch, SPELL_SPOOK);

if ( percent > prob ) {
 act("$n attempts to place fear in the heart of $N but fails!", FALSE, ch, 0, victim, TO_NOTVICT);
 act("$n's gaze meets yours and $e attempts to frighten you, but you are not afraid!", FALSE, ch, 0, victim, TO_VICT);
 act("Your gaze meets $N's and $E sneers back defiantly!", FALSE, ch, 0, victim, TO_CHAR);
 }
 else {
 act("$n places fear in the heart of $N and $E RUNS!", FALSE, ch, 0, victim, TO_NOTVICT);
 act("$n's gazes meets yours and fear strikes your heart, you RUN!", FALSE, ch, 0, victim, TO_VICT);
 act("Your gaze meets $N's and fear strikes $S heart, $E RUNS!", FALSE, ch, 0, victim, TO_CHAR);
 do_flee(victim, NULL, 0, 0);
 }
} /* End of NPC */
}

ASPELL(spell_vitality)
{
  int nutrient_amount = 4+(GET_LEVEL(ch)/4);
 //  gain_condition(victim, FULL, nutrient_amount);
 // gain_condition(victim, THIRST, nutrient_amount);
  if (victim != ch) send_to_char(ch, "%s has been revitalized.\r\n", CAP(GET_NAME(victim)));
  act("You feel your hunger suddenly disappear.", TRUE, victim, 0, 0, TO_CHAR);
  GET_COND(victim, THIRST) = MIN(24, GET_COND(victim, THIRST) + nutrient_amount);
  GET_COND(victim, FULL) = MIN(24, GET_COND(victim, FULL)+ nutrient_amount);
   if ((GET_COND(victim, THIRST)>20) && (GET_COND(victim, FULL)>20)) {
    act("$n burps loudly.", TRUE, victim, 0, 0, TO_ROOM);
    act("You burp loudly.", TRUE, victim, 0, 0, TO_CHAR);
  }
}

ASPELL(spell_stun)
{
  if (GET_LEVEL(victim)>=LVL_IMMORT) {
    send_to_char(ch, "You're trying to stun an immortal? Good luck!\r\n");
    return;
  }
  if (pk_allowed) {
    send_to_char(ch, "%s", NOEFFECT_PC);
    return;
  }
  if (GET_POS(victim) <= POS_STUNNED) {
    send_to_char(ch, "%s can't be stunned cause %s's already in bad shape!\r\n", CAP(GET_NAME(victim)), HSSH(victim));
    return;
  }
  if ((!mag_savingthrow(ch, victim, SAVING_PARA, affected_by_spell(ch, SPELL_INTIMIDATE) ? 15 : 0)) || (GET_TOT_INT(victim) < 6)) {
    send_to_char(ch, VICTIM_RESISTS);
    return;
  }
  GET_STUN_RECOVER_CHANCE(victim) = 100;
  GET_STUN_DURATION(victim) = 2;
  GET_POS(victim) = POS_STUNNED;
  act("You are stunned!", FALSE, victim, 0, ch, TO_CHAR);
  act("$n seems to be frozen!", TRUE, victim, 0, ch, TO_ROOM);
  // act("$N freezes in $S tracks due to your massive onslaught!", FALSE, ch, 0, victim, TO_CHAR);
  // act("$N freezes in $S tracks due to $n's massive onslaught!", FALSE, ch, 0, victim, TO_ROOM);
}

ASPELL(spell_succor)
{
  if ((GET_LEVEL(victim) >= LVL_IMMORT) && (GET_LEVEL(ch) <= GET_LEVEL(victim)) ){
     send_to_char(ch, "%sFoolish mortal, you shouldn't mess with the Gods!%s\r\n", CCWHT(ch, C_CMP), CCNRM(ch, CMP));
     victim = ch;}
  if (victim == NULL || IS_NPC(victim)){
     send_to_char(ch, "You can only succor other players!\r\n");
    return;}

  if (IS_FIGHTING(victim)){
    send_to_char(ch, "You cannot succor %s while %s is engaged in combat!\r\n", GET_NAME(victim), HSSH(victim));
    return;}

   act("$n vanishes in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
   char_from_room(victim);
   char_to_room(victim, real_room(CONFIG_SORCERERS_GUILD));
   act("$n appears in a puff of smoke.", TRUE, victim, 0, 0, TO_ROOM);

  look_at_room(IN_ROOM(victim), victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}


ASPELL(spell_bind_portal)
{
  char encrypted_string[64];
  int length;
  long encrypted_location;
  //encrypted_location = (long)IN_ROOM(ch)+(long)GET_IDNUM(ch)+(long)spellnum;
  encrypted_location = (((room_vnum)GET_ROOM_VNUM(IN_ROOM(ch))+(long)GET_IDNUM(ch))*(long)spellnum)<<2;
  length = sprintascii(encrypted_string, encrypted_location);
  switch (spellnum) {
    case SPELL_BEFRIEND_DRYAD:
     /* if (!IS_IN_NATURE(IN_ROOM(ch)) || !CAN_USE_ROOM(ch, IN_ROOM(ch))) {
        send_to_char(ch, "There are no dryads in this area to be befriended.\r\n");
        return;
      }*/ //Check for room type
      send_to_char(ch, "A dryad shyly appears before you and whispers '\\c0lG%s\\c0ln' into your ear.\r\n", encrypted_string);
      act("A dryad suddenly appears and whispers something in $n's ear.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case SPELL_BIND_PORTAL_MINOR:
     // if (!CAN_USE_ROOM(ch, IN_ROOM(ch))) {
     //   send_to_char(ch, "Something prevents you from binding the portal.\r\n");
     //   return;
     // }
     // act("You open up a shimmering blue portal and gaze into it...", FALSE, ch, 0, 0, TO_CHAR);
      send_to_char(ch, "You open up a shimmering blue portal and gaze into it...\r\n");
      send_to_char(ch, "'\\c0lB%s\\c0ln' briefly appears as a portal shimmers into view and then disappears.\r\n", encrypted_string);
      act("Undecipherable characters briefly shimmer before your eyes.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case SPELL_BIND_PORTAL_MAJOR:
      //if (!CAN_USE_ROOM(ch, IN_ROOM(ch))) {
       // send_to_char(ch, "Something prevents you from binding the portal.\r\n");
       // return;
     // }
      //act("You open up a shimmering red portal and gaze into it...", FALSE, ch, 0, 0, TO_CHAR);
      send_to_char(ch, "You open up a %sshimmering red portal%s and gaze into it...\r\n", CCRED(ch, C_CMP), CCNRM(ch, C_CMP));
      send_to_char(ch, "'\\c0lR%s\\c0ln' briefly appears as a portal shimmers into view and then disappears.\r\n", encrypted_string);
      act("Undecipherable characters briefly shimmer before your eyes.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case SPELL_LOCATE_SHADOW_PLANE:
     // if (!IS_GOOD_LIGHT_FOR_SHADOWS(IN_ROOM(ch)) || !CAN_USE_ROOM(ch, IN_ROOM(ch))) {
      //  send_to_char(ch, "The plane of shadow doesn't touch this location.\r\n");
      //  return;
     // }
      send_to_char(ch, "'\\c0lc%s\\c0ln' briefly appears as you see shadows twirl into a portal out of the corner of your eyes.\r\n", encrypted_string);
      act("Out of the corner of your eye, you see shadows swirl.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    default:
    break;
  }
}

ASPELL(spell_portal)
{
 if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOPORTAL) /*|| !CAN_USE_ROOM(ch, param1)  || (!is_on_same_plane(IN_ROOM(ch), param1)*/ && (spellnum != SPELL_PLANAR_TRAVEL))
    send_to_char(ch, "Powerful magic prevents you from opening a portal.\r\n");
  else if (IN_ROOM(ch) == param1) send_to_char(ch, "You are already there!!!\r\n");
  else if (param1 == NOWHERE) {send_to_char(ch, "That is not a valid room.\r\n"); } 
  else if (((spellnum == SPELL_SHADOW_WALK) || (spellnum == SPELL_SHADOW_DOOR)) && (!IS_GOOD_LIGHT_FOR_SHADOWS(IN_ROOM(ch)) || !IS_GOOD_LIGHT_FOR_SHADOWS(param1)))
    send_to_char(ch, "The plane of shadow doesn't touch that location from here.\r\n");
  else {
    switch (spellnum) {
      case SPELL_PLANAR_TRAVEL:
      case SPELL_DIMENSION_DOOR:
        act("\\c0lRA shimmering red portal briefly appears out of nowhere before your eyes.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lRYou open up a shimmering red portal.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("$n steps into the shimmering red portal before it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lWYou step through the shimmering red portal before it closes behind you.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_DIMENSION_SHIFT:
        act("\\c0lBA shimmering blue portal appears out of nowhere before your eyes.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lBYou open up a shimmering blue portal.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("$n steps into the shimmering blue portal as it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lWYou step through the shimmering blue portal as it closes behind you.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_DIMENSION_WALK:
        act("\\c0lgA shimmering green portal appears out of nowhere before your eyes.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lgYou open up a shimmering green portal.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("$n steps into the shimmering green portal as it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lWYou step through the shimmering green portal as it closes behind you.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_PASS_WITHOUT_TRACE:
        act("A pretty, young girl appears as if out of nowhere and leads $n off into the wilderness.", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lGA young dryad appears as if out of nowhere and leads you off into the wilderness by your hand.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_SHADOW_DOOR:
      case SPELL_SHADOW_WALK:
        act("\\c0lcA shimmering dark portal appears out of the shadows before your eyes.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lcYou open up a shimmering dark portal in the shadows.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("$n steps into the shimmering dark portal before it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lWYou step through the shimmering dark portal before it closes behind you.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_TRAIL_OF_WOODLANDS:
        act("\\c0lGThe shimmering green of the dryads's portal is revealed to you.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("\\c0lGA shimmering green portal briefly appears out of nowhere before your eyes.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        act("$n steps into the shimmering green portal before it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("\\c0lWYou step through the shimmering green portal before it closes behind you.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        break;
      default:
        act("$n opens a portal in thin air and steps through.", TRUE, ch, 0, 0, TO_ROOM);
        act("You open a portal out of thin air and steps through.", TRUE, ch, 0, 0, TO_CHAR);
        break;
    }
    char_from_room(ch);
    char_to_room(ch, param1);
    switch (spellnum) {
      case SPELL_PLANAR_TRAVEL:
      case SPELL_DIMENSION_DOOR:
        /* A ripple in the extra-dimensional space throws you out of the passage. <-- misteleport (yellow) */
        act("\\c0lWYou journey through a shimmering red passage.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("\\c0lRYou step out of a red portal and look about.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("\\c0lRA red portal briefly shimmers open as $n emerges from it.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case SPELL_DIMENSION_SHIFT:
        act("\\c0lWYou journey through a shimmering blue passage.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("\\c0lRYou step out of a blue portal and look about.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("\\c0lRA blue portal briefly shimmers open as $n emerges from it.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case SPELL_DIMENSION_WALK:
        act("\\c0lWYou journey through a shimmering green passage.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("\\c0lgYou step out of a green portal and look about.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("\\c0lgA green portal briefly shimmers open as $n emerges from it.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case SPELL_PASS_WITHOUT_TRACE:
        act("You follow the dryad through the wilderness back to her tree.", FALSE, ch, 0, 0, TO_CHAR);
        act("The dryad gives you a kiss on the cheek and disappears as you look about.", FALSE, ch, 0, 0, TO_CHAR);
       act("\\c0lGA green portal briefly shimmers open as $n emerges from it.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);

        break;
      case SPELL_SHADOW_DOOR:
      case SPELL_SHADOW_WALK:
        act("\\c0lWYou journey through the plane of shadows.\\c0ln", FALSE, ch, 0, 0, TO_CHAR); 
        act("\\c0lcYou step out of the plane of shadows and look about.\\c0ln", FALSE, ch, 0, 0, TO_CHAR); 
        act("\\c0lcA dark portal briefly shimmers open as $n emerges from it.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case SPELL_TRAIL_OF_WOODLANDS:
         act("\\c0lWYou journey through a shimmering green tunnel through the earth past the massive roots of trees.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
	act("\\c0lGYou step out of a green tunnel and look about.\\c0ln", FALSE, ch, 0, 0, TO_CHAR);
        act("\\c0lGA green portal briefly shimmers open as $n emerges from it.\\c0ln", FALSE, ch, 0, 0, TO_ROOM);
        break;
      default:
        send_to_room(param1, "A shimmering portal appears out of thin air and %s steps out.\r\n", GET_NAME(ch));
        break;
    }
    look_at_room(IN_ROOM(ch), ch, 0);
    entry_memory_mtrigger(ch);
    greet_mtrigger(ch, -1);
    greet_memory_mtrigger(ch);
  }
}
