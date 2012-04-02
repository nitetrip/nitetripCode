/************************************************************************
 *  OasisOLC - Rooms / redit.c					v2.0	*
 *  Original author: Levork						*
 *  Copyright 1996 Harvey Gilpin					*
 *  Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "genolc.h"
#include "genwld.h"
#include "oasis.h"
#include "improved-edit.h"
#include "dg_olc.h"
#include "constants.h"

/*------------------------------------------------------------------------*/
char buf[MAX_INPUT_LENGTH];


/*
 * External data structures.
 */
extern struct room_data *world;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern const char *room_bits[];
extern const char *sorted_room_bits[];
extern const char *sector_types[];
extern const char *sizes[];
extern const char *exit_bits[];
extern struct zone_data *zone_table;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern room_rnum r_newbie_start_room;
extern room_rnum r_sorin_start_room;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum newbie_start_room;
extern room_vnum sorin_start_room;
extern struct descriptor_data *descriptor_list;
extern zone_rnum real_zone_by_thing(room_vnum vznum);

char      *delete_doubledollar(char *string);

/*------------------------------------------------------------------------*/


/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

ACMD(do_room_copy)
{
   room_data *room_src, *room_dst;
   int room_num, j, buf_num;
   zone_rnum dst_zone;
   struct descriptor_data *dsc;
   char buf[MAX_INPUT_LENGTH];
     
   one_argument(argument, buf);
   
   if (!*buf) {
     send_to_char(ch, "Usage: rclone <target room>\r\n");
     return;
   }

   if (real_room((buf_num = atoi(buf))) != NOWHERE) {
     send_to_char(ch, "That room already exist!\r\n");
     return;
   }

   if ((dst_zone = real_zone_by_thing(buf_num)) == NOWHERE) {
     send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
     return;
   }
    
   if (!can_edit_zone(ch, zone_table[dst_zone].number, SCMD_OASIS_REDIT) ||
       !can_edit_zone(ch, world[IN_ROOM(ch)].zone, SCMD_OASIS_REDIT)) {
     send_to_char(ch, "You may only copy rooms within your designated zone(s)!\r\n");
     return;
   }
   
   
   room_src = &world[IN_ROOM(ch)];
   CREATE(room_dst, struct room_data, 1);

   room_dst->zone = dst_zone;
 
   /*
   * Allocate space for all strings.
   */
   send_to_char(ch, "Cloning room....\r\n");
   
   room_dst->name = str_udup(world[IN_ROOM(ch)].name);
   room_dst->description = str_udup(world[IN_ROOM(ch)].description);
   room_dst->description = str_udup(world[IN_ROOM(ch)].description);
   room_dst->number = buf_num;
   room_dst->room_flags = ROOM_FLAGS(IN_ROOM(ch));
   room_dst->size = world[IN_ROOM(ch)].size;
  /*
   * Extra descriptions, if necessary.
   */
  
  send_to_char(ch, "Cloning extra descriptions....\r\n");
  if (world[IN_ROOM(ch)].ex_description) {
    struct extra_descr_data *tdesc, *temp, *temp2;
    CREATE(temp, struct extra_descr_data, 1);

    room_dst->ex_description = temp;
    for (tdesc = world[IN_ROOM(ch)].ex_description; tdesc; tdesc = tdesc->next) {
      temp->keyword = strdup(tdesc->keyword);
      temp->description = strdup(tdesc->description);
      if (tdesc->next) {
	CREATE(temp2, struct extra_descr_data, 1);
	temp->next = temp2;
	temp = temp2;
      } else
	temp->next = NULL;
    }
  }
   /*
    * Now save the room in the right place:
    */ 
  send_to_char(ch, "Saving new room...\r\n");

  if ((room_num = add_room(room_dst)) < 0) {
    send_to_char(ch, "Something went wrong...\r\n");
    log("SYSERR: do_room_copy: Something failed! (%d)", room_num);
    return;
  }
    /* Idea contributed by C.Raehl 4/27/99 */
  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    if (dsc == ch->desc)
      continue;

    if (STATE(dsc) == CON_ZEDIT) {
      for (j = 0; OLC_ZONE(dsc)->cmd[j].command != 'S'; j++)
        switch (OLC_ZONE(dsc)->cmd[j].command) {
          case 'O':
          case 'M':
            OLC_ZONE(dsc)->cmd[j].arg3 += (OLC_ZONE(dsc)->cmd[j].arg3 >= room_num);
            break;
          case 'D':
            OLC_ZONE(dsc)->cmd[j].arg2 += (OLC_ZONE(dsc)->cmd[j].arg2 >= room_num);
            /* Fall through */
          case 'R':
            OLC_ZONE(dsc)->cmd[j].arg1 += (OLC_ZONE(dsc)->cmd[j].arg1 >= room_num);
            break;
          }
    } else if (STATE(dsc) == CON_REDIT) {
      for (j = 0; j < NUM_OF_DIRS; j++)
        if (OLC_ROOM(dsc)->dir_option[j])
          if (OLC_ROOM(dsc)->dir_option[j]->to_room >= room_num)
            OLC_ROOM(dsc)->dir_option[j]->to_room++;
    }
  }
  add_to_save_list(real_zone_by_thing(atoi(buf)), SL_WLD);
  redit_save_to_disk(real_zone_by_thing(atoi(buf)));
  send_to_char(ch, "Room cloned to %d.\r\nAll Done.\r\n", buf_num);
    
}

ACMD(do_dig)
{

  int direction, opposite, to_room, buf_num;
  room_data *room_src, *room_dst;
  struct room_direction_data *exit, *exit2;
  zone_rnum real_zone = 0;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2) {
    send_to_char(ch, "Usage: dig <direction> <room vnum> - to create an exit\r\n"
                     "       dig <direction> -1     - to delete an exit\r\n");
    return;
  }

  buf_num = atoi(buf2);
  if (buf_num == 0) {
     send_to_char(ch, "Invalid room vnum!\r\n");
     return;}

  /* May the char alter this room ?*/
  if ( !can_edit_zone(ch,
                      zone_table[real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))].number,
                      SCMD_OASIS_REDIT )) {
    send_to_char(ch, "You may only dig within your designated zone!\r\n");
    return;
  }

  /* skip target room check if is this a 'remove exit' call */
  if (buf_num >= 0) {
    /* May the char alter the target room ?*/
    if ( !can_edit_zone(ch, 
                        zone_table[real_zone_by_thing(buf_num)].number,
                        SCMD_OASIS_REDIT )) {
      send_to_char(ch, "You may only dig within your designated zone!\r\n");
      return;
    }
  }
  
  if ((direction = search_block(buf, dirs, FALSE)) == -1) {
    send_to_char(ch, "No such direction!\r\n");
    return;
  }

  opposite = rev_dir[direction];
  room_src = &world[IN_ROOM(ch)];

  /* remove exit - added by Welcor 220500 */
  if ((buf_num < 0) && (room_src->dir_option[direction])) {
    if (room_src->dir_option[direction]->general_description) {
      free(room_src->dir_option[direction]->general_description);
      room_src->dir_option[direction]->general_description = NULL;
    }
    if (room_src->dir_option[direction]->keyword) {
      free(room_src->dir_option[direction]->keyword);
      room_src->dir_option[direction]->keyword = NULL;
    }
    room_src->dir_option[direction] = NULL;
    send_to_char(ch, "Exit removed!\r\n");
    real_zone = real_zone_by_thing(world[IN_ROOM(ch)].number);
    add_to_save_list(zone_table[real_zone].number, SL_WLD);
    redit_save_to_disk(real_zone);
    return;
  }

  if ((to_room = real_room(buf_num)) == NOWHERE) {
    send_to_char(ch, "Target room not found - aborting.\r\n");
    return;
  }

  room_dst = &world[to_room];

  if (room_src->dir_option[direction]) {
    send_to_char(ch, "An exit already exist in that direction!\r\n");
    return;
  } else if (room_dst->dir_option[opposite]) {
    send_to_char(ch, "The target room already has an exit that way!\r\n");
    return;
  } else {
    CREATE(exit, struct room_direction_data, 1);
    room_src->dir_option[direction] = exit;
    exit->to_room = to_room;
    send_to_char(ch, "Making exit to the %s leading to %s\r\n", buf, buf2);

    CREATE(exit2, struct room_direction_data, 1);
    room_dst->dir_option[opposite] = exit2;
    exit2->to_room = IN_ROOM(ch);
    send_to_char(ch, "and opposite exit, leading back here.\r\n");

    real_zone = real_zone_by_thing(world[IN_ROOM(ch)].number);
    add_to_save_list(zone_table[real_zone].number, SL_WLD);
    redit_save_to_disk(real_zone);
  
    /* Only save twice if it is two different zones */
    if (real_zone_by_thing(world[IN_ROOM(ch)].number)!=real_zone_by_thing(world[to_room].number)) {
      real_zone = real_zone_by_thing(world[to_room].number);
      add_to_save_list(zone_table[real_zone].number, SL_WLD);
      redit_save_to_disk(real_zone);
    }
  }  
}

/****************************************************************************
* BuildWalk - OasisOLC Extension by D. Tyler Barnes                         *
****************************************************************************/

/* For buildwalk. Finds the next free vnum in the zone */
room_vnum redit_find_new_vnum(zone_rnum zone) {

  room_vnum vnum;
  room_rnum rnum = real_room((vnum = zone_table[zone].bot));

  if (rnum != NOWHERE) {
    for(; world[rnum].number <= vnum; rnum++, vnum++)
      if (vnum > zone_table[zone].top)
        return(NOWHERE);
  }
  return(vnum);

}

int buildwalk(struct char_data *ch, int dir) {

  struct room_data *room;
  room_vnum vnum;
  room_rnum rnum;
  char buf[MAX_STRING_LENGTH];

  if (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_BUILDWALK) ||
      !can_edit_zone(ch, zone_table[world[ch->in_room].zone].number, SCMD_OASIS_REDIT)) 
    return (0);

  if ((vnum = redit_find_new_vnum(world[ch->in_room].zone)) == NOWHERE) {
    send_to_char(ch, "No free vnums are available in this zone!\r\n");
    return (0);
  }

  /* Set up data for add_room function */
  CREATE(room, struct room_data, 1);
  room->name = strdup("New BuildWalk Room");
  sprintf(buf, "This unfinished room was created by %s.\r\n", GET_NAME(ch));
  room->description = strdup(buf);
  room->number = vnum;
  room->zone = world[ch->in_room].zone;

  /* Add the room */
  add_room(room);

  /* Link rooms */
  CREATE(EXIT(ch, dir), struct room_direction_data, 1);
  EXIT(ch, dir)->to_room = (rnum = real_room(vnum));
  CREATE(world[rnum].dir_option[rev_dir[dir]], struct room_direction_data, 1);
  world[rnum].dir_option[rev_dir[dir]]->to_room = ch->in_room;

  /* Memory cleanup */
  free(room->name);
  free(room->description);
  free(room);

  /* Report room creation to user */
  send_to_char(ch, "%sRoom #%d created by BuildWalk.%s\r\n",
                   yel, vnum, nrm);
  return(1);
}

void redit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_ROOM(d), struct room_data, 1);

  OLC_ROOM(d)->name = strdup("An unfinished room");
  OLC_ROOM(d)->description = strdup("You are in an unfinished room.\r\n");
  OLC_ROOM(d)->number = NOWHERE;
  OLC_ROOM(d)->min_level = 0; // 0 means there is no min/max level
  OLC_ROOM(d)->max_level = 0;
  OLC_ITEM_TYPE(d) = WLD_TRIGGER;
  OLC_ROOM(d)->size = 0;
  redit_disp_menu(d);
  OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void redit_setup_existing(struct descriptor_data *d, int real_num)
{
  struct room_data *room;
  struct trig_proto_list *proto, *fproto;
  int counter;

  /*
   * Build a copy of the room for editing.
   */
  CREATE(room, struct room_data, 1);

  *room = world[real_num];
  /*
   * Allocate space for all strings.
   */
  room->name = str_udup(world[real_num].name);
  room->description = str_udup(world[real_num].description);
  room->description = str_udup(world[real_num].description);

  /*
   * Exits - We allocate only if necessary.
   */
  for (counter = 0; counter < NUM_OF_DIRS; counter++) {
    if (world[real_num].dir_option[counter]) {
      CREATE(room->dir_option[counter], struct room_direction_data, 1);

      /*
       * Copy the numbers over.
       */
      *room->dir_option[counter] = *world[real_num].dir_option[counter];
      /*
       * Allocate the strings.
       */
      if (world[real_num].dir_option[counter]->general_description)
        room->dir_option[counter]->general_description = strdup(world[real_num].dir_option[counter]->general_description);
      if (world[real_num].dir_option[counter]->keyword)
        room->dir_option[counter]->keyword = strdup(world[real_num].dir_option[counter]->keyword);
    }
  }

  /*
   * Extra descriptions, if necessary.
   */
  if (world[real_num].ex_description) {
    struct extra_descr_data *tdesc, *temp, *temp2;
    CREATE(temp, struct extra_descr_data, 1);

    room->ex_description = temp;
    for (tdesc = world[real_num].ex_description; tdesc; tdesc = tdesc->next) {
      temp->keyword = strdup(tdesc->keyword);
      temp->description = strdup(tdesc->description);
      if (tdesc->next) {
	CREATE(temp2, struct extra_descr_data, 1);
	temp->next = temp2;
	temp = temp2;
      } else
	temp->next = NULL;
    }
  }

  if (SCRIPT(&world[real_num]))
    script_copy(room, &world[real_num], WLD_TRIGGER);

  proto = world[real_num].proto_script;
  while (proto) {
    CREATE(fproto, struct trig_proto_list, 1);
    fproto->vnum = proto->vnum;
    if (room->proto_script==NULL)
      room->proto_script = fproto;
    proto = proto->next;
    fproto = fproto->next; /* NULL */
  }
  /*
   * Attach copy of room to player's descriptor.
   */
  OLC_ROOM(d) = room;
  OLC_VAL(d) = 0;
  OLC_ITEM_TYPE(d) = WLD_TRIGGER;
  dg_olc_script_copy(d);
  redit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void redit_save_internally(struct descriptor_data *d)
{
  int j, room_num, new_room = FALSE;
  struct descriptor_data *dsc;
  
  if (OLC_ROOM(d)->number == NOWHERE) {
    new_room = TRUE;
    OLC_ROOM(d)->number = OLC_NUM(d);
  }
  /* FIXME: Why is this not set elsewhere? */
  OLC_ROOM(d)->zone = OLC_ZNUM(d);

  if ((room_num = add_room(OLC_ROOM(d))) == NOWHERE) {
    write_to_output(d, "Something went wrong...\r\n");
    log("SYSERR: redit_save_internally: Something failed! (%d)", room_num);
    return;
  }

  /* Update script info for this room */
  /* Free old proto list */
  if (world[room_num].proto_script &&
      world[room_num].proto_script != OLC_SCRIPT(d)) {
    struct trig_proto_list *proto, *fproto;
    proto = world[room_num].proto_script;
    while (proto) {
      fproto = proto;
      proto = proto->next;
      free(fproto);
    }
  }    
       
  world[room_num].proto_script = OLC_SCRIPT(d);
  assign_triggers(&world[room_num], WLD_TRIGGER);
  
  /* Don't adjust numbers on a room update. */
  if (!new_room)
    return;

  /* Idea contributed by C.Raehl 4/27/99 */
  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    if (dsc == d)
      continue;

    if (STATE(dsc) == CON_ZEDIT) {
      for (j = 0; OLC_ZONE(dsc)->cmd[j].command != 'S'; j++)
        switch (OLC_ZONE(dsc)->cmd[j].command) {
          case 'O':
          case 'M':
          case 'T':
          case 'V':
            OLC_ZONE(dsc)->cmd[j].arg3 += (OLC_ZONE(dsc)->cmd[j].arg3 >= room_num);
            break;
          case 'D':
            OLC_ZONE(dsc)->cmd[j].arg2 += (OLC_ZONE(dsc)->cmd[j].arg2 >= room_num);
            /* Fall through */
          case 'R':
            OLC_ZONE(dsc)->cmd[j].arg1 += (OLC_ZONE(dsc)->cmd[j].arg1 >= room_num);
            break;
          }
    } else if (STATE(dsc) == CON_REDIT) {
      for (j = 0; j < NUM_OF_DIRS; j++)
        if (OLC_ROOM(dsc)->dir_option[j])
          if (OLC_ROOM(dsc)->dir_option[j]->to_room >= room_num)
            OLC_ROOM(dsc)->dir_option[j]->to_room++;
    }
  }
}

/*------------------------------------------------------------------------*/

void redit_save_to_disk(zone_vnum zone_num)
{
  save_rooms(zone_num);		/* :) */
}

/*------------------------------------------------------------------------*/

void free_room(struct room_data *room)
{
  int i;
  struct extra_descr_data *tdesc, *next;

  if (room->name)
    free(room->name);
  if (room->description)
    free(room->description);

  /*
   * Free exits.
   */
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (room->dir_option[i]) {
      if (room->dir_option[i]->general_description)
	free(room->dir_option[i]->general_description);
      if (room->dir_option[i]->keyword)
	free(room->dir_option[i]->keyword);
      free(room->dir_option[i]);
    }
  }

  /*
   * Free extra descriptions.
   */
  for (tdesc = room->ex_description; tdesc; tdesc = next) {
    next = tdesc->next;
    if (tdesc->keyword)
      free(tdesc->keyword);
    if (tdesc->description)
      free(tdesc->description);
    free(tdesc);
  }
  free(room);	/* XXX ? */
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For extra descriptions.
 */
void redit_disp_extradesc_menu(struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);
  struct room_data *room;
  room = OLC_ROOM(d);
  clear_screen(d);
  write_to_output(d,
	  "%s1%s) Keyword: %s%s\r\n"
	  "%s2%s) Description:\r\n%s%s\r\n"
	  "%s3%s) Pain room message: %s%s\r\n"
          "%s4%s) Pain room damage: %s%d\r\n"
          "%s5%s) Pain rate (in secs): %s%d\r\n"
          "%s6%s) No Magic message to caster: %s%s\r\n"
          "%s7%s) No magic message to room (use $n for the caster): %s%s\r\n"   
          "%s8%s) Goto next description: ",
	  grn, nrm, yel, extra_desc->keyword ? extra_desc->keyword : "<NONE>",
	  grn, nrm, yel, extra_desc->description ? extra_desc->description : "<NONE>",
          grn, nrm, yel, room->pain_message ? room->pain_message : "<NONE>",
          grn, nrm, yel, room->pain_damage,
          grn, nrm, yel, room->pain_rate,
          grn, nrm, yel, room->nomagic_message_caster,
           grn, nrm, yel, room->nomagic_message_room,
          grn, nrm
	  );

  write_to_output(d, !extra_desc->next ? "<NOT SET>\r\n" : "Set.\r\n");
  write_to_output(d, "Enter choice (0 to quit) : ");
  OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

/*
 * For exits.
 */
void redit_disp_exit_menu(struct descriptor_data *d)
{
  /*
   * if exit doesn't exist, alloc/create it
   */
  if (OLC_EXIT(d) == NULL)
    CREATE(OLC_EXIT(d), struct room_direction_data, 1);

  /*
   * Weird door handling! 
   */
  if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR) && !IS_SET(OLC_EXIT(d)->exit_info, EX_HIDDEN)) {
    if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
      strcpy(buf, "Pickproof");
    else
      strcpy(buf, "Is a door");
  } else
    if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR) && 
	!IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
	strcpy(buf, "Hidden door");
    else if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
	strcpy(buf, "Hidden pickproof door");
    else
	strcpy(buf, "No door");

  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
	  "%s1%s) Exit to     : %s%d\r\n"
	  "%s2%s) Description :-\r\n%s%s\r\n"
	  "%s3%s) Door name   : %s%s\r\n"
	  "%s4%s) Key         : %s%d\r\n"
	  "%s5%s) Door flags  : %s%s\r\n"
	  "%s6%s) Purge exit.\r\n"
	  "Enter choice, 0 to quit : ",

	  grn, nrm, cyn, OLC_EXIT(d)->to_room != -1 ? world[OLC_EXIT(d)->to_room].number : -1,
	  grn, nrm, yel, OLC_EXIT(d)->general_description ? OLC_EXIT(d)->general_description : "<NONE>",
	  grn, nrm, yel, OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>",
	  grn, nrm, cyn, OLC_EXIT(d)->key,
	  grn, nrm, cyn, buf, grn, nrm
	  );

  OLC_MODE(d) = REDIT_EXIT_MENU;
}

/*
 * For exit flags.
 */
void redit_disp_exit_flag_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  write_to_output(d, "%s0%s) No door\r\n"
	  "%s1%s) Closeable door\r\n"
	  "%s2%s) Pickproof\r\n"
	  "%s3%s) Hidden door\r\n"
	  "%s4%s) Hidden pickproof door\r\n"
	  "Enter choice : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm);
}

/*
 * For room flags.
 */
void redit_disp_flag_menu(struct descriptor_data *d)
{
  char bitbuf[MAX_INPUT_LENGTH];
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (counter = 0; counter < room_bits_count; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		sorted_room_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(OLC_ROOM(d)->room_flags, room_bits, bitbuf, sizeof(bitbuf));
  write_to_output(d, "\r\nRoom flags: %s%s%s\r\n"
	  "Enter room flags, 0 to quit : ", cyn, bitbuf, nrm);
  OLC_MODE(d) = REDIT_FLAGS;
}

/*
 * For sector type.
 */
void redit_disp_sector_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(d);
  for (counter = 0; counter < NUM_ROOM_SECTORS; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		sector_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter sector type : ");
  OLC_MODE(d) = REDIT_SECTOR;
}

/*
 * Size menu.
 */

 void redit_disp_size_menu(struct descriptor_data *d)
 {
    int counter, columns = 0;

  clear_screen(d);
  for (counter = 0; counter < NUM_ROOM_SIZES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		sizes[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter size : ");
  OLC_MODE(d) = REDIT_SIZE;

 }

/*
 * The main menu.
 */
void redit_disp_menu(struct descriptor_data *d)
{
  char rbitbuf[MAX_INPUT_LENGTH], sbitbuf[MAX_INPUT_LENGTH], sizebuf[MAX_INPUT_LENGTH];
  struct room_data *room;

  get_char_colors(d->character);
  clear_screen(d);
  room = OLC_ROOM(d);

  sprintbit(room->room_flags, room_bits, rbitbuf, sizeof(rbitbuf));
  sprinttype(room->sector_type, sector_types, sbitbuf, sizeof(sbitbuf));
  sprinttype(room->size, sizes, sizebuf, sizeof(sizebuf));
  write_to_output(d,
	  "-- Room number : [%s%d%s]  	Room zone: [%s%d%s]\r\n"
	  "%s1%s) Name        : %s%s\r\n"
	  "%s2%s) Description :\r\n%s%s"
	  "%s3%s) Room flags  : %s%s\r\n"
          "%s4%s) Sector type : %s%s\r\n"
	  "%s5%s) Exit north  : %s%d\r\n"
	  "%s6%s) Exit east   : %s%d\r\n"
	  "%s7%s) Exit south  : %s%d\r\n"
	  "%s8%s) Exit west   : %s%d\r\n"
	  "%s9%s) Exit up     : %s%d\r\n"
	  "%sA%s) Exit down   : %s%d\r\n"
	  "%sB%s) Extra descriptions menu\r\n"
          "%sC%s) Room Light  : %s%d\r\n"
	  "%sS%s) Script      : %s%s\r\n"
          "%sZ%s) Size        : %s%s\r\n"
          "%sM%s) Min Level   : %s%2d  %sN%s) Message: %s%s\r\n"
          "%sO%s) Max Level   : %s%2d  %sP%s) Message: %s%s\r\n"
	  "%sQ%s) Quit\r\n"
	  "Enter choice : ",

	  cyn, OLC_NUM(d), nrm,
	  cyn, zone_table[OLC_ZNUM(d)].number, nrm,
	  grn, nrm, yel, room->name,
	  grn, nrm, yel, room->description,
	  grn, nrm, cyn, rbitbuf,
	  grn, nrm, cyn, sbitbuf,
	  grn, nrm, cyn,
	  room->dir_option[NORTH] && room->dir_option[NORTH]->to_room != -1 ?
	  world[room->dir_option[NORTH]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[EAST] && room->dir_option[EAST]->to_room != -1 ?
	  world[room->dir_option[EAST]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[SOUTH] && room->dir_option[SOUTH]->to_room != -1 ?
	  world[room->dir_option[SOUTH]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[WEST] && room->dir_option[WEST]->to_room != -1 ?
	  world[room->dir_option[WEST]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[UP] && room->dir_option[UP]->to_room != -1 ? 
	  world[room->dir_option[UP]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[DOWN] && room->dir_option[DOWN]->to_room != -1 ?
	  world[room->dir_option[DOWN]->to_room].number : -1,
	  grn, nrm,
          grn, nrm, cyn, room->nat_light,
	  grn, nrm, cyn, room->proto_script?"Set.":"Not Set.",
          grn, nrm, cyn, sizebuf,
          grn, nrm, cyn, room->min_level, grn, nrm, cyn, room->min_level_message,
          grn, nrm, cyn, room->max_level, grn, nrm, cyn, room->max_level_message,
	  grn, nrm
	  );

  OLC_MODE(d) = REDIT_MAIN_MENU;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse(struct descriptor_data *d, char *arg)
{
  int number;
  char *oldtext = NULL;

  switch (OLC_MODE(d)) {
  case REDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      redit_save_internally(d);
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits room %d.", GET_NAME(d->character), OLC_NUM(d));
#if (OLC_AUTO_SAVE)
      redit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
      write_to_output(d, "Room saved to disk.\r\n");
#else
      write_to_output(d, "Room saved to memory.\r\n");
#endif
      /*
       * Do NOT free strings! Just the room structure.
       */
      cleanup_olc(d, CLEANUP_STRUCTS);
      break;
    case 'n':
    case 'N':
      /*
       * Free everything up, including strings, etc.
       */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\nDo you wish to save this room internally? : ");
      break;
    }
    return;

  case REDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) { /* Something has been modified. */
	write_to_output(d, "Do you wish to save this room internally? : ");
	OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
      } else
	cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      write_to_output(d, "Enter room name:-\r\n] ");
      OLC_MODE(d) = REDIT_NAME;
      break;
    case '2':
      OLC_MODE(d) = REDIT_DESC;
      clear_screen(d);
      send_editor_help(d);
      write_to_output(d, "Enter room description:\r\n\r\n");

      if (OLC_ROOM(d)->description) {
	write_to_output(d, OLC_ROOM(d)->description);
	oldtext = strdup(OLC_ROOM(d)->description);
      }
      string_write(d, &OLC_ROOM(d)->description, MAX_ROOM_DESC, 0, oldtext);
      OLC_VAL(d) = 1;
      break;
    case '3':
      redit_disp_flag_menu(d);
      break;
    case '4':
      redit_disp_sector_menu(d);
      break;
    case '5':
      OLC_VAL(d) = NORTH;
      redit_disp_exit_menu(d);
      break;
    case '6':
      OLC_VAL(d) = EAST;
      redit_disp_exit_menu(d);
      break;
    case '7':
      OLC_VAL(d) = SOUTH;
      redit_disp_exit_menu(d);
      break;
    case '8':
      OLC_VAL(d) = WEST;
      redit_disp_exit_menu(d);
      break;
    case '9':
      OLC_VAL(d) = UP;
      redit_disp_exit_menu(d);
      break;
    case 'a':
    case 'A':
      OLC_VAL(d) = DOWN;
      redit_disp_exit_menu(d);
      break;
    case 'b':
    case 'B':
      /*
       * If the extra description doesn't exist.
       */
      if (!OLC_ROOM(d)->ex_description)
	CREATE(OLC_ROOM(d)->ex_description, struct extra_descr_data, 1);
      OLC_DESC(d) = OLC_ROOM(d)->ex_description;
      redit_disp_extradesc_menu(d);
      break;
    case 'c':
    case 'C':
      write_to_output(d, "Room light is currently at: %s%2d%s  [-100 to 100 scale]\r\n", cyn, OLC_ROOM(d)->nat_light, nrm);
      write_to_output(d, "\r\nEnter new value for room light : ");
      OLC_MODE(d) = REDIT_ROOM_LIGHT;
      break;
    case 'o':
    case 'O':
      write_to_output(d, "\r\nEnter new Max Level for room: ");
      OLC_MODE(d) = REDIT_ROOM_MAX_LEVEL;
      break;
    case 'p':
    case 'P':
      write_to_output(d, "\r\nEnter new Max Level Message for room: ");
      OLC_MODE(d) = REDIT_ROOM_MAX_LEVEL_MSG;
      break;
    case 'm':
    case 'M':
      write_to_output(d, "\r\nEnter new Min Level for room: ");
      OLC_MODE(d) = REDIT_ROOM_MIN_LEVEL;
      break;
    case 'n':
    case 'N':
      write_to_output(d, "\r\nEnter new Min Level Message for room: ");
      OLC_MODE(d) = REDIT_ROOM_MIN_LEVEL_MSG;
      break;
    case 's':
    case 'S':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      dg_script_menu(d);
      return;
    case 'z':
    case 'Z':
      redit_disp_size_menu(d);
      break;
    default:
      write_to_output(d, "Invalid choice!");
      redit_disp_menu(d);
      break;
    }
    return;


   case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg)) return;
    break;

   case REDIT_NAME:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_ROOM(d)->name)
      free(OLC_ROOM(d)->name);
    arg[MAX_ROOM_NAME - 1] = '\0';
    OLC_ROOM(d)->name = str_udup(arg);
    break;
  case REDIT_ROOM_PAINDAM:
        number = atoi(arg);
       OLC_ROOM(d)->pain_damage = number;
       write_to_output(d, "Pain room damage set to to: %d\r\n", number);
       redit_disp_extradesc_menu(d);
       return;
  case REDIT_ROOM_PAINRATE:
       number = atoi(arg);
       OLC_ROOM(d)->pain_rate = number;
       write_to_output(d, "Pain room rate set to : %d secs\r\n", number);
       redit_disp_extradesc_menu(d);
       return;
  case REDIT_ROOM_PAINMSG:
       OLC_ROOM(d)->pain_message = str_udup(arg);
       write_to_output(d, "Pain room message set to : %s\r\n", OLC_ROOM(d)->pain_message);
       redit_disp_extradesc_menu(d);
       return;
  case REDIT_ROOM_NOMAGIC_MSG_CASTER:
       OLC_ROOM(d)->nomagic_message_caster = str_udup(arg);
       delete_doubledollar(OLC_ROOM(d)->nomagic_message_caster);
       write_to_output(d, "Caster NO MAGIC message set to : %s\r\n", OLC_ROOM(d)->nomagic_message_caster);
       redit_disp_extradesc_menu(d);
       break;
    case REDIT_ROOM_NOMAGIC_MSG_ROOM:
       OLC_ROOM(d)->nomagic_message_room = str_udup(arg);
       delete_doubledollar(OLC_ROOM(d)->nomagic_message_room);
       write_to_output(d, "Room NO MAGIC  message set to : %s\r\n", OLC_ROOM(d)->nomagic_message_room);
       redit_disp_extradesc_menu(d);
       break;
  case REDIT_ROOM_LIGHT:
    number = atoi(arg);
    if (number < MIN_ROOM_LIGHT || number > MAX_ROOM_LIGHT) {
      write_to_output(d, "That is not a valid light setting!\r\n");
      }
    else {
      OLC_ROOM(d)->nat_light = number;
      write_to_output(d, "Room lighting set to: %d\r\n", number);
       redit_disp_menu(d);
    }
   break;
  case REDIT_ROOM_MIN_LEVEL:
    number = atoi(arg);
    if (number < 0 || number >= LVL_SAINT) {
       write_to_output(d, "That is not a valid minimum level!\r\n");
       }
    else {
      OLC_ROOM(d)->min_level = number;
      write_to_output(d, "Minimum level set to %d\r\n", number);
      redit_disp_menu(d);
      }
    break;
  case REDIT_ROOM_MIN_LEVEL_MSG:
       OLC_ROOM(d)->min_level_message = str_udup(arg);
       if ( strcmp(OLC_ROOM(d)->min_level_message, "undefined") == 0 )
        OLC_ROOM(d)->min_level_message = "You are not experienced enough to go there.";
       write_to_output(d, "Min Level message set to : %s\r\n", OLC_ROOM(d)->min_level_message);
       OLC_VAL(d) = 1;
       redit_disp_menu(d);
       return;
  case REDIT_ROOM_MAX_LEVEL_MSG:
       OLC_ROOM(d)->max_level_message = str_udup(arg);
       if ( strcmp(OLC_ROOM(d)->max_level_message, "undefined") == 0 )
        OLC_ROOM(d)->max_level_message = "You are too experienced to go there.";
       write_to_output(d, "Max Level message set to : %s\r\n", OLC_ROOM(d)->max_level_message);
       OLC_VAL(d) = 1;
       redit_disp_menu(d);
       return;
  case REDIT_ROOM_MAX_LEVEL:
    number = atoi(arg);
    if (number < 0 || number >= LVL_SAINT) {
       write_to_output(d, "That is not a valid maximum level!\r\n");
       }
    else {
      OLC_ROOM(d)->max_level = number;
      write_to_output(d, "Maximum level set to %d\r\n", number);
      redit_disp_menu(d);
      }
     break;

  case REDIT_DESC:
    /*
     * We will NEVER get here, we hope.
     */
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Reached REDIT_DESC case in parse_redit().");
    write_to_output(d, "Oops, in REDIT_DESC.\r\n");
    break;

  case REDIT_FLAGS:
    number = atoi(arg);
    if (number < 0 || number > room_bits_count) {
      write_to_output(d, "That is not a valid choice!\r\n");
      redit_disp_flag_menu(d);
    } else if (number == 0)
      break;
    else {
      /* convert the argument from the sorted flag value to the actual
       * flag value */
      int i;
      for( i = 0; i < room_bits_count; i++ )
      {
        if( strcmp(room_bits[i], sorted_room_bits[number - 1]) == 0 )
        {
          number = i + 1;
          break;
        }
      }

      TOGGLE_BIT(OLC_ROOM(d)->room_flags, 1ULL << (number - 1));
      redit_disp_flag_menu(d);
    }
    return;

  case REDIT_SECTOR:
    number = atoi(arg);
    if (number < 0 || number >= NUM_ROOM_SECTORS) {
      write_to_output(d, "Invalid choice!");
      redit_disp_sector_menu(d);
      return;
    }
    OLC_ROOM(d)->sector_type = number;
    break;


  case REDIT_SIZE:
    number = atoi(arg);
    if (number < 0 || number >= NUM_ROOM_SECTORS) {
      write_to_output(d, "Invalid choice!");
      redit_disp_size_menu(d);
      return;
    }
    OLC_ROOM(d)->size = number;
    break;

  case REDIT_EXIT_MENU:
    switch (*arg) {
    case '0':
      break;
    case '1':
      OLC_MODE(d) = REDIT_EXIT_NUMBER;
      write_to_output(d, "Exit to room number : ");
      return;
    case '2':
      OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
      send_editor_help(d);
      write_to_output(d, "Enter exit description:\r\n\r\n");
      if (OLC_EXIT(d)->general_description) {
	write_to_output(d, OLC_EXIT(d)->general_description);
	oldtext = strdup(OLC_EXIT(d)->general_description);
      }
      string_write(d, &OLC_EXIT(d)->general_description, MAX_EXIT_DESC, 0, oldtext);
      return;
    case '3':
      OLC_MODE(d) = REDIT_EXIT_KEYWORD;
      write_to_output(d, "Enter keywords : ");
      return;
    case '4':
      OLC_MODE(d) = REDIT_EXIT_KEY;
      write_to_output(d, "Enter key number : ");
      return;
    case '5':
      OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
      redit_disp_exit_flag_menu(d);
      return;
    case '6':
      /*
       * Delete an exit.
       */
      if (OLC_EXIT(d)->keyword)
	free(OLC_EXIT(d)->keyword);
      if (OLC_EXIT(d)->general_description)
	free(OLC_EXIT(d)->general_description);
      if (OLC_EXIT(d))
	free(OLC_EXIT(d));
      OLC_EXIT(d) = NULL;
      break;
    default:
      write_to_output(d, "Try again : ");
      return;
    }
    break;

  case REDIT_EXIT_NUMBER:
    if ((number = atoi(arg)) != -1)
      if ((number = real_room(number)) == NOWHERE) {
	write_to_output(d, "That room does not exist, try again : ");
	return;
      }
    OLC_EXIT(d)->to_room = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DESCRIPTION:
    /*
     * We should NEVER get here, hopefully.
     */
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Reached REDIT_EXIT_DESC case in parse_redit");
    write_to_output(d, "Oops, in REDIT_EXIT_DESCRIPTION.\r\n");
    break;

  case REDIT_EXIT_KEYWORD:
    if (OLC_EXIT(d)->keyword)
      free(OLC_EXIT(d)->keyword);
    OLC_EXIT(d)->keyword = str_udup(arg);
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_KEY:
    OLC_EXIT(d)->key = atoi(arg);
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DOORFLAGS:
    number = atoi(arg);
    if (number < 0 || number > 5) {
      write_to_output(d, "That's not a valid choice!\r\n");
      redit_disp_exit_flag_menu(d);
    } else {
      /*
       * Doors are a bit idiotic, don't you think? :) -- I agree. -gg
       */
      OLC_EXIT(d)->exit_info = (number == 0 ? 0 :
				(number == 1 ? EX_ISDOOR :
				(number == 2 ? EX_ISDOOR | EX_PICKPROOF : 
				(number == 3 ? EX_ISDOOR | EX_HIDDEN : 
				(number == 4 ? EX_ISDOOR | EX_HIDDEN | EX_PICKPROOF : 0)))));
      /*
       * Jump back to the menu system.
       */
      redit_disp_exit_menu(d);
    }
    return;

  case REDIT_EXTRADESC_KEY:
    if (genolc_checkstring(d, arg))
      OLC_DESC(d)->keyword = strdup(arg);
    redit_disp_extradesc_menu(d);
    return;

  case REDIT_EXTRADESC_MENU:
    switch ((number = atoi(arg))) {
    case 0:
      /*
       * If something got left out, delete the extra description
       * when backing out to the menu.
       */
      if (OLC_DESC(d)->keyword == NULL || OLC_DESC(d)->description == NULL) {
	struct extra_descr_data **tmp_desc;
	if (OLC_DESC(d)->keyword)
	  free(OLC_DESC(d)->keyword);
	if (OLC_DESC(d)->description)
	  free(OLC_DESC(d)->description);

	/*
	 * Clean up pointers.
	 */
	for (tmp_desc = &(OLC_ROOM(d)->ex_description); *tmp_desc; tmp_desc = &((*tmp_desc)->next))
	  if (*tmp_desc == OLC_DESC(d)) {
	    *tmp_desc = NULL;
	    break;
	  }
	free(OLC_DESC(d));
      }
      break;
    case 1:
      OLC_MODE(d) = REDIT_EXTRADESC_KEY;
      write_to_output(d, "Enter keywords, separated by spaces : ");
      return;
    case 2:
      OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
      send_editor_help(d);
      write_to_output(d, "Enter extra description:\r\n\r\n");
      if (OLC_DESC(d)->description) {
	write_to_output(d, "%s", OLC_DESC(d)->description);
	oldtext = strdup(OLC_DESC(d)->description);
      }
      string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, oldtext);
      return;
    case 3:
    OLC_MODE(d) = REDIT_ROOM_PAINMSG;
    write_to_output(d, "\r\nEnter pain room message : ");
    return;
    case 4: 
      OLC_MODE(d) = REDIT_ROOM_PAINDAM;
      write_to_output(d, "\r\nEnter pain room damage : ");
      return;
    case 5:
      OLC_MODE(d) = REDIT_ROOM_PAINRATE;
      write_to_output(d, "\r\nEnter new pain room rate in secs : ");
      return;
    case 6:
      OLC_MODE(d) = REDIT_ROOM_NOMAGIC_MSG_CASTER;
      write_to_output(d, "\r\nEnter new no magic room message for the caster: ");
      return;
    case 7:
      OLC_MODE(d) = REDIT_ROOM_NOMAGIC_MSG_ROOM;
      write_to_output(d, "\r\nEnter new no magic room message for the room: ");
      return;
    case 8 :
      if (OLC_DESC(d)->keyword == NULL || OLC_DESC(d)->description == NULL) {
	write_to_output(d, "You can't edit the next extra description without completing this one.\r\n");
	redit_disp_extradesc_menu(d);
      } else {
	struct extra_descr_data *new_extra;

	if (OLC_DESC(d)->next)
	  OLC_DESC(d) = OLC_DESC(d)->next;
	else {
	  /*
	   * Make new extra description and attach at end.
	   */
	  CREATE(new_extra, struct extra_descr_data, 1);
	  OLC_DESC(d)->next = new_extra;
	  OLC_DESC(d) = new_extra;
	}
	redit_disp_extradesc_menu(d);
      }
      return;
    }
    break;

  default:
    /*
     * We should never get here.
     */
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Reached default case in parse_redit");
    break;
  }
  /*
   * If we get this far, something has been changed.
   */
  OLC_VAL(d) = 1;
  redit_disp_menu(d);
}

void redit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
  case REDIT_DESC:
    redit_disp_menu(d);
    break;
  case REDIT_EXIT_DESCRIPTION:
    redit_disp_exit_menu(d);
    break;
  case REDIT_EXTRADESC_DESCRIPTION:
    redit_disp_extradesc_menu(d);
    break;
  }
}
