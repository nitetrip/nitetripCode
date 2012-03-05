/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
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
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include <stdlib.h>

/* external variables */
extern int mini_mud;
extern int pk_allowed;
extern struct spell_info_type spell_info[];

/* external functions */
byte saving_throws(int class_num, int type, int level); /* class.c */
void clearMemory(struct char_data *ch);
void weight_change_object(struct obj_data *obj, int weight);

/* local functions */
int mag_materials(struct char_data *ch, int item0, int item1, int item2, int extract, int verbose);
void perform_mag_groups(int level, struct char_data *ch, struct char_data *tch, int spellnum, int savetype);
int mag_savingthrow(struct char_data *ch, int type, int modifier);
void affect_update(void);
int check_mag_resists(struct char_data *ch, struct char_data *victim, int damage, int type);

/*
 * Saving throws are now in class.c as of bpl13.
 */


/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */
int mag_savingthrow(struct char_data *ch, int type, int modifier)
{
  /* NPCs use warrior tables according to some book */
  int class_sav = CLASS_WARRIOR;
  int save;

  if (!IS_NPC(ch))
    class_sav = GET_CLASS(ch);

  save = saving_throws(class_sav, type, GET_LEVEL(ch));
  save += GET_SAVE(ch, type);
  save += modifier;

  /* Dwarves and gnomes get intrinsic saves vs. magic */
  if ((type == SAVING_SPELL || type == SAVING_ROD) &&
           (IS_DWARF(ch) || IS_GNOME(ch)))
    save += (-5 * (GET_CON(ch) / 3.5));

  /* Throwing a 0 is always a failure. */
  if (MAX(1, save) < rand_number(0, 99))
    return (TRUE);

  /* Oops, failed. Sorry. */
  return (FALSE);
}


/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  struct affected_type *af, *next, *af2, *next2;
  struct char_data *i;
  int iteration = 0;

  for (i = character_list; i; i = i->next) {
   startofloop:
    for (af = i->affected; af; af = next) {
      next = af->next;
      if (af->duration >= 1)
	af->duration--;
      else if (af->duration == -1)	/* No action */
	af->duration = -1;	/* GODs only! unlimited */
      else {
	if ((af->type > 0) && (af->type <= MAX_SPELLS))
	  if (!af->next || (af->next->type != af->type) ||
	      (af->next->duration > 0))
	    if (spell_info[af->type].wear_off_msg)
	      send_to_char(i, "%s\r\n", spell_info[af->type].wear_off_msg);
	affect_remove(i, af);
      }
    }
    /* for AFF2   Anubis */
    for (af2 = i->affected2; af2; af2 = next2) {      
      next2 = af2->next;      
      if (af2->duration >= 1)
	af2->duration--;
      else if (af2->duration == -1)	/* No action */
	af2->duration = -1;	/* GODs only! unlimited */
      else {
	if ((af2->type > 0) && (af2->type <= MAX_SPELLS))
	  if (!af2->next || (af2->next->type != af2->type) ||
	      (af2->next->duration > 0))
	    if (spell_info[af2->type].wear_off_msg)
	      send_to_char(i, "%s\r\n", spell_info[af2->type].wear_off_msg);	
	affect2_remove(i, af2);
      }
    }
    iteration++;
  if ((AFF_FLAGGED(i, AFF_HASTE)) && (iteration < 2))
   goto startofloop;

    iteration = 0;
 }
}


/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(struct char_data *ch, int item0, int item1, int item2,
		      int extract, int verbose)
{
  struct obj_data *tobj;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

  for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {
    if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
      obj0 = tobj;
      item0 = -1;
    } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
      obj1 = tobj;
      item1 = -1;
    } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
      obj2 = tobj;
      item2 = -1;
    }
  }
  if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
    if (verbose) {
      switch (rand_number(0, 2)) {
      case 0:
	send_to_char(ch, "A wart sprouts on your nose.\r\n");
	break;
      case 1:
	send_to_char(ch, "Your hair falls out in clumps.\r\n");
	break;
      case 2:
	send_to_char(ch, "A huge corn develops on your big toe.\r\n");
	break;
      }
    }
    return (FALSE);
  }
  if (extract) {
    if (item0 < 0)
      extract_obj(obj0);
    if (item1 < 0)
      extract_obj(obj1);
    if (item2 < 0)
      extract_obj(obj2);
  }
  if (verbose) {
    send_to_char(ch, "A puff of smoke rises from your pack.\r\n");
    act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  return (TRUE);
}




/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
int mag_damage(int level, struct char_data *ch, struct char_data *victim,
		     int spellnum, int savetype)
{
  int dam = 0;

  if (victim == NULL || ch == NULL)
    return (0);

  switch (spellnum) {
    /* Rearranged alphabetically from the circle mess, all single-target attack spells here. -mak */
  case SPELL_BOULDER: 
    dam = dice(3, 100) + 100;
    dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON);
    break;
  case SPELL_BURNING_HANDS:
    if (IS_MAGIC_USER(ch))
      dam = dice(3, 8) + 3;
    else
      dam = dice(3, 6) + 3;
    dam = check_mag_resists(ch, victim, dam, ATTACK_FIRE);
    break;
  case SPELL_CALL_LIGHTNING:
    dam = dice(7, 8) + 7;
      dam = check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC);
    break;
  case SPELL_CHILL_TOUCH:	/* chill touch also has an affect */
    if (IS_MAGIC_USER(ch))
      dam = dice(1, 8) + 1;
    else
      dam = dice(1, 6) + 1;
    dam = check_mag_resists(ch, victim, dam, ATTACK_COLD);
    break;
  case SPELL_CHI_FIST:
    dam = dice(1, 60) + 120;
    dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON);
    break;
  case SPELL_COLOR_SPRAY:
    if (IS_MAGIC_USER(ch))
      dam = dice(9, 8) + 9;
    else
      dam = dice(9, 6) + 9;
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;
  case SPELL_DISPEL_EVIL:
    dam = dice(6, 8) + 6;
      dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    if (IS_EVIL(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_GOOD(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;
  case SPELL_DISPEL_GOOD:
    dam = dice(6, 8) + 6;
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    if (IS_GOOD(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_EVIL(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;
  case SPELL_ENERGY_DRAIN:
    if (GET_LEVEL(victim) <= 2)
      dam = 100;
    else
      dam = dice(1, 10);
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;
  case SPELL_FINGER_OF_DEATH:
    if (IS_SORCEROR(ch))
        dam = dice(50, 20) + 50;
    else
        dam = dice(20, 10) + 30;	
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;
  case SPELL_FIREBALL:
    if (IS_MAGIC_USER(ch))
      dam = dice(11, 8) + 11;
    else
      dam = dice(11, 6) + 11;
    dam = check_mag_resists(ch, victim, dam, ATTACK_FIRE);
    break;
  case SPELL_HARM:
    dam = dice(8, 8) + 8;
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;
  case SPELL_ICE_LANCE : 
    dam = dice(30, 30) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_COLD);
    break;
  case SPELL_LIGHTNING_BOLT:
    if (IS_MAGIC_USER(ch))
      dam = dice(7, 8) + 7;
    else
      dam = dice(7, 6) + 7;
    dam = check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC);
    break;
  case SPELL_MAGIC_MISSILE:
    if (IS_MAGIC_USER(ch))
      dam = dice(1, 8) + 1;
    else
      dam = dice(1, 6) + 1;  
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;
  case SPELL_METEOR: 
    dam = dice(20, 40) + 100;
    dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON);
    break;
  case SPELL_PILLAR_OF_FLAME:
    dam = dice(400, 2) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_FIRE);
    break;
  case SPELL_ROAR : 
    dam = dice(300, 2) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;
  case SPELL_SHOCKING_GRASP:
    if (IS_MAGIC_USER(ch))
      dam = dice(5, 8) + 5;
    else
      dam = dice(5, 6) + 5;
    dam = check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC);
    break;
  case SPELL_SONIC_BLAST:
    dam = dice(20, 5) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;

    /* Area attack spells, also in the mag_areas function below. */
  case SPELL_EARTHQUAKE:
    dam = dice(2, 8) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;
   case SPELL_HORNET_SWARM:
    dam = dice(30, 5) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_PIERCE);
    break;
   case SPELL_ICE_STORM:
    dam = dice(7, 40) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_COLD);
    break;
 case SPELL_METEOR_SHOWER:
    dam = dice(20, 40) + 100;
    dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON);
    break;
  case SPELL_THUNDER_SWARM:
    dam = dice(30, 10) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC);
    break;



  } /* switch(spellnum) */


  /* divide damage by two if victim makes his saving throw */
  if (mag_savingthrow(victim, savetype, 0))
    dam /= 2;

  /* and finally, inflict the damage */
  return (damage(ch, victim, dam, spellnum));
}


/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
 */

#define MAX_SPELL_AFFECTS 5	/* change if more needed */

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
		      int spellnum, int savetype)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  bool affect2 = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i;


  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type = spellnum;
    af[i].bitvector = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
  }

  switch (spellnum) {

  case SPELL_AIRWALK:
    if (AFF_FLAGGED(ch, AFF_AIRWALK)) {
       send_to_char(ch, "%s", NOEFFECT);
       return;
       }
    af[0].duration = GET_LEVEL(ch) * 2;
    af[0].bitvector = AFF_AIRWALK;
    to_vict = "Your begin to hover inches off the ground!";
    break;

  case SPELL_ARMOR:
    af[0].location = APPLY_AC;
    af[0].modifier = -20;
    af[0].duration = 24;
    accum_duration = TRUE;
    to_vict = "You feel someone protecting you.";
    break;

  case SPELL_BLESS:
    af[0].location = APPLY_HITROLL;
    af[0].modifier = 2;
    af[0].duration = 6;

    af[1].location = APPLY_SAVING_SPELL;
    af[1].modifier = -1;
    af[1].duration = 6;

    accum_duration = TRUE;
    to_vict = "You feel righteous.";
    break;

  case SPELL_BLINDNESS:
    if (MOB_FLAGGED(victim,MOB_NOBLIND) || mag_savingthrow(victim, savetype, 0)) {
      send_to_char(ch, "You fail.\r\n");
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = -4;
    af[0].duration = 2;
    af[0].bitvector = AFF_BLIND;

    af[1].location = APPLY_AC;
    af[1].modifier = 40;
    af[1].duration = 2;
    af[1].bitvector = AFF_BLIND;

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;

  case SPELL_BLOODLUST:
    if (AFF_FLAGGED(ch, AFF_FURY)) {
       send_to_char(ch, "%s", NOEFFECT);
       return;
       }
    af[0].duration = 1;
    af[0].bitvector = AFF_FURY;
    to_vict = "A power surges through your body drawing every ounce of anger!";
    to_room = "A power surges through $n's body drawing every ounce of $s anger to the surface!";
    break;

  case SPELL_CHILL_TOUCH:
    af[0].location = APPLY_STR;
    if (mag_savingthrow(victim, savetype, 0))
      af[0].duration = 1;
    else
      af[0].duration = 4;
    af[0].modifier = -1;
    accum_duration = TRUE;
    to_vict = "You feel your strength wither!";
    break;

  case SPELL_CURSE:
    if (mag_savingthrow(victim, savetype, 0)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].duration = 1 + (GET_LEVEL(ch) / 2);
    af[0].modifier = -1;
    af[0].bitvector = AFF_CURSE;

    af[1].location = APPLY_DAMROLL;
    af[1].duration = 1 + (GET_LEVEL(ch) / 2);
    af[1].modifier = -1;
    af[1].bitvector = AFF_CURSE;

    accum_duration = TRUE;
    accum_affect = TRUE;
    to_room = "$n briefly glows red!";
    to_vict = "You feel very uncomfortable.";
    break;
  
  case SPELL_DERVISH_SPIN:
    af[0].duration = 1;
    af[0].bitvector = AFF_DERVISH_SPIN;
    af[1].duration = 1;
    af[1].location = APPLY_ATTACKS;
    af[1].modifier = -1;
    accum_duration = FALSE;
    to_vict = "You take up a bedouin melody and whirl into a dervish spin.";
    to_room = "$n begins to sing a bedouin melody and whirl madly in a dervish spin!";
    break;


  case SPELL_DETECT_ALIGN:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_ALIGN;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_DETECT_INVIS:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_INVIS;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_DETECT_MAGIC:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_MAGIC;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_FORT:
   if (!AFF_FLAGGED(ch, AFF_SANCTUARY)) {
   af[0].duration = 4;
   af[0].bitvector = AFF_FORT;

   accum_duration = FALSE;
   to_vict = "You are surrounded by a pitch black aura of hate.";
   to_room = "$n is surrounded by a pitch black aura of hate.";
   }
   else
   to_vict = "Nothing seems to happen.";
   break;

  case SPELL_HASTE:
    if (AFF_FLAGGED(ch, AFF_HASTE)) {
       send_to_char(ch, "%s", NOEFFECT);
       return;
       }
    af[0].duration = GET_LEVEL(ch) / 2;
    af[0].bitvector = AFF_HASTE;
    to_vict = "Suddenly the world seems to be moving FASTER.";
    break;


  case SPELL_INFRAVISION:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_INFRAVISION;
    accum_duration = TRUE;
    to_vict = "Your eyes glow red.";
    to_room = "$n's eyes glow red.";
    break;

  case SPELL_INVISIBLE:
    if (!victim)
      victim = ch;

    af[0].duration = 12 + (GET_LEVEL(ch) / 4);
    af[0].modifier = -40;
    af[0].location = APPLY_AC;
    af[0].bitvector = AFF_INVISIBLE;
    accum_duration = TRUE;
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_POISON:
    if (mag_savingthrow(victim, savetype, 0) || victim->char_specials.immune[ATTACK_POISON] > 0 || (victim->char_specials.resist[ATTACK_POISON] > 0 && rand_number(1,100) > 50))
    {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].location = APPLY_STR;
    af[0].duration = GET_LEVEL(ch);
    af[0].modifier = -2;
    af[0].bitvector = AFF_POISON;
    to_vict = "You feel very sick.";
    to_room = "$n gets violently ill!";
    break;

  case SPELL_PROT_FROM_EVIL:
    af[0].duration = 24;
    af[0].bitvector = AFF_PROTECT_EVIL;
    accum_duration = TRUE;
    to_vict = "You feel invulnerable!";
    break;

  case SPELL_SANCTUARY:
    if (!AFF_FLAGGED(ch, AFF_FORT)) {
    af[0].duration = 4;
    af[0].bitvector = AFF_SANCTUARY;

    accum_duration = FALSE;
    to_vict = "A white aura momentarily surrounds you.";
    to_room = "$n is surrounded by a white aura.";
    }
       else
   to_vict = "Nothing seems to happen.";  
   break;

  case SPELL_SLEEP:
    if (!pk_allowed && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
      return;
    if (mag_savingthrow(victim, savetype, 0))
      return;

    af[0].duration = 4 + (GET_LEVEL(ch) / 4);
    af[0].bitvector = AFF_SLEEP;

    if (GET_POS(victim) > POS_SLEEPING) {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      GET_POS(victim) = POS_SLEEPING;
    }
    break;

  case SPELL_STRENGTH:
    if (GET_ADD(victim) == 100)
      return;

    af[0].location = APPLY_STR;
    af[0].duration = (GET_LEVEL(ch) / 2) + 4;
    af[0].modifier = 1 + (level > 18);
    accum_duration = TRUE;
    accum_affect = TRUE;
    to_vict = "You feel stronger!";
    break;

  case SPELL_SENSE_LIFE:
    to_vict = "Your feel your awareness improve.";
    af[0].duration = GET_LEVEL(ch);
    af[0].bitvector = AFF_SENSE_LIFE;
    accum_duration = TRUE;
    break;

  case SPELL_SUSTAIN:
    af[0].duration = (GET_LEVEL(ch) / 2);
    af[0].bitvector = AFF_SUSTAIN;
    accum_duration = FALSE;
    to_vict = "You feel your hunger fade away.";
    break;

  case SPELL_WATERWALK:
    af[0].duration = 24;
    af[0].bitvector = AFF_WATERWALK;
    accum_duration = TRUE;
    to_vict = "You feel webbing between your toes.";
    break;
  }

  /* TO USE THE AFF2 flags just add affect2 = TRUE; to the spell case */

  if(affect2 == FALSE) {
  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (AFF_FLAGGED(victim, af[i].bitvector)) {
	send_to_char(ch, "%s", NOEFFECT);
	return;
      }

  /*
   * If the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect)) {
    send_to_char(ch, "%s", NOEFFECT);
    return;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector || (af[i].location != APPLY_NONE))
      affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);
  }
  
  else
  {
    if (IS_NPC(victim) && !affected2_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (AFF2_FLAGGED(victim, af[i].bitvector)) {
	send_to_char(ch, "%s", NOEFFECT);
	return;
      }
  
  if (affected2_by_spell(victim,spellnum) && !(accum_duration||accum_affect)) {
    send_to_char(ch, "%s", NOEFFECT);
    return;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector || (af[i].location != APPLY_NONE))
      affect2_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);
  }

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}


/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */
void perform_mag_groups(int level, struct char_data *ch,
			struct char_data *tch, int spellnum, int savetype)
{
  switch (spellnum) {
    case SPELL_GROUP_HEAL:
    mag_points(level, ch, tch, SPELL_HEAL, savetype);
    break;
  case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, SPELL_ARMOR, savetype);
    break;
  case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL);
    break;
  case SPELL_VIGORIZE_GROUP:
    mag_points(level, ch, tch, SPELL_VIGORIZE_CRITICAL, savetype);
    break;
  }
}


/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */
void mag_groups(int level, struct char_data *ch, int spellnum, int savetype)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;

  if (ch == NULL)
    return;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    return;
  if (ch->master != NULL)
    k = ch->master;
  else
    k = ch;
  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;
    if (!AFF_FLAGGED(tch, AFF_GROUP))
      continue;
    if (ch == tch)
      continue;
    perform_mag_groups(level, ch, tch, spellnum, savetype);
  }

  if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
    perform_mag_groups(level, ch, k, spellnum, savetype);
  perform_mag_groups(level, ch, ch, spellnum, savetype);
}


/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented.
 */
void mag_masses(int level, struct char_data *ch, int spellnum, int savetype)
{
  struct char_data *tch, *tch_next;
  const char *to_char = NULL, *to_room = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (tch == ch)
      continue;
    if (to_char != NULL)
        act(to_char, FALSE, ch, 0, 0, TO_CHAR);
    if (to_room != NULL)
        act(to_room, FALSE, ch, 0, 0, TO_ROOM);     

    switch (spellnum) {
        case SPELL_MASS_HEAL:
        mag_points(level, ch, tch, SPELL_HEAL_CRITICAL, savetype);
        to_char = "You call a healing ray of sun from the heavens.\r\n";
        to_room = "$n calls a healing ray of sun from the heavens, which warms you.\r\n";
        break;
        case SPELL_HEALING_WIND:
        mag_points(level, ch, tch, SPELL_HEAL_SERIOUS, savetype);
        to_char = "You summon a warm, balmy breeze, fragrant of healing herbs, which soothes your wounds. \r\n";
        to_room = "$n summons a warm, balmy breeze, fragrant of healing herbs, which soothes your wounds. \r\n";
        break;
    }
  }
}


/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
 */
void mag_areas(int level, struct char_data *ch, int spellnum, int savetype)
{
  struct char_data *tch, *next_tch;
  const char *to_char = NULL, *to_room = NULL;

  if (ch == NULL)
    return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum) {
  case SPELL_EARTHQUAKE:
    to_char = "You gesture and the earth begins to shake all around you!";
    to_room = "$n gracefully gestures and the earth begins to shake violently!";
    break;
  case SPELL_HORNET_SWARM:
    to_char = "You make a slight buzzing sound, and a swarm of maddened hornets quickly arrives!";
    to_room = "$n makes a slight buzzing sound, summoning a swarm of maddened hornets!";
    break;
  case SPELL_ICE_STORM: 
    to_char = "Razors of ice condense from the air and slash madly about!";
    to_room = "$n sends a chill throughout the room, condensing all the moisture into razor-shards of ice, which slice about madly!";
    break;
  case SPELL_METEOR_SHOWER:
    to_char = "You raise your hands to the heavens and pull down a storm of meteors!";
    to_room = "$n raises $s hands to the heavens and pulls down a storm of meteors!";
    break;
  case SPELL_THUNDER_SWARM:
    to_char = "Every hair on your body stands on end as you focus an electral charge. With a thunderous flash, bolts of lightning arc from you in every direction!";
    to_room = "Every hair on your body stands on end. With a thunderous flash, bolts of lightning arc from $m in every direction!";
    break;
        }

  if (to_char != NULL)
    act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, FALSE, ch, 0, 0, TO_ROOM);
  

  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    /*
     * The skips: 1: the caster
     *            2: immortals
     *            3: if no pk on this mud, skips over all players
     *            4: pets (charmed NPCs)
     */

    if (tch == ch)
      continue;
    if (!IS_NPC(tch) && GET_LEVEL(tch) >= LVL_SAINT)
      continue;
    if (!pk_allowed && !IS_NPC(ch) && !IS_NPC(tch))
      continue;
    if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
      continue;

    /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
    mag_damage(level, ch, tch, spellnum, 1);
  }
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in CircleMUD; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] = {
  "\r\n",
  "$n makes a strange magical gesture; you feel a strong breeze!",
  "$n animates a corpse!",
  "$N appears from a cloud of thick blue smoke!",
  "$N appears from a cloud of thick green smoke!",
  "$N appears from a cloud of thick red smoke!",
  "$N disappears in a thick black cloud!"
  "As $n makes a strange magical gesture, you feel a strong breeze.",
  "As $n makes a strange magical gesture, you feel a searing heat.",
  "As $n makes a strange magical gesture, you feel a sudden chill.",
  "As $n makes a strange magical gesture, you feel the dust swirl.",
  "$n magically divides!",
  "$n animates a corpse!"
};

/*
 * Keep the \r\n because these use send_to_char.
 */
const char *mag_summon_fail_msgs[] = {
  "\r\n",
  "There are no such creatures.\r\n",
  "Uh oh...\r\n",
  "Oh dear.\r\n",
  "Gosh durnit!\r\n",
  "The elements resist!\r\n",
  "You failed.\r\n",
  "There is no corpse!\r\n"
};

/* These mobiles do not exist. */
#define MOB_MONSUM_I		130
#define MOB_MONSUM_II		140
#define MOB_MONSUM_III		150
#define MOB_GATE_I		160
#define MOB_GATE_II		170
#define MOB_GATE_III		180

/* Defined mobiles. */
#define MOB_ELEMENTAL_BASE	20	/* Only one for now. */
#define MOB_CLONE		10
#define MOB_ZOMBIE		11
#define MOB_AERIALSERVANT	19


void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
		      int spellnum, int savetype)
{
  struct char_data *mob = NULL;
  struct obj_data *tobj, *next_obj;
  int pfail = 0, msg = 0, fmsg = 0, num = 1, handle_corpse = FALSE, i;
  mob_vnum mob_num;

  if (ch == NULL)
    return;

  switch (spellnum) {
  case SPELL_CLONE:
    msg = 10;
    fmsg = rand_number(2, 6);	/* Random fail message. */
    mob_num = MOB_CLONE;
    pfail = 50;	/* 50% failure, should be based on something later. */
    break;

  case SPELL_ANIMATE_DEAD:
    if (obj == NULL || !IS_CORPSE(obj)) {
      act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
      load_mtrigger(mob);
      return;
    }
    handle_corpse = TRUE;
    msg = 11;
    fmsg = rand_number(2, 6);	/* Random fail message. */
    mob_num = MOB_ZOMBIE;
    pfail = 10;	/* 10% failure, should vary in the future. */
    break;

  default:
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    send_to_char(ch, "You are too giddy to have any followers!\r\n");
    return;
  }
  if (rand_number(0, 101) < pfail) {
    send_to_char(ch, "%s", mag_summon_fail_msgs[fmsg]);
    return;
  }
  for (i = 0; i < num; i++) {
    if (!(mob = read_mobile(mob_num, VIRTUAL))) {
      send_to_char(ch, "You don't quite remember how to make that creature.\r\n");
      return;
    }
    char_to_room(mob, IN_ROOM(ch));
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
    if (spellnum == SPELL_CLONE) {
      /* Don't mess up the prototype; use new string copies. */
      mob->player.name = strdup(GET_NAME(ch));
      mob->player.short_descr = strdup(GET_NAME(ch));
    }
    act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
    add_follower(mob, ch);
  }
  if (handle_corpse) {
    for (tobj = obj->contains; tobj; tobj = next_obj) {
      next_obj = tobj->next_content;
      obj_from_obj(tobj);
      obj_to_char(tobj, mob);
    }
    extract_obj(obj);
  }
}

/* added manaadd to this function - mak 8.8.04 */
void mag_points(int level, struct char_data *ch, struct char_data *victim,
		     int spellnum, int savetype)
{
  int healing = 0, move = 0, manaadd = 0;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_CURE_LIGHT:
    healing = dice(1, 8) + 1 + (level / 4);
    send_to_char(victim, "You feel better.\r\n");
    break;
  case SPELL_CURE_CRITIC:
    healing = dice(3, 8) + 3 + (level / 4);
    send_to_char(victim, "You feel a lot better!\r\n");
    break;
  case SPELL_DIVINE_HEAL:
    healing = 500;
    send_to_char(victim, "You are filled with warmth and a sense of reverence.\r\n");
    break;
    case SPELL_EXCOMMUNICATE:
    if ((IS_GOOD(victim)) && (IS_GOOD(ch)))
        {
        if ((GET_ALIGNMENT(ch)) >= (GET_ALIGNMENT(victim)))
            {
            manaadd = -(GET_MANA(victim));
            send_to_char(victim, "$n calls upon the name of your lord, excommunicating you for your heresy!\r\n");
            send_to_char(ch, "You incite the name your lord, excommunicating $N for $S heresy!\r\n");
            }
        }
        else
            {
            send_to_char(ch, "You are not holy enough to excommunicate $N! \r\n"); 
            send_to_char(victim, "The heretic $n tries to excommunicate you, but their blasphemy is clear to all!\r\n");
            }
        if ((IS_NEUTRAL(victim)) && (IS_NEUTRAL(ch)))
            {
            if ((abs(GET_ALIGNMENT(victim))) >= (abs(GET_ALIGNMENT(ch))))
                {
                manaadd = -(GET_MANA(victim));
                send_to_char(ch, "You expose $N's bias and lack of balance, shaming $M!\r\n");
                send_to_char(victim, "$n exposes your bias and lack of balance, shaming you before your god!\r\n");
                }
            else
                {
                send_to_char(ch, "$N laughs at your accusations of imbalance!\r\n");
                send_to_char(victim, "You laugh at $n's accusations of imbalance!\r\n");
                }
            }
        if (((IS_EVIL(victim)) && (IS_EVIL(ch))))
                {
                if ((GET_ALIGNMENT(ch)) < (GET_ALIGNMENT(victim)))
                    {
                    manaadd = -(GET_MANA(victim));
                    send_to_char(ch, "You laugh maniacially as you strip $N of $S powers for not being evil enough!\r\n");
                    send_to_char(victim, "$n laughs maniacally, a vision of pure evil, and strips you of your powers!\r\n");
                    }
                }
                else
                    {
                    send_to_char(ch, "Your evil is not focused enough to excommunicate $N!\r\n");
                    send_to_char(victim, "$n thought $e was more versed in the ways of evil than you. The fool!\r\n");
                    }
        
          {
            send_to_char(ch, "Your god has no power over $N!\r\n");
            send_to_char(victim, "$n's god has no power over you!\r\n");
            }
    break;
  case SPELL_HEAL:
    healing = 60;
    send_to_char(victim, "A warm feeling briefly overtakes you.\r\n");
    break;
  case SPELL_HEAL_CRITICAL:
    healing = 180;
    send_to_char(victim, "A warm feeling floods your body.\r\n");
    break;
  case SPELL_HEAL_SERIOUS:
    healing = 120;
    send_to_char(victim, "A warm feeling floods your body.\r\n");
    break;
  case SPELL_SOVEREIGN_HEAL:
    healing = 240;
    send_to_char(victim, "You glow slightly as warmth floods through your veins.\r\n");
    break;
  case SPELL_VIGORIZE_CRITICAL:
    move = 45 + (level / 4);
    send_to_char(victim, "You feel very well-rested. \r\n");
    break;
  case SPELL_VIGORIZE_LIGHT:
    move = 15 + (level / 4);
    send_to_char(victim, "You begin to get a second wind. \r\n");
    break;
  case SPELL_VIGORIZE_SERIOUS:
    move = 30 + (level / 4);
    send_to_char(victim, "You feel a renewed vigor in your legs. \r\n");
    break;
  case SPELL_VITALIZE_MANA:
    manaadd = GET_MOVE(victim);
    move = -(GET_MOVE(victim));
    send_to_char(victim, "Your mind feels refreshed, though you collapse from the effort.\r\n");
    break;
  }
  GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + healing);
  GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);
  GET_MANA(victim) = MIN(GET_MAX_MANA(victim), GET_MANA(victim) + manaadd);
  update_pos(victim);
}


void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
		        int spellnum, int type)
{
  int spell = 0, msg_not_affected = TRUE;
  const char *to_vict = NULL, *to_room = NULL;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_CURE_BLIND:
    spell = SPELL_BLINDNESS;
    to_vict = "Your vision returns!";
    to_room = "There's a momentary gleam in $n's eyes.";
    break;
  case SPELL_REMOVE_POISON:
    spell = SPELL_POISON;
    to_vict = "A warm feeling runs through your body!";
    to_room = "$n looks better.";
    break;
  case SPELL_REMOVE_CURSE:
    spell = SPELL_CURSE;
    to_vict = "You don't feel so unlucky.";
    break;
  default:
    log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
    return;
  }

  if (!affected_by_spell(victim, spell)) {
    if (msg_not_affected)
      send_to_char(ch, "%s", NOEFFECT);
    return;
  }

  affect_from_char(victim, spell);
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);

}


void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
		         int spellnum, int savetype)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum) {
    case SPELL_BLESS:
      if (!OBJ_FLAGGED(obj, ITEM_BLESS) &&
	  (GET_OBJ_WEIGHT(obj) <= 5 * GET_LEVEL(ch))) {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS);
	to_char = "$p glows briefly.";
      }
      break;
    case SPELL_CURSE:
      if (!OBJ_FLAGGED(obj, ITEM_NODROP)) {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	  GET_OBJ_VAL(obj, 2)--;
	to_char = "$p briefly glows red.";
      }
      break;
    case SPELL_INVISIBLE:
      if (!OBJ_FLAGGED(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
        SET_BIT(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
        to_char = "$p vanishes.";
      }
      break;
    case SPELL_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3)) {
      GET_OBJ_VAL(obj, 3) = 1;
      to_char = "$p steams briefly.";
      }
      break;
    case SPELL_REMOVE_CURSE:
      if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
          GET_OBJ_VAL(obj, 2)++;
        to_char = "$p briefly glows blue.";
      }
      break;
    case SPELL_REMOVE_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3)) {
        GET_OBJ_VAL(obj, 3) = 0;
        to_char = "$p steams briefly.";
      }
      break;
  }

  if (to_char == NULL)
    send_to_char(ch, "%s", NOEFFECT);
  else
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, TRUE, ch, obj, 0, TO_ROOM);
  else if (to_char != NULL)
    act(to_char, TRUE, ch, obj, 0, TO_ROOM);

}



void mag_creations(int level, struct char_data *ch, int spellnum)
{
  struct obj_data *tobj;
  obj_vnum z;

  if (ch == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used. */

  switch (spellnum) {
  case SPELL_CREATE_FOOD:
    z = 10;
    break;
  default:
    send_to_char(ch, "Spell unimplemented, it would seem.\r\n");
    return;
  }

  if (!(tobj = read_object(z, VIRTUAL))) {
    send_to_char(ch, "I seem to have goofed.\r\n");
    log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
	    spellnum, z);
    return;
  }
  obj_to_char(tobj, ch);
  act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
  act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
  load_otrigger(tobj);
}

int check_mag_resists(struct char_data *ch, struct char_data *victim, int damage, int type)
{
 if (victim->char_specials.resist[type] > 0 || victim->char_specials.resist[ATTACK_MAGIC] > 0)
  damage = damage - (damage / 3);
 if (victim->char_specials.vulnerable[type] > 0 || victim->char_specials.vulnerable[ATTACK_MAGIC] > 0)
  damage = damage + (damage / 3);
 if (victim->char_specials.immune[type] > 0 || victim->char_specials.immune[ATTACK_MAGIC] > 0) 
  damage = 0;  
 return(damage); 
}
