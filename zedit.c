/************************************************************************
 * OasisOLC - Zones / zedit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
dtester
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "db.h"
#include "constants.h"
#include "genolc.h"
#include "genzon.h"
#include "oasis.h"
#include "dg_scripts.h"

/*-------------------------------------------------------------------*/

extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct index_data *obj_index;
extern struct obj_data *obj_proto;
extern struct descriptor_data *descriptor_list;
extern struct index_data **trig_index;
int number, subcmd;

extern int copy_object(struct obj_data *to, struct obj_data *from);

/*-------------------------------------------------------------------*/

/*
 * Nasty internal macros to clean up the code.
 */
#define MYCMD		(OLC_ZONE(d)->cmd[subcmd])
#define OLC_CMD(d)	(OLC_ZONE(d)->cmd[OLC_VAL(d)])

/* Prototypes. */
int start_change_command(struct descriptor_data *d, int pos);

/*-------------------------------------------------------------------*/

void zedit_setup(struct descriptor_data *d, int room_num)
{
  struct zone_data *zone;
  int subcmd = 0, count = 0, cmd_room = -1;

  /*
   * Allocate one scratch zone structure.  
   */
  CREATE(zone, struct zone_data, 1);

  /*
   * Copy all the zone header information over.
   */
  zone->name = strdup(zone_table[OLC_ZNUM(d)].name);
  zone->lifespan = zone_table[OLC_ZNUM(d)].lifespan;
#if _CIRCLEMUD >= CIRCLEMUD_VERSION(3,0,21)
  zone->bot = zone_table[OLC_ZNUM(d)].bot;
#endif
  zone->top = zone_table[OLC_ZNUM(d)].top;
  zone->reset_mode = zone_table[OLC_ZNUM(d)].reset_mode;
  /*
   * The remaining fields are used as a 'has been modified' flag
   */
  zone->number = 0;	/* Header information has changed.	*/
  zone->age = 0;	/* The commands have changed.		*/

  /*
   * Start the reset command list with a terminator.
   */
  CREATE(zone->cmd, struct reset_com, 1);
  zone->cmd[0].command = 'S';

  /*
   * Add all entries in zone_table that relate to this room.
   */
  while (ZCMD(OLC_ZNUM(d), subcmd).command != 'S') {
    switch (ZCMD(OLC_ZNUM(d), subcmd).command) {
    case 'M':
    case 'O':
    case 'T':
    case 'V':
      cmd_room = ZCMD(OLC_ZNUM(d), subcmd).arg3;
      break;
    case 'D':
    case 'R':
      cmd_room = ZCMD(OLC_ZNUM(d), subcmd).arg1;
      break;
    default:
      break;
    }
    if (cmd_room == room_num) {
      add_cmd_to_list(&(zone->cmd), &ZCMD(OLC_ZNUM(d), subcmd), count);
      count++;
    }
    subcmd++;
  }

  OLC_ZONE(d) = zone;
  /*
   * Display main menu.
   */
  zedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Create a new zone.
 */
void zedit_new_zone(struct char_data *ch, zone_vnum vzone_num, room_vnum bottom, room_vnum top)
{
  int result;
  const char *error;
  struct descriptor_data *dsc;

  if ((result = create_new_zone(vzone_num, bottom, top, &error)) < 0) {
    write_to_output(ch->desc, "%s", error);
    return;
  }

  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    switch (STATE(dsc)) {
      case CON_REDIT:
        OLC_ROOM(dsc)->zone += (OLC_ZNUM(dsc) >= result);
        /* Fall through. */
      case CON_ZEDIT:
      case CON_MEDIT:
      case CON_SEDIT:
      case CON_OEDIT:
      case CON_TRIGEDIT:
        OLC_ZNUM(dsc) += (OLC_ZNUM(dsc) >= result);
        break;
      default:
        break;
    }
  }

  mudlog(BRF, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s creates new zone #%d", GET_NAME(ch), vzone_num);
  write_to_output(ch->desc, "Zone create successfully\r\n");
}

/*-------------------------------------------------------------------*/

/*
 * Save all the information in the player's temporary buffer back into
 * the current zone table.
 */
void zedit_save_internally(struct descriptor_data *d)
{
  int	mobloaded = FALSE,
	newcmdpos= 0,
	subcmd = 0;
  room_rnum room_num = real_room(OLC_NUM(d));
  long objloaded = 0;
  struct obj_data *obj;

  if (room_num == NOWHERE) {
    log("SYSERR: zedit_save_internally: OLC_NUM(d) room %d not found.", OLC_NUM(d));
    return;
  }

  remove_room_zone_commands(OLC_ZNUM(d), room_num);

  obj = NULL;

  /*
   * Now add all the entries in the players descriptor list  
   */
  for (subcmd = 0; MYCMD.command != 'S'; subcmd++, newcmdpos++) {
    add_cmd_to_list(&(zone_table[OLC_ZNUM(d)].cmd), &MYCMD, newcmdpos);

    /*
     * This was bugged - The list was shrinking as we copied it, and 
     * we ended up trashing the zone command list. 
     * Thanks to Chris Gilbert (chris@buzzbee.freeserve.co.uk) for clarifying this.
     */

    /*
     * Since Circle does not keep track of what rooms the 'G', 'E', and
     * 'P' commands are exitted in, but OasisOLC groups zone commands
     * by rooms, this creates interesting problems when builders use these
     * commands without loading a mob or object first.  This fix prevents such
     * commands from being saved and 'wandering' through the zone command
     * list looking for mobs/objects to latch onto.
     * C.Raehl 4/27/99
     */
    switch (MYCMD.command) {
      case 'G':
      case 'E':
        CREATE(obj, obj_data, 1);
        if (obj != NULL)
        {
            copy_object(obj, &obj_proto[MYCMD.arg1]);
            if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
                objloaded = MYCMD.arg1;
            free_obj(obj);
        }

        if (mobloaded)
          break;
        write_to_output(d, "Equip/Give command not saved since no mob was loaded first.\r\n");
        remove_cmd_from_list(&(zone_table[OLC_ZNUM(d)].cmd), newcmdpos);
        newcmdpos--;
        continue;
      case 'P':
        if (objloaded == MYCMD.arg3) /* is this item loaded ? */
          break;
        write_to_output(d, "Put command not saved since the container was not loaded first.\r\n");
        remove_cmd_from_list(&(zone_table[OLC_ZNUM(d)].cmd), newcmdpos);
        newcmdpos--;
        continue;
      case 'M':
        mobloaded = TRUE;
        break;
      case 'O':
        objloaded = MYCMD.arg1;  /* save the rnum */
        break;
      default:
        mobloaded = FALSE;
        objloaded = 0;    
        break;
    }
  }

  /*
   * Finally, if zone headers have been changed, copy over  
   */
  if (OLC_ZONE(d)->number) {
    free(zone_table[OLC_ZNUM(d)].name);
    zone_table[OLC_ZNUM(d)].name = strdup(OLC_ZONE(d)->name);
#if _CIRCLEMUD >= CIRCLEMUD_VERSION(3,0,21)
    zone_table[OLC_ZNUM(d)].bot = OLC_ZONE(d)->bot;
#endif
    zone_table[OLC_ZNUM(d)].top = OLC_ZONE(d)->top;
    zone_table[OLC_ZNUM(d)].reset_mode = OLC_ZONE(d)->reset_mode;
    zone_table[OLC_ZNUM(d)].lifespan = OLC_ZONE(d)->lifespan;
  }
  add_to_save_list(zone_table[OLC_ZNUM(d)].number, SL_ZON);
}

/*-------------------------------------------------------------------*/

void zedit_save_to_disk(zone_rnum rzone)
{
  save_zone(rzone);
}

/*-------------------------------------------------------------------*/

/*
 * Error check user input and then setup change  
 */
int start_change_command(struct descriptor_data *d, int pos)
{
  int subcmd = 0;

  /*
   * Error check to ensure users hasn't given too large an index  
   */
  while (MYCMD.command != 'S')
    subcmd++;

  if (pos < 0 || pos >= subcmd)
    return 0;

  /*
   * Ok, let's get editing.
   */
  OLC_VAL(d) = pos;
  return 1;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * the main menu 
 */
void zedit_disp_menu(struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  int subcmd = 0, room, counter = 0;

  struct zone_data *zone;
  zone = OLC_ZONE(d);

  get_char_colors(d->character);
  clear_screen(d);
  room = real_room(OLC_NUM(d));

  /*
   * Menu header  
   */
  sprintf(buf,
	  "Room number: %s%d%s		Room zone: %s%d\r\n"
	  "%sZ%s) Zone name      : %s%s\r\n"
	  "%sL%s) Lifespan       : %s%d minutes\r\n"
#if _CIRCLEMUD >= CIRCLEMUD_VERSION(3,0,21)
	  "%sB%s) Bottom of zone : %s%d\r\n"
#endif
	  "%sT%s) Top of zone    : %s%d\r\n"
	  "%sR%s) Reset Mode     : %s%s%s\r\n"
          "[Command list]\r\n",

	  cyn, OLC_NUM(d), nrm,
	  cyn, zone_table[OLC_ZNUM(d)].number,
	  grn, nrm, yel, OLC_ZONE(d)->name ? OLC_ZONE(d)->name : "<NONE!>",
	  grn, nrm, yel, OLC_ZONE(d)->lifespan,
#if _CIRCLEMUD >= CIRCLEMUD_VERSION(3,0,21)
	  grn, nrm, yel, OLC_ZONE(d)->bot,
#endif
	  grn, nrm, yel, OLC_ZONE(d)->top,
	  grn, nrm,
          yel,
          OLC_ZONE(d)->reset_mode ? ((OLC_ZONE(d)->reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset",
          nrm
      	  );

  /*
   * Print the commands for this room into display buffer.
   */
  while (MYCMD.command != 'S') {
    /*
     * Translate what the command means.
     */
    switch (MYCMD.command) {
    case 'M':
      sprintf(buf2, "%sLoad %s [%s%d%s], Max : %d Chance : %d%%",
              MYCMD.if_flag ? " then " : "",
              mob_proto[MYCMD.arg1].player.short_descr, cyn,
              mob_index[MYCMD.arg1].vnum, yel, MYCMD.arg2, 100 - MYCMD.arg4
              );
      break;
    case 'G':
      sprintf(buf2, "%sGive it %s [%s%d%s], Max : %d Chance : %d%%",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      MYCMD.arg2, 100 - MYCMD.arg4
	      );
      break;
    case 'O':
      sprintf(buf2, "%sLoad %s [%s%d%s], Max : %d Chance : %d%%",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      MYCMD.arg2, 100 - MYCMD.arg4
	      );
      break;
    case 'E':
      sprintf(buf2, "%sEquip with %s [%s%d%s], %s, Max : %d Chance : %d%%",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      equipment_types[MYCMD.arg3],
	      MYCMD.arg2, 100 - MYCMD.arg4
	      );
      break;
    case 'P':
      sprintf(buf2, "%sPut %s [%s%d%s] in %s [%s%d%s], Max : %d Chance : %d%%",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      obj_proto[MYCMD.arg3].short_description,
	      cyn, obj_index[MYCMD.arg3].vnum, yel,
	      MYCMD.arg2, 100 - MYCMD.arg4
	      );
      break;
    case 'R':
      sprintf(buf2, "%sRemove %s [%s%d%s] from room. Chance : %d%%",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg2].short_description,
	      cyn, obj_index[MYCMD.arg2].vnum, yel, 100 - MYCMD.arg4
	      );
      break;
    case 'D':
      sprintf(buf2, "%sSet door %s as %s.",
	      MYCMD.if_flag ? " then " : "",
	      dirs[MYCMD.arg2],
	      MYCMD.arg3 ? ((MYCMD.arg3 == 1) ? "closed" : "locked") : "open"
	      );
      break;
    case 'T':
      sprintf(buf2, "%sAttach trigger %s%s%s [%s%d%s] to %s",
        MYCMD.if_flag ? " then " : "",
        cyn, trig_index[real_trigger(MYCMD.arg2)]->proto->name, yel, 
        cyn, MYCMD.arg2, yel,
        ((MYCMD.arg1 == MOB_TRIGGER) ? "mobile" :
          ((MYCMD.arg1 == OBJ_TRIGGER) ? "object" :
            ((MYCMD.arg1 == WLD_TRIGGER)? "room" : "????"))));
      break;
    case 'V':
      sprintf(buf2, "%sAssign global %s:%d to %s = %s",
        MYCMD.if_flag ? " then " : "",
        MYCMD.sarg1, MYCMD.arg2,
        ((MYCMD.arg1 == MOB_TRIGGER) ? "mobile" :
          ((MYCMD.arg1 == OBJ_TRIGGER) ? "object" :
            ((MYCMD.arg1 == WLD_TRIGGER)? "room" : "????"))),
        MYCMD.sarg2);
      break;
	case 'Z':
      break;
	default:
      strcpy(buf2, "<Unknown Command>");
      break;
    }
    /*
     * Build the display buffer for this command  
     */
    sprintf(buf1, "%s%d - %s%s\r\n", nrm, counter++, yel, buf2);
    strcat(buf, buf1);
    subcmd++;
  }
  /*
   * Finish off menu  
   */
  sprintf(buf1,
	  "%s%d - <END OF LIST>\r\n"
	  "%sN%s) New command.       %sE%s) Edit a command.\r\n"
	  "%sD%s) Delete a command.  %sC%s) Change a command's chance of happening.\r\n"
	  "%sQ%s) Quit\r\nEnter your choice : ",
	  nrm, counter, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm
	  );

  strcat(buf, buf1);
  write_to_output(d, "%s", buf);

  OLC_MODE(d) = ZEDIT_MAIN_MENU;
}

/*-------------------------------------------------------------------*/

/* ZONE FLAG */

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/

/*
 * Print the command type menu and setup response catch. 
 */
void zedit_disp_comtype(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
	"%sM%s) Load Mobile to room             %sO%s) Load Object to room\r\n"
	"%sE%s) Equip mobile with object        %sG%s) Give an object to a mobile\r\n"
	"%sP%s) Put object in another object    %sD%s) Open/Close/Lock a Door\r\n"
	"%sR%s) Remove an object from the room\r\n"
        "%sT%s) Assign a trigger                %sV%s) Set a global variable\r\n"
        "What sort of command will this be? : ",
	grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm,
	grn, nrm, grn, nrm, grn, nrm, grn, nrm
	);
  OLC_MODE(d) = ZEDIT_COMMAND_TYPE;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg1 and set
 * up the input catch clause  
 */
void zedit_disp_arg1(struct descriptor_data *d)
{
  switch (OLC_CMD(d).command) {
  case 'M':
    write_to_output(d, "Input mob's vnum : ");
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'O':
  case 'E':
  case 'P':
  case 'G':
    write_to_output(d, "Input object vnum : ");
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'D':
  case 'R':
    /*
     * Arg1 for these is the room number, skip to arg2  
     */
    OLC_CMD(d).arg1 = real_room(OLC_NUM(d));
    zedit_disp_arg2(d);
    break;
  case 'T':
  case 'V':
    write_to_output(d, "Input trigger type (0:mob, 1:obj, 2:room) :");
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  default:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_disp_arg1(): Help!");
    write_to_output(d, "Oops...\r\n");
    return;
  }
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg2 and set
 * up the input catch clause.
 */
void zedit_disp_arg2(struct descriptor_data *d)
{
  int i;

  switch (OLC_CMD(d).command) {
  case 'M':
  case 'O':
  case 'E':
  case 'P':
  case 'G':
    write_to_output(d, "Input the maximum number that can exist on the mud : ");
    break;
  case 'D':
    for (i = 0; *dirs[i] != '\n'; i++) {
      write_to_output(d, "%d) Exit %s.\r\n", i, dirs[i]);
    }
    write_to_output(d, "Enter exit number for door : ");
    break;
  case 'R':
    write_to_output(d, "Input object's vnum : ");
    break;
  case 'T':
    write_to_output(d, "Enter the trigger VNum : ");
    break;
  case 'V':
    write_to_output(d, "Global's context (0 for none) : ");
    break;
  default:
    /*
     * We should never get here, but just in case...
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_disp_arg2(): Help!");
    write_to_output(d, "Oops...\r\n");
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG2;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg3 and set
 * up the input catch clause.
 */
void zedit_disp_arg3(struct descriptor_data *d)
{
  int i = 0;

  switch (OLC_CMD(d).command) {
  case 'E':
    while (*equipment_types[i] != '\n') {
      write_to_output(d, "%2d) %26.26s %2d) %26.26s\r\n", i,
	   equipment_types[i], i + 1, (*equipment_types[i + 1] != '\n') ?
	      equipment_types[i + 1] : "");
      if (*equipment_types[i + 1] != '\n')
	i += 2;
      else
	break;
    }
    write_to_output(d, "Location to equip : ");
    break;
  case 'P':
    write_to_output(d, "Virtual number of the container : ");
    break;
  case 'D':
    write_to_output(d, 	"0)  Door open\r\n"
		"1)  Door closed\r\n"
		"2)  Door locked\r\n"
		"Enter state of the door : ");
    break;
  case 'V':
  case 'T':
  case 'M':
  case 'O':
  case 'R':
  case 'G':
  default:
    /*
     * We should never get here, just in case.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_disp_arg3(): Help!");
    write_to_output(d, "Oops...\r\n");
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG3;
}

/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void zedit_parse(struct descriptor_data *d, char *arg)
{
  int pos, i = 0;

  switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
  case ZEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      /*
       * Save the zone in memory, hiding invisible people.
       */
      zedit_save_internally(d);
#if (OLC_AUTO_SAVE)
      write_to_output(d, "Saving zone info to disk.\r\n");
      zedit_save_to_disk(OLC_ZNUM(d));
#else
      write_to_output(d, "Saving zone info in memory.\r\n");
#endif
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits zone info for room %d.", GET_NAME(d->character), OLC_NUM(d));
      /* FALL THROUGH */
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Do you wish to save the zone info? : ");
      break;
    }
    break;
   /* End of ZEDIT_CONFIRM_SAVESTRING */

/*-------------------------------------------------------------------*/
  case ZEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_ZONE(d)->age || OLC_ZONE(d)->number) {
	write_to_output(d, "Do you wish to save the changes to the zone info? (y/n) : ");
	OLC_MODE(d) = ZEDIT_CONFIRM_SAVESTRING;
      } else {
	write_to_output(d, "No changes made.\r\n");
	cleanup_olc(d, CLEANUP_ALL);
      }
      break;
    case 'n':
    case 'N':
      /*
       * New entry.
       */
      write_to_output(d, "What number in the list should the new command be? : ");
      OLC_MODE(d) = ZEDIT_NEW_ENTRY;
      break;
    case 'e':
    case 'E':
      /*
       * Change an entry.
       */
      write_to_output(d, "Which command do you wish to change? : ");
      OLC_MODE(d) = ZEDIT_CHANGE_ENTRY;
      break;
    case 'd':
    case 'D':
      /*
       * Delete an entry.
       */
      write_to_output(d, "Which command do you wish to delete? : ");
      OLC_MODE(d) = ZEDIT_DELETE_ENTRY;
      break;
    case 'z':
    case 'Z':
      /*
       * Edit zone name.
       */
      write_to_output(d, "Enter new zone name : ");
      OLC_MODE(d) = ZEDIT_ZONE_NAME;
      break;
#if _CIRCLEMUD >= CIRCLEMUD_VERSION(3,0,21)
    case 'b':
    case 'B':
      /*
       * Edit bottom of zone.
       */
      if (GET_LEVEL(d->character) < LVL_IMPL)
	zedit_disp_menu(d);
      else {
	write_to_output(d, "Enter new bottom of zone : ");
	OLC_MODE(d) = ZEDIT_ZONE_BOT;
      }
      break;
#endif
    case 't':
    case 'T':
      /*
       * Edit top of zone.
       */
      if (GET_LEVEL(d->character) < LVL_IMPL)
	zedit_disp_menu(d);
      else {
	write_to_output(d, "Enter new top of zone : ");
	OLC_MODE(d) = ZEDIT_ZONE_TOP;
      }
      break;
    case 'l':
    case 'L':
      /*
       * Edit zone lifespan.
       */
      write_to_output(d, "Enter new zone lifespan : ");
      OLC_MODE(d) = ZEDIT_ZONE_LIFE;
      break;
    case 'r':
    case 'R':
      /*
       * Edit zone reset mode.
       */
      write_to_output(d, "\r\n"
		"0) Never reset\r\n"
		"1) Reset only when no players in zone\r\n"
		"2) Normal reset\r\n"
		"Enter new zone reset type : ");
      OLC_MODE(d) = ZEDIT_ZONE_RESET;
      break;

    case 'c':
    case 'C':
      /*
       *      Change loadchance 
       */
       
      write_to_output(d, " \r\nWhich command? ");
      OLC_MODE(d) = ZEDIT_PROB;
      break;
    
    default:
      zedit_disp_menu(d);
      break;
    }
    break;
    /* End of ZEDIT_MAIN_MENU */

/*-------------------------------------------------------------------*/
  case ZEDIT_NEW_ENTRY:
    /*
     * Get the line number and insert the new line.
     */
    pos = atoi(arg);
    if (isdigit(*arg) && new_command(OLC_ZONE(d), pos)) {
      if (start_change_command(d, pos)) {
	zedit_disp_comtype(d);
	OLC_ZONE(d)->age = 1;
      }
    } else
      zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_DELETE_ENTRY:
    /*
     * Get the line number and delete the line.
     */
    pos = atoi(arg);
    if (isdigit(*arg)) {
      delete_command(OLC_ZONE(d), pos);
      OLC_ZONE(d)->age = 1;
    }
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_CHANGE_ENTRY:
    /*
     * Parse the input for which line to edit, and goto next quiz.
     */
    pos = atoi(arg);
    if (isdigit(*arg) && start_change_command(d, pos)) {
      zedit_disp_comtype(d);
      OLC_ZONE(d)->age = 1;
    } else
      zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_COMMAND_TYPE:
    /*
     * Parse the input for which type of command this is, and goto next
     * quiz.
     */
    OLC_CMD(d).command = toupper(*arg);
    if (!OLC_CMD(d).command || (strchr("MOPEDGRTV", OLC_CMD(d).command) == NULL)) {  
      write_to_output(d, "Invalid choice, try again : ");
    } else {
      if (OLC_VAL(d)) {	/* If there was a previous command. */
        if (OLC_CMD(d).command == 'T' || OLC_CMD(d).command == 'V') {
          OLC_CMD(d).if_flag = 1;
          zedit_disp_arg1(d);
        } else {
	write_to_output(d, "Is this command dependent on the success of the previous one? (y/n)\r\n");
	OLC_MODE(d) = ZEDIT_IF_FLAG;
        }
      } else {	/* 'if-flag' not appropriate. */
	OLC_CMD(d).if_flag = 0;
	zedit_disp_arg1(d);
      }
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_IF_FLAG:
    /*
     * Parse the input for the if flag, and goto next quiz.
     */
    switch (*arg) {
    case 'y':
    case 'Y':
      OLC_CMD(d).if_flag = 1;
      break;
    case 'n':
    case 'N':
      OLC_CMD(d).if_flag = 0;
      break;
    default:
      write_to_output(d, "Try again : ");
      return;
    }
    zedit_disp_arg1(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_PROB:
    /*
     *       Which command to change loadchance for? :/
     */
     
    pos = atoi(arg);
    if (isdigit(*arg) && start_change_command(d, pos)) {
      OLC_MODE(d) = ZEDIT_PROB2;
      OLC_ZONE(d)->age = 1;
      write_to_output(d, "\r\nChance of loading (0-100) (input 0 for 'always load'): "); 
      break;
    } else
      write_to_output(d, "\r\nInvalid choice.\r\n\r\n");
      zedit_disp_menu(d);
    break;
/*-------------------------------------------------------------------*/
  case ZEDIT_PROB2:
    /*
     *       Change the loadchance
     */    
     
    OLC_CMD(d).arg4 = 100 - atoi(arg);
    OLC_MODE(d) = ZEDIT_MAIN_MENU;
    zedit_disp_menu(d);   
    break;    
    
 
 /*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG1:
    /*
     * Parse the input for arg1, and goto next quiz.
     */
    if (!isdigit(*arg)) {
      write_to_output(d, "Must be a numeric value, try again : ");
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
      if ((pos = real_mobile(atoi(arg))) != NOBODY) {
	OLC_CMD(d).arg1 = pos;
	zedit_disp_arg2(d);
      } else
	write_to_output(d, "That mobile does not exist, try again : ");
      break;
    case 'O':
    case 'P':
    case 'E':
    case 'G':
      if ((pos = real_object(atoi(arg))) != NOTHING) {
	OLC_CMD(d).arg1 = pos;
	zedit_disp_arg2(d);
      } else
	write_to_output(d, "That object does not exist, try again : ");
      break;
    case 'T':
    case 'V':
      if (atoi(arg)<MOB_TRIGGER || atoi(arg)>WLD_TRIGGER)
        write_to_output(d, "Invalid input.");
      else {
       OLC_CMD(d).arg1 = atoi(arg);
        zedit_disp_arg2(d);
      }
      break;
    case 'D':
    case 'R':
    default:
      /*
       * We should never get here.
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_parse(): case ARG1: Ack!");
      write_to_output(d, "Oops...\r\n");
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG2:
    /*
     * Parse the input for arg2, and goto next quiz.
     */
    if (!isdigit(*arg)) {
      write_to_output(d, "Must be a numeric value, try again : ");
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
    case 'O':
      OLC_CMD(d).arg2 = atoi(arg);
      OLC_CMD(d).arg3 = real_room(OLC_NUM(d));
      zedit_disp_menu(d);
      break;
    case 'G':
      OLC_CMD(d).arg2 = atoi(arg);
      zedit_disp_menu(d);
      break;
    case 'P':
    case 'E':
      OLC_CMD(d).arg2 = atoi(arg);
      zedit_disp_arg3(d);
      break;
    case 'V':
      OLC_CMD(d).arg2 = atoi(arg);
      OLC_CMD(d).arg3 = real_room(OLC_NUM(d));
      write_to_output(d, "Enter the global name : ");
      OLC_MODE(d) = ZEDIT_SARG1;
      break;
    case 'T':
      if (real_trigger(atoi(arg)) != NOTHING) {
        OLC_CMD(d).arg2 = atoi(arg);
        OLC_CMD(d).arg3 = real_room(OLC_NUM(d));
        zedit_disp_menu(d);
      } else
        write_to_output(d, "That trigger does not exist, try again : ");
      break;
    case 'D':
      pos = atoi(arg);
      /*
       * Count directions.
       */
      if (pos < 0 || pos > NUM_OF_DIRS)
	write_to_output(d, "Try again : ");
      else {
	OLC_CMD(d).arg2 = pos;
	zedit_disp_arg3(d);
      }
      break;
    case 'R':
      if ((pos = real_object(atoi(arg))) != NOTHING) {
	OLC_CMD(d).arg2 = pos;
	zedit_disp_menu(d);
      } else
	write_to_output(d, "That object does not exist, try again : ");
      break;
    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_parse(): case ARG2: Ack!");
      write_to_output(d, "Oops...\r\n");
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG3:
    /*
     * Parse the input for arg3, and go back to main menu.
     */
    if (!isdigit(*arg)) {
      write_to_output(d, "Must be a numeric value, try again : ");
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'E':
      pos = atoi(arg);
      /*
       * Count number of wear positions.  We could use NUM_WEARS, this is
       * more reliable.
       */
      while (*equipment_types[i] != '\n')
	i++;
      if (pos < 0 || pos > i)
	write_to_output(d, "Try again : ");
      else {
	OLC_CMD(d).arg3 = pos;
	zedit_disp_menu(d);
      }
      break;
    case 'P':
      if ((pos = real_object(atoi(arg))) != NOTHING) {
	OLC_CMD(d).arg3 = pos;
	zedit_disp_menu(d);
      } else
	write_to_output(d, "That object does not exist, try again : ");
      break;
    case 'D':
      pos = atoi(arg);
      if (pos < 0 || pos > 2)
	write_to_output(d, "Try again : ");
      else {
	OLC_CMD(d).arg3 = pos;
	zedit_disp_menu(d);
      }
      break;
    case 'M':
    case 'O':
    case 'G':
    case 'R':
    case 'T':
    case 'V':
    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_parse(): case ARG3: Ack!");
      write_to_output(d, "Oops...\r\n");
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_SARG1:
    if (strlen(arg)) {
      OLC_CMD(d).sarg1 = strdup(arg);
      OLC_MODE(d) = ZEDIT_SARG2;
      write_to_output(d, "Enter the global value : ");
    } else
      write_to_output(d, "Must have some name to assign : ");
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_SARG2:
    if (strlen(arg)) {
      OLC_CMD(d).sarg2 = strdup(arg);
      zedit_disp_menu(d);
    } else
      write_to_output(d, "Must have some value to set it to :");
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_NAME:
    /*
     * Add new name and return to main menu.
     */
    if (genolc_checkstring(d, arg)) {
      if (OLC_ZONE(d)->name)
        free(OLC_ZONE(d)->name);
      else
        log("SYSERR: OLC: ZEDIT_ZONE_NAME: no name to free!");
      OLC_ZONE(d)->name = strdup(arg);
      OLC_ZONE(d)->number = 1;
    }
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_RESET:
    /*
     * Parse and add new reset_mode and return to main menu.
     */
    pos = atoi(arg);
    if (!isdigit(*arg) || pos < 0 || pos > 2)
      write_to_output(d, "Try again (0-2) : ");
    else {
      OLC_ZONE(d)->reset_mode = pos;
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_LIFE:
    /*
     * Parse and add new lifespan and return to main menu.
     */
    pos = atoi(arg);
    if (!isdigit(*arg) || pos < 0 || pos > 240)
      write_to_output(d, "Try again (0-240) : ");
    else {
      OLC_ZONE(d)->lifespan = pos;
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
    }
    break;

/*-------------------------------------------------------------------*/
#if _CIRCLEMUD >= CIRCLEMUD_VERSION(3,0,21)
  case ZEDIT_ZONE_BOT:
    /*
     * Parse and add new bottom room in zone and return to main menu.
     */
    if (OLC_ZNUM(d) == 0)
      OLC_ZONE(d)->bot = LIMIT(atoi(arg), 0, OLC_ZONE(d)->top);
    else
      OLC_ZONE(d)->bot = LIMIT(atoi(arg), zone_table[OLC_ZNUM(d) - 1].top + 1, OLC_ZONE(d)->top);
    OLC_ZONE(d)->number = 1;
    zedit_disp_menu(d);
    break;
#endif

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_TOP:
    /*
     * Parse and add new top room in zone and return to main menu.
     */
    if (OLC_ZNUM(d) == top_of_zone_table)
      OLC_ZONE(d)->top = LIMIT(atoi(arg), genolc_zonep_bottom(OLC_ZONE(d)), 32000);
    else
      OLC_ZONE(d)->top = LIMIT(atoi(arg), genolc_zonep_bottom(OLC_ZONE(d)), genolc_zone_bottom(OLC_ZNUM(d) + 1) - 1);
    OLC_ZONE(d)->number = 1;
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/

/* FLAGS HERE */

/*-------------------------------------------------------------------*/
  default:
    /*
     * We should never get here, but just in case...
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: zedit_parse(): Reached default case!");
    write_to_output(d, "Oops...\r\n");
    break;
  }
}

/*
 * End of parse_zedit()  
 */
