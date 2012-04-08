/************************************************************************
 * OasisOLC - Objects / oedit.c					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "constants.h"
#include "shop.h"
#include "genolc.h"
#include "genobj.h"
#include "oasis.h"
#include "improved-edit.h"
#include "dg_olc.h"

/*------------------------------------------------------------------------*/

/*
 * External variable declarations.
 */

extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern obj_rnum top_of_objt;
extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct shop_data *shop_index;
extern struct attack_hit_type attack_hit_text[];
extern struct spell_info_type spell_info[];
extern struct board_info_type board_info[];
extern struct descriptor_data *descriptor_list;
extern zone_rnum real_zone_by_thing(room_vnum vznum);
extern const char *sizes[];

extern const char *sorted_item_types[];
extern const char *sorted_extra_bits[];
extern const char *sorted_apply_types[];
extern size_t apply_types_count;
extern const char *sorted_res_types[];
extern const char *sorted_affected_bits[];


/*------------------------------------------------------------------------*/

/*
 * Handy macros.
 */
#define S_PRODUCT(s, i) ((s)->producing[(i)])

/*------------------------------------------------------------------------*\
  Utility and exported functions
\*------------------------------------------------------------------------*/

void oedit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_OBJ(d), struct obj_data, 1);

  clear_object(OLC_OBJ(d));
  OLC_OBJ(d)->name = strdup("unfinished object");
  OLC_OBJ(d)->description = strdup("An unfinished object is lying here.");
  OLC_OBJ(d)->short_description = strdup("an unfinished object");
  GET_OBJ_WEAR(OLC_OBJ(d)) = ITEM_WEAR_TAKE;
  OLC_OBJ(d)->size = 0;
  OLC_VAL(d) = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  oedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void oedit_setup_existing(struct descriptor_data *d, int real_num)
{
  struct obj_data *obj;

  /*
   * Allocate object in memory.
   */
  CREATE(obj, struct obj_data, 1);
  copy_object(obj, &obj_proto[real_num]);

  /*
   * Attach new object to player's descriptor.
   */
  OLC_OBJ(d) = obj;
  OLC_VAL(d) = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  dg_olc_script_copy(d);

  oedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void oedit_save_internally(struct descriptor_data *d)
{
  int i;
  obj_rnum robj_num;
  struct descriptor_data *dsc;
  struct obj_data *obj;
  
  i = (real_object(OLC_NUM(d)) == NOTHING);

  if ((robj_num = add_object(OLC_OBJ(d), OLC_NUM(d))) == NOWHERE) {
    log("oedit_save_internally: add_object failed.");
    return;
  }

  /* Make sure scripts are updated too. - Welcor */
  /* Free old proto list  */
  if (obj_proto[robj_num].proto_script &&
      obj_proto[robj_num].proto_script != OLC_SCRIPT(d)) {
    struct trig_proto_list *proto, *fproto;
    proto = obj_proto[robj_num].proto_script;
    while (proto) {
      fproto = proto;
      proto = proto->next;
      free(fproto);
    }
  }    

  /* this will handle new instances of the object: */
  obj_proto[robj_num].proto_script = OLC_SCRIPT(d);

  /* this takes care of the objects currently in-game */
  for (obj = object_list; obj; obj = obj->next) {
    if (obj->item_number != robj_num)
      continue;
    /* remove any old scripts */
    if (SCRIPT(obj)) {
      extract_script(SCRIPT(obj));
      SCRIPT(obj) = NULL;
    }
    obj->proto_script = OLC_SCRIPT(d);
    assign_triggers(obj, OBJ_TRIGGER);
  }

  if (!i)	/* If it's not a new object, don't renumber. */
    return;

  /*
   * Renumber produce in shops being edited.
   */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_SEDIT)
      for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++)
	if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
	  S_PRODUCT(OLC_SHOP(dsc), i)++;


  /* Update other people in zedit too. From: C.Raehl 4/27/99 */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        switch (OLC_ZONE(dsc)->cmd[i].command) {
          case 'P':
            OLC_ZONE(dsc)->cmd[i].arg3 += (OLC_ZONE(dsc)->cmd[i].arg3 >= robj_num);
            /* Fall through. */
          case 'E':
          case 'G':
          case 'O':
            OLC_ZONE(dsc)->cmd[i].arg1 += (OLC_ZONE(dsc)->cmd[i].arg1 >= robj_num);
            break;
          case 'R':
            OLC_ZONE(dsc)->cmd[i].arg2 += (OLC_ZONE(dsc)->cmd[i].arg2 >= robj_num);
            break;
          default:
          break;
        }
}

/*------------------------------------------------------------------------*/

void oedit_save_to_disk(zone_rnum zone_num)
{
  save_objects(zone_num);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For container flags.
 */
void oedit_disp_container_flags_menu(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];
  
  get_char_colors(d->character);
  clear_screen(d);

  sprintbit(GET_OBJ_VAL(OLC_OBJ(d), 1), container_bits, bitbuf, sizeof(bitbuf));
  write_to_output(d, 
	  "%s1%s) CLOSEABLE\r\n"
	  "%s2%s) PICKPROOF\r\n"
	  "%s3%s) CLOSED\r\n"
	  "%s4%s) LOCKED\r\n"
	  "Container flags: %s%s%s\r\n"
	  "Enter flag, 0 to quit : ",
	  grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, bitbuf, nrm);
}

/*
 * Size menu.
 */
 void oedit_disp_size_menu(struct descriptor_data *d)
 {

    int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (counter = 0; counter < NUM_OBJ_SIZES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		sizes[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter size number : ");


 }



/*
 * For extra descriptions.
 */
void oedit_disp_extradesc_menu(struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
	  "Extra desc menu\r\n"
	  "%s1%s) Keyword: %s%s\r\n"
	  "%s2%s) Description:\r\n%s%s\r\n"
	  "%s3%s) Goto next description: %s\r\n"
	  "%s0%s) Quit\r\n"
	  "Enter choice : ",

     	  grn, nrm, yel, (extra_desc->keyword && *extra_desc->keyword) ? extra_desc->keyword : "<NONE>",
	  grn, nrm, yel, (extra_desc->description && *extra_desc->description) ? extra_desc->description : "<NONE>",
	  grn, nrm, !extra_desc->next ? "<Not set>\r\n" : "Set.", grn, nrm);
  OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/*
 * Ask for *which* apply to edit.
 */
void oedit_disp_prompt_apply_menu(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];
  int counter;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
    if (OLC_OBJ(d)->affected[counter].modifier) {
      sprinttype(OLC_OBJ(d)->affected[counter].location, apply_types, bitbuf, sizeof(bitbuf));
      write_to_output(d, " %s%d%s) %+d to %s\r\n", grn, counter + 1, nrm,
	      OLC_OBJ(d)->affected[counter].modifier, bitbuf);
    } else {
      write_to_output(d, " %s%d%s) None.\r\n", grn, counter + 1, nrm);
    }
  }
  write_to_output(d, "\r\nEnter affection to modify (0 to quit) : ");
  OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

/*
 * Ask for liquid type.
 */
void oedit_liquid_type(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_LIQ_TYPES; counter++) {
    write_to_output(d, " %s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
	    drinks[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%sEnter drink type : ", nrm);
  OLC_MODE(d) = OEDIT_VALUE_3;
}

/*
 * Display resist menu
 */
void oedit_disp_resist(struct descriptor_data *d)
{
  int i, j;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < MAX_ATTACK_TYPES; i++) {
    /* determine sorted attack type val */
    for( j = 0; j < MAX_ATTACK_TYPES; j++ )
    {
      if( strcmp(res_types[j], sorted_res_types[i]) == 0 )
        break;
    }    

    if(OLC_OBJ(d)->resist[j] == 0)
      write_to_output(d, "%s%2d%s) %-12.12s\r\n", grn, i + 1, nrm, sorted_res_types[i]);
    else
      write_to_output(d, "%s%2d%s) %-12.12s %s-set-%s\r\n", grn, i + 1, nrm, sorted_res_types[i], cyn, nrm);	
    	
  }
  write_to_output(d, "\r\nEnter resist to toggle (0 to quit) : ");
}

/*-------------------------------------------------------------------*/
/*
 * Display immune menu
 */
void oedit_disp_immune(struct descriptor_data *d)
{
  int i, j;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < MAX_ATTACK_TYPES; i++) {
    /* determine sorted attack type val */
    for( j = 0; j < MAX_ATTACK_TYPES; j++ )
    {
      if( strcmp(res_types[j], sorted_res_types[i]) == 0 )
        break;
    }    

    if(OLC_OBJ(d)->immune[j] == 0)
      write_to_output(d, "%s%2d%s) %-12.12s\r\n", grn, i + 1, nrm, sorted_res_types[i]);	
    else
      write_to_output(d, "%s%2d%s) %-12.12s %s-set-%s\r\n", grn, i + 1, nrm, sorted_res_types[i], cyn, nrm);	
    	
  }  
  write_to_output(d, "\r\nEnter immunity to toggle (0 to quit) : ");
}

/*-------------------------------------------------------------------*/
/*
 * Display vulnerable menu
 */
void oedit_disp_vulnerable(struct descriptor_data *d)
{
  int i, j;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < MAX_ATTACK_TYPES; i++) {
    /* determine sorted attack type val */
    for( j = 0; j < MAX_ATTACK_TYPES; j++ )
    {
      if( strcmp(res_types[j], sorted_res_types[i]) == 0 )
        break;
    }    

    if(OLC_OBJ(d)->vulnerable[j] == 0)
      write_to_output(d, "%s%2d%s) %-12.12s\r\n", grn, i + 1, nrm, sorted_res_types[i]);	
    else
      write_to_output(d, "%s%2d%s) %-12.12s %s-set-%s\r\n", grn, i + 1, nrm, sorted_res_types[i], cyn, nrm);	
  }  
  write_to_output(d, "\r\nEnter vulnerability to toggle (0 to quit) : ");
}

/* ask for trap type */
void oedit_trap_type(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_TRAP_TYPES; counter++) {
    write_to_output(d, " %s%2d%s) %s%-35.35s %s", grn, counter, nrm, yel,
	    traps[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%sEnter trap type : ", nrm);
  OLC_MODE(d) = OEDIT_VALUE_1;
}

void oedit_trap_range1(struct descriptor_data *d)
{
  
  get_char_colors(d->character);
  clear_screen(d);  
  write_to_output(d, "\r\nMinimum damage for damage traps\r\n"
                     "or minimum vnum for teleport traps");
  write_to_output(d, "\r\n%sEnter value  : ", nrm);
  OLC_MODE(d) = OEDIT_VALUE_2;
}

void oedit_trap_range2(struct descriptor_data *d)
{
  
  get_char_colors(d->character);
  clear_screen(d);  
  write_to_output(d, "\r\nMaximum damage for damage traps\r\n"
                     "or maximum vnum for teleport traps (0 for single room)");
  write_to_output(d, "\r\n%sEnter value  : ", nrm);
  OLC_MODE(d) = OEDIT_VALUE_3;
}

void oedit_trap_location(struct descriptor_data *d)
{
  
  get_char_colors(d->character);
  clear_screen(d);  
  write_to_output(d, "\r\nLocation to trap\r\n"
                     "Enter direction number\r\n"
                     "  0) North\r\n"
                     "  1) East\r\n"
                     "  2) South\r\n"
                     "  3) West\r\n"
                     "  4) Up\r\n"
                     "  5) Down\r\n"
                     "or object VNUM" );
  write_to_output(d, "\r\n%sEnter value  : ", nrm);
  OLC_MODE(d) = OEDIT_VALUE_4;
}


/*
 * The actual apply to set.
 */
void oedit_disp_apply_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_APPLIES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		sorted_apply_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter apply type (0 is no apply) : ");
  OLC_MODE(d) = OEDIT_APPLY;
}

/*
 * Weapon type.
 */
void oedit_disp_weapon_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ATTACK_TYPES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		attack_hit_text[counter].singular,
		!(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter weapon type : ");
}

/*
 * Spell type.
 */
void oedit_disp_spells_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_SPELLS; counter++) {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
		spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%sEnter spell choice (-1 for none) : ", nrm);
}

/*
 * Object value #1
 */
void oedit_disp_val1_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_1;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_LIGHT:
    /*
     * values 0 and 1 are unused.. jump to 2
     */
    oedit_disp_val3_menu(d);
    break;
  case ITEM_SCROLL:
  case ITEM_WAND:
  case ITEM_STAFF:
  case ITEM_POTION:
    write_to_output(d, "Spell level : ");
    break;
  case ITEM_WEAPON:
    /*
     * This doesn't seem to be used if I remembe right.
     */
    write_to_output(d, "Modifier to Hitroll : ");
    break;
  case ITEM_ARMOR:
    write_to_output(d, "Apply to AC : ");
    break;
  case ITEM_CONTAINER:
    write_to_output(d, "Max weight to contain : ");
    break;
  case ITEM_KEY:
    write_to_output(d, "Number of charges (9999 = infinite) : ");
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    write_to_output(d, "Max drink units : ");
    break;
  case ITEM_FOOD:
    write_to_output(d, "Hours to fill stomach : ");
    break;
  case ITEM_MONEY:
    write_to_output(d, "Number of gold coins : ");
    break;
  case ITEM_TRAP:
    oedit_trap_type(d);
    break;
  case ITEM_NOTE:
    /*
     * This is supposed to be language, but it's unused.
     */
    break;
  default:
    oedit_disp_menu(d);
  }
}

/*
 * Object value #2
 */
void oedit_disp_val2_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_2;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_SCROLL:
  case ITEM_POTION:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    write_to_output(d, "Max number of charges : ");
    break;
  case ITEM_WEAPON:
    write_to_output(d, "Number of damage dice : ");
    break;
  case ITEM_FOOD:
    /*
     * Values 2 and 3 are unused, jump to 4...Odd.
     */
    oedit_disp_val4_menu(d);
    break;
  case ITEM_CONTAINER:
    /*
     * These are flags, needs a bit of special handling.
     */
    oedit_disp_container_flags_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    write_to_output(d, "Initial drink units (9999 = infinite) : ");
    break;
  case ITEM_TRAP:
    oedit_trap_range1(d);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/*
 * Object value #3
 */
void oedit_disp_val3_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_3;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_LIGHT:
    write_to_output(d, "Number of hours (0 = burnt, -1 is infinite) : ");
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    write_to_output(d, "Number of charges remaining : ");
    break;
  case ITEM_WEAPON:
    write_to_output(d, "Size of damage dice : ");
    break;
  case ITEM_CONTAINER:
    write_to_output(d, "Vnum of key to open container (-1 for no key) : ");
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    oedit_liquid_type(d);
    break;
  case ITEM_TRAP:
    oedit_trap_range2(d);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/*
 * Object value #4
 */
void oedit_disp_val4_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_4;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_SCROLL:
  case ITEM_POTION:
  case ITEM_WAND:
  case ITEM_STAFF:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WEAPON:
    oedit_disp_weapon_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
  case ITEM_FOOD:
    write_to_output(d, "Poisoned (0 = not poison) : ");
    break;
  case ITEM_TRAP:
     oedit_trap_location(d);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/*
 * Object type.
 */
void oedit_disp_type_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEM_TYPES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		sorted_item_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter object type : ");
}

/*
 * Object extra flags.
 */
void oedit_disp_extra_menu(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEM_FLAGS; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		sorted_extra_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(GET_OBJ_EXTRA(OLC_OBJ(d)), extra_bits, bitbuf, sizeof(bitbuf));
  write_to_output(d, "\r\nObject flags: %s%s%s\r\n"
	  "Enter object extra flag (0 to quit) : ",
	  cyn, bitbuf, nrm);
}

/* class restrictions */
void oedit_disp_class_menu(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEMCLASS_FLAGS; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		class_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(GET_OBJ_CLASS(OLC_OBJ(d)), class_bits, bitbuf, sizeof(bitbuf));
  write_to_output(d, "\r\nClass Restrictions: %s%s%s\r\n"
	  "Enter class restriction flag (0 to quit) : ",
	  cyn, bitbuf, nrm);
}


/*
 * Object perm flags.
 */
void oedit_disp_perm_menu(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];  
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_AFF_FLAGS; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, 
          nrm, sorted_affected_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(GET_OBJ_PERM(OLC_OBJ(d)), affected_bits, bitbuf, sizeof(bitbuf));
  write_to_output(d, "\r\nObject permanent flags: %s%s%s\r\n"
          "Enter object perm flag (0 to quit) : ", cyn, bitbuf, nrm);
}

/*
 * Object wear flags.
 */
void oedit_disp_wear_menu(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEM_WEARS; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		wear_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, bitbuf, sizeof(bitbuf));
  write_to_output(d, "\r\nWear flags: %s%s%s\r\n"
	  "Enter wear flag, 0 to quit : ", cyn, bitbuf, nrm);
}

/*
 * Display main menu.
 */
void oedit_disp_menu(struct descriptor_data *d)
{
  char tbitbuf[MAX_INPUT_LENGTH], ebitbuf[MAX_INPUT_LENGTH], resistbuf[MAX_INPUT_LENGTH];
  char vulnbuf[MAX_INPUT_LENGTH], immunebuf[MAX_INPUT_LENGTH], cbitbuf[MAX_INPUT_LENGTH];
  char sizebuf[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  size_t len = 0;
  int i, nlen;
  bool resists = FALSE;

  obj = OLC_OBJ(d);
  get_char_colors(d->character);
  clear_screen(d);

  sprinttype(GET_OBJ_TYPE(obj), item_types, tbitbuf, sizeof(tbitbuf));
  sprinttype(obj->size, sizes, sizebuf, sizeof(sizebuf));
  sprintbit(GET_OBJ_EXTRA(obj), extra_bits, ebitbuf, sizeof(ebitbuf));
  sprintbit(GET_OBJ_CLASS(obj), class_bits, cbitbuf, sizeof(cbitbuf));
  write_to_output(d,
	  "-- Item number : [%s%d%s]\r\n"
	  "%s1%s) Namelist : %s%s\r\n"
	  "%s2%s) S-Desc   : %s%s\r\n"
	  "%s3%s) L-Desc   :-\r\n%s%s\r\n"
	  "%s4%s) A-Desc   :-\r\n%s%s"
	  "%s5%s) Type        : %s%s\r\n"
	  "%s6%s) Extra flags : %s%s\r\n",

	  cyn, OLC_NUM(d), nrm,
	  grn, nrm, yel, (obj->name && *obj->name) ? obj->name : STRING_UNDEFINED,
	  grn, nrm, yel, (obj->short_description && *obj->short_description) ? obj->short_description : STRING_UNDEFINED,
	  grn, nrm, yel, (obj->description && *obj->description) ? obj->description : STRING_UNDEFINED,
	  grn, nrm, yel, (obj->action_description && *obj->action_description) ? obj->action_description : "<not set>\r\n",
	  grn, nrm, cyn, tbitbuf,
	  grn, nrm, cyn, ebitbuf
	  );
  sprintbit(GET_OBJ_WEAR(obj), wear_bits, tbitbuf, sizeof(tbitbuf));
  sprintbit(GET_OBJ_PERM(obj), affected_bits, ebitbuf, sizeof(ebitbuf));
  /* to show resists that are set  Anubis */
	  for(i = 0;i < MAX_ATTACK_TYPES; i++) {
	    if (obj->resist[i] == 1) {
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
	    if (obj->immune[i] == 1) {
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
	    if (obj->vulnerable[i] == 1) {
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
	  "%s7%s) Wear flags  : %s%s\r\n"
	  "%s8%s) Weight      : %s%d\r\n"
	  "%s9%s) Cost        : %s%d\r\n"
	  "%sA%s) Cost/Day    : %s%d\r\n"
	  "%sB%s) Timer       : %s%d\r\n"
	  "%sC%s) Values      : %s%d %d %d %d\r\n"
	  "%sD%s) Applies menu\r\n"
	  "%sE%s) Extra descriptions menu\r\n"
          "%sF%s) Class Restr : %s%s\r\n"  
	  "%sI%s) Immune      : %s%s\r\n"
    "%sM%s) Min Level   : %s%d\r\n"
    "%sN%s) Max Level   : %s%d\r\n"
    "%sP%s) Perm Affects: %s%s\r\n"
    "%sR%s) Resist      : %s%s\r\n"
    "%sS%s) Script      : %s%s\r\n"
    "%sV%s) Vulnerable  : %s%s\r\n"
    "%sZ%s) Size        : %s%s\r\n"
    "%sQ%s) Quit\r\n"
	  "Enter choice : ",

	  grn, nrm, cyn, tbitbuf,
	  grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
	  grn, nrm, cyn, GET_OBJ_COST(obj),
	  grn, nrm, cyn, GET_OBJ_RENT(obj),
	  grn, nrm, cyn, GET_OBJ_TIMER(obj),
	  grn, nrm, cyn, GET_OBJ_VAL(obj, 0),
	  GET_OBJ_VAL(obj, 1),
	  GET_OBJ_VAL(obj, 2),
	  GET_OBJ_VAL(obj, 3),
	  grn, nrm, grn, nrm,
          grn, nrm, cyn, cbitbuf,
	  grn, nrm, cyn, immunebuf,
          grn, nrm, cyn, GET_OBJ_LEVEL(obj),
          grn, nrm, cyn, GET_OBJ_MAX_LEVEL(obj),
          grn, nrm, cyn, ebitbuf,
          grn, nrm, cyn, resistbuf,
          grn, nrm, cyn, obj->proto_script?"Set.":"Not Set.",
          grn, nrm, cyn, vulnbuf,
          grn, nrm, cyn, sizebuf,
	  grn, nrm
  );
  OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/***************************************************************************
 main loop (of sorts).. basically interpreter throws all input to here
 ***************************************************************************/

void oedit_parse(struct descriptor_data *d, char *arg)
{
  int number, max_val, min_val, i = -1;
  char *oldtext = NULL;

  switch (OLC_MODE(d)) {

  case OEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      oedit_save_internally(d);
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, 
              "OLC: %s edits obj %d", GET_NAME(d->character), OLC_NUM(d));
#if (OLC_AUTO_SAVE)
      oedit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
      write_to_output(d, "Object saved to disk.\r\n");
#else
      write_to_output(d, "Object saved to memory.\r\n");
#endif

      /* Fall through. */
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'a': /* abort quit */
    case 'A': 
      oedit_disp_menu(d);
      return;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Do you wish to save this object internally?\r\n");
      return;
    }

  case OEDIT_MAIN_MENU:
    /*
     * Throw us out to whichever edit mode based on user input.
     */
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) {	/* Something has been modified. */
	write_to_output(d, "Do you wish to save this object internally? : ");
	OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
      } else
	cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      write_to_output(d, "Enter namelist : ");
      OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
      break;
    case '2':
      write_to_output(d, "Enter short desc : ");
      OLC_MODE(d) = OEDIT_SHORTDESC;
      break;
    case '3':
      write_to_output(d, "Enter long desc :-\r\n| ");
      OLC_MODE(d) = OEDIT_LONGDESC;
      break;
    case '4':
      OLC_MODE(d) = OEDIT_ACTDESC;
      send_editor_help(d);
      write_to_output(d, "Enter action description:\r\n\r\n");
      if (OLC_OBJ(d)->action_description) {
	write_to_output(d, OLC_OBJ(d)->action_description);
	oldtext = strdup(OLC_OBJ(d)->action_description);
      }
      string_write(d, &OLC_OBJ(d)->action_description, MAX_MESSAGE_LENGTH, 0, oldtext);
      OLC_VAL(d) = 1;
      break;
    case '5':
      oedit_disp_type_menu(d);
      OLC_MODE(d) = OEDIT_TYPE;
      break;
    case '6':
      oedit_disp_extra_menu(d);
      OLC_MODE(d) = OEDIT_EXTRAS;
      break;
    case '7':
      oedit_disp_wear_menu(d);
      OLC_MODE(d) = OEDIT_WEAR;
      break;
    case '8':
      write_to_output(d, "Enter weight : ");
      OLC_MODE(d) = OEDIT_WEIGHT;
      break;
    case '9':
      write_to_output(d, "Enter cost : ");
      OLC_MODE(d) = OEDIT_COST;
      break;
    case 'a':
    case 'A':
      write_to_output(d, "Enter cost per day : ");
      OLC_MODE(d) = OEDIT_COSTPERDAY;
      break;
    case 'b':
    case 'B':
      write_to_output(d, "Enter timer : ");
      OLC_MODE(d) = OEDIT_TIMER;
      break;
    case 'c':
    case 'C':
      /*
       * Clear any old values  
       */
      GET_OBJ_VAL(OLC_OBJ(d), 0) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 1) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 2) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
      oedit_disp_val1_menu(d);
      break;
    case 'd':
    case 'D':
      oedit_disp_prompt_apply_menu(d);
      break;
    case 'e':
    case 'E':
      /*
       * If extra descriptions don't exist.
       */
      if (OLC_OBJ(d)->ex_description == NULL) {
	CREATE(OLC_OBJ(d)->ex_description, struct extra_descr_data, 1);
	OLC_OBJ(d)->ex_description->next = NULL;
      }
      OLC_DESC(d) = OLC_OBJ(d)->ex_description;
      oedit_disp_extradesc_menu(d);
      break;
    case 'F':
    case 'f':
      oedit_disp_class_menu(d);
      OLC_MODE(d) = OEDIT_CLASS;
      break;

    case 'i':
    case 'I':
      OLC_MODE(d) = OEDIT_IMMUNE;         
      oedit_disp_immune(d);
      return;
    case 'm':
    case 'M':
      write_to_output(d, "Enter new minimum level: ");
      OLC_MODE(d) = OEDIT_LEVEL;
      break;
    case 'n':
    case 'N':
      write_to_output(d, "Enter new maximum level: ");
      OLC_MODE(d) = OEDIT_MAX_LEVEL;
      break;
    case 'p':
    case 'P':
      oedit_disp_perm_menu(d);
      OLC_MODE(d) = OEDIT_PERM;
      break;
    case 'r':
    case 'R':
      OLC_MODE(d) = OEDIT_RESIST;         
      oedit_disp_resist(d);
      return;
    case 's':
    case 'S':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      dg_script_menu(d);
      return;
    case 'v':
    case 'V':
      OLC_MODE(d) = OEDIT_VULNERABLE;         
      oedit_disp_vulnerable(d);
      return;
    case 'z':
    case 'Z':
      OLC_MODE(d) = OEDIT_SIZE;
      oedit_disp_size_menu(d);
      return;
    default:
      oedit_disp_menu(d);
      break;
    }
    return;			/*
				 * end of OEDIT_MAIN_MENU 
				 */

  case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg)) return;
    break;


  case OEDIT_EDIT_NAMELIST:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_OBJ(d)->name)
      free(OLC_OBJ(d)->name);
    OLC_OBJ(d)->name = str_udup(arg);
    break;

  case OEDIT_SHORTDESC:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_OBJ(d)->short_description)
      free(OLC_OBJ(d)->short_description);
    OLC_OBJ(d)->short_description = str_udup(arg);
    break;

  case OEDIT_LONGDESC:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_OBJ(d)->description)
      free(OLC_OBJ(d)->description);
    OLC_OBJ(d)->description = str_udup(arg);
    break;

  case OEDIT_TYPE:
    number = atoi(arg);
    if ((number < 1) || (number >= NUM_ITEM_TYPES)) {
      write_to_output(d, "Invalid choice, try again : ");
      return;
    }
    else
    {
      /* convert the argument from the sorted item type to the actual
       * item type */
      int i;
      for( i = 0; i < NUM_ITEM_TYPES; i++ )
      {
        if( strcmp(item_types[i], sorted_item_types[number]) == 0 )
        {
          number = i;
          break;
        }
      }

      GET_OBJ_TYPE(OLC_OBJ(d)) = number;
    }
    break;

  case OEDIT_SIZE:
    number = atoi(arg);
    if ((number < 0) || (number >= NUM_OBJ_SIZES)) {
      write_to_output(d, "Invalid choice, try again : ");
      return;
    } else
      OLC_OBJ(d)->size = number;
    break;

  case OEDIT_EXTRAS:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ITEM_FLAGS)) {
      oedit_disp_extra_menu(d);
      return;
    } else if (number == 0)
      break;
    else {
      /* convert sorted extra bit value to actual bit value */
      int i;
      for( i = 0; i < extra_bits_count; i++ )
      {
        if( strcmp(extra_bits[i], sorted_extra_bits[number - 1]) == 0 )
        {
          number = i + 1;
          break;
        }
      }

      TOGGLE_BIT(GET_OBJ_EXTRA(OLC_OBJ(d)), 1ULL << (number - 1));
      oedit_disp_extra_menu(d);
      return;
    }

  case OEDIT_CLASS:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ITEMCLASS_FLAGS)) {
      oedit_disp_class_menu(d);
      return;
    } else if (number == 0)
      break;
    else {
      TOGGLE_BIT(GET_OBJ_CLASS(OLC_OBJ(d)), 1ULL << (number - 1));
      oedit_disp_class_menu(d);
      return;
    }

  case OEDIT_WEAR:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ITEM_WEARS)) {
      write_to_output(d, "That's not a valid choice!\r\n");
      oedit_disp_wear_menu(d);
      return;
    } else if (number == 0)	/* Quit. */
      break;
    else {
      TOGGLE_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << (number - 1));
      oedit_disp_wear_menu(d);
      return;
    }

  case OEDIT_WEIGHT:
    GET_OBJ_WEIGHT(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_COST:
    GET_OBJ_COST(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_COSTPERDAY:
    GET_OBJ_RENT(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_TIMER:
    GET_OBJ_TIMER(OLC_OBJ(d)) = atoi(arg);
    break;

  case OEDIT_LEVEL:
    GET_OBJ_LEVEL(OLC_OBJ(d)) = atoi(arg);
    break;
  
  case OEDIT_MAX_LEVEL:
    GET_OBJ_MAX_LEVEL(OLC_OBJ(d)) = atoi(arg);
    break;
  case OEDIT_PERM:
    if ((number = atoi(arg)) == 0)
      break;
    if (number > 0 && number <= NUM_AFF_FLAGS)
    {
      /* map the sorted value to the actual bit flag */
      int i;
      for( i = 0; i < affected_bits_count; i++ )
      {
        if( strcmp(affected_bits[i], sorted_affected_bits[number - 1]) == 0 )
        {
          number = i + 1;
          break;
        }
      }

      TOGGLE_BIT(GET_OBJ_PERM(OLC_OBJ(d)), 1ULL << (number - 1));
    }
    oedit_disp_perm_menu(d);
    return;

  case OEDIT_VALUE_1:
    /*
     * Lucky, I don't need to check any of these for out of range values.
     * Hmm, I'm not so sure - Rv
     */

    if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_MONEY || GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_KEY || GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_LIGHT)
	OLC_VAL(d) = 1;

	GET_OBJ_VAL(OLC_OBJ(d), 0) = atoi(arg);
    
	/*
     * proceed to menu 2 
     */
    oedit_disp_val2_menu(d);
    return;
  case OEDIT_VALUE_2:
    /*
     * Here, I do need to check for out of range values.
     */
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      if (number < 0 || number >= NUM_SPELLS)
	oedit_disp_val2_menu(d);
      else {
	GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
	oedit_disp_val3_menu(d);
      }
      break;
    case ITEM_CONTAINER:
      /*
       * Needs some special handling since we are dealing with flag values
       * here.
       */
      if (number < 0 || number > 4)
	oedit_disp_container_flags_menu(d);
      else if (number != 0) {
        TOGGLE_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), 1 << (number - 1));
        OLC_VAL(d) = 1;
	oedit_disp_val2_menu(d);
      } else
	oedit_disp_val3_menu(d);
      break;

    default:
      GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
      oedit_disp_val3_menu(d);
    }
    return;

  case OEDIT_VALUE_3:
    number = atoi(arg);
    /*
     * Quick'n'easy error checking.
     */
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      min_val = -1;
      max_val = NUM_SPELLS - 1;
      break;
    case ITEM_WEAPON:
      min_val = 1;
      max_val = 50;
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      min_val = 0;
      max_val = 20;
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      min_val = 0;
      max_val = NUM_LIQ_TYPES - 1;
      break;
    case ITEM_KEY:
      min_val = 0;
      max_val = 32099;
      break;
    default:
      min_val = -32000;
      max_val = 32000;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 2) = LIMIT(number, min_val, max_val);
    oedit_disp_val4_menu(d);
    return;

  case OEDIT_VALUE_4:
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      min_val = -1;
      max_val = NUM_SPELLS - 1;
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      min_val = 1;
      max_val = NUM_SPELLS - 1;
      break;
    case ITEM_WEAPON:
      min_val = 0;
      max_val = NUM_ATTACK_TYPES - 1;
      break;
    default:
      min_val = -32000;
      max_val = 32000;
      break;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 3) = LIMIT(number, min_val, max_val);
    break;

  case OEDIT_PROMPT_APPLY:
    if ((number = atoi(arg)) == 0)
      break;
    else if (number < 0 || number > MAX_OBJ_AFFECT) {
      oedit_disp_prompt_apply_menu(d);
      return;
    }
    OLC_VAL(d) = number - 1;
    OLC_MODE(d) = OEDIT_APPLY;
    oedit_disp_apply_menu(d);
    return;

  case OEDIT_APPLY:
    if ((number = atoi(arg)) == 0) {
      OLC_OBJ(d)->affected[OLC_VAL(d)].location = 0;
      OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = 0;
      oedit_disp_prompt_apply_menu(d);
    } else if (number < 0 || number >= NUM_APPLIES)
      oedit_disp_apply_menu(d);
    else {
      int counter;
      /* map sorted apply value to actual apply value */
      int i;
      for( i = 0; i < apply_types_count; i++ )
      {
        if( strcmp(apply_types[i], sorted_apply_types[number]) == 0 )
        {
          number = i;
          break;
        }
      }

      /* add in check here if already applied.. deny builders another */
      if (GET_LEVEL(d->character) < LVL_IMPL) {
        for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
          if (OLC_OBJ(d)->affected[counter].location == number) {
            write_to_output(d, "Object already has that apply.");
            return;
          }
        }
      }

      OLC_OBJ(d)->affected[OLC_VAL(d)].location = number;
      write_to_output(d, "Modifier : ");
      OLC_MODE(d) = OEDIT_APPLYMOD;
    }
    return;
    
 /*-----------------------------------------------------------------*/    
 case OEDIT_RESIST:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= MAX_ATTACK_TYPES) {
      /* map sorted res type to actual res type */
      int j;
      for( j = 0; j < MAX_ATTACK_TYPES; j++ )
      {
        if( strcmp(res_types[j], sorted_res_types[i - 1]) == 0 )
        {
          i = j + 1;
          break;
        }
      }

      if ( OLC_OBJ(d)->resist[i - 1] == 1)
        OLC_OBJ(d)->resist[i - 1] = 0;
      else
        OLC_OBJ(d)->resist[i - 1] = 1;
    }
    oedit_disp_resist(d);
    return;
/*-----------------------------------------------------------------*/   
 case OEDIT_IMMUNE:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= MAX_ATTACK_TYPES) {
      /* map sorted res type to actual res type */
      int j;
      for( j = 0; j < MAX_ATTACK_TYPES; j++ )
      {
        if( strcmp(res_types[j], sorted_res_types[i - 1]) == 0 )
        {
          i = j + 1;
          break;
        }
      }

      if ( OLC_OBJ(d)->immune[i - 1] == 1)
        OLC_OBJ(d)->immune[i - 1] = 0;
      else
        OLC_OBJ(d)->immune[i - 1] = 1;
    }
    oedit_disp_immune(d);
    return;
/*-----------------------------------------------------------------*/    
 case OEDIT_VULNERABLE:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= MAX_ATTACK_TYPES) {
      /* map sorted res type to actual res type */
      int j;
      for( j = 0; j < MAX_ATTACK_TYPES; j++ )
      {
        if( strcmp(res_types[j], sorted_res_types[i - 1]) == 0 )
        {
          i = j + 1;
          break;
        }
      }

      if ( OLC_OBJ(d)->vulnerable[i - 1] == 1)
        OLC_OBJ(d)->vulnerable[i - 1] = 0;
      else
        OLC_OBJ(d)->vulnerable[i - 1] = 1;
    }
    oedit_disp_vulnerable(d);
    return;
/*-----------------------------------------------------------------*/

  case OEDIT_APPLYMOD:
    OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = atoi(arg);
    oedit_disp_prompt_apply_menu(d);
    return;

  case OEDIT_EXTRADESC_KEY:
    if (genolc_checkstring(d, arg)) {
      if (OLC_DESC(d)->keyword)
        free(OLC_DESC(d)->keyword);
      OLC_DESC(d)->keyword = str_udup(arg);
    }
    oedit_disp_extradesc_menu(d);
    return;

  case OEDIT_EXTRADESC_MENU:
    switch ((number = atoi(arg))) {
    case 0:
      if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
	struct extra_descr_data **tmp_desc;

	if (OLC_DESC(d)->keyword)
	  free(OLC_DESC(d)->keyword);
	if (OLC_DESC(d)->description)
	  free(OLC_DESC(d)->description);

	/*
	 * Clean up pointers  
	 */
	for (tmp_desc = &(OLC_OBJ(d)->ex_description); *tmp_desc; tmp_desc = &((*tmp_desc)->next)) {
	  if (*tmp_desc == OLC_DESC(d)) {
	    *tmp_desc = NULL;
	    break;
	  }
	}
	free(OLC_DESC(d));
      }
    break;

    case 1:
      OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
      write_to_output(d, "Enter keywords, separated by spaces :-\r\n| ");
      return;

    case 2:
      OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
      send_editor_help(d);
      write_to_output(d, "Enter the extra description:\r\n\r\n");
      if (OLC_DESC(d)->description) {
	write_to_output(d, OLC_DESC(d)->description);
	oldtext = strdup(OLC_DESC(d)->description);
      }
      string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, oldtext);
      OLC_VAL(d) = 1;
      return;

    case 3:
      /*
       * Only go to the next description if this one is finished.
       */
      if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) {
	struct extra_descr_data *new_extra;

	if (OLC_DESC(d)->next)
	  OLC_DESC(d) = OLC_DESC(d)->next;
	else {	/* Make new extra description and attach at end. */
	  CREATE(new_extra, struct extra_descr_data, 1);
	  OLC_DESC(d)->next = new_extra;
	  OLC_DESC(d) = OLC_DESC(d)->next;
	}
      }
      /*
       * No break - drop into default case.
       */
    default:
      oedit_disp_extradesc_menu(d);
      return;
    }
    break;
  default:
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Reached default case in oedit_parse()!");
    write_to_output(d, "Oops...\r\n");
    break;
  }

  /*
   * If we get here, we have changed something.  
   */
  OLC_VAL(d) = 1;
  oedit_disp_menu(d);
}

void oedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
  case OEDIT_ACTDESC:
    oedit_disp_menu(d);
    break;
  case OEDIT_EXTRADESC_DESCRIPTION:
    oedit_disp_extradesc_menu(d);
    break;
  }
}
