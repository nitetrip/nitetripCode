/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                           
	rm          *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"


/* external globals */
extern int mini_mud;

/* external functions */
SPECIAL(pet_shops);
SPECIAL(postmaster);
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(guild_guard);
SPECIAL(guild);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(bank);
SPECIAL(gen_board);
SPECIAL(class_branch_guildmaster);

/* local functions */
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
  mob_rnum rnum;

  if ((rnum = real_mobile(mob)) != NOBODY)
    mob_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
  obj_rnum rnum;

  if ((rnum = real_object(obj)) != NOTHING)
    obj_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
  room_rnum rnum;

  if ((rnum = real_room(room)) != NOWHERE)
    world[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  ASSIGNMOB(1, puff);
  ASSIGNMOB(508, guild);
  ASSIGNMOB(503, class_branch_guildmaster);
  ASSIGNMOB(4130, guild);
  ASSIGNMOB(4131, guild);
  ASSIGNMOB(4132, guild);
  ASSIGNMOB(4133, guild);
  ASSIGNMOB(10131, guild);
  ASSIGNMOB(10127, guild);
  ASSIGNMOB(10112, guild);
  ASSIGNMOB(10145, guild);
  ASSIGNMOB(5302, guild);
  ASSIGNMOB(5399, guild);
  ASSIGNMOB(10106, receptionist);

}



/* assign special procedures to objects */
void assign_objects(void)
{
  ASSIGNOBJ(500, gen_board);	/* imm board */
  ASSIGNOBJ(512, gen_board);	/* imp board */
  ASSIGNOBJ(513, gen_board);	/* god board */
  ASSIGNOBJ(529, gen_board);    /* TODO board */
  ASSIGNOBJ(4000, gen_board);   /* valeks board */
  ASSIGNOBJ(582, gen_board);    /* coders board */
  ASSIGNOBJ(4001, gen_board);   /* valeks board */
  ASSIGNOBJ(524, gen_board);    /* liege board */
  ASSIGNOBJ(2599, gen_board);   /* count board */
  ASSIGNOBJ(4299, gen_board);   /* deity board */
  ASSIGNOBJ(10200, gen_board);  /* suggestion board in mid */
  ASSIGNOBJ(10201, gen_board);  /* social board in mid */
  ASSIGNOBJ(20500, gen_board);  /* academy board */
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
  /*room_rnum i;*/

}
