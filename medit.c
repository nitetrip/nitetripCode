/************************************************************************
 * OasisOLC - Mobiles / medit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genmob.h"
#include "genzon.h"
#include "genshp.h"
#include "oasis.h"
#include "handler.h"
#include "constants.h"
#include "improved-edit.h"
#include "dg_olc.h"
#include "screen.h"
#include "constants.h"

/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct char_data *character_list;
extern mob_rnum top_of_mobt;
extern struct zone_data *zone_table;
extern struct attack_hit_type attack_hit_text[];
extern struct shop_data *shop_index;
extern struct descriptor_data *descriptor_list;
extern const char *sizes[];
extern const char *mob_race_types[];
extern const char *mob_class_types[];
#if CONFIG_OASIS_MPROG
extern const char *mobprog_types[];
#endif

/*-------------------------------------------------------------------*/

/*
 * Handy internal macros.
 */
#if CONFIG_OASIS_MPROG
#define GET_MPROG(mob)		(mob_index[(mob)->nr].mobprogs)
#define GET_MPROG_TYPE(mob)	(mob_index[(mob)->nr].progtypes)
#endif

/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
#if CONFIG_OASIS_MPROG
void medit_disp_mprog(struct descriptor_data *d);
void medit_change_mprog(struct descriptor_data *d);
const char *medit_get_mprog_type(struct mob_prog_data *mprog);
#endif

void get_defaults(struct char_data *mob);
/*-------------------------------------------------------------------*\
  utility functions
\*-------------------------------------------------------------------*/

void medit_save_to_disk(zone_vnum foo)
{
  save_mobiles(real_zone(foo));
}

void medit_setup_new(struct descriptor_data *d)
{
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure.
   */
  CREATE(mob, struct char_data, 1);

  init_mobile(mob);

  GET_MOB_RNUM(mob) = NOBODY;
  /*
   * Set up some default strings.
   */
  GET_ALIAS(mob) = strdup("mob unfinished");
  GET_SDESC(mob) = strdup("the unfinished mob");
  GET_LDESC(mob) = strdup("An unfinished mob stands here.\r\n");
  GET_DDESC(mob) = strdup("It looks unfinished.\r\n");
  GET_SIZE(mob) = 3;
#if CONFIG_OASIS_MPROG
  OLC_MPROGL(d) = NULL;
  OLC_MPROG(d) = NULL;
#endif

  OLC_MOB(d) = mob;
  /* Has changed flag. (It hasn't so far, we just made it.) */
  OLC_VAL(d) = FALSE;
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;

  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num)
{
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure. 
   */
  CREATE(mob, struct char_data, 1);

  copy_mobile(mob, mob_proto + rmob_num);

#if CONFIG_OASIS_MPROG
  {
    MPROG_DATA *temp;
    MPROG_DATA *head;

    if (GET_MPROG(mob))
      CREATE(OLC_MPROGL(d), MPROG_DATA, 1);
    head = OLC_MPROGL(d);
    for (temp = GET_MPROG(mob); temp; temp = temp->next) {
      OLC_MPROGL(d)->type = temp->type;
      OLC_MPROGL(d)->arglist = strdup(temp->arglist);
      OLC_MPROGL(d)->comlist = strdup(temp->comlist);
      if (temp->next) {
        CREATE(OLC_MPROGL(d)->next, MPROG_DATA, 1);
        OLC_MPROGL(d) = OLC_MPROGL(d)->next;
      }
    }
    OLC_MPROGL(d) = head;
    OLC_MPROG(d) = OLC_MPROGL(d);
  }
#endif

  OLC_MOB(d) = mob;
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;
  dg_olc_script_copy(d);

  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Ideally, this function should be in db.c, but I'll put it here for
 * portability.
 */
void init_mobile(struct char_data *mob)
{
  clear_char(mob);

  GET_HIT(mob) = GET_MANA(mob) = 1;
  GET_MAX_MANA(mob) = GET_MAX_MOVE(mob) = 100;
  GET_NDD(mob) = GET_SDD(mob) = 1;
  GET_WEIGHT(mob) = 200;
  GET_HEIGHT(mob) = 198;

  mob->real_abils.str = mob->real_abils.intel = mob->real_abils.wis = 11;
  mob->real_abils.dex = mob->real_abils.con = mob->real_abils.cha = 11;
  mob->aff_abils = mob->real_abils;

  SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
  mob->player_specials = &dummy_mob;
}

/*-------------------------------------------------------------------*/

/*
 * Save new/edited mob to memory.
 */
void medit_save_internally(struct descriptor_data *d)
{
  int i;
  mob_rnum new_rnum;
  struct descriptor_data *dsc;
  struct char_data *mob;

  i = (real_mobile(OLC_NUM(d)) == NOBODY);

  if ((new_rnum = add_mobile(OLC_MOB(d), OLC_NUM(d))) == NOBODY) {
    log("medit_save_internally: add_mobile failed.");
    return;
  }

  /* Make sure scripts are updated too. - Welcor */

  /*
     Testing

   */
#if 0
  if (OLC_MOB(d)->proto_script &&
      OLC_MOB(d)->proto_script != OLC_SCRIPT(d)) {
    struct trig_proto_list *proto, *fproto;
    proto = OLC_MOB(d)->proto_script;
    while (proto) {
      fproto = proto;
      proto = proto->next;
      free(fproto);
    }
  }
#endif
  OLC_MOB(d)->proto_script = OLC_SCRIPT(d);

  /* Free old proto list  */
  if (mob_proto[new_rnum].proto_script &&
      mob_proto[new_rnum].proto_script != OLC_SCRIPT(d)) {
    struct trig_proto_list *proto, *fproto;
    proto = mob_proto[new_rnum].proto_script;
    while (proto) {
      fproto = proto;
      proto = proto->next;
      free(fproto);
    }
  }
  /* this will handle new instances of the mob: */
  mob_proto[new_rnum].proto_script = OLC_SCRIPT(d);

  /* this takes care of the mobs currently in-game */
  for (mob = character_list; mob; mob = mob->next) {
    if (GET_MOB_RNUM(mob) != new_rnum) {
      continue;
    }
    /* remove any old scripts */
    if (SCRIPT(mob)) {
      extract_script(SCRIPT(mob));
      SCRIPT(mob) = NULL;
    }
    mob->proto_script = OLC_SCRIPT(d);

    assign_triggers(mob, MOB_TRIGGER);
  }

  if (!i)	/* Only renumber on new mobiles. */
    return;

  /*
   * Update keepers in shops being edited and other mobs being edited.
   */
  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    if (STATE(dsc) == CON_SEDIT)
      S_KEEPER(OLC_SHOP(dsc)) += (S_KEEPER(OLC_SHOP(dsc)) >= new_rnum);
    else if (STATE(dsc) == CON_MEDIT)
      GET_MOB_RNUM(OLC_MOB(dsc)) += (GET_MOB_RNUM(OLC_MOB(dsc)) >= new_rnum);
  }

  /*
   * Update other people in zedit too. From: C.Raehl 4/27/99
   */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        if (OLC_ZONE(dsc)->cmd[i].command == 'M')
          if (OLC_ZONE(dsc)->cmd[i].arg1 >= new_rnum)
            OLC_ZONE(dsc)->cmd[i].arg1++;
}

/**************************************************************************
 Menu functions
 **************************************************************************/

/*
 * Display positions. (sitting, standing, etc)
 */
void medit_disp_positions(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; *position_types[i] != '\n'; i++) {
    write_to_output(d, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
  }
  write_to_output(d, "Enter position number : ");
}

/*-------------------------------------------------------------------*/

#if CONFIG_OASIS_MPROG
/*
 * Get the type of MobProg.
 */
const char *medit_get_mprog_type(struct mob_prog_data *mprog)
{
  switch (mprog->type) {
  case IN_FILE_PROG:	return ">in_file_prog";
  case ACT_PROG:	return ">act_prog";
  case SPEECH_PROG:	return ">speech_prog";
  case RAND_PROG:	return ">rand_prog";
  case FIGHT_PROG:	return ">fight_prog";
  case HITPRCNT_PROG:	return ">hitprcnt_prog";
  case DEATH_PROG:	return ">death_prog";
  case ENTRY_PROG:	return ">entry_prog";
  case GREET_PROG:	return ">greet_prog";
  case ALL_GREET_PROG:	return ">all_greet_prog";
  case GIVE_PROG:	return ">give_prog";
  case BRIBE_PROG:	return ">bribe_prog";
  }
  return ">ERROR_PROG";
}

/*-------------------------------------------------------------------*/

/*
 * Display the MobProgs.
 */
void medit_disp_mprog(struct descriptor_data *d)
{
  struct mob_prog_data *mprog = OLC_MPROGL(d);

  OLC_MTOTAL(d) = 1;

  clear_screen(d);
  while (mprog) {
    write_to_output(d, "%d) %s %s\r\n", OLC_MTOTAL(d), medit_get_mprog_type(mprog),
		(mprog->arglist ? mprog->arglist : "NONE"));
    OLC_MTOTAL(d)++;
    mprog = mprog->next;
  }
  write_to_output(d,  "%d) Create New Mob Prog\r\n"
		"%d) Purge Mob Prog\r\n"
		"Enter number to edit [0 to exit]:  ",
		OLC_MTOTAL(d), OLC_MTOTAL(d) + 1);
  OLC_MODE(d) = MEDIT_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProgs.
 */
void medit_change_mprog(struct descriptor_data *d)
{
  clear_screen(d);
  write_to_output(d,  "1) Type: %s\r\n"
		"2) Args: %s\r\n"
		"3) Commands:\r\n%s\r\n\r\n"
		"Enter number to edit [0 to exit]: ",
	medit_get_mprog_type(OLC_MPROG(d)),
	(OLC_MPROG(d)->arglist ? OLC_MPROG(d)->arglist: "NONE"),
	(OLC_MPROG(d)->comlist ? OLC_MPROG(d)->comlist : "NONE"));

  OLC_MODE(d) = MEDIT_CHANGE_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProg type.
 */
void medit_disp_mprog_types(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < NUM_PROGS-1; i++) {
    write_to_output(d, "%s%2d%s) %s\r\n", grn, i, nrm, mobprog_types[i]);
  }
  write_to_output(d, "Enter mob prog type : ");
  OLC_MODE(d) = MEDIT_MPROG_TYPE;
}
#endif

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
void medit_disp_sex(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < NUM_GENDERS; i++) {
    write_to_output(d, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
  }
  write_to_output(d, "Enter gender number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display the race of the mobile.
 */
void medit_disp_race(struct descriptor_data *d)
{
    int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (counter = 0; counter < NUM_MOB_RACES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		mob_race_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter race number : ");

}

/*-------------------------------------------------------------------*/



/*
 * Size menu
 */
 void medit_disp_size_menu(struct descriptor_data *d)
 {
    int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (counter = 0; counter < NUM_MOB_SIZES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		sizes[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter size number : ");

 }

/*
 * Display the class of the mobile.
 */
void medit_disp_class(struct descriptor_data *d)
{
    int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (counter = 0; counter < NUM_MOB_CLASSES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		mob_class_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter class number : ");

}

/*-------------------------------------------------------------------*/

/*
 * Display attack types menu.
 */
void medit_disp_attack_types(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < NUM_ATTACK_TYPES; i++) {
    write_to_output(d, "%s%2d%s) %s\r\n", grn, i, nrm, attack_hit_text[i].singular);
  }
  write_to_output(d, "Enter attack type : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display mob-flags menu.
 */
void medit_disp_mob_flags(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];
  int i, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < NUM_MOB_FLAGS; i++) {
    write_to_output(d, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, action_bits[i],
		!(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(MOB_FLAGS(OLC_MOB(d)), action_bits, bitbuf, sizeof(bitbuf));
  write_to_output(d, "\r\nCurrent flags : %s%s%s\r\nEnter mob flags (0 to quit) : ",
		  cyn, bitbuf, nrm);
}

/*-------------------------------------------------------------------*/

/*
 * Display affection flags menu.
 */
void medit_disp_aff_flags(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];
  int i, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < NUM_AFF_FLAGS; i++) {
    write_to_output(d, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, affected_bits[i],
			!(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(AFF_FLAGS(OLC_MOB(d)), affected_bits, bitbuf, sizeof(bitbuf));
  write_to_output(d, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit) : ",
			  cyn, bitbuf, nrm);
}

/*-------------------------------------------------------------------*/

/*
 * Display affection2 flags menu.
 */
void medit_disp_aff2_flags(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];
  int i, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < NUM_AFF2_FLAGS; i++) {
    write_to_output(d, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, affected2_bits[i],
			!(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(AFF2_FLAGS(OLC_MOB(d)), affected2_bits, bitbuf, sizeof(bitbuf));
  write_to_output(d, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit) : ",
			  cyn, bitbuf, nrm);
}

/*-------------------------------------------------------------------*/

/*
 * Display resist menu
 */
void medit_disp_resist(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < MAX_ATTACK_TYPES; i++) {
    if(OLC_MOB(d)->char_specials.resist[i] == 0)
      write_to_output(d, "%s%2d%s) %-12.12s\r\n", grn, i + 1, nrm, res_types[i]);	
    else
      write_to_output(d, "%s%2d%s) %-12.12s %s-set-%s\r\n", grn, i + 1, nrm, res_types[i],
        cyn, nrm);	
    	
  }  
  write_to_output(d, "\r\nEnter resist to toggle (0 to quit) : ");			 
}

/*-------------------------------------------------------------------*/
/*
 * Display immune menu
 */
void medit_disp_immune(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < MAX_ATTACK_TYPES; i++) {
    if(OLC_MOB(d)->char_specials.immune[i] == 0)
      write_to_output(d, "%s%2d%s) %-12.12s\r\n", grn, i + 1, nrm, res_types[i]);
    else
      write_to_output(d, "%s%2d%s) %-12.12s %s-set-%s\r\n", grn, i + 1, nrm, res_types[i],
        cyn, nrm);
  }
  write_to_output(d, "\r\nEnter immunity to toggle (0 to quit) : ");
}

/*-------------------------------------------------------------------*/
/*
 * Display vulnerable menu
 */
void medit_disp_vulnerable(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < MAX_ATTACK_TYPES; i++) {
    if(OLC_MOB(d)->char_specials.vulnerable[i] == 0)
      write_to_output(d, "%s%2d%s) %-12.12s\r\n", grn, i + 1, nrm, res_types[i]);
    else
      write_to_output(d, "%s%2d%s) %-12.12s %s-set-%s\r\n", grn, i + 1, nrm, res_types[i],
        cyn, nrm);

  }
  write_to_output(d, "\r\nEnter vulnerability to toggle (0 to quit) : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void medit_disp_menu(struct descriptor_data *d)
{
  struct char_data *mob;
  char mbitbuf[MAX_INPUT_LENGTH], abitbuf[MAX_INPUT_LENGTH], resistbuf[MAX_INPUT_LENGTH];
  char vulnbuf[MAX_INPUT_LENGTH], immunebuf[MAX_INPUT_LENGTH], a2bitbuf[MAX_INPUT_LENGTH];
  size_t len = 0;
  int i, nlen;
  bool resists = FALSE;


  mob = OLC_MOB(d);
  get_char_colors(d->character);
  clear_screen(d);
//  Here is the actual menu
  write_to_output(d,
	  "-- Mob Number:  [%s%d%s]\r\n"
	  "%s1%s) Sex: %s%-7.7s%s	         %s2%s) Alias: %s%s\r\n"
	  "%s3%s) S-Desc: %s%s\r\n"
	  "%s4%s) L-Desc:-\r\n%s%s"
	  "%s5%s) D-Desc:-\r\n%s%s"
     "%s6%s) Level: [%s%2d%s]        %s7%s) Difficulty(0-14): [%s%d%s]          %s8%s) Alignment: [%s%4d%s]\r\n"
     "      %sA%s)%s Hitroll:         [%4d]        %sB%s)%s Damroll:            [%4d]\r\n"
     "      %sC%s)%s NumDamDice:      [%4d]        %sD%s)%s SizeDamDice:        [%4d]\r\n"
     "      %sE%s)%s Base Hit Points: [%6d]      %sF%s)%s Armor Class:        [%4d]\r\n"
     "      %sG%s)%s Exp:             [%6d]      %sH%s)%s Gold:               [%8d]\r\n",
	  cyn, OLC_NUM(d), nrm,
	  grn, nrm, yel, genders[(int)GET_SEX(mob)], nrm,
	  grn, nrm, yel, GET_ALIAS(mob),
	  grn, nrm, yel, GET_SDESC(mob),
	  grn, nrm, yel, GET_LDESC(mob),
	  grn, nrm, yel, GET_DDESC(mob),
	  grn, nrm, cyn, GET_LEVEL(mob), nrm,
          grn, nrm, cyn, GET_DIFFICULTY(mob), nrm,
          grn, nrm, cyn, GET_ALIGNMENT(mob), nrm,
	  grn, nrm, cyn, GET_HITROLL(mob), grn, nrm, cyn, GET_DAMROLL(mob),
	  grn, nrm, cyn, GET_NDD(mob), grn, nrm, cyn, GET_SDD(mob),
	  grn, nrm, cyn, GET_HIT(mob), grn, nrm, cyn, GET_AC(mob),
	  grn, nrm, cyn, GET_EXP(mob), grn, nrm, cyn, GET_GOLD(mob)
	  );

  sprintbit(MOB_FLAGS(mob), action_bits, mbitbuf, sizeof(mbitbuf));
  sprintbit(AFF_FLAGS(mob), affected_bits, abitbuf, sizeof(abitbuf));
  sprintbit(AFF2_FLAGS(mob), affected2_bits, a2bitbuf, sizeof(a2bitbuf));

 /* show set resists */
	  for(i = 0;i < MAX_ATTACK_TYPES; i++) {
	    if (mob->char_specials.resist[i] > 0) {
	      nlen = snprintf(resistbuf + len, sizeof(resistbuf) - len, "%s ", res_types[i]);
	       resists = TRUE;
	     if (len + nlen >= sizeof(resistbuf) || nlen < 0)
          break;
        len += nlen;
	    }
	  }
	if (resists == FALSE) 
		  snprintf(resistbuf, sizeof(resistbuf), "None");
  resists = FALSE;
  len = 0;
  nlen = 0;

   for(i = 0;i < MAX_ATTACK_TYPES; i++) {
	    if (mob->char_specials.immune[i] > 0) {
	      nlen = snprintf(immunebuf + len, sizeof(immunebuf) - len, "%s ", res_types[i]);
	       resists = TRUE;
	     if (len + nlen >= sizeof(immunebuf) || nlen < 0)
          break;
        len += nlen;
	    }
	  }
	if (resists == FALSE)
		  snprintf(immunebuf, sizeof(immunebuf), "None");
  resists = FALSE;
  len = 0;
  nlen = 0;

   for(i = 0;i < MAX_ATTACK_TYPES; i++) {
	    if (mob->char_specials.vulnerable[i] > 0) {
	      nlen = snprintf(vulnbuf + len, sizeof(vulnbuf) - len, "%s ", res_types[i]);
	       resists = TRUE;
	     if (len + nlen >= sizeof(vulnbuf) || nlen < 0)
          break;
        len += nlen;
	    }
	  }
	if (resists == FALSE)
		  snprintf(vulnbuf, sizeof(vulnbuf), "None");


  write_to_output(d,
	  "%sI%s) Position   : %s%s\r\n"
	  "%sJ%s) Default    : %s%s\r\n"
	  "%sK%s) Attack     : %s%s\r\n"
	  "%sL%s) NPC Flags  : %s%s\r\n"
	  "%sM%s) AFF Flags  : %s%s\r\n"
	  "%sN%s) Num Attack : %s%d\r\n"
	  "%sO%s) Immune     : %s%s\r\n"
	  "%sP%s) Vulnerable : %s%s\r\n"
	  "%sR%s) Resist     : %s%s\r\n"
#if CONFIG_OASIS_MPROG
	  "%sP%s) Mob Progs : %s%s\r\n"
#endif
          "%sS%s) Script     : %s%s\r\n"
          "%sT%s) AFF2 Flags : %s%s\r\n"
          "%sU%s) Race       : %s%s\r\n"
          "%sV%s) Class      : %s%s\r\n"
          "%sZ%s) Size       : %s%s\r\n"
	  "%sQ%s) Quit\r\n"
	  "Enter choice : ",

	  grn, nrm, yel, position_types[(int)GET_POS(mob)],
	  grn, nrm, yel, position_types[(int)GET_DEFAULT_POS(mob)],
	  grn, nrm, yel, attack_hit_text[(int)GET_ATTACK(mob)].singular,
	  grn, nrm, cyn, mbitbuf,
	  grn, nrm, cyn, abitbuf,
	  grn, nrm, yel, GET_NUM_ATTACKS(mob),
	  grn, nrm, cyn, immunebuf,
	  grn, nrm, cyn, vulnbuf,
	  grn, nrm, cyn, resistbuf,
#if CONFIG_OASIS_MPROG
	  grn, nrm, cyn, (OLC_MPROGL(d) ? "Set." : "Not Set."),
#endif
          grn, nrm, cyn, OLC_SCRIPT(d) ?"Set.":"Not Set.",
          grn, nrm, cyn, a2bitbuf,
          grn, nrm, cyn, mob_race_types[(int)GET_RACE(mob)],
          grn, nrm, cyn, mob_class_types[(int)GET_CLASS(mob)],
          grn, nrm, cyn, sizes[(int)GET_SIZE(mob)],
	  grn, nrm
	  );

  OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/************************************************************************
 *			The GARGANTAUN event handler			*
 ************************************************************************/

void medit_parse(struct descriptor_data *d, char *arg)
{
  int i = -1;
  char *oldtext = NULL;

  if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
    i = atoi(arg);
    if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && !isdigit(arg[1])))) {
      write_to_output(d, "Field must be numerical, try again : ");
      return;
    }
  } else {	/* String response. */
    if (!genolc_checkstring(d, arg))
      return;
  }
  switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
  case MEDIT_CONFIRM_SAVESTRING:
    /*
     * Ensure mob has MOB_ISNPC set or things will go pear shaped.
     */
    SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
    switch (*arg) {
    case 'y':
    case 'Y':
      /*
       * Save the mob in memory and to disk.
       */
      medit_save_internally(d);
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits mob %d", GET_NAME(d->character), OLC_NUM(d));
#if (OLC_AUTO_SAVE)
      medit_save_to_disk(zone_table[real_zone_by_thing(OLC_NUM(d))].number);
      write_to_output(d, "Mobile saved to disk.\r\n");
#else
      write_to_output(d, "Mobile saved to memory.\r\n");
#endif
      /* FALL THROUGH */
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Do you wish to save the mobile? : ");
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  case MEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) {	/* Anything been changed? */
	write_to_output(d, "Do you wish to save the changes to the mobile? (y/n) : ");
	OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
      } else
	cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      OLC_MODE(d) = MEDIT_SEX;
      medit_disp_sex(d);
      return;
    case '2':
      OLC_MODE(d) = MEDIT_ALIAS;
      i--;
      break;
    case '3':
      OLC_MODE(d) = MEDIT_S_DESC;
      i--;
      break;
    case '4':
      OLC_MODE(d) = MEDIT_L_DESC;
      i--;
      break;
    case '5':
      OLC_MODE(d) = MEDIT_D_DESC;
      send_editor_help(d);
      write_to_output(d, "Enter mob description:\r\n\r\n");
      if (OLC_MOB(d)->player.description) {
	write_to_output(d, "%s", OLC_MOB(d)->player.description);
	oldtext = strdup(OLC_MOB(d)->player.description);
      }
      string_write(d, &OLC_MOB(d)->player.description, MAX_MOB_DESC, 0, oldtext);
      OLC_VAL(d) = 1;
      return;
    case '6':
      OLC_MODE(d) = MEDIT_LEVEL;
      i++;
      break;
    case '7':
      OLC_MODE(d) = MEDIT_DIFFICULTY;
      i++;
      break;
    case '8':
      OLC_MODE(d) = MEDIT_ALIGNMENT;
      i++;
      break;
   case 'A':
   case 'a':
     OLC_MODE(d) = MEDIT_HITROLL;
      i++;
      break;
    case 'B':
    case 'b':
     OLC_MODE(d) = MEDIT_DAMROLL;
     i++;
     break;
    case 'c':
    case 'C':
      OLC_MODE(d) = MEDIT_NDD;
      i++;
      break;
    case 'd':
    case 'D':
      OLC_MODE(d) = MEDIT_SDD;
      i++;
      break;
/*    case 'c':
    case 'C':
      OLC_MODE(d) = MEDIT_NUM_HP_DICE;
      i++;
      break;
    case 'd':
    case 'D':
      OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
      i++;
      break; */
    case 'e':
    case 'E':
      OLC_MODE(d) = MEDIT_ADD_HP;
      i++;
      break;
    case 'f':
    case 'F':
      OLC_MODE(d) = MEDIT_AC;
      i++;
      break;
    case 'g':
    case 'G':
      OLC_MODE(d) = MEDIT_EXP;
      i++;
      break;
    case 'h':
    case 'H':
      OLC_MODE(d) = MEDIT_GOLD;
      i++;
      break;
    case 'i':
    case 'I':
      OLC_MODE(d) = MEDIT_POS;
      medit_disp_positions(d);
      return;
    case 'j':
    case 'J':
      OLC_MODE(d) = MEDIT_DEFAULT_POS;
      medit_disp_positions(d);
      return;
    case 'k':
    case 'K':
      OLC_MODE(d) = MEDIT_ATTACK;
      medit_disp_attack_types(d);
      return;
    case 'l':
    case 'L':
      OLC_MODE(d) = MEDIT_NPC_FLAGS;
      medit_disp_mob_flags(d);
      return;
    case 'm':
    case 'M':
      OLC_MODE(d) = MEDIT_AFF_FLAGS;
      medit_disp_aff_flags(d);
      return;
    case 'n':
    case 'N':
      OLC_MODE(d) = MEDIT_NUM_ATTACKS;
      i++;
      break;
    case 'o':
    case 'O':
      OLC_MODE(d) = MEDIT_IMMUNE;
      medit_disp_immune(d);
      return;
    case 'p':
    case 'P':
      OLC_MODE(d) = MEDIT_VULNERABLE;
      medit_disp_vulnerable(d);
      return;
    case 'r':
    case 'R':
      OLC_MODE(d) = MEDIT_RESIST;
      medit_disp_resist(d);
      return;
#if CONFIG_OASIS_MPROG
    case 'p':
    case 'P':
      OLC_MODE(d) = MEDIT_MPROG;
      medit_disp_mprog(d);
      return;
#endif
    case 's':
    case 'S':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      dg_script_menu(d);
      return;
    case 't':
    case 'T':
      OLC_MODE(d) = MEDIT_AFF2_FLAGS;
      medit_disp_aff2_flags(d);
      return;
    case 'u':
    case 'U':
      OLC_MODE(d) = MEDIT_RACE;
      medit_disp_race(d);
      return;
    case 'v':
    case 'V':
      OLC_MODE(d) = MEDIT_CLASS;
      medit_disp_class(d);
      return;
    case 'z':
    case 'Z':
      OLC_MODE(d) = MEDIT_SIZE;
      medit_disp_size_menu(d);
      return;
    default:
      medit_disp_menu(d);
      return;
    }
    if (i == 0)
      break;
    else if (i == 1)
      write_to_output(d, "\r\nEnter new value : ");
    else if (i == -1)
      write_to_output(d, "\r\nEnter new text :\r\n] ");
    else
      write_to_output(d, "Oops...\r\n");
    return;
/*-------------------------------------------------------------------*/
  case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg)) return;
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_ALIAS:
    if (GET_ALIAS(OLC_MOB(d)))
      free(GET_ALIAS(OLC_MOB(d)));
    GET_ALIAS(OLC_MOB(d)) = str_udup(arg);
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_S_DESC:
    if (GET_SDESC(OLC_MOB(d)))
      free(GET_SDESC(OLC_MOB(d)));
    GET_SDESC(OLC_MOB(d)) = str_udup(arg);
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_L_DESC:
    if (GET_LDESC(OLC_MOB(d)))
      free(GET_LDESC(OLC_MOB(d)));
    if (arg && *arg) {
      GET_LDESC(OLC_MOB(d)) = strdup(strcat(arg, "\r\n"));
    } else
      GET_LDESC(OLC_MOB(d)) = strdup(STRING_UNDEFINED);

    break;
/*-------------------------------------------------------------------*/
  case MEDIT_D_DESC:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: medit_parse(): Reached D_DESC case!");
    write_to_output(d, "Oops...\r\n");
    break;
/*-------------------------------------------------------------------*/
#if CONFIG_OASIS_MPROG
  case MEDIT_MPROG_COMLIST:
    /*
     * We should never get here, but if we do, bail out.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: medit_parse(): Reached MPROG_COMLIST case!");
    break;
#endif
/*-------------------------------------------------------------------*/
  case MEDIT_NPC_FLAGS:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= NUM_MOB_FLAGS)
      TOGGLE_BIT(MOB_FLAGS(OLC_MOB(d)), 1 << (i - 1));
    medit_disp_mob_flags(d);
    return;
/*-------------------------------------------------------------------*/
  case MEDIT_AFF_FLAGS:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= NUM_AFF_FLAGS)
      TOGGLE_BIT(AFF_FLAGS(OLC_MOB(d)), 1ULL << (i - 1));

    /* Remove unwanted bits right away. */
    REMOVE_BIT(AFF_FLAGS(OLC_MOB(d)), 
               AFF_CHARM | AFF_POISON | AFF_GROUP | AFF_SLEEP);
    medit_disp_aff_flags(d);
    return;
/*-----------------------------------------------------------------*/
  case MEDIT_AFF2_FLAGS:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= NUM_AFF_FLAGS)
      TOGGLE_BIT(AFF2_FLAGS(OLC_MOB(d)), 1ULL << (i - 1));

    /* Remove unwanted bits right away.  But we don't have any here yet.  Anubis*/

    medit_disp_aff2_flags(d);
    return;
/*-----------------------------------------------------------------*/
 case MEDIT_RESIST:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= MAX_ATTACK_TYPES) {
      if ( OLC_MOB(d)->char_specials.resist[i - 1] == 1)
        OLC_MOB(d)->char_specials.resist[i - 1] = 0;
      else
        OLC_MOB(d)->char_specials.resist[i - 1] = 1;
    }
    medit_disp_resist(d);
    return;
/*-----------------------------------------------------------------*/   
 case MEDIT_IMMUNE:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= MAX_ATTACK_TYPES) {
      if ( OLC_MOB(d)->char_specials.immune[i - 1] == 1)
        OLC_MOB(d)->char_specials.immune[i - 1] = 0;
      else
        OLC_MOB(d)->char_specials.immune[i - 1] = 1;
    }
    medit_disp_immune(d);
    return;
/*-----------------------------------------------------------------*/    
 case MEDIT_VULNERABLE:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= MAX_ATTACK_TYPES) {
      if ( OLC_MOB(d)->char_specials.vulnerable[i - 1] == 1)
        OLC_MOB(d)->char_specials.vulnerable[i - 1] = 0;
      else
        OLC_MOB(d)->char_specials.vulnerable[i - 1] = 1;
    }
    medit_disp_vulnerable(d);
    return;
#if CONFIG_OASIS_MPROG
  case MEDIT_MPROG:
    if ((i = atoi(arg)) == 0)
      medit_disp_menu(d);
    else if (i == OLC_MTOTAL(d)) {
      struct mob_prog_data *temp;
      CREATE(temp, struct mob_prog_data, 1);
      temp->next = OLC_MPROGL(d);
      temp->type = -1;
      temp->arglist = NULL;
      temp->comlist = NULL;
      OLC_MPROG(d) = temp;
      OLC_MPROGL(d) = temp;
      OLC_MODE(d) = MEDIT_CHANGE_MPROG;
      medit_change_mprog (d);
    } else if (i < OLC_MTOTAL(d)) {
      struct mob_prog_data *temp;
      int x = 1;
      for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
        x++;
      OLC_MPROG(d) = temp;
      OLC_MODE(d) = MEDIT_CHANGE_MPROG;
      medit_change_mprog (d);
    } else if (i == (OLC_MTOTAL(d) + 1)) {
      write_to_output(d, "Which mob prog do you want to purge? ");
      OLC_MODE(d) = MEDIT_PURGE_MPROG;
    } else
      medit_disp_menu(d);
    return;

  case MEDIT_PURGE_MPROG:
    if ((i = atoi(arg)) > 0 && i < OLC_MTOTAL(d)) {
      struct mob_prog_data *temp;
      int x = 1;

      for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
	x++;
      OLC_MPROG(d) = temp;
      REMOVE_FROM_LIST(OLC_MPROG(d), OLC_MPROGL(d), next);
      free(OLC_MPROG(d)->arglist);
      free(OLC_MPROG(d)->comlist);
      free(OLC_MPROG(d));
      OLC_MPROG(d) = NULL;
      OLC_VAL(d) = 1;
    }
    medit_disp_mprog(d);
    return;

  case MEDIT_CHANGE_MPROG:
    if ((i = atoi(arg)) == 1)
      medit_disp_mprog_types(d);
    else if (i == 2) {
      write_to_output(d, "Enter new arg list: ");
      OLC_MODE(d) = MEDIT_MPROG_ARGS;
    } else if (i == 3) {
      write_to_output(d, "Enter new mob prog commands:\r\n");
      /*
       * Pass control to modify.c for typing.
       */
      OLC_MODE(d) = MEDIT_MPROG_COMLIST;
      if (OLC_MPROG(d)->comlist) {
        write_to_output(d, "%s", OLC_MPROG(d)->comlist);
        oldtext = strdup(OLC_MPROG(d)->comlist);
      }
      string_write(d, &OLC_MPROG(d)->comlist, MAX_STRING_LENGTH, 0, oldtext);
      OLC_VAL(d) = 1;
    } else
      medit_disp_mprog(d);
    return;
#endif

/*-------------------------------------------------------------------*/

/*
 * Numerical responses.
 */

#if CONFIG_OASIS_MPROG
  case MEDIT_MPROG_TYPE:
    /*
     * This calculation may be off by one too many powers of 2?
     * Someone who actually uses MobProgs will have to check.
     */
    OLC_MPROG(d)->type = (1 << LIMIT(atoi(arg), 0, NUM_PROGS - 1));
    OLC_VAL(d) = 1;
    medit_change_mprog(d);
    return;

  case MEDIT_MPROG_ARGS:
    OLC_MPROG(d)->arglist = strdup(arg);
    OLC_VAL(d) = 1;
    medit_change_mprog(d);
    return;
#endif

  case MEDIT_SEX:
    GET_SEX(OLC_MOB(d)) = LIMIT(i, 0, NUM_GENDERS - 1);
    break;
    
  case MEDIT_RACE:
    GET_RACE(OLC_MOB(d)) = LIMIT(i, 0, NUM_MOB_RACES - 1);
    break;
  
  case MEDIT_CLASS:
    GET_CLASS(OLC_MOB(d)) = LIMIT(i, 0, NUM_MOB_CLASSES - 1);
    break;

  case MEDIT_SIZE:
    GET_SIZE(OLC_MOB(d)) = LIMIT(i, 0, NUM_MOB_SIZES -1);
    break;

  case MEDIT_HITROLL:
    GET_HITROLL(OLC_MOB(d)) = LIMIT(i, 0, 100);
    break;

  case MEDIT_DAMROLL:
    GET_DAMROLL(OLC_MOB(d)) = LIMIT(i, 0, 50);
    break;

  case MEDIT_NDD:
    GET_NDD(OLC_MOB(d)) = LIMIT(i, 0, 30);
    break;

  case MEDIT_SDD:
    GET_SDD(OLC_MOB(d)) = LIMIT(i, 0, 127);
    break;

  case MEDIT_NUM_HP_DICE:
    GET_HIT(OLC_MOB(d)) = LIMIT(i, 0, 30);
    break;

  case MEDIT_SIZE_HP_DICE:
    GET_MANA(OLC_MOB(d)) = LIMIT(i, 0, 1000);
    break;

  case MEDIT_ADD_HP:
    GET_HIT(OLC_MOB(d)) = LIMIT(i, 0, 30000);
    break;

  case MEDIT_AC:
    GET_AC(OLC_MOB(d)) = LIMIT(i, -20, 10);
    break;

  case MEDIT_EXP:
    GET_EXP(OLC_MOB(d)) = MAX(i, 0);
    break;

  case MEDIT_GOLD:
    GET_GOLD(OLC_MOB(d)) = MAX(i, 0);
    break;

  case MEDIT_POS:
    GET_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS - 1);
    break;

  case MEDIT_DEFAULT_POS:
    GET_DEFAULT_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS - 1);
    break;

  case MEDIT_ATTACK:
    GET_ATTACK(OLC_MOB(d)) = LIMIT(i, 0, NUM_ATTACK_TYPES - 1);
    break;

  case MEDIT_LEVEL:
    GET_LEVEL(OLC_MOB(d)) = LIMIT(i, 1, 40);
    get_defaults(OLC_MOB(d)); // Changes to Level will bring up a set of default values
    break;
  case MEDIT_DIFFICULTY:
    GET_DIFFICULTY(OLC_MOB(d)) = LIMIT(i, 0, 14); 
    get_defaults(OLC_MOB(d)); // Changes to Difficulty will bring up a set of default values
    break;
  case MEDIT_ALIGNMENT:
    GET_ALIGNMENT(OLC_MOB(d)) = LIMIT(i, -1000, 1000);
    break;
  case MEDIT_NUM_ATTACKS:
    GET_NUM_ATTACKS(OLC_MOB(d)) = LIMIT(i, 1, 10);
    break; 

  default:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: medit_parse(): Reached default case!");
    write_to_output(d, "Oops...\r\n");
    break;
  }
/*-------------------------------------------------------------------*/

/*
 * END OF CASE
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag
 */

  OLC_VAL(d) = TRUE;
  medit_disp_menu(d);
}

void medit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {

#if CONFIG_OASIS_MPROG
  case MEDIT_MPROG_COMLIST:
    medit_change_mprog(d);
    break;
#endif

  case MEDIT_D_DESC:
  default:
     medit_disp_menu(d);
     break;
  }
}

void get_defaults(struct char_data *mob)
{

if (GET_LEVEL(mob) < 1)
  GET_LEVEL(mob) = 1;

GET_NDD(mob) = 1;
GET_SDD(mob) = GET_LEVEL(mob) + 4;

switch (GET_LEVEL(mob)) {
case 1:
  GET_DAMROLL(mob) = 1;
  GET_HITROLL(mob) = 1;
  GET_HIT(mob) = 120;
  GET_AC(mob) = 10;
  GET_EXP(mob) = 250;
  GET_GOLD(mob) = 250;
 break;
case 2:
  GET_DAMROLL(mob) = 2;
  GET_HITROLL(mob) = 2;
  GET_HIT(mob) = 200;
  GET_AC(mob) = 10;
  GET_EXP(mob) = 375;
  GET_GOLD(mob) = 300;
 break;
case 3:
  GET_DAMROLL(mob) = 3;
  GET_HITROLL(mob) = 3;
  GET_HIT(mob) = 280;
  GET_AC(mob) = 10;
  GET_EXP(mob) = 500;
  GET_GOLD(mob) = 350;
 break;
case 4:
  GET_DAMROLL(mob) = 4;
  GET_HITROLL(mob) = 4;
  GET_HIT(mob) = 360;
  GET_AC(mob) = 9;
  GET_EXP(mob) = 750;
  GET_GOLD(mob) = 400;
 break;
case 5:
  GET_DAMROLL(mob) = 5;
  GET_HITROLL(mob) = 5;
  GET_HIT(mob) = 440;
  GET_AC(mob) = 9;
  GET_EXP(mob) = 1125;
  GET_GOLD(mob) = 450;
 break;
case 6:
  GET_DAMROLL(mob) = 5;
  GET_HITROLL(mob) = 6;
  GET_HIT(mob) = 520;
  GET_AC(mob) = 9;
  GET_EXP(mob) = 1625;
  GET_GOLD(mob) = 500;
 break;
case 7:
  GET_DAMROLL(mob) = 6;
  GET_HITROLL(mob) = 7;
  GET_HIT(mob) = 600;
  GET_AC(mob) = 8;
  GET_EXP(mob) = 3250;
  GET_GOLD(mob) = 550;
 break;
case 8:
  GET_DAMROLL(mob) = 6;
  GET_HITROLL(mob) = 8;
  GET_HIT(mob) = 680;
  GET_AC(mob) = 8;
  GET_EXP(mob) = 5725;
  GET_GOLD(mob) = 600;
 break;
case 9:
  GET_DAMROLL(mob) = 7;
  GET_HITROLL(mob) = 9;
  GET_HIT(mob) = 760;
  GET_AC(mob) = 8;
  GET_EXP(mob) = 7500;
  GET_GOLD(mob) = 650;
 break;
case 10:
  GET_DAMROLL(mob) = 7;
  GET_HITROLL(mob) = 10;
  GET_HIT(mob) = 840;
  GET_AC(mob) = 6;
  GET_EXP(mob) = 10000;
  GET_GOLD(mob) = 700;
 break;
case 11:
  GET_DAMROLL(mob) = 8;
  GET_HITROLL(mob) = 11;
  GET_HIT(mob) = 920;
  GET_AC(mob) = 6;
  GET_EXP(mob) = 15750;
  GET_GOLD(mob) = 750;
 break;
case 12:
  GET_DAMROLL(mob) = 8;
  GET_HITROLL(mob) = 12;
  GET_HIT(mob) = 1000;
  GET_AC(mob) = 6;
  GET_EXP(mob) = 22275;
  GET_GOLD(mob) = 800;
 break;
case 13:
  GET_DAMROLL(mob) = 9;
  GET_HITROLL(mob) = 13;
  GET_HIT(mob) = 1080;
  GET_AC(mob) = 4;
  GET_EXP(mob) = 31000;
  GET_GOLD(mob) = 850;
 break;
case 14:
  GET_DAMROLL(mob) = 9;
  GET_HITROLL(mob) = 14;
  GET_HIT(mob) = 1160;
  GET_AC(mob) = 4;
  GET_EXP(mob) = 40500;
  GET_GOLD(mob) = 900;
 break;
case 15:
  GET_DAMROLL(mob) = 10;
  GET_HITROLL(mob) = 15;
  GET_HIT(mob) = 1240;
  GET_AC(mob) = 0;
  GET_EXP(mob) = 42750;
  GET_GOLD(mob) = 1000;
 break;
case 16:
  GET_DAMROLL(mob) = 10;
  GET_HITROLL(mob) = 16;
  GET_HIT(mob) = 1320;
  GET_AC(mob) = 0;
  GET_EXP(mob) = 53275;
  GET_GOLD(mob) = 2000;
 break;
case 17:
  GET_DAMROLL(mob) = 11;
  GET_HITROLL(mob) = 17;
  GET_HIT(mob) = 1400;
  GET_AC(mob) = -6;
  GET_EXP(mob) = 65000;
  GET_GOLD(mob) = 3000;
 break;
case 18:
  GET_DAMROLL(mob) = 11;
  GET_HITROLL(mob) = 18;
  GET_HIT(mob) = 1480;
  GET_AC(mob) = -6;
  GET_EXP(mob) = 76575;
  GET_GOLD(mob) = 4000;
 break;
case 19:
  GET_DAMROLL(mob) = 12;
  GET_HITROLL(mob) = 19;
  GET_HIT(mob) = 1560;
  GET_AC(mob) = -8;
  GET_EXP(mob) = 88500;
  GET_GOLD(mob) = 5000;
 break;
case 20:
  GET_DAMROLL(mob) = 12;
  GET_HITROLL(mob) = 22;
  GET_HIT(mob) = 1640;
  GET_AC(mob) = -8;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 6000;
 break;
case 21:
  GET_DAMROLL(mob) = 13;
  GET_HITROLL(mob) = 24;
  GET_HIT(mob) = 1720;
  GET_AC(mob) = -10;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 7000;
 break;
case 22:
  GET_DAMROLL(mob) = 13;
  GET_HITROLL(mob) = 26;
  GET_HIT(mob) = 1800;
  GET_AC(mob) = -10;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 8000;
 break;
case 23:
  GET_DAMROLL(mob) = 14;
  GET_HITROLL(mob) = 28;
  GET_HIT(mob) = 1880;
  GET_AC(mob) = -12;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 9000;
 break;
case 24:
  GET_DAMROLL(mob) = 14;
  GET_HITROLL(mob) = 30;
  GET_HIT(mob) = 1960;
  GET_AC(mob) = -12;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 10000;
 break;
case 25:
  GET_DAMROLL(mob) = 15;
  GET_HITROLL(mob) = 31;
  GET_HIT(mob) = 2040;
  GET_AC(mob) = -15;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 15000;
 break;
case 26:
  GET_DAMROLL(mob) = 15;
  GET_HITROLL(mob) = 32;
  GET_HIT(mob) = 2120;
  GET_AC(mob) = -15;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 20000;
 break;
case 27:
  GET_DAMROLL(mob) = 16;
  GET_HITROLL(mob) = 33;
  GET_HIT(mob) = 2200;
  GET_AC(mob) = -18;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 25000;
 break;
case 28:
  GET_DAMROLL(mob) = 16;
  GET_HITROLL(mob) = 34;
  GET_HIT(mob) = 2280;
  GET_AC(mob) = -18;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 30000;
 break;
case 29:
  GET_DAMROLL(mob) = 17;
  GET_HITROLL(mob) = 35;
  GET_HIT(mob) = 2360;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 35000;
 break;
case 30:
  GET_DAMROLL(mob) = 17;
  GET_HITROLL(mob) = 36;
  GET_HIT(mob) = 2440;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 40000;
 break;
case 31:
  GET_DAMROLL(mob) = 18;
  GET_HITROLL(mob) = 37;
  GET_HIT(mob) = 2520;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 45000;
 break;
case 32:
  GET_DAMROLL(mob) = 18;
  GET_HITROLL(mob) = 38;
  GET_HIT(mob) = 2600;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 50000;
 break;
case 33:
  GET_DAMROLL(mob) = 19;
  GET_HITROLL(mob) = 39;
  GET_HIT(mob) = 2680;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 60000;
 break;
case 34:
  GET_DAMROLL(mob) = 19;
  GET_HITROLL(mob) = 40;
  GET_HIT(mob) = 2760;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 70000;
 break;
case 35:
  GET_DAMROLL(mob) = 20;
  GET_HITROLL(mob) = 40;
  GET_HIT(mob) = 2840;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 80000;
 break;
case 36:
  GET_DAMROLL(mob) = 21;
  GET_HITROLL(mob) = 40;
  GET_HIT(mob) = 2920;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 90000;
 break;
case 37:
  GET_DAMROLL(mob) = 22;
  GET_HITROLL(mob) = 40;
  GET_HIT(mob) = 3000;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 100000;
 break;
case 38:
  GET_DAMROLL(mob) = 23;
  GET_HITROLL(mob) = 40;
  GET_HIT(mob) = 3080;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 125000;
 break;
case 39:
  GET_DAMROLL(mob) = 24;
  GET_HITROLL(mob) = 40;
  GET_HIT(mob) = 3160;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 150000;
 break;
case 40:
  GET_DAMROLL(mob) = 25;
  GET_HITROLL(mob) = 40;
  GET_HIT(mob) = 3240;
  GET_AC(mob) = -20;
  GET_EXP(mob) = 100000;
  GET_GOLD(mob) = 200000;
 break;
default:
break;
}

switch(GET_DIFFICULTY(mob)) {
case 0:
  GET_HITROLL(mob) /= 3;
  GET_HIT(mob)  /= 8;
  GET_AC(mob) += 7;
  GET_EXP(mob) /= 3;
  GET_GOLD(mob) /= 3;
  GET_NDD(mob) = 1;
  GET_SDD(mob) -= 3;
  GET_DAMROLL(mob) /= 3;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10); 
 break;
case 1:
  GET_HITROLL(mob) /= 2;
  GET_HIT(mob) /= 4;
  GET_AC(mob) += 5;
  GET_EXP(mob) /= 2;
  GET_GOLD(mob) /= 2;
  GET_NDD(mob) = 1;
  GET_SDD(mob) -= 2;
  GET_DAMROLL(mob) /= 2;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
 break;
case 2:
  GET_HITROLL(mob) *= .75;
  GET_HIT(mob) /= 2;
  GET_AC(mob) += 5;
  GET_EXP(mob) *= .75;
  GET_GOLD(mob) *= .75;
  GET_NDD(mob) = 1;
  GET_SDD(mob) -= 1;
  GET_DAMROLL(mob) *= .75;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
 break;
case 3: // 3 is the default difficulty
  break;
case 4:
  GET_HITROLL(mob) *= 1.5;
  GET_HIT(mob) *= 1.5;
  GET_AC(mob) -= 2;
  GET_EXP(mob) *= 1.5;
  GET_GOLD(mob) *= 1.5;
  GET_NDD(mob) = 1.5;
  GET_DAMROLL(mob) *= 1.5;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
//  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;
case 5:
  GET_HITROLL(mob) *= 2;
  GET_HIT(mob) *= 2;
  GET_AC(mob) -= 2;
  GET_EXP(mob) *= 2;
  GET_GOLD(mob) *= 2;
  GET_NDD(mob) = 2;
  GET_DAMROLL(mob) *= 2;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
//  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;
case 6:
  GET_HITROLL(mob) *= 3;
  GET_HIT(mob) *= 3;
  GET_AC(mob) -= 2;
  GET_EXP(mob) *= 3;
  GET_GOLD(mob) *= 3;
  GET_NDD(mob) = 3;
  GET_DAMROLL(mob) *= 3;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
//  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;

case 7:
  GET_HITROLL(mob) *= 3.5;
  GET_HIT(mob) *= 3.5;
  GET_AC(mob) -= 2;
  GET_EXP(mob) *= 3.5;
  GET_GOLD(mob) *= 3.5;
  GET_NDD(mob) = 3.5;
  GET_DAMROLL(mob) *= 3.5;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
//  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;

case 8:
  GET_HITROLL(mob) *= 4;
  GET_HIT(mob) *= 4;
  GET_AC(mob) -= 4;
  GET_EXP(mob) *= 4;
  GET_GOLD(mob) *= 4;
  GET_NDD(mob) = 4;
  GET_DAMROLL(mob) *= 4;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;
case 9:
  GET_HITROLL(mob) *= 4.5;
  GET_HIT(mob) *= 4.5;
  GET_AC(mob) -= 2;
  GET_EXP(mob) *= 4.5;
  GET_GOLD(mob) *= 4.5;
  GET_NDD(mob) = 4.5;
  GET_DAMROLL(mob) *= 4.5;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
   SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;
case 10:
  GET_HITROLL(mob) *= 5;
  GET_HIT(mob) *= 5;
  GET_AC(mob) -= 2;
  GET_EXP(mob) *= 5;
  GET_GOLD(mob) *= 5;
  GET_NDD(mob) = 5;
  GET_DAMROLL(mob) *= 5;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;
case 11:
  GET_HITROLL(mob) *= 5.5;
  GET_HIT(mob) *= 5.5;
  GET_AC(mob) -= 2;
  GET_EXP(mob) *= 5.5;
  GET_GOLD(mob) *= 5.5;
  GET_NDD(mob) = 5.5;
  GET_DAMROLL(mob) *= 5.5;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;
case 12:
  GET_HITROLL(mob) *= 6;
  GET_HIT(mob) *= 6;
  GET_AC(mob) -= 2;
  GET_EXP(mob) *= 6;
  GET_GOLD(mob) *= 6;
  GET_NDD(mob) = 2;
  GET_DAMROLL(mob) *= 6;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;
case 13:
  GET_HITROLL(mob) *= 7;
  GET_HIT(mob) *= 7;
  GET_AC(mob) -= 2;
  GET_EXP(mob) *= 7;
  GET_GOLD(mob) *= 7;
  GET_NDD(mob) = 7;
  GET_DAMROLL(mob) *= 7;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;
case 14:
  GET_HITROLL(mob) *= 8;
  GET_HIT(mob) *= 8;
  GET_AC(mob) -= 8;
  GET_EXP(mob) *= 8;
  GET_GOLD(mob) *= 8;
  GET_NDD(mob) = 8;
  GET_DAMROLL(mob) *= 8;
  if (GET_AC(mob) < 0)
    GET_AC(mob) = MAX(GET_AC(mob), -20);
  if (GET_AC(mob) > 0)
    GET_AC(mob) - MAX(GET_AC(mob), 10);
  if (GET_HITROLL(mob) > 40)
   GET_HITROLL(mob) = 40;
  if (GET_DAMROLL(mob) > 160)
   GET_DAMROLL(mob) = 160;
  SET_BIT(MOB_FLAGS(mob), MOB_NOCHARM);
 break;
default:
break;
}

}
