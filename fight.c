/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct message_list fight_messages[MAX_MESSAGES];
extern int pk_allowed;		/* see config.c */
extern int max_exp_gain;	/* see config.c */
extern int max_exp_loss;	/* see config.c */
extern int max_npc_corpse_time, max_pc_corpse_time;
extern int immort_level_ok; /* see config.c */

/* External procedures */
void check_dump(struct char_data *ch, int drop);
char *fread_action(FILE *fl, int nr);
ACMD(do_flee);
int backstab_mult(int level);
int thaco(int ch_class, int level);
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
bool mob_magic_user(struct char_data *ch);
void clanlog(struct char_data *ch, const char *str, ...);
void fix_size(struct char_data *ch);
int level_exp(int chclass, int level);

/* local functions */
void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type);
void appear(struct char_data *ch);
void load_messages(void);
void free_messages(void);
void free_messages_type(struct msg_type *msg);
void check_killer(struct char_data *ch, struct char_data *vict);
void make_corpse(struct char_data *ch, struct char_data *killer);
void change_alignment(struct char_data *ch, struct char_data *victim);
void death_cry(struct char_data *ch);
void raw_kill(struct char_data * ch, struct char_data * killer);
void die(struct char_data * ch, struct char_data * killer);
void group_gain(struct char_data *ch, struct char_data *victim);
void solo_gain(struct char_data *ch, struct char_data *victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
int compute_armor_class(struct char_data *ch);
int compute_thaco(struct char_data *ch, struct char_data *vict);
int get_num_attacks(struct char_data *ch);
int get_weapon_prof_to_hit(struct char_data *ch);
int get_weapon_prof_dam(struct char_data *ch);
int calc_weap_resists(struct char_data *ch, struct char_data *victim, int damage);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},		/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},	/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* TODO: may want to move this to a more generic library */
char *replace_color(struct char_data *ch, const char *str);

/* The Fight related routines */

void appear(struct char_data *ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);
  if (affected_by_spell(ch, SKILL_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);  
  if (affected_by_spell(ch, SKILL_HIDE))
    affect_from_char(ch, SKILL_HIDE);

  if (GET_LEVEL(ch) < LVL_SAINT)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
 /* else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, TO_ROOM);       zapped mak 5.10.04 */
}


int compute_armor_class(struct char_data *ch)
{
  int armorclass = GET_AC(ch);

  if (AWAKE(ch))
    armorclass -= dex_app[GET_DEX(ch)].defensive;

  return (MIN(armorclass, 100));      /* 10 is the highest we can go*/
}


void free_messages_type(struct msg_type *msg)
{
  if (msg->attacker_msg)	free(msg->attacker_msg);
  if (msg->victim_msg)		free(msg->victim_msg);
  if (msg->room_msg)		free(msg->room_msg);
}


void free_messages(void)
{
  int i;

  for (i = 0; i < MAX_MESSAGES; i++)
    while (fight_messages[i].msg) {
      struct message_type *former = fight_messages[i].msg;

      free_messages_type(&former->die_msg);
      free_messages_type(&former->miss_msg);
      free_messages_type(&former->hit_msg);
      free_messages_type(&former->god_msg);

      fight_messages[i].msg = fight_messages[i].msg->next;
      free(former);
    }
}


void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }

  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = NULL;
  }

  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}


void update_pos(struct char_data *victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}


void check_killer(struct char_data *ch, struct char_data *vict)
{
  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;

  SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
  send_to_char(ch, "If you want to be a PLAYER KILLER, so be it...\r\n");
  mudlog(BRF, LVL_SAINT, TRUE, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name);
  clanlog(ch, "PC Killer bit set on %s for initiating attack on %s at %s.", GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name);
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

   /* If either party is invisible, he becomes visible */
  if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
    appear(ch);
    
  FIGHTING(ch) = vict;
  if (!AFF_FLAGGED(ch, AFF_HEALING_DREAM))
  {
      GET_POS(ch) = POS_FIGHTING;
  }

  if (!pk_allowed)
    check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
  DAMBACK(ch) = 0;
}



void make_corpse(struct char_data *ch, struct char_data *killer)
{
  char buf2[MAX_NAME_LENGTH + 64];
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  IN_ROOM(corpse) = NOWHERE;
  corpse->name = strdup("corpse");

  snprintf(buf2, sizeof(buf2), "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "the corpse of %s", GET_NAME(ch));
  corpse->short_description = strdup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
  else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i)) {
      remove_otrigger(GET_EQ(ch, i), ch);
      obj_to_obj(unequip_char(ch, i), corpse);
    }

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {

  /* Fix the PCs size first if it needs fixing */
  fix_size(ch);

    /*
     * following 'if' clause added to fix gold duplication loophole
     * The above line apparently refers to the old "partially log in,
     * kill the game character, then finish login sequence" duping
     * bug. The duplication has been fixed (knock on wood) but the
     * test below shall live on, for a while. -gg 3/3/2002
     */
    if (IS_NPC(ch) || ch->desc) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;
  
  if(killer != NULL) {
    if(!IS_NPC(ch) && !IS_NPC(killer) )
      corpse->killer = GET_IDNUM(killer);
    corpse->corpse_id = GET_IDNUM(ch);
  }
  obj_to_room(corpse, IN_ROOM(ch));

}


/* When ch kills victim */
void change_alignment(struct char_data *ch, struct char_data *victim)
{
  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}



void death_cry(struct char_data *ch)
{
  int door;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (CAN_GO(ch, door))
      send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room, "Your blood freezes as you hear someone's death cry.\r\n");
}



void raw_kill(struct char_data * ch, struct char_data * killer)
{
  if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  /* To make ordinary commands work in scripts.  welcor*/  
  GET_POS(ch) = POS_STANDING; 
  
  if (killer) {
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  } else

  death_cry(ch); 
  update_pos(ch);
  make_corpse(ch, killer);
  check_dump(ch, 0); 
  extract_char(ch);
}



void die(struct char_data * ch, struct char_data * killer)
{
  gain_exp(ch, -(GET_EXP(ch) / 2));
  if (!IS_NPC(ch))
    REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
  raw_kill(ch, killer);
}



void perform_group_gain(struct char_data *ch, int base,
			     struct char_data *victim)
{
  if (((GET_LEVEL(ch) == (LVL_SAINT - 1)) && (immort_level_ok == 1)) || (GET_LEVEL(ch) >= LVL_SAINT) || (PLR_FLAGGED(ch, PLR_NOEXPGAIN)))
	{
	 send_to_char(ch, "You learn nothing from this kill.\r\n");
	 return;
	}
 
  int share;
  share = base;

  if (share > 1)
    send_to_char(ch, "You receive your share of experience -- %d points.\r\n", share);
  else
    send_to_char(ch, "You receive your share of experience -- one measly little point!\r\n");
      
  gain_exp(ch, share);
  change_alignment(ch, victim);

}


void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_members, base, tot_gain;
  struct char_data *k;
  struct follow_type *f;
  int parts_of_exp = 0;
  int part_value = 0;
  int max_gain = 0;

  if (!(k = ch->master))
    k = ch;

  if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
    tot_members = 1;
  else
    tot_members = 0;

  /* round up to the next highest tot_members */
  tot_gain = GET_EXP(victim);

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(max_exp_loss * 2 / 3, tot_gain);

    base = tot_gain ;

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) 
      parts_of_exp += GET_LEVEL(f->follower); 

  parts_of_exp += GET_LEVEL(k);
  part_value = base / parts_of_exp;
  
    if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch)) {
      tot_gain = GET_LEVEL(k) * part_value;
      max_gain = level_exp(GET_CLASS(k), GET_LEVEL(k) +1 ) / 8;
      if (tot_gain > max_gain)
        tot_gain = max_gain;
      perform_group_gain(k, tot_gain, victim);
    }


  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
       tot_gain = GET_LEVEL(f->follower) * part_value;
      max_gain = level_exp(GET_CLASS(f->follower), GET_LEVEL(f->follower) +1 )  / 8;
       if (tot_gain > max_gain)
         tot_gain = max_gain;
       perform_group_gain(f->follower, tot_gain, victim);
     }


}


void solo_gain(struct char_data *ch, struct char_data *victim)
{
  if (((GET_LEVEL(ch) == (LVL_SAINT - 1)) && (immort_level_ok == 1)) || (GET_LEVEL(ch) >= LVL_SAINT) || (PLR_FLAGGED(ch, PLR_NOEXPGAIN)))
	{
	 send_to_char(ch, "You learn nothing from this kill.\r\n");
	 return;
	}

  int exp;
  int final_exp;
  exp = GET_EXP(victim);

  /* Calculate level-difference bonus */

  final_exp = (level_exp(GET_CLASS(ch), GET_LEVEL(ch) +1 ) / 8);
    if (exp > final_exp)
      exp = final_exp;


   //  exp += MAX(0, (MIN(8, (level_exp(GET_CLASS(ch), GET_LEVEL(ch)))) / 8));

  exp = MAX(exp, 1);

  if (exp > 1)
    send_to_char(ch, "You receive %d experience points.\r\n", exp);
  else
    send_to_char(ch, "You receive one lousy experience point.\r\n");

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


char *replace_color(struct char_data *ch, const char *str)
{
    static char buf[MAX_STRING_LENGTH];
    char *cp = buf;
    char *color;

    for( ; *str; str++)
    {
        if( *str == '@')
        {
	    switch( *(++str) )
	    {
	    case 'n':
		color = CCNRM(ch, C_CMP);
		for( ; *(color); *(cp++) = *(color++) );
		break;
	    case 'R':
		color = CCBRD(ch, C_CMP);
		for( ; *(color); *(cp++) = *(color++) );
		break;
	    case 'Y':
		color = CCBYL(ch, C_CMP);
		for( ; *(color); *(cp++) = *(color++) );
		break;
	    /* TODO: implement support for the rest of the colors */
	    default:
		*( cp++ ) = '@';
		break;
	    }
	}
	else
	    *(cp++) = *str;

        *cp = 0;
    }

    /* TODO: this is not a safe function in that it does no checking
     * to be sure it doesn't overrun MAX_STRING_LENGTH */
    return( buf );
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data *ch, struct char_data *victim,
		      int w_type)
{
  char *buf;
  int msgnum;

  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n tries to #w $N, but misses.",	/* 0: -1     */
      "You try to #w $N, but miss.",
      "$n tries to #w you, but misses."
    },
    
    {
      "$N seems impervious to $n's #w attempt.",	/* 1: 0     */
      "Your #w has no effect on $N.",
      "$n tries to #w you, but it has no effect."
    },

    {
      "$n tickles $N with $s #W.",	/* 2: 1..2  */
      "You @Ytickle@n $N with your #w.",
      "$n @Rtickles@n you with $s #W."
    },

    {
      "$n barely #W $N.",		/* 3: 3..4  */
      "You @Ybarely@n #w $N.",
      "$n @Rbarely@n #W you."
    },

    {
      "$n #W $N.",			/* 4: 5..6  */
      "You @Y#w@n $N.",
      "$n @R#W@n you."
    },

    {
      "$n #W $N hard.",			/* 5: 7..10  */
      "You #w $N @Yhard@n.",
      "$n #W you @Rhard@n."
    },

    {
      "$n #W $N very hard.",		/* 6: 11..15  */
      "You #w $N @Yvery hard@n.",
      "$n #W you @Rvery hard@n."
    },

    {
      "$n #W $N extremely hard.",	/* 7: 16..20  */
      "You #w $N @Yextremely hard@n.",
      "$n #W you @Rextremely hard@n."
    },

    {
      "$n mutilates $N with $s #w.",	/* 8: 21..25  */
      "You @Ymutilate@n $N with your #w.",
      "$n @Rmutilates@n you with $s #w."
    },

    {
      "$n massacres $N with $s #w.",	/* 9: 26..30 */
      "You @Ymassacre@n $N with your #w.",
      "$n @Rmassacres@n you with $s #w."
    },

    {
      "$n obliterates $N to tiny bits with $s #w.",	/* 10: 31..35*/
      "You @Yobliterate@n $N to tiny bits with your #w.",
      "$n @Robliterates@n you to tiny bits with $s #w."
    },

    {
     "$n annihilates $N to itsy-bitsy pieces with $s #w.",  /* 11: 36..40  */
     "You @Yannihilate@n $N to itsy-bitsy pieces with $s #w.",
     "$n @Rannihilates@n you to itsy-bitsy pieces with $s #w."
    },

    {
     "$n pulverizes $N to a pulp with $s vicious #w.",   /* 12: 41..50   */
     "You @Ypulverize@n $N to a pulp with your vicious #w.",
     "$n @Rpulverizes@n you to a pulp with $s vicious #w."
    },

    {
     "$n decimates $N with $s mighty #w!",	   /* 13: 51..60  */
     "You @Ydecimate@n $N with your mighty #w!",
     "$n @Rdecimates@n you with $s mighty #w!"
    },

    {
     "$n totally demolishes $N with a wicked #w!",	/* 14: 61..70  */
     "You @Ytotally demolish@n $N with a wicked #w!",
     "$n @Rtotally demolishes@n you with a wicked #w!"
    },

    {
     "$n completely devastates $N with $s ruthless #w!",  /* 15: 71..85   */
     "You @Ycompletely devastate@n $N with your ruthless #w!",
     "$n @Rcompletely devastates@n you with $s ruthless #w!"
    },

    {
     "$n utterly eradicates $N with $s fiendishly diabolical #w!",/* 16: 86..100   */
     "You @Yutterly eradicate@n $N with your fiendishly diabolical #w!",
     "$n @Rutterly eradicates@n you with $s fiendishly diabolical #w!"
    },

    {
     "$n calmly disembowels $N with $s deadly #w!",	/* 17: > 100   */
     "You @Ycalmly disembowel@n $N with your deadly #w!",
     "$n @Rcalmly disembowels@n you with $s deadly #w!"
    }
  };


  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == -1) 	msgnum = 0;	/* miss */
  else if (dam == 0)	msgnum = 1;	/* immune */
  else if (dam <= 2)    msgnum = 2;
  else if (dam <= 4)    msgnum = 3;
  else if (dam <= 6)    msgnum = 4;
  else if (dam <= 10)   msgnum = 5;
  else if (dam <= 15)   msgnum = 6;
  else if (dam <= 20)   msgnum = 7;
  else if (dam <= 25)   msgnum = 8;
  else if (dam <= 30)   msgnum = 9;
  else if (dam <= 35)   msgnum = 10;
  else if (dam <= 40)   msgnum = 11;
  else if (dam <= 50)   msgnum = 12;
  else if (dam <= 60)   msgnum = 13;
  else if (dam <= 70)   msgnum = 14;
  else if (dam <= 85)   msgnum = 15;
  else if (dam <= 100)  msgnum = 16;
  else			msgnum = 17;

  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* the color macros only work for PCs without causing SYSERRs in the logs */

  /* damage message to damager */
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  if( !IS_NPC(ch) )
    buf = replace_color( ch, buf );
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);

  /* damage message to damagee */
  buf = replace_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  if( !IS_NPC(victim) )
    buf = replace_color( victim, buf );
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
}


/*
 *  message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;
 
  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_SAINT)) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
        /*
         * Don't send redundant color codes for TYPE_SUFFERING & other types
         * of damage without attacker_msg.
         */
	

	if (GET_POS(vict) == POS_DEAD) {
          if (msg->die_msg.attacker_msg) {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

	  send_to_char(vict, CCRED(vict, C_CMP));
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
          if (msg->hit_msg.attacker_msg) {
            if (!IS_NPC(ch)) {
                send_to_char(ch, CCYEL(ch, C_CMP));
                act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                send_to_char(ch, CCNRM(ch, C_CMP)); }
          }

	  if (!IS_NPC(vict)) {
	  send_to_char(vict, CCRED(vict, C_CMP));
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));
				}

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
        if (msg->miss_msg.attacker_msg) {
	  send_to_char(ch, CCYEL(ch, C_CMP));
	  act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(ch, CCNRM(ch, C_CMP));
        }

	send_to_char(vict, CCRED(vict, C_CMP));
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	send_to_char(vict, CCNRM(vict, C_CMP));

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return (1);
    }
  }
  return (0);
}


void backstab_message(int dam, struct char_data *ch, struct char_data *vict)
{

/* 
 * Wanted to put in multiple BS messages and this is a lot easier than with /mud/lib/misc/messages
 * Changed Backstab mults in class.c as well.  1-10x
 *
 * 1st Message = TO_CHAR = To Backstabber
 * 2nd Message = TO_VICT | TO_SLEEP = To Victim
 * 3rd Message = TO_NOTVICT = To everyone in the from BUT Victim
 *
 */


struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);
char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
int missmsg;


if (GET_POS(vict) == POS_DEAD) { /* You killed $N with your backstab */
snprintf(buf, sizeof(buf), "%sThe only thing $N sees as you approach is the glimmer of your drawn weapon ... but it is too late for $M, you have already struck and left nothing behind but a corpse.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou catch the glimmer of a drawn weapon out of the corner of your eye ... but it is too late for you, $n has already struck and you see your life flash before your eyes.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "The only evidence of $n's approach is the glimmer of $s drawn weapon ... but it is too late for $N, $E has already been struck down, the expression on $S face changes quickly from surprise to lifelessness as $S corpse falls to the ground");
}
else { /* The person you backstabbed didn't die .. so what happened ? */
if (dam == 0) { /* You backstabbed someone who is IMM pierce */
if (!weap) {
snprintf(buf, sizeof(buf), "%sAs you attempt to backstab $N, an unseen force diverts your hand leaving $M unscathed.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sAs $n attempts to backstab you, an unseen force diverts $s hand leaving you unscathed.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "As $n attempts to backstab $N, an unseen force diverts $s hand leaving $M unscathed.");
}
else {
snprintf(buf, sizeof(buf), "%sAs you plunge $p into $N, an unseen force diverts your hand leaving $M unscathed.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sAs $n plunges $p into you, an unseen force diverts $s hand leaving you unscathed.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "As $n plunges $p into $N, an unseen force diverts $s hand leaving $M unscathed.");
}
}
else if (dam == -1) { /* You missed! */
missmsg = rand_number(1,4);
switch (missmsg)
{
case 1:
snprintf(buf, sizeof(buf), "%sYou trip as you approach $N, and miss $M entirely.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou sidestep as you notice $N trip behind you, who then falls flat on $s face.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "$n trips as $e approaches $N, missing $M entirely.");
break;
case 2:
snprintf(buf, sizeof(buf), "%s$N avoids your backstab, and you fall flat on your face.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou avoid $n's backstab, who falls flat on $s face.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "$N avoids $n's backstab, who falls flat on $s face.");
break;
case 3:
snprintf(buf, sizeof(buf), "%s$N looks annoyed at your pitiful attempt at backstabbing $M.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou are annoyed at $n's pitiful attempt at backstabbing you.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "$N looks annoyed at $n's pitiful attempt at backstabbing $M.");
break;
case 4:
snprintf(buf, sizeof(buf), "%sYour backstab misses $N by a hair.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%s$n's backstab misses you by a hair.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "$n's backstab misses $N by a hair.");
break;
default:
   log("SYSERR: Error in backstab_message() in fight.c - Invalid miss msg.");
break;
}
}
else {
if (!weap) { /* You backstabbed with no weapon - you're a mob, aren't you? */
if (dam > 700) {
snprintf(buf, sizeof(buf), "%sWithout the slightest sound, you sneak up behind $N and tear a gaping hole in $s back.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou can feel a small trickle of blood dripping out of your mouth and down your chin and the presence of $n behind you is suddenly ... apparent.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "It is a gruesome scene as $n tears a gaping hole in $N's back with $s backstab.");
}
if (dam <=700) {
snprintf(buf, sizeof(buf), "%sWith quick and silent movements you appear behind $N, and surgically impale $M.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou cough up a lungful of blood as $n impales you with $s backstab.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "Nothing more than a gasp is audible as $n impales $N between the ribs.");
}
if (dam <=550) {
snprintf(buf, sizeof(buf), "%sYour double pierce leaves $N face down on the ground, bleeding profusely from $S wounds.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou are flung to the ground, where you lie bleeding from $n's double pierce.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "$N is flung to the ground, where $E lies bleeding from $n's double pierce.");
}
if (dam <=400) {
snprintf(buf, sizeof(buf), "%sYour wicked backstab draws large amounts of blood from $N.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sWith a wicked backstab, $n drains you of some 'unneeded' blood.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "With a wicked backstab, $n drains $N of some 'unneeded' blood.");
}
if (dam <=250) {
snprintf(buf, sizeof(buf), "%sWith a quick movement of your weapon you puncture $N.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou feel the force of $n's quick thrust as $e punctures you from behind.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "With a quick movement of $s weapon, $n punctures $N.");
}
if (dam <=100) {
snprintf(buf, sizeof(buf), "%sThe force of your backstab sends $N to $S knees.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sA sharp pain in your back, delivered by $n, sends you to your knees.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "$N falls to $S knees after the backstab by $n.");
}
if (dam <=25) {
snprintf(buf, sizeof(buf), "%s$N stumbles forward after your backstab.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou stumble forward from the force of $n's backstab.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "$N stumbles forward from the force of $n's backstab.");
}
}
else { /* You backstabbed someone with a weapon. */
if (dam > 700) {
snprintf(buf, sizeof(buf), "%sWithout the slightest sound, you sneak up behind $N and tear a gaping hole in $s back with $p.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou can feel a small trickle of blood dripping out of your mouth and down your chin when the presence of $n behind you is suddenly ... apparent.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "It is a gruesome scene as $n tears a gaping hole in $N's back with $s backstab.");
}
if (dam <=700) {
snprintf(buf, sizeof(buf), "%sWith quick and silent movements you appear behind $N, and surgically impale $M on $p.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou cough up a lung full of blood as $n impales you with $p.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "Nothing more than a gasp is audible as $n impales $N with $p.");
}
if (dam <=550) {
snprintf(buf, sizeof(buf), "%sYour double pierce leaves $N face down on the ground, bleeding profusely from $S wounds.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou are flung to the ground, where you lie bleeding from $n's double pierce.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "$N is flung to the ground, where $E lies bleeding from $n's double pierce.");
}
if (dam <=400) {
snprintf(buf, sizeof(buf), "%sYour wicked backstab draws large amounts of blood from $N.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sWith a wicked backstab, $n drains you of some 'unneeded' blood.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "With a wicked backstab, $n drains $N of some 'unneeded' blood.");
}
if (dam <=250) {
snprintf(buf, sizeof(buf), "%sWith a quick movement of $p, you puncture $N.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sWith a quick movement of $p, $n punctures you from behind.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "With a quick movement of $p, $n punctures $N from behind.");
}
if (dam <=100) {
snprintf(buf, sizeof(buf), "%sThe resulting force as you plunge $p into $N's back sends $M to $S knees.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sThe force as $n plunges $p into your back sends you to your knees.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "The resulting force as $n plunges $p into $N's back sends $M to $S knees.");
}
if (dam <=25) {
snprintf(buf, sizeof(buf), "%s$N stumbles forward as you thrust $p into $S back.%s", CCYEL(ch, C_CMP), CCNRM(ch, C_CMP));
snprintf(buf2, sizeof(buf2), "%sYou stumble forward as $n thrusts $p into your back.%s", CCRED(vict, C_CMP), CCNRM(vict, C_CMP));
snprintf(buf3, sizeof(buf3), "$N stumbles forward as $n thrusts $p into $S back.");
}
}
}
}
/* Send messages */
if (!IS_NPC(ch))
{
send_to_char(vict, CCYEL(vict, C_CMP));
send_to_char(vict, CCRED(vict, C_CMP));
send_to_char(vict, CCNRM(vict, C_CMP));
send_to_char(ch, CCYEL(ch, C_CMP));
send_to_char(ch, CCRED(ch, C_CMP));
send_to_char(ch, CCNRM(ch, C_CMP));
}

act(buf, FALSE, ch, weap, vict, TO_CHAR);
act(buf2, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
act(buf3, FALSE, ch, weap, vict, TO_NOTVICT);
}


/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{
  float wimp, maxhit, wimpval;
  char wimpbuf[MAX_INPUT_LENGTH];
  int wimpval2 = 0, total_duration = 0;
  struct affected_type *aff;

  if (GET_POS(victim) <= POS_DEAD) {
    /* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
    if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
      return (-1);

    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
		GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim, ch);
    return (-1);			/* -je, 7/7/92 */
  }

  /* peaceful rooms */
  if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
      ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return (0);
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    return (0);

  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_SAINT))
    dam = 0;

  if (victim != ch) {
    /* Start the attacker fighting the victim */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, victim);

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }
  }

  /* If you attack a pet, it hates your guts */
  if (victim->master == ch)
    stop_follower(victim);

 
  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >=2)
    dam /= 2;

  if (AFF_FLAGGED(ch, AFF_FURY))
   dam += ( dam / 3 ); /* Fury adds an additional 1/3 dam */

  /* Cut Damage by 25% if victim has fort */
  if ((AFF_FLAGGED(victim, AFF_FORT) || AFF_FLAGGED(victim, AFF_REFLECT_DAMAGE)) && (attacktype != TYPE_SUNDAM)){
    DAMBACK(victim) = DAMBACK(victim) + ((dam * .75) / 4);
    dam *= .75; }


  /* Cut Damage by 10% if victim has dervish spin - Mak */
  if (AFF_FLAGGED(victim, AFF_DERVISH_SPIN) && dam >=2)
    dam *= .9;

 // If victim is paralyzed damage will triple
  if (AFF_FLAGGED(victim, AFF_PARALYZE))
   dam *= 3;  

  /* Check for PK if this is not a PK MUD */
  if (!pk_allowed) {
    check_killer(ch, victim);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }

  if (dam != -1) {
  /* Set the maximum damage per round and subtract the hit points */
  dam = MAX(MIN(dam, 1800), 0);
  GET_HIT(victim) -= dam;

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   * 
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  }
  if (!IS_WEAPON(attacktype))
  {
    if (attacktype == SKILL_BACKSTAB)
      backstab_message(dam, ch, victim);
    else
      skill_message(dam, ch, victim, attacktype);
  }
  else {
      dam_message(dam, ch, victim, attacktype);
    }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are mortally wounded, and will die soon, if not aided.\r\n");
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are incapacitated and will slowly die, if not aided.\r\n");
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You're stunned, but will probably regain consciousness again.\r\n");
    break;
  case POS_DEAD:
    if (attacktype == TYPE_SUNDAM)
    {
      act("Without warning, $n screams out a death cry as $s body erupts into flames and falls into a heap of ashes.", TRUE, ch, 0, ch, TO_NOTVICT);
      send_to_char(ch, "With an awful smell, you kneel down as your body crumbles into a pile of smoke and ashes.\r\n");
    }
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are dead!  Sorry...\r\n");
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) / 4))
      send_to_char(victim, "That really did HURT!\r\n");

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
        if (!IS_NPC(victim) && !AFF_FLAGGED(victim, AFF_HEALING_DREAM)) { 
      send_to_char(victim, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
		CCRED(victim, C_SPR), CCNRM(victim, C_SPR)); }
      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))
	do_flee(victim, NULL, 0, 0);
    }

	/*
	 *  Doing the new wimpy by percentage, and I needed floats.. so I calculate it all here
         */
    if (!IS_NPC(victim))
	  {
	wimp = GET_WIMP_LEV(victim);
 	maxhit = GET_MAX_HIT(victim);
	wimpval = ((wimp / 100) * maxhit);
        snprintf(wimpbuf, sizeof(wimpbuf), "%f", wimpval);
	wimpval2 = atoi(wimpbuf);
	  }
	if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) && GET_HIT(victim) < wimpval2  && GET_HIT(victim) > 0 && !AFF_FLAGGED(victim, AFF_HEALING_DREAM)) {
      send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
      do_flee(victim, NULL, 0, 0);
    }
    break;
  }

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {
    do_flee(victim, NULL, 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = IN_ROOM(victim);
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  /* stop someone from fighting if they're stunned or worse */
  if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
    stop_fighting(victim);

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD) {

   if (AFF_FLAGGED(ch, AFF_FURY))
     { REMOVE_BIT(AFF_FLAGS(ch), AFF_FURY);
       send_to_char(ch, "Your muscles begin to quiver as your fury subsides.\r\n");
       act("$n's muscles begin to quiver as the fury fades from $s eyes.", FALSE, ch, 0, 0, TO_ROOM);
      }

    if (!IS_NPC(victim) )
    	ch->player_specials->last_kill = GET_IDNUM(victim);
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (AFF_FLAGGED(ch, AFF_GROUP))
	group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, LVL_SAINT, TRUE, "%s killed by %s at [%5d]", GET_NAME(victim), GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(victim)));
      //infochan("%s killed by %s.", GET_NAME(victim), GET_NAME(ch));
      clanlog(victim, "%s killed by %s.", GET_NAME(victim), GET_NAME(ch)); 
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }

    die(victim, ch);
    return (-1);
  }
  if (attacktype == TYPE_SUNDAM){
     if (GET_POS(victim) > POS_DEAD) {
          send_to_char(ch, "You SCREAM out in agony as your skin blisters from the searing sunlight!!\r\n");
          act("$n SCREAMS out in agony as $s skin blisters from the searing sunlight!!", TRUE, ch, 0, ch, TO_NOTVICT);
          }
   }

  return (dam);
}


/*
 * Calculate the THAC0 of the attacker.
 *
 * 'victim' currently isn't used but you could use it for special cases like
 * weapons that hit evil creatures easier or a weapon that always misses
 * attacking an animal.
 */
int compute_thaco(struct char_data *ch, struct char_data *victim)
{
  int calc_thaco;

  if (!IS_NPC(ch))
    calc_thaco = thaco(GET_CLASS(ch), GET_LEVEL(ch));
  else		/* THAC0 for monsters is set in the HitRoll */
    calc_thaco = 20;
  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit; 
  calc_thaco -= GET_HITROLL(ch);
  if(!IS_NPC(ch))  
    calc_thaco -= get_weapon_prof_to_hit(ch);

  return calc_thaco;
}


void hit(struct char_data *ch, struct char_data *victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
   int  level_diff, w_type, victim_ac, calc_thaco, dam, diceroll;
  
  /* check if the character has a fight trigger */
  fight_mtrigger(ch);

  /* Do some sanity checking, in case someone flees, etc. */
  if (victim == NULL) {
    stop_fighting(ch);       
    return;
  }
  else if (IN_ROOM(ch) != IN_ROOM(victim)) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  /* Find the weapon type (for display purposes only) */
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_PUNCH; /* unarmed attack changed to punch by Anubis */
  }

  /* Calculate chance of hit. Lower THAC0 is better for attacker. */
  calc_thaco = compute_thaco(ch, victim);

  /* Calculate the raw armor including magic armor.  Lower AC is better for defender. */
  victim_ac = compute_armor_class(victim);

  /* roll the die and take your chances... */
  diceroll = rand_number(1, 200);

  /*
   * Decide whether this is a hit or a miss.
   *
   *  Victim asleep = hit, otherwise:
   *     1   = Automatic miss.
   *   2..19 = Checked vs. AC.
   *    20   = Automatic hit.
   */
  if (diceroll == 20 || !AWAKE(victim))
    dam = TRUE;
  else if (diceroll == 1)
    dam = FALSE;
  else {      /* new to hit system  Anubis */
    if (calc_thaco - victim_ac <= diceroll) 
      dam = TRUE;   
    else 
      dam = FALSE; 
  if (affected_by_spell(ch, SPELL_ACCURACY))
     dam = TRUE; // Accuracy hits 100% of the time
  }
  
  if (!dam)
    /* the attacker missed the victim */
    damage(ch, victim, -1, type == SKILL_BACKSTAB ? SKILL_BACKSTAB : w_type);
  else {
    /* okay, we know the guy has been hit.  now calculate damage. */

    /* Start with the damage bonuses: the damroll and strength apply */
    dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
    dam += GET_DAMROLL(ch);

    //Anubis
    if(!IS_NPC(ch))
      dam += get_weapon_prof_dam(ch);    
 
    /* Maybe holding arrow? */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      /* Add weapon-based damage if a weapon is being wielded */
      dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
    } else {
      /* If no weapon, add bare hand damage instead */
      if ((GET_LEVEL(victim)<LVL_IMMORT) && AFF_FLAGGED(ch, AFF_PARALYZING_TOUCH) && !IS_NECROMANCER(victim) &&(GET_POS(victim) > POS_PARALYZED)) {
       struct affected_type af;
      level_diff = GET_LEVEL(ch) - GET_LEVEL(victim);
      if ((level_diff >9) && !MOB_FLAGGED(victim, MOB_NOPARALYZE) && !AFF_FLAGGED(victim, AFF_FREE_ACTION)) {
        af.duration = MAX(1, GET_LEVEL(ch)/10);
        af.bitvector = AFF_PARALYZE;
        af.type = SPELL_PARALYZE;
        GET_POS(victim) = POS_PARALYZED;
        affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
        act("You feel your limbs freeze!", FALSE, victim, 0, ch, TO_CHAR);
        act("$n seems paralyzed!", TRUE, victim, 0, ch, TO_ROOM);
      }
      else if (GET_POS(victim) > POS_STUNNED) {
        if (level_diff >= 0) {
          GET_STUN_RECOVER_CHANCE(victim) = 50;
          GET_STUN_DURATION(victim) = -1; //indefinatly
        }
        else {
          GET_STUN_RECOVER_CHANCE(victim) = 100;
          GET_STUN_DURATION(victim) = 2;
        }
        GET_POS(victim) = POS_STUNNED;
        act("You are stunned!", FALSE, victim, 0, ch, TO_CHAR);
        act("$n seems to be frozen!", TRUE, victim, 0, ch, TO_ROOM);
      }
    }
      if (IS_NPC(ch))
	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      else
	dam += rand_number(0, 2);	/* Max 2 bare hand damage for players */
    }

    /*
     * Include a damage multiplier if victim isn't ready to fight:
     *
     * Position sitting  1.33 x normal
     * Position resting  1.66 x normal
     * Position sleeping 2.00 x normal
     * Position stunned  2.33 x normal
     * Position incap    2.66 x normal
     * Position mortally 3.00 x normal
     *
     * Note, this is a hack because it depends on the particular
     * values of the POSITION_XXX constants.
     */
    if (GET_POS(victim) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

   /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);

    dam = calc_weap_resists(ch, victim, dam);

    /*  commented out cause it spams us with bugs - mak */

  /* This section was created to parse things that should only happen on a pc/npc's LAST attack of a round */
 /* if (type == TYPE_LASTATTACK) */
  {
  /*  Should there be a check to DISABLE crit strike if its a backstaber with 1 attack/round ?*/
 /* if (GET_SKILL(ch, SKILL_CRIT_HIT)) */
  {
  /*  finish this CRIT HIT action */
  }
  }

    if (type == SKILL_BACKSTAB)
      damage(ch, victim, dam * backstab_mult(GET_LEVEL(ch)), SKILL_BACKSTAB);
    else
      damage(ch, victim, dam, w_type);

  }

  /* check if the victim has a hitprcnt trigger */
  hitprcnt_mtrigger(victim);
}



/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;
  int number_of_attacks;
  int i; 

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(ch);
      continue;
    }

	number_of_attacks = get_num_attacks(ch);
    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
	GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
	continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
	GET_POS(ch) = POS_FIGHTING;
	act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      if (AFF_FLAGGED(ch, AFF_HEALING_DREAM))
	send_to_char(ch, "You are unable to fight while in healing dream!!\r\n");
      else
        send_to_char(ch, "You can't fight while sitting!!\r\n");
      continue;
    }

    for(i=0;i<number_of_attacks;i++)
    {
    if (i == (number_of_attacks - 1))
     hit(ch, FIGHTING(ch), TYPE_LASTATTACK);
    else
     hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    }

  if (AFF_FLAGGED(ch, AFF_FORT))
     damage(ch, FIGHTING(ch), DAMBACK(ch), SPELL_FORT);
 if (AFF_FLAGGED(ch, AFF_REFLECT_DAMAGE))
     damage(ch, FIGHTING(ch), DAMBACK(ch), SPELL_REFLECT_DAMAGE);

   
    /* combat class and race checks here   Anubis  
    if(!MOB_FLAGGED(ch, MOB_NOTDEADYET) )
    {
     mob_magic_user(ch); 
    }
    */
    
    
    if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
      char actbuf[MAX_INPUT_LENGTH] = "";
      (GET_MOB_SPEC(ch)) (ch, ch, 0, actbuf);

      }
  }
}

int get_num_attacks(struct char_data *ch)
{
   int num_attacks = 1;
   
   if(IS_NPC(ch) )
   {
    if(GET_MOB_ATTACKS(ch) > 0)
    num_attacks = GET_MOB_ATTACKS(ch);
    MIN( num_attacks, 10);
   }
   else if(IS_PRIEST(ch) || IS_MAGIC_USER(ch) || IS_ROGUE(ch) )
   {
    if(GET_LEVEL(ch) >= 1)
      num_attacks = 1;
   }
   else if(IS_FIGHTER(ch) )
   {
    if(GET_LEVEL(ch) >= 1)
      num_attacks = 1 ;
    if(GET_LEVEL(ch) >= 10)
      num_attacks = 2;    
  }
   else if(IS_SHAMAN(ch) || IS_CLERIC(ch) || IS_KNIGHT(ch) || IS_BATTLEMAGE(ch) || IS_ENCHANTER(ch) || IS_THIEF(ch) || IS_BARD(ch) )
   {
    if(GET_LEVEL(ch) >= 15)
      num_attacks = 2;
   }
   else if(IS_WARRIOR(ch) )
   {
    if(GET_LEVEL(ch) >= 15)
      num_attacks = 3;
    if(GET_LEVEL(ch) >= 20 )
      num_attacks = 4;
  }
   else if(IS_RANGER(ch) || IS_CHAOSMAGE(ch) || IS_BOUNTYHUNTER(ch) )
   {
    if(GET_LEVEL(ch) >= 25)
      num_attacks = 3;
    if(GET_LEVEL(ch) >= 35)
      num_attacks = 4;
   }
   else if(IS_DRUID(ch) || IS_DISCIPLE(ch) || IS_CRUSADER(ch) || IS_SORCEROR(ch) || IS_JESTER(ch)  || IS_BLADE(ch) )
   {
    if(GET_LEVEL(ch) >= 25)
      num_attacks = 3;
   }
   else if(IS_BARBARIAN(ch) )
   {
    if(GET_LEVEL(ch) >= 30)
      num_attacks = 5;
    if(GET_LEVEL(ch) >= 40)
      num_attacks = 6;
   }
   else if(IS_CHAOSMAGE(ch) || IS_ASSASSIN(ch) )
   {
    if(GET_LEVEL(ch) >= 25)
      num_attacks = 3;
    if(GET_LEVEL(ch) >= 35)
      num_attacks = 4;
   }
   else if(IS_BOUNTYHUNTER(ch) )
   {
    if(GET_LEVEL(ch) >= 40)
      num_attacks = 5;
   }
   else if(IS_NECROMANCER(ch) || IS_ALCHEMIST(ch) )
       num_attacks = 2;
   else if(IS_PALADIN(ch) || IS_SKNIGHT(ch) )
   {
    if(GET_LEVEL(ch) >= 25)
      num_attacks = 4;
    if(GET_LEVEL(ch) >= 40)
      num_attacks = 5;
   }
   
  num_attacks += GET_ATTACKS(ch);
  if (num_attacks < 1)
      num_attacks = 1;    
  return(num_attacks);
}

int get_weapon_prof_to_hit(struct char_data *ch)
  {
    struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
    int w_type;
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    {    
      w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;     
    switch (w_type) {
      case TYPE_STING:
      case TYPE_PIERCE:
      case TYPE_STAB:                   
        if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 1 )
            return(-5);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 10 )
            return(-2);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 20 )
            return(-1);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 30 )
            return(0);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 45 )
            return(1);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 60 )
            return(2);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 79 )
            return(3);
        else
          return(4); 
        break;
      case TYPE_WHIP:
        if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 1 )
            return(-5);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 10 )
            return(-2);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 20 )
            return(-1);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 30 )
            return(0);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 45 )
            return(1);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 60 )
            return(2);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 79 )
            return(3);
        else
            return(4); 
        break;
      case TYPE_BLUDGEON:  
      case TYPE_POUND:
      case TYPE_CRUSH:
        if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 1 )
            return(-5);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 10 )
            return(-2);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 20 )
            return(-1);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 30 )
            return(0);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 45 )
            return(1);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 60 )
            return(2);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 79 )
            return(3);
        else
          return(4); 
        break;
      case TYPE_SLASH:
      case TYPE_CLAW:
        if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 1 )
            return(-5);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 10 )
            return(-2);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 20 )
            return(-1);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 30 )
            return(0);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 45 )
            return(1);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 60 )
            return(2);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 79 )
            return(3);
        else
          return(4); 
        break;
      default:          
          return(0);
          break;  
       }  
     }
   else       
     return(0);
}

int get_weapon_prof_dam(struct char_data *ch)
  {
    struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
    int w_type;   
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    {    
      w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;     
    switch (w_type) {
      case TYPE_STING:
      case TYPE_PIERCE:
      case TYPE_STAB:                   
        if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 1 )
            return(-5);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 10 )
            return(-2);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 20 )
            return(-1);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 30 )
            return(0);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 45 )
            return(1);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 60 )
            return(2);
        else if(GET_SKILL(ch, SKILL_PROF_PIERCE) <= 79 )
            return(3);
        else
          return(4); 
        break;
      case TYPE_WHIP:
        if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 1 )
            return(-5);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 10 )
            return(-2);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 20 )
            return(-1);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 30 )
            return(0);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 45 )
            return(1);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 60 )
            return(2);
        else if(GET_SKILL(ch, SKILL_PROF_WHIP) <= 79 )
            return(3);
        else
            return(4); 
        break;
      case TYPE_BLUDGEON:  
      case TYPE_POUND:
      case TYPE_CRUSH:
        if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 1 )
            return(-5);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 10 )
            return(-2);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 20 )
            return(-1);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 30 )
            return(0);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 45 )
            return(1);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 60 )
            return(2);
        else if(GET_SKILL(ch, SKILL_PROF_BLUDGEON) <= 79 )
            return(3);
        else
          return(4); 
        break;
      case TYPE_SLASH:
      case TYPE_CLAW:
        if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 1 )
            return(-5);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 10 )
            return(-2);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 20 )
            return(-1);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 30 )
            return(0);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 45 )
            return(1);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 60 )
            return(2);
        else if(GET_SKILL(ch, SKILL_PROF_SLASH) <= 79 )
            return(3);
        else
          return(4); 
        break;
      default:          
          return(0);
          break;  
       }  
     }
   else       
     return(0);
}

int calc_weap_resists(struct char_data *ch, struct char_data *victim, int damage)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  int w_type;

    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
      w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
    else {
     if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
        w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_PUNCH; /* unarmed attack changed to punch by Anubis */
    }

    switch (w_type) {
      case TYPE_STING:
      case TYPE_PIERCE:
      case TYPE_STAB:
        if(victim->char_specials.resist[ATTACK_PIERCE] > 0 || victim->char_specials.resist[ATTACK_PHYSICAL] > 0 )
         damage = damage - (damage / 3);
        if(victim->char_specials.vulnerable[ATTACK_PIERCE] > 0 || victim->char_specials.vulnerable[ATTACK_PHYSICAL] > 0 )   
         damage = damage + (damage / 3);
        if(victim->char_specials.immune[ATTACK_PIERCE] > 0 || victim->char_specials.immune[ATTACK_PHYSICAL] > 0 )
         damage = 0;
        return(damage);
         break;
      case TYPE_WHIP:
        if(victim->char_specials.resist[ATTACK_WHIP] > 0 || victim->char_specials.resist[ATTACK_PHYSICAL] > 0 )
         damage = damage - (damage / 3);
        if(victim->char_specials.vulnerable[ATTACK_WHIP] > 0 || victim->char_specials.vulnerable[ATTACK_PHYSICAL] > 0 )   
         damage = damage + (damage / 3);
        if(victim->char_specials.immune[ATTACK_WHIP] > 0 || victim->char_specials.immune[ATTACK_PHYSICAL] > 0 )
         damage = 0;
        return(damage);
         break;
      case TYPE_BLUDGEON:
      case TYPE_POUND:
      case TYPE_CRUSH:
        if(victim->char_specials.resist[ATTACK_BLUDGEON] > 0 || victim->char_specials.resist[ATTACK_PHYSICAL] > 0 )
         damage = damage - (damage / 3);
        if(victim->char_specials.vulnerable[ATTACK_BLUDGEON] > 0 || victim->char_specials.vulnerable[ATTACK_PHYSICAL] > 0 )   
         damage = damage + (damage / 3);
        if(victim->char_specials.immune[ATTACK_BLUDGEON] > 0 || victim->char_specials.immune[ATTACK_PHYSICAL] > 0 )
         damage = 0;
        return(damage);
         break;
      case TYPE_SLASH:
      case TYPE_CLAW:
        if(victim->char_specials.resist[ATTACK_SLASH] > 0 || victim->char_specials.resist[ATTACK_PHYSICAL] > 0 )
         damage = damage - (damage / 3);
        if(victim->char_specials.vulnerable[ATTACK_SLASH] > 0 || victim->char_specials.vulnerable[ATTACK_PHYSICAL] > 0 )   
         damage = damage + (damage / 3);
        if(victim->char_specials.immune[ATTACK_SLASH] > 0 || victim->char_specials.immune[ATTACK_PHYSICAL] > 0 )
         damage = 0;
        return(damage);
         break;
      case TYPE_HIT:
      case TYPE_PUNCH:
      case TYPE_THRASH:
      case TYPE_BITE:
      case TYPE_MAUL:
      case TYPE_BLAST:
      case TYPE_KICK:
        if(victim->char_specials.resist[ATTACK_PHYSICAL] > 0 )
         damage = damage - (damage / 3);
        if(victim->char_specials.vulnerable[ATTACK_PHYSICAL] > 0 )
         damage = damage + (damage / 3);
        if(victim->char_specials.immune[ATTACK_PHYSICAL] > 0 )
         damage = 0;
        return(damage);
         break;
      default:
          return(damage);
          break;
       }

  return(damage);
}

/* dunno where to put this, so I'm going to try here can move it later - Seymour --gotta fix this */ 
void check_pain_room()
{
struct descriptor_data *d;
/* char_data *ch; */
int pain_dam;
 for (d = descriptor_list; d; d = d->next) {
    if (d->connected) continue;

   if (ROOM_FLAGGED(IN_ROOM(d->character), ROOM_PAIN)){

//   for (ch = world[IN_ROOM(d->character)].people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(d->character)) { 
       if (world[IN_ROOM(d->character)].pain_check >= world[IN_ROOM(d->character)].pain_rate){
          send_to_room(IN_ROOM(d->character), "%s\r\n", world[IN_ROOM(d->character)].pain_message);

       pain_dam = world[IN_ROOM(d->character)].pain_damage;
          if(AFF_FLAGGED(d->character, AFF_SANCTUARY) && pain_dam >= 2 )
             pain_dam *= .5;
          if (AFF_FLAGGED(d->character, AFF_FORT) && pain_dam >= 3)
             pain_dam *= .75;
          if (AFF_FLAGGED(d->character, AFF_REFLECT_DAMAGE) && pain_dam >= 3)
             pain_dam *= .75;

          if (AFF_FLAGGED(d->character, AFF_DERVISH_SPIN) && pain_dam >= 2)
             pain_dam *= .9;
          if (GET_LEVEL(d->character) >= LVL_SAINT)
             pain_dam = 0;
          GET_HIT(d->character) = GET_HIT(d->character) - pain_dam;
          world[IN_ROOM(d->character)].pain_check = 0;
          update_pos(d->character);
          if(GET_POS(d->character) == POS_DEAD)
             die(d->character, d->character);
          } else world[IN_ROOM(d->character)].pain_check++;
    }
   }

 }
}




