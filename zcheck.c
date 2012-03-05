/**************************************************************************
2/17/2001

The functions in this file were created to help me maintain game balance.
They check items and make sure they don't cause too much damange, or make 
the wearer invincible, or make the mob completely invulnerable.  They 
make sure an unscruplous builder does not create a hard-to-find mob with 
a million experience points which he created simply so he cam use it for 
easy leveling as a player.  (Can you tell I've run into some bad builders?).  
They also make it easy to check for a loading room so you can see that a test 
mob with a million XP is not actually used anywhere.  It should be easy enough 
to add new checks, ex. checking that a SLASH weapon cannot be wielded by a 
cleric, or an elven blade by a wraith.  Anything you have a flag for can be 
checked.

The following help entries will explain the details.  One refers to WLIST,
a function I uploaded here some time ago.  Comments to raymond@brokersys.com.

ZONE_CHECK.C

function:  ACMD(do_zcheck)   
  Check certain values in mob, room and object structures to make sure they
  fit within game limits.
  
  syntax:  zcheck 
           no arguments, check your current zone
           zcheck * 
           scan every zone in the game

  NOTE:  The values are initially set to 1 or otherwise ridiculously low
         levels so you can see the output for each message and edit it to
         your liking.

function ACMD(do_checkloadstatus)
  calls void check_load_status
  Given a virtual number, checkload will report where and how the object
  or mob loads and its percent chance to load.
  
syntax:  checkload m|o <vnum>

***************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"

/*   external vars  */
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;   
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct char_data *mob_proto;
extern zone_rnum top_of_zone_table;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern room_rnum top_of_world;
extern int get_weapon_dam(struct obj_data *o);
extern struct char_data *read_mobile(mob_vnum nr, int type);
extern void char_to_room(struct char_data * ch, room_rnum room);

/* local functions */
ACMD(do_zcheck);
ACMD(do_checkloadstatus);

/**************************************************************************************/
/*                    Change any value to 0 to prevent checking it                    */
/**************************************************************************************/

/* Item limits */

#define MAX_AVERAGE_DAM_ALLOWED  20    /* for weapons */
#define MAX_AFFECTS_ALLOWED      3
#define MAX_MONEY_VALUE_ALLOWED  100000

/* Armor class limits*/
#define TOTAL_WEAR_CHECKS  (NUM_WEARS-2)  /*minus Wield and Take*/
struct zcheck_armor {
  bitvector_t bitvector;          /*from Structs.h                       */
  int ac_allowed;                 /* Max. AC allowed for this body part  */
  char *message;                  /* phrase for error message            */
}
zarmor[] = {
  {ITEM_WEAR_FINGER, -5, "Ring"},
  {ITEM_WEAR_NECK,   -10, "Necklace"},
  {ITEM_WEAR_BODY,   -50, "Body armor"},
  {ITEM_WEAR_HEAD,   -10, "Head gear"},
  {ITEM_WEAR_LEGS,   -15, "Legwear"},
  {ITEM_WEAR_FEET,   -10, "Footwear"},
  {ITEM_WEAR_HANDS,  -5, "Glove"},
  {ITEM_WEAR_ARMS,   -15, "Armwear"},
  {ITEM_WEAR_SHIELD, -20, "Shield"},
  {ITEM_WEAR_ABOUT,  -10, "Cloak"},
  {ITEM_WEAR_WAIST,  -10, "Belt"},
  {ITEM_WEAR_WRIST,  -10, "Wristwear"},
  {ITEM_WEAR_WIELD,  -5, "Wield"},
  {ITEM_WEAR_HOLD,   -5, "Held item"},
 // {ITEM_WEAR_MASK,   -5, "Mask"}
};

/*These two are strictly boolean*/
#define CAN_WEAR_WEAPONS         1     /* toggle - can a weapon also be a piece of armor? */
#define MAX_APPLIES_LIMIT        1     /* toggle - is there a limit at all?               */
/*
  Applies limits
  !! Very Important:  Keep these in the same order as in Structs.h
  These will be ignored if MAX_APPLIES_LIMIT = 0
*/
struct zcheck_affs {
  int aff_type;    /*from Structs.h*/
  int max_aff;     /*max. allowed value*/
  int min_aff;
  char *message;   /*phrase for error message*/
}
zaffs[] = {
  {APPLY_NONE,          0, 0, "unused0"},
  {APPLY_STR,           3, -5,  "strength"},
  {APPLY_DEX,           3, -5, "dexterity"},
  {APPLY_INT,           3, -5, "intelligence"},
  {APPLY_WIS,           3, -5, "wisdom"},
  {APPLY_CON,           3, -5, "constitution"},
  {APPLY_CHA,           3, -5, "charisma"},
  {APPLY_CLASS,         0, 0, "class"},
  {APPLY_LEVEL,         0, 0, "level"},
  {APPLY_AGE,           20, -20, "age"},
  {APPLY_CHAR_WEIGHT,   0, 0, "char weight"},
  {APPLY_CHAR_HEIGHT,   0, 0, "char height"},
  {APPLY_HIT,           50, -50, "hit points"},
  {APPLY_MANA,          50, -50, "mana"},
  {APPLY_MOVE,          50, -50, "moves"},
  {APPLY_HIT_GAIN,      5, 0, "hit gain"},
  {APPLY_MANA_GAIN,     5, 0, "mana gain"},
  {APPLY_MOVE_GAIN,     5, 0, "move gain"},
  {APPLY_GOLD,     50, -50, "gold gain"},
  {APPLY_EXP,      50, 0, "experience gain"},
 // {APPLY_AP,            5, -15, "magical AC"},
  {APPLY_HITROLL,       5, -5, "hitroll"},
  {APPLY_DAMROLL,       5, -5, "damroll"},
  {APPLY_SAVING_PARA,   10, -10, "saving throw (paralysis)"},
  {APPLY_SAVING_ROD,    10, -10, "saving throw (rod)"},
  {APPLY_SAVING_PETRI,  10, -10, "saving throw (petrify)"},
  {APPLY_SAVING_BREATH, 10, -10, "saving throw (breath)"},
  {APPLY_SAVING_SPELL,  10, -10, "saving throw (spell)"},
  {APPLY_RACE,          0, 0, "race"},
// {APPLY_TURN_LEVEL,    5, -5, "turn level"},
  {APPLY_ATTACKS,       2, -1, "attacks per 2 rounds"},
  {APPLY_SIZE,          1, -1, "size"},
// {APPLY_AGGR_GENERAL,  25, -25, "general aggression"},
// {APPLY_AGGR_EVIL,     25, -25, "evil aggression"},
// {APPLY_AGGR_GOOD,     25, -25, "good aggression"},
// {APPLY_AGGR_NEUTRAL,  25, -25, "neutral aggression"},
// {APPLY_AGGR_WIMPY,    25, -25, "wimpy aggression"},
//  {APPLY_AGGR_COWARD,   25, -25, "coward aggression"},
//  {APPLY_AGGR_MEMORY,   25, -25, "memory aggression"},
//  {APPLY_FIRE_RESIST,   25, -25, "fire resistance"},
//  {APPLY_ELEC_RESIST,   25, -25, "electrical resistance"},
//  {APPLY_COLD_RESIST,   25, -25, "cold resistance"},
//  {APPLY_POIS_RESIST,   25, -25, "poison resistance"},
//  {APPLY_SONC_RESIST,   25, -25, "sonic resistance"},
//  {APPLY_ACID_RESIST,   25, -25, "acid resistance"},
//  {APPLY_GAS_RESIST,    25, -25, "gas resistance"},
//  {APPLY_LGHT_RESIST,   25, -25, "light resistance"},
//  {APPLY_DIVN_RESIST,   25, -25, "divine resistance"},
//  {APPLY_SUMN_RESIST,   25, -25, "summon resistance"},
//  {APPLY_LIFE_RESIST,   25, -25, "life resistance"},
//  {APPLY_FEAR_RESIST,   25, -25, "fear resistance"},
//  {APPLY_MISC_RESIST,   25, -25, "miscellaneous resistance"},
//  {APPLY_SLSH_RESIST,   25, -25, "slash resistance"},
//  {APPLY_PIER_RESIST,   25, -25, "pierce resistance"},
//  {APPLY_BLDG_RESIST,   25, -25, "bludgeon resistance"}
};

/* Mobile limits */
#define MAX_DAMROLL_ALLOWED      15
#define MAX_HITROLL_ALLOWED      15
#define MAX_EXP_ALLOWED          150000
#define MAX_GOLD_ALLOWED         100000
#define MAX_NUM_ATTACKS_ALLOWED  12
#define MAX_LEVEL_ALLOWED        40

/* Room limits */
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
#define TOTAL_OFF_ZONES       3
const int offlimit_zones[] = {0, 3, 12};  /*what zones can no room connect to (virtual num) */

/***************************************************************************************/

ACMD (do_zcheck)
{  
  zone_vnum zone = 0;
  struct obj_data *obj;
  struct char_data *mob;
  room_vnum exroom=0;
  char buf1[SMALL_BUFSIZE];
  char buf2[500000]= ""; /* 1 megabyte */
  int ac=0;
  int affs=0;
  int i, j, k;
  bool check_all = FALSE;
  
  one_argument(argument, buf1);
  if (!strcmp(buf1, "*"))
    check_all = TRUE;
  else zone = world[ch->in_room].number/100;  /*get virtual number of zone to check*/  
  


 /************** Check mobs *****************/
  sprintf(buf2 + strlen(buf2), "Checking Mobs for limits...\r\n");
  /*check mobs first*/
  for (i=0; i<top_of_mobt;i++)
  {
    if (((mob_index[i].vnum/100) == zone) || check_all)   /*is mob in this zone?*/
    {
      mob=read_mobile(mob_index[i].vnum,VIRTUAL);          
      char_to_room(mob,0);
      if (MAX_DAMROLL_ALLOWED)  
      {             
        if (GET_DAMROLL(mob)>MAX_DAMROLL_ALLOWED)
          sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Damroll of %d is too high (limit: %d)\r\n", GET_MOB_VNUM(mob), GET_NAME(mob), GET_DAMROLL(mob), MAX_DAMROLL_ALLOWED);
      }
      if (MAX_HITROLL_ALLOWED)  
      {
        if (GET_HITROLL(mob)>MAX_HITROLL_ALLOWED)
          sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Hitroll of %d is too high (limit: %d)\r\n", GET_MOB_VNUM(mob), GET_NAME(mob), GET_HITROLL(mob), MAX_HITROLL_ALLOWED);
      }
      if (MAX_GOLD_ALLOWED)  
      {
        if (GET_GOLD(mob)>MAX_GOLD_ALLOWED)
          sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Has %ld gold (limit: %d)\r\n", GET_MOB_VNUM(mob), GET_NAME(mob), GET_GOLD(mob), MAX_GOLD_ALLOWED);
      }
      if (MAX_EXP_ALLOWED)  
      {
        if (GET_EXP(mob)>MAX_EXP_ALLOWED)
          sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Has %d experience (limit: %d)\r\n",
          GET_MOB_VNUM(mob), GET_NAME(mob), GET_EXP(mob), MAX_EXP_ALLOWED);
      }
      if (MAX_LEVEL_ALLOWED)  
      {
        if (GET_LEVEL(mob)>MAX_LEVEL_ALLOWED)
          sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Is level %d (limit: %d)\r\n",
          GET_MOB_VNUM(mob), GET_NAME(mob), GET_LEVEL(mob), MAX_LEVEL_ALLOWED);
      }
      if (MAX_NUM_ATTACKS_ALLOWED)
      {
        if (GET_MOB_ATTACKS(mob)>MAX_NUM_ATTACKS_ALLOWED)
          sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Has %d attacks (limit: %d)\r\n",
          GET_MOB_VNUM(mob), GET_NAME(mob), GET_ATTACKS(mob), MAX_NUM_ATTACKS_ALLOWED);
      }
      /** ADDITIONAL MOB CHECKS HERE **/
      extract_char(mob);  
    }   /*mob is in zone*/
  }  /*check mobs*/ 

 /** Check objects **/
  sprintf(buf2 + strlen(buf2), "\r\nChecking Objects for limits...\r\n");
  for (i=0; i<top_of_objt; i++)
  {
    if (((obj_index[i].vnum/100) == zone) || check_all)   /*is object in this zone?*/
    {
      obj=read_object(obj_index[i].vnum,VIRTUAL);            
      switch (GET_OBJ_TYPE(obj)) {
        case ITEM_MONEY:
          if (MAX_MONEY_VALUE_ALLOWED)
            if (GET_OBJ_VAL(obj, 0)>MAX_MONEY_VALUE_ALLOWED)
              sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Object is worth %d (money limit %d)\r\n", GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, 0), MAX_MONEY_VALUE_ALLOWED);
          break;
        case ITEM_WEAPON:
          if (MAX_AVERAGE_DAM_ALLOWED)
            if (get_weapon_dam(obj)>MAX_AVERAGE_DAM_ALLOWED)
              sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Average damage is %d (limit %d)\r\n", GET_OBJ_VNUM(obj), obj->short_description, (int)(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)), MAX_AVERAGE_DAM_ALLOWED);
          if (!CAN_WEAR_WEAPONS)
          {
            /* first remove legitimate weapon bits 
            if (OBJWEAR_FLAGGED(obj, ITEM_WEAR_TAKE)) TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
            if (OBJWEAR_FLAGGED(obj, ITEM_WEAR_WIELD)) TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WIELD);
            if (OBJWEAR_FLAGGED(obj, ITEM_WEAR_HOLD)) TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD);
            */
	    if (obj->obj_flags.wear_flags)  /* any bits still on? */
              sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Weapons cannot be worn as armor.\r\n",
            GET_OBJ_VNUM(obj), obj->short_description);    
          }  /* wear weapons */
          break;
        case ITEM_ARMOR:
          ac=GET_OBJ_VAL(obj,0);
          for (j=0; j<TOTAL_WEAR_CHECKS;j++)
          {
            if (ac<zarmor[j].ac_allowed)
              sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Item has AC %d (%s limit is %d)\r\n",  GET_OBJ_VNUM(obj), obj->short_description, ac, zarmor[j].message, zarmor[j].ac_allowed);
          }
          break;             
      }  /* switch on Item_Type */
      /* first check for over-all affections */
      if (MAX_AFFECTS_ALLOWED)
      {
        affs=0;
        for (j = 0; j < MAX_OBJ_AFFECT; j++) if (obj->affected[j].modifier) affs++;
        if (affs>MAX_AFFECTS_ALLOWED)
          sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Has %d affects (limit %d).\r\n", GET_OBJ_VNUM(obj), obj->short_description, affs, MAX_AFFECTS_ALLOWED);
      }
      if (MAX_APPLIES_LIMIT)
      {   
        for (j=0;j<MAX_OBJ_AFFECT;j++)
        {
          if (obj->affected[j].modifier > zaffs[obj->affected[j].location].max_aff)
          {                      
            sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Apply to %s is %d (upper limit %d).\r\n", GET_OBJ_VNUM(obj), obj->short_description,  zaffs[obj->affected[j].location].message, obj->affected[j].modifier, zaffs[obj->affected[j].location].max_aff);
          }
          if (obj->affected[j].modifier < zaffs[obj->affected[j].location].min_aff)
          {                      
            sprintf(buf2 + strlen(buf2), "[%5d] %-30s : Apply to %s is %d (lower limit %d).\r\n", GET_OBJ_VNUM(obj), obj->short_description,  zaffs[obj->affected[j].location].message, obj->affected[j].modifier, zaffs[obj->affected[j].location].min_aff);
          }
        }
      }
    /** ADDITIONAL OBJ CHECKS HERE **/
    extract_obj(obj);
    }   /* object is in zone */
  } /* check objects */

  /** Check rooms **/
  sprintf(buf2 + strlen(buf2), "\r\nChecking Rooms for limits...\r\n");
  for (i=0; i<top_of_world; i++) {
    if ((zone_table[world[i].zone].number==zone) || check_all) {
      for (j = 0; j < NUM_OF_DIRS; j++) {
      /* check for exit, but ignore off limits if you're in an offlimit zone */
        if (world[i].dir_option[j]) {
          exroom = world[i].dir_option[j]->to_room;   /*get zone it connects to*/
          if VALID_ROOM_RNUM(exroom) {
            if ((world[exroom].number/100)!=zone) {
              exroom = world[exroom].number;
              for (k = 0; k < TOTAL_OFF_ZONES; k++) {
              /* don't send warning if the room itself is in the same off-limit zone. */
                if (((exroom/100)==offlimit_zones[k]) && ((world[i].number/100) != offlimit_zones[k]))
                  sprintf(buf2 + strlen(buf2), "[%5d] Exit %s cannot connect to %d (zone off limits).\r\n", world[i].number, dirs[j], exroom);
              } /* bad exit? */
            }
          }
          else {
            sprintf(buf2 + strlen(buf2), "[%5d] Exit %s cannot connect to %d (room %d is out of bounds).\r\n", world[i].number, dirs[j], exroom, exroom);
          }
        }  /* exit exist? */
      }
    }  /* is room in this zone? */
  } /* checking rooms */
  page_string (ch->desc, buf2, 1); 
}

/**********************************************************************************/

#define ZCMD zone_table[zone].cmd[cmd_no]  /* from DB.C */

/* 
  type  = 'O' or 'M'
  which = real num.
  name  = common name of mob or object  
*/
void check_load_status (char type, mob_rnum which, struct char_data *ch, char *name)   
{
  zone_rnum zone;
  int cmd_no;
  room_vnum lastroom_v = NOWHERE;
  room_rnum lastroom_r = NOWHERE;
  mob_rnum lastmob_r = NOWHERE;
  char mobname[100];
  char buf[100000];
  struct char_data *mob;  
  
  sprintf(buf + strlen(buf), "Checking load info for %s...\r\n", name);

  for (zone=0; zone <= top_of_zone_table; zone++)
  {    
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) 
    {
      if (type == 'm' || type == 'M')
      {
        switch (ZCMD.command) {
          case 'M':                   /* read a mobile */
            if (ZCMD.arg1 == which)
            {
              sprintf(buf + strlen(buf), "  [%5d] %s (%d%% load, %d MAX)\r\n",world[ZCMD.arg3].number, world[ZCMD.arg3].name, (100-ZCMD.arg4), ZCMD.arg2);
            }
            break;
        }
      }
      else
      {
        switch (ZCMD.command) {
          case 'M':
            lastroom_v = world[ZCMD.arg3].number;
            lastroom_r = ZCMD.arg3;
            lastmob_r = ZCMD.arg1;
            mob = read_mobile(lastmob_r, REAL);
            char_to_room(mob, 0);
            strcpy(mobname, GET_NAME(mob));
            extract_char(mob);
            break;
          case 'O':  /* read an object */
            lastroom_v = world[ZCMD.arg3].number;
            lastroom_r = ZCMD.arg3;
            if (ZCMD.arg1 == which)
            {                       
              sprintf(buf + strlen(buf), "  [%5d] %s (%d%% Load, %d Max)\r\n",lastroom_v, world[lastroom_r].name, (100-ZCMD.arg4), ZCMD.arg2);
            }
            break;
          case 'P':  /* object to object */
            if (ZCMD.arg1 == which)
            {
              sprintf(buf + strlen(buf), "  [%5d] %s (Put in another object [%d%% Load, %d Max])\r\n",lastroom_v, world[lastroom_r].name, (100-ZCMD.arg4), ZCMD.arg2);
            }
            break;
          case 'G': /* obj_to_char */
            if (ZCMD.arg1 == which)
            {
              sprintf(buf + strlen(buf), "  [%5d] %s (Given to %s [%d][%d%% Load, %d Max])\r\n",lastroom_v, world[lastroom_r].name, mobname, mob_index[lastmob_r].vnum, (100-ZCMD.arg4), ZCMD.arg2);
            }
            break;
          case 'E': /* object to equipment list */
            if (ZCMD.arg1 == which)
            {
              sprintf(buf + strlen(buf), "  [%5d] %s (Equipped to %s [%d][%d%% Load, %d Max])\r\n",lastroom_v, world[lastroom_r].name, mobname, mob_index[lastmob_r].vnum, (100-ZCMD.arg4), ZCMD.arg2);
            }
            break;
          case 'R': /* rem obj from room */
            lastroom_v = world[ZCMD.arg1].number;
            lastroom_r = ZCMD.arg1;
            if (ZCMD.arg2 == which)
            {
              sprintf(buf + strlen(buf), "  [%5d] %s (Removed from room)\r\n",lastroom_v, world[lastroom_r].name);                    
            }
            break;
        }/* switch for object */
      }  /* else */
    } /* for cmd_no */
  }  /* for zone */
  page_string (ch->desc, buf, 1);
}

ACMD(do_checkloadstatus)
{  
  mob_rnum which;
  struct char_data *mob;
  struct obj_data *obj;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  two_arguments(argument, buf1, buf2);
  if ((!*buf1) || (!*buf2) || (!isdigit(*buf2)))
  {
    send_to_char(ch, "Checkload <M | O> <vnum>\r\n");
    return;
  }   

  /* These lines do nothing but look up the name of the object or mob
    so the output is more user friendly */
  if (*buf1 == 'm')
  {          
    which = real_mobile(atoi(buf2));
    mob = read_mobile(which, REAL);
    char_to_room(mob, 0);
    strcpy(buf2, GET_NAME(mob));
    extract_char(mob);
  }
  else
  {
    which = real_object(atoi(buf2));
    obj = read_object(which, REAL);
    strcpy(buf2, obj->short_description);
    extract_obj(obj);
  }   
  check_load_status(buf1[0], which, ch, buf2);  
}
