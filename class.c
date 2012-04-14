/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */



#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"

extern int siteok_everyone;
extern void racial_ability_modifiers(struct char_data *ch);
extern void set_height_by_race(struct char_data *ch);
extern void set_weight_by_race(struct char_data *ch);
void clanlog(struct char_data *ch, const char *str, ...);

/* local functions */
void snoop_check(struct char_data *ch);
int parse_class(char arg);
int parse_full_class(char *arg);
bitvector_t find_class_bitvector(const char *arg);
byte saving_throws_nat(int class_num, int type, int level);
int thaco(int class_num, int level);
void roll_real_abils(struct char_data *ch);
void do_start(struct char_data *ch);
int backstab_mult(int level);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(int chclass, int level);
const char *title_male(int chclass, int level);
const char *title_female(int chclass, int level);
byte saving_throws_tot(struct char_data *ch, int type);
/* Names first */

const char *class_abbrevs[] = {
  "Mag",
  "Cle",
  "Thi",
  "War",
  "SKn",
  "Pal",
  "Ass",
  "ChM",
  "Sha",
  "Dru",
  "Ran",
  "Pri",
  "Dis",
  "Cru",
  "Fig",
  "Bar",
  "Mon",
  "Kni",
  "Rog",
  "Brd",
  "Jes",
  "Bla",
  "BHu",
  "BMa",
  "Sor",
  "Enc",
  "Nec",
  "Alc",
  "\n"
};


const char *pc_class_types[] = {
  "Magic User",
  "Cleric",
  "Thief",
  "Warrior",
  "Shadow Knight",
  "Paladin",
  "Assassin",
  "Chaos Mage",
  "Shaman",
  "Druid",
  "Ranger",
  "Priest",
  "Disciple",
  "Crusader",
  "Fighter",
  "Barbarian",
  "Monk",
  "Knight",
  "Rogue",
  "Bard",
  "Jester",
  "Blade",
  "Bounty Hunter",
  "Battle Mage",
  "Sorceror",
  "Enchanter",
  "Necromancer",
  "Alchemist",
  "\n"
};

const char *mob_class_abbrevs[] = {
  "Non",
  "Mag",
  "Cle",
  "Thi",
  "War",
  "SKn",
  "Pal",
  "Ass",
  "ChM",
  "Sha",
  "Dru",
  "Ran",
  "Pri",
  "Dis",
  "Cru",
  "Fig",
  "Bar",
  "Mon",
  "Kni",
  "Rog",
  "Brd",
  "Jes",
  "Bla",
  "BHu",
  "BMa",
  "Sor",
  "Enc",
  "Nec",
  "Alc",
  "\n"
};


const char *mob_class_types[] = {
  "None",
  "Magic User",
  "Cleric",
  "Thief",
  "Warrior",
  "Shadow Knight",
  "Paladin",
  "Assassin",
  "Chaos Mage",
  "Shaman",
  "Druid",
  "Ranger",
  "Priest",
  "Disciple",
  "Crusader",
  "Fighter",
  "Barbarian",
  "Monk",
  "Knight",
  "Rogue",
  "Bard",
  "Jester",
  "Blade",
  "Bounty Hunter",
  "Battle Mage",
  "Sorceror",
  "Enchanter",
  "Necromancer",
  "Alchemist",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n"
"Select a class:\r\n"
"  [P]riest\r\n"
"  [R]ogue\r\n"
"  [F]ighter\r\n"
"  [M]agic-user\r\n";

/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'm': return CLASS_MAGIC_USER;
  case 'f': return CLASS_FIGHTER;
  case 'p': return CLASS_PRIEST;
  case 'r': return CLASS_ROGUE;
  default:  return CLASS_UNDEFINED;
  }
}

int parse_full_class(char *arg)
{

  if(!strcmp(arg, "mage") || !strcmp(arg, "magic") || !strcmp(arg, "magicuser"))
    return CLASS_MAGIC_USER;
  else if(!strcmp(arg, "cleric"))
    return CLASS_CLERIC;
  else if(!strcmp(arg, "warrior"))
    return CLASS_WARRIOR;
  else if(!strcmp(arg, "thief"))
    return CLASS_THIEF;
  else if(!strcmp(arg, "shadow") || !strcmp(arg, "shadowknight"))
    return CLASS_SKNIGHT;
  else if(!strcmp(arg, "paladin"))
    return CLASS_PALADIN;
  else if(!strcmp(arg, "assassin"))
    return CLASS_ASSASSIN;
  else if(!strcmp(arg, "chaos") || !strcmp(arg, "chaosmage"))
    return CLASS_CHAOSMAGE;
  else if(!strcmp(arg, "shaman"))
    return CLASS_SHAMAN;
  else if(!strcmp(arg, "druid"))
    return CLASS_DRUID;
  else if(!strcmp(arg, "ranger"))
    return CLASS_RANGER;
  else if(!strcmp(arg, "priest"))
    return CLASS_PRIEST;
  else if(!strcmp(arg, "disciple"))
    return CLASS_DISCIPLE;
  else if(!strcmp(arg, "crusader"))
    return CLASS_CRUSADER;
  else if(!strcmp(arg, "fighter"))
    return CLASS_FIGHTER;
  else if(!strcmp(arg, "barbarian"))
    return CLASS_BARBARIAN;
  else if(!strcmp(arg, "monk"))
    return CLASS_MONK;
  else if(!strcmp(arg, "knight"))
    return CLASS_KNIGHT;
  else if(!strcmp(arg, "rogue"))
    return CLASS_ROGUE;
  else if(!strcmp(arg, "bard"))
    return CLASS_BARD;
  else if(!strcmp(arg, "jester"))
    return CLASS_JESTER;
  else if(!strcmp(arg, "blade"))
    return CLASS_BLADE;
  else if(!strcmp(arg, "bounty") || !strcmp(arg, "bountyhunter") || !strcmp(arg, "hunter"))
    return CLASS_BOUNTYHUNTER;
  else if(!strcmp(arg, "battle") || !strcmp(arg, "battlemage"))
    return CLASS_BATTLEMAGE;
  else if(!strcmp(arg, "sorceror"))
    return CLASS_SORCEROR;
  else if(!strcmp(arg, "enchanter"))
    return CLASS_ENCHANTER;
  else if(!strcmp(arg, "necromancer"))
    return CLASS_NECROMANCER;
  else if(!strcmp(arg, "alchemist"))
    return CLASS_ALCHEMIST;
  else  
    return CLASS_UNDEFINED;  
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.) up to the limit of your bitvector_t, typically 0-31.
 */
bitvector_t find_class_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_class(arg[rpos]));

  return (ret);
}


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
  /* MAG CLE THI WAR SK  PAL ASS CHM SHA DRU RAN DIS CRU FIG BAR MON KNI ROG BRD JES BLA BHU BMA SOR ENC NEC ALC*/
  {  95, 95, 85, 80, 80, 80, 85, 95, 95, 95, 90, 95, 95, 80, 80, 85, 80, 85, 85, 90, 85, 85, 95, 95, 95, 95, 95}, /* learned level */
  { 100, 100,12, 12, 15, 15, 12, 100,100,100,15, 100,100,15, 15, 15, 15, 12, 12, 12, 12, 12, 100,100,100,100,100},/* max per practice */
  {  25, 25, 0,  0,  0,	 0,  0,  25, 25, 25, 0,  25, 25, 0,  0,  0,  0,  0,  0,  0,  0,  0,  25, 25, 25, 25, 25	},/* min per practice */
  { SPELL,	SPELL,	SKILL,	SKILL,	SKILL,	SKILL,	SKILL,	SPELL, SPELL, SPELL, SKILL, SPELL, SPELL, SKILL, SKILL, SKILL, SKILL, SKILL, SKILL, SKILL, SKILL, SKILL, SPELL, SPELL, SPELL, SPELL	},	/* prac name */
};

/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
struct guild_info_type guild_info[] = {

/* Middenheim */
  { CLASS_MAGIC_USER,	325,	SCMD_NORTH	},
  { CLASS_CLERIC,	503,	SCMD_NORTH	},
  { CLASS_THIEF,	327,	SCMD_NORTH	},
  { CLASS_WARRIOR,	324,	SCMD_NORTH	},
  { CLASS_SKNIGHT,      329,    SCMD_NORTH      },
  { CLASS_PALADIN,      323,    SCMD_NORTH	},
  { CLASS_ASSASSIN,	0,	SCMD_NORTH	},
  { CLASS_CHAOSMAGE,	0,	SCMD_NORTH	},


/* this must go last -- add new guards above! */
  { -1, NOWHERE, -1}
};



/*
 * Saving throws for:
 * MCTW
 *   PARA, ROD, PETRI, BREATH, SPELL
 *     Levels 0-40
 *
 * Do not forget to change extern declaration in magic.c if you add to this.
 */

byte saving_throws_nat(int class_num, int type, int level)
{
  switch (class_num) {
  case CLASS_MAGIC_USER:
  case CLASS_BATTLEMAGE:
  case CLASS_SORCEROR:
  case CLASS_CHAOSMAGE:
  case CLASS_ENCHANTER:
  case CLASS_NECROMANCER:
  case CLASS_ALCHEMIST:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 66;
      case  6: return 65;
      case  7: return 63;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 57;
      case 12: return 55;
      case 13: return 54;
      case 14: return 53;
      case 15: return 53;
      case 16: return 52;
      case 17: return 51;
      case 18: return 50;
      case 19: return 48;
      case 20: return 46;
      case 21: return 45;
      case 22: return 44;
      case 23: return 42;
      case 24: return 40;
      case 25: return 38;
      case 26: return 36;
      case 27: return 34;
      case 28: return 32;
      case 29: return 30;
      case 30: return 28;
      case 31: return  27;
      case 32: return  26;
      case 33: return  25;
      case 34: return  24;
      case 35: return  23;
      case 36: return  22;
      case 37: return  21;
      case 38: return  20;
      case 39: return  19;
      case 40: return  18;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for mage paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
     case  0: return 90;
      case  1: return 55;
      case  2: return 53;
      case  3: return 51;
      case  4: return 49;
      case  5: return 47;
      case  6: return 45;
      case  7: return 43;
      case  8: return 41;
      case  9: return 40;
      case 10: return 39;
      case 11: return 37;
      case 12: return 35;
      case 13: return 33;
      case 14: return 31;
      case 15: return 30;
      case 16: return 29;
      case 17: return 27;
      case 18: return 25;
      case 19: return 23;
      case 20: return 21;
      case 21: return 20;
      case 22: return 19;
      case 23: return 17;
      case 24: return 15;
      case 25: return 14;
      case 26: return 13;
      case 27: return 12;
      case 28: return 11;
      case 29: return 10;
      case 30: return  10;
      case 31: return  10;
      case 32: return  10;
      case 33: return  10;
      case 34: return  10;
      case 35: return  10;
      case 36: return  10;
      case 37: return  10;
      case 38: return  10;
      case 39: return  10;
      case 40: return  10;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;    
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for mage rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
     case  0: return 90;
      case  1: return 65;
      case  2: return 63;
      case  3: return 61;
      case  4: return 59;
      case  5: return 57;
      case  6: return 55;
      case  7: return 53;
      case  8: return 51;
      case  9: return 50;
      case 10: return 49;
      case 11: return 47;
      case 12: return 45;
      case 13: return 43;
      case 14: return 41;
      case 15: return 40;
      case 16: return 39;
      case 17: return 37;
      case 18: return 35;
      case 19: return 33;
      case 20: return 31;
      case 21: return 30;
      case 22: return 29;
      case 23: return 27;
      case 24: return 25;
      case 25: return 23;
      case 26: return 21;
      case 27: return 19;
      case 28: return 17;
      case 29: return 15;
      case 30: return 13;
      case 31: return 13;
      case 32: return 13;
      case 33: return 13;
      case 34: return 13;
      case 35: return 13;
      case 36: return 13;
      case 37: return 13;
      case 38: return 13;
      case 39: return 13;
      case 40: return 13;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;  
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for mage petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 75;
      case  2: return 73;
      case  3: return 71;
      case  4: return 69;
      case  5: return 67;
      case  6: return 65;
      case  7: return 63;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 57;
      case 12: return 55;
      case 13: return 53;
      case 14: return 51;
      case 15: return 50;
      case 16: return 49;
      case 17: return 47;
      case 18: return 45;
      case 19: return 43;
      case 20: return 41;
      case 21: return 40;
      case 22: return 39;
      case 23: return 37;
      case 24: return 35;
      case 25: return 33;
      case 26: return 31;
      case 27: return 29;
      case 28: return 27;
      case 29: return 25;
      case 30: return 23;
      case 31: return 22;
      case 32: return 21;
      case 33: return 20;
      case 34: return 19;
      case 35: return 18;
      case 36: return 17;
      case 37: return 16;
      case 38: return 15;
      case 39: return 14;
      case 40: return 13;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;      
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for mage breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
       case  0: return 90;
      case  1: return 60;
      case  2: return 58;
      case  3: return 56;
      case  4: return 54;
      case  5: return 52;
      case  6: return 50;
      case  7: return 48;
      case  8: return 46;
      case  9: return 45;
      case 10: return 44;
      case 11: return 42;
      case 12: return 40;
      case 13: return 38;
      case 14: return 36;
      case 15: return 35;
      case 16: return 34;
      case 17: return 32;
      case 18: return 30;
      case 19: return 28;
      case 20: return 26;
      case 21: return 25;
      case 22: return 24;
      case 23: return 22;
      case 24: return 20;
      case 25: return 18;
      case 26: return 16;
      case 27: return 14;
      case 28: return 12;
      case 29: return 10;
      case 30: return  8;
      case 31: return  8;
      case 32: return  8;
      case 33: return  8;
      case 34: return  8;
      case 35: return  8;
      case 36: return  8;
      case 37: return  8;
      case 38: return  8;
      case 39: return  8;
      case 40: return  8;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;      
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for mage spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
    break;
  case CLASS_CLERIC:
  case CLASS_PRIEST:
  case CLASS_SHAMAN:
  case CLASS_DRUID:
  case CLASS_RANGER:
  case CLASS_DISCIPLE:
  case CLASS_CRUSADER:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 60;
      case  2: return 59;
      case  3: return 48;
      case  4: return 46;
      case  5: return 45;
      case  6: return 43;
      case  7: return 40;
      case  8: return 37;
      case  9: return 35;
      case 10: return 34;
      case 11: return 33;
      case 12: return 31;
      case 13: return 30;
      case 14: return 29;
      case 15: return 27;
      case 16: return 26;
      case 17: return 25;
      case 18: return 24;
      case 19: return 23;
      case 20: return 22;
      case 21: return 21;
      case 22: return 20;
      case 23: return 18;
      case 24: return 15;
      case 25: return 14;
      case 26: return 12;
      case 27: return 10;
      case 28: return  9;
      case 29: return  8;
      case 30: return  8;
      case 31: return  8;
      case 32: return  8;
      case 33: return  8;
      case 34: return  8;
      case 35: return  8;
      case 36: return  8;
      case 37: return  8;
      case 38: return  8;
      case 39: return  8;
      case 40: return  8;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;    
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for cleric paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 66;
      case  5: return 65;
      case  6: return 63;
      case  7: return 60;
      case  8: return 57;
      case  9: return 55;
      case 10: return 54;
      case 11: return 53;
      case 12: return 51;
      case 13: return 50;
      case 14: return 49;
      case 15: return 47;
      case 16: return 46;
      case 17: return 45;
      case 18: return 44;
      case 19: return 43;
      case 20: return 42;
      case 21: return 41;
      case 22: return 40;
      case 23: return 38;
      case 24: return 35;
      case 25: return 34;
      case 26: return 32;
      case 27: return 30;
      case 28: return 29;
      case 29: return 28;
      case 30: return 27;
      case 31: return 26;
      case 32: return 25;
      case 33: return 24;
      case 34: return 23;
      case 35: return 23;
      case 36: return 22;
      case 37: return 21;
      case 38: return 20;
      case 39: return 19;
      case 40: return 18;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;   
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for cleric rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 65;
      case  2: return 64;
      case  3: return 63;
      case  4: return 61;
      case  5: return 60;
      case  6: return 58;
      case  7: return 55;
      case  8: return 53;
      case  9: return 50;
      case 10: return 49;
      case 11: return 48;
      case 12: return 46;
      case 13: return 45;
      case 14: return 44;
      case 15: return 43;
      case 16: return 41;
      case 17: return 40;
      case 18: return 39;
      case 19: return 38;
      case 20: return 37;
      case 21: return 36;
      case 22: return 35;
      case 23: return 33;
      case 24: return 31;
      case 25: return 29;
      case 26: return 27;
      case 27: return 25;
      case 28: return 24;
      case 29: return 23;
      case 30: return 22;
      case 31: return 21;
      case 32: return 20;
      case 33: return 19;
      case 34: return 18;
      case 35: return 17;
      case 36: return 16;
      case 37: return 15;
      case 38: return 14;
      case 39: return 13;
      case 40: return 12;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;  
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for cleric petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 80;
      case  2: return 79;
      case  3: return 78;
      case  4: return 76;
      case  5: return 75;
      case  6: return 73;
      case  7: return 70;
      case  8: return 67;
      case  9: return 65;
      case 10: return 64;
      case 11: return 63;
      case 12: return 61;
      case 13: return 60;
      case 14: return 59;
      case 15: return 57;
      case 16: return 56;
      case 17: return 55;
      case 18: return 54;
      case 19: return 53;
      case 20: return 52;
      case 21: return 51;
      case 22: return 50;
      case 23: return 48;
      case 24: return 45;
      case 25: return 44;
      case 26: return 42;
      case 27: return 40;
      case 28: return 39;
      case 29: return 38;
      case 30: return 37;
      case 31: return 35;
      case 32: return 33;
      case 33: return 31;
      case 34: return 29;
      case 35: return 27;
      case 36: return 26 ;
      case 37: return 25;
      case 38: return 24;
      case 39: return 23;
      case 40: return 22;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;   
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for cleric breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
     case  0: return 90;
      case  1: return 75;
      case  2: return 74;
      case  3: return 73;
      case  4: return 71;
      case  5: return 70;
      case  6: return 68;
      case  7: return 65;
      case  8: return 63;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 56;
      case 13: return 55;
      case 14: return 54;
      case 15: return 53;
      case 16: return 51;
      case 17: return 50;
      case 18: return 49;
      case 19: return 48;
      case 20: return 47;
      case 21: return 46;
      case 22: return 45;
      case 23: return 43;
      case 24: return 41;
      case 25: return 39;
      case 26: return 37;
      case 27: return 35;
      case 28: return 34;
      case 29: return 33;
      case 30: return 32;
      case 31: return 31;
      case 32: return 30;
      case 33: return 29;
      case 34: return 28;
      case 35: return 27;
      case 36: return 26;
      case 37: return 25;
      case 38: return 24;
      case 39: return 23;
      case 40: return 22;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;     
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for cleric spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
    break;
  case CLASS_THIEF:
  case CLASS_ROGUE:
  case CLASS_BARD:
  case CLASS_JESTER:
  case CLASS_ASSASSIN:
  case CLASS_BLADE:
  case CLASS_BOUNTYHUNTER:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
       case  0: return 90;
      case  1: return 65;
      case  2: return 64;
      case  3: return 63;
      case  4: return 62;
      case  5: return 61;
      case  6: return 60;
      case  7: return 59;
      case  8: return 58;
      case  9: return 57;
      case 10: return 56;
      case 11: return 55;
      case 12: return 54;
      case 13: return 53;
      case 14: return 52;
      case 15: return 51;
      case 16: return 50;
      case 17: return 49;
      case 18: return 48;
      case 19: return 47;
      case 20: return 46;
      case 21: return 45;
      case 22: return 44;
      case 23: return 43;
      case 24: return 42;
      case 25: return 41;
      case 26: return 40;
      case 27: return 39;
      case 28: return 38;
      case 29: return 37;
      case 30: return 36;
      case 31: return 34;
      case 32: return 32;
      case 33: return 30;
      case 34: return 28;
      case 35: return 26;
      case 36: return 25;
      case 37: return 24;
      case 38: return 23;
      case 39: return 22;
      case 40: return 21;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for thief paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
       case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 64;
      case  5: return 62;
      case  6: return 60;
      case  7: return 58;
      case  8: return 56;
      case  9: return 54;
      case 10: return 52;
      case 11: return 50;
      case 12: return 48;
      case 13: return 46;
      case 14: return 44;
      case 15: return 42;
      case 16: return 40;
      case 17: return 38;
      case 18: return 36;
      case 19: return 34;
      case 20: return 32;
      case 21: return 30;
      case 22: return 28;
      case 23: return 26;
      case 24: return 24;
      case 25: return 22;
      case 26: return 20;
      case 27: return 18;
      case 28: return 16;
      case 29: return 14;
      case 30: return 13;
      case 31: return 12;
      case 32: return 11;
      case 33: return 10;
      case 34: return   9;
      case 35: return   8;
      case 36: return   8;
      case 37: return   8;
      case 38: return   8;
      case 39: return   8;
      case 40: return   8;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for thief rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 60;
      case  2: return 59;
      case  3: return 58;
      case  4: return 58;
      case  5: return 56;
      case  6: return 55;
      case  7: return 54;
      case  8: return 53;
      case  9: return 52;
      case 10: return 51;
      case 11: return 50;
      case 12: return 49;
      case 13: return 48;
      case 14: return 47;
      case 15: return 46;
      case 16: return 45;
      case 17: return 44;
      case 18: return 43;
      case 19: return 42;
      case 20: return 41;
      case 21: return 40;
      case 22: return 39;
      case 23: return 38;
      case 24: return 37;
      case 25: return 36;
      case 26: return 35;
      case 27: return 34;
      case 28: return 33;
      case 29: return 32;
      case 30: return 31;
      case 31: return 29;
      case 32: return 27;
      case 33: return 25;
      case 34: return 23;
      case 35: return 21;
      case 36: return 20;
      case 37: return 19;
      case 38: return 18;
      case 39: return 17;
      case 40: return 16;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;  
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for thief petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 80;
      case  2: return 79;
      case  3: return 78;
      case  4: return 77;
      case  5: return 76;
      case  6: return 75;
      case  7: return 74;
      case  8: return 73;
      case  9: return 72;
      case 10: return 71;
      case 11: return 70;
      case 12: return 69;
      case 13: return 68;
      case 14: return 67;
      case 15: return 66;
      case 16: return 65;
      case 17: return 64;
      case 18: return 63;
      case 19: return 62;
      case 20: return 61;
      case 21: return 60;
      case 22: return 59;
      case 23: return 58;
      case 24: return 57;
      case 25: return 56;
      case 26: return 55;
      case 27: return 54;
      case 28: return 53;
      case 29: return 52;
      case 30: return 51;
      case 31: return 47;
      case 32: return 43;
      case 33: return 40;
      case 34: return 36;
      case 35: return 32;
      case 36: return 30;
      case 37: return 28;
      case 38: return 27;
      case 39: return 26;
      case 40: return 25;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;  
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for thief breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 75;
      case  2: return 73;
      case  3: return 71;
      case  4: return 69;
      case  5: return 67;
      case  6: return 65;
      case  7: return 63;
      case  8: return 61;
      case  9: return 59;
      case 10: return 57;
      case 11: return 55;
      case 12: return 53;
      case 13: return 51;
      case 14: return 49;
      case 15: return 47;
      case 16: return 45;
      case 17: return 43;
      case 18: return 41;
      case 19: return 39;
      case 20: return 37;
      case 21: return 35;
      case 22: return 33;
      case 23: return 31;
      case 24: return 29;
      case 25: return 27;
      case 26: return 25;
      case 27: return 23;
      case 28: return 21;
      case 29: return 19;
      case 30: return 17;
      case 31: return 16;
      case 32: return 15;
      case 33: return 14;
      case 34: return 13;
      case 35: return 12;
      case 36: return 10;
      case 37: return 10;
      case 38: return 10;
      case 39: return 10;
      case 40: return 10;
      case 41: return 0;
      case 42: return 0;
      case 43: return 0;
      case 44: return 0;    
      case 45: return 0;
      case 46: return 0;
      case 47: return 0;
	  default:
	log("SYSERR: Missing level for thief spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
    break;
  case CLASS_WARRIOR:
  case CLASS_FIGHTER:
  case CLASS_BARBARIAN:
  case CLASS_MONK:
  case CLASS_KNIGHT:
  case CLASS_SKNIGHT:
  case CLASS_PALADIN:
  switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 65;
      case  5: return 62;
      case  6: return 58;
      case  7: return 55;
      case  8: return 53;
      case  9: return 52;
      case 10: return 50;
      case 11: return 47;
      case 12: return 43;
      case 13: return 40;
      case 14: return 38;
      case 15: return 37;
      case 16: return 35;
      case 17: return 32;
      case 18: return 28;
      case 19: return 25;
      case 20: return 24;
      case 21: return 23;
      case 22: return 22;
      case 23: return 20;
      case 24: return 19;
      case 25: return 17;
      case 26: return 16;
      case 27: return 15;
      case 28: return 14;
      case 29: return 13;
      case 30: return 12;
      case 31: return 11;
      case 32: return 10;
      case 33: return  9;
      case 34: return  8;
      case 35: return  8;
      case 36: return  8;
      case 37: return  8;
      case 38: return  8;
      case 39: return  8;
      case 40: return  8;
      case 41: return  1;	/* Some mobiles. */
      case 42: return  0;
      case 43: return  0;
      case 44: return  0;
      case 45: return  0;
      case 46: return  0;
      case 47: return  0;
      default:
	log("SYSERR: Missing level for warrior paralyzation saving throw.");
	break;	
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 80;
      case  2: return 78;
      case  3: return 77;
      case  4: return 75;
      case  5: return 72;
      case  6: return 68;
      case  7: return 65;
      case  8: return 63;
      case  9: return 62;
      case 10: return 60;
      case 11: return 57;
      case 12: return 53;
      case 13: return 50;
      case 14: return 48;
      case 15: return 47;
      case 16: return 45;
      case 17: return 42;
      case 18: return 38;
      case 19: return 35;
      case 20: return 34;
      case 21: return 33;
      case 22: return 32;
      case 23: return 30;
      case 24: return 29;
      case 25: return 27;
      case 26: return 26;
      case 27: return 25;
      case 28: return 24;
      case 29: return 23;
      case 30: return 22;
      case 31: return 20;
      case 32: return 18;
      case 33: return 16;
      case 34: return 14;
      case 35: return 12;
      case 36: return 10;
      case 37: return  8;
      case 38: return  8;
      case 39: return  8;
      case 40: return  8;
      case 41: return  3;
      case 42: return  2;
      case 43: return  1;
      case 44: return  0;
      case 45: return  0;
      case 46: return  0;
      case 47: return  0;
      default:
	log("SYSERR: Missing level for warrior rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 75;
      case  2: return 73;
      case  3: return 72;
      case  4: return 70;
      case  5: return 67;
      case  6: return 63;
      case  7: return 60;
      case  8: return 58;
      case  9: return 57;
      case 10: return 55;
      case 11: return 52;
      case 12: return 48;
      case 13: return 45;
      case 14: return 43;
      case 15: return 42;
      case 16: return 40;
      case 17: return 37;
      case 18: return 33;
      case 19: return 30;
      case 20: return 29;
      case 21: return 28;
      case 22: return 26;
      case 23: return 25;
      case 24: return 24;
      case 25: return 23;
      case 26: return 21;
      case 27: return 20;
      case 28: return 19;
      case 29: return 18;
      case 30: return 17;
      case 31: return 16;
      case 32: return 15;
      case 33: return 14;
      case 34: return 13;
      case 35: return 12;
      case 36: return 11;
      case 37: return 10;
      case 38: return  9;
      case 39: return  8;
      case 40: return  7;
      case 41: return  6;
      case 42: return  5;
      case 43: return  4;
      case 44: return  3;
      case 45: return  2;
      case 46: return  1;
      case 47: return  0;
      default:
	log("SYSERR: Missing level for warrior petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 85;
      case  2: return 83;
      case  3: return 82;
      case  4: return 80;
      case  5: return 75;
      case  6: return 70;
      case  7: return 65;
      case  8: return 63;
      case  9: return 62;
      case 10: return 60;
      case 11: return 55;
      case 12: return 50;
      case 13: return 45;
      case 14: return 43;
      case 15: return 42;
      case 16: return 40;
      case 17: return 37;
      case 18: return 33;
      case 19: return 30;
      case 20: return 29;
      case 21: return 28;
      case 22: return 26;
      case 23: return 25;
      case 24: return 24;
      case 25: return 23;
      case 26: return 21;
      case 27: return 20;
      case 28: return 19;
      case 29: return 18;
      case 30: return 17;
      case 31: return 16;
      case 32: return 15;
      case 33: return 14;
      case 34: return 13;
      case 35: return 12;
      case 36: return 11;
      case 37: return 10;
      case 38: return  9;
      case 39: return  8;
      case 40: return  8;
      case 41: return  6;
      case 42: return  5;
      case 43: return  4;
      case 44: return  3;
      case 45: return  2;
      case 46: return  1;
      case 47: return  0;
      default:
	log("SYSERR: Missing level for warrior breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 85;
      case  2: return 83;
      case  3: return 82;
      case  4: return 80;
      case  5: return 77;
      case  6: return 73;
      case  7: return 70;
      case  8: return 68;
      case  9: return 67;
      case 10: return 65;
      case 11: return 62;
      case 12: return 58;
      case 13: return 55;
      case 14: return 53;
      case 15: return 52;
      case 16: return 50;
      case 17: return 47;
      case 18: return 43;
      case 19: return 40;
      case 20: return 39;
      case 21: return 38;
      case 22: return 36;
      case 23: return 35;
      case 24: return 34;
      case 25: return 33;
      case 26: return 31;
      case 27: return 30;
      case 28: return 29;
      case 29: return 28;
      case 30: return 27;
      case 31: return 25;
      case 32: return 23;
      case 33: return 21;
      case 34: return 19;
      case 35: return 17;
      case 36: return 15;
      case 37: return 13;
      case 38: return 11;
      case 39: return  9;
      case 40: return  8;
      case 41: return  6;
      case 42: return  5;
      case 43: return  4;
      case 44: return  3;
      case 45: return  2;
      case 46: return  1;
      case 47: return  0;
      default:
	log("SYSERR: Missing level for warrior spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
    break;
  
  default:
    log("SYSERR: Invalid class saving throw.");
    break;
    }

  /* Should not get here unless something is wrong. */
  return 100;
}


byte saving_throws_tot(struct char_data *ch, int type) {
  byte save;
  save = (GET_SAVE(ch, type)+saving_throws_nat(GET_CLASS(ch), type, GET_LEVEL(ch)));
  /* Dwarves and gnomes get intrinsic saves vs. magic */
  if ((type == SAVING_SPELL || type == SAVING_ROD) && (IS_DWARF(ch) || IS_GNOME(ch)))
    save += (-5 * (GET_TOT_CON(ch) / 3.5));
  return save;
}


/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
/* This is no longer actualy to hit ac 0  it is now a bonus to your hitroll
  which would allow the player to hit someone with an ac that is equal */
int thaco(int class_num, int level)
{
  switch (class_num) {
  case CLASS_MAGIC_USER:
  case CLASS_ENCHANTER:
  case CLASS_ALCHEMIST:
  case CLASS_BATTLEMAGE:
  case CLASS_CHAOSMAGE:
  case CLASS_SORCEROR:
  case CLASS_NECROMANCER:
    switch (level) {
    case  0: return 100;
    case  1: return  0;
    case  2: return  0;
    case  3: return  1;
    case  4: return  1;
    case  5: return  1;
    case  6: return  1;
    case  7: return  2;
    case  8: return  2;
    case  9: return  2;
    case 10: return  2;
    case 11: return  3;
    case 12: return  3;
    case 13: return  3;
    case 14: return  3;
    case 15: return  4;
    case 16: return  4;
    case 17: return  4;
    case 18: return  4;
    case 19: return  5;
    case 20: return  5;
    case 21: return  5;
    case 22: return  5;
    case 23: return  6;
    case 24: return  6;
    case 25: return  6;
    case 26: return  6;
    case 27: return  7;
    case 28: return  7;
    case 29: return  7;
    case 30: return  7;
    case 31: return  8;
    case 32: return  8;
    case 33: return  8;
    case 34: return  8;
    case 35: return  9;
    case 36: return  9;
    case 37: return  9;
    case 38: return  9;
    case 39: return  10;
    case 40: return  10;
    case 41: return  20;
    case 42: return  20;
    case 43: return  20;
    case 44: return  20;
    case 45: return  20;
	case 46: return  20;
    case 47: return  20;
	default:
      log("SYSERR: Missing level for mage thac0.");
    }
  case CLASS_CLERIC:
  case CLASS_PRIEST:
  case CLASS_SHAMAN:
  case CLASS_DRUID:
  case CLASS_CRUSADER:
  case CLASS_DISCIPLE:
  case CLASS_RANGER:
    switch (level) {
    case  0: return 100;
    case  1: return  0;
    case  2: return  0;
    case  3: return  1;
    case  4: return  1;
    case  5: return  2;
    case  6: return  2;
    case  7: return  3;
    case  8: return  3;
    case  9: return  3;
    case 10: return  3;
    case 11: return  4;
    case 12: return  4;
    case 13: return  5;
    case 14: return  5;
    case 15: return  6;
    case 16: return  6;
    case 17: return  6;
    case 18: return  6;
    case 19: return  7;
    case 20: return  7;
    case 21: return  8;
    case 22: return  8;
    case 23: return  9;
    case 24: return  9;
    case 25: return  9;
    case 26: return  9;
    case 27: return  10;
    case 28: return  10;
    case 29: return  11;
    case 30: return  11;
    case 31: return  12;
    case 32: return  12;
    case 33: return  12;
    case 34: return  12;
    case 35: return  13;
    case 36: return  13;
    case 37: return  14;
    case 38: return  14;
    case 39: return  15;
    case 40: return  15;
    case 41: return  20;
    case 42: return  20;
    case 43: return  20;
    case 44: return  20;
    case 45: return  20;
	case 46: return  20;
    case 47: return  20;
	default:
      log("SYSERR: Missing level for cleric thac0.");
    }
  case CLASS_THIEF:
  case CLASS_ROGUE:
  case CLASS_ASSASSIN:
  case CLASS_BARD:
  case CLASS_BOUNTYHUNTER:
  case CLASS_JESTER:
  case CLASS_BLADE:
    switch (level) {
    case  0: return 100;
    case  1: return  0;
    case  2: return  0;
    case  3: return  1;
    case  4: return  1;
    case  5: return  2;
    case  6: return  2;
    case  7: return  3;
    case  8: return  3;
    case  9: return  3;
    case 10: return  3;
    case 11: return  4;
    case 12: return  4;
    case 13: return  5;
    case 14: return  5;
    case 15: return  6;
    case 16: return  6;
    case 17: return  6;
    case 18: return  6;
    case 19: return  7;
    case 20: return  7;
    case 21: return  8;
    case 22: return  8;
    case 23: return  9;
    case 24: return  9;
    case 25: return  9;
    case 26: return  9;
    case 27: return  10;
    case 28: return  10;
    case 29: return  11;
    case 30: return  11;
    case 31: return  12;
    case 32: return  12;
    case 33: return  12;
    case 34: return  12;
    case 35: return  13;
    case 36: return  13;
    case 37: return  14;
    case 38: return  14;
    case 39: return  15;
    case 40: return  15;
    case 41: return  20;
    case 42: return  20;
    case 43: return  20;
    case 44: return  20;
    case 45: return  20;
	case 46: return  20;
    case 47: return  20;
	default:
      log("SYSERR: Missing level for thief thac0.");
    }
  case CLASS_WARRIOR:
  case CLASS_FIGHTER:
  case CLASS_BARBARIAN:
  case CLASS_MONK:
  case CLASS_PALADIN:
  case CLASS_KNIGHT:
  case CLASS_SKNIGHT:
    switch (level) {
    case  0: return 100;
    case  1: return  1;
    case  2: return  1;
    case  3: return  2;
    case  4: return  2;
    case  5: return  3;
    case  6: return  3;
    case  7: return  4;
    case  8: return  4;
    case  9: return  5;
    case 10: return  5;
    case 11: return  6;
    case 12: return  6;
    case 13: return  7;
    case 14: return  7;
    case 15: return  8;
    case 16: return  8;
    case 17: return  9;
    case 18: return  9;
    case 19: return  10;
    case 20: return  10;
    case 21: return  11;
    case 22: return  11;
    case 23: return  12;
    case 24: return  12;
    case 25: return  13;
    case 26: return  13;
    case 27: return  14;
    case 28: return  14;
    case 29: return  15;
    case 30: return  15;
    case 31: return  16;
    case 32: return  16;
    case 33: return  17;
    case 34: return  17;
    case 35: return  18;
    case 36: return  18;
    case 37: return  19;
    case 38: return  19;
    case 39: return  20;
    case 40: return  20;
    case 41: return  20;
    case 42: return  20;
    case 43: return  20;
    case 44: return  20;
    case 45: return  20;
	case 46: return  20;
    case 47: return  20;
	default:
      log("SYSERR: Missing level for warrior thac0.");
    }

  default:
    log("SYSERR: Unknown class in thac0 chart.");
  }

  /* Will not get there unless something is wrong. */
  return 100;
}


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(struct char_data *ch)
{
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = rand_number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
	temp ^= table[k];
	table[k] ^= temp;
	temp ^= table[k];
      }
  }

  ch->real_abils.str_add = 0;

  switch (GET_CLASS(ch)) {
  case CLASS_MAGIC_USER:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_PRIEST:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_ROGUE:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_FIGHTER:
    ch->real_abils.str = table[0];
    ch->real_abils.con = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = rand_number(0, 100);
    break;

  }
//  ch->aff_abils = ch->real_abils;
  racial_ability_modifiers(ch);
  ch->aff_abils = ch->real_abils;
  set_height_by_race(ch);
  set_weight_by_race(ch);
}


/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;
  SET_BIT(PRF_FLAGS(ch), PRF_AUTOEXIT); // New players get autoexit by default
  SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_DISPEXP | PRF_DISPTARGET); // DISPLAY ALL default for new players
  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (3 & 1)) | (PRF_COLOR_2 * (3 & 2) >> 1)); // New players - color complete



  set_title(ch, NULL);

  GET_MAX_HIT(ch)  = 10;
  GET_MAX_MANA(ch) = 100;
  GET_MAX_MOVE(ch) = 82;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    break;

  case CLASS_CLERIC:
    break;

  case CLASS_THIEF:
    break;

  case CLASS_WARRIOR:
    break;

  case CLASS_PALADIN:
    break;

  case CLASS_SKNIGHT:
   break;

  case CLASS_ASSASSIN:
   break;

  case CLASS_CHAOSMAGE:
   break;
  }

  advance_level(ch);
  mudlog(BRF, MAX(LVL_SAINT, GET_INVIS_LEV(ch)), TRUE, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
  clanlog(ch, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  if (siteok_everyone)
    SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data *ch)
{
  int add_hp, add_mana = 0, add_move = 0, i;

  add_hp = con_app[GET_CON(ch)].hitp;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
  case CLASS_ENCHANTER:
  case CLASS_NECROMANCER:
  case CLASS_ALCHEMIST:
    add_hp += rand_number(4, 8);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(0, 2);
    break;
        
  case CLASS_BATTLEMAGE:
  case CLASS_SORCEROR:
    add_hp += rand_number(6, 10);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(0, 2);

  case CLASS_CLERIC:
  case CLASS_PRIEST:
  case CLASS_SHAMAN:
  case CLASS_DRUID:
    add_hp += rand_number(5, 10);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(0, 2);
    break;

  case CLASS_THIEF:
  case CLASS_ROGUE:
  case CLASS_BOUNTYHUNTER:  
    add_hp += rand_number(7, 13);
    add_mana = 0;
    add_move = rand_number(1, 3);
    break;

  case CLASS_WARRIOR:
  case CLASS_FIGHTER:
  case CLASS_BARBARIAN:
  case CLASS_MONK:
  case CLASS_KNIGHT:
    add_hp += rand_number(11, 18);
    add_mana = 0;
    add_move = rand_number(1, 3);
    break;
  
  case CLASS_BARD:
  case CLASS_JESTER:
  case CLASS_BLADE:
    add_hp += rand_number(7, 13);
    add_mana = rand_number(GET_LEVEL(ch), (int) (1 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 6);
    add_move = rand_number(1, 3);
    break;
  
  
  case CLASS_PALADIN:
  case CLASS_RANGER: 
    add_hp += rand_number(11, 16);
    add_mana = rand_number(GET_LEVEL(ch), (int) (1 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 6);
    add_move = rand_number(0, 2);
    break;
    
  case CLASS_SKNIGHT:
    add_hp += rand_number(11, 16);
    add_mana = rand_number(GET_LEVEL(ch), (int) (1 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 6);
    add_move = rand_number(0, 2);
    break;
    
  case CLASS_ASSASSIN:
    add_hp += rand_number(9, 15);
    add_mana = 0;
    add_move = rand_number(1, 3);
    break;

  case CLASS_CHAOSMAGE:
    add_hp += rand_number(5, 10);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(0, 2);
    break;         
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  if (IS_MAGIC_USER(ch) || IS_CLERIC(ch) || IS_CHAOSMAGE(ch))
    GET_PRACTICES(ch) += MAX(2, wis_app[GET_WIS(ch)].bonus);
  else
    GET_PRACTICES(ch) += MIN(2, MAX(1, wis_app[GET_WIS(ch)].bonus));

  if (GET_LEVEL(ch) >= LVL_SAINT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }
  
  snoop_check(ch);
  save_char(ch, GET_LOADROOM(ch));
  check_max_on_level(ch);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
  if (level <= 0)
    return 1;	  /* level 0 */
  else if (level <= 5)
    return 3;	  /* level 1 - 5 */
  else if (level <= 11)
    return 4;	  /* level 6 - 11 */
  else if (level <= 17)
    return 5;	  /* level 12 - 17 */
  else if (level <= 23)
    return 6;	  /* level 18 - 23 */
  else if (level <= 28)
    return 7;        /* level 24 - 28 */
  else if (level <= 33)
    return 8;	  /* level 29 - 33 */
  else if (level <= 36)
    return 9;	  /* level 34 - 36 */
  else if (level <= 38)
    return 10;	  /* level 37 - 38 */
  else if (level < LVL_SAINT)
    return 11;	   /* all remaining mortal levels: levels 39 and 40 */
  else
    return 20;	  /* immortals */
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  if (GET_LEVEL(ch) >= LVL_GOD)
   return FALSE;
  
  if (!IS_NPC(ch))
  {  
  
  if (OBJ_CLASS(obj, ITEM_FIGHTERS_ONLY) && IS_FIGHTER_TYPE(ch))
   return FALSE;
   
  if (OBJ_CLASS(obj, ITEM_PRIESTS_ONLY) && IS_PRIEST_TYPE(ch))
   return FALSE;
   
  if (OBJ_CLASS(obj, ITEM_MAGES_ONLY) && IS_MAGE_TYPE(ch))
   return FALSE;

  if (OBJ_CLASS(obj, ITEM_ROGUES_ONLY) && IS_ROGUE_TYPE(ch))
   return FALSE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_MAGE) && IS_MAGIC_USER(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_SKNIGHT) && IS_SKNIGHT(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_PALADIN) && IS_PALADIN(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_ASSASSIN) && IS_ASSASSIN(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_CHAOSMAGE) && IS_CHAOSMAGE(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_SHAMAN) && IS_SHAMAN(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_ANTI_PRIEST) && IS_PRIEST(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_DISCIPLE) && IS_DISCIPLE(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_CRUSADER) && IS_CRUSADER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_FIGHTER) && IS_FIGHTER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_BARBARIAN) && IS_BARBARIAN(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_MONK) && IS_MONK(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_KNIGHT) && IS_KNIGHT(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_ROGUE) && IS_ROGUE(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_BARD) && IS_BARD(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_JESTER) && IS_JESTER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_BLADE) && IS_BLADE(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_BOUNTYHUNTER) && IS_BOUNTYHUNTER(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_ANTI_BATTLEMAGE) && IS_BATTLEMAGE(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_SORCEROR) && IS_SORCEROR(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_ENCHANTER) && IS_ENCHANTER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_NECROMANCER) && IS_NECROMANCER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_ALCHEMIST) && IS_ALCHEMIST(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_CLERIC_ONLY) && IS_CLERIC(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_THIEF_ONLY) && IS_THIEF(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_SKNIGHT_ONLY) && IS_SKNIGHT(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_PALADIN_ONLY) && IS_PALADIN(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_ASSASSIN_ONLY) && IS_ASSASSIN(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_CHAOSMAGE_ONLY) && IS_CHAOSMAGE(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_SHAMAN_ONLY) && IS_SHAMAN(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_DRUID_ONLY) && IS_DRUID(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_RANGER_ONLY) && IS_RANGER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_DISCIPLE_ONLY) && IS_DISCIPLE(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_CRUSADER_ONLY) && IS_CRUSADER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_WARRIOR_ONLY) && IS_WARRIOR(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BARBARIAN_ONLY) && IS_BARBARIAN(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_MONK_ONLY) && IS_MONK(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_KNIGHT_ONLY) && IS_KNIGHT(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BARD_ONLY) && IS_BARD(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_JESTER_ONLY) && IS_JESTER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BLADE_ONLY) && IS_BLADE(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BOUNTYHUNTER_ONLY) && IS_BOUNTYHUNTER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BATTLEMAGE_ONLY) && IS_BATTLEMAGE(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_SORCEROR_ONLY) && IS_SORCEROR(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_ENCHANTER_ONLY) && IS_ENCHANTER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_NECROMANCER_ONLY) && IS_NECROMANCER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_ALCHEMIST_ONLY) && IS_ALCHEMIST(ch))
    return FALSE;
  
  if (!GET_OBJ_CLASS(obj) || OBJ_CLASS(obj, ITEM_CLASSRESERVED) )
  return FALSE;

return TRUE;
}
else
  {  
  
   if (OBJ_CLASS(obj, ITEM_FIGHTERS_ONLY) && IS_MOB_FIGHTER_TYPE(ch))
   return FALSE;
   
  if (OBJ_CLASS(obj, ITEM_PRIESTS_ONLY) && IS_MOB_PRIEST_TYPE(ch))
   return FALSE;
   
  if (OBJ_CLASS(obj, ITEM_MAGES_ONLY) && IS_MOB_MAGE_TYPE(ch))
   return FALSE;
   
  if (OBJ_CLASS(obj, ITEM_ROGUES_ONLY) && IS_MOB_ROGUE_TYPE(ch))
   return FALSE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_MAGE) && IS_MOB_MAGIC_USER(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_ANTI_CLERIC) && IS_MOB_CLERIC(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_ANTI_WARRIOR) && IS_MOB_WARRIOR(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_ANTI_THIEF) && IS_MOB_THIEF(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_SKNIGHT) && IS_MOB_SKNIGHT(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_PALADIN) && IS_MOB_PALADIN(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_ASSASSIN) && IS_MOB_ASSASSIN(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_CHAOSMAGE) && IS_MOB_CHAOSMAGE(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_SHAMAN) && IS_MOB_SHAMAN(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_DRUID) && IS_MOB_DRUID(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_RANGER) && IS_MOB_RANGER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_PRIEST) && IS_MOB_PRIEST(ch))
    return TRUE;
  
  if (OBJ_CLASS(obj, ITEM_ANTI_DISCIPLE) && IS_MOB_DISCIPLE(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_CRUSADER) && IS_MOB_CRUSADER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_FIGHTER) && IS_MOB_FIGHTER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_BARBARIAN) && IS_MOB_BARBARIAN(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_MONK) && IS_MOB_MONK(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_KNIGHT) && IS_MOB_KNIGHT(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_ROGUE) && IS_MOB_ROGUE(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_BARD) && IS_MOB_BARD(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_JESTER) && IS_MOB_JESTER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_BLADE) && IS_MOB_BLADE(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_BOUNTYHUNTER) && IS_MOB_BOUNTYHUNTER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_BATTLEMAGE) && IS_MOB_BATTLEMAGE(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_SORCEROR) && IS_MOB_SORCEROR(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_ENCHANTER) && IS_MOB_ENCHANTER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_NECROMANCER) && IS_MOB_NECROMANCER(ch))
    return TRUE;
    
  if (OBJ_CLASS(obj, ITEM_ANTI_ALCHEMIST) && IS_MOB_ALCHEMIST(ch))
    return TRUE;

  if (OBJ_CLASS(obj, ITEM_CLERIC_ONLY) && IS_MOB_CLERIC(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_THIEF_ONLY) && IS_MOB_THIEF(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_SKNIGHT_ONLY) && IS_MOB_SKNIGHT(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_PALADIN_ONLY) && IS_MOB_PALADIN(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_ASSASSIN_ONLY) && IS_MOB_ASSASSIN(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_CHAOSMAGE_ONLY) && IS_MOB_CHAOSMAGE(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_SHAMAN_ONLY) && IS_MOB_SHAMAN(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_DRUID_ONLY) && IS_MOB_DRUID(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_RANGER_ONLY) && IS_MOB_RANGER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_DISCIPLE_ONLY) && IS_MOB_DISCIPLE(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_CRUSADER_ONLY) && IS_MOB_CRUSADER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_WARRIOR_ONLY) && IS_MOB_WARRIOR(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BARBARIAN_ONLY) && IS_MOB_BARBARIAN(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_MONK_ONLY) && IS_MOB_MONK(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_KNIGHT_ONLY) && IS_MOB_KNIGHT(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BARD_ONLY) && IS_MOB_BARD(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_JESTER_ONLY) && IS_MOB_JESTER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BLADE_ONLY) && IS_MOB_BLADE(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BOUNTYHUNTER_ONLY) && IS_MOB_BOUNTYHUNTER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_BATTLEMAGE_ONLY) && IS_MOB_BATTLEMAGE(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_SORCEROR_ONLY) && IS_MOB_SORCEROR(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_ENCHANTER_ONLY) && IS_MOB_ENCHANTER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_NECROMANCER_ONLY) && IS_MOB_NECROMANCER(ch))
    return FALSE;

  if (OBJ_CLASS(obj, ITEM_ALCHEMIST_ONLY) && IS_MOB_ALCHEMIST(ch))
    return FALSE;
  

  if (!GET_OBJ_CLASS(obj) || OBJ_CLASS(obj, ITEM_CLASSRESERVED) )
  return FALSE;

return TRUE;
}

}


/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
    spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_MAGIC_USER, 1); 
                ^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^  ^
                (first prereq)  (second prereq) (actual skill/spell)     (Class)    (level) 

   The code assumes if there is not a first prereq, there wont be a second prereq.
 */


void init_spell_levels(void)
{
  /* MAGES */
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_MAGIC_USER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_MAGIC_USER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_MAGIC_USER, 1); 
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_SHIELD, CLASS_MAGIC_USER, 1);

  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_MAGIC_USER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_MAGIC_USER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_MAGIC_USER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_MAGIC_USER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_MAGIC_USER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_MAGIC_USER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_MAGIC_USER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_MAGIC_USER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_MAGIC_USER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_PHASE_DOOR, CLASS_MAGIC_USER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_MAGIC_USER, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_MAGIC_USER, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_MAGIC_USER, 7);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ACID_ARROW, CLASS_MAGIC_USER, 7);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_MAGIC_USER, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_MAGIC_USER, 8);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_MAGIC_USER, 8);
  spell_level(SPELL_FLEET_FEET, TYPE_UNDEFINED, SPELL_HASTE, CLASS_MAGIC_USER, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_MAGIC_USER, 9);
  spell_level(SPELL_CHILL_TOUCH, SPELL_SHOCKING_GRASP, SPELL_SPOOK, CLASS_MAGIC_USER, 9);
  spell_level(SPELL_DETECT_MAGIC, TYPE_UNDEFINED, SPELL_DISPEL_MAGIC, CLASS_MAGIC_USER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_MAGIC_USER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_MAGIC_USER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_MAGIC_USER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENERGY_DRAIN, CLASS_MAGIC_USER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURSE, CLASS_MAGIC_USER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_MAGIC_USER, 9);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_CHARM_MONSTER, CLASS_MAGIC_USER, 14);

  
   /* BATTLEMAGE */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_BATTLEMAGE, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_BATTLEMAGE, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_BATTLEMAGE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_BATTLEMAGE, 2);
 spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_CHARM_PERSON, CLASS_BATTLEMAGE, 14);
  
spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_BATTLEMAGE, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_BATTLEMAGE, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_BATTLEMAGE, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_BATTLEMAGE, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_BATTLEMAGE, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_BATTLEMAGE, 4);
  spell_level(SPELL_PHASE_DOOR, TYPE_UNDEFINED, SPELL_SHRINK, CLASS_BATTLEMAGE, 22);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_BATTLEMAGE, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_BATTLEMAGE, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_BATTLEMAGE, 6);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ACID_ARROW, CLASS_BATTLEMAGE, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_BATTLEMAGE, 7);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_BATTLEMAGE, 8);
  spell_level(SPELL_FLEET_FEET, TYPE_UNDEFINED, SPELL_HASTE, CLASS_BATTLEMAGE, 8);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_BATTLEMAGE, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_BATTLEMAGE, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_BATTLEMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_BATTLEMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_BATTLEMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_BATTLEMAGE, 9);
  spell_level(SPELL_CHILL_TOUCH, SPELL_SHOCKING_GRASP, SPELL_SPOOK, CLASS_BATTLEMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENERGY_DRAIN, CLASS_BATTLEMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURSE, CLASS_BATTLEMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_BATTLEMAGE, 9);
  spell_level(SPELL_BURNING_HANDS, TYPE_UNDEFINED, SPELL_FIREBOLT, CLASS_BATTLEMAGE, 11);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FIREBALL, CLASS_BATTLEMAGE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHARM, CLASS_BATTLEMAGE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_BATTLEMAGE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CLENCHED_FIST, CLASS_BATTLEMAGE, 21); 
   
   /*  CHAOS MAGES  */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_CHAOSMAGE, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_CHAOSMAGE, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_CHAOSMAGE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_CHAOSMAGE, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_CHAOSMAGE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_CHAOSMAGE, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_CHAOSMAGE, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_CHAOSMAGE, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_CHAOSMAGE, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_CHAOSMAGE, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_CHAOSMAGE, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_CHAOSMAGE, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_CHAOSMAGE, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_CHAOSMAGE, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_CHAOSMAGE, 6);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ACID_ARROW, CLASS_CHAOSMAGE, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_CHAOSMAGE, 7);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_CHAOSMAGE, 8);
  spell_level(SPELL_FLEET_FEET, TYPE_UNDEFINED, SPELL_HASTE, CLASS_CHAOSMAGE, 8);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_CHAOSMAGE, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_CHAOSMAGE, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_CHAOSMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_CHAOSMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_CHAOSMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_CHAOSMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENERGY_DRAIN, CLASS_CHAOSMAGE, 9);
  spell_level(SPELL_CHILL_TOUCH, SPELL_SHOCKING_GRASP, SPELL_SPOOK, CLASS_CHAOSMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURSE, CLASS_CHAOSMAGE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_CHAOSMAGE, 9);
  spell_level(SPELL_BURNING_HANDS, TYPE_UNDEFINED, SPELL_FIREBOLT, CLASS_CHAOSMAGE, 11);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FIREBALL, CLASS_CHAOSMAGE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHARM, CLASS_CHAOSMAGE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_CHAOSMAGE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CLENCHED_FIST, CLASS_CHAOSMAGE, 21);  
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENCHANT_WEAPON, CLASS_CHAOSMAGE, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CLONE, CLASS_CHAOSMAGE, 30); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ETHEREAL_PROJECTION, CLASS_CHAOSMAGE, 35);
  spell_level(SPELL_ETHEREAL_PROJECTION,TYPE_UNDEFINED, SPELL_ETHEREAL_SPHERE, CLASS_CHAOSMAGE, 40);

  /* SORCORER */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_SORCEROR, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_SORCEROR, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_SORCEROR, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_SORCEROR, 2);
  spell_level(SPELL_SHRINK, TYPE_UNDEFINED, SPELL_ENLARGE, CLASS_SORCEROR, 23);

  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_SORCEROR, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_SORCEROR, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_SORCEROR, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_SORCEROR, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_SORCEROR, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_SORCEROR, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_SORCEROR, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_SORCEROR, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_SORCEROR, 6);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ACID_ARROW, CLASS_SORCEROR, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_SORCEROR, 7);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_SORCEROR, 8);
  spell_level(SPELL_FLEET_FEET, TYPE_UNDEFINED, SPELL_HASTE, CLASS_SORCEROR, 8);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_SORCEROR, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_SORCEROR, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_SORCEROR, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_SORCEROR, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_SORCEROR, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_SORCEROR, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENERGY_DRAIN, CLASS_SORCEROR, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURSE, CLASS_SORCEROR, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_SORCEROR, 9);
  spell_level(SPELL_CHILL_TOUCH, SPELL_SHOCKING_GRASP, SPELL_SPOOK, CLASS_SORCEROR, 9);
  spell_level(SPELL_BURNING_HANDS, TYPE_UNDEFINED, SPELL_FIREBOLT, CLASS_SORCEROR, 11);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FIREBALL, CLASS_SORCEROR, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHARM, CLASS_SORCEROR, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_SORCEROR, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CLENCHED_FIST, CLASS_SORCEROR, 21);  
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENCHANT_WEAPON, CLASS_SORCEROR, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ICE_LANCE, CLASS_SORCEROR, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_PILLAR_OF_FLAME, CLASS_SORCEROR, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ICE_STORM, CLASS_SORCEROR, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BALL_LIGHTNING, CLASS_SORCEROR, 28);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CLONE, CLASS_SORCEROR, 30);
  spell_level(SPELL_CLENCHED_FIST, TYPE_UNDEFINED, SPELL_FLAILING_FISTS, CLASS_SORCEROR, 30);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_METEOR, CLASS_SORCEROR, 30);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_THUNDER_SWARM, CLASS_SORCEROR, 32);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_METEOR_SHOWER, CLASS_SORCEROR, 33);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_PRISMATIC_SPRAY, CLASS_SORCEROR, 33);
  spell_level(SPELL_METEOR_SHOWER, SPELL_PRISMATIC_SPRAY, SPELL_ASTRAL_PROJECTION, CLASS_SORCEROR, 35);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHAIN_LIGHTNING, CLASS_SORCEROR, 37);
  spell_level(SPELL_ASTRAL_PROJECTION, SPELL_CHAIN_LIGHTNING, SPELL_ASTRAL_ASCENSION, CLASS_SORCEROR, 40);

  /* ENCHANTER */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_ENCHANTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_ENCHANTER, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_ENCHANTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_ENCHANTER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_ENCHANTER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_ENCHANTER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_ENCHANTER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_ENCHANTER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_ENCHANTER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_ENCHANTER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_ENCHANTER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_ENCHANTER, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_ENCHANTER, 6);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ACID_ARROW, CLASS_ENCHANTER, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_ENCHANTER, 7);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_ENCHANTER, 8);
  spell_level(SPELL_FLEET_FEET, TYPE_UNDEFINED, SPELL_HASTE, CLASS_ENCHANTER, 8);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_ENCHANTER, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_ENCHANTER, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_ENCHANTER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_ENCHANTER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_ENCHANTER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_ENCHANTER, 9);
  spell_level(SPELL_CHILL_TOUCH, SPELL_SHOCKING_GRASP, SPELL_SPOOK, CLASS_ENCHANTER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENERGY_DRAIN, CLASS_ENCHANTER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURSE, CLASS_ENCHANTER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_ENCHANTER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FIREBALL, CLASS_ENCHANTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHARM, CLASS_ENCHANTER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_ENCHANTER, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENCHANT_WEAPON, CLASS_ENCHANTER, 17);

  /* ALCHEMIST */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_ALCHEMIST, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_ALCHEMIST, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_ALCHEMIST, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_ALCHEMIST, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_ALCHEMIST, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_ALCHEMIST, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_ALCHEMIST, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_ALCHEMIST, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_ALCHEMIST, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_ALCHEMIST, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_ALCHEMIST, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_ALCHEMIST, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_ALCHEMIST, 6);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ACID_ARROW, CLASS_ALCHEMIST, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_ALCHEMIST, 7);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_ALCHEMIST, 8);
  spell_level(SPELL_FLEET_FEET, TYPE_UNDEFINED, SPELL_HASTE, CLASS_ALCHEMIST, 8);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_ALCHEMIST, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_ALCHEMIST, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_ALCHEMIST, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_ALCHEMIST, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_ALCHEMIST, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_ALCHEMIST, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENERGY_DRAIN, CLASS_ALCHEMIST, 9);
  spell_level(SPELL_CHILL_TOUCH, SPELL_SHOCKING_GRASP, SPELL_SPOOK, CLASS_ALCHEMIST, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURSE, CLASS_ALCHEMIST, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_ALCHEMIST, 14);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FIREBALL, CLASS_ALCHEMIST, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHARM, CLASS_ALCHEMIST, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_ALCHEMIST, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENCHANT_WEAPON, CLASS_ALCHEMIST, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENCHANT_WEAPON, CLASS_ALCHEMIST, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CLONE, CLASS_ALCHEMIST, 30);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ASTRAL_PROJECTION, CLASS_ALCHEMIST, 35);
  spell_level(SPELL_ASTRAL_PROJECTION,TYPE_UNDEFINED, SPELL_ETHEREAL_PROJECTION, CLASS_ALCHEMIST, 36);
  spell_level(SPELL_ASTRAL_PROJECTION, SPELL_ETHEREAL_PROJECTION, SPELL_ASTRAL_ASCENSION, CLASS_ALCHEMIST,40);
  spell_level(SPELL_ASTRAL_ASCENSION,TYPE_UNDEFINED, SPELL_ETHEREAL_SPHERE, CLASS_ALCHEMIST, 40);
  
  /* NECROMANCER */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_NECROMANCER, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_WHIP, CLASS_NECROMANCER, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_NECROMANCER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_NECROMANCER, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_NECROMANCER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_NECROMANCER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_NECROMANCER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_NECROMANCER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_NECROMANCER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_NECROMANCER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_NECROMANCER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_NECROMANCER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_NECROMANCER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_NECROMANCER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_NECROMANCER, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_NECROMANCER, 6);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ACID_ARROW, CLASS_NECROMANCER, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_NECROMANCER, 7);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_NECROMANCER, 8);
  spell_level(SPELL_FLEET_FEET, TYPE_UNDEFINED, SPELL_HASTE, CLASS_NECROMANCER, 8);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_NECROMANCER, 8);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_CONTROL_UNDEAD, CLASS_NECROMANCER, 8);
 
 spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_NECROMANCER, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_NECROMANCER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_NECROMANCER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_NECROMANCER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_NECROMANCER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENERGY_DRAIN, CLASS_NECROMANCER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURSE, CLASS_NECROMANCER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_NECROMANCER, 9);
  spell_level(SPELL_CHILL_TOUCH, SPELL_SHOCKING_GRASP, SPELL_SPOOK, CLASS_NECROMANCER, 9);
  spell_level(SPELL_LIGHTNING_BOLT, TYPE_UNDEFINED, SPELL_RESISTANCE_TO_ELEC, CLASS_NECROMANCER, 14);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FIREBALL, CLASS_NECROMANCER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHARM, CLASS_NECROMANCER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_NECROMANCER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ENCHANT_WEAPON, CLASS_NECROMANCER, 17);
  spell_level(SPELL_CHILL_TOUCH, TYPE_UNDEFINED, SPELL_RESISTANCE_TO_COLD, CLASS_NECROMANCER, 20); 
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_GHOUL_GAUNTLET, CLASS_NECROMANCER, 21);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_WITHER, CLASS_NECROMANCER, 25);
  spell_level(SPELL_DETECT_INVIS,TYPE_UNDEFINED, SPELL_BAT_SONAR, CLASS_NECROMANCER, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DEATHS_DOOR, CLASS_NECROMANCER, 26); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DECREPIFY, CLASS_NECROMANCER, 26);
  spell_level(SPELL_ARMOR, SPELL_BLINDNESS, SPELL_SHADOW_ARMOR, CLASS_NECROMANCER, 26);
  spell_level(SPELL_SHADOW_ARMOR, SPELL_BAT_SONAR, SPELL_CLOAK_OF_SHADOWS, CLASS_NECROMANCER, 27);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_SKELETAL_GUISE, CLASS_NECROMANCER, 27);
  spell_level(SPELL_CHARM,SPELL_DEATHS_DOOR, SPELL_ANIMATE_DEAD, CLASS_NECROMANCER, 27);
  spell_level(SPELL_VITALIZE_MANA,SPELL_ENERGY_DRAIN, SPELL_BLOOD_REVEL, CLASS_NECROMANCER, 28);
  spell_level(SPELL_RESISTANCE_TO_ELEC, TYPE_UNDEFINED, SPELL_IMMUNITY_TO_ELEC, CLASS_NECROMANCER, 36);
  
 spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CLONE, CLASS_NECROMANCER, 30);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FINGER_OF_DEATH, CLASS_NECROMANCER, 30);
  spell_level(SPELL_BAT_SONAR,TYPE_UNDEFINED, SPELL_GRANT_BAT_SONAR, CLASS_NECROMANCER, 30);
  spell_level(SPELL_CLOAK_OF_SHADOWS, TYPE_UNDEFINED, SPELL_CLOAK_OF_THE_NIGHT, CLASS_NECROMANCER, 31);
  spell_level(SPELL_CLOAK_OF_THE_NIGHT,TYPE_UNDEFINED, SPELL_CLOAK_OF_DARKNESS, CLASS_NECROMANCER, 35);
  spell_level(SPELL_CLOAK_OF_DARKNESS, SPELL_ANIMATE_DEAD, SPELL_HANG, CLASS_NECROMANCER, 35);
  spell_level(SPELL_DEATHS_DOOR, SPELL_DECREPIFY, SPELL_PARALYZE, CLASS_NECROMANCER, 36);
  spell_level(SPELL_RESISTANCE_TO_COLD, TYPE_UNDEFINED, SPELL_IMMUNITY_TO_COLD, CLASS_NECROMANCER, 36);
  spell_level(SPELL_HANG, SPELL_FINGER_OF_DEATH, SPELL_MASS_SUICIDE, CLASS_NECROMANCER, 40);
  /* PRIESTS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_PRIEST, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_LIGHT, CLASS_PRIEST, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_PRIEST, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_FOOD,CLASS_PRIEST, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_WATER, CLASS_PRIEST, 2);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_MAGICAL_VESTMANTS, CLASS_CLERIC, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MINOR, CLASS_PRIEST, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_PRIEST, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_EVIL, CLASS_PRIEST, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_BLIND, CLASS_PRIEST, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLESS, CLASS_PRIEST, 5);
  spell_level(SPELL_DETECT_EVIL,TYPE_UNDEFINED, SPELL_DETECT_GOOD, CLASS_PRIEST, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MAJOR, CLASS_PRIEST, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_PRIEST, 6);
  spell_level(SPELL_DETECT_GOOD,SPELL_DETECT_EVIL, SPELL_DETECT_NEUTRAL, CLASS_PRIEST, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_PRIEST, 6);
  spell_level(SPELL_DETECT_EVIL,SPELL_DETECT_GOOD, SPELL_DETECT_ALIGN, CLASS_PRIEST, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_PRIEST, 7);
  spell_level(SPELL_DETECT_EVIL,SPELL_BLESS, SPELL_PROT_FROM_EVIL, CLASS_PRIEST, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_PRIEST, 8);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_FLAMESTRIKE, CLASS_PRIEST, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_ARMOR, CLASS_PRIEST, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_CRITIC, CLASS_PRIEST, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_REMOVE_POISON, CLASS_PRIEST, 10);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_WORD_OF_RECALL, CLASS_PRIEST, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_EARTHQUAKE, CLASS_PRIEST, 12);
  spell_level(SPELL_DETECT_EVIL,SPELL_FLAMESTRIKE, SPELL_DISPEL_EVIL, CLASS_PRIEST, 14);
  spell_level(SPELL_DETECT_GOOD,SPELL_POISON, SPELL_DISPEL_GOOD, CLASS_PRIEST, 14);

  
  /* CLERICS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_CLERIC, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_LIGHT, CLASS_CLERIC, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_CLERIC, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_FOOD,CLASS_CLERIC, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_WATER, CLASS_CLERIC, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MINOR, CLASS_CLERIC, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_CLERIC, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_EVIL, CLASS_CLERIC, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_BLIND, CLASS_CLERIC, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLESS, CLASS_CLERIC, 5);
  spell_level(SPELL_DETECT_EVIL,TYPE_UNDEFINED, SPELL_DETECT_GOOD, CLASS_CLERIC, 5);
  spell_level(SPELL_DETECT_ALIGN, TYPE_UNDEFINED, SPELL_SHIELD_AGAINST_EVIL, CLASS_CLERIC, 5);
  spell_level(SPELL_DETECT_ALIGN, TYPE_UNDEFINED, SPELL_SHIELD_AGAINST_GOOD, CLASS_CLERIC, 5);

  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MAJOR, CLASS_CLERIC, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_CLERIC, 6);
  spell_level(SPELL_DETECT_GOOD,SPELL_DETECT_EVIL, SPELL_DETECT_NEUTRAL, CLASS_CLERIC, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_CLERIC, 6);

  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_AID, CLASS_CLERIC, 7);
  spell_level(SPELL_CREATE_FOOD, SPELL_CREATE_WATER, SPELL_VITALITY, CLASS_CLERIC, 7);
  spell_level(SPELL_DETECT_EVIL,SPELL_DETECT_GOOD, SPELL_DETECT_ALIGN, CLASS_CLERIC, 7);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_DISPEL_SILENCE, CLASS_CLERIC, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_CLERIC, 7);
  spell_level(SPELL_DETECT_EVIL,SPELL_BLESS, SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_CLERIC, 8);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_FLAMESTRIKE, CLASS_CLERIC, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_ARMOR, CLASS_CLERIC, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_CRITIC, CLASS_CLERIC, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SUMMON, CLASS_CLERIC, 10);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_REMOVE_POISON, CLASS_CLERIC, 10);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_LIGHT, CLASS_CLERIC, 11);
  spell_level(SPELL_BLINDNESS,TYPE_UNDEFINED, SPELL_SILENCE, CLASS_CLERIC, 11);
  spell_level(SPELL_BLESS,TYPE_UNDEFINED, SPELL_FREE_ACTION, CLASS_CLERIC, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_WORD_OF_RECALL, CLASS_CLERIC, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_EARTHQUAKE, CLASS_CLERIC, 12);
  spell_level(SPELL_SHIELD_AGAINST_EVIL, TYPE_UNDEFINED, SPELL_PROTECTION_FROM_EVIL, CLASS_CLERIC, 13);
  spell_level(SPELL_SHIELD_AGAINST_GOOD, TYPE_UNDEFINED, SPELL_PROTECTION_FROM_GOOD, CLASS_CLERIC, 13);
  spell_level(SPELL_DETECT_EVIL,SPELL_FLAMESTRIKE, SPELL_DISPEL_EVIL, CLASS_CLERIC, 14);
  spell_level(SPELL_DETECT_GOOD,SPELL_POISON, SPELL_DISPEL_GOOD, CLASS_CLERIC, 14);
  spell_level(SPELL_WORD_OF_RECALL, SPELL_SUMMON, SPELL_RECALL_TO_SORIN, CLASS_CLERIC, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SANCTUARY, CLASS_CLERIC, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CALL_LIGHTNING, CLASS_CLERIC, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_SERIOUS, CLASS_CLERIC, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL, CLASS_CLERIC, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_CLERIC, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CONTROL_WEATHER, CLASS_CLERIC, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SENSE_LIFE, CLASS_CLERIC, 18);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HARM, CLASS_CLERIC, 19);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL_SERIOUS, CLASS_CLERIC, 20);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_HEAL, CLASS_CLERIC, 22);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_CRITICAL, CLASS_CLERIC, 23);
  spell_level(SPELL_DISPEL_EVIL, TYPE_UNDEFINED, SPELL_SMITE_EVIL, CLASS_CLERIC, 23);
  spell_level(SPELL_DISPEL_GOOD, TYPE_UNDEFINED, SPELL_SMITE_GOOD, CLASS_CLERIC, 23);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL, CLASS_CLERIC, 24);
 spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL_LIGHT, CLASS_CLERIC, 24);

  spell_level(SPELL_BLESS,TYPE_UNDEFINED, SPELL_REGENERATION, CLASS_CLERIC, 25);
  spell_level(SPELL_DISPEL_EVIL,SPELL_BLESS, SPELL_REMOVE_CURSE, CLASS_CLERIC, 26);

  /* DISCIPLES */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_DISCIPLE, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_LIGHT, CLASS_DISCIPLE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_DISCIPLE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_FOOD,CLASS_DISCIPLE, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_WATER, CLASS_DISCIPLE, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MINOR, CLASS_DISCIPLE, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_DISCIPLE, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_EVIL, CLASS_DISCIPLE, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_BLIND, CLASS_DISCIPLE, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLESS, CLASS_DISCIPLE, 5);
  spell_level(SPELL_DETECT_EVIL,TYPE_UNDEFINED, SPELL_DETECT_GOOD, CLASS_DISCIPLE, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MAJOR, CLASS_DISCIPLE, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_DISCIPLE, 6);
  spell_level(SPELL_DETECT_GOOD,SPELL_DETECT_EVIL, SPELL_DETECT_NEUTRAL, CLASS_DISCIPLE, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_DISCIPLE, 6);
  spell_level(SPELL_DETECT_EVIL,SPELL_DETECT_GOOD, SPELL_DETECT_ALIGN, CLASS_DISCIPLE, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_DISCIPLE, 7);
  spell_level(SPELL_DETECT_EVIL,SPELL_BLESS, SPELL_PROT_FROM_EVIL, CLASS_DISCIPLE, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_DISCIPLE, 8);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_FLAMESTRIKE, CLASS_DISCIPLE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_ARMOR, CLASS_DISCIPLE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_CRITIC, CLASS_DISCIPLE, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SUMMON, CLASS_DISCIPLE, 10);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_REMOVE_POISON, CLASS_DISCIPLE, 10);
  spell_level(SPELL_BLINDNESS,TYPE_UNDEFINED, SPELL_SILENCE, CLASS_DISCIPLE, 11);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_LIGHT, CLASS_DISCIPLE, 11);
  spell_level(SPELL_BLESS,TYPE_UNDEFINED, SPELL_FREE_ACTION, CLASS_DISCIPLE, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_WORD_OF_RECALL, CLASS_DISCIPLE, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_EARTHQUAKE, CLASS_DISCIPLE, 12);
  spell_level(SPELL_DETECT_EVIL,SPELL_FLAMESTRIKE, SPELL_DISPEL_EVIL, CLASS_DISCIPLE, 14);
  spell_level(SPELL_DETECT_GOOD,SPELL_POISON, SPELL_DISPEL_GOOD, CLASS_DISCIPLE, 14);
  spell_level(SPELL_WORD_OF_RECALL, SPELL_SUMMON, SPELL_RECALL_TO_SORIN, CLASS_DISCIPLE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SANCTUARY, CLASS_DISCIPLE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CALL_LIGHTNING, CLASS_DISCIPLE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL, CLASS_DISCIPLE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_SERIOUS, CLASS_DISCIPLE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_DISCIPLE, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CONTROL_WEATHER, CLASS_DISCIPLE, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SENSE_LIFE, CLASS_DISCIPLE, 18);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HARM, CLASS_DISCIPLE, 19);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL_SERIOUS, CLASS_DISCIPLE, 20);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_HEAL, CLASS_DISCIPLE, 22);
  spell_level(SPELL_DISPEL_EVIL, TYPE_UNDEFINED, SPELL_SMITE_EVIL, CLASS_DISCIPLE, 23);
  spell_level(SPELL_DISPEL_GOOD, TYPE_UNDEFINED, SPELL_SMITE_GOOD, CLASS_DISCIPLE, 23);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_CRITICAL, CLASS_DISCIPLE, 23);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL_CRITICAL, CLASS_DISCIPLE, 24);
  spell_level(SPELL_BLESS,TYPE_UNDEFINED, SPELL_REGENERATION, CLASS_DISCIPLE, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DEATHS_DOOR, CLASS_DISCIPLE, 26);
  spell_level(SPELL_DISPEL_EVIL,SPELL_BLESS, SPELL_REMOVE_CURSE, CLASS_DISCIPLE, 26);
  spell_level(SPELL_WORD_OF_RECALL, SPELL_SUMMON, SPELL_GROUP_RECALL, CLASS_DISCIPLE, 27);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_GROUP, CLASS_DISCIPLE, 27);
  spell_level(SPELL_GROUP_RECALL, SPELL_RECALL_TO_SORIN, SPELL_GROUP_SORIN_RECALL, CLASS_DISCIPLE, 28);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MASS_HEAL, CLASS_DISCIPLE, 28);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SOVEREIGN_HEAL, CLASS_DISCIPLE, 30);
  spell_level(SPELL_SMITE_EVIL, TYPE_UNDEFINED, SPELL_HOLY_WORD, CLASS_DISCIPLE, 33);
  spell_level(SPELL_SMITE_GOOD, TYPE_UNDEFINED, SPELL_UNHOLY_WORD, CLASS_DISCIPLE, 33);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DIVINE_HEAL, CLASS_DISCIPLE, 35);

  /* CRUSADERS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_CRUSADER, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_CRUSADER, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_CRUSADER, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_LIGHT, CLASS_CRUSADER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_CRUSADER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_FOOD,CLASS_CRUSADER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_WATER, CLASS_CRUSADER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MINOR, CLASS_CRUSADER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_CRUSADER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_EVIL, CLASS_CRUSADER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_BLIND, CLASS_CRUSADER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLESS, CLASS_CRUSADER, 5);
  spell_level(SPELL_DETECT_EVIL,TYPE_UNDEFINED, SPELL_DETECT_GOOD, CLASS_CRUSADER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MAJOR, CLASS_CRUSADER, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_CRUSADER, 6);
  spell_level(SPELL_DETECT_GOOD,SPELL_DETECT_EVIL, SPELL_DETECT_NEUTRAL, CLASS_CRUSADER, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_CRUSADER, 6);
  spell_level(SPELL_DETECT_EVIL,SPELL_DETECT_GOOD, SPELL_DETECT_ALIGN, CLASS_CRUSADER, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_CRUSADER, 7);
  spell_level(SPELL_DETECT_EVIL,SPELL_BLESS, SPELL_PROT_FROM_EVIL, CLASS_CRUSADER, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_CRUSADER, 8);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_FLAMESTRIKE, CLASS_CRUSADER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_ARMOR, CLASS_CRUSADER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_CRITIC, CLASS_CRUSADER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SUMMON, CLASS_CRUSADER, 10);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_REMOVE_POISON, CLASS_CRUSADER, 10);
  spell_level(SPELL_BLINDNESS,TYPE_UNDEFINED, SPELL_SILENCE, CLASS_CRUSADER, 11);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_LIGHT, CLASS_CRUSADER, 11);
  spell_level(SPELL_BLESS,TYPE_UNDEFINED, SPELL_FREE_ACTION, CLASS_CRUSADER, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_WORD_OF_RECALL, CLASS_CRUSADER, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_EARTHQUAKE, CLASS_CRUSADER, 12);
  spell_level(SPELL_DETECT_EVIL,SPELL_FLAMESTRIKE, SPELL_DISPEL_EVIL, CLASS_CRUSADER, 14);
  spell_level(SPELL_DETECT_GOOD,SPELL_POISON, SPELL_DISPEL_GOOD, CLASS_CRUSADER, 14);
  spell_level(SPELL_WORD_OF_RECALL, SPELL_SUMMON, SPELL_RECALL_TO_SORIN, CLASS_CRUSADER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SANCTUARY, CLASS_CRUSADER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CALL_LIGHTNING, CLASS_CRUSADER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL, CLASS_CRUSADER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_SERIOUS, CLASS_CRUSADER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_CRUSADER, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CONTROL_WEATHER, CLASS_CRUSADER, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SENSE_LIFE, CLASS_CRUSADER, 18);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HARM, CLASS_CRUSADER, 19);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL_SERIOUS, CLASS_CRUSADER, 20);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_HEAL, CLASS_CRUSADER, 22);
  spell_level(SPELL_DISPEL_EVIL, TYPE_UNDEFINED, SPELL_SMITE_EVIL, CLASS_CRUSADER, 23);
  spell_level(SPELL_DISPEL_GOOD, TYPE_UNDEFINED, SPELL_SMITE_GOOD, CLASS_CRUSADER, 23);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VIGORIZE_CRITICAL, CLASS_CRUSADER, 23);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL_CRITICAL, CLASS_CRUSADER, 24);
  spell_level(SPELL_BLESS,TYPE_UNDEFINED, SPELL_REGENERATION, CLASS_CRUSADER, 25);
  spell_level(SPELL_DISPEL_EVIL,SPELL_BLESS, SPELL_REMOVE_CURSE, CLASS_CRUSADER, 26);
  spell_level(SPELL_SMITE_EVIL, TYPE_UNDEFINED, SPELL_HOLY_WORD, CLASS_CRUSADER, 33);
  spell_level(SPELL_SMITE_GOOD, TYPE_UNDEFINED, SPELL_UNHOLY_WORD, CLASS_CRUSADER, 33);
  spell_level(SPELL_HOLY_WORD,SPELL_UNHOLY_WORD, SPELL_EXCOMMUNICATE, CLASS_CRUSADER, 40);
  
  /* SHAMANS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_SHAMAN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_LIGHT, CLASS_SHAMAN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_SHAMAN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_FOOD, CLASS_SHAMAN, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_WATER, CLASS_SHAMAN, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MINOR, CLASS_SHAMAN, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_SHAMAN, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_EVIL, CLASS_SHAMAN, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_BLIND, CLASS_SHAMAN, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLESS, CLASS_SHAMAN, 5);
  spell_level(SPELL_DETECT_EVIL,TYPE_UNDEFINED, SPELL_DETECT_GOOD, CLASS_SHAMAN, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MAJOR, CLASS_SHAMAN, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_SHAMAN, 6);
  spell_level(SPELL_DETECT_GOOD,SPELL_DETECT_EVIL, SPELL_DETECT_NEUTRAL, CLASS_SHAMAN, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_SHAMAN, 6);
  spell_level(SPELL_DETECT_EVIL,SPELL_DETECT_GOOD, SPELL_DETECT_ALIGN, CLASS_SHAMAN, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_SHAMAN, 7);
  spell_level(SPELL_DETECT_EVIL,SPELL_BLESS, SPELL_PROT_FROM_EVIL, CLASS_SHAMAN, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_SHAMAN, 8);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_FLAMESTRIKE, CLASS_SHAMAN, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_ARMOR, CLASS_SHAMAN, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_CRITIC, CLASS_SHAMAN, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_REMOVE_POISON, CLASS_SHAMAN, 10);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ELEMENTAL_HANDS, CLASS_SHAMAN, 11);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_WORD_OF_RECALL, CLASS_SHAMAN, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_EARTHQUAKE, CLASS_SHAMAN, 12);
  spell_level(SPELL_DETECT_EVIL,SPELL_FLAMESTRIKE, SPELL_DISPEL_EVIL, CLASS_SHAMAN, 14);
  spell_level(SPELL_DETECT_GOOD,SPELL_POISON, SPELL_DISPEL_GOOD, CLASS_SHAMAN, 14);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_SHAMAN, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_PILLAR_OF_FLAME, CLASS_SHAMAN, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HORNET_SWARM, CLASS_SHAMAN, 27);



  /* DRUIDS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_DRUID, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_DRUID, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_LIGHT, CLASS_DRUID, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_DRUID, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_FOOD,CLASS_DRUID, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_WATER, CLASS_DRUID, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MINOR, CLASS_DRUID, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_DRUID, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_EVIL, CLASS_DRUID, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_BLIND, CLASS_DRUID, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLESS, CLASS_DRUID, 5);
  spell_level(SPELL_DETECT_EVIL,TYPE_UNDEFINED, SPELL_DETECT_GOOD, CLASS_DRUID, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MAJOR, CLASS_DRUID, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_DRUID, 6);
  spell_level(SPELL_DETECT_GOOD,SPELL_DETECT_EVIL, SPELL_DETECT_NEUTRAL, CLASS_DRUID, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_DRUID, 6);
  spell_level(SPELL_DETECT_EVIL,SPELL_DETECT_GOOD, SPELL_DETECT_ALIGN, CLASS_DRUID, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_DRUID, 7);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_SUNRAY, CLASS_DRUID, 8);
  spell_level(SPELL_DETECT_EVIL,SPELL_BLESS, SPELL_PROT_FROM_EVIL, CLASS_DRUID, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_DRUID, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLAMESTRIKE, CLASS_DRUID, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_ARMOR, CLASS_DRUID, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_CRITIC, CLASS_DRUID, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_REMOVE_POISON, CLASS_DRUID, 10);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ELEMENTAL_HANDS, CLASS_DRUID, 11);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_WORD_OF_RECALL, CLASS_DRUID, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_EARTHQUAKE, CLASS_DRUID, 12);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ELEMENTAL_SHIELD, CLASS_DRUID, 13);
  spell_level(SPELL_DETECT_EVIL,SPELL_FLAMESTRIKE, SPELL_DISPEL_EVIL, CLASS_DRUID, 14);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_CONTROL_PLANT, CLASS_DRUID, 14);
  spell_level(SPELL_DETECT_GOOD,SPELL_POISON, SPELL_DISPEL_GOOD, CLASS_DRUID, 14);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_DRUID, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_PILLAR_OF_FLAME, CLASS_DRUID, 26);
  spell_level(SPELL_ELEMENTAL_HANDS, TYPE_UNDEFINED, SPELL_ELEMENTAL_STRIKE, CLASS_DRUID, 26);
  spell_level(SPELL_CREATE_FOOD,SPELL_CREATE_WATER, SPELL_SUSTAIN, CLASS_DRUID, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HORNET_SWARM, CLASS_DRUID, 27);
  spell_level(SPELL_ELEMENTAL_STRIKE, TYPE_UNDEFINED, SPELL_ELEMENTAL_BURST, CLASS_DRUID, 28);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ICE_LANCE, CLASS_DRUID, 28);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ICE_STORM, CLASS_DRUID, 29);
  spell_level(SPELL_ELEMENTAL_SHIELD, TYPE_UNDEFINED, SPELL_ELEMENTAL_AURA, CLASS_DRUID, 29);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BOULDER, CLASS_DRUID, 31);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_THUNDER_SWARM, CLASS_DRUID, 32);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SEARING_ORB, CLASS_DRUID, 33);
  spell_level(SPELL_SUSTAIN,SPELL_GROUP_ARMOR, SPELL_SUSTAIN_GROUP, CLASS_DRUID, 26);
  spell_level(SPELL_ELEMENTAL_BLAST, SPELL_HEALING_WIND, SPELL_ARBOREAL_FORM, CLASS_DRUID, 35);
  spell_level(SPELL_ELEMENTAL_BURST, TYPE_UNDEFINED, SPELL_ELEMENTAL_BLAST, CLASS_DRUID, 35);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEALING_WIND, CLASS_DRUID, 35);
  spell_level(SPELL_ARBOREAL_FORM, SPELL_GROUP_ARMOR, SPELL_FORESTATION, CLASS_DRUID, 40);
  
  
  /* RANGERS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_RANGER, 1);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ANIMAL_FRIENDSHIP, CLASS_RANGER, 1);

  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_RANGER, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_RANGER, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_LIGHT, CLASS_RANGER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_RANGER, 1);
  spell_level(SPELL_ANIMAL_FRIENDSHIP, TYPE_UNDEFINED, SPELL_CHARM_BEAST, CLASS_RANGER, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_FOOD, CLASS_RANGER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CREATE_WATER, CLASS_RANGER, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MINOR, CLASS_RANGER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_RANGER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_EVIL, CLASS_RANGER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_BLIND, CLASS_RANGER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLESS, CLASS_RANGER, 5);
  spell_level(SPELL_DETECT_EVIL,TYPE_UNDEFINED, SPELL_DETECT_GOOD, CLASS_RANGER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CAUSE_MAJOR, CLASS_RANGER, 6);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_RANGER, 6);
  spell_level(SPELL_DETECT_GOOD,SPELL_DETECT_EVIL, SPELL_DETECT_NEUTRAL, CLASS_RANGER, 6);

  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_BARKSKIN, CLASS_RANGER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_RANGER, 6);
  spell_level(SPELL_DETECT_EVIL,SPELL_DETECT_GOOD, SPELL_DETECT_ALIGN, CLASS_RANGER, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_RANGER, 7);
  spell_level(SPELL_DETECT_EVIL,SPELL_BLESS, SPELL_PROT_FROM_EVIL, CLASS_RANGER, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_POISON, CLASS_RANGER, 8);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLAMESTRIKE, CLASS_DRUID, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_GROUP_ARMOR, CLASS_RANGER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_CRITIC, CLASS_RANGER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_REMOVE_POISON, CLASS_RANGER, 10);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ELEMENTAL_HANDS, CLASS_RANGER, 11);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_WORD_OF_RECALL, CLASS_RANGER, 12);
  
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_EARTHQUAKE, CLASS_RANGER, 12);
  spell_level(SPELL_DETECT_EVIL,SPELL_FLAMESTRIKE, SPELL_DISPEL_EVIL, CLASS_RANGER, 14);
  spell_level(SPELL_DETECT_GOOD,SPELL_POISON, SPELL_DISPEL_GOOD, CLASS_RANGER, 14);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_RANGER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BOLT_OF_STEEL, CLASS_RANGER, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FREE_ACTION, CLASS_RANGER, 26);
  spell_level(SPELL_CREATE_FOOD,SPELL_CREATE_WATER, SPELL_SUSTAIN, CLASS_RANGER, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HORNET_SWARM, CLASS_RANGER, 27);
  spell_level(SPELL_BOLT_OF_STEEL, TYPE_UNDEFINED, SPELL_FLAMING_ARROW, CLASS_RANGER, 27);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ICE_LANCE, CLASS_RANGER, 28);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ICE_STORM, CLASS_RANGER, 29);
  spell_level(SPELL_FLAMING_ARROW, TYPE_UNDEFINED, SPELL_HAIL_OF_ARROWS, CLASS_RANGER, 30);
  spell_level(SPELL_HAIL_OF_ARROWS, TYPE_UNDEFINED, SPELL_HORNETS_DART, CLASS_RANGER, 31);

  
  /* ROGUES */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_SNEAK, CLASS_ROGUE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_ROGUE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_ROGUE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_STEAL, CLASS_ROGUE, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_HIDE, CLASS_ROGUE, 5);
 

  /* THIEVES */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_SNEAK, CLASS_THIEF, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_THIEF, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_THIEF, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PICK_LOCK, CLASS_THIEF, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BACKSTAB, CLASS_THIEF, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_STEAL, CLASS_THIEF, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_HIDE, CLASS_THIEF, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_TRACK, CLASS_THIEF, 15);

  /*   ASSASSINS	*/
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_ASSASSIN, 25); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_SNEAK, CLASS_ASSASSIN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_ASSASSIN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_ASSASSIN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PICK_LOCK, CLASS_ASSASSIN, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BACKSTAB, CLASS_ASSASSIN, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_STEAL, CLASS_ASSASSIN, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_HIDE, CLASS_ASSASSIN, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_TRACK, CLASS_ASSASSIN, 15);
  spell_level(SKILL_BACKSTAB,TYPE_UNDEFINED, SKILL_CIRCLE, CLASS_ASSASSIN, 26);
  
  /* Bounty Hunter */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_SNEAK, CLASS_BOUNTYHUNTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_BOUNTYHUNTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_BOUNTYHUNTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PICK_LOCK, CLASS_BOUNTYHUNTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BACKSTAB, CLASS_BOUNTYHUNTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_STEAL, CLASS_BOUNTYHUNTER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_HIDE, CLASS_BOUNTYHUNTER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_TRACK, CLASS_BOUNTYHUNTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_BOUNTYHUNTER, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_BOUNTYHUNTER, 26);
  
  /* BARDS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_SNEAK, CLASS_BARD, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_BARD, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_BARD, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_STEAL, CLASS_BARD, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_HIDE, CLASS_BARD, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FREE_ACTION, CLASS_BARD, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_BARD, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_BARD, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_BARD, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_BARD, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_BARD, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_BARD, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_BARD, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_BARD, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_BARD, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_BARD, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_BARD, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_BARD, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_BARD, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_BARD, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_BARD, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ROAR, CLASS_BARD, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_BARD, 18);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_BARD, 19);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SPOOK, CLASS_BARD, 19);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_BARD, 20);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_BARD, 20);
  spell_level(SPELL_LOCATE_OBJECT,TYPE_UNDEFINED, SPELL_TELEVIEW_MINOR, CLASS_BARD, 21);
  spell_level(SPELL_TELEVIEW_MINOR,TYPE_UNDEFINED, SPELL_TELEVIEW_MAJOR, CLASS_BARD, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_BARD, 20);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_BARD, 22);

  /* JESTER */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_SNEAK, CLASS_JESTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_JESTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_JESTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_STEAL, CLASS_JESTER, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_HIDE, CLASS_JESTER, 5);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FREE_ACTION, CLASS_JESTER, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_JESTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_JESTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_JESTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_JESTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_JESTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_JESTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_JESTER, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_JESTER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_JESTER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_JESTER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_JESTER, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_JESTER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_JESTER, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_JESTER, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ROAR, CLASS_JESTER, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_JESTER, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_JESTER, 18);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_JESTER, 19);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SPOOK, CLASS_JESTER, 19);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_JESTER, 20);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_JESTER, 20);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_JESTER, 20);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_JESTER, 22);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SONIC_BLAST, CLASS_JESTER, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DERVISH_SPIN, CLASS_JESTER, 35);
  spell_level(SPELL_SONIC_BLAST, TYPE_UNDEFINED, SPELL_WAIL_OF_THE_BANSHEE, CLASS_JESTER, 35);
  spell_level(SPELL_AIRWALK, SPELL_LOCATE_OBJECT, SPELL_ARCANE_LORE, CLASS_JESTER, 35);
  spell_level(SPELL_ARCANE_LORE,TYPE_UNDEFINED, SPELL_TALES_OF_ARCANE_LORE, CLASS_JESTER, 40);

  /* BLADE */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_SNEAK, CLASS_BLADE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_BLADE, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_BLADE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_BLADE, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_STEAL, CLASS_BLADE, 4);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_HIDE, CLASS_BLADE, 5);      
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FREE_ACTION, CLASS_BLADE, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_MAGIC_MISSILE, CLASS_BLADE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_BLADE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_MAGIC, CLASS_BLADE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHILL_TOUCH, CLASS_BLADE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_COLOR_SPRAY, CLASS_BLADE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FEATHER_FALL, CLASS_BLADE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INFRAVISION, CLASS_BLADE, 15);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_BLADE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_INVISIBLE, CLASS_BLADE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHOCKING_GRASP, CLASS_BLADE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_VITALIZE_MANA, CLASS_BLADE, 16); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LIGHTNING_BOLT, CLASS_BLADE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLINDNESS, CLASS_BLADE, 16);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_BLADE, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ARMOR, CLASS_BLADE, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_ROAR, CLASS_BARD, 17);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BURNING_HANDS, CLASS_BLADE, 18);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_BLADE, 19);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SPOOK, CLASS_BLADE, 19);
  spell_level(SPELL_FLEET_FEET,SPELL_FEATHER_FALL, SPELL_AIRWALK, CLASS_BLADE, 20);
  spell_level(SPELL_INVISIBLE,SPELL_INFRAVISION, SPELL_IMPROVED_INVISIBILITY, CLASS_BLADE, 20);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_STRENGTH, CLASS_BLADE, 20);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SLEEP, CLASS_BLADE, 22);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DERVISH_SPIN, CLASS_BLADE, 35);
  spell_level(SPELL_SONIC_BLAST, TYPE_UNDEFINED, SPELL_WAIL_OF_THE_BANSHEE, CLASS_BLADE, 35);

  /* FIGHTERS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_FIGHTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_FIGHTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_FIGHTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_KICK, CLASS_FIGHTER, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_RESCUE, CLASS_FIGHTER, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_TRACK, CLASS_FIGHTER, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BASH, CLASS_FIGHTER, 12);
 spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_RAGE, CLASS_FIGHTER, 12);

  /* WARRIORS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_WARRIOR, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_WARRIOR, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_WARRIOR, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_KICK, CLASS_WARRIOR, 1);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_ACCURACY, CLASS_WARRIOR, 2);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_RESCUE, CLASS_WARRIOR, 3);
   spell_level(SPELL_ACCURACY, TYPE_UNDEFINED, SPELL_STRENGTH_BURST, CLASS_WARRIOR, 7);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_TRACK, CLASS_WARRIOR, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BASH, CLASS_WARRIOR, 12);
 
   /* MONKS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_MONK, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_MONK, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_MONK, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_KICK, CLASS_MONK, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_RESCUE, CLASS_MONK, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_TRACK, CLASS_MONK, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BASH, CLASS_MONK, 12);
  spell_level(SPELL_AIRWALK, TYPE_UNDEFINED, SPELL_FLAMEWALK, CLASS_MONK, 20);
  spell_level(SPELL_FLAMEWALK, TYPE_UNDEFINED, SPELL_WINDWALK, CLASS_MONK, 23);

  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_BLIND, CLASS_MONK, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CHI_FIST, CLASS_MONK, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FLEET_FEET, CLASS_MONK, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_LOCATE_OBJECT, CLASS_MONK, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_INVIS, CLASS_MONK, 26);
  spell_level(SPELL_DETECT_INVIS,TYPE_UNDEFINED, SPELL_DETECT_POISON, CLASS_MONK, 26);
  spell_level(SPELL_DETECT_INVIS,TYPE_UNDEFINED, SPELL_INVISIBILITY_TO_ENEMIES, CLASS_MONK, 27);
  spell_level(SPELL_DETECT_POISON,TYPE_UNDEFINED, SPELL_REMOVE_POISON, CLASS_MONK, 27);
  spell_level(SPELL_LOCATE_OBJECT,SPELL_DETECT_INVIS,SPELL_SECOND_SIGHT, CLASS_MONK, 27);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_WATERWALK, CLASS_MONK, 27);
  spell_level(SPELL_DETECT_INVIS, TYPE_UNDEFINED, SPELL_SENSE_LIFE, CLASS_MONK, 27);
  spell_level(SPELL_LOCATE_OBJECT,TYPE_UNDEFINED, SPELL_TELEVIEW_MINOR, CLASS_BARD, 27);
  spell_level(SPELL_INVISIBILITY_TO_ENEMIES,TYPE_UNDEFINED, SPELL_BENEFICENCE, CLASS_BARD, 28);
  spell_level(SPELL_FLEET_FEET,SPELL_WATERWALK, SPELL_AIRWALK, CLASS_MONK, 28);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEALING_DREAM, CLASS_MONK, 34);
  spell_level(SPELL_TELEVIEW_MINOR,SPELL_AIRWALK, SPELL_TELEVIEW_MAJOR, CLASS_MONK, 35);
  spell_level(SPELL_TELEVIEW_MAJOR,SPELL_SECOND_SIGHT,SPELL_TELEPORT, CLASS_MONK, 35);

  /* BARBARIAN */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_BARBARIAN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_BARBARIAN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_BARBARIAN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_KICK, CLASS_BARBARIAN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_RESCUE, CLASS_BARBARIAN, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_TRACK, CLASS_BARBARIAN, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BASH, CLASS_BARBARIAN, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_FURY, CLASS_BARBARIAN, 26);
  spell_level(SKILL_FURY,TYPE_UNDEFINED, SPELL_BLOODLUST, CLASS_BARBARIAN, 27);
  spell_level(SKILL_WHIRLWIND, TYPE_UNDEFINED, SPELL_DEATH_STRIKE, CLASS_BARBARIAN, 35);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SKILL_WHIRLWIND, CLASS_BARBARIAN, 25);
  /* KNIGHTS */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_KNIGHT, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_KNIGHT, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_KNIGHT, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_KICK, CLASS_KNIGHT, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_RESCUE, CLASS_KNIGHT, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_TRACK, CLASS_KNIGHT, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BASH, CLASS_KNIGHT, 12);


  /* SKs */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_WHIP, CLASS_SKNIGHT, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_SKNIGHT, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_SKNIGHT, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_SKNIGHT, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_KICK, CLASS_SKNIGHT, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_RESCUE, CLASS_SKNIGHT, 3);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_TRACK, CLASS_SKNIGHT, 9);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BASH, CLASS_SKNIGHT, 12);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SHADOW_ARMOR, CLASS_SKNIGHT, 26);
  spell_level(SPELL_SHADOW_ARMOR,TYPE_UNDEFINED, SPELL_CLOAK_OF_SHADOWS, CLASS_SKNIGHT, 28);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FINGER_OF_DEATH, CLASS_SKNIGHT, 32);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_FORT, CLASS_SKNIGHT, 35);
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_REFLECT_DAMAGE, CLASS_SKNIGHT, 35);
    spell_level(SPELL_SOMNOLENT_GAZE, TYPE_UNDEFINED, SPELL_ASPHYXIATE, CLASS_SKNIGHT, 29);
  /* Pal */
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_PIERCE, CLASS_PALADIN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_SLASH, CLASS_PALADIN, 1);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_PROF_BLUDGEON, CLASS_PALADIN, 1); 
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_RESCUE, CLASS_PALADIN, 1);
  /* spell_level(SKILL_LAYHANDS, CLASS_PALADIN, 5); */
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_SYNOSTODWEOMER, CLASS_PALADIN, 15);

  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SKILL_BASH, CLASS_PALADIN, 12);
  spell_level(SPELL_MAGICAL_VESTMANTS, TYPE_UNDEFINED, SPELL_DRAW_UPON_HOLY_MIGHT, CLASS_PALADIN, 11);  
  spell_level(TYPE_UNDEFINED, TYPE_UNDEFINED, SPELL_TOWER_OF_STRENGTH, CLASS_PALADIN, 25);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_LIGHT, CLASS_PALADIN, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_DETECT_EVIL, CLASS_PALADIN, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_BLESS, CLASS_PALADIN, 26);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_CURE_CRITIC, CLASS_PALADIN, 27);
  spell_level(SPELL_DETECT_EVIL,SPELL_BLESS, SPELL_DISPEL_EVIL, CLASS_PALADIN, 27);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_HEAL, CLASS_PALADIN, 35);
  spell_level(TYPE_UNDEFINED,TYPE_UNDEFINED, SPELL_SANCTUARY, CLASS_PALADIN, 35);


}


/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */
#define EXP_MAX  70000000

/* Function to return the exp required for each class/level */
int level_exp(int chclass, int level)
{
  if (level > LVL_IMPL || level < 0) {
    log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }

  /*
   * Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist.
   */
   if (level > LVL_SAINT) {
     return EXP_MAX - ((LVL_IMPL - level) * 1000);
   }

  /* Exp required for normal mortals is below */

  switch (chclass) {

    case CLASS_MAGIC_USER:
    case CLASS_BATTLEMAGE:
    case CLASS_SORCEROR:
    case CLASS_ENCHANTER:
    case CLASS_NECROMANCER:
    case CLASS_ALCHEMIST:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5000;
      case  4: return 10000;
      case  5: return 20000;
      case  6: return 40000;
      case  7: return 60000;
      case  8: return 90000;
      case  9: return 135000;
      case 10: return 250000;
      case 11: return 375000;
      case 12: return 750000;
      case 13: return 1125000;
      case 14: return 1500000;
      case 15: return 1875000;
      case 16: return 2250000;
      case 17: return 2625000;
      case 18: return 3000000;
      case 19: return 3375000;
      case 20: return 3750000;
      case 21: return 4000000;
      case 22: return 4300000;
      case 23: return 4600000;
      case 24: return 4900000;
      case 25: return 5200000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8050000;
      case 32: return 8700000;
      case 33: return 9450000;
      case 34: return 10300000;
      case 35: return 11250000;
      case 36: return 12250000;   
      case 37: return 13250000;
      case 38: return 14250000;
      case 39: return 15250000;
      case 40: return 16250000;  
      /* add new levels here */
      case LVL_SAINT: return 46250000;
    }
    break;

    case CLASS_CLERIC:
    case CLASS_PRIEST:
    case CLASS_SHAMAN:
    case CLASS_DRUID:
    case CLASS_RANGER:
    case CLASS_DISCIPLE:
    case CLASS_CRUSADER:    
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 1500;
      case  3: return 3000;
      case  4: return 6000;
      case  5: return 13000;
      case  6: return 27500;
      case  7: return 55000;
      case  8: return 110000;
      case  9: return 225000;
      case 10: return 450000;
      case 11: return 675000;
      case 12: return 900000;
      case 13: return 1125000;
      case 14: return 1350000;
      case 15: return 1575000;
      case 16: return 1800000;
      case 17: return 2100000;
      case 18: return 2400000;
      case 19: return 2700000;
      case 20: return 3000000;
      case 21: return 3250000;
      case 22: return 3500000;
      case 23: return 3800000;
      case 24: return 4100000;
      case 25: return 4400000;
      case 26: return 4800000;
      case 27: return 5200000;
      case 28: return 5600000;
      case 29: return 6000000;
      case 30: return 6400000;
      case 31: return 8050000;
      case 32: return 8700000;
      case 33: return 9450000;
      case 34: return 10300000;
      case 35: return 11250000;
      case 36: return 12250000;   
      case 37: return 13250000;
      case 38: return 14250000;
      case 39: return 15250000;
      case 40: return 16250000;  
      /* add new levels here */
      case LVL_SAINT: return 46250000;
    }
    break;

    case CLASS_THIEF:
    case CLASS_ROGUE:
    case CLASS_BOUNTYHUNTER:
    case CLASS_BARD:
    case CLASS_BLADE:
    case CLASS_JESTER:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 1250;
      case  3: return 2500;
      case  4: return 5000;
      case  5: return 10000;
      case  6: return 20000;
      case  7: return 30000;
      case  8: return 70000;
      case  9: return 110000;
      case 10: return 160000;
      case 11: return 220000;
      case 12: return 440000;
      case 13: return 660000;
      case 14: return 880000;
      case 15: return 1100000;
      case 16: return 1500000;
      case 17: return 2000000;
      case 18: return 2500000;
      case 19: return 3000000;
      case 20: return 3500000;
      case 21: return 3650000;
      case 22: return 3800000;
      case 23: return 4100000;
      case 24: return 4400000;
      case 25: return 4700000;
      case 26: return 5100000;
      case 27: return 5500000;
      case 28: return 5900000;
      case 29: return 6300000;
      case 30: return 6650000;
      case 31: return 8050000;
      case 32: return 8700000;
      case 33: return 9450000;
      case 34: return 10300000;
      case 35: return 11250000;
      case 36: return 12250000;   
      case 37: return 13250000;
      case 38: return 14250000;
      case 39: return 15250000;
      case 40: return 16250000;  
      /* add new levels here */
      case LVL_SAINT: return 46250000;
    }
    break;

    case CLASS_WARRIOR:
    case CLASS_FIGHTER:
    case CLASS_MONK:
    case CLASS_BARBARIAN:
    case CLASS_KNIGHT:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8050000;
      case 32: return 8700000;
      case 33: return 9450000;
      case 34: return 10300000;
      case 35: return 11250000;
      case 36: return 12250000;   
      case 37: return 13250000;
      case 38: return 14250000;
      case 39: return 15250000;
      case 40: return 16250000;  
      /* add new levels here */
      case LVL_SAINT: return 46250000;
    }
    break;
    
    case CLASS_SKNIGHT:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8050000;
      case 32: return 8700000;
      case 33: return 9450000;
      case 34: return 10300000;
      case 35: return 11250000;
      case 36: return 12250000;
      case 37: return 13250000;
      case 38: return 14250000;
      case 39: return 15250000;
      case 40: return 16250000;
      /* add new levels here */
       case LVL_SAINT: return 46250000;
     }
     break;
     
         case CLASS_PALADIN:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8050000;
      case 32: return 8700000;
      case 33: return 9450000;
      case 34: return 10300000;
      case 35: return 11250000;
      case 36: return 12250000;
      case 37: return 13250000;
      case 38: return 14250000;
      case 39: return 15250000;
      case 40: return 16250000;
      case LVL_SAINT: return 46250000;
    }
    break;
   
    case CLASS_CHAOSMAGE:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5000;
      case  4: return 10000;
      case  5: return 20000;
      case  6: return 40000;
      case  7: return 60000;
      case  8: return 90000;
      case  9: return 135000;
      case 10: return 250000;
      case 11: return 375000;
      case 12: return 750000;
      case 13: return 1125000;
      case 14: return 1500000;
      case 15: return 1875000;
      case 16: return 2250000;
      case 17: return 2625000;
      case 18: return 3000000;
      case 19: return 3375000;
      case 20: return 3750000;
      case 21: return 4000000;
      case 22: return 4300000;
      case 23: return 4600000;
      case 24: return 4900000;
      case 25: return 5200000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8050000;
      case 32: return 8700000;
      case 33: return 9450000;
      case 34: return 10300000;
      case 35: return 11250000;
      case 36: return 12250000;   
      case 37: return 13250000;
      case 38: return 14250000;
      case 39: return 15250000;
      case 40: return 16250000;  
      /* add new levels here */
      case LVL_SAINT: return 46250000;    
    }
    break;

    case CLASS_ASSASSIN:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 1250;
      case  3: return 2500;
      case  4: return 5000;
      case  5: return 10000;
      case  6: return 20000;
      case  7: return 30000;
      case  8: return 70000;
      case  9: return 110000;
      case 10: return 160000;
      case 11: return 220000;
      case 12: return 440000;
      case 13: return 660000;
      case 14: return 880000;
      case 15: return 1100000;
      case 16: return 1500000;
      case 17: return 2000000;
      case 18: return 2500000;
      case 19: return 3000000;
      case 20: return 3500000;
      case 21: return 3650000;
      case 22: return 3800000;
      case 23: return 4100000;
      case 24: return 4400000;
      case 25: return 4700000;
      case 26: return 5100000;
      case 27: return 5500000;
      case 28: return 5900000;
      case 29: return 6300000;
      case 30: return 6650000;
      case 31: return 8050000;
      case 32: return 8700000;
      case 33: return 9450000;
      case 34: return 10300000;
      case 35: return 11250000;
      case 36: return 12250000;   
      case 37: return 13250000;
      case 38: return 14250000;
      case 39: return 15250000;
      case 40: return 16250000;  
      /* add new levels here */
      case LVL_SAINT: return 46250000;
    }
    break;            
  }

  /*
   * This statement should never be reached if the exp tables in this function
   * are set up properly.  If you see exp of 123456 then the tables above are
   * incomplete -- so, complete them!
   */
  log("SYSERR: XP tables not set up correctly in class.c!");
  return 123456;
}


/* 
 * Default titles of male characters.
 */
const char *title_male(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Man";
  if (level == LVL_IMPL)
    return "the Implementor";

  switch (chclass) {

/*******************************************************************************
********************************************************************************
                 These are the MALE Mage class titles
********************************************************************************
*******************************************************************************/

    case CLASS_MAGIC_USER:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case  2: return "the Spell Student";
      case  3: return "the Scholar of Magic";
      case  4: return "the Delver in Spells";
      case  5: return "the Medium of Magic";
      case  6: return "the Scribe of Magic";
      case  7: return "the Seer";
      case  8: return "the Sage";
      case  9: return "the Illusionist";
      case 10: return "the Abjurer";
      case 11: return "the Invoker";
      case 12: return "the Creator";
      case 13: return "the Conjurer";
      case 14: return "the Magician";
      case 15: return "the Magius";
      case LVL_SAINT: return "the Immortal Warlock";
      case LVL_DEITY: return "the Avatar of Magic";
      case LVL_GOD: return "the God of Magic";
      default: return "the Mage";
    }
    break;
    case CLASS_BATTLEMAGE:
    switch (level) {
      case 10: return "the Firemage";
      case 11: return "the Invoker";
      case 12: return "the Conjurer";
      case 13: return "the Flamedancer";
      case 14: return "the Earth Mage";
      case 15: return "the Apprentice Battlemage";
      case 16: return "the Battlemage";
      case 17: return "the Master Battlemage";
      case 18: return "the Battlemage Lord";
      case 19: return "the Flame";
      case 20: return "Lightning Eye";
      case 21: return "Firewielder";
      case 22: return "Stormbringer";
      case 23: return "Earthshaker";
      case 24: return "the Fellmage";
      case 25: return "the Clueless";
      }
      break;

  case CLASS_CHAOSMAGE:
    switch (level) {
      case  25: return "the Apprentice Chaosmage";
      case  26: return "the Journeyman Chaosmage";
      case  27: return "the Chaosmage";
      case  28: return "the Master Chaosmage";
      case  29: return "the Planar Scourge";
      case  30: return "Flamebringer";
      case  31: return "Plaguebringer";
      case  32: return "the Thousandfold Foe";
      case  33: return "the Black Ram";
      case 34: return "the Planar Deulist";
      case 35: return "the Ethereal Traveler";
      case 36: return "Initiate of the Inner Planes";
      case 37: return "Adept of the Outer Planes";
      case 38: return "the Astral Traveler";
      case 39: return "Initiate of the Outer Planes";
      case 40: return "Adept of the Outer Planes";
      case LVL_SAINT: return "the Patron Saint of Chaos";
      case LVL_BUILDER: return "the Wreaker of Chaos";
      case LVL_LIEGE: return "the Bastard of Crayons";
      case LVL_COUNT: return "the Count of Chaos";
      case LVL_DEITY: return "the Minion of Chaos";
      case LVL_GOD: return "the God of Chaos";
      default: return "the Chaos Mage";
    }
    break;

    case CLASS_SORCEROR:
    switch (level) {
      case  25: return "the Red Sorceror";
      case  26: return "the Black Sorceror";
      case  27: return "the Green Sorceror";
      case  28: return "the Scholar of Bigsby";
      case  29: return "the White Sorceror";
      case  30: return "the Meteor Plucker";
      case  31: return "the Grey Sorceror";
      case  32: return "Thunderbringer";
      case  33: return "the Firestarter";
      case 34: return "the Ethereal Scourge";
      case 35: return "the Destroyer";
      case 36: return "the Planar Traveler";
      case 37: return "the Astral Scourge";
      case 38: return "the Scourge of the Inner Planes";
      case 39: return "the Scourge of the Outer Planes";
      case 40: return "the Fell Sorceror";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Weaver of Sorcery";
      case LVL_LIEGE: return "the Lord of Sorcery";
      case LVL_COUNT: return "the Cunt of Sorcery";
      case LVL_DEITY: return "the Avatar of Sorcery";
      case LVL_GOD: return "the God of Sorcery";
      default: return "the Sorceror";
    }
    break;

 case CLASS_ENCHANTER:
     switch(level) {
      case 10: return "the Craftsman";
      case 11: return "the Runesman";
      case 12: return "the Scribe";
      case 13: return "the Apprentice Binder";
      case 14: return "the Enchanting";
      case 15: return "the Apprentice Enchanter";
      case 16: return "the Enchanter";
      case 17: return "the Master Enchanter";
      case 18: return "the Diviner";
      case 19: return "the Conjurer";
      case 20: return "the Learned";
      case 21: return "the Learned Enchanter";
      case 22: return "the Sage";
      case 23: return "the Master Sage";
      case 24: return "the Ethereal";
      case 25: return "the Clueless";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Enchanter";
      }
      break;

    case CLASS_NECROMANCER:
    switch (level) {
      case  25: return "the Graverobber";
      case  26: return "the Defiler";
      case  27: return "the Desecrator";
      case  28: return "of the Charnal House";
      case  29: return "the Embalmer";
      case  30: return "the Reanimator";
      case  31: return "the Pestilent";
      case  32: return "the Necrophile";
      case  33: return "the Necromancer";
      case 34: return "the Zombie Lord";
      case 35: return "the Skeletal Lord";
      case 36: return "Lichmaker";
      case 37: return "Wraithhand";
      case 38: return "Deadeye";
      case 39: return "the Ghul Lord";
      case 40: return "Lord of the Dead";
      case LVL_SAINT: return "the Gratefully Dead";
      case LVL_BUILDER: return "Lich Lord";
      case LVL_LIEGE: return "the Socketfucker";
      case LVL_COUNT: return "Spectreherder";
      case LVL_DEITY: return "the Demilich";
      case LVL_GOD: return "God of Death";
      default: return "the Necromancer";
    }
    break;

    case CLASS_ALCHEMIST:
    switch (level) {
      case  25: return "the Reeking";
      case  26: return "the Tonic Maker";
      case  27: return "the Brewer";
      case  28: return "the Transmuter";
      case  29: return "the Binder";
      case  30: return "the Foul-Smelling";
      case  31: return "the Sulfurous";
      case  32: return "the Philtrist";
      case  33: return "Brimstone";
      case 34: return "the Distiller";
      case 35: return "the Chemist";
      case 36: return "the Alchemist";
      case 37: return "the Master Alchemist";
      case 38: return "Lord of Alchemy";
      case 39: return "Seeker of the Philosopher's Stone";
      case 40: return "Keeper of the Philosopher's Stone";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Potion Manufacturer";
      case LVL_LIEGE: return "the Stavist";
      case LVL_COUNT: return "the Molder";
      case LVL_DEITY: return "Goldfinger";
      case LVL_GOD: return "God of Matter";
      default: return "the Alchemist";
    }
    break; 

/*******************************************************************************
********************************************************************************
                 These are the MALE priest class titles
********************************************************************************
*******************************************************************************/


    case CLASS_PRIEST:
    switch (level) {
      case  1: return "the Believer";
      case  2: return "the Attendant";
      case  3: return "the Acolyte";
      case  4: return "the Novice";
      case  5: return "the Missionary";
      case  6: return "the Adept";
      case  7: return "the Deacon";
      case  8: return "the Vicar";
      case  9: return "the Priest";
      case 10: return "the Minister";
      case 11: return "the Canon";
      case 12: return "the Levite";
      case 13: return "the Curate";
      case 14: return "the Monk";
      case 15: return "the Clueless";
      case LVL_SAINT: return "the Immortal Cardinal";
      case LVL_DEITY: return "the Inquisitor";
      case LVL_GOD: return "the God of good and evil";
      default: return "the Priest";
    }
    break;

     case CLASS_CLERIC:
     switch(level) {
      case 10: return "the Devout";
      case 11: return "the Student of Scripture";
      case 12: return "the Scholar of Scripture";
      case 13: return "the Master of Scripture";
      case 14: return "the Doctor of Scripture";
      case 15: return "the Healer";
      case 16: return "the Chaplain";
      case 17: return "the Expositor";
      case 18: return "the Bishop";
      case 19: return "the Arch Bishop";
      case 20: return "the Patriarch";
      case 21: return "the Cleric Scholar";
      case 22: return "the Cleric Lord";
      case 23: return "the High Cleric";
      case 24: return "the High Cleric Lord";
      case 25: return "the Clueless";
      case LVL_DEITY: return "the Inquisitor";
      case LVL_GOD: return "the God of good and evil";
      default: return "the Cleric";
      }
      break;
  case CLASS_DISCIPLE:
    switch (level) {
      case  25: return "the Apprentice of the Cloth";
      case  26: return "the Initiate of the Cloth";
      case  27: return "the Adept of the Cloth";
      case  28: return "the Journeyman of the Cloth";
      case  29: return "the Disciple of the Cloth";
      case  30: return "the Metropolitan of the Cloth";
      case  31: return "the Bishop of the Cloth";
      case  32: return "the Patriarch of the Cloth";
      case  33: return "Lord Disciple";
      case 34: return "the Holy";
      case 35: return "the Prophet";
      case 36: return "the Holy Prophet";
      case 37: return "the Holy Lord Prophet";
      case 38: return "the Eminence";
      case 39: return "the Eminent Prophet";
      case 40: return "the Eminent Holy Prophet";
      case LVL_SAINT: return "the Immortal Cardinal";
      case LVL_BUILDER: return "the Cardinal Archivist";
      case LVL_LIEGE: return "the Lord my God";
      case LVL_COUNT: return "Christ was a Pederast";
      case LVL_DEITY: return "the Lamb of God";
      case LVL_GOD: return "the Lord Your God";
      default: return "the Disciple";
    }
    break;

    case CLASS_CRUSADER:
    switch (level) {
      case  25: return "the Evangelist";
      case  26: return "the Crusader";
      case  27: return "the Fanatic";
      case  28: return "the Chosen";
      case  29: return "Heathenbane";
      case  30: return "Slayer of the Infidel";
      case  31: return "Scourge of the Unbeliever";
      case  32: return "Scourge of Damascus";
      case  33: return "Conqueror of the Holy Land";
      case 34: return "the Inquisitor";
      case 35: return "the Grand Inquisitor";
      case 36: return "of Antioch";
      case 37: return "the Talib";
      case 38: return "the Fedayee";
      case 39: return "the Iman";
      case 40: return "the Ayotollah";
      case LVL_SAINT: return "the Martyr";
      case LVL_BUILDER: return "the Mosquebuilder";
      case LVL_LIEGE: return "Cousin of the Prophet";
      case LVL_COUNT: return "the Avenger";
      case LVL_DEITY: return "the Avenging Angel";
      case LVL_GOD: return "the God of Martyrs";
      default: return "the Crusader";
    }
    break;

 case CLASS_SHAMAN:
     switch(level) {
      case 10: return "the Herbalist";
      case 11: return "the Herbsman";
      case 12: return "the Mushroom Gatherer";
      case 13: return "the Rootsman";
      case 14: return "the Treetalker";
      case 15: return "of the Green Spirit";
      case 16: return "of the Blue Spirit";
      case 17: return "the Young Shaman";
      case 18: return "the Shaman";
      case 19: return "the Master Shaman";
      case 20: return "Leaftongue";
      case 21: return "the Poulticer";
      case 22: return "Bearskull";
      case 23: return "the Brujo";
      case 24: return "the Witchdoctor";
      case 25: return "Vinetalker";
      case LVL_DEITY: return "the Inquisitor";
      case LVL_GOD: return "the God of good and evil";
      default: return "the Shaman";
      }
      break;

    case CLASS_RANGER:
    switch (level) {
      case  25: return "Swiftleg";
      case  26: return "Greycloak";
      case  27: return "the Woodsman";
      case  28: return "the Journeyman Woodsman";
      case  29: return "the Master Woodsman";
      case  30: return "the Sojourner";
      case  31: return "of the Deep Wood";
      case  32: return "the Strider";
      case  33: return "the Ranger";
      case 34: return "the Master Ranger";
      case 35: return "Lord of the Range";
      case 36: return "Master of the Wild";
      case 37: return "the Dunedan";
      case 38: return "of the Old Wood";
      case 39: return "Lord of the Wood";
      case 40: return "Ashface";
      case LVL_SAINT: return "the Immortal Ranger";
      case LVL_BUILDER: return "the Gardener";
      case LVL_LIEGE: return "the Keeper of the Wood";
      case LVL_COUNT: return "Appleseed";
      case LVL_DEITY: return "the Divine Woodsman";
      case LVL_GOD: return "the God of the Wood";
      default: return "the Ranger";
    }
    break;

    case CLASS_DRUID:
    switch (level) {
      case  25: return "the Treehugger";
      case  26: return "the Treesitter";
      case  27: return "the Initiate of the 1st Circle";
      case  28: return "the Adept of the 1st Circle ";
      case  29: return "the Initiate of the 2nd Circle";
      case  30: return "the Adept of the 2nd Circle";
      case  31: return "the Initiate of the 3rd Circle";
      case  32: return "the Adept of the 3rd Circle";
      case  33: return "the Initiate of the 4th Circle";
      case 34: return "the Adept of the 4th Circle";
      case 35: return "the Initiate of the 5th Circle";
      case 36: return "the Adept of the 5th Circle";
      case 37: return "the Grand Druid";
      case 38: return "the Archdruid";
      case 39: return "the Soul of the Wood";
      case 40: return "Entwalker";
      case LVL_SAINT: return "the Arborsexual";
      case LVL_BUILDER: return "the Steward of the Wood";
      case LVL_LIEGE: return "is wearing a tall green hat";
      case LVL_COUNT: return "the Woodlord";
      case LVL_DEITY: return "the Inquisitor";
      case LVL_GOD: return "the God of good and evil";
      default: return "the Druid";
    }
    break;

/*******************************************************************************
********************************************************************************
                 These are the MALE Rogue class titles
********************************************************************************
*******************************************************************************/

    case CLASS_ROGUE:
    switch (level) {
      case  1: return "the Shifty";
      case  2: return "the Grifter";
      case  3: return "the Con";
      case  4: return "the Drifter";
      case  5: return "the Indigent";
      case  6: return "the Outcast";
      case  7: return "the Shadow";
      case  8: return "the Lurker";
      case  9: return "the Dealer";
      case 10: return "the Alleyman";
      case 11: return "the Robber";
      case 12: return "the Magsman";
      case 13: return "the Highwayman";
      case 14: return "the Burglar";
      case 15: return "the Clueless";
      case LVL_SAINT: return "the Immortal Rogue";
      case LVL_DEITY: return "the Demi God of thieves";
      case LVL_GOD: return "the God of thieves and tradesmen";
      default: return "the Rogue";
    }
    break;

    case CLASS_THIEF:
    switch (level) {
      case 10: return "the Pilferer";
      case 11: return "the Footpad";
      case 12: return "the Filcher";
      case 13: return "the Pick-Pocket";
      case 14: return "the Sneak";
      case 15: return "the Pincher";
      case 16: return "the Cut-Purse";
      case 17: return "the Snatcher";
      case 18: return "the Sharper";
      case 19: return "the Rogue";
      case 20: return "the Robber";
      case 21: return "the Magsman";
      case 22: return "the Highwayman";
      case 23: return "the Burglar";
      case 24: return "the Thief";
      case 25: return "the Master Thief";
      case LVL_SAINT: return "fixme";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Thief";
      }
      break;

  case CLASS_ASSASSIN:
    switch (level) {
      case  25: return "the Silent";
      case  26: return "the Skulker";
      case  27: return "the Killer";
      case  28: return "the Poisoner";
      case  29: return "the Knife";
      case  30: return "Sureknife";
      case  31: return "Backbiter";
      case  32: return "the Hitman";
      case  33: return "the Button Man";
      case 34: return "the Assassin";
      case 35: return "the Master Assassin";
      case 36: return "Lordsbane";
      case 37: return "Countsbane";
      case 38: return "Dukesbane";
      case 39: return "the Attentater";
      case 40: return "Kingsbane";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Poison Brewer";
      case LVL_LIEGE: return "the Don";
      case LVL_COUNT: return "Consiglieri";
      case LVL_DEITY: return "the Divine Assassin";
      case LVL_GOD: return "Christpuncher";
      default: return "the Assassin";
    }
    break;

    case CLASS_BOUNTYHUNTER:
    switch (level) {
      case  25: return "the Deputy";
      case  26: return "the Sheriff";
      case  27: return "the Hunter";
      case  28: return "the Pursuer";
      case  29: return "the Hunter";
      case  30: return "the Bounty Hunter";
      case  31: return "Longarm";
      case  32: return "the Relentless";
      case  33: return "the Farsighted";
      case 34: return "the Avenger";
      case 35: return "the Nemesis";
      case 36: return "the Master Bounty Hunter";
      case 37: return "Lordhunter";
      case 38: return "the Avenging Blade";
      case 39: return "the Shadow";
      case 40: return "the Inescapable";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Bondsman";
      case LVL_LIEGE: return "the Master Bondsman";
      case LVL_COUNT: return "Overseer of Fugitives";
      case LVL_DEITY: return "the Divine Hunter";
      case LVL_GOD: return "the God of Vengeance";
      default: return "the Bounty Hunter";
    }
    break;

 case CLASS_BARD:
     switch(level) {
      case 10: return "the Rhymer";
      case 11: return "the Sonnateer";
      case 12: return "the Skald";
      case 13: return "the Racaraide";
      case 14: return "the Jongleur";
      case 15: return "the Troubador";
      case 16: return "the Minstrel";
      case 17: return "the Lorist";
      case 18: return "the Apprentice Bard";
      case 19: return "the Journeyman Bard";
      case 20: return "the Bard";
      case 21: return "the Master Bard";
      case 22: return "the Lord Bard";
      case 23: return "Bardhard";
      case 24: return "the Fellbard";
      case 25: return "the Pure-Pitched";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Bard";
      }
      break;

    case CLASS_JESTER:
    switch (level) {
      case  25: return "the Mime";
      case  26: return "the Juggler";
      case  27: return "the Poet";
      case  28: return "the Knife Juggler";
      case  29: return "the Balladist";
      case  30: return "the Herald";
      case  31: return "the Canter";
      case  32: return "the Satirist";
      case  33: return "Acidtongue";
      case 34: return "the Knife Thrower";
      case 35: return "the Dread Mime";
      case 36: return "the Danse Macabre";
      case 37: return "the Dark Poet";
      case 38: return "the Mad Lyricist";
      case 39: return "the Fire Spitter";
      case 40: return "the Ill";
      case LVL_SAINT: return "the Sad Clown";
      case LVL_BUILDER: return "Author of the Dope Rhymez";
      case LVL_LIEGE: return "Editor of the Dope Rhymez";
      case LVL_COUNT: return "Executive Producer of the Dopy Rhymez";
      case LVL_DEITY: return "the Divine Mime";
      case LVL_GOD: return "the God of Mimes";
      default: return "the Jester";
    }
    break;

    case CLASS_BLADE:
    switch (level) {
      case  25: return "the Apprentice Blade";
      case  26: return "the Journeyman Blade";
      case  27: return "the Blade";
      case  28: return "the Master Blade";
      case  29: return "Quickblade";
      case  30: return "Slingblade";
      case  31: return "Twinblade";
      case  32: return "Redblade";
      case  33: return "Blackblade";
      case 34: return "the Blademaster";
      case 35: return "Shadowblade";
      case 36: return "Dervish Blade";
      case 37: return "the Steel of Damascus";
      case 38: return "Fellblade";
      case 39: return "Tenblade";
      case 40: return "Dreadblade";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Blade Forger";
      case LVL_LIEGE: return "the Lord of Bladesmiths";
      case LVL_COUNT: return "the Commissioner of Blades and Standards";
      case LVL_DEITY: return "Tripleblade with Lubricated Strip";
      case LVL_GOD: return "the God of Steel";
      default: return "the Blade";
    }
    break; 



/*******************************************************************************
********************************************************************************
                 These are the MALE Warrior class titles
********************************************************************************
*******************************************************************************/

    case CLASS_FIGHTER:
    switch(level) {
      case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentry";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordsman";
      case  9: return "the Fencer";
      case 10: return "the Combatant";
      case 11: return "the Hero";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckler";
      case 14: return "the Mercenary";
      case 15: return "The Clueless";
      case LVL_SAINT: return "the Immortal Warlord";
      case LVL_DEITY: return "the Extirpator";
      case LVL_GOD: return "the God of war";
      default: return "the Fighter";
    }
    break;

 case CLASS_WARRIOR:
     switch(level) {
      case 10: return "the Hero";
      case 11: return "the Myrmidon";
      case 12: return "the Swashbuckler";
      case 13: return "the Mercenary";
      case 14: return "the Apprentice Warrior";
      case 15: return "the Journeyman Warrior";
      case 16: return "the Warrior";
      case 17: return "the Master Warrior";
      case 18: return "the Lone Warrior";
      case 19: return "the Dog of War";
      case 20: return "the Swordmaster";
      case 21: return "the Pikeman";
      case 22: return "the Lord of War";
      case 23: return "the Warbringer";
      case 24: return "the Lieutenent";
      case 25: return "the Colonel";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Warrior";
      }
      break;

  case CLASS_MONK:
    switch (level) {
      case  25: return "the Grasshopper";
      case  26: return "the Celibate";
      case  27: return "The Austere";
      case  28: return "Deathgrip";
      case  29: return "of the Deadly Touch";
      case  30: return "Redfist";
      case  31: return "Monkeyfist";
      case  32: return "Cranefoot";
      case  33: return "Silentfist";
      case 34: return "Dragonfist";
      case 35: return "Tenfist";
      case 36: return "the Jujitsu Master";
      case 37: return "the Kung Fu Master";
      case 38: return "the Drunken Master";
      case 39: return "the Wu Tang Master";
      case 40: return "the Shaolin Master";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Shriner";
      case LVL_LIEGE: return "the Ancient";
      case LVL_COUNT: return "the Monastic";
      case LVL_DEITY: return "the Divine Shaolin Warrior";
      case LVL_GOD: return "God of the Drunken Masters";
      default: return "the Monk";
    }
    break;

    case CLASS_BARBARIAN:
    switch (level) {
      case  25: return "Youngblood";
      case  26: return "Quickaxe";
      case  27: return "the Red";
      case  28: return "the Berserker";
      case  29: return "the Raider";
      case  30: return "Longstride";
      case  31: return "Redaxe";
      case  32: return "the Black";
      case  33: return "Longbeard";
      case 34: return "Redbeard";
      case 35: return "the Pillager";
      case 36: return "the Looter";
      case 37: return "Scourge of the South";
      case 38: return "Scourge of the East";
      case 39: return "Scourge of the West";
      case 40: return "Scourge of the North";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Fjordmaker";
      case LVL_LIEGE: return "of the Long House";
      case LVL_COUNT: return "Lord of the Icelands";
      case LVL_DEITY: return "the Divine Scourge";
      case LVL_GOD: return "God of Pillage";
      default: return "the Barbarian";
    }
    break;

 case CLASS_KNIGHT:
     switch(level) {
      case 10: return "the Apprentice Knight";
      case 11: return "the Journeyman Knight";
      case 12: return "the Knight Errant";
      case 13: return "the White Knight";
      case 14: return "the Green Knight";
      case 15: return "the Red Knight";
      case 16: return "the Purple Knight";
      case 17: return "the Vassal";
      case 18: return "the Master Knight";
      case 19: return "the Lord Knight";
      case 20: return "the High Knight";
      case 21: return "Knight of the Long Table";
      case 22: return "Knight of the Crown";
      case 23: return "Knight of the Land";
      case 24: return "Knight of the Round Table";
      case 25: return "the Dread Knight";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Knight";
      }
      break;

    case CLASS_PALADIN:
    switch (level) {
      case  25: return "the Chevalier";
      case  26: return "the Pure";
      case  27: return "the Chaste";
      case  28: return "the Well-Groomed";
      case  29: return "the Polite";
      case  30: return "the Patient";
      case  31: return "the Civil";
      case  32: return "the Lawful";
      case  33: return "the Church-Going";
      case 34: return "the Pious";
      case 35: return "the Soft-Spoken";
      case 36: return "the Decorous";
      case 37: return "the Proper";
      case 38: return "the Tearful";
      case 39: return "the Constant";
      case 40: return "Manus Dei";
      case LVL_SAINT: return "the Swaggart-Haired";
      case LVL_BUILDER: return "the Cathedralist";
      case LVL_LIEGE: return "Steward of the See";
      case LVL_COUNT: return "the Cowardly";
      case LVL_DEITY: return "the Avenging Hand of God";
      case LVL_GOD: return "the Whitebred Eurocentric God";
      default: return "the Paladin";
    }
    break;

    case CLASS_SKNIGHT:
    switch (level) {
      case  25: return "Lucifer's Errandboy";
      case  26: return "Manservant Hecubus";
      case  27: return "the Devilish";
      case  28: return "the Sinister";
      case  29: return "the Blackadder";
      case  30: return "the Hellraiser";
      case  31: return "the Blackguard";
      case  32: return "the Infidel";
      case  33: return "the Villein";
      case 34: return "the Villein of the Deepest Dye";
      case 35: return "the Black Knight";
      case 36: return "the Shadow Knight";
      case 37: return "Vassal of the Nine Hells";
      case 38: return "Messenger of Beelzebub";
      case 39: return "Drinking Buddy of Asmodeus";
      case 40: return "Executor of Shaitan";
      case LVL_SAINT: return "the Dark";
      case LVL_BUILDER: return "Architect of the Hells";
      case LVL_LIEGE: return "Assistant Manager of Hell";
      case LVL_COUNT: return "Fry Chef of Hell";
      case LVL_DEITY: return "the Lucifer";
      case LVL_GOD: return "the Chirstpuncher";
      default: return "the Shadow Knight";
    }
    break;
    



  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}


/* 
 * Default titles of female characters.
 */
const char *title_female(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Woman";
  if (level == LVL_IMPL)
    return "the Implementress";

switch (chclass) {
/*******************************************************************************
********************************************************************************
                 These are the FEMALE Mage class titles
********************************************************************************
*******************************************************************************/

    case CLASS_MAGIC_USER:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case  2: return "the Spell Student";
      case  3: return "the Scholar of Magic";
      case  4: return "the Delver in Spells";
      case  5: return "the Medium of Magic";
      case  6: return "the Scribe of Magic";
      case  7: return "the Seer";
      case  8: return "the Sage";
      case  9: return "the Illusionist";
      case 10: return "the Abjurer";
      case 11: return "the Invoker";
      case 12: return "the Creator";
      case 13: return "the Conjurer";
      case 14: return "the Magician";
      case 15: return "the Clueless";
      case LVL_SAINT: return "the Immortal Warlock";
      case LVL_DEITY: return "the Avatar of Magic";
      case LVL_GOD: return "the God of Magic";
      default: return "the Mage";
    }
    break;
    case CLASS_BATTLEMAGE:
    switch (level) {
      case 10: return "the Firemage";
      case 11: return "the Invoker";
      case 12: return "the Conjurer";
      case 13: return "the Flamedancer";
      case 14: return "the Earth Mage";
      case 15: return "the Apprentice Battlemage";
      case 16: return "the Battlemage";
      case 17: return "the Master Battlemage";
      case 18: return "the Battlemage Lord";
      case 19: return "the Flame";
      case 20: return "Lightning Eye";
      case 21: return "Firewielder";
      case 22: return "Stormbringer";
      case 23: return "Earthshaker";
      case 24: return "the Fellmage";
      case 25: return "the Clueless";
      }
      break;

  case CLASS_CHAOSMAGE:
    switch (level) {
      case  25: return "the Apprentice Chaosmage";
      case  26: return "the Journeyman Chaosmage";
      case  27: return "the Chaosmage";
      case  28: return "the Master Chaosmage";
      case  29: return "the Planar Scourge";
      case  30: return "Flamebringer";
      case  31: return "Plaguebringer";
      case  32: return "the Thousandfold Foe";
      case  33: return "the Black Ram";
      case 34: return "the Planar Deulist";
      case 35: return "the Ethereal Traveler";
      case 36: return "Initiate of the Inner Planes";
      case 37: return "Adept of the Outer Planes";
      case 38: return "the Astral Traveler";
      case 39: return "Initiate of the Outer Planes";
      case 40: return "Adept of the Outer Planes";
      case LVL_SAINT: return "the Patron Saint of Chaos";
      case LVL_BUILDER: return "the Wreaker of Chaos";
      case LVL_LIEGE: return "the Bastard of Crayons";
      case LVL_COUNT: return "the Count of Chaos";
      case LVL_DEITY: return "the Minion of Chaos";
      case LVL_GOD: return "the God of Chaos";
      default: return "the Chaos Mage";
    }
    break;

    case CLASS_SORCEROR:
    switch (level) {
      case  25: return "the Red Sorceror";
      case  26: return "the Black Sorceror";
      case  27: return "the Green Sorceror";
      case  28: return "the Scholar of Bigsby";
      case  29: return "the White Sorceror";
      case  30: return "the Meteor Plucker";
      case  31: return "the Grey Sorceror";
      case  32: return "Thunderbringer";
      case  33: return "the Firestarter";
      case 34: return "the Ethereal Scourge";
      case 35: return "the Destroyer";
      case 36: return "the Planar Traveler";
      case 37: return "the Astral Scourge";
      case 38: return "the Scourge of the Inner Planes";
      case 39: return "the Scourge of the Outer Planes";
      case 40: return "the Fell Sorceror";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Weaver of Sorcery";
      case LVL_LIEGE: return "the Lord of Sorcery";
      case LVL_COUNT: return "the Cunt of Sorcery";
      case LVL_DEITY: return "the Avatar of Sorcery";
      case LVL_GOD: return "the God of Sorcery";
      default: return "the Sorceror";
    }
    break;

 case CLASS_ENCHANTER:
     switch(level) {
      case 10: return "the Craftsman";
      case 11: return "the Runesman";
      case 12: return "the Scribe";
      case 13: return "the Apprentice Binder";
      case 14: return "the Enchanting";
      case 15: return "the Apprentice Enchanter";
      case 16: return "the Enchanter";
      case 17: return "the Master Enchanter";
      case 18: return "the Diviner";
      case 19: return "the Conjurer";
      case 20: return "the Learned";
      case 21: return "the Learned Enchanter";
      case 22: return "the Sage";
      case 23: return "the Master Sage";
      case 24: return "the Ethereal";
      case 25: return "the Clueless";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Enchanter";
      }
      break;

    case CLASS_NECROMANCER:
    switch (level) {
      case  25: return "the Graverobber";
      case  26: return "the Defiler";
      case  27: return "the Desecrator";
      case  28: return "of the Charnal House";
      case  29: return "the Embalmer";
      case  30: return "the Reanimator";
      case  31: return "the Pestilent";
      case  32: return "the Necrophile";
      case  33: return "the Necromancer";
      case 34: return "the Zombie Lord";
      case 35: return "the Skeletal Lord";
      case 36: return "Lichmaker";
      case 37: return "Wraithhand";
      case 38: return "Deadeye";
      case 39: return "the Ghul Lord";
      case 40: return "Lord of the Dead";
      case LVL_SAINT: return "the Gratefully Dead";
      case LVL_BUILDER: return "Lich Lord";
      case LVL_LIEGE: return "the Socketfucker";
      case LVL_COUNT: return "Spectreherder";
      case LVL_DEITY: return "the Demilich";
      case LVL_GOD: return "God of Death";
      default: return "the Necromancer";
    }
    break;

    case CLASS_ALCHEMIST:
    switch (level) {
      case  25: return "the Reeking";
      case  26: return "the Tonic Maker";
      case  27: return "the Brewer";
      case  28: return "the Transmuter";
      case  29: return "the Binder";
      case  30: return "the Foul-Smelling";
      case  31: return "the Sulfurous";
      case  32: return "the Philtrist";
      case  33: return "Brimstone";
      case 34: return "the Distiller";
      case 35: return "the Chemist";
      case 36: return "the Alchemist";
      case 37: return "the Master Alchemist";
      case 38: return "Lord of Alchemy";
      case 39: return "Seeker of the Philosopher's Stone";
      case 40: return "Keeper of the Philosopher's Stone";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Potion Manufacturer";
      case LVL_LIEGE: return "the Stavist";
      case LVL_COUNT: return "the Molder";
      case LVL_DEITY: return "Goldfinger";
      case LVL_GOD: return "God of Matter";
      default: return "the Alchemist";
    }
    break;

/*******************************************************************************
********************************************************************************
                 These are the FEMALE priest class titles
********************************************************************************
*******************************************************************************/


    case CLASS_PRIEST:
    switch (level) {
      case  1: return "the Believer";
      case  2: return "the Attendant";
      case  3: return "the Acolyte";
      case  4: return "the Novice";
      case  5: return "the Missionary";
      case  6: return "the Adept";
      case  7: return "the Deacon";
      case  8: return "the Vicar";
      case  9: return "the Priest";
      case 10: return "the Minister";
      case 11: return "the Canon";
      case 12: return "the Levite";
      case 13: return "the Curate";
      case 14: return "the Monk";
      case 15: return "the Clueless";
      case LVL_SAINT: return "the Immortal Cardinal";
      case LVL_DEITY: return "the Inquisitor";
      case LVL_GOD: return "the God of good and evil";
      default: return "the Priest";
    }
    break;

     case CLASS_CLERIC:
     switch(level) {
      case 10: return "the Devout";
      case 11: return "the Student of Scripture";
      case 12: return "the Scholar of Scripture";
      case 13: return "the Master of Scripture";
      case 14: return "the Doctor of Scripture";
      case 15: return "the Healer";
      case 16: return "the Chaplain";
      case 17: return "the Expositor";
      case 18: return "the Bishop";
      case 19: return "the Arch Bishop";
      case 20: return "the Patriarch";
      case 21: return "the Cleric Scholar";
      case 22: return "the Cleric Lord";
      case 23: return "the High Cleric";
      case 24: return "the High Cleric Lord";
      case 25: return "the Clueless";
      case LVL_DEITY: return "the Inquisitor";
      case LVL_GOD: return "the God of good and evil";
      default: return "the Cleric";
      }
      break;
  case CLASS_DISCIPLE:
    switch (level) {
      case  25: return "the Apprentice of the Cloth";
      case  26: return "the Initiate of the Cloth";
      case  27: return "the Adept of the Cloth";
      case  28: return "the Journeyman of the Cloth";
      case  29: return "the Disciple of the Cloth";
      case  30: return "the Metropolitan of the Cloth";
      case  31: return "the Bishop of the Cloth";
      case  32: return "the Patriarch of the Cloth";
      case  33: return "Lord Disciple";
      case 34: return "the Holy";
      case 35: return "the Prophet";
      case 36: return "the Holy Prophet";
      case 37: return "the Holy Lord Prophet";
      case 38: return "the Eminence";
      case 39: return "the Eminent Prophet";
      case 40: return "the Eminent Holy Prophet";
      case LVL_SAINT: return "the Immortal Cardinal";
      case LVL_BUILDER: return "the Cardinal Archivist";
      case LVL_LIEGE: return "the Lord my God";
      case LVL_COUNT: return "Christ was a Pederast";
      case LVL_DEITY: return "the Lamb of God";
      case LVL_GOD: return "the Lord Your God";
      default: return "the Disciple";
    }
    break;

    case CLASS_CRUSADER:
    switch (level) {
      case  25: return "the Evangelist";
      case  26: return "the Crusader";
      case  27: return "the Fanatic";
      case  28: return "the Chosen";
      case  29: return "Heathenbane";
      case  30: return "Slayer of the Infidel";
      case  31: return "Scourge of the Unbeliever";
      case  32: return "Scourge of Damascus";
      case  33: return "Conqueror of the Holy Land";
      case 34: return "the Inquisitor";
      case 35: return "the Grand Inquisitor";
      case 36: return "of Antioch";
      case 37: return "the Talib";
      case 38: return "the Fedayee";
      case 39: return "the Iman";
      case 40: return "the Ayotollah";
      case LVL_SAINT: return "the Martyr";
      case LVL_BUILDER: return "the Mosquebuilder";
      case LVL_LIEGE: return "Cousin of the Prophet";
      case LVL_COUNT: return "the Avenger";
      case LVL_DEITY: return "the Avenging Angel";
      case LVL_GOD: return "the God of Martyrs";
      default: return "the Crusader";
    }
    break;

 case CLASS_SHAMAN:
     switch(level) {
      case 10: return "the Herbalist";
      case 11: return "the Herbsman";
      case 12: return "the Mushroom Gatherer";
      case 13: return "the Rootsman";
      case 14: return "the Treetalker";
      case 15: return "of the Green Spirit";
      case 16: return "of the Blue Spirit";
      case 17: return "the Young Shaman";
      case 18: return "the Shaman";
      case 19: return "the Master Shaman";
      case 20: return "Leaftongue";
      case 21: return "the Poulticer";
      case 22: return "Bearskull";
      case 23: return "the Brujo";
      case 24: return "the Witchdoctor";
      case 25: return "Vinetalker";
      case LVL_DEITY: return "the Inquisitor";
      case LVL_GOD: return "the God of good and evil";
      default: return "the Shaman";
      }
      break;

    case CLASS_RANGER:
    switch (level) {
      case  25: return "Swiftleg";
      case  26: return "Greycloak";
      case  27: return "the Woodsman";
      case  28: return "the Journeyman Woodsman";
      case  29: return "the Master Woodsman";
      case  30: return "the Sojourner";
      case  31: return "of the Deep Wood";
      case  32: return "the Strider";
      case  33: return "the Ranger";
      case 34: return "the Master Ranger";
      case 35: return "Lord of the Range";
      case 36: return "Master of the Wild";
      case 37: return "the Dunedan";
      case 38: return "of the Old Wood";
      case 39: return "Lord of the Wood";
      case 40: return "Ashface";
      case LVL_SAINT: return "the Immortal Ranger";
      case LVL_BUILDER: return "the Gardener";
      case LVL_LIEGE: return "the Keeper of the Wood";
      case LVL_COUNT: return "Appleseed";
      case LVL_DEITY: return "the Divine Woodsman";
      case LVL_GOD: return "the God of the Wood";
      default: return "the Ranger";
    }
    break;

    case CLASS_DRUID:
    switch (level) {
      case  25: return "the Treehugger";
      case  26: return "the Treesitter";
      case  27: return "the Initiate of the 1st Circle";
      case  28: return "the Adept of the 1st Circle ";
      case  29: return "the Initiate of the 2nd Circle";
      case  30: return "the Adept of the 2nd Circle";
      case  31: return "the Initiate of the 3rd Circle";
      case  32: return "the Adept of the 3rd Circle";
      case  33: return "the Initiate of the 4th Circle";
      case 34: return "the Adept of the 4th Circle";
      case 35: return "the Initiate of the 5th Circle";
      case 36: return "the Adept of the 5th Circle";
      case 37: return "the Grand Druid";
      case 38: return "the Archdruid";
      case 39: return "the Soul of the Wood";
      case 40: return "Entwalker";
      case LVL_SAINT: return "the Arborsexual";
      case LVL_BUILDER: return "the Steward of the Wood";
      case LVL_LIEGE: return "is wearing a tall green hat";
      case LVL_COUNT: return "the Woodlord";
      case LVL_DEITY: return "the Inquisitor";
      case LVL_GOD: return "the God of good and evil";
      default: return "the Druid";
    }
    break;

/*******************************************************************************
********************************************************************************
                 These are the FEMALE Rogue class titles
********************************************************************************
*******************************************************************************/

    case CLASS_ROGUE:
    switch (level) {
      case  1: return "the Shifty";
      case  2: return "the Grifter";
      case  3: return "the Con";
      case  4: return "the Drifter";
      case  5: return "the Indigent";
      case  6: return "the Outcast";
      case  7: return "the Shadow";
      case  8: return "the Lurker";
      case  9: return "the Dealer";
      case 10: return "the Alleyman";
      case 11: return "the Robber";
      case 12: return "the Magsman";
      case 13: return "the Highwayman";
      case 14: return "the Burglar";
      case 15: return "the Clueless";
      case LVL_SAINT: return "the Immortal Rogue";
      case LVL_DEITY: return "the Demi God of thieves";
      case LVL_GOD: return "the God of thieves and tradesmen";
      default: return "the Rogue";
    }
    break;

    case CLASS_THIEF:
    switch (level) {
      case 10: return "the Pilferer";
      case 11: return "the Footpad";
      case 12: return "the Filcher";
      case 13: return "the Pick-Pocket";
      case 14: return "the Sneak";
      case 15: return "the Pincher";
      case 16: return "the Cut-Purse";
      case 17: return "the Snatcher";
      case 18: return "the Sharper";
      case 19: return "the Rogue";
      case 20: return "the Robber";
      case 21: return "the Magsman";
      case 22: return "the Highwayman";
      case 23: return "the Burglar";
      case 24: return "the Thief";
      case 25: return "the Master Thief";
      case LVL_SAINT: return "fixme";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Thief";
      }
      break;

  case CLASS_ASSASSIN:
    switch (level) {
      case  25: return "the Silent";
      case  26: return "the Skulker";
      case  27: return "the Killer";
      case  28: return "the Poisoner";
      case  29: return "the Knife";
      case  30: return "Sureknife";
      case  31: return "Backbiter";
      case  32: return "the Hitman";
      case  33: return "the Button Man";
      case 34: return "the Assassin";
      case 35: return "the Master Assassin";
      case 36: return "Lordsbane";
      case 37: return "Countsbane";
      case 38: return "Dukesbane";
      case 39: return "the Attentater";
      case 40: return "Kingsbane";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Poison Brewer";
      case LVL_LIEGE: return "the Don";
      case LVL_COUNT: return "Consiglieri";
      case LVL_DEITY: return "the Divine Assassin";
      case LVL_GOD: return "Christpuncher";
      default: return "the Assassin";
    }
    break;

    case CLASS_BOUNTYHUNTER:
    switch (level) {
      case  25: return "the Deputy";
      case  26: return "the Sheriff";
      case  27: return "the Hunter";
      case  28: return "the Pursuer";
      case  29: return "the Hunter";
      case  30: return "the Bounty Hunter";
      case  31: return "Longarm";
      case  32: return "the Relentless";
      case  33: return "the Farsighted";
      case 34: return "the Avenger";
      case 35: return "the Nemesis";
      case 36: return "the Master Bounty Hunter";
      case 37: return "Lordhunter";
      case 38: return "the Avenging Blade";
      case 39: return "the Shadow";
      case 40: return "the Inescapable";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Bondsman";
      case LVL_LIEGE: return "the Master Bondsman";
      case LVL_COUNT: return "Overseer of Fugitives";
      case LVL_DEITY: return "the Divine Hunter";
      case LVL_GOD: return "the God of Vengeance";
      default: return "the Bounty Hunter";
    }
    break;

 case CLASS_BARD:
     switch(level) {
      case 10: return "the Rhymer";
      case 11: return "the Sonnateer";
      case 12: return "the Skald";
      case 13: return "the Racaraide";
      case 14: return "the Jongleur";
      case 15: return "the Troubador";
      case 16: return "the Minstrel";
      case 17: return "the Lorist";
      case 18: return "the Apprentice Bard";
      case 19: return "the Journeyman Bard";
      case 20: return "the Bard";
      case 21: return "the Master Bard";
      case 22: return "the Lord Bard";
      case 23: return "Bardhard";
      case 24: return "the Fellbard";
      case 25: return "the Pure-Pitched";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Bard";
      }
      break;

    case CLASS_JESTER:
    switch (level) {
      case  25: return "the Mime";
      case  26: return "the Juggler";
      case  27: return "the Poet";
      case  28: return "the Knife Juggler";
      case  29: return "the Balladist";
      case  30: return "the Herald";
      case  31: return "the Canter";
      case  32: return "the Satirist";
      case  33: return "Acidtongue";
      case 34: return "the Knife Thrower";
      case 35: return "the Dread Mime";
      case 36: return "the Danse Macabre";
      case 37: return "the Dark Poet";
      case 38: return "the Mad Lyricist";
      case 39: return "the Fire Spitter";
      case 40: return "the Ill";
      case LVL_SAINT: return "the Sad Clown";
      case LVL_BUILDER: return "Author of the Dope Rhymez";
      case LVL_LIEGE: return "Editor of the Dope Rhymez";
      case LVL_COUNT: return "Executive Producer of the Dopy Rhymez";
      case LVL_DEITY: return "the Divine Mime";
      case LVL_GOD: return "the God of Mimes";
      default: return "the Jester";
    }
    break;

    case CLASS_BLADE:
    switch (level) {
      case  25: return "the Apprentice Blade";
      case  26: return "the Journeyman Blade";
      case  27: return "the Blade";
      case  28: return "the Master Blade";
      case  29: return "Quickblade";
      case  30: return "Slingblade";
      case  31: return "Twinblade";
      case  32: return "Redblade";
      case  33: return "Blackblade";
      case 34: return "the Blademaster";
      case 35: return "Shadowblade";
      case 36: return "Dervish Blade";
      case 37: return "the Steel of Damascus";
      case 38: return "Fellblade";
      case 39: return "Tenblade";
      case 40: return "Dreadblade";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Blade Forger";
      case LVL_LIEGE: return "the Lord of Bladesmiths";
      case LVL_COUNT: return "the Commissioner of Blades and Standards";
      case LVL_DEITY: return "Tripleblade with Lubricated Strip";
      case LVL_GOD: return "the God of Steel";
      default: return "the Blade";
    }
    break; 



/*******************************************************************************
********************************************************************************
                 These are the FEMALE Warrior class titles
********************************************************************************
*******************************************************************************/

    case CLASS_FIGHTER:
    switch(level) {
      case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentry";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordsman";
      case  9: return "the Fencer";
      case 10: return "the Combatant";
      case 11: return "the Hero";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckler";
      case 14: return "the Mercenary";
      case 15: return "The Clueless";
      case LVL_SAINT: return "the Immortal Warlord";
      case LVL_DEITY: return "the Extirpator";
      case LVL_GOD: return "the God of war";
      default: return "the Fighter";
    }
    break;

 case CLASS_WARRIOR:
     switch(level) {
      case 10: return "the Hero";
      case 11: return "the Myrmidon";
      case 12: return "the Swashbuckler";
      case 13: return "the Mercenary";
      case 14: return "the Apprentice Warrior";
      case 15: return "the Journeyman Warrior";
      case 16: return "the Warrior";
      case 17: return "the Master Warrior";
      case 18: return "the Lone Warrior";
      case 19: return "the Dog of War";
      case 20: return "the Swordmaster";
      case 21: return "the Pikeman";
      case 22: return "the Lord of War";
      case 23: return "the Warbringer";
      case 24: return "the Lieutenent";
      case 25: return "the Colonel";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Warrior";
      }
      break;

  case CLASS_MONK:
    switch (level) {
      case  25: return "the Grasshopper";
      case  26: return "the Celibate";
      case  27: return "The Austere";
      case  28: return "Deathgrip";
      case  29: return "of the Deadly Touch";
      case  30: return "Redfist";
      case  31: return "Monkeyfist";
      case  32: return "Cranefoot";
      case  33: return "Silentfist";
      case 34: return "Dragonfist";
      case 35: return "Tenfist";
      case 36: return "the Jujitsu Master";
      case 37: return "the Kung Fu Master";
      case 38: return "the Drunken Master";
      case 39: return "the Wu Tang Master";
      case 40: return "the Shaolin Master";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Shriner";
      case LVL_LIEGE: return "the Ancient";
      case LVL_COUNT: return "the Monastic";
      case LVL_DEITY: return "the Divine Shaolin Warrior";
      case LVL_GOD: return "God of the Drunken Masters";
      default: return "the Monk";
    }
    break;

    case CLASS_BARBARIAN:
    switch (level) {
      case  25: return "Youngblood";
      case  26: return "Quickaxe";
      case  27: return "the Red";
      case  28: return "the Berserker";
      case  29: return "the Raider";
      case  30: return "Longstride";
      case  31: return "Redaxe";
      case  32: return "the Black";
      case  33: return "Longbeard";
      case 34: return "Redbeard";
      case 35: return "the Pillager";
      case 36: return "the Looter";
      case 37: return "Scourge of the South";
      case 38: return "Scourge of the East";
      case 39: return "Scourge of the West";
      case 40: return "Scourge of the North";
      case LVL_SAINT: return "the Retired";
      case LVL_BUILDER: return "the Fjordmaker";
      case LVL_LIEGE: return "of the Long House";
      case LVL_COUNT: return "Lord of the Icelands";
      case LVL_DEITY: return "the Divine Scourge";
      case LVL_GOD: return "God of Pillage";
      default: return "the Barbarian";
    }
    break;

 case CLASS_KNIGHT:
     switch(level) {
      case 10: return "the Apprentice Knight";
      case 11: return "the Journeyman Knight";
      case 12: return "the Knight Errant";
      case 13: return "the White Knight";
      case 14: return "the Green Knight";
      case 15: return "the Red Knight";
      case 16: return "the Purple Knight";
      case 17: return "the Vassal";
      case 18: return "the Master Knight";
      case 19: return "the Lord Knight";
      case 20: return "the High Knight";
      case 21: return "Knight of the Long Table";
      case 22: return "Knight of the Crown";
      case 23: return "Knight of the Land";
      case 24: return "Knight of the Round Table";
      case 25: return "the Dread Knight";
      case LVL_DEITY: return "fixme";
      case LVL_GOD: return "fixme";
      default: return "the Knight";
      }
      break;

    case CLASS_PALADIN:
    switch (level) {
      case  25: return "the Chevalier";
      case  26: return "the Pure";
      case  27: return "the Chaste";
      case  28: return "the Well-Groomed";
      case  29: return "the Polite";
      case  30: return "the Patient";
      case  31: return "the Civil";
      case  32: return "the Lawful";
      case  33: return "the Church-Going";
      case 34: return "the Pious";
      case 35: return "the Soft-Spoken";
      case 36: return "the Decorous";
      case 37: return "the Proper";
      case 38: return "the Tearful";
      case 39: return "the Constant";
      case 40: return "Manus Dei";
      case LVL_SAINT: return "the Swaggart-Haired";
      case LVL_BUILDER: return "the Cathedralist";
      case LVL_LIEGE: return "Steward of the See";
      case LVL_COUNT: return "the Cowardly";
      case LVL_DEITY: return "the Avenging Hand of God";
      case LVL_GOD: return "the Whitebred Eurocentric God";
      default: return "the Paladin";
    }
    break;

    case CLASS_SKNIGHT:
    switch (level) {
      case  25: return "Lucifer's Errandboy";
      case  26: return "Manservant Hecubus";
      case  27: return "the Devilish";
      case  28: return "the Sinister";
      case  29: return "the Blackadder";
      case  30: return "the Hellraiser";
      case  31: return "the Blackguard";
      case  32: return "the Infidel";
      case  33: return "the Villein";
      case 34: return "the Villein of the Deepest Dye";
      case 35: return "the Black Knight";
      case 36: return "the Shadow Knight";
      case 37: return "Vassal of the Nine Hells";
      case 38: return "Messenger of Beelzebub";
      case 39: return "Drinking Buddy of Asmodeus";
      case 40: return "Executor of Shaitan";
      case LVL_SAINT: return "the Dark";
      case LVL_BUILDER: return "Architect of the Hells";
      case LVL_LIEGE: return "Assistant Manager of Hell";
      case LVL_COUNT: return "Fry Chef of Hell";
      case LVL_DEITY: return "the Lucifer";
      case LVL_GOD: return "the Chirstpuncher";
      default: return "the Shadow Knight";
    }
    break;
    

  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}


int get_advance_hitpoints(struct char_data *ch)
{
  switch (GET_CLASS(ch)) {
    case CLASS_MAGIC_USER: return rand_number(5, 8);
    case CLASS_CLERIC: return rand_number(6, 10);
    case CLASS_THIEF: return rand_number(8, 12);
    case CLASS_FIGHTER: return rand_number(10, 14);
    case CLASS_PALADIN: return rand_number(9, 13);
    case CLASS_SKNIGHT: return rand_number(9, 13);
    case CLASS_BARD: return rand_number(8, 12);
    case CLASS_MONK: return rand_number(9, 13);
    //case CLASS_SAMURAI: return rand_number(12, 16);
    case CLASS_RANGER: return rand_number(9, 13);
    case CLASS_DRUID: return rand_number(6, 10);
    case CLASS_NECROMANCER: return rand_number(5, 8);
    default: return rand_number(10, 14);
  }
}


