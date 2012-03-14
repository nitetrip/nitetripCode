/* ************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
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
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
//dan clan system
#include "clan.h"


/* extern variables */
extern int top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
extern int top_of_zonet;
extern struct zone_list *zone_info_table;
extern char *zonelist;
extern struct time_info_data time_info;
extern const char *pc_race_types[];
extern const char *pc_class_types[];
extern const char *sizes[];
extern const char *mob_race_types[];
extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *class_abbrevs[];
extern char *race_abbrevs[];
extern char *clan_names[];
extern char *clan_names_output[];
extern char *clan_ranks[];
extern struct spell_info_type spell_info[];

/* extern functions */
ACMD(do_action);
ACMD(do_insult);
bitvector_t find_class_bitvector(const char *arg);
bitvector_t find_race_bitvector(const char *arg);
int level_exp(int chclass, int level);
char *title_male(int chclass, int level);
char *title_female(int chclass, int level);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
int compute_armor_class(struct char_data *ch);
byte saving_throws_nat(int class_num, int type, int level); /* class.c */
int get_num_attacks(struct char_data *ch);
int parse_class(char arg);
void get_one_line(FILE *fl, char *buf);

/* local functions */
void load_zonelist(FILE *fl);
int sort_commands_helper(const void *a, const void *b);
void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur);
void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode);
void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show, int showcont);
void show_obj_modifiers(struct obj_data *obj, struct char_data *ch);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
void perform_mortal_where(struct char_data *ch, char *arg);
void perform_immort_where(struct char_data *ch, char *arg);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
void sort_commands(void);
ACMD(do_commands);
ACMD(do_attributes);
ACMD(do_scan);
ACMD(do_zone);

void diag_char_to_char(struct char_data *i, struct char_data *ch);
void look_at_char(struct char_data *i, struct char_data *ch);
void list_one_char(struct char_data *i, struct char_data *ch);
void list_char_to_char(struct char_data *list, struct char_data *ch);
void do_auto_exits(room_rnum target_room, struct char_data *ch);
ACMD(do_exits);
void look_in_direction(struct char_data *ch, int dir);
void look_in_obj(struct char_data *ch, char *arg);
char *find_exdesc(char *word, struct extra_descr_data *list);
void look_at_target(struct char_data *ch, char *arg);
/* local globals */
int *cmd_sort_info;

/* For show_obj_to_char 'mode'.	/-- arbitrary */
#define SHOW_OBJ_LONG		0
#define SHOW_OBJ_SHORT		1
#define SHOW_OBJ_ACTION		2
#define SHOW_OBJ_NOMODS		3


void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode)
{
  if (!obj || !ch) {
    log("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", obj, ch);
    return;
  }

  switch (mode) {
  case SHOW_OBJ_LONG:
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
     {
     if ((!OBJ_FLAGGED(obj, ITEM_GREATER_HIDDEN)) && (!OBJ_FLAGGED(obj, ITEM_LESSER_HIDDEN)))
     send_to_char(ch, "[%d] %s", GET_OBJ_VNUM(obj), obj->description);
     else
     send_to_char(ch, "[%d] %s [HIDDEN]", GET_OBJ_VNUM(obj), obj->description);
    }
    else
    send_to_char(ch, "%s", obj->description);
    break;

  case SHOW_OBJ_SHORT:
    /* drop through */
  case SHOW_OBJ_NOMODS:
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
     {
     if ((!OBJ_FLAGGED(obj, ITEM_GREATER_HIDDEN)) && (!OBJ_FLAGGED(obj, ITEM_LESSER_HIDDEN)))
     send_to_char(ch, "%s", obj->short_description);
     else
     send_to_char(ch, "%s [HIDDEN]", obj->short_description);
    }
    else
    send_to_char(ch, "%s", obj->short_description);
    break;

  case SHOW_OBJ_ACTION:
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_NOTE:
      if (obj->action_description) {
        char notebuf[MAX_NOTE_LENGTH + 64];

        snprintf(notebuf, sizeof(notebuf), "There is something written on it:\r\n\r\n%s", obj->action_description);
        page_string(ch->desc, notebuf, TRUE);
      } else
	send_to_char(ch, "It's blank.\r\n");
      return;

    case ITEM_DRINKCON:
      send_to_char(ch, "It looks like a drink container.");
      break;

    default:
      send_to_char(ch, "You see nothing special.");
      break;
    }
    break;

  default:
    log("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
    return;
  }

  if (mode == SHOW_OBJ_SHORT)
    show_obj_modifiers(obj, ch);
  send_to_char(ch, "\r\n");
}


void show_obj_modifiers(struct obj_data *obj, struct char_data *ch)
{
  if (OBJ_FLAGGED(obj, ITEM_INVISIBLE))
    send_to_char(ch, " (invisible)");

  if (((OBJ_FLAGGED(obj, ITEM_BLESS)) || (OBJ_FLAGGED(obj, ITEM_ANGELIC)) || ((OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL)) && (OBJ_FLAGGED(obj, ITEM_ANTI_EVIL))) ||(OBJ_FLAGGED(obj, ITEM_GOOD_FLAGGED))) && ((AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) || (AFF_FLAGGED(ch, AFF_DETECT_GOOD))))
    send_to_char(ch, " ..It glows blue!");
    
  if (((OBJ_FLAGGED(obj, ITEM_EVIL_FLAGGED)) || (OBJ_FLAGGED(obj, ITEM_DEMONIC)) || (((OBJ_FLAGGED(obj, ITEM_ANTI_GOOD)) && (OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL))))) && ((AFF_FLAGGED(ch, AFF_DETECT_EVIL)) || (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))))
    send_to_char(ch, " ..it glows red!");
    
  if (((OBJ_FLAGGED(obj, ITEM_ANTI_GOOD)) && (OBJ_FLAGGED(obj, ITEM_ANTI_EVIL))) && (AFF_FLAGGED(ch, AFF_DETECT_NEUTRAL)))
        send_to_char(ch, " ..it glows gray!");

  if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
    send_to_char(ch, " ..It glows yellow!");

  if (OBJ_FLAGGED(obj, ITEM_GLOW))
    send_to_char(ch, " ..It has a soft glowing aura!");
 
  if (OBJ_FLAGGED(obj, ITEM_RADIATE))
    send_to_char(ch, " ..It radiates a bright light!");


  if (OBJ_FLAGGED(obj, ITEM_HUM))
    send_to_char(ch, " ..It emits a faint humming sound!");
}


void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show, int showcont)
{
  /* 
    * Editing:  act.item.c act.wizard.c structs.h constants.c oasis.h act.informative.c 
    *               config.c spec_assign.c spec_procs.c act.movement.c
    */ 
    
    /* showcont values: 0 list of objs in room 
     * 			     1 list of objs in a container in a room
     *				     2 list of objs in inv or in a container in inv/eq
     */
   
  
  struct obj_data *i;
  bool found = FALSE;
  
  if (showcont == 0) {
    found = TRUE; /* Never want to see "Nothing." in a room. */
   for (i = list; i; i = i->next_content) {
    if (((OBJ_FLAGGED(i, ITEM_GREATER_HIDDEN)) || (OBJ_FLAGGED(i, ITEM_LESSER_HIDDEN))) && (GET_LEVEL(ch) > LVL_DEITY)) {
   show_obj_to_char(i, ch, mode);
   found = TRUE;
   }
   if ((!OBJ_FLAGGED(i, ITEM_GREATER_HIDDEN)) && (!OBJ_FLAGGED(i, ITEM_LESSER_HIDDEN))) {
      show_obj_to_char(i, ch, mode);   
      found = TRUE;
   }
   }
  }
  
  if (showcont == 1) {
      
      for (i = list; i; i = i->next_content) {
    	 if (CAN_SEE_OBJ(ch, i)) {
    		if ((!OBJ_FLAGGED(i, ITEM_GREATER_HIDDEN)) && (!OBJ_FLAGGED(i, ITEM_LESSER_HIDDEN))) {
  		show_obj_to_char(i, ch, mode);
  		found = TRUE;
 }
 	        if (((OBJ_FLAGGED(i, ITEM_GREATER_HIDDEN)) || (OBJ_FLAGGED(i, ITEM_LESSER_HIDDEN))) && (GET_LEVEL(ch) > LVL_DEITY)) {
		show_obj_to_char(i, ch, mode);   
   		found = TRUE;	       
	       }
 }
 }
 }  
    
  if (showcont == 2) {
    for (i = list; i; i = i->next_content) {
       if (CAN_SEE_OBJ(ch, i)) {
       show_obj_to_char(i, ch, mode);
       found = TRUE; 
      }
      }
  }

  if (!found)
    send_to_char(ch, " Nothing.\r\n");
}


void diag_char_to_char(struct char_data *i, struct char_data *ch)
{
  struct {
    byte percent;
    const char *text;
  } diagnosis[] = {
    { 100, "is in excellent condition."			},
    {  90, "has a few scratches."			},
    {  75, "has some small wounds and bruises."		},
    {  50, "has quite a few wounds."			},
    {  30, "has some big nasty wounds and scratches."	},
    {  15, "looks pretty hurt."				},
    {   0, "is in awful condition."			},
    {  -1, "is bleeding awfully from big wounds."	},
  };
  int percent, ar_index;
  const char *pers = PERS(i, ch);

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "%c%s %s\r\n", UPPER(*pers), pers + 1, diagnosis[ar_index].text);
}


void look_at_char(struct char_data *i, struct char_data *ch)
{
  int j, found;
  struct obj_data *tmp_obj;

  if (!ch->desc)
    return;

   if (i->player.description)
    send_to_char(ch, "%s", i->player.description);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  diag_char_to_char(i, ch);

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;

  if (found) {
    send_to_char(ch, "\r\n");	/* act() does capitalization. */
    act("$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
	send_to_char(ch, "%s", wear_where[j]);
	show_obj_to_char(GET_EQ(i, j), ch, SHOW_OBJ_SHORT);
      }
  }
  if (ch != i && (IS_THIEF(ch) || GET_LEVEL(ch) >= LVL_SAINT)) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) && (rand_number(0, 20) < GET_LEVEL(ch))) {
	show_obj_to_char(tmp_obj, ch, SHOW_OBJ_NOMODS);
	found = TRUE;
      }
    }

    if (!found)
      send_to_char(ch, "You can't see anything.\r\n");
  }
}


void list_one_char(struct char_data *i, struct char_data *ch)
{
  const char *positions[] = {
    " is lying here, dead.",
    " is lying here, mortally wounded.",
    " is lying here, incapacitated.",
    " is here, stiff as a board.",
    " is lying here, stunned.",
    " is lying here, stunned.",
    " is here, lying in a shallow grave.",
    " is here sleeping.",
    " is here meditating.",
    " is here resting.",
    " is here sitting.",
    "!FIGHTING!",
    " is here standing.",
    " is here floating."
  };

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS) && IS_NPC(i)) 
     send_to_char(ch,"[%d] ", GET_MOB_VNUM(i));
  
if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
    if (AFF_FLAGGED(i, AFF_INVISIBLE))
      send_to_char(ch, "*");

    if (IS_CLOAKED(i))
      send_to_char(ch, "#");

  
    if (AFF_FLAGGED(i, AFF_CHARM)) send_to_char(ch, "(Charmed) ");

    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
      if (IS_EVIL(i))
	send_to_char(ch, "(Red Aura) ");
      else if (IS_GOOD(i))
	send_to_char(ch, "(Blue Aura) "); }
        
    if (AFF_FLAGGED(ch, AFF_DETECT_EVIL)) {
        if (IS_EVIL(i))
          send_to_char(ch, "(Red Aura) ");}
    
    if (AFF_FLAGGED(ch, AFF_DETECT_GOOD)) {
        if (IS_GOOD(i))
            send_to_char(ch, "(Blue Aura) ");}
    
    if (AFF_FLAGGED(ch, AFF_DETECT_NEUTRAL)) {
        if (IS_NEUTRAL(i))
            send_to_char(ch, "(Gray Aura) ");}
        
        
   
  
  if (IS_INVIS(i)) send_to_char(ch, " (invisible)");
  if (AFF_FLAGGED(i, AFF_HIDE)) send_to_char(ch, " (hidden)");
  if (!IS_NPC(i) && !i->desc) send_to_char(ch, " (linkless)");
  if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING)) send_to_char(ch, " (writing)");
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK)) send_to_char(ch, " (buildwalk)");

    send_to_char(ch, "%s", i->player.long_descr);

    if (AFF_FLAGGED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);

    if (AFF_FLAGGED(i, AFF_FORT))
      act("...$e is surrounded by a pitch black aura!", FALSE, i, 0, ch, TO_VICT);

    if (AFF_FLAGGED(i, AFF_DERVISH_SPIN))
      act("...$e is whirling madly in a dervish spin!", FALSE, i, 0, ch, TO_VICT);
      
    if (AFF_FLAGGED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);

    return;
  }

  if (IS_NPC(i))
    send_to_char(ch, "%c%s", UPPER(*i->player.short_descr), i->player.short_descr + 1);
  else
 //   send_to_char(ch, "%s %s", i->player.name, GET_TITLE(i));
     send_to_char(ch, "%s", GET_TITLE(i) != NULL ? GET_TITLE(i) : GET_NAME(ch));


  if (AFF_FLAGGED(i, AFF_INVISIBLE))
    send_to_char(ch, " (invisible)");
  if (IS_CLOAKED(i)) send_to_char(ch, " (cloaked)");
  if (AFF_FLAGGED(i, AFF_HIDE))
    send_to_char(ch, " (Hidden)");
  if (!IS_NPC(i) && !i->desc)
    send_to_char(ch, " (%sLinkdead%s)", CCBCN(ch, C_SPR), CCNRM(ch, C_SPR));
  if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
    send_to_char(ch, " (Writing)");
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK))
    send_to_char(ch, " (Buildwalk)");

  if (GET_POS(i) != POS_FIGHTING)
    send_to_char(ch, "%s", positions[(int) GET_POS(i)]);
  else {
    if (FIGHTING(i)) {
      send_to_char(ch, " is here, fighting ");
      if (FIGHTING(i) == ch)
	send_to_char(ch, "YOU!");
      else {
	if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
	  send_to_char(ch, "%s!", PERS(FIGHTING(i), ch));
	else
	  send_to_char(ch,  "someone who has already left!");
      }
    } else			/* NIL fighting pointer */
      send_to_char(ch, " is here struggling with thin air.");
  }

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      send_to_char(ch, " (Red Aura)");
    else if (IS_GOOD(i))
      send_to_char(ch, " (Blue Aura)");
  }
  send_to_char(ch, "\r\n");

  if (AFF_FLAGGED(i, AFF_SANCTUARY))
    act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);

  if (AFF_FLAGGED(i, AFF_DERVISH_SPIN))
    act("...$e is whirling madly in a dervish spin!", FALSE, i,0,ch, TO_VICT);

  if (AFF_FLAGGED(i, AFF_FORT))
    act("...$e is surrounded by a pitch black aura!", FALSE, i, 0, ch, TO_VICT);
}



void list_char_to_char(struct char_data *list, struct char_data *ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room)
    if (ch != i) {
      if (CAN_SEE(ch, i))
	list_one_char(i, ch);
      else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) &&
	       AFF_FLAGGED(i, AFF_INFRAVISION))
	send_to_char(ch, "You see a pair of glowing red eyes looking your way.\r\n");
    else if (AFF_FLAGGED(ch, AFF_SENSE_LIFE))
      send_to_char(ch, "You sense a hidden life form in the room.\r\n");

    }
}


void do_auto_exits(room_rnum target_room, struct char_data *ch)
{
  int door, slen = 0;

  send_to_char(ch, "%s[ Exits: ", CCCYN(ch, C_NRM));

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (!W_EXIT(target_room, door) || W_EXIT(target_room, door)->to_room == NOWHERE)
      continue;
    if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED))
      continue;

    send_to_char(ch, "%c ", LOWER(*dirs[door]));
    slen++;
  }

  send_to_char(ch, "%s]%s\r\n", slen ? "" : "None ", CCNRM(ch, C_NRM));
}


ACMD(do_exits)
{
  int door, len = 0;

  if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
    return;
  }

  send_to_char(ch, "Obvious exits:\r\n");

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (!EXIT(ch, door) || EXIT(ch, door)->to_room == NOWHERE)
      continue;
    if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
      continue;

    len++;

    if (GET_LEVEL(ch) >= LVL_SAINT)
      send_to_char(ch, "%-5s - [%5d] %s\r\n", dirs[door], GET_ROOM_VNUM(EXIT(ch, door)->to_room),
		world[EXIT(ch, door)->to_room].name);
    else
      send_to_char(ch, "%-5s - %s\r\n", dirs[door], IS_DARK(EXIT(ch, door)->to_room) &&
		!CAN_SEE_IN_DARK(ch) ? "Too dark to tell." : world[EXIT(ch, door)->to_room].name);
  }

  if (!len)
    send_to_char(ch, " None.\r\n");
}



void look_at_room(room_rnum target_room, struct char_data *ch, int ignore_brief)
{
  if (!ch->desc)
    return;

  if (IS_DARK(target_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char(ch, "You see nothing but infinite darkness...\r\n");
    return;
  }
  send_to_char(ch, "%s", CCBCN(ch, C_NRM));
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

    sprinttype(world[target_room].size, sizes, buf2, sizeof(buf2));
    sprintbit(ROOM_FLAGS(target_room), room_bits, buf, sizeof(buf));
    send_to_char(ch, "[%5d] %s [ %s] [ %s ]", GET_ROOM_VNUM(target_room), world[target_room].name, buf, 
buf2);
  } else
    send_to_char(ch, "%s", world[target_room].name);

  send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));

  if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
      ROOM_FLAGGED(target_room, ROOM_DEATH))
    send_to_char(ch, "%s", world[target_room].description);

  /* autoexits */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(target_room, ch);

  /* now list characters & objects */
  send_to_char(ch, "%s", CCNRM(ch, C_NRM));
  list_obj_to_char(world[target_room].contents, ch, SHOW_OBJ_LONG, FALSE,0);
  send_to_char(ch, "%s", CCNRM(ch, C_NRM));
  list_char_to_char(world[target_room].people, ch);
  send_to_char(ch, "%s", CCNRM(ch, C_NRM));
}



void look_in_direction(struct char_data *ch, int dir)
{
  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description)
      send_to_char(ch, "%s", EXIT(ch, dir)->general_description);
    else
	send_to_char(ch, "You see nothing special there.\r\n");

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && EXIT(ch, dir)->keyword)
	  {
    if (!EXIT_FLAGGED(EXIT(ch, dir), EX_HIDDEN))
 send_to_char(ch, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
	  }
	else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
  } else
  send_to_char(ch, "You see nothing special there.\r\n");
}



void look_in_obj(struct char_data *ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char(ch, "Look in what?\r\n");
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				 FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	     (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	     (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
    send_to_char(ch, "There's nothing inside that!\r\n");
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
	send_to_char(ch, "It is closed.\r\n");
      else {
	send_to_char(ch, "%s", fname(obj->name));
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(ch, " (carried): \r\n");
	  list_obj_to_char(obj->contains, ch, SHOW_OBJ_NOMODS, TRUE,2);
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(ch, " (here): \r\n");
	  list_obj_to_char(obj->contains, ch, SHOW_OBJ_NOMODS, TRUE,1);
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(ch, " (used): \r\n");
	  list_obj_to_char(obj->contains, ch, SHOW_OBJ_NOMODS, TRUE,2);
	  break;
	}

	
      }
    } else {		/* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, 1) <= 0)
	send_to_char(ch, "It is empty.\r\n");
      else {
	if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0)) {
	  send_to_char(ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
	} else {
          char buf2[MAX_STRING_LENGTH];
	  amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
	  sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2, sizeof(buf2));
	  send_to_char(ch, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
	}
      }
    }
  }
}



char *find_exdesc(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
void look_at_target(struct char_data *ch, char *arg)
{
  int bits, found = FALSE, j, fnum, i = 0;
  struct char_data *found_char = NULL;
  struct obj_data *obj, *found_obj = NULL;
  char *desc;

  if (!ch->desc)
    return;

  if (!*arg) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (CAN_SEE(found_char, ch))
	act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }

  /* Strip off "number." from 2.foo and friends. */
  if (!(fnum = get_number(&arg))) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[IN_ROOM(ch)].ex_description)) != NULL && ++i == fnum) {
    page_string(ch->desc, desc, FALSE);
    return;
  }

  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL && ++i == fnum) {
	send_to_char(ch, "%s", desc);
	found = TRUE;
      }

  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	send_to_char(ch, "%s", desc);
	found = TRUE;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[IN_ROOM(ch)].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	send_to_char(ch, "%s", desc);
	found = TRUE;
      }

  /* If an object was found back in generic_find */
  if (bits) {
    if (!found)
      show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
    else {
      show_obj_modifiers(found_obj, ch);
      send_to_char(ch, "\r\n");
    }
  } else if (!found)
    send_to_char(ch, "You do not see that here.\r\n");
}


ACMD(do_look)
{
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char(ch, "You can't see anything but stars!\r\n");
  else if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
  else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
    list_char_to_char(world[IN_ROOM(ch)].people, ch);	/* glowing red eyes */
  } else {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
	send_to_char(ch, "Read what?\r\n");
      else
	look_at_target(ch, arg);
      return;
    }
    if (!*arg)			/* "look" alone, without an argument at all */
      look_at_room(IN_ROOM(ch), ch, 1);
    else if (is_abbrev(arg, "inside"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if (is_abbrev(arg, "at"))
      look_at_target(ch, arg2);
    else
      look_at_target(ch, arg);
  }
}



ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  char tempsave[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Examine what?\r\n");
    return;
  }

  /* look_at_target() eats the number. */
  look_at_target(ch, strcpy(tempsave, arg));	/* strcpy: OK */

  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char(ch, "When you look inside, you see:\r\n");
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char(ch, "You're broke!\r\n");
  else if (GET_GOLD(ch) == 1)
    send_to_char(ch, "You have one miserable little gold coin.\r\n");
  else
    send_to_char(ch, "You have %d gold coins.\r\n", GET_GOLD(ch));
}


ACMD(do_score)
{
  struct time_info_data playing_time;

  if (IS_NPC(ch))
    return;

  send_to_char(ch, "You are %d years old.\r\n", GET_AGE(ch));

  if (age(ch)->month == 0 && age(ch)->day == 0)
    send_to_char(ch, "It's your birthday today.\r\n");

  send_to_char(ch, "You have %d(%d) hit, %d(%d) mana and %d(%d) movement points.\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  send_to_char(ch, "Your armor class is %d, and your alignment is %d.\r\n",
	  compute_armor_class(ch), GET_ALIGNMENT(ch));

  if (GET_LEVEL(ch) < LVL_SAINT)
	{
    if (PLR_FLAGGED(ch, PLR_PROGRESS))
	send_to_char(ch, "You have scored %d exp, but need to progress at your guild.\r\n", GET_EXP(ch));
	else
	send_to_char(ch, "You have scored %d exp, and you need %d more to level.\r\n", GET_EXP(ch), level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1) - GET_EXP(ch));
    }  
  else
	send_to_char(ch, "You have scored %d exp, and are Immortal.\r\n", GET_EXP(ch));
 
  if (GET_GOLD(ch) == 1)	 
  send_to_char(ch, "You carry %d coin, and have %d more in the bank.\r\n", GET_GOLD(ch), GET_BANK_GOLD(ch));
  else
  send_to_char(ch, "You carry %d coins, and have %d more in the bank.\r\n", GET_GOLD(ch), GET_BANK_GOLD(ch));

  playing_time = *real_time_passed((time(0) - ch->player.time.logon) +
				  ch->player.time.played, 0);
  send_to_char(ch, "You have been playing for %d day%s and %d hour%s.\r\n",
     playing_time.day, playing_time.day == 1 ? "" : "s",
     playing_time.hours, playing_time.hours == 1 ? "" : "s");


 if(GET_CLAN(ch) > CLAN_NONE)
    send_to_char(ch,   "Clan : %s (%s)\r\nRank : %s%s\r\n",
     get_blank_clan_name(GET_CLAN(ch)), get_clan_name(GET_CLAN(ch)),
     get_rank_name(GET_CLAN(ch), GET_CLAN_RANK(ch)), "");


  send_to_char(ch, "Your title is %s (level %d).\r\n",
	   GET_TITLE(ch), GET_LEVEL(ch));

  switch (GET_POS(ch)) {
  case POS_DEAD:
    send_to_char(ch, "You are DEAD!\r\n");
    break;
  case POS_MORTALLYW:
    send_to_char(ch, "You are mortally wounded!  You should seek help!\r\n");
    break;
  case POS_INCAP:
    send_to_char(ch, "You are incapacitated, slowly fading away...\r\n");
    break;
  case POS_STUNNED:
    send_to_char(ch, "You are stunned!  You can't move!\r\n");
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You are sleeping.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You are resting.\r\n");
    break;
  case POS_SITTING:
    send_to_char(ch, "You are sitting.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "You are fighting %s.\r\n", FIGHTING(ch) ? PERS(FIGHTING(ch), ch) : "thin air");
    break;
  case POS_STANDING:
    send_to_char(ch, "You are standing.\r\n");
    break;
  default:
    send_to_char(ch, "You are floating.\r\n");
    break;
  }

  if (GET_COND(ch, DRUNK) > 10)
    send_to_char(ch, "You are intoxicated.\r\n");
  if (GET_COND(ch, FULL) == 0)
    send_to_char(ch, "You are hungry.\r\n");
  if (GET_COND(ch, THIRST) == 0)
    send_to_char(ch, "You are thirsty.\r\n");

    if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    send_to_char(ch, "You are summonable by other players.\r\n");


//Spells
 if (affected_by_spell(ch, SPELL_ACCURACY))
    send_to_char(ch, "You hit with exactness and precision.\r\n");
  if (affected_by_spell(ch, SPELL_AID))
    send_to_char(ch, "You feel more fit.\r\n");
  if (affected_by_spell(ch, SPELL_ARMOR))
    send_to_char(ch, "You feel protected.\r\n");
  if (affected_by_spell(ch, SPELL_BARKSKIN))
    send_to_char(ch, "Your skin is all covered with bark.\r\n");
  if (affected_by_spell(ch, SPELL_BLESS))
    send_to_char(ch, "You feel righteous.\r\n");
  if (affected_by_spell(ch, SPELL_CHAMPIONS_STRENGTH))
    send_to_char(ch, "The strength of past heroes run through your muscles.\r\n");
  if (affected_by_spell(ch, SPELL_CURSE))
    send_to_char(ch, "You have been cursed.\r\n");
  if (affected_by_spell(ch, SPELL_DEATH_STRIKE))
    send_to_char(ch, "Your attacks are ferocious since you're filled with Ki power.\r\n");
  if (affected_by_spell(ch, SPELL_DRAW_UPON_HOLY_MIGHT))
    send_to_char(ch, "Your deity has endowed you with power.\r\n");
  if (affected_by_spell(ch, SPELL_ELEMENTAL_SHIELD))
    send_to_char(ch, "You are protected by an elemental shield.\r\n");
  if (affected_by_spell(ch, SPELL_ELEMENTAL_AURA))
    send_to_char(ch, "You are surrounded by an elemental aura.\r\n");
  if (affected_by_spell(ch, SPELL_ENFEEBLEMENT))
    send_to_char(ch, "Your muscles feel exhausted.\r\n");
  if (affected_by_spell(ch, SPELL_ENLARGE))
    send_to_char(ch, "You have been enlarged in size.\r\n");
  if (affected_by_spell(ch, SPELL_FLAMEWALK))
    send_to_char(ch, "You feel resistant to fire attacks.\r\n");
  if (affected_by_spell(ch, SPELL_INTIMIDATE))
    send_to_char(ch, "You feel intimidated.\r\n");
  if (affected_by_spell(ch, SPELL_IMMUNITY_TO_COLD))
    send_to_char(ch, "You are immune to cold.\r\n");
  if (affected_by_spell(ch, SPELL_IMMUNITY_TO_ELEC))
    send_to_char(ch, "You are immune to electricity.\r\n");
  if (affected_by_spell(ch, SPELL_MAGICAL_VESTMANTS))
    send_to_char(ch, "Your faith protects you against the dangers of the world.\r\n");
  if (affected_by_spell(ch, SPELL_POWER_STRIKE))
    send_to_char(ch, "Your attacks hit at maximum damage.\r\n");
  if (affected_by_spell(ch, SPELL_RESISTANCE_TO_COLD))
    send_to_char(ch, "You are resistant to cold.\r\n");
  if (affected_by_spell(ch, SPELL_RESISTANCE_TO_ELEC))
    send_to_char(ch, "You are resistant to electricity.\r\n");
  if (affected_by_spell(ch, SPELL_SHIELD))
    send_to_char(ch, "You sense a strong shield of magic protecting you.\r\n");
  if (affected_by_spell(ch, SPELL_SHRINK))
    send_to_char(ch, "You have been shrunk in size.\r\n");
  if (affected_by_spell(ch, SPELL_SKELETAL_GUISE))
    send_to_char(ch, "Your features are skeletal.\r\n");
  if (affected_by_spell(ch, SPELL_SOMNOLENT_GAZE))
    send_to_char(ch, "You feel lethargic.\r\n");
  if (affected_by_spell(ch, SPELL_STRENGTH))
    send_to_char(ch, "You feel physically strong.\r\n");
  if (affected_by_spell(ch, SPELL_STRENGTH_BURST))
    send_to_char(ch, "Your muscles are filled with Ki power.\r\n");
  if (affected_by_spell(ch, SPELL_WINDWALK))
    send_to_char(ch, "You feel resistant to gas and electrical attacks.\r\n");
  if (affected_by_spell(ch, SPELL_WITHER))
    send_to_char(ch, "Your wielding arm is all shriveled up.\r\n");


}


ACMD(do_inventory)
{
  send_to_char(ch, "You are carrying:\r\n");
  list_obj_to_char(ch->carrying, ch, SHOW_OBJ_NOMODS, TRUE, 2);
}


ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
	send_to_char(ch, "%s", wear_where[i]);
	show_obj_to_char(GET_EQ(ch, i), ch, SHOW_OBJ_SHORT);
	found = TRUE;
      } else {
	send_to_char(ch, "%s", wear_where[i]);
	send_to_char(ch, "Something.\r\n");
	found = TRUE;
      }
    }
  }
  if (!found)
    send_to_char(ch, " Nothing.\r\n");
}


ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  /* day in [1..35] */
  day = time_info.day + 1;

  /* 35 days in a month, 7 days a week */
  weekday = ((35 * time_info.month) + day) % 7;

  send_to_char(ch, "It is %d o'clock %s, on %s.\r\n",
	  (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
	  time_info.hours >= 12 ? "pm" : "am", weekdays[weekday]);

  /*
   * Peter Ajamian <peter@PAJAMIAN.DHS.ORG> supplied the following as a fix
   * for a bug introduced in the ordinal display that caused 11, 12, and 13
   * to be incorrectly displayed as 11st, 12nd, and 13rd.  Nate Winters
   * <wintersn@HOTMAIL.COM> had already submitted a fix, but it hard-coded a
   * limit on ordinal display which I want to avoid.	-dak
   */

  suf = "th";

  if (((day % 100) / 10) != 1) {
    switch (day % 10) {
    case 1:
      suf = "st";
      break;
    case 2:
      suf = "nd";
      break;
    case 3:
      suf = "rd";
      break;
    }
  }

  send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[time_info.month], time_info.year);
}


ACMD(do_weather)
{
  const char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"
  };

  if (OUTSIDE(ch))
    {
    send_to_char(ch, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due");
    if (GET_LEVEL(ch) >= LVL_DEITY)
      send_to_char(ch, "Pressure: %d (change: %d), Sky: %d (%s)\r\n",
                 weather_info.pressure,
                 weather_info.change,
                 weather_info.sky,
                 sky_look[weather_info.sky]);
    }
  else
    send_to_char(ch, "You have no feeling about the weather at all.\r\n");
}


ACMD(do_help)
{
  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_table) {
    send_to_char(ch, "No help available.\r\n");
    return;
  }

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      send_to_char(ch, "There is no help on that word.\r\n");
      mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s tried to get help on %s", GET_NAME(ch), argument);
      return;
    } else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen))) {
      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
	mid--;
      page_string(ch->desc, help_table[mid].entry, 0);
      return;
    } else {
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
    }
  }
}



#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-c classlist] [-d racelist] [-s] [-o] [-q] [-r] [-z]\r\n"

/* FIXME: This whole thing just needs rewritten. */
ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  char mode;
  int low = 0, high = LVL_IMPL, localwho = 0, questwho = 0;
  int showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0;
  int who_room = 0, showrace = 0;
  int godnumber = 0;
  const char *goddesc[] = {
    "*SAINT*",
    "CREATOR",
    "*LIEGE*",
    "*COUNT*",
    "*DEITY*",
    " *GOD* ",
    " *IMP* ",    
  };


  skip_spaces(&argument);
  strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  while (*buf) {
    char arg[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
    } else if (*arg == '-') {
      mode = *(arg + 1);       /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'z':
	localwho = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 's':
	short_list = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'q':
	questwho = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'r':
	who_room = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'c':
	half_chop(buf1, arg, buf);
	showclass = find_class_bitvector(arg);
	break;
      case 'd':
        half_chop(buf1, arg, buf);
        showrace = find_race_bitvector(arg);
        break;
      default:
	send_to_char(ch, "%s", WHO_FORMAT);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(ch, "%s", WHO_FORMAT);
      return;
    }
  }				/* end while (parser) */

  send_to_char(ch, "Currently on Sorin:\r\n----------------------\r\n");

  for (d = descriptor_list; d; d = d->next) {
    if (!IS_PLAYING(d))
      continue;

    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
      continue;
    if (localwho && world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone)
      continue;
    if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
    if (short_list) {
      send_to_char(ch, "%s[%2d %s ] %-12.12s%s%s",
	      (GET_LEVEL(tch) >= LVL_SAINT ? CCYEL(ch, C_SPR) : ""),
	      GET_LEVEL(tch), CLASS_ABBR(tch), GET_NAME(tch),
	      (GET_LEVEL(tch) >= LVL_SAINT ? CCNRM(ch, C_SPR) : ""),
	      ((!(++num_can_see % 4)) ? "\r\n" : ""));
    } else {
      if(GET_LEVEL(tch) < LVL_SAINT) {
        num_can_see++;
        send_to_char( ch, "[%2d %s ] ", GET_LEVEL(tch), CLASS_ABBR(tch) );
      }
      else
      {
        godnumber = GET_LEVEL(tch) - 41;
        num_can_see++;
        if(GET_LEVEL(tch) == LVL_SAINT)
	  send_to_char( ch, "[%s%s%s] ",
	    CCGRN(ch, C_SPR), goddesc[godnumber], CCNRM(ch, C_SPR) );
        else if(GET_LEVEL(tch) == LVL_BUILDER)
          send_to_char( ch, "[%s%s%s] ",
	    CCBYL(ch, C_SPR), goddesc[godnumber], CCNRM(ch, C_SPR) );
        else if(GET_LEVEL(tch) == LVL_LIEGE)
          send_to_char( ch, "[%s%s%s] ",
	    CCBGR(ch, C_SPR), goddesc[godnumber], CCNRM(ch, C_SPR) );
        else if(GET_LEVEL(tch) == LVL_COUNT)
          send_to_char( ch, "[%s%s%s] ",
	    CCBBL(ch, C_SPR), goddesc[godnumber], CCNRM(ch, C_SPR) );
        else if(GET_LEVEL(tch) == LVL_DEITY)  
          send_to_char( ch, "[%s%s%s] ",
	    CCBMG(ch, C_SPR), goddesc[godnumber], CCNRM(ch, C_SPR) );
        else if(GET_LEVEL(tch) == LVL_GOD)
          send_to_char( ch, "[%s%s%s] ",
	    CCBCN(ch, C_SPR), goddesc[godnumber], CCNRM(ch, C_SPR) );
        else if(GET_LEVEL(tch) == LVL_IMPL)
          send_to_char( ch, "[%s%s%s] ",
	    CCRED(ch, C_SPR), goddesc[godnumber], CCNRM(ch, C_SPR) );
      }

      if ( strcmp(get_clan_name(GET_CLAN(tch)), "Clanless") != 0 ) 
      {
        send_to_char( ch, "[%s%s%s]  ",
	    CCCYN(ch, C_SPR),
	    get_clan_name(GET_CLAN(tch)),
	    CCNRM(ch, C_SPR) );
      }

      send_to_char( ch, "%s",
	  GET_TITLE(tch) != NULL ? GET_TITLE(tch) : GET_NAME(tch) );

      if (GET_INVIS_LEV(tch))
        send_to_char(ch, " (invis%d)", GET_INVIS_LEV(tch));
      else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
	send_to_char(ch, " (invis)");

      if (PLR_FLAGGED(tch, PLR_MAILING))
	send_to_char(ch, " (mailing)");
      else if (PLR_FLAGGED(tch, PLR_WRITING))
	send_to_char(ch, " (writing)");

      if (d->olc)
	send_to_char(ch, " (OLC)");

      if (PRF_FLAGGED(tch, PRF_BUILDWALK))
	send_to_char(ch, " (Buildwalking)");
      if (PRF_FLAGGED(tch, PRF_AFK))
        send_to_char(ch, " (%sAFK%s)", CCBCN(ch, C_SPR), CCNRM(ch, C_SPR));
      if (PRF_FLAGGED(tch, PRF_DEAF))
	send_to_char(ch, " (deaf)");
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	send_to_char(ch, " (notell)");
      if (PRF_FLAGGED(tch, PRF_QUEST))
	send_to_char(ch, " (Quest)");

      //dan clan system      
      //      if (GET_CLAN(tch) > CLAN_NONE)
      //         send_to_char(ch, "[%s%s%s]", CCCYN(ch, C_SPR), get_clan_name(GET_CLAN(tch)), CCNRM(ch, C_SPR));      

      if (PLR_FLAGGED(tch, PLR_THIEF))
	send_to_char(ch, " (%sTHIEF%s)", CCBGR(ch, C_SPR), CCNRM(ch, C_SPR));
      if (PLR_FLAGGED(tch, PLR_KILLER))
	send_to_char(ch, " (%sKILLER%s)", CCBRD(ch, C_SPR), CCNRM(ch, C_SPR));
      if (GET_LEVEL(tch) >= LVL_SAINT)
	send_to_char(ch, CCNRM(ch, C_SPR));

      send_to_char(ch, "\r\n");
    }				/* endif shortlist */
  }				/* end of for */
  if (short_list && (num_can_see % 4))
    send_to_char(ch, "\r\n");

  if (num_can_see == 0)
    send_to_char(ch, "\r\nNobody at all!\r\n");
  else if (num_can_see == 1)
    send_to_char(ch, "\r\nOne lonely character displayed.\r\n");
  else
    send_to_char(ch, "\r\n%d characters displayed.\r\n", num_can_see);
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-r racelist] [-o] [-p]\r\n"

/* BIG OL' FIXME: Rewrite it all. Similar to do_who(). */
ACMD(do_users)
{
  char line[200], line2[220], idletime[10], classname[20];
  char state[30], *timeptr, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  int low = 0, high = LVL_IMPL, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0, showrace = 0;
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
  while (*buf) {
    char buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      case 'c':
	playing = 1;
	half_chop(buf1, arg, buf);
	showclass = find_class_bitvector(arg);
	break;
      case 'r':
        playing = 1;
        half_chop(buf1, arg, buf);
        showrace = find_race_bitvector(arg);
        break;
      default:
	send_to_char(ch, "%s", USERS_FORMAT);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(ch, "%s", USERS_FORMAT);
      return;
    }
  }				/* end while (parser) */
  send_to_char(ch,
         "Num Class       Name         State          Idl Login@   Site\r\n"
         "--- ----------- ------------ -------------- --- -------- -----------------------\r\n");

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING && playing)
      continue;
    if (STATE(d) == CON_PLAYING && deadweight)
      continue;
    if (IS_PLAYING(d)) {
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (showrace && !(showrace & (1 << GET_RACE(tch))))
        continue;
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
	continue;

      if (d->original)
	sprintf(classname, "[%2d %s %s]", GET_LEVEL(d->original),
		CLASS_ABBR(d->original), RACE_ABBR(d->original));
      else
	sprintf(classname, "[%2d %s %s]", GET_LEVEL(d->character),
		CLASS_ABBR(d->character), RACE_ABBR(d->character));
    } else
      strcpy(classname, "   -   ");

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (STATE(d) == CON_PLAYING && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[STATE(d)]);

    if (d->character && STATE(d) == CON_PLAYING && GET_LEVEL(d->character) < LVL_DEITY)
      sprintf(idletime, "%3d", d->character->char_specials.timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    sprintf(line, "%3d %-7s %-12s %-14s %-3s %-8s ", d->desc_num, classname,
	d->original && d->original->player.name ? d->original->player.name :
	d->character && d->character->player.name ? d->character->player.name :
	"UNDEFINED",
	state, idletime, timeptr);

    if (d->host && *d->host)
       {
        if( (d->original ? GET_LEVEL(d->original) : d->character ? GET_LEVEL(d->character) : 1) >= LVL_GOD && GET_LEVEL(ch) != LVL_IMPL)
   	 strcat(line, "[Hostname Protected]\r\n");
	else
        sprintf(line + strlen(line), "[%s]\r\n", d->host);
       }
    else
      strcat(line, "[Hostname unknown]\r\n");

    if (STATE(d) != CON_PLAYING) {
      sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
      strcpy(line, line2);
    }
    if (STATE(d) != CON_PLAYING ||
		(STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
      send_to_char(ch, "%s", line);
      num_can_see++;
    }
  }

  send_to_char(ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char(ch, "\033[H\033[J");
    break;
  case SCMD_VERSION:
    send_to_char(ch, "%s\r\n", circlemud_version);
    send_to_char(ch, "%s\r\n", DG_SCRIPT_VERSION);
    break;
  case SCMD_WHOAMI:
    send_to_char(ch, "%s\r\n", GET_NAME(ch));
    break;
  default:
    log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
    return;
  }
}


void perform_mortal_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct descriptor_data *d;

  if (!*arg) {
    send_to_char(ch, "Players in your Zone\r\n--------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (IN_ROOM(i) == NOWHERE || !CAN_SEE(ch, i))
	continue;
      if (world[IN_ROOM(ch)].zone != world[IN_ROOM(i)].zone)
	continue;
      send_to_char(ch, "%-20s - %s\r\n", GET_NAME(i), world[IN_ROOM(i)].name);
    }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next) {
      if (IN_ROOM(i) == NOWHERE || i == ch)
	continue;
      if (!CAN_SEE(ch, i) || world[IN_ROOM(i)].zone != world[IN_ROOM(ch)].zone)
	continue;
      if (!isname(arg, i->player.name))
	continue;
      send_to_char(ch, "%-25s - %s\r\n", GET_NAME(i), world[IN_ROOM(i)].name);
      return;
    }
    send_to_char(ch, "Nobody around by that name.\r\n");
  }
}


void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
			        int recur)
{
  if (num > 0)
    send_to_char(ch, "O%3d. %-25s - ", num, obj->short_description);
  else
    send_to_char(ch, "%33s", " - ");

  if (IN_ROOM(obj) != NOWHERE)
    send_to_char(ch, "[%5d] %s\r\n", GET_ROOM_VNUM(IN_ROOM(obj)), world[IN_ROOM(obj)].name);
  else if (obj->carried_by)
    send_to_char(ch, "carried by %s\r\n", PERS(obj->carried_by, ch));
  else if (obj->worn_by)
    send_to_char(ch, "worn by %s\r\n", PERS(obj->worn_by, ch));
  else if (obj->in_obj) {
    send_to_char(ch, "inside %s%s\r\n", obj->in_obj->short_description, (recur ? ", which is" : " "));
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else
    send_to_char(ch, "in an unknown location\r\n");
}



void perform_immort_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;
  
  if (GET_LEVEL(ch) <= LVL_DEITY){
   mudlog(NRM, MAX(LVL_COUNT, GET_INVIS_LEV(ch)), TRUE, "(GC) %s used WHERE on %s", GET_NAME(ch), arg);
    }
  if (!*arg) {
    send_to_char(ch, "Players\r\n-------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (STATE(d) == CON_PLAYING || IS_EDITING(d)) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (IN_ROOM(i) != NOWHERE)) {
	  if (d->original)
	    send_to_char(ch, "%-20s - [%5d] %s (in %s)\r\n",
		GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(d->character)),
		world[IN_ROOM(d->character)].name, GET_NAME(d->character));
	  else
            if (!IS_EDITING(d))
	    send_to_char(ch, "%-20s - [%5d] %s\r\n", GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
            else
	    send_to_char(ch, "%-20s - [%5d] %s - In OLC - %s\r\n", GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name,                                    connected_types[STATE(d)]);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE && isname(arg, i->player.name)) {
	found = 1;
	send_to_char(ch, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i),
		GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char(ch, "Couldn't find any such thing.\r\n");
  }
}



ACMD(do_where)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_COUNT)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
  char buf[MAX_STRING_LENGTH];
  size_t i, len = 0;
  int nlen;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You ain't nothin' but a hound-dog.\r\n");
    return;
  }

  for (i = 1; i < LVL_SAINT; i++) {
    nlen = snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d-%-8d : ", i,
		level_exp(GET_CLASS(ch), i), level_exp(GET_CLASS(ch), i + 1) - 1);
    if (len + nlen >= sizeof(buf) || nlen < 0)
      break;
    len += nlen;

    switch (GET_SEX(ch)) {
    case SEX_MALE:
    case SEX_NEUTRAL:
      nlen = snprintf(buf + len, sizeof(buf) - len, "%s\r\n", title_male(GET_CLASS(ch), i));
      break;
    case SEX_FEMALE:
      nlen = snprintf(buf + len, sizeof(buf) - len, "%s\r\n", title_female(GET_CLASS(ch), i));
      break;
    default:
      nlen = snprintf(buf + len, sizeof(buf) - len, "Oh dear.  You seem to be sexless.\r\n");
      break;
    }
    if (len + nlen >= sizeof(buf) || nlen < 0)
      break;
    len += nlen;
  }

  if (len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d          : Immortality\r\n",
		LVL_SAINT, level_exp(GET_CLASS(ch), LVL_SAINT));
  page_string(ch->desc, buf, TRUE);
}



ACMD(do_consider)
{
 struct char_data *victim;
  int diff, chardam;
char buf[MAX_STRING_LENGTH];

  one_argument(argument, buf);

  if (!*buf) {
    send_to_char(ch, "Consider killing who?\r\n");
    return;
  }
  if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Consider killing who?\r\n");
    return;
  }
  if (victim == ch) {
    send_to_char(ch, "Easy!  Very easy indeed!\r\n");
    return;
  }
  if (GET_LEVEL(victim) >= LVL_SAINT && !IS_NPC(victim)) {
    send_to_char(ch, "Would you like to borrow a cross and a shovel?\r\n");
    return;
  }



  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  *buf = '\0';
strcat(buf, "\r\n");
    strcat(buf,                "Comparison           You      Enemy\r\n");
    strcat(buf,                "----------           ---      -----\r\n");

  if (diff <= -10)
    sprintf(buf + strlen(buf), "Level             :%5d      Muhahahaha ... what a wimp!\r\n", GET_LEVEL(ch));
  else if (diff <= -5)
    sprintf(buf + strlen(buf), "Level             :%5d      Greatly Inferior\r\n", GET_LEVEL(ch));
  else if (diff <= -2)  
    sprintf(buf + strlen(buf), "Level             :%5d      Extremely Inferior\r\n", GET_LEVEL(ch));
  else if (diff <= -1) 
    sprintf(buf + strlen(buf), "Level             :%5d      Inferior\r\n", GET_LEVEL(ch));
  else if (diff == 0)
    sprintf(buf + strlen(buf), "Level             :%5d      Lower\r\n", GET_LEVEL(ch));
  else if (diff <= 1)
    sprintf(buf + strlen(buf), "Level             :%5d      Equivalent\r\n", GET_LEVEL(ch));
  else if (diff <= 2)
    sprintf(buf + strlen(buf), "Level             :%5d      Higher\r\n", GET_LEVEL(ch));
  else if (diff <= 3)
    sprintf(buf + strlen(buf), "Level             :%5d      Superior\r\n", GET_LEVEL(ch));
  else if (diff <= 5)
    sprintf(buf + strlen(buf), "Level             :%5d      Extremely Superior\r\n", GET_LEVEL(ch));
  else if (diff <= 10)
    sprintf(buf + strlen(buf), "Level             :%5d      Greatly Superior\r\n", GET_LEVEL(ch));
  else
    sprintf(buf + strlen(buf), "Level             :%5d      Gulp ... maybe you should reconsider!\r\n", GET_LEVEL(ch));


        send_to_char(ch, buf);
  *buf = '\0';

  diff = (GET_HIT(victim) - GET_HIT(ch));
  if (diff <= -10)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Muhahahaha ... what a wimp!\r\n", GET_HIT(ch));
  else if (diff <= -5)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Greatly Inferior\r\n", GET_HIT(ch));
  else if (diff <= -2)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Extremely Inferior\r\n", GET_HIT(ch));
  else if (diff <= -1)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Inferior\r\n", GET_HIT(ch));
  else if (diff == 0)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Lower\r\n", GET_HIT(ch));
  else if (diff <= 1)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Equivalent\r\n", GET_HIT(ch));
  else if (diff <= 2)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Higher\r\n", GET_HIT(ch));
  else if (diff <= 3)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Superior\r\n", GET_HIT(ch));
  else if (diff <= 5)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Extremely Superior\r\n", GET_HIT(ch));
  else if (diff <= 10)
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Greatly Superior\r\n", GET_HIT(ch));
  else
    sprintf(buf + strlen(buf), "Hit Points        :%5d      Gulp ... maybe you should reconsider!\r\n", GET_HIT(ch));


        send_to_char(ch, buf);
  *buf = '\0';

  diff = (GET_DAMROLL(victim) - GET_DAMROLL(ch) );
  if (GET_EQ(ch, WEAR_WIELD))
     chardam = get_ave_dam(GET_EQ(ch, WEAR_WIELD)) + GET_DAMROLL(ch);
  else
     chardam = GET_DAMROLL(ch);
  if (diff <= -10)
    sprintf(buf + strlen(buf), "Damage            :%5d      Muhahahaha ... what a wimp!\r\n", chardam);
  else if (diff <= -5)
    sprintf(buf + strlen(buf), "Damage            :%5d      Greatly Inferior\r\n", chardam);
  else if (diff <= -2)
    sprintf(buf + strlen(buf), "Damage            :%5d      Extremely Inferior\r\n", chardam);
  else if (diff <= -1)
    sprintf(buf + strlen(buf), "Damage            :%5d      Inferior\r\n", chardam);
  else if (diff == 0)
    sprintf(buf + strlen(buf), "Damage            :%5d      Lower\r\n", chardam);
  else if (diff <= 1)
    sprintf(buf + strlen(buf), "Damage            :%5d      Equivalent\r\n", chardam);
  else if (diff <= 2)
    sprintf(buf + strlen(buf), "Damage            :%5d      Higher\r\n", chardam);
  else if (diff <= 3)
    sprintf(buf + strlen(buf), "Damage            :%5d      Superior\r\n", chardam);
  else if (diff <= 5)
    sprintf(buf + strlen(buf), "Damage            :%5d      Extremely Superior\r\n", chardam);
  else if (diff <= 10)
    sprintf(buf + strlen(buf), "Damage            :%5d      Greatly Superior\r\n", chardam);
  else
    sprintf(buf + strlen(buf), "Damage            :%5d      Gulp ... maybe you should reconsider!\r\n", chardam);


        send_to_char(ch, buf);
  *buf = '\0';

  diff = (GET_HITROLL(victim) * 7 - GET_HITROLL(ch) * 7);
  if (diff <= -10)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Muhahahaha ... what a wimp!\r\n", GET_HITROLL(ch) * 7);
  else if (diff <= -5)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Greatly Inferior\r\n", GET_HITROLL(ch) * 7);
  else if (diff <= -2)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Extremely Inferior\r\n", GET_HITROLL(ch) * 7);
  else if (diff <= -1)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Inferior\r\n", GET_HITROLL(ch) * 7);
  else if (diff == 0)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Lower\r\n", GET_HITROLL(ch) * 7);
  else if (diff <= 1)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Equivalent\r\n", GET_HITROLL(ch) * 7);
  else if (diff <= 2)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Higher\r\n", GET_HITROLL(ch) * 7);
  else if (diff <= 3)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Superior\r\n", GET_HITROLL(ch) * 7);
  else if (diff <= 5)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Extremely Superior\r\n", GET_HITROLL(ch) * 7);
  else if (diff <= 10)
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Greatly Superior\r\n", GET_HITROLL(ch) * 7);
  else
    sprintf(buf + strlen(buf), "Hit Ability       :%5d      Gulp ... maybe you should reconsider!\r\n", GET_HITROLL(ch) * 7);

        send_to_char(ch, buf);
  *buf = '\0';

  diff = (GET_EXP(victim) / 100000 - GET_EXP(ch) / 100000);
  if (diff <= -10)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Muhahahaha ... what a wimp!\r\n", GET_EXP(ch) / 100000);
  else if (diff <= -5)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Greatly Inferior\r\n", GET_EXP(ch) / 100000);
  else if (diff <= -2)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Extremely Inferior\r\n", GET_EXP(ch) / 100000);
  else if (diff <= -1)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Inferior\r\n", GET_EXP(ch) / 100000);
  else if (diff == 0)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Lower\r\n", GET_EXP(ch) / 100000);
  else if (diff <= 1)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Equivalent\r\n", GET_EXP(ch) / 100000);
  else if (diff <= 2)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Higher\r\n", GET_EXP(ch) / 100000);
  else if (diff <= 3)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Superior\r\n", GET_EXP(ch) / 100000);
  else if (diff <= 5)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Extremely Superior\r\n", GET_EXP(ch) / 100000);
  else if (diff <= 10)
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Greatly Superior\r\n", GET_EXP(ch) / 100000);
  else
    sprintf(buf + strlen(buf), "Attack Experience :%5d      Gulp ... maybe you should reconsider!\r\n", GET_EXP(ch) / 100000);

        send_to_char(ch, buf);
  *buf = '\0';

  diff = (GET_HIT(victim) + (GET_DAMROLL(victim) + (GET_HITROLL(victim)) - GET_HIT(ch) + GET_DAMROLL(ch) + (GET_HITROLL(ch))));

    sprintf(buf + strlen(buf), "\r\nEstimation of Success:\r\n");
  if (diff <= -10)
    sprintf(buf + strlen(buf), "Why do you even bother?\r\n");
  else if (diff <= -5)
    sprintf(buf + strlen(buf), "Go for it, it will be easy!\r\n");
  else if (diff <= -2)
    sprintf(buf + strlen(buf), "There should not be any problem winning.\r\n");
  else if (diff <= -1)
    sprintf(buf + strlen(buf), "It will be farily easy.\r\n");
  else if (diff == 0)
    sprintf(buf + strlen(buf), "This will be a fair fight!\r\n");
  else if (diff <= 1)
    sprintf(buf + strlen(buf), "Some luck would be needed!\r\n");
  else if (diff <= 2)
    sprintf(buf + strlen(buf), "Alot of luck will be needed!\r\n");
  else if (diff <= 3)
    sprintf(buf + strlen(buf), "I hope you have got a cleric!\r\n");
  else if (diff <= 5)
    sprintf(buf + strlen(buf), "Maybe you should reconsider?\r\n");
  else if (diff <= 10)
    sprintf(buf + strlen(buf), "Hold on! .. are you crazy?\r\n");
  else
    sprintf(buf + strlen(buf), "You ARE crazy, this is too much for you!\r\n");

        send_to_char(ch, buf);
  *buf = '\0';
}



ACMD(do_diagnose)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "%s", NOPERSON);
    else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char(ch, "Diagnose who?\r\n");
  }
}


const char *ctypes[] = {
  "off", "sparse", "normal", "complete", "\n"
};

ACMD(do_color)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char(ch, "Usage: color { Off | Sparse | Normal | Complete }\r\n");
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));

  send_to_char(ch, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR), CCNRM(ch, C_OFF), ctypes[tp]);
}


ACMD(do_toggle)
{
  char buf2[4];

  if (IS_NPC(ch))
    return;

  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");	/* strcpy: OK */
  else
    sprintf(buf2, "%-3.3d", GET_WIMP_LEV(ch));	/* sprintf: OK */

  if (GET_LEVEL(ch) >= LVL_SAINT) {
    send_to_char(ch,
          "      Buildwalk: %-3s    "
          "Clear Screen in OLC: %-3s\r\n",
        ONOFF(PRF_FLAGGED(ch, PRF_BUILDWALK)),
        ONOFF(PRF_FLAGGED(ch, PRF_CLS))
    );

    send_to_char(ch,
	  "      No Hassle: %-3s    "
	  "      Holylight: %-3s    "
	  "     Room Flags: %-3s\r\n"

//clan system - dan
          "  Clan Channels: %-3s\r\n",
	ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
	ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
	ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)),
        ONOFF(PRF_FLAGGED(ch, PRF_ALLCTELL))
    );
  }

  send_to_char(ch,
          "Exp Pnt Display: %-3s    "
	  "Hit Pnt Display: %-3s    "
	  "     Brief Mode: %-3s    "
	  " Summon Protect: %-3s\r\n"

	  "   Move Display: %-3s    "
	  "   Compact Mode: %-3s    "
	  "       On Quest: %-3s\r\n"

	  "   Mana Display: %-3s    "
	  "         NoTell: %-3s    "
	  "   Repeat Comm.: %-3s\r\n"

	  " Auto Show Exit: %-3s    "
	  "           Deaf: %-3s    "
	  "     Wimp Level: %-3s\r\n"

	  " Gossip Channel: %-3s    "
	  "Auction Channel: %-3s    "
	  "  Grats Channel: %-3s\r\n"

//dan clan system
          "   Clan Channel: %-3s    "

	  "    Color Level: %s\r\n",
          ONOFF(PRF_FLAGGED(ch, PRF_DISPEXP)),
	  ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
	  ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
	  ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
	  YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
	  ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
	  YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
	  YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
	  buf2,

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),
         
//dan clan system
          ONOFF(PRF_FLAGGED(ch, PRF_CLANTALK)),
	 
           ctypes[COLOR_LEV(ch)]);
}


int sort_commands_helper(const void *a, const void *b)
{
  return strcmp(cmd_info[*(const int *)a].command, cmd_info[*(const int *)b].command);
}


void sort_commands(void)
{
  int a, num_of_cmds = 0;

  while (cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++;	/* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}


ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) || IS_NPC(vict)) {
      send_to_char(ch, "Who is that?\r\n");
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char(ch, "You can't see the commands of people above your level.\r\n");
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  send_to_char(ch, "The following %s%s are available to %s:\r\n",
	  wizhelp ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) {
    i = cmd_sort_info[cmd_num];

    if (cmd_info[i].minimum_level < 0 || GET_LEVEL(vict) < cmd_info[i].minimum_level)
      continue;

    if ((cmd_info[i].minimum_level >= LVL_SAINT) != wizhelp)
      continue;

    if (!wizhelp && socials != (cmd_info[i].command_pointer == do_action || cmd_info[i].command_pointer == do_insult))
      continue;

    send_to_char(ch, "%-11s%s", cmd_info[i].command, no++ % 7 == 0 ? "\r\n" : "");
  }

  if (no % 7 != 1)
    send_to_char(ch, "\r\n");
}

ACMD(do_prereq)
{
 /* 	The folowing buffer has to be able to hold all the skills with prereqs for each class
  * 	Increase/decrease as you want I personally use georges buffer patch
  */
 
int chclass, skill_num;

char buf[MAX_INPUT_LENGTH];  	
  skip_spaces(&argument);

  
  if (!*argument){
     send_to_char(ch, "Syntax: prereq <skillname>\r\n");
     chclass = GET_CLASS(ch);
     return ; }
  else
	chclass = parse_class(*argument);

if ((skill_num = find_skill_num(argument))!=TYPE_UNDEFINED){
    chclass = GET_CLASS(ch);    
    sprintf(buf, "Pre-requisites for %s :-\r\n", 
	    	spell_info[skill_num].name);
	send_to_char(ch, buf);
	    
    if (spell_info[skill_num].first_prereq[chclass]!= TYPE_UNDEFINED){
	    sprintf(buf, "    %s", 
	    	spell_info[spell_info[skill_num].first_prereq[chclass]].name);
	    if (spell_info[skill_num].second_prereq[chclass]!= TYPE_UNDEFINED){
		    sprintf(buf+strlen(buf), "   %s", 
	    		spell_info[spell_info[skill_num].second_prereq[chclass]].name);
	    	}
	    send_to_char(ch, buf);
    }else{
        send_to_char(ch, "     None.");
    }   
	send_to_char(ch, "\r\n");
    }else{
  		/* if they get here not a skill or a class*/
  		send_to_char(ch, "Syntax: prereq <skillname>\r\n");
  }
}    

ACMD(do_attributes)
{
  struct affected_type *aff;
  char buf[MAX_INPUT_LENGTH]; 
  char buf2[MAX_INPUT_LENGTH];
  char buf3[MAX_INPUT_LENGTH];
  int found = FALSE;
  struct follow_type *fol;

  if (IS_NPC(ch))
    return;


  sprinttype(GET_SEX(ch), genders, buf, sizeof(buf));
  snprintf(buf3, sizeof(buf3), "%c%s", UPPER(*buf), buf + 1);
  sprinttype(GET_RACE(ch), pc_race_types, buf2, sizeof(buf2));
  sprinttype(GET_SIZE(ch), sizes, buf3, sizeof(buf3));
  send_to_char(ch, "Sex: %s, Age: %d, Race: %s, Size: %s\r\n",
   buf, GET_AGE(ch), buf2, buf3);
  sprinttype(ch->player.chclass, IS_NPC(ch) ? mob_race_types : pc_class_types, buf, sizeof(buf));
  send_to_char(ch, "Class: %s, Level: [%2d], XP: [%7d], Align: [%4d]\r\n",
    buf, GET_LEVEL(ch), GET_EXP(ch), GET_ALIGNMENT(ch) );
  send_to_char(ch, "Hunger: [%2d]  Thirst: [%2d]  Drunk: [%2d]\r\n", GET_COND(ch, FULL), GET_COND(ch, THIRST), GET_COND(ch, DRUNK));
  send_to_char(ch, "AC: [%d(%+d)], Hitroll: [%2d], Damroll: [%2d]\r\n",
	  GET_AC(ch), dex_app[GET_DEX(ch)].defensive, ch->points.hitroll + str_app[STRENGTH_APPLY_INDEX(ch)].tohit,
	  ch->points.damroll + str_app[STRENGTH_APPLY_INDEX(ch)].todam);
  send_to_char(ch, "Attacks: %d\r\n", get_num_attacks(ch));
  send_to_char(ch, "Abilities: Str[%d/%d]  Int[%d]  Wis[%d]  "
	  "Dex[%d]  Con[%d]  Cha[%d]\r\n",
	   GET_STR(ch), GET_ADD(ch), GET_INT(ch), GET_WIS(ch), GET_DEX(ch), 
	   GET_CON(ch), GET_CHA(ch) );
	send_to_char(ch, "Saving Throws: Para[%d]  Rod[%d]  Petri[%d]  Breath[%d]  Spell[%d]\r\n",
	   saving_throws_nat(GET_CLASS(ch), SAVING_PARA, GET_LEVEL(ch)) + GET_SAVE(ch, 0),
	   saving_throws_nat(GET_CLASS(ch), SAVING_ROD, GET_LEVEL(ch))+ GET_SAVE(ch, 1), 
	   saving_throws_nat(GET_CLASS(ch), SAVING_PETRI, GET_LEVEL(ch)) + GET_SAVE(ch, 2),
	   saving_throws_nat(GET_CLASS(ch), SAVING_BREATH, GET_LEVEL(ch)) + GET_SAVE(ch, 3), 
	   saving_throws_nat(GET_CLASS(ch), SAVING_SPELL, GET_LEVEL(ch)) + GET_SAVE(ch, 4));
    
    if (ch->master)
    send_to_char(ch, "You are following:  %s.\r\n", GET_NAME(ch->master));

	if (ch->followers)
	{
    send_to_char(ch, "You are being followed by:");
	for (fol = ch->followers; fol; fol = fol->next) {
      send_to_char(ch, "\r\n%s %s", found++ ? "" : "", PERS(fol->follower, ch));
      send_to_char(ch, "%s", fol->next ? "" : "");
      found = FALSE;
      }
    }
		
	send_to_char(ch, "\r\n----------------------------------------------------------------------\r\n");
	send_to_char(ch, "                             Affects\r\n");
	send_to_char(ch, "----------------------------------------------------------------------\r\n");
	/*for spells and affects*/
	if (ch->affected) {
    for (aff = ch->affected; aff; aff = aff->next) {
		{
		snprintf(buf, sizeof(buf), "%-21s ", skill_name(aff->type));
        act(buf, TRUE, ch, 0, 0, TO_CHAR);
		}
      if (aff->modifier)
		{
        snprintf(buf, sizeof(buf), "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
	    act(buf, TRUE, ch, 0, 0, TO_CHAR);
		}
	}
  }
  
  if (ch->affected2) {
    for (aff = ch->affected2; aff; aff = aff->next) {
		{
		snprintf(buf, sizeof(buf), "%-21s ", skill_name(aff->type));
        act(buf, TRUE, ch, 0, 0, TO_CHAR);
		}
      if (aff->modifier)
		{
        snprintf(buf, sizeof(buf), "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
	    act(buf, TRUE, ch, 0, 0, TO_CHAR);
		}
    }
  }
}

ACMD(do_scan)
{  
    struct char_data *i;
    int is_in, dir, dis, maxdis = 3, found = 0;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];   
    char buf[MAX_STRING_LENGTH];

    const char *distance[] = {
        "Right here",
        "Immediately ",
        "Far ",
        "Very far ",
        "Extremely far ",
        "Far in the distance ",
        "Very far in the distance ",
        "Extremely far in the distance ",
    };

    if (IS_AFFECTED(ch, AFF_BLIND))
    {
        act("You can't see anything, you're blind!", TRUE, ch, 0, 0, TO_CHAR);
        return;
    }  

    /*  
     * By level is dumb, but we'll keep it here just in case we change our
     * minds.
     *
    maxdis = ((GET_LEVEL(ch)/30)+3);
    if (GET_LEVEL(ch) >= LVL_SAINT)*/
 
    if (!GET_SKILL(ch, SKILL_DIST_SIGHT))
    {
        if (AFF_FLAGGED(ch, AFF_DIST_SIGHT))
            maxdis = 4; /* Wearing a DIST_SIGHT obj/having a DIST_SIGHT spell only gives you 1 extra room to see */
        else
            maxdis = 3; /* Default scan dist for people without the skill */
    }

    if (GET_SKILL(ch, SKILL_DIST_SIGHT))
    {
        if (GET_SKILL(ch, SKILL_DIST_SIGHT) <= 33)
            maxdis = 5;
        if ((GET_SKILL(ch, SKILL_DIST_SIGHT) >= 34 ) && (GET_SKILL(ch, SKILL_DIST_SIGHT) <=66 ))
            maxdis = 6;
        if (GET_SKILL(ch, SKILL_DIST_SIGHT) >= 67)
            maxdis = 7;
    }

    half_chop(argument, arg, arg2);

    if (!*arg)			/* "scan" alone, without an argument at all */
    {     
        act("You quickly scan the area and see:", TRUE, ch, 0, 0, TO_CHAR);
        act("$n quickly scans the area.", FALSE, ch, 0, 0, TO_ROOM);
        is_in = ch->in_room;
        for (dir = 0; dir < NUM_OF_DIRS; dir++)
	{
            ch->in_room = is_in;
    	    for (dis = 0; dis <= maxdis; dis++)
	    {
      		if (((dis == 0) && (dir == 0)) || (dis > 0))
		{
        	    for (i = world[ch->in_room].people; i; i = i->next_in_room)
		    {
          		if ((!((ch == i) && (dis == 0))) &&
				CAN_SEE(ch, i) && !AFF_FLAGGED(i, AFF_HIDE))
			{
            		    sprintf(buf, "%-35.35s: %s%s%s%s",
				GET_NAME(i),
				distance[dis],
                    		((dis > 0) && (dir < (NUM_OF_DIRS - 2))) ? "to the " : "",
                    		(dis > 0) ? dirs[dir] : "",
                    		((dis > 0) && (dir > (NUM_OF_DIRS - 3))) ? "wards" : "");
            		    act(buf, TRUE, ch, 0, 0, TO_CHAR);
            		    found++;
		  	}
        	    }
      		}

      		if (!CAN_GO(ch, dir) ||
			(world[ch->in_room].dir_option[dir]->to_room == is_in))
		{
      		    if (EXIT(ch, dir))
		    {
      			if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) &&
				(dis != maxdis))
			{
			    if (!EXIT_FLAGGED(EXIT(ch, dir), EX_HIDDEN))
			    {
      				if (dis != 0)
				{
		 		    sprintf(buf, "Something BLOCKS your view         : %s%s%s%s",
					distance[dis + 1],
                    			((dis > 0) && (dir < (NUM_OF_DIRS - 2))) ? "to the " : "",
                    			(dis > 0) ? dirs[dir] : "",
                    			((dis > 0) && (dir > (NUM_OF_DIRS - 3))) ? "wards" : "");
				}
	  			else
				{
	  			    sprintf(buf, "Something BLOCKS your view         : Immediately %s", dirs[dir]);
				}

	  			act(buf, TRUE, ch, 0, 0, TO_CHAR);
			    }
			}
		    }
	  	    break;
		}
	  	else
        	    ch->in_room = world[ch->in_room].dir_option[dir]->to_room;
     	    }
    	}
    }
    else 
    {     
        /* replaced algorithm; switch/case is sufficient for any set of
         * directions which do not duplicate the initial letter (such as
         * we have); this saves calling is_abbrev multiple times
         * CJW 12/23/05
         */
        switch(LOWER(*arg))
        {
        case 'n':
      	    dir = NORTH;
      	    act("You quickly scan the area to the north:", TRUE, ch, 0, 0, TO_CHAR);
      	    act("$n quickly scans to the north.", FALSE, ch, 0, 0, TO_ROOM);
            break;
        case 'e':
      	    dir = EAST;
      	    act("You quickly scan the area to the east:", TRUE, ch, 0, 0, TO_CHAR);
      	    act("$n quickly scans to the east.", FALSE, ch, 0, 0, TO_ROOM);
            break;
        case 's':
      	    dir = SOUTH;
      	    act("You quickly scan the area to the south:", TRUE, ch, 0, 0, TO_CHAR);
      	    act("$n quickly scans to the south.", FALSE, ch, 0, 0, TO_ROOM);
            break;
        case 'w':
      	    dir = WEST;
      	    act("You quickly scan the area to the west:", TRUE, ch, 0, 0, TO_CHAR);
      	    act("$n quickly scans to the west.", FALSE, ch, 0, 0, TO_ROOM);
            break;
        case 'u':
      	    dir = UP;
      	    act("You quickly scan the area above you:", TRUE, ch, 0, 0, TO_CHAR);
      	    act("$n quickly scans the area above.", FALSE, ch, 0, 0, TO_ROOM);
            break;
        case 'd':
      	    dir = DOWN;
      	    act("You quickly scan the area below you:", TRUE, ch, 0, 0, TO_CHAR);
      	    act("$n quickly scans the area below.", FALSE, ch, 0, 0, TO_ROOM);
            break;
     	default:
      	    act("Not a valid direction.", TRUE, ch, 0, 0, TO_CHAR);
      	    return;
            break;
      	}    
    
     	is_in = ch->in_room;     
     	ch->in_room = is_in;
     	for (dis = 0; dis <= maxdis; dis++)
	{
            if (((dis == 0) && (dir == 0)) || (dis > 0))
	    {
                for (i = world[ch->in_room].people; i; i = i->next_in_room)
	        {	
          	    if ((!((ch == i) && (dis == 0))) &&
			CAN_SEE(ch, i) && dis != 0 &&
			!AFF_FLAGGED(i, AFF_HIDE) )
		    {
              		sprintf(buf, "%-35.35s: %s%s%s%s",
				GET_NAME(i),
				distance[dis],
                     		((dis > 0) && (dir < (NUM_OF_DIRS - 2))) ? "to the " : "",
                      		(dis > 0) ? dirs[dir] : "",
                      		((dis > 0) && (dir > (NUM_OF_DIRS - 3))) ? "wards" : "");
             		act(buf, TRUE, ch, 0, 0, TO_CHAR);
              		found++;
		    }
          	}
            }        

      	    if (!CAN_GO(ch, dir) ||
		(world[ch->in_room].dir_option[dir]->to_room == is_in))
	    {
      		if (EXIT(ch, dir))
		{
      		    if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && (dis != maxdis))
		    {
      			if (!EXIT_FLAGGED(EXIT(ch, dir), EX_HIDDEN))
			{
      			    if (dis != 0)
			    {
		 		sprintf(buf, "Something BLOCKS your view         : %s%s%s%s",
					distance[dis + 1],
                    			((dis > 0) && (dir < (NUM_OF_DIRS - 2))) ? "to the " : "",
                    			(dis > 0) ? dirs[dir] : "",
                    			((dis > 0) && (dir > (NUM_OF_DIRS - 3))) ? "wards" : "");
			    }
	  		    else
			    {
	  			sprintf(buf, "Something BLOCKS your view         : Immediately %s", dirs[dir]);
			    }

	  		    act(buf, TRUE, ch, 0, 0, TO_CHAR);
			}
		    }
		}
	  	break;
	    }
            else
         	ch->in_room = world[ch->in_room].dir_option[dir]->to_room;
    	}      
    }

    if (found == 0)
    	act("Nobody anywhere near you.", TRUE, ch, 0, 0, TO_CHAR);
    ch->in_room = is_in;
}

ACMD(do_zone)
{
  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, zonelist, 0);
    return;
  }
  if (!zone_info_table) {
    send_to_char(ch, "No zone info available.\r\n");
    return;
  }

  bot = 0;
  top = top_of_zonet;
  minlen = strlen(argument);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      send_to_char(ch, "There is no information on that zone.\r\n");
      return;
    } else if (!(chk = strn_cmp(argument, zone_info_table[mid].num, minlen))) {
      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, zone_info_table[mid - 1].num, minlen))))
	mid--;
      page_string(ch->desc, zone_info_table[mid].desc, 0);
      return;
    } else {
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
    }
  }
}


