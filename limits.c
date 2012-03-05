/* 
************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
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
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "dg_scripts.h"

/* external variables */
void check_dump_on_tick(int nr);
void check_size(struct char_data *ch);
extern int max_exp_gain;
extern int max_exp_loss;
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
extern int immort_level_ok;
extern int use_autowiz;
extern int min_wizlist_lev;
extern int free_rent;
extern const char *pc_class_types[];

/* local functions */
int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
void run_autowiz(void);

void Crash_rentsave(struct char_data *ch, int cost);
int level_exp(int chclass, int level);
char *title_male(int chclass, int level);
char *title_female(int chclass, int level);
void update_char_objects(struct char_data *ch);	/* handler.c */
void reboot_wizlists(void);
void clanlog(struct char_data *ch, const char *str, ...);
void perform_cinfo( int clan_number, const char *messg, ... );
void die(struct char_data *ch);
void affect_update(void);
/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (grafage < 15)
    return (p0);					/* < 15   */
  else if (grafage <= 29)
    return (p1 + (((grafage - 15) * (p2 - p1)) / 15));	/* 15..29 */
  else if (grafage <= 44)
    return (p2 + (((grafage - 30) * (p3 - p2)) / 15));	/* 30..44 */
  else if (grafage <= 59)
    return (p3 + (((grafage - 45) * (p4 - p3)) / 15));	/* 45..59 */
  else if (grafage <= 79)
    return (p4 + (((grafage - 60) * (p5 - p4)) / 20));	/* 60..79 */
  else
    return (p6);					/* >= 80 */
}



/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain *= 2;
      break;
    case POS_RESTING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_SITTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    }

    if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
      gain *= 2;

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON) && !(ch->char_specials.immune[ATTACK_POISON] > 0) && !(ch->char_specials.resist[ATTACK_POISON] > 0))
      gain /= 4;
    else if (AFF_FLAGGED(ch, AFF_POISON) && !(ch->char_specials.immune[ATTACK_POISON] > 0))
      gain /=2;

  if (AFF_FLAGGED(ch, AFF_HASTE))
    gain *= 2;

  if (AFF_FLAGGED(ch, AFF_HEALING_DREAM))
    gain *= 3;

  gain = gain + ch->char_specials.managain;

  return (gain);
}


/* Hitpoint gain pr. game hour */
int hit_gain(struct char_data *ch)
{
  int gain;
  
    if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {

    gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 10, 4);

    /* Class/Level calculations */

    /* Skill/Spell calculations */

     
    /* Position calculations    */

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8);	/* Divide by 8 */
      break;
    }

    if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
      gain /= 2;	/* Ouch. */

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

    if (AFF_FLAGGED(ch, AFF_POISON) && !(ch->char_specials.immune[ATTACK_POISON] > 0) && !(ch->char_specials.resist[ATTACK_POISON] > 0))
      gain /= 4;
    else if (AFF_FLAGGED(ch, AFF_POISON) && !(ch->char_specials.immune[ATTACK_POISON] > 0))
      gain /=2;

  if (AFF_FLAGGED(ch, AFF_HASTE))
    gain *= 2;

  if (AFF_FLAGGED(ch, AFF_HEALING_DREAM))
    gain *= 3;

  gain = gain + ch->char_specials.hitgain;

  return (gain);
}



/* move gain pr. game hour */
int move_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 12, 10);

    /* Class/Level calculations */

    /* Skill/Spell calculations */


    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8);	/* Divide by 8 */
      break;
    }

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

    if (AFF_FLAGGED(ch, AFF_POISON) && !(ch->char_specials.immune[ATTACK_POISON] > 0) && !(ch->char_specials.resist[ATTACK_POISON] > 0))
      gain /= 4;
    else if (AFF_FLAGGED(ch, AFF_POISON) && !(ch->char_specials.immune[ATTACK_POISON] > 0))
      gain /=2;

  if (AFF_FLAGGED(ch, AFF_HASTE))
    gain *= 2;

  if (AFF_FLAGGED(ch, AFF_HEALING_DREAM))
    gain *= 3;

  gain = gain + ch->char_specials.movegain;

  return (gain);
}



void set_title(struct char_data *ch, char *title)
{

   const char *i = NULL;
   char lbuf[MAX_STRING_LENGTH], *buf, buf2[MAX_STRING_LENGTH];

   buf = lbuf;

   if (title == NULL) {
     if (GET_SEX(ch) == SEX_FEMALE)
       sprintf(buf2, "* %s", title_female(GET_CLASS(ch), GET_LEVEL(ch)));
     else
       sprintf(buf2, "* %s", title_male(GET_CLASS(ch), GET_LEVEL(ch)));

   set_title(ch, buf2);

   return;
   }

   if (strlen(title) > MAX_TITLE_LENGTH)
     title[MAX_TITLE_LENGTH] = '\0';

     sprintf(buf2, "%c", '*');

     if (strchr(title, *buf2) == NULL)
     {
	 send_to_char(ch, "Title has to contain the (*) asterisk sign to be valid.\r\nTitle unchanged\r\n");
        return;
       }

   if (GET_TITLE(ch) != NULL) {
    free(GET_TITLE(ch));
    GET_TITLE(ch) = NULL;
     }

  for (;;) {

     if (*title == '*') {
         i = GET_NAME(ch);
         while ((*buf = *(i++)))
         buf++;

	 title++;
     }
     else if (!(*(buf++) = *(title++)))
        break;

 }

   *(--buf) = '\0';

   GET_TITLE(ch) = strdup(lbuf);

}


void run_autowiz(void)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (use_autowiz) {
    size_t res;
    char buf[256];

#if defined(CIRCLE_UNIX)
    res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &",
	min_wizlist_lev, WIZLIST_FILE, LVL_SAINT, IMMLIST_FILE, (int) getpid());
#elif defined(CIRCLE_WINDOWS)
    res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s",
	min_wizlist_lev, WIZLIST_FILE, LVL_SAINT, IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    /* Abusing signed -> unsigned conversion to avoid '-1' check. */
    if (res < sizeof(buf)) {
      mudlog(CMP, LVL_SAINT, FALSE, "Initiating autowiz.");
      system(buf);
      reboot_wizlists();
    } else
      log("Cannot run autowiz: command-line doesn't fit in buffer.");
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}

void gain_exp(struct char_data *ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_SAINT)))
    return;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return;
  }
  if (gain > 0) {
    gain = MIN(max_exp_gain, gain);	/* put a cap on the max gain per kill */

    GET_EXP(ch) += gain;
    while (GET_LEVEL(ch) < LVL_SAINT - immort_level_ok &&
	GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1)) {
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      mudlog(BRF, MAX(LVL_SAINT, GET_INVIS_LEV(ch)), TRUE, "%s advanced %d level%s to level %d.",
		GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
         if (num_levels == 1)
		 perform_cinfo(GET_CLAN(ch), "%s advanced to level %d!", GET_NAME(ch), GET_LEVEL(ch)); 
         else
         perform_cinfo(GET_CLAN(ch), "%s advanced to level %d!", GET_NAME(ch), GET_LEVEL(ch)); 
	  if (num_levels == 1)
		{
       send_to_char(ch, "You rise a level!\r\n");
         if (GET_LEVEL(ch) == 10)
			{
		   send_to_char(ch, "\r\n");
		   send_to_char(ch, "With the sound of the death cry\nringing in your ears you are suddenly\nstruck by a vision of your future. You\nappear stronger, more experienced, and\nmore powerful.  It is time to make an\nimportant decision, you must meet with\nyour guildmaster before continuing on\nyour quest...\r\n\r\n");
           SET_BIT(PLR_FLAGS(ch), PLR_NOEXPGAIN);
		   SET_BIT(PLR_FLAGS(ch), PLR_PROGRESS);
			}
	   if (GET_LEVEL(ch) == 25)
			{
		   send_to_char(ch, "\r\n");
		   send_to_char(ch, "With the sound of the death cry\nringing in your ears you are suddenly\nstruck by a vision of your future. You\nappear stronger, more experienced, and\nmore powerful.  It is time to make an\nimportant decision, you must meet with\nyour guildmaster before continuing on\nyour quest...\r\n\r\n");
           SET_BIT(PLR_FLAGS(ch), PLR_NOEXPGAIN);		 
		   SET_BIT(PLR_FLAGS(ch), PLR_PROGRESS);
			}
		}
	  else
	send_to_char(ch, "You rise %d levels!\r\n", num_levels);

     /*if (GET_SEX(ch) == SEX_FEMALE)
      sprintf(buf,"* %s", title_female(GET_RACE(ch), GET_LEVEL(ch)));
     else
      sprintf(buf,"* %s", title_male(GET_RACE(ch), GET_LEVEL(ch)));*/

      set_title(ch, NULL);

      if (GET_LEVEL(ch) >= LVL_SAINT)
        run_autowiz();
    }
  } else if (gain < 0) {
    gain = MAX(-max_exp_loss, gain);	/* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;
  }
}

void gain_exp_regardless(struct char_data *ch, int gain)
{

  int is_altered = FALSE;
  int num_levels = 0;
  
  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < LVL_IMPL &&
	GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1)) {
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      mudlog(BRF, MAX(LVL_SAINT, GET_INVIS_LEV(ch)), TRUE, "%s advanced %d level%s to level %d.",
		GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
      if (num_levels == 1)
       send_to_char(ch, "You rise a level!\r\n");
	  else
	send_to_char(ch, "You rise %d levels!\r\n", num_levels);

/*     if (GET_SEX(ch) == SEX_FEMALE)
      sprintf(buf,"* %s", title_female(GET_RACE(ch), GET_LEVEL(ch)));
     else
      sprintf(buf,"* %s", title_male(GET_RACE(ch), GET_LEVEL(ch)));*/

      set_title(ch, NULL);


      if (GET_LEVEL(ch) >= LVL_SAINT)
        run_autowiz();
    }
  }
}

void gain_condition(struct char_data *ch, int condition, int value)
{
  bool intoxicated;

  if (IS_NPC(ch) || GET_COND(ch, condition) == -1)	/* No change */
    return;

  if (IS_AFFECTED2(ch, AFF_SUSTAIN)) /* also no change  Anubis */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case FULL:
    send_to_char(ch, "You are hungry.\r\n");
    break;
  case THIRST:
    send_to_char(ch, "You are thirsty.\r\n");
    break;
  case DRUNK:
    if (intoxicated)
      send_to_char(ch, "You are now sober.\r\n");
    break;
  default:
    break;
  }
 }


void check_idling(struct char_data *ch)
{
  if (++(ch->char_specials.timer) > idle_void) {
    if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE) {
      GET_WAS_IN(ch) = IN_ROOM(ch);
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You have been idle, and are pulled into a void.\r\n");
      save_char(ch, GET_LOADROOM(ch));
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    } else if (ch->char_specials.timer > idle_rent_time) {
      if (IN_ROOM(ch) != NOWHERE)
	char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc) {
	STATE(ch->desc) = CON_DISCONNECT;
	/*
	 * For the 'if (d->character)' test in close_socket().
	 * -gg 3/1/98 (Happy anniversary.)
	 */
	ch->desc->character = NULL;
	ch->desc = NULL;
      }
      if (free_rent)
	Crash_rentsave(ch, 0);
      else
	Crash_idlesave(ch);
      mudlog(CMP, LVL_DEITY, TRUE, "%s force-rented and extracted (idle).", GET_NAME(ch));
      extract_char(ch);
    }
  }
}



/* Update PCs, NPCs, and objects * This is the TICK parsing area */
void point_update(void)
{
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;
  int iteration = 0;

   /* rooms */
     room_rnum nr;
       for (nr = 0; nr <= top_of_world ; nr++) {
      check_dump_on_tick(nr);
      }

   /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

 startofloop: /* Haste functionality */

    gain_condition(i, FULL, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);

 iteration++; /* Haste functionality */

  if ((AFF_FLAGGED(i, AFF_HASTE)) && (iteration < 2)) /* Haste functionality */
    goto startofloop;

   iteration = 0; /* Haste functionality */

/*
 *  Check to see if eq should pop off
 *  Point of clarity:
 *  point_update() comes AFTER affect_update()
 *  and thus all affs have been reapplied and
 *  size will be what it should be for THIS
 *  tick.
 */
   check_size(i);

   /* change alignment */
   
   /* Vampire sun damage */
   /* This section should probably go in fight.c's damage() but I'm lazy right now.  Maybe I'll move it one day FRENZY */
    if ((GET_RACE(i) == RACE_VAMPIRE)  && (GET_LEVEL(i) < LVL_SAINT) && OUTSIDE(i)  && (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT) && !IS_NPC(i) && (i->char_specials.vulnerable[ATTACK_LIGHT] > 0))
	damage(i, i, GET_LEVEL(i) * 20, TYPE_SUNDAM);
    else if ((GET_RACE(i) == RACE_VAMPIRE)  && (GET_LEVEL(i) < LVL_SAINT) && OUTSIDE(i)  && (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT) && !IS_NPC(i) && !(i->char_specials.immune[ATTACK_LIGHT] > 0) && !(i->char_specials.resist[ATTACK_LIGHT] > 0))
	damage(i, i, GET_LEVEL(i) * 10, TYPE_SUNDAM);
    else if ((GET_RACE(i) == RACE_VAMPIRE)  && (GET_LEVEL(i) < LVL_SAINT) && OUTSIDE(i)  && (weather_info.sunlight == SUN_RISE || weather_info.sunlight ==  SUN_LIGHT) && !IS_NPC(i) && !(i->char_specials.immune[ATTACK_LIGHT] > 0))
	damage(i, i, GET_LEVEL(i) * 5, TYPE_SUNDAM);


    if (GET_POS(i) >= POS_STUNNED) {
      GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
      GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
      GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));

    if (AFF_FLAGGED(i, AFF_HEALING_DREAM) && IS_FULLY_HEALED(i) ) {
/* need wake messages */
      GET_POS(i) = POS_SITTING;
      affect_from_char(i, SPELL_HEALING_DREAM);
      
     
      }
 
  /* This section should probably go in fight.c's damage() but I'm lazy right now.  Maybe I'll move it one day FRENZY */
   if (AFF_FLAGGED(i, AFF_POISON) && (i->char_specials.vulnerable[ATTACK_POISON] > 0))
      {
       if (damage(i, i, 4, SPELL_POISON) == -1)
	  continue;  /* Oops, they died. */
      }
   else if (AFF_FLAGGED(i, AFF_POISON) && !(i->char_specials.immune[ATTACK_POISON] > 0) && !(i->char_specials.resist[ATTACK_POISON] > 0))
      {
       if (damage(i, i, 2, SPELL_POISON) == -1)
	  continue;  /* Oops, they died. */
      }
   else if (AFF_FLAGGED(i, AFF_POISON) && !(i->char_specials.immune[ATTACK_POISON] > 0))
      {
       if (damage(i, i, 1, SPELL_POISON) == -1)
	  continue;  /* Oops, they died. */
      }
   else if (AFF_FLAGGED(i, AFF_POISON) && i->char_specials.immune[ATTACK_POISON] > 0)
      {
      act("You feel burning poison in your blood, but you manage to ignore it.", TRUE, i, NULL, NULL, TO_CHAR | TO_SLEEP);
      act("$n looks really sick but resists $s sickness.", TRUE, i, NULL, NULL, TO_ROOM);
      }
   /*   if (AFF_FLAGGED(i, AFF_POISON))       Original poison dam, will take this out when I'm sure
      	if (damage(i, i, 2, SPELL_POISON) == -1)    the new stuff works perfectly. - FRENZY
	  continue;   Oops, they died. -gg 6/24/98  */


      if (GET_POS(i) <= POS_STUNNED)
	update_pos(i);
    } else if (GET_POS(i) == POS_INCAP) {
      if (damage(i, i, 1, TYPE_SUFFERING) == -1)
	continue;
    } else if (GET_POS(i) == POS_MORTALLYW) {
      if (damage(i, i, 2, TYPE_SUFFERING) == -1)
	continue;
    }
    if (!IS_NPC(i)) {
      update_char_objects(i);
      if (GET_LEVEL(i) < idle_max_level)
	check_idling(i);
    }
  }

  /* objects */
  for (j = object_list; j; j = next_thing) {
    next_thing = j->next;	/* Next in object list */

 if (j->worn_by) {
    if (OBJ_FLAGGED(j, ITEM_GOOD_FLAGGED))
      GET_ALIGNMENT(j->worn_by) += 15;
    if (OBJ_FLAGGED(j, ITEM_EVIL_FLAGGED))
      GET_ALIGNMENT(j->worn_by) -= 15;
    if (OBJ_FLAGGED(j, ITEM_ANGELIC))
      GET_ALIGNMENT(j->worn_by) += 100;
    if (OBJ_FLAGGED(j, ITEM_DEMONIC))
      GET_ALIGNMENT(j->worn_by) -= 100;
    if (GET_ALIGNMENT(j->worn_by) > MAX_ALIGNMENT)
       GET_ALIGNMENT(j->worn_by) = MAX_ALIGNMENT;
    if (GET_ALIGNMENT(j->worn_by) < MIN_ALIGNMENT)
       GET_ALIGNMENT(j->worn_by) = MIN_ALIGNMENT;

 }
    /* If this is a corpse */
    if (IS_CORPSE(j)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
	GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j)) {

	if (j->carried_by)
	  act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
	else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
	  act("$p decays into dust and blows away.",
	      TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
	  act("$p decays into dust and blows away.",
	      TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
	}
	for (jj = j->contains; jj; jj = next_thing2) {
	  next_thing2 = jj->next_content;	/* Next in inventory */
	  obj_from_obj(jj);

	  if (j->in_obj)
	    obj_to_obj(jj, j->in_obj);
	  else if (j->carried_by)
	    obj_to_room(jj, IN_ROOM(j->carried_by));
	  else if (IN_ROOM(j) != NOWHERE)
	    obj_to_room(jj, IN_ROOM(j));
	  else
	    core_dump();
	}
	extract_obj(j);
      }
    }
    /* If the timer is set, count it down and at 0, try the trigger */
    /* note to .rej hand-patchers: make this last in your point-update() */
    else if (GET_OBJ_TIMER(j)>0) {
      GET_OBJ_TIMER(j)--; 
      if (!GET_OBJ_TIMER(j))
        timer_otrigger(j);
    }
  }
}
