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

/* external functions */
byte saving_throws_nat(int class_num, int type, int level);
void clearMemory(struct char_data *ch);
void weight_change_object(struct obj_data *obj, int weight);
int mag_savingthrow(struct char_data *ch, struct char_data *victim, int type, int modifier);
void name_to_drinkcon(struct obj_data *obj, int type);
void name_from_drinkcon(struct obj_data *obj);
int compute_armor_class(struct char_data *ch);
ACMD(do_flee);

/*
 * Special spells appear below.
 */

ASPELL(spell_create_water)
{
  int water;

  if (ch == NULL || obj == NULL)
    return;
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
{return; }// just return  out for now}
/*{
 if ((param1 != NOWHERE) && IS_VALID_EXIT(ch, param1) && EXIT_FLAGGED(EXIT(ch, param1), EX_CLOSED) && (EXIT(ch, param1)->to_room != NOWHERE)) {
   send_to_char(ch, "PARAM1 IS NOT NOWHERE.\r\n");
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
  else if (EXIT_FLAGGED(EXIT(ch, param1), EX_CLOSED))
     send_to_char(ch, "PARAM1 IS NOT NOWHERE.\r\n");
  else
    send_to_char(ch, "You can't phase through that.\r\n");
}*/



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

  if (GET_LEVEL(victim) > MIN(LVL_SAINT - 1, level + 3)) {
    send_to_char(ch, "%s", SUMMON_FAIL);
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOMAGIC))
	{
    send_to_char(ch, "Powerful magic prevents your summon.\r\n");
    return;
	}

  if (!pk_allowed) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and $N travels\r\n"
	  "through time and space towards you, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely send $M back.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      send_to_char(victim, "%s just tried to summon you to: %s.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to summon you.\r\n",
	      GET_NAME(ch), world[IN_ROOM(ch)].name,
	      (ch->player.sex == SEX_MALE) ? "He" : "She");

      send_to_char(ch, "You failed because %s has summon protection on.\r\n", GET_NAME(victim));
      mudlog(BRF, LVL_SAINT, TRUE, "%s failed summoning %s to %s.", GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
      return;
    }
  }

  if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
      (IS_NPC(victim) && mag_savingthrow(ch, victim, SAVING_SPELL, 0))) {
    send_to_char(ch, "%s", SUMMON_FAIL);
    return;
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
{
  int i;

  if (ch == NULL || obj == NULL)
    return;

  /* Either already enchanted or not a weapon. */
  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC))
    return;

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj->affected[i].location != APPLY_NONE)
      return;

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
  * 
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
  int nutrient_amount = 10+(GET_LEVEL(ch)/4);
  gain_condition(victim, FULL, nutrient_amount);
  gain_condition(victim, THIRST, nutrient_amount);
  if (victim != ch) send_to_char(ch, "%s has been revitalized.\r\n", CAP(GET_NAME(victim)));
  act("You feel your hunger suddenly disappear.", TRUE, victim, 0, 0, TO_CHAR);
  if ((GET_COND(victim, THIRST)>20) && (GET_COND(victim, FULL)>20)) {
    act("$n burps loudly.", TRUE, victim, 0, 0, TO_ROOM);
    act("You burp loudly.", TRUE, victim, 0, 0, TO_CHAR);
  }
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
      send_to_char(ch, "A dryad shyly appears before you and whispers '%s' into your ear.\r\n", encrypted_string);
      act("A dryad suddenly appears and whispers something in $n's ear.", TRUE, ch, 0, 0, TO_ROOM);
      break;
    case SPELL_BIND_PORTAL_MINOR:
     // if (!CAN_USE_ROOM(ch, IN_ROOM(ch))) {
     //   send_to_char(ch, "Something prevents you from binding the portal.\r\n");
     //   return;
     // }
     // act("You open up a shimmering blue portal and gaze into it...", FALSE, ch, 0, 0, TO_CHAR);
      send_to_char(ch, "You open up a %sshimmering blue portal%s and gaze into it...\r\n", CCBLU(ch, C_CMP), CCNRM(ch, C_CMP)); 
      send_to_char(ch, "'%s' briefly appears as a portal shimmers into view and then disappears.\r\n", encrypted_string);
      act("Undecipherable characters briefly shimmer before your eyes.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case SPELL_BIND_PORTAL_MAJOR:
      //if (!CAN_USE_ROOM(ch, IN_ROOM(ch))) {
       // send_to_char(ch, "Something prevents you from binding the portal.\r\n");
       // return;
     // }
      //act("You open up a shimmering red portal and gaze into it...", FALSE, ch, 0, 0, TO_CHAR);
      send_to_char(ch, "You open up a %sshimmering red portal%s and gaze into it...\r\n", CCRED(ch, C_CMP), CCNRM(ch, C_CMP));
      send_to_char(ch, "'%s%s%s' briefly appears as a portal shimmers into view and then disappears.\r\n", CCRED(ch, C_CMP),encrypted_string, CCNRM(ch, C_CMP));
      act("Undecipherable characters briefly shimmer before your eyes.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case SPELL_LOCATE_SHADOW_PLANE:
     // if (!IS_GOOD_LIGHT_FOR_SHADOWS(IN_ROOM(ch)) || !CAN_USE_ROOM(ch, IN_ROOM(ch))) {
      //  send_to_char(ch, "The plane of shadow doesn't touch this location.\r\n");
      //  return;
     // }
      send_to_char(ch, "'%s' briefly appears as you see shadows twirl into a portal out of the corner of your eyes.\r\n", encrypted_string);
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
  else if (param1 == NOWHERE) send_to_char(ch, "That is not a valid room.\r\n");
  else if (((spellnum == SPELL_SHADOW_WALK) || (spellnum == SPELL_SHADOW_DOOR)) && (!IS_GOOD_LIGHT_FOR_SHADOWS(IN_ROOM(ch)) || !IS_GOOD_LIGHT_FOR_SHADOWS(param1)))
    send_to_char(ch, "The plane of shadow doesn't touch that location from here.\r\n");
  else {
    switch (spellnum) {
      case SPELL_PLANAR_TRAVEL:
      case SPELL_DIMENSION_DOOR:
        act("@RA shimmering red portal briefly appears out of nowhere before your eyes.@n", FALSE, ch, 0, 0, TO_ROOM);
        act("@RYou open up a shimmering red portal.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("$n steps into the shimmering red portal before it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("@WYou step through the shimmering red portal before it closes behind you.@n", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_DIMENSION_SHIFT:
        act("@BA shimmering blue portal appears out of nowhere before your eyes.@n", FALSE, ch, 0, 0, TO_ROOM);
        act("@BYou open up a shimmering blue portal.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("$n steps into the shimmering blue portal as it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("@WYou step through the shimmering blue portal as it closes behind you.@n", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_DIMENSION_WALK:
        act("@GA shimmering green portal appears out of nowhere before your eyes.@n", FALSE, ch, 0, 0, TO_ROOM);
        act("@GYou open up a shimmering green portal.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("$n steps into the shimmering green portal as it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("@WYou step through the shimmering green portal as it closes behind you.@n", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_PASS_WITHOUT_TRACE:
        act("@GA pretty, young girl appears as if out of nowhere and leads $n off into the wilderness.@n", FALSE, ch, 0, 0, TO_ROOM);
        act("@GA young dryad appears as if out of nowhere and leads you off into the wilderness by your hand.@n", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_SHADOW_DOOR:
      case SPELL_SHADOW_WALK:
        act("@cA shimmering dark portal appears out of the shadows before your eyes.@n", FALSE, ch, 0, 0, TO_ROOM);
        act("@cYou open up a shimmering dark portal in the shadows.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("$n steps into the shimmering dark portal before it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("@WYou step through the shimmering dark portal before it closes behind you.@n", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_TRAIL_OF_WOODLANDS:
        act("@gThe shimmering green of the dryads's portal is revealed to you.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("@gA shimmering green portal briefly appears out of nowhere before your eyes.@n", FALSE, ch, 0, 0, TO_ROOM);
        act("$n steps into the shimmering green portal before it closes behind $m.", FALSE, ch, 0, 0, TO_ROOM);
        act("@WYou step through the shimmering green portal before it closes behind you.@n", FALSE, ch, 0, 0, TO_CHAR);
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
        act("@WYou journey through a shimmering red passage.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("@RYou step out of a red portal and look about.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("@RA red portal briefly shimmers open as $n emerges from it.@n", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case SPELL_DIMENSION_SHIFT:
        act("@WYou journey through a shimmering blue passage.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("@BYou step out of a blue portal and look about.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("@BA blue portal briefly shimmers open as $n emerges from it.@n", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case SPELL_DIMENSION_WALK:
        act("@WYou journey through a shimmering green passage.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("@GYou step out of a green portal and look about.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("@GA green portal briefly shimmers open as $n emerges from it.@n", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case SPELL_PASS_WITHOUT_TRACE:
        act("@WYou follow the dryad through the wilderness back to her tree.@n", FALSE, ch, 0, 0, TO_CHAR);
        act("@GThe dryad gives you a kiss on the cheek and disappears as you look about.@n", FALSE, ch, 0, 0, TO_CHAR);
        break;
      case SPELL_SHADOW_DOOR:
      case SPELL_SHADOW_WALK:
        act("You journey through the plane of shadows.", FALSE, ch, 0, 0, TO_CHAR); //colorize?
        act("@cYou step out of the plane of shadows and look about.@n", FALSE, ch, 0, 0, TO_CHAR); // Colorize?
        act("A dark portal briefly shimmers open as $n emerges from it.", FALSE, ch, 0, 0, TO_ROOM); // Colorize?
        break;
      case SPELL_TRAIL_OF_WOODLANDS:
        // act("@WYou journey through a shimmering green tunnel through the earth past the massive roots of trees.@n", FALSE, ch, 0, 0, TO_CHAR);
        send_to_char(ch, "You journey through a %sshimmering green tunnel%s through the earth past the massive roots of trees.\r\n",CCGRN(ch, C_CMP), CCNRM(ch, C_CMP));
	//act("@gYou step out of a green tunnel and look about.@n", FALSE, ch, 0, 0, TO_CHAR);
        send_to_char(ch, "You step out of a %sgreen tunnel%s and look about.", CCGRN(ch, C_CMP), CCNRM(ch, C_CMP)); 
        act("@gA green portal briefly shimmers open as $n emerges from it.@n", FALSE, ch, 0, 0, TO_ROOM);
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



