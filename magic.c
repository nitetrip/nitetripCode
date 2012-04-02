/* ************************************************************************
*  File: magic.c                                       Part of CircleMUD *
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
struct config_data config_info;
extern struct time_info_data time_info;

/* external functions */
byte saving_throws_nat(int class_num, int type, int level); /* class.c */
void clearMemory(struct char_data *ch);
void weight_change_object(struct obj_data *obj, int weight);
byte saving_throws_tot(struct char_data *ch, int type);
int get_advance_hitpoints(struct char_data *ch);

/* local functions */
int mag_materials(struct char_data *ch, int item0, int item1, int item2, int extract, int verbose, int spellnum);
void perform_mag_groups(int level, struct char_data *ch, struct char_data *tch, int param1, int spellnum, int savetype);
int mag_savingthrow(struct char_data *ch, struct char_data *victim, int type, int modifier);
void affect_update(void);
int check_mag_resists(struct char_data *ch, struct char_data *victim, int damage, int type);
void mag_affects(int level, struct char_data *ch, struct char_data *victim, int param1, int spellnum, int savetype);

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
int mag_savingthrow(struct char_data *ch, struct char_data *victim, int type, int modifier)
{
  int save;
  int level_diff = GET_LEVEL(ch)-GET_LEVEL(victim);

  save = saving_throws_tot(victim, type) + level_diff*5/((level_diff > 0) ? 4:2);
  save += modifier;

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
  int iteration = 1, j=0;

  for (i = character_list; i; i = i->next) {
   if (AFF_FLAGGED(i, AFF_HASTE)) iteration = 2;
   for (j = 0; j < iteration; j++) { // this will do the loop twice if AFF_HASTED - leaves room for easy future expansion (if we need spells to wear off quicker)
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
        if (af->type == SPELL_PARALYZE) GET_POS(i) = POS_STANDING;
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
    }
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
		      int extract, int verbose, int spellnum)
{
  struct obj_data *tobj;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;
  
  switch (spellnum)
        {
        case SPELL_DECREPIFY:
        item0 = 13624; /* brain */
        item1 = 13625; /* eye */
        item2 = 13626; /* heart */
        break;
        }

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
int save_dam_reduction_factor = 2;


  if (victim == NULL || ch == NULL)
    return (0);

  switch (spellnum) {
    /* Rearranged alphabetically from the circle mess, all single-target attack spells here. -mak */
 case SPELL_ACID_ARROW:
    dam = MIN(18, MAX(8, 8+2*(GET_LEVEL(ch)-7)));
    dam = rand_number(dam, 3*dam);
    break;
  case SPELL_BALL_LIGHTNING:
    dam = MIN(140,MAX(68, 68+(GET_LEVEL(ch)-28)*24));
    dam = rand_number(2*dam, 3*dam);
    break;
  case SPELL_BOLT_OF_STEEL:
    act("You conjure forth a long bow of fine oak strung with a magical steel bolt.", FALSE, ch, 0, victim, TO_CHAR);
    act("A long bow of fine oak strung with a steel bolt appears in $n's hands.", FALSE, ch, 0, victim, TO_ROOM);
    dam = dice(6, 10) + GET_LEVEL(ch);
    break;
  case SPELL_BOULDER: 
    dam = dice(3, 100) + 100;
    dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON);
    break;
  case SPELL_BURNING_HANDS:
    if (IS_MAGIC_USER(ch))
      dam = dice((GET_LEVEL(ch)), 8) + (GET_LEVEL(ch));
    else
      dam = dice((GET_LEVEL(ch)), 6) + 3;
    dam = check_mag_resists(ch, victim, dam, ATTACK_FIRE);
    break;
  /*case SPELL_CALL_LIGHTNING:
  OLD SPELL dam = dice(7, 8) + 7;
      dam = check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC);
    break; */
  case SPELL_CALL_LIGHTNING:
    if (OUTSIDE(ch) && IS_RAINING()) {
      dam = MIN(53,MAX(25,25+(GET_LEVEL(ch)-11)*7));
      dam = rand_number(dam, 2*dam);
    }
    else {
      send_to_char(ch, "You fail to call lightning from the sky.\r\n");
      return (0);
    }
    break;

  case SPELL_CAUSE_MINOR:
  case SPELL_CAUSE_MAJOR:
    dam =  ((spellnum == SPELL_CAUSE_MINOR)?dice(3,4):dice(5,4)+1);
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
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
  case SPELL_CLENCHED_FIST:
    dam = MIN(90,MAX(45, 45+(GET_LEVEL(ch)-21)*15));
    dam = rand_number(2*dam, 3*dam);
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
  case SPELL_ELEMENTAL_BLAST:
    savetype = SAVING_BREATH;  
    dam = rand_number(0,3);
    switch (dam) {
      case 0: dam = check_mag_resists(ch, victim, dam, ATTACK_FIRE); break;
      case 1: dam = check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC); break;
      case 2: dam = check_mag_resists(ch, victim, dam, ATTACK_COLD); break;
      case 3: dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON); break;
    }
    dam = MIN(145,(MAX(GET_LEVEL(ch)-35, 0)*20+85));
    dam = rand_number(dam, 2*dam);
    act("You delicately focus the breeze towards $N as it grows in strength and begins to tear at $S skin.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n gestures towards $N, causing $M to crumple to the ground and cry out in agony as $E is flayed.", FALSE, ch, 0, victim, TO_ROOM);
    break;
    
      case SPELL_ELEMENTAL_HANDS:
    savetype = SAVING_BREATH;  
    dam = rand_number(0,3);
    switch (dam) {
      case 0: dam = check_mag_resists(ch, victim, dam, ATTACK_FIRE); break;
      case 1: dam = check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC); break;
      case 2: dam = check_mag_resists(ch, victim, dam, ATTACK_COLD); break;
      case 3: dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON); break;
    }
    dam = MIN(10,(MAX(GET_LEVEL(ch)-5, 0)+5));
    dam = rand_number(dam, 3*dam);
    act("Your hands vaporize and the atoms bombard $N, causing $M to stagger under the blow.", FALSE, ch, 0, victim, TO_CHAR);
    act("$N staggers under a blow from $n's seemingly non-existent hands.", FALSE, ch, 0, victim, TO_ROOM);
    break;
  case SPELL_ELEMENTAL_STRIKE:
    savetype = SAVING_BREATH;
    dam = rand_number(0,3);
    switch (dam) {
      case 0: check_mag_resists(ch, victim, dam, ATTACK_FIRE); break;
      case 1: check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC); break;
      case 2: dam = check_mag_resists(ch, victim, dam, ATTACK_COLD); break;
      case 3: dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON); break;
    }
    dam = MIN(100,(MAX(GET_LEVEL(ch)-23, 0)*12+52));
    dam = rand_number(dam, 2*dam);
    act("You delicately focus the breeze towards $N as it grows in strength and begins to tear at $S skin.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n gestures towards $N, causing $M to crumple to the ground and cry out in agony as $E is flayed.", FALSE, ch, 0, victim, TO_ROOM);
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
    case SPELL_FIREBOLT:
    dam = MIN(100,MAX(52, 52+(GET_LEVEL(ch)-18)*12));
    dam = rand_number(dam, 2*dam);
    break;
  case SPELL_FLAMESTRIKE:
    dam = MIN(27,MAX(15,15+(GET_LEVEL(ch)-9)*3));
    dam = rand_number(dam, 2*dam);
    break;
  case SPELL_FLAMING_ARROW:
    act("You conjure up a long bow of fine oak strung with an arrow that bursts into flames.", FALSE, ch, 0, victim, TO_CHAR);
    act("A long bow of fine oak strung with a arrow that bursts into flames appears in $n's hands.", FALSE, ch, 0, victim, TO_ROOM);
    dam = dice(12, 6) + (GET_LEVEL(ch));
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
     dam = dice((GET_LEVEL(ch)), 8) + (GET_LEVEL(ch));
     dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;
  case SPELL_METEOR: 
    dam = dice(20, 40) + 100;
    dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON);
    break;
  case SPELL_MOON_MOTE:
    if (OUTSIDE(ch) && !IS_DAYTIME) {
      dam = MIN(76,MAX(40,40+(GET_LEVEL(ch)-15)*9));
      dam = rand_number(dam, 2*dam);
      dam = (int)((double)dam*MOONLIGHT_SCALOR);
    }
    else {
      send_to_char(ch, "You fail to focus light from the moon.\r\n");
      return (0);
    }
    break;
  case SPELL_PILLAR_OF_FLAME:
    dam = dice(400, 2) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_FIRE);
    break;
  case SPELL_PRISMATIC_SPRAY:
    dam = MIN(165, MAX(90, 90+25*(GET_LEVEL(ch)-33)));
    dam = rand_number(2*dam, 3*dam);
    break;  
  case SPELL_ROAR: 
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
    case SPELL_SMITE_EVIL:
      dam = (GET_LEVEL(victim)>GET_LEVEL(ch)) ? MIN(175, dice(GET_LEVEL(ch),4)) : 175;
     if (IS_EVIL(ch)) {
      victim = ch;
      dam = MIN(175, GET_HIT(victim) - 1);
    }
    else if (IS_GOOD(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;

  case SPELL_SMITE_GOOD:
      dam = (GET_LEVEL(victim)>GET_LEVEL(ch)) ? MIN(175, dice(GET_LEVEL(ch),4)) : 175;
    if (IS_GOOD(ch)) {
      victim = ch;
      dam = MIN(175, GET_HIT(victim) - 1);
    }
    else if (IS_EVIL(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;
  case SPELL_SUNRAY:    /* sunray also has an affect */
    //if (IS_UNDEAD(victim))(no IS_FUNGUS yes) || IS_FUNGUS(victim)) 
    //  {
      dam = MIN(32, MAX(12, 12+4*(GET_LEVEL(ch)-8)));
      dam = rand_number(dam, MIN(90, MAX(40, 40+10*(GET_LEVEL(ch)-8))));
      if (OUTSIDE(victim) && IS_DAYTIME)
        dam *= 2;
      //if IS_FUNGUS(victim)
       // save_dam_reduction_factor = 1;
    //}
    //else
      dam = 0;
    act("$N is bathed in your sunlight.", FALSE, ch, 0, victim, TO_CHAR);
    act("$N is bathed in a ray of sunlight cast by $n.", FALSE, ch, 0, victim, TO_ROOM);
    break;



    /* Area attack spells, also in the mag_areas function below. */
  case SPELL_BREATH_FIRE:
  case SPELL_BREATH_GAS:
  case SPELL_BREATH_FROST:
  case SPELL_BREATH_ACID:
  case SPELL_BREATH_LIGHTNING:
    dam = MIN(MAX_DAMAGE_PER_HIT, MAX(25, 25+5*(GET_LEVEL(ch)/2)));
    dam = rand_number(dam, 3*dam);
    break;
  case SPELL_CHAIN_LIGHTNING:
    dam = MIN(95, MAX(60, 60+15*(GET_LEVEL(ch)-37)));
    dam = rand_number(2*dam, 3*dam);
    break;
  case SPELL_EARTHQUAKE:
    dam = dice(2, 8) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_MAGIC);
    break;
  case SPELL_ELEMENTAL_BURST:
    dam = rand_number(0,3);
    savetype = SAVING_BREATH;
    switch (dam) {
      case 0: dam = check_mag_resists(ch, victim, dam, ATTACK_FIRE); break;
      case 1: dam = check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC); break;
      case 2: dam = check_mag_resists(ch, victim, dam, ATTACK_COLD); break;
      case 3: dam = check_mag_resists(ch, victim, dam, ATTACK_BLUDGEON); break;
    }
    dam = MIN(72,(MAX(GET_LEVEL(ch)-28, 0)*8+40));
    dam = rand_number(dam, 2*dam);
    act("You are hit by $N's elemental burst.", FALSE, victim, 0, ch, TO_CHAR);
    break;

 case SPELL_FIREBALL:
    dam = MIN(79,(MAX(GET_LEVEL(ch)-27, 0)*8+47));
    dam = rand_number(dam, 2*dam);
    break;

  case SPELL_FLAILING_FISTS:
    dam = MIN(70,MAX(46, 46+(GET_LEVEL(ch)-30)*8));
    dam = rand_number(2*dam, 3*dam);
    break;
  case SPELL_HAIL_OF_ARROWS:
    dam = dice(8, 8) + GET_LEVEL(ch);
    if (dam) {
      dam = rand_number(2*dam, 3*dam);
    }
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
  case SPELL_SEARING_ORB:
    dam = MIN(36, MAX(20, 20+4*(GET_LEVEL(ch)-33)));
    dam = rand_number(dam, 2*dam);

      if (GET_LEVEL(victim)<11) dam = GET_HIT(victim);  //100% damage
      else if (GET_LEVEL(victim)<16) dam = MAX(dam*2, (GET_HIT(victim)/2)); //50% damage
      else if (GET_LEVEL(victim)<21) dam = MAX(dam*2, (GET_HIT(victim)/4)); //25% damage
      else if (GET_LEVEL(victim)<26) dam = MAX(dam*2, (GET_HIT(victim)/8)); //12.5% damage
      else if (GET_LEVEL(victim)<31) dam = MAX(dam*2, (GET_HIT(victim)/14));  //7% damage
      else if (GET_LEVEL(victim)<36) dam = MAX(dam*2, (GET_HIT(victim)/25)); //4% damage
      else dam = MAX(dam*2, (GET_HIT(victim)/50)); //2% damage
      check_mag_resists(ch, victim, dam, ATTACK_LIGHT);
    break;
  case SPELL_SONIC_BLAST:
    dam = MIN(45,(MAX(GET_LEVEL(ch)-18, 0)*5+25));
    dam = rand_number(dam, 2*dam);
    break;

 case SPELL_SUNBURST:  /* sunburst also has an affect */
  //    if (IS_UNDEAD(victim) || IS_FUNGUS(victim)) {
      dam = MIN(25, MAX(10, 10+3*(GET_LEVEL(ch)-13)));
      dam = rand_number(dam, MIN(70, MAX(30, 30+8*(GET_LEVEL(ch)-13))));
      if (OUTSIDE(victim) && IS_DAYTIME)
        dam *= 2;
     // if IS_FUNGUS(victim)
      //  save_dam_reduction_factor = 1;
    // }
    else //(this is the extra damage routine) 
      dam = 0;
    break;

  case SPELL_THUNDER_SWARM:
    dam = dice(30, 10) + level;
    dam = check_mag_resists(ch, victim, dam, ATTACK_ELECTRIC);
    break;
  case SPELL_WAIL_OF_THE_BANSHEE:    
      if (GET_LEVEL(victim)<11) dam = GET_HIT(victim);  //100% damage
      else if (GET_LEVEL(victim)<16) dam = GET_HIT(victim)/2; //50% damage
      else if (GET_LEVEL(victim)<21) dam = GET_HIT(victim)/4; //25% damage
      else if (GET_LEVEL(victim)<26) dam = GET_HIT(victim)/8; //12.5% damage
      else if (GET_LEVEL(victim)<31) dam = GET_HIT(victim)/14;  //7% damage
      else if (GET_LEVEL(victim)<36) dam = GET_HIT(victim)/25; //4% damage
      else dam = GET_HIT(victim)/50; //2% damage
      act("$N's shriek chills you to the very bone.", FALSE, victim, 0, ch, TO_CHAR);
    break;
  case SPELL_VAMPIRIC_TOUCH:
     //  Actual success is "the caster needs to make a hit roll versus armor class 100 minus the victim's dexterity bonus."
     //  FIXME - when armor class and thaco is working.
     //  Variables used -caster_add_hits

	if (rand_number(1,10) >= 7) { // 70 % chance of Success!
          dam = GET_LEVEL(ch);
      		if (GET_LEVEL(victim) < 6) dam += rand_number(5,15);
      		if (GET_LEVEL(victim) == 6) dam += rand_number(6,18);
      		if (GET_LEVEL(victim) == 7) dam += rand_number(7,21);
      		if (GET_LEVEL(victim) == 8) dam += rand_number(8,24);
	  	if (GET_LEVEL(victim) == 9) dam += rand_number(9,27);
	  	if (GET_LEVEL(victim) > 10) dam += rand_number(10,30);
      dam = MIN(GET_HIT(victim), dam); // Can not gain more than the victim's total HP
      GET_HIT(ch) = MIN(GET_MAX_HIT(ch), (GET_HIT(ch) + dam)); // Can only fill to the maximum
      act("$n drains $N - what a waste of energy!", FALSE, ch, 0, victim, TO_NOTVICT);
      act("$n drains some of your energy!", FALSE, ch, 0, victim, TO_VICT);
      act("You drain $N of some of $S energy.\r\nYou feel better as life force flows into you.", FALSE, ch, 0, victim, TO_CHAR);
     } else {
      act("$n reaches out to touch you but misses!", FALSE, ch, 0, victim, TO_VICT);
      act("You need more draining lessons.", FALSE, ch, 0, victim, TO_CHAR);
      act("$N fails to drain $s - what a waste of energy.", FALSE, ch, 0, victim, TO_NOTVICT);
    }
    break;
    default:
    log("SYSERR: unknown spellnum %d passed to mag_damage in magic.c.", spellnum);
    break;


  } /* switch(spellnum) */


  /* divide damage by two if victim makes his saving throw */
  if (mag_savingthrow(ch, victim, savetype, 0))
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

#define MAX_SPELL_AFFECTS 63	/* change if more needed */

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
		      int param1, int spellnum, int savetype)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  bool affect2 = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i, duration, new_position = GET_POS(victim);// set this to current position to avoid bugs when the position is not set


  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type = spellnum;
    af[i].bitvector = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
  }

  switch (spellnum) {

  case SPELL_ACCURACY:
    af[0].duration = 1;
    af[0].round_duration = TRUE;
    af[0].location = APPLY_AC;
    af[0].modifier = 17; //ac penalty
    accum_duration = FALSE;
    // to_vict = "You now attack with exactness and precision!";
    to_vict = "You feel your senses temporarily increase!";
    to_room = "$n's attacks hit with exactness and precision!";
    break;

  case SPELL_AID:
    mag_affects(level, ch, victim, param1, 0, SPELL_BLESS);
    af[0].location = APPLY_HIT;
    af[0].modifier = get_advance_hitpoints(ch);
    af[0].duration = GET_LEVEL(ch);
    accum_duration = FALSE;
    to_vict = "You feel more fit.";
    to_room = "$n looks more fit.";
    break;

  case SPELL_AIRWALK:
    if (AFF_FLAGGED(victim, AFF_AIRWALK)) {
       send_to_char(ch, "%s", NOEFFECT);
       return;
       }
    af[0].duration = GET_LEVEL(ch) * 2;
    af[0].bitvector = AFF_AIRWALK;
    to_room = "$n begins to hover inches off the ground!";
    to_vict = "Your begin to hover inches off the ground!";
    break;

  case SPELL_ARMOR:
    af[0].location = APPLY_AC;
    af[0].modifier = -20;
    af[0].duration = 24;
    accum_duration = TRUE;
    to_vict = "You feel someone protecting you.";
    break;
  case SPELL_BARKSKIN:
    if (affected_by_spell(victim, SPELL_ARMOR)) {
      send_to_char(ch, "This spell does not work in conjunction with Armor.\r\n");
      return;
    }
    af[0].location = APPLY_AC;
    af[0].modifier = -30;
    af[0].duration = 24;
    af[1].location = APPLY_DEX;
    af[1].modifier = -1;
    af[1].duration = 24;
    accum_duration = FALSE;
    to_room = "$n's skin is replaced by bark.";
    to_vict = "You feel your skin replaced by bark.";
    break;


  case SPELL_GRANT_BAT_SONAR:
  case SPELL_BAT_SONAR:
    duration = (spellnum == SPELL_BAT_SONAR) ? 2*GET_LEVEL(ch) : GET_LEVEL(ch);
    af[0].duration = duration;
    af[0].bitvector = AFF_BAT_SONAR;
    af[1].location = APPLY_SONC_RESIST;
    af[1].modifier = -50;    
    af[1].duration = duration;
    accum_duration = FALSE;
    to_vict = "A whole new world of sound opens up to you.";
    to_room = "$n suddenly cocks his head as if listening.";
    break;
  
  case SPELL_BENEFICENCE:
    af[0].duration = 2*GET_LEVEL(ch)/5+8;
    af[0].bitvector = AFF_BENEFICENCE;
    accum_duration = FALSE;
    to_vict = "You feel protected by an aura of mystical peace and harmony.";
    to_room = "$n is protected by an aura of mystical peace and harmony.";
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

  case SPELL_COLOR_SPRAY:
  case SPELL_SEARING_ORB:
  case SPELL_SUNRAY:
  case SPELL_BLINDNESS:
    if (MOB_FLAGGED(victim,MOB_NOBLIND) || mag_savingthrow(ch, victim, savetype, 0)) {
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
    if (mag_savingthrow(ch, victim, savetype, 0))
      af[0].duration = 1;
    else
      af[0].duration = 4;
    af[0].modifier = -1;
    accum_duration = TRUE;
    to_vict = "You feel your strength wither!";
       break;

  case SPELL_CLOAK_OF_DARKNESS:
    if (AFF_FLAGGED(victim, AFF_CLOAK_OF_THE_NIGHT) || AFF_FLAGGED(victim, AFF_CLOAK_OF_SHADOWS)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    duration = 4+GET_LEVEL(ch);
    af[0].duration = duration;
    af[0].bitvector = AFF_CLOAK_OF_DARKNESS;
    accum_duration = FALSE;
    to_vict = "A cloud of darkness surrounds your body.";
    to_room = "A cloud of darkness surrounds $n, blotting out the light.";
    break;
  
  case SPELL_CLOAK_OF_SHADOWS:
    if (!IS_GOOD_LIGHT_FOR_SHADOWS(IN_ROOM(victim)) || AFF_FLAGGED(victim, AFF_CLOAK_OF_THE_NIGHT) || AFF_FLAGGED(victim, AFF_CLOAK_OF_DARKNESS)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    duration = 4+GET_LEVEL(ch)/2;
    af[0].duration = duration;
    af[0].bitvector = AFF_CLOAK_OF_SHADOWS;
    accum_duration = FALSE;
    to_vict = "You disappear into the shadows.";
    to_room = "$n disappears into the shadows.";
    break;
  
  case SPELL_CLOAK_OF_THE_NIGHT:
    if (AFF_FLAGGED(victim, AFF_CLOAK_OF_SHADOWS) || AFF_FLAGGED(victim, AFF_CLOAK_OF_DARKNESS)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    duration = 4+GET_LEVEL(ch)/2;
    af[0].duration = duration;
    af[0].bitvector = AFF_CLOAK_OF_THE_NIGHT;
    accum_duration = FALSE;
    to_vict = "A cloud of darkness surrounds your body.";
    to_room = "A cloud of darkness surrounds $n, blotting out the light.";
    break;

  case SPELL_CURSE:
    if (mag_savingthrow(ch, victim, savetype, 0)) {
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

  case SPELL_DEATH_STRIKE:
  case SPELL_POWER_STRIKE:
    af[0].duration = 1;
    af[0].round_duration = TRUE;
    af[0].location = APPLY_AC;
    af[0].modifier = 10; //ac penalty
    accum_duration = FALSE;
    to_vict = ((spellnum == SPELL_POWER_STRIKE) ? "You concentrate an aura of power against your opponent!" : "You concenrate a massive aura of power against your opponent!");
    // "You draw upon your Ki power and your attacks become ferocious!");
    to_room = "$n becomes ferocious as $e draws upon $s Ki power!";
    break;
  case SPELL_DEATHS_DOOR:
    if AFF_FLAGGED(victim, AFF_REGENERATION) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    duration = GET_LEVEL(ch)*3;
    af[0].duration = duration;
    af[0].bitvector = AFF_DEATHS_DOOR;
    af[0].location = APPLY_CON;
    af[0].modifier = 1;
   /* af[1].duration = duration;
    af[1].modifier = 1;
    af[1].bitvector = AFF_DEATHS_DOOR; */
    accum_duration = FALSE;
    send_to_char(ch, "%s looks more hardy.\r\n", CAP(GET_NAME(victim)));
    to_vict = "You feel more hardy.";
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

case SPELL_DECREPIFY:
    af[0].duration = 1 + (GET_LEVEL(ch) /2);
    af[0].bitvector = AFF_DECREPIFY;
    af[1].duration = 1 + (GET_LEVEL(ch) /2);
    af[1].location = APPLY_STR;
    af[1].modifier = -(dice(1, 6) + (GET_LEVEL(ch) /10));
    accum_duration = FALSE;
    to_vict = "You feel very decrepid.";
    to_room = "$n looks very decrepid.";
    break;

  case SPELL_DETECT_ALIGN:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_ALIGN;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle as you become aware of the soul of man.";
    to_room = "$n's eyes glow briefly.";
    break;
    
  case SPELL_DETECT_EVIL:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_EVIL;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle as you become aware of the evil of the world.";
    to_room = "$n's eyes glow briefly.";
    break; 
    
  case SPELL_DETECT_GOOD:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_GOOD;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle as you become aware of the goodness of the world.";
    to_room = "$n's eyes glow briefly.";
    break;    

  case SPELL_DETECT_INVIS:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_INVIS;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    to_room = "$n's eyes glow briefly.";
    break;

  case SPELL_DETECT_MAGIC:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_MAGIC;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    to_room = "$n's eyes glow briefly.";
    break;

  case SPELL_DETECT_NEUTRAL:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_NEUTRAL;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle as you become aware of the balance of the world.";
    to_room = "$n's eyes glow briefly.";
    break;

  case SPELL_DREAMSIGHT:
    af[0].duration = 2*GET_LEVEL(ch);
    af[0].bitvector = AFF_DREAMSIGHT;
    accum_duration = FALSE;
    to_vict = "You feel your senses heighten.";
    to_room = "$n gets a strange twinkle in $s eyes.";    
    break;

  case SPELL_DRAW_UPON_HOLY_MIGHT:
    af[0].duration = GET_LEVEL(ch);
    af[0].location = APPLY_STR;
    af[0].modifier = 1+(GET_LEVEL(ch)/10);
    accum_duration = FALSE;
    to_vict = "Your body shudders violently as the power of your god is channeled into you.";
    to_room = "$n's body shudders violently as a bright aura envelopes $m.";
    break;


  case SPELL_ELEMENTAL_AURA:
    if (affected_by_spell(victim, SPELL_SKELETAL_GUISE)) {
      send_to_char(ch, "This spell does not work in conjunction with Skeletal Guise.\r\n");
      return;
    }
    if (affected_by_spell(victim, SPELL_ELEMENTAL_SHIELD)) {
      send_to_char(ch, "This spell does not work in conjunction with Elemental Shield.\r\n");
      return;
    }
    duration = GET_LEVEL(ch);
    switch (rand_number(0, 3)) {
      case ELEMENTAL_TYPE_GREY:
        af[0].location = APPLY_SLSH_RESIST;
        af[0].duration = duration;
        af[0].modifier = 50;
        af[1].location = APPLY_PIER_RESIST;
        af[1].duration = duration;
        af[1].modifier = 50;
        af[2].location = APPLY_BLDG_RESIST;
        af[2].duration = duration;
        af[2].modifier = 50;
        to_vict = "You see a dull grey aura of elemental power surround you.";
        to_room = "You see a dull grey aura of elemental power surround $n.";
        break;
     case ELEMENTAL_TYPE_RED:
        af[0].location = APPLY_FIRE_IMMUNE;
        af[0].duration = duration;
        af[0].modifier = MAX_RESIST;
        af[1].location = APPLY_LGHT_IMMUNE;
        af[1].duration = duration;
        af[1].modifier = MAX_RESIST;

        to_vict = "You see a flickering red aura of elemental power surround you.";
        to_room = "You see a flickering red aura of elemental power surround $n.";
        break;
      case ELEMENTAL_TYPE_BLUE:
        af[0].location = APPLY_COLD_IMMUNE;
        af[0].duration = duration;
        af[0].modifier = MAX_RESIST;
        af[1].location = APPLY_ACID_IMMUNE;
        af[1].duration = duration;
        af[1].modifier = MAX_RESIST;
        to_vict = "You see a shimmering blue aura of elemental power surround you.";
        to_room = "You see a shimmering blue aura of elemental power surround $n.";
        break;
      case ELEMENTAL_TYPE_WHITE:
        af[0].location = APPLY_ELEC_IMMUNE;
        af[0].duration = duration;
        af[0].modifier = MAX_RESIST;
        af[1].location = APPLY_GAS_IMMUNE;
        af[1].duration = duration;
        af[1].modifier = MAX_RESIST;
        to_vict = "You see a hazy white aura of elemental power surround you.";
        to_room = "You see a hazy white aura of elemental power surround $n.";
        break;
      default:
        break;
    }
    accum_duration = FALSE;
    break;


  case SPELL_ELEMENTAL_SHIELD:
    if (affected_by_spell(victim, SPELL_ELEMENTAL_AURA)) {
      send_to_char(ch, "This spell does not work in conjunction with Elemental Aura.\r\n");
      return;
    }
    duration = GET_LEVEL(ch)*2;
    switch (rand_number(0, 3)) {
      case ELEMENTAL_TYPE_GREY:
        af[0].location = APPLY_BLDG_RESIST;
        af[0].duration = duration;
        af[0].modifier = 50;
        to_vict = "You see a dull grey shield of elemental power form before you.";
        to_room = "You see a dull grey shield of elemental power form before $n.";
        break;
      case ELEMENTAL_TYPE_RED:
        af[0].location = APPLY_FIRE_RESIST;
        af[0].duration = duration;
        af[0].modifier = 50;
        af[1].location = APPLY_LGHT_RESIST;
        af[1].duration = duration;
        af[1].modifier = 50;
        to_vict = "You see a flickering red shield of elemental power form before you.";
        to_room = "You see a flickering red shield of elemental power form before $n.";
        break;
      case ELEMENTAL_TYPE_BLUE:
        af[0].location = APPLY_COLD_RESIST;
        af[0].duration = duration;
        af[0].modifier = 50;
        af[1].location = APPLY_ACID_RESIST;
        af[1].duration = duration;
        af[1].modifier = 50;
        to_vict = "You see a shimmering blue shield of elemental power form before you.";
        to_room = "You see a shimmering blue shield of elemental power form before $n.";
        break;
      case ELEMENTAL_TYPE_WHITE:
        af[0].location = APPLY_ELEC_RESIST;
        af[0].duration = duration;
        af[0].modifier = 50;
        af[1].location = APPLY_GAS_RESIST;
        af[1].duration = duration;
        af[1].modifier = 50;
        to_vict = "You see a hazy white shield of elemental power form before you.";
        to_room = "You see a hazy white shield of elemental power form before $n.";
        break;
      default:
        break;
    }
    accum_duration = FALSE;
    break;

  case SPELL_ENFEEBLEMENT:
    if (mag_savingthrow(ch, victim, savetype, 0)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    if (GET_TOT_STR(victim) < 6) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].location = APPLY_STR;
    af[0].duration = 3+GET_LEVEL(ch)/3;
    af[0].round_duration = FALSE;
    af[0].modifier = 5-(GET_NAT_STR(victim)+((GET_NAT_STR(victim)>18)?10:(GET_NAT_ADD(victim)/10)));
    accum_duration = FALSE;
    accum_affect = FALSE;
    to_room = "$n suddenly looks very weak!";
    to_vict = "You muscles suddenly feel exhausted.";
    break;

  case SPELL_ENLARGE:
    if (affected_by_spell(victim, SPELL_SHRINK)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    } 
    duration = 4+(GET_LEVEL(ch)>>1);
    af[0].duration = duration;
    af[0].location = APPLY_SIZE;
    af[0].modifier = 1;
    af[1].duration = duration;
    af[1].location = APPLY_DAMROLL;
    af[1].modifier = 2;
    accum_duration = FALSE;
    to_vict = "You grow up to a larger size.";
    to_room = "$n grows up to a larger size.";
    break;

  case SPELL_FEATHER_FALL:
    af[0].duration = GET_LEVEL(ch);
    af[0].bitvector = AFF_FEATHER_FALL;
    accum_duration = FALSE;
    to_vict = "You feel like you are drifting on the wind.";
    break;

  case SPELL_FLAMEWALK:
    duration = GET_LEVEL(ch);
    af[0].location = APPLY_FIRE_RESIST;
    af[0].duration = duration;
    af[0].modifier = 50;
    to_vict = "You suddenly feel resistant to fire.";
    accum_duration = FALSE;
    break;

 case SPELL_GHOUL_TOUCH:
  case SPELL_HOLD_ANIMAL:
  case SPELL_HOLD_BEAST:
  case SPELL_HOLD_MONSTER:
  case SPELL_HOLD_PERSON:
  case SPELL_HOLD_PLANT:
  case SPELL_HOLD_UNDEAD:
  case SPELL_PARALYZE:
    if (AFF_FLAGGED(victim, AFF_FREE_ACTION)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    if ((affected_by_spell(victim, SPELL_PARALYZE)) || AFF_FLAGGED(victim, AFF_PARALYZE)) {
      send_to_char(ch, "Your victim is already paralyzed!\r\n");
      return;
    }
    else if (MOB_FLAGGED(victim, MOB_NOPARALYZE) || AFF_FLAGGED(victim, AFF_FREE_ACTION) || (mag_savingthrow(ch, victim, SAVING_PARA, 0)) || IS_NECROMANCER(victim))
    {
      send_to_char(ch, VICTIM_RESISTS);
      act("You feel tense for a moment.", FALSE, victim, 0, ch, TO_CHAR);
      set_fighting(victim, ch);
      return;
    }
    else if (((spellnum == SPELL_HOLD_BEAST)/* && IS_TYPE_BEAST(ch)*/) ||
    ((spellnum == SPELL_GHOUL_TOUCH)/* && IS_TYPE_NECRO_VULN(ch)*/) ||
    ((spellnum == SPELL_HOLD_ANIMAL)/* && IS_TYPE_ANIMAL(ch)*/) ||
    ((spellnum == SPELL_HOLD_MONSTER)/* && IS_TYPE_MONSTER(ch)*/) ||
    ((spellnum == SPELL_HOLD_PERSON)/* && IS_TYPE_PERSON(ch)*/) ||
    ((spellnum == SPELL_HOLD_PLANT)/* && IS_TYPE_PLANT(victim)*/) ||
    ((spellnum == SPELL_HOLD_UNDEAD)/* && IS_UNDEAD(victim)*/) ||
    (spellnum == SPELL_PARALYZE))
    {
      af[0].duration = GET_LEVEL(ch)/10+3;
      af[0].bitvector = AFF_PARALYZE;
      af[0].round_duration = FALSE;
      new_position = POS_PARALYZED;
      accum_duration = FALSE;
      to_vict = "You feel your limbs freeze!";
      to_room = "$n seems paralyzed!";
      spellnum = SPELL_PARALYZE;
 //     break;
    }
    else {
      send_to_char(ch, "%s", NOEFFECT_RACE);
      return;
    }
  break;


  case SPELL_FLEET_FEET:
    af[0].duration = GET_LEVEL(ch);
    af[0].bitvector = AFF_FLEET_FEET;
    accum_duration = FALSE;
    to_vict = "Your feet and legs feel lighter than normal.";
    break;

  case SPELL_FREE_ACTION:
    if (affected_by_spell(victim, SPELL_PARALYZE) || GET_POS(victim) == POS_PARALYZED) {
      affect_from_char(victim, SPELL_PARALYZE);
      new_position = POS_STANDING;
    }
    duration = GET_LEVEL(ch)/4+3;
    af[0].duration = duration;
    af[0].bitvector = AFF_FREE_ACTION;
    af[1].location = APPLY_DEX;
    af[1].duration = duration;
    af[1].modifier = (GET_LEVEL(ch)/10)+1;
    af[1].bitvector = AFF_FREE_ACTION;
    accum_duration = FALSE;
    to_vict = "You can move more easily.";
    to_room = "$n can move more easily.";
    break;


  case SPELL_FORT:
   if (!AFF_FLAGGED(victim, AFF_SANCTUARY) && !AFF_FLAGGED(victim, AFF_REFLECT_DAMAGE)) {
   af[0].duration = 4;
   af[0].bitvector = AFF_FORT;

   accum_duration = FALSE;
   to_vict = "You are surrounded by a pitch black aura of hate.";
   to_room = "$n is surrounded by a pitch black aura of hate.";
   }
   else
   to_vict = "Nothing seems to happen.";
   break;

  case SPELL_GHOUL_GAUNTLET:
      if (AFF_FLAGGED(victim, AFF_FREE_ACTION)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }

    af[0].duration = (GET_LEVEL(ch)/10)+1;
    af[0].bitvector = AFF_PARALYZING_TOUCH;
    accum_duration = FALSE;
    to_vict = "You feel a sudden itch in your hands.";
    break;


  case SPELL_HASTE:
    duration = GET_LEVEL(ch)/4 + GET_TOT_CON(victim)/2;
    af[0].duration = duration;
    af[0].bitvector = AFF_HASTE;
    af[1].location = APPLY_AC;
    af[1].modifier = -5;
    af[1].duration = duration;
    af[1].bitvector = AFF_HASTE;
    accum_duration = FALSE;
    to_vict = "You feel... FAST!!";
    break;

  case SPELL_HEALING_DREAM:
    af[0].duration = 32000;
    af[0].bitvector = AFF_HEALING_DREAM;
    accum_duration = FALSE;
    if (victim == ch) new_position = POS_SLEEPING;
    else if (GET_POS(victim) != POS_SLEEPING) {
      send_to_char(ch, "%s must already be sleeping for healing dream to work.\r\n", CAP(GET_NAME(victim)));
      return;
    }
    send_to_char(ch, "Peaceful dreams of joy and happiness fill %s's sleep.\r\n", GET_NAME(victim));
    to_vict = "Peaceful dreams of joy and happiness fill your sleep.";
    break;
  case SPELL_HORNETS_DART:
    if (mag_savingthrow(ch, victim, savetype, 0)) {
      send_to_char(ch, "%s resists the deadly toxin injected by your hornet dart.\r\n", CAP(GET_NAME(victim)));
      send_to_char(victim, PS_RESISTS_POISON);
      return;
    }
    af[0].duration = 10;
    af[0].location = APPLY_STR;
    af[0].round_duration = TRUE;
    af[0].modifier = MIN(-1, GET_LEVEL(ch)/-4);
    af[0].bitvector = AFF_POISON;
    af[0].type = SPELL_POISON;
    accum_duration = TRUE;
    accum_affect = TRUE;
    to_room = "$n looks sickened by $N's hornet dart.";
    to_vict = "Burning pain courses through your blood as $N's hornet dart stings you.";
    break;


  case SPELL_IMMUNITY_TO_COLD:
    duration = GET_LEVEL(ch);
    af[0].location = APPLY_COLD_IMMUNE;
    af[0].duration = duration;
    af[0].modifier = 100;
    to_vict = "You feel a chill leave your body!";
    to_room = "$n shivers for a moment.";
    accum_duration = FALSE;
    break;

  case SPELL_IMMUNITY_TO_ELEC:
    duration = GET_LEVEL(ch);
    af[0].location = APPLY_ELEC_IMMUNE;
    af[0].duration = duration;
    af[0].modifier = 100;
    to_vict = "You feel insulated against electricity!";
       accum_duration = FALSE;
    break;

  
  case SPELL_INFRAVISION:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_INFRAVISION;
    accum_duration = TRUE;
    to_vict = "Your eyes glow red.";
    to_room = "$n's eyes glow red.";
    break;

  case SPELL_INTIMIDATE:
    if (mag_savingthrow(ch, victim, SAVING_SPELL, 0) || (GET_TOT_INT(victim) < 6)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].duration = 32000; //close as we can get to infinity. no worries, spell wears off immediatly after battle ends
    af[0].round_duration = FALSE;
    af[0].location = APPLY_HITROLL;
    af[0].modifier = MIN(-1, GET_LEVEL(ch)/-4);
    af[1].duration = 32000;
    af[1].round_duration = FALSE;
    af[1].location = APPLY_DAMROLL;
    af[1].modifier = MIN(-1, GET_LEVEL(ch)/-4);
    accum_duration = FALSE;
    to_room = "$n is intimidated by $N's ferocious attacks.";
    to_vict = "$N intimidates you with $S fierceness in battle!";
    // to_char = "$N glances around nervously as $E is intimidated by you!";
    break;


  case SPELL_IMPROVED_INVISIBILITY:
    if (IS_INVIS(victim) || AFF_FLAGGED(victim, AFF_IMPROVED_INVIS)) {
      send_to_char(ch, "Your victim is already invisible!\r\n");
      return;
    }
    af[0].duration = 4+GET_LEVEL(ch);
    af[0].modifier = -40;
    af[0].location = APPLY_AC;
    af[0].bitvector = AFF_IMPROVED_INVIS;
    accum_duration = FALSE;
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_INVISIBILITY_TO_ENEMIES:
    if (IS_INVIS(victim) || AFF_FLAGGED(victim, AFF_IMPROVED_INVIS)) {
      send_to_char(ch, "Your victim is already invisible!\r\n");
      return;
    }
    af[0].duration = 4+GET_LEVEL(ch)/2;
    af[0].modifier = -40;
    af[0].location = APPLY_AC;

    af[0].bitvector = AFF_INVIS_TO_ENEMIES;
    accum_duration = FALSE;
    to_vict = "Hmmm...you still see yourself.";
    to_room = "$n looks unaffected.";
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

  case SPELL_MAGICAL_VESTMANTS:
    af[0].location = APPLY_AC;
    af[0].modifier = -10*(GET_LEVEL(ch)/10)-10;
    af[0].duration = 24;
    accum_duration = FALSE;
    to_vict = "You draw upon your faith to protect you against the dangers of the world.";
    to_room = "$n grasps $s holy symbol and makes a few strange gestures.";
    break;
 case SPELL_PACIFY:
    if (mag_savingthrow(ch, victim, SAVING_SPELL, 0)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    duration = -(25+((GET_LEVEL(ch)*5)/8)); //used here as a temp variable for modifier NOT duration...
    af[0].location = APPLY_AGGR_GENERAL;
    af[0].modifier = duration;
    af[0].duration = 24;
    af[0].round_duration = FALSE;
    af[1].location = APPLY_AGGR_EVIL;
    af[1].modifier = duration;
    af[1].duration = 24;
    af[1].round_duration = FALSE;
    af[2].location = APPLY_AGGR_GOOD;
    af[2].modifier = duration;
    af[2].duration = 24;
    af[2].round_duration = FALSE;
    af[3].location = APPLY_AGGR_NEUTRAL;
    af[3].modifier = duration;
    af[3].duration = 24;
    af[3].round_duration = FALSE;
    af[4].location = APPLY_AGGR_WIMPY;
    af[4].modifier = duration;
    af[4].duration = 24;
    af[4].round_duration = FALSE;
    af[5].location = APPLY_AGGR_COWARD;
    af[5].modifier = duration;
    af[5].duration = 24;
    af[5].round_duration = FALSE;
    af[6].location = APPLY_AGGR_MEMORY;
    af[6].modifier = duration;
    af[6].duration = 24;
   af[6].round_duration = FALSE;
    accum_duration = FALSE;
    to_vict = "You feel more at ease.";
    to_room = "$n seems to be more at ease.";
    break;


  case SPELL_POISON:
    if (mag_savingthrow(ch, victim, savetype, 0) || victim->char_specials.immune[ATTACK_POISON] > 0 || (victim->char_specials.resist[ATTACK_POISON] > 0 && rand_number(1,100) > 50))
    {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].location = APPLY_STR;
    af[0].duration = GET_LEVEL(ch);
    af[0].modifier = -2;
    af[0].bitvector = AFF_POISON;
    accum_duration = TRUE;
    to_vict = "You feel very sick.";
    to_room = "$n gets violently ill!";
    break;

  case SPELL_REFLECT_DAMAGE:
    af[0].duration = MAX(0,(GET_LEVEL(ch)-1)/10)+2;
    af[0].bitvector = AFF_REFLECT_DAMAGE;
    accum_duration = FALSE;
    to_vict = "You are surrounded by a reflecting shield.";
    to_room = "$n is surrounded by a reflecting shield.";
    break;
  case SPELL_REGENERATION:
    if AFF_FLAGGED(victim, AFF_DEATHS_DOOR) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].location = APPLY_CON;
    af[0].duration = GET_LEVEL(ch);
    af[0].modifier = 1;
    af[0].bitvector = AFF_REGENERATION;
    accum_duration = FALSE;
    send_to_char(ch, "%s looks more fit.\r\n", CAP(GET_NAME(victim)));
    to_vict = "You feel more fit.";
    break;

  case SPELL_RESISTANCE_TO_COLD:
    if (affected_by_spell(victim, SPELL_IMMUNITY_TO_COLD)) {
      send_to_char(ch, "You're already immune to cold!\r\n");
      return;
    }
    duration = GET_LEVEL(ch)*2;
    af[0].location = APPLY_COLD_RESIST;
    af[0].duration = duration;
    af[0].modifier = 50;
    to_vict = "You feel a chill leave your body!";
    accum_duration = FALSE;
    break;

  case SPELL_RESISTANCE_TO_ELEC:
    if (affected_by_spell(victim, SPELL_IMMUNITY_TO_ELEC)) {
      send_to_char(ch, "You're already immune to electricity!\r\n");
      return;
    }
    duration = GET_LEVEL(ch)*2;
    af[0].location = APPLY_ELEC_RESIST;
    af[0].duration = duration;
    af[0].modifier = 50;
    to_vict = "You feel insulated against electricity!";
    accum_duration = FALSE;
    break;


  case SPELL_SANCTUARY:
    if (!AFF_FLAGGED(ch, AFF_FORT) && !AFF_FLAGGED(ch, AFF_REFLECT_DAMAGE)) {
    af[0].duration = 4;
    af[0].bitvector = AFF_SANCTUARY;
    accum_duration = FALSE;
    to_vict = "A white aura momentarily surrounds you.";
    to_room = "$n is surrounded by a white aura.";
    }
       else
   to_vict = "Nothing seems to happen.";  
   break;
  case SPELL_SECOND_SIGHT:
    af[0].duration = 8 + GET_LEVEL(ch);
    af[0].bitvector = AFF_SECOND_SIGHT;
    accum_duration = FALSE;
    to_vict = "Your eyes become enlarged and glow blue.";
    to_room = "$n's eyes become enlarged and glow blue.";
    break;


  case SPELL_SHADOW_ARMOR:
    if (!IS_GOOD_LIGHT_FOR_SHADOWS(IN_ROOM(victim))) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].duration = 24;
    af[0].bitvector = AFF_SHADOW_ARMOR;
    accum_duration = FALSE;
    to_vict = "The shadows in the room swarm about your body, providing a protective barrier.";
    to_room = "The shadows in the room swarm about $n's body, providing a protective barrier.";    
    break;

  case SPELL_SHIELD:
    af[0].location = APPLY_AC;
    af[0].modifier = -10;
    af[0].duration = 24;
    af[1].location = APPLY_SAVING_PARA;
    af[1].duration = 24;
    af[1].modifier = -5;
    af[2].location = APPLY_SAVING_ROD;
    af[2].duration = 24;
    af[2].modifier = -5;
    af[3].location = APPLY_SAVING_PETRI;
    af[3].duration = 24;
    af[3].modifier = -5;
    af[4].location = APPLY_SAVING_BREATH;
    af[4].duration = 24;
    af[4].modifier = -5;
    af[5].location = APPLY_SAVING_SPELL;
    af[5].duration = 24;
    af[5].modifier = -5;
    accum_duration = FALSE;
    to_vict = "You sense a strong shield of magic protecting you.";
    break;
  

case SPELL_SHIELD_AGAINST_EVIL:
  case SPELL_PROTECTION_FROM_EVIL:
    if IS_EVIL(victim) {
      act("The evilness in your spirit prevents the spell form working.", FALSE, victim, 0, ch, TO_CHAR);
      return;
    }
    duration = 10+GET_LEVEL(ch)/4;
    af[0].duration = duration;
    af[0].bitvector = AFF_PROTECT_EVIL;
    accum_duration = TRUE;
    to_vict = "You have a righteous feeling!";
    to_room = "A blue aura shaped as a shield briefly grows in front of $n.";
    break;

  case SPELL_SHIELD_AGAINST_GOOD:
  case SPELL_PROTECTION_FROM_GOOD:
    if IS_GOOD(victim) {
      act("The goodness in your spirit prevents the spell form working.", FALSE, victim, 0, ch, TO_CHAR);
      return;
    }
    duration = 10+GET_LEVEL(ch)/4;
    af[0].duration = duration;
    af[0].bitvector = AFF_PROTECT_GOOD;
    accum_duration = TRUE;
    to_vict = "You have a wicked feeling!";
    to_room = "A red aura shaped as a shield briefly grows in front of $n.";
    break;


  case SPELL_SHRINK:
    if (affected_by_spell(victim, SPELL_ENLARGE)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    duration = 4+(GET_LEVEL(ch)>>1);
    af[0].duration = duration;
    af[0].location = APPLY_SIZE;
    af[0].modifier = -1;
    af[1].duration = duration;
    af[1].location = APPLY_AC;
    af[1].modifier = -20;
    accum_duration = FALSE;
    to_vict = "You shrink down to a smaller size.";
    to_room = "$n shrinks down to a smaller size.";
    break;

  case SPELL_SKELETAL_GUISE:
    if (affected_by_spell(victim, SPELL_ELEMENTAL_AURA)) {
      send_to_char(ch, "This spell does not work in conjunction with Elemental Aura.\r\n");
      return;
    }
    duration = 6+((GET_LEVEL(ch)-1)/10)*3;
    af[0].location = APPLY_SLSH_RESIST;
    af[0].duration = duration;
    af[0].modifier = 50;
    af[1].location = APPLY_PIER_RESIST;
    af[1].duration = duration;
    af[1].modifier = 50;
    af[2].location = APPLY_CHA;
    af[2].duration = duration;
    af[2].modifier = -3;
    accum_duration = FALSE;
    to_vict = "You feel your skin grow taut as it stretches tightly over your bones.";
    to_room = "$n's features grow taut and skeletal.";
    break;


  case SPELL_SILENCE:
    if MOB_FLAGGED(victim, MOB_NOSILENCE) {
      send_to_char(ch, "%s absorbs some of the energy from that spell.\r\n", CAP(GET_NAME(victim)));
      return;
    }
    if (mag_savingthrow(ch, victim, SAVING_SPELL, 0)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].duration = MAX(0,(GET_LEVEL(ch)-1)/10)+3;
    af[0].round_duration = FALSE;
    af[0].bitvector = AFF_SILENCE;
    accum_duration = FALSE;
    to_vict = "You have been silenced!";
    to_room = "$n seems to be silenced!";
    break;


  case SPELL_SLEEP:
    if (!pk_allowed && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
      return;
    if (mag_savingthrow(ch, victim, savetype, 0))
      return;

    af[0].duration = 4 + (GET_LEVEL(ch) / 4);
    af[0].bitvector = AFF_SLEEP;
    accum_duration = TRUE;

    if (GET_POS(victim) > POS_SLEEPING) {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      new_position = POS_SLEEPING;
    } else {
      send_to_char(ch, "Zzzzz...\r\n");
      act("$n snores loudly... Zzzz...", TRUE, victim, 0, 0, TO_ROOM); }
    break;
  case SPELL_SLEEPWALK:
    af[0].duration = GET_LEVEL(ch);
    af[0].bitvector = AFF_SLEEPWALK;
    accum_duration = TRUE;
    to_vict = "You feel your body lift off the ground.";
    to_room = "$n slowly stands up as if in a dazed state of mind.";
    break;
 case SPELL_SOMNOLENT_GAZE:
    if (AFF_FLAGGED(ch, AFF_BLIND)) {
      send_to_char(ch, "You can't gaze at anything while you're blinded!");
      return;
    }
    act("You stare into $N's eyes...", FALSE, ch, 0, victim, TO_NOTVICT);
    act("$N stares into your eyes...", FALSE, victim, 0, ch, TO_CHAR);
    if (mag_savingthrow(ch, victim, SAVING_SPELL, 20) || AFF_FLAGGED(victim, AFF_HASTE)) {
      act("$N blinks.", FALSE, ch, 0, victim, TO_NOTVICT);
      act("$N's gaze makes you blink.", FALSE, victim, 0, ch, TO_CHAR);
      return;
    }
    duration = (GET_LEVEL(ch)/30)+1;
    af[0].duration = duration; //close as we can get to infinity. no worries, spell wears off immediatly after battle ends
    af[0].round_duration = FALSE;
    af[0].location = APPLY_AP;
    af[0].modifier = 40;
    af[1].duration = duration;
    af[1].round_duration = FALSE;
    af[1].location = APPLY_HITROLL;
    af[1].modifier = -4;
    af[2].location = APPLY_ATTACKS;
    af[2].duration = duration;
    af[2].round_duration = FALSE;
    af[2].modifier = -2;
    accum_duration = FALSE;
    to_room = "$n's eyes glaze over as $S actions slow down!";
    /* $n yawns. */
    to_vict = "$N's gaze makes you feel very lethargic!";
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

  case SPELL_STRENGTH_BURST:
  case SPELL_CHAMPIONS_STRENGTH:
    if (GET_TOT_STR(victim) >= MAX_STAT_ATTRIBUTE) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].location = APPLY_STR;
    af[0].duration = 1;
    af[0].modifier = MAX_STAT_ATTRIBUTE + 10; //the 10 is for the ten subdivisions between str 18 and str 19
    af[0].round_duration = (spellnum == SPELL_STRENGTH_BURST);
    accum_duration = FALSE;
    accum_affect = FALSE;
    to_vict = ((spellnum == SPELL_STRENGTH_BURST) ? "You feel your strength temporarily increase!" : "You feel the strength of past heroes run through your body");
    break;


  case SPELL_SENSE_LIFE:
    to_vict = "Your feel your awareness improve.";
    af[0].duration = GET_LEVEL(ch);
    af[0].bitvector = AFF_SENSE_LIFE;
    accum_duration = TRUE;
    break;

  case SPELL_SUSTAIN:
    if ( (GET_COND(victim, FULL) < 20) || (GET_COND(victim, THIRST) <20) ){
     send_to_char(victim, "You are not full enough for sustain to work!\r\n");
     if (ch != victim) send_to_char(ch, "%s is not full enough for sustain to work!\r\n", GET_NAME(victim));
     return; }
    af[0].duration = (GET_LEVEL(ch) / 2);
    af[0].bitvector = AFF_SUSTAIN;
    accum_duration = FALSE;
    to_vict = "You feel your hunger fade away.";
    break;
  case SPELL_TOWER_OF_STRENGTH:
    if AFF_FLAGGED(victim, AFF_SANCTUARY) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].duration = MAX(1,(GET_LEVEL(ch)/5));
    af[0].bitvector = AFF_SANCTUARY;
    accum_duration = FALSE;
    to_vict = "From your holiness and purity, you will a tower of strength about your body.";
    to_room = "$n is surrounded by white tower of strength.";
    break;

  case SPELL_WATERWALK:
    af[0].duration = 24;
    af[0].bitvector = AFF_WATERWALK;
    accum_duration = TRUE;
    to_room = "$n grows webbing between $s toes";
    to_vict = "You feel webbing between your toes.";
    break;
  case SPELL_WINDWALK:
    duration = GET_LEVEL(ch);
    af[0].duration = duration;
    af[0].bitvector = AFF_AIRWALK;
    af[1].location = APPLY_ELEC_RESIST;
    af[1].duration = duration;
    af[1].modifier = 50;
    af[2].location = APPLY_GAS_RESIST;
    af[2].duration = duration;
    af[2].modifier = 50;
    to_vict = "You feel the wind stir around your body.";
    accum_duration = FALSE;
    break;

  case SPELL_WITHER:
    if (mag_savingthrow(ch, victim, SAVING_PETRI, 0)) {
      send_to_char(ch, "%s", NOEFFECT);
      return;
    }
    af[0].location = APPLY_ATTACKS;
    af[0].duration = MAX(1, GET_LEVEL(ch)/10);
    af[0].round_duration = FALSE;
    af[0].modifier = -1;
    to_vict = "Your wielding arm shrivels to that of an old man.";
    to_room = "$n's wielding arms shrivels up to the bone!";
    break;

    default:
     log("SYSERR: unknown spellnum %d passed to mag_affects magic.c.", spellnum);
    break;

  }

  /* TO USE THE AFF2 flags just add affect2 = TRUE; to the spell case */

  if(affect2 == FALSE) {
  /*   * If this is a mob that has this affect set in its mob file, do not
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


  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);

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

  GET_POS(victim) = new_position;
}


/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */
void perform_mag_groups(int level, struct char_data *ch,
			struct char_data *tch, int param1, int spellnum, int savetype)
{
  const char *to_room = NULL;
  switch (spellnum) {
    case SPELL_GROUP_HEAL:
    mag_points(level, ch, tch, param1, SPELL_HEAL, savetype);
    break;
  case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, param1, SPELL_ARMOR, savetype);
    break;
  case SPELL_HEROES_FEAST:
    send_to_char(ch, "You conjure up a great feast for your group members to gorge themselves on.\r\n");
    to_room = "$n conjures up a great feast for $s group members to gorge themselves on.";
    act(to_room, TRUE, ch, 0, ch, TO_ROOM);
    spell_vitality(level, ch, tch, NULL, param1, SPELL_HEROES_FEAST);
    break;
  case SPELL_SUSTAIN_GROUP:
    mag_affects(level, ch, tch, param1, SPELL_SUSTAIN, savetype);
    break;
  case SPELL_GROUP_RECALL:
    send_to_char(ch, "You open a shimmering portal and step through it along with your group.\r\n");
    to_room = "$n opens a shimmering portal that disappears when $s entire group enters.";
    act(to_room, TRUE, ch, 0, ch, TO_ROOM);
    spell_recall(level, ch, tch, NULL, NOWHERE, SPELL_GROUP_RECALL);
    break;
  case SPELL_GROUP_SORIN_RECALL:
    send_to_char(ch, "You open a shimmering portal and step through it along with your group.\r\n");
    to_room = "$n opens a shimmering portal that disappears when $s entire group enters.";
    act(to_room, TRUE, ch, 0, ch, TO_ROOM);
    spell_recall(level, ch, tch, NULL, NOWHERE, SPELL_GROUP_SORIN_RECALL);
    break;
  case SPELL_VIGORIZE_GROUP:
    mag_points(level, ch, tch, param1, SPELL_VIGORIZE_CRITICAL, savetype);
    break;
  case SPELL_TALES_OF_ARCANE_LORE:
    spell_arcane_lore(level, ch, tch, NULL,NOWHERE,SPELL_TALES_OF_ARCANE_LORE);
    break;
  case SPELL_FORESTATION:
    spell_arboreal_form(level, ch, tch, NULL, NOWHERE, SPELL_FORESTATION);
    break;
  case SPELL_ETHEREAL_SPHERE:
    spell_ethereal_projection(level, ch, tch, NULL, NOWHERE, SPELL_ETHEREAL_SPHERE);
    break;
  case SPELL_ASTRAL_ASCENSION:
    spell_astral_projection(level, ch, tch, NULL, NOWHERE, SPELL_ASTRAL_ASCENSION);
    break;
  case SPELL_MASS_SUICIDE:
    spell_hang(level, ch, tch, NULL, NOWHERE, SPELL_MASS_SUICIDE);
    break;
  case SPELL_DIMENSION_DOOR:
  case SPELL_PLANAR_TRAVEL:
  case SPELL_SHADOW_DOOR:
  case SPELL_TRAIL_OF_WOODLANDS:
    spell_portal(level, tch, ch, NULL, param1, spellnum);
    break;
  default:
     log("SYSERR: unknown spellnum %d passed to mag_groups (magic.c).", spellnum);
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
void mag_groups(int level, struct char_data *ch, int param1, int spellnum, int savetype)
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
    perform_mag_groups(level, ch, tch, param1, spellnum, savetype);
  }

  if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
  perform_mag_groups(level, ch, k, param1, spellnum, savetype);
  perform_mag_groups(level, ch, ch, param1, spellnum, savetype);
}


/*
 * mass spells affect every creature in the room except the caster.
 * ****Might change this to affect caster also - sey 03/12
 * There weren't any implemented spells in circle stock here.
 * I added the return message fields    - mak 8.10.04
 *
 */
void mag_masses(int level, struct char_data *ch, int spellnum, int savetype)
{
    const char *to_char = NULL, *to_room = NULL, *to_vict = NULL, *to_notvict = NULL;
    struct char_data *tch, *tch_next;
    int param1 = NOWHERE;
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) {
        tch_next = tch->next_in_room;
        if (tch == ch)
        continue;
  
      switch (spellnum) 
        {
        case SPELL_MASS_HEAL:
        mag_points(level, ch, tch, param1, SPELL_HEAL_CRITICAL, savetype);
        to_char = "You call a healing ray of sun from the heavens.";
        to_room = "$n calls a healing ray of sun from the heavens, which warms you.";
        break;
        case SPELL_HEALING_WIND:
        mag_points(level, ch, tch, param1, SPELL_HEAL_SERIOUS, savetype);
        to_char = "You summon a warm, balmy breeze, fragrant of healing herbs, which soothes your wounds.";
        to_room = "$n summons a warm, balmy breeze, fragrant of healing herbs, which soothes your wounds.";
        break;
        }
    }
   if (to_char != NULL)
        act(to_char, FALSE, ch, 0, 0, TO_CHAR);
    if (to_room != NULL)
        act(to_room, FALSE, ch, 0, 0, TO_ROOM);
    if (to_vict != NULL)
        act(to_vict, FALSE, ch, 0, 0, TO_VICT);
    if (to_notvict != NULL)
        act(to_notvict, FALSE, ch, 0, 0, TO_NOTVICT);
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
  case SPELL_ASPHYXIATE:
    to_char = "You breath some noxious gas into the air!";
    to_room = "$n breaths some noxious gas into the air!";
    break;
  case SPELL_BREATH_FIRE:
    to_char = "You breath fire into the room burning everyone in the area!";
    to_room = "$n breaths fire into the room that burns everyone nearby!";
    break;

  case SPELL_BREATH_GAS:
    to_char = "You breath gas into the room suffocating everyone in the area!";
    to_room = "$n breaths gas into the room that suffocates everyone nearby!";
    break;
  case SPELL_BREATH_FROST:
    to_char = "You breath frost into the room freezing everyone in the area!";
    to_room = "$n breaths frost into the room that freezes everyone nearby!";
    break;
  case SPELL_BREATH_ACID:
    to_char = "You breath acid into the room eroding everyone in the area!";
    to_room = "$n breaths acid into the room that erodes everyone nearby!";
    break;
  case SPELL_BREATH_LIGHTNING:
    to_char = "You breath lightning into the room electrocuting everyone in the area!";
    to_room = "$n breaths lightning into the room that electrocutes everyone nearby!";
    break;

  case SPELL_CHAIN_LIGHTNING:
    to_char = "Lightning bolts spray from your fingertips and connect with nearby lifeforms!";
    to_room = "$n sprays bolts of lightning from $s fingertips which connect with nearby lifeforms.";
    break;
  case SPELL_EARTHQUAKE:
    to_char = "You gesture and the earth begins to shake all around you!";
    to_room = "$n gracefully gestures and the earth begins to shake violently!";
    break;
  case SPELL_ELEMENTAL_BURST:
    to_char = "You burst into an explosion of elemental power!";
    to_room = "$n bursts into an explosion of elemental power!";
    break;
  case SPELL_FLAILING_FISTS:
    to_char = "A fury of magical fists appear around your body and flails everywhere!";
    to_room = "A fury of fists appears over $n's head and flails everywhere!";
    break;
  case SPELL_HAIL_OF_ARROWS:
    to_char = "You conjure forth a magical bow and shoot a splitting arrow into the air!";
    to_room = "$n conjures up a magical bow and shoots a splitting arrow into the air.";
    break;
  case SPELL_HOLY_WORD:
    spellnum = SPELL_SMITE_EVIL;
    to_char = "With a single word, you call upon the might of your faith.";
    to_room ="With a single word, $n calls upon the might of $s faith.";
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
    case SPELL_SEARING_ORB:
    to_char = "You create an orb hotter than the sun and hurl it toward your enemies!";
    to_room = "$n creates an orb hotter than the sun and hurls it towards $s enemies.";
    break;
  case SPELL_SUNBURST:
    to_char = "You explode into a burst of bright light!";
    to_room = "$n explodes into a burst of bright light.";
    break;
  case SPELL_THUNDER_SWARM:
    to_char = "Every hair on your body stands on end as you focus an electral charge. With a thunderous flash, bolts of lightning arc from you in every direction!";
    to_room = "Every hair on your body stands on end. With a thunderous flash, bolts of lightning arc from $m in every direction!";
    break;
  case SPELL_UNHOLY_WORD:
    spellnum = SPELL_SMITE_GOOD;
    to_char = "A chill swirls about your spine as you utter a single word!";
    to_room = "A chill swirls about your spine as $n utters a single word.";
    break;
  case SPELL_WAIL_OF_THE_BANSHEE:
    to_char = "You belt out an ear-piercing shriek that chills your enemies to the bone.";
    to_room = "$n belts out a chilling ear-piercing shriek!";
    break;
  default:
     log("SYSERR: unknown spellnum %d passed to mag_areas in magic.c.", spellnum);
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
 if (spellnum != SPELL_CALM) mag_damage(level, ch, tch, spellnum, 1);
    //add entries here for area spells that also cast specific spells on each victim
    switch (spellnum) {
      case SPELL_ASPHYXIATE:
        mag_affects(level, ch, tch, 0, SPELL_ASPHYXIATE, savetype);
        break;
      case SPELL_CALM:
        mag_affects(level, ch, tch, 0, SPELL_PACIFY, savetype);
        break;
      case SPELL_LIFE_LEECH:
        mag_points(level, ch, tch, TRUE, SPELL_LIFE_LEECH, savetype);
        break;
      case SPELL_SEARING_ORB:
        mag_affects(level, ch, tch, 0, SPELL_BLINDNESS, savetype);
        spell_stun(level, ch, tch, NULL, NOWHERE, SPELL_SEARING_ORB);
       break;
      case SPELL_CHAIN_LIGHTNING:
      case SPELL_SUNBURST:
        mag_affects(level, ch, tch, 0, SPELL_BLINDNESS, savetype);
      break;
      case SPELL_WAIL_OF_THE_BANSHEE:
      spell_spook(level, ch, tch, NULL, NOWHERE, SPELL_WAIL_OF_THE_BANSHEE);
       break;
    }


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
    pfail = 50;	/* FIXME 50% failure, should be based on something later. */
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
    pfail = 10;	/* FIXME 10% failure, should vary in the future. */
    break;

  default:
     log("SYSERR: unknown spellnum %d passed to mag_summons in magic.c.", spellnum);
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

/* added manaadd to this function to allow for vitalize/excommunicate
* also added to_vict to_room and to_notvict. not to be confused with send_to_char(),
* which is also used in here for much the same thing. 		-mak 8.10.04
*/
void mag_points(int level, struct char_data *ch, struct char_data *victim,
		     int param1, int spellnum, int savetype)
{

  int healing = 0, move = 0, manaadd = 0;
  const char *to_vict = NULL, *to_room = NULL, *to_notvict = NULL, *to_char = NULL;
  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_BLOOD_REVEL:
    healing = -(GET_HIT(victim) - 10);
    manaadd = GET_HIT(victim);
    send_to_char(victim, "You sink your fangs into your wrist and drink deeply. You feel a surge of power!\r\n");
    break;
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
      if (GET_CLASS(victim) == CLASS_DISCIPLE || GET_CLASS(victim) == CLASS_CRUSADER || GET_CLASS(victim) == CLASS_CLERIC || GET_CLASS(victim) == MOB_CLASS_CLERIC || GET_CLASS(victim) == MOB_CLASS_DISCIPLE || GET_CLASS(victim) == MOB_CLASS_CRUSADER)
          {
          if ((IS_GOOD(victim)))
            {
            if ((IS_GOOD(ch)))
                {
                if ((GET_ALIGNMENT(ch)) >= (GET_ALIGNMENT(victim)))
                    {
                    manaadd = -(GET_MANA(victim));
                    send_to_char(victim, "%s calls upon the name of your lord, excommunicating you for your heresy!\r\n", GET_NAME(ch));
                    send_to_char(ch, "You incite the name your lord, excommunicating %s for their heresy!\r\n", GET_NAME(victim));
                    to_notvict = "$N calls upon the heavens to excommunicate $n as a heretic!";
                    break;
                    }
                else
                    {
                    send_to_char(ch, "You are not holy enough to excommunicate %s!\r\n", GET_NAME(victim)); 
                    send_to_char(victim, "The heretic %s tries to excommunicate you, but their blasphemy is clear to all!\r\n", GET_NAME(ch));
                    to_notvict = "The heretic $N tries to excommunicate $n, but the heresy of the accusation is clear to all!";
                    break;
                    }
                }
            else
                {
                send_to_char(ch, "Your god has no power over %s!\r\n", GET_NAME(victim));
                send_to_char(victim, "%s's god has no power over you!\r\n", GET_NAME(ch));
                to_notvict = "$N calls $n's faith heresy! The bigot!";
                break;
                }
            }
        if ((IS_NEUTRAL(victim)))
            {
            if (IS_NEUTRAL(ch))
                {
                if ((abs(GET_ALIGNMENT(victim))) >= (abs(GET_ALIGNMENT(ch))))
                    {
                    manaadd = -(GET_MANA(victim));
                    send_to_char(ch, "You expose %s's bias and lack of balance!\r\n", GET_NAME(victim));
                    send_to_char(victim, "%s exposes your bias and lack of balance, shaming you before your god!\r\n", GET_NAME(ch));
                    to_notvict = "$N exposes $n's bias and lack of balance!";
                    break;
                    }
                else
                    {
                    send_to_char(ch, "%s laughs at your accusations of imbalance!\r\n", GET_NAME(victim));
                    send_to_char(victim, "You laugh at %s's accusations of imbalance!\r\n", GET_NAME(ch));
                    to_notvict = "$n laughs at $N's accusations of imbalance!";
                    break;
                    }
                }
            else
                {
                send_to_char(ch, "Your god has no power over %s!\r\n", GET_NAME(victim));
                send_to_char(victim, "%s's god has no power over you!\r\n", GET_NAME(ch));
                to_notvict = "$N calls $n's faith heresy! The bigot!";
                break;
                }
            }
        if (IS_EVIL(victim))
            {
            if (IS_EVIL(ch))
                {
                if ((GET_ALIGNMENT(ch)) <= (GET_ALIGNMENT(victim)))
                    {
                    manaadd = -(GET_MANA(victim));
                    send_to_char(ch, "You laugh maniacially as you strip %s of their dark powers for not being evil enough!\r\n", GET_NAME(victim));
                    send_to_char(victim, "%s laughs maniacally, a vision of pure evil, and strips you of your powers!\r\n", GET_NAME(ch));
                    to_notvict = "$N laughs maniacally at $n for not being evil enough!";
                    break;
                    }
                else
                    {
                    send_to_char(ch, "Your evil is not focused enough to excommunicate %s!\r\n", GET_NAME(victim));
                    send_to_char(victim, "%s presumed to be more versed in the ways of evil than you. The fool!\r\n", GET_NAME(ch));
                    to_notvict = "$N foolishly claims to be more evil than $n! How arrogant.";
                    break;
                    }
                }
            else
                {
                send_to_char(ch, "Your god has no power over %s!\r\n", GET_NAME(victim));
                send_to_char(victim, "%s's god has no power over you!\r\n", GET_NAME(ch));
                to_notvict = "$N calls $n's faith heresy! The bigot!";
                break;
                }
            }
        }
    else
        {
        send_to_char(ch, "Your god is not the source of %s's power!", GET_NAME(victim));
        }
    break;

  case SPELL_HEAL:
  case SPELL_HEAL_LIGHT:
    healing = 60;
    to_vict = "A warm feeling briefly overtakes you.";
    to_notvict = "$N heals $n.";
    to_char = "You heal $N.";
    break;
 
 case SPELL_HEAL_CRITICAL:
    healing = 180;
    send_to_char(victim, "A warm feeling floods your body.\r\n");
    to_notvict = "$N heals $n.";
    to_char = "You heal $N.";
    break;
  case SPELL_HEAL_SERIOUS:
    healing = 120;
    send_to_char(victim, "A warm feeling floods your body.\r\n");
    to_notvict = "$N heals $n.";
    to_char = "You heal $N.";
    break;
  case SPELL_SOVEREIGN_HEAL:
    healing = 240;
    send_to_char(victim, "You glow slightly as warmth floods through your veins.\r\n");
    to_room = "A golden glow surrounds $n.";
    break;
  case SPELL_SYNOSTODWEOMER:
    healing = GET_HIT(ch)/2;
    send_to_char(ch, "You feel weaker as you transfer your life force to %s.\r\n", GET_NAME(victim));
    send_to_char(victim, "You feel revived as %s transfers %s life force to you.\r\n", GET_NAME(ch), HSHR(ch));
    to_room = "$n grows pale as $e transfers $s life force to $N.";
    GET_HIT(ch) = MAX(1, GET_HIT(ch) - healing);
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
     default:
    log("SYSERR: unknown spellnum %d passed to mag_points in magic.c.", spellnum);
    return;
  }

  GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + healing);
  GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);
  GET_MANA(victim) = MIN(GET_MAX_MANA(victim), GET_MANA(victim) + manaadd);

 update_pos(victim);

  if (to_char != NULL)
    act(to_char, FALSE, ch, 0, victim, TO_CHAR);
  if (to_vict != NULL)
    act(to_vict, FALSE, ch, 0, victim, TO_VICT);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
  if (to_notvict != NULL)
    act(to_notvict, TRUE, victim, 0, ch, TO_NOTVICT);


}


void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
		        int spellnum, int type)
{
  int spell = 0, msg_not_affected = TRUE, new_position = GET_POS(victim);
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

  case SPELL_DISPEL_MAGIC:
    if (affected_by_spell(victim, SPELL_PARALYZE) || GET_POS(victim) == POS_PARALYZED) {
      spell = SPELL_PARALYZE;
      msg_not_affected = TRUE;
      new_position = POS_STANDING;
      to_vict = "You can move again!";
      to_room = "$n can move again.";
    }
    if (affected_by_spell(victim, SPELL_SLEEP)) {
      spell = SPELL_SLEEP;
      msg_not_affected = TRUE;
      new_position = POS_RESTING;
      to_vict = "You are awoken by $N";
      to_room = "$n wakes up";
    }
    break;

  case SPELL_DISPEL_SILENCE:
    spell = SPELL_SILENCE;
    msg_not_affected = TRUE;
    to_vict = "Your voice returns to you.";
    to_room = "Sound once again emits from $n.";
    break;
  default:
    log("SYSERR: unknown spellnum %d passed to mag_unaffects in magic.c.", spellnum);
    return;
  }

  if (!affected_by_spell(victim, spell)) {
    if (msg_not_affected)
      send_to_char(ch, "%s", NOEFFECT);
    return;
  }

  affect_from_char(victim, spell);
  if (to_vict != NULL) {
    GET_POS(victim) = new_position;
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
    }
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
    case SPELL_DISPEL_MAGIC:
      if (OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
        REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
        to_char = "$p flickers into view.";
      }
      break;
    case SPELL_INVISIBLE:
      if (!OBJ_FLAGGED(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
        SET_BIT(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
        to_char = "$p vanishes."; }
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
    default:
      log("SYSERR: unknown spellnum %d passed to mag_alter_objs in magic.c.", spellnum);
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
  damage = damage - (damage / 2); // Resist Dam does 50% less
 if (victim->char_specials.vulnerable[type] > 0 || victim->char_specials.vulnerable[ATTACK_MAGIC] > 0)
  damage = damage + (damage / 2);// Vuln to Dam does 50% more
  if (victim->char_specials.immune[type] > 0 || victim->char_specials.immune[ATTACK_MAGIC] > 0) 
  damage = 0;
 return(damage);
}
