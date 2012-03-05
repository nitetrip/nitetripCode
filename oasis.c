/************************************************************************
 * OasisOLC - General / oasis.c					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genmob.h"
#include "genshp.h"
#include "genzon.h"
#include "genwld.h"
#include "genobj.h"
#include "oasis.h"
#include "screen.h"
#include "dg_olc.h"

const char *nrm, *grn, *cyn, *yel;

/*
 * External data structures.
 */
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern zone_rnum top_of_zone_table;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;

/*
 * Internal data structures.
 */
struct olc_scmd_info_t {
  const char *text;
  int con_type;
} olc_scmd_info[] = {
  { "room",	CON_REDIT },
  { "object",	CON_OEDIT },
  { "zone",	CON_ZEDIT },
  { "mobile",	CON_MEDIT },
  { "shop",	CON_SEDIT },
  { "trigger",  CON_TRIGEDIT },
  { "clan",     CON_CLANEDIT }, //dan clan system
  { "\n",	-1	  }
};

/* -------------------------------------------------------------------------- */

/*
 * Only player characters should be using OLC anyway.
 */
void clear_screen(struct descriptor_data *d)
{
  if (PRF_FLAGGED(d->character, PRF_CLS))
    send_to_char(d->character, "[H[J");
}

/* -------------------------------------------------------------------------- */

/*
 * Exported ACMD do_oasis function.
 *
 * This function is the OLC interface.  It deals with all the 
 * generic OLC stuff, then passes control to the sub-olc sections.
 */
ACMD(do_oasis)
{
  int number = -1, save = 0, real_num;
  struct descriptor_data *d;
  char *buf3;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  /*
   * No screwing around as a mobile.
   */
  if (IS_NPC(ch))
    return;
  
  /*
   * The command to see what needs to be saved, typically 'olc'.
   */
  if (subcmd == SCMD_OLC_SAVEINFO) {
    do_show_save_list(ch);
    return;
  }

  /*
   * Parse any arguments.
   */
  buf3 = two_arguments(argument, buf1, buf2);
  if (!*buf1) {		/* No argument given. */
    switch (subcmd) {
    case SCMD_OASIS_ZEDIT:
    case SCMD_OASIS_REDIT:
      number = GET_ROOM_VNUM(IN_ROOM(ch));
      break;
    case SCMD_OASIS_TRIGEDIT:
    case SCMD_OASIS_OEDIT:
    case SCMD_OASIS_MEDIT:
    case SCMD_OASIS_SEDIT:
      send_to_char(ch, "Specify a %s VNUM to edit.\r\n", olc_scmd_info[subcmd].text);
      return;
    
      default:
       log("SYSERR: (OLC) Invalid subcmd passed to do_oasis, subcmd - (%d)", subcmd);

    }
  } else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) == 0) {
      save = TRUE;
      if (is_number(buf2))
	number = atoi(buf2);
      else if (GET_OLC_ZONE(ch) >= 0) {
	zone_rnum zlok;
	if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
	  number = -1;
	else
	  number = genolc_zone_bottom(zlok); /* Or .top or in between. */
      }

      if (number < 0) {
	send_to_char(ch, "Save which zone?\r\n");
	return;
      }
      if (subcmd == SCMD_OASIS_TRIGEDIT) {
	send_to_char(ch, "Triggers save automatically.\r\n");
	return;
      }
    } else if (subcmd == SCMD_OASIS_ZEDIT && GET_LEVEL(ch) >= LVL_IMPL) {
      if (str_cmp("new", buf1) || !buf3 || !*buf3)
	send_to_char(ch, "Unknown command, perhaps 'new zone lower-room-number upper-room-number'?\r\n");
      else {
	char sbot[MAX_INPUT_LENGTH], stop[MAX_INPUT_LENGTH];
	room_vnum bottom, top;

	skip_spaces(&buf3);	/* actually, atoi() doesn't care... */
	two_arguments(buf3, sbot, stop);

	number = atoi(buf2);
	bottom = atoi(sbot);
	top = atoi(stop);

	zedit_new_zone(ch, number, bottom, top);
      }
      return;
    } else {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
  }

  /*
   * If a numeric argument was given (like a room number), get it.
   */
  if (number == -1)
    number = atoi(buf1);

  /*
   * Check that whatever it is isn't already being edited.
   */
  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == olc_scmd_info[subcmd].con_type)
      if (d->olc && OLC_NUM(d) == number) {
	send_to_char(ch, "That %s is currently being edited by %s.\r\n", olc_scmd_info[subcmd].text, PERS(d->character, ch));
	return;
      }
  d = ch->desc;
 
  /*
   * Give descriptor an OLC structure.
   */
 if (d->olc) {
    mudlog(BRF, LVL_SAINT, TRUE, "SYSERR: do_oasis: Player already had olc structure.");
    free(d->olc);
  } 
  CREATE(d->olc, struct oasis_olc_data, 1);

  /*
   * Find the zone.
   */
  if ((OLC_ZNUM(d) = real_zone_by_thing(number)) == -1) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /*
   * Everyone but IMPLs can only edit zones they have been assigned.
   */
  if (((zone_table[OLC_ZNUM(d)].number != GET_OLC_ZONE(ch) ) || 
(GET_OLC_ZONE(ch) < 1) ) && (GET_LEVEL(ch) < LVL_COUNT ) ) {
      send_to_char(ch, "You do not have permission to edit this zone.\r\n");
    mudlog(BRF, LVL_GOD, TRUE, "OLC: %s tried to edit zone %d allowed zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
    free(d->olc);
    d->olc = NULL;
    return;
  }

  if (save) {
    const char *type = NULL;
 
    if (subcmd >= 0 && subcmd <= (int)(sizeof(olc_scmd_info) / sizeof(struct olc_scmd_info_t) - 1))
      type = olc_scmd_info[subcmd].text;
    else {
      send_to_char(ch, "Oops, I forgot what you wanted to save.\r\n");
      return;
    }
    send_to_char(ch, "Saving all %ss in zone %d.\r\n", type, zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s saves %s info for zone %d.", GET_NAME(ch), type, zone_table[OLC_ZNUM(d)].number);

    switch (subcmd) {
      case SCMD_OASIS_REDIT: save_rooms(OLC_ZNUM(d)); break;
      case SCMD_OASIS_ZEDIT: save_zone(OLC_ZNUM(d)); break;
      case SCMD_OASIS_OEDIT: save_objects(OLC_ZNUM(d)); break;
      case SCMD_OASIS_MEDIT: save_mobiles(OLC_ZNUM(d)); break;
      case SCMD_OASIS_SEDIT: save_shops(OLC_ZNUM(d)); break;
    }
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;

  /*
   * Steal player's descriptor and start up subcommands.
   */
  switch (subcmd) {
   case SCMD_OASIS_CLANEDIT:
      do_oasis_clanedit(ch, argument, cmd, subcmd);
      break;
  case SCMD_OASIS_TRIGEDIT:
    if ((real_num = real_trigger(number)) != NOTHING)
      trigedit_setup_existing(d, real_num);
    else
      trigedit_setup_new(d);
    STATE(d) = CON_TRIGEDIT;
    break;
  case SCMD_OASIS_REDIT:
    if ((real_num = real_room(number)) != NOWHERE)
      redit_setup_existing(d, real_num);
    else
      redit_setup_new(d);
    STATE(d) = CON_REDIT;
    break;
  case SCMD_OASIS_ZEDIT:
    if ((real_num = real_room(number)) == NOWHERE) {
      send_to_char(ch, "That room does not exist.\r\n");
      free(d->olc);
      d->olc = NULL;
      return;
    }
    zedit_setup(d, real_num);
    STATE(d) = CON_ZEDIT;
    break;
  case SCMD_OASIS_MEDIT:
    if ((real_num = real_mobile(number)) != NOTHING)
      medit_setup_existing(d, real_num);
    else
      medit_setup_new(d);
    STATE(d) = CON_MEDIT;
    break;
  case SCMD_OASIS_OEDIT:
    if ((real_num = real_object(number)) != NOTHING)
      oedit_setup_existing(d, real_num);
    else
      oedit_setup_new(d);
    STATE(d) = CON_OEDIT;
    break;
  case SCMD_OASIS_SEDIT:
    if ((real_num = real_shop(number)) != NOTHING)
      sedit_setup_existing(d, real_num);
    else
      sedit_setup_new(d);
    STATE(d) = CON_SEDIT;
    break;

    }
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
  mudlog(CMP, LVL_SAINT, TRUE, "OLC: %s starts editing zone %d allowed zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

/*------------------------------------------------------------*\
 Exported utilities 
\*------------------------------------------------------------*/

/*
 * Set the colour string pointers for that which this char will
 * see at color level NRM.  Changing the entries here will change 
 * the colour scheme throughout the OLC.
 */
void get_char_colors(struct char_data *ch)
{
  nrm = CCNRM(ch, C_NRM);
  grn = CCGRN(ch, C_NRM);
  cyn = CCCYN(ch, C_NRM);
  yel = CCYEL(ch, C_NRM);
}

/*
 * This procedure frees up the strings and/or the structures
 * attatched to a descriptor, sets all flags back to how they
 * should be.
 */
void cleanup_olc(struct descriptor_data *d, byte cleanup_type)
{
  /*
   * Clean up WHAT?
   */
  if (d->olc == NULL)
    return;

  /*
   * Check for a room. free_room doesn't perform
   * sanity checks, we must be careful here.
   */
  if (OLC_ROOM(d)) {
    switch (cleanup_type) {
    case CLEANUP_ALL:
      free_room(OLC_ROOM(d));
      break;
    case CLEANUP_STRUCTS:
      free(OLC_ROOM(d));
      break;
    default: /* The caller has screwed up. */
      log("SYSERR: cleanup_olc: Unknown type!");
      break;
    }
  }

  /*
   * Check for an existing object in the OLC.  The strings
   * aren't part of the prototype any longer.  They get added
   * with strdup().
   */
  if (OLC_OBJ(d)) {
    free_object_strings(OLC_OBJ(d));
    free(OLC_OBJ(d));
  }

  /*
   * Check for a mob.  free_mobile() makes sure strings are not in
   * the prototype.
   */
  if (OLC_MOB(d))
    free_mobile(OLC_MOB(d));

  /*
   * Check for a zone.  cleanup_type is irrelevant here, free() everything.
   */
  if (OLC_ZONE(d)) {
    free(OLC_ZONE(d)->name);
    free(OLC_ZONE(d)->cmd);
    free(OLC_ZONE(d));
  }

  /*
   * Check for a shop.  free_shop doesn't perform sanity checks, we must
   * be careful here.
   */
  if (OLC_SHOP(d)) {
    switch (cleanup_type) {
    case CLEANUP_ALL:
      free_shop(OLC_SHOP(d));
      break;
    case CLEANUP_STRUCTS:
      free(OLC_SHOP(d));
      break;
    default:
      /* The caller has screwed up but we already griped above. */
      break;
    }
  }
  /* Triggers */
#if 0
  /* 
   * this is the command list - it's been copied to disk already,
   * so just free it -- Welcor
   */
  if (OLC_STORAGE(d)) { 
    free(OLC_STORAGE(d));
    OLC_STORAGE(d) = NULL;
  }
  /*
   * Free this one regardless. If we've left olc, we've either made
   * a fresh copy of it in the trig index, or we lost connection.
   * Either way, we need to get rid of this.
   */
  if (OLC_TRIG(d)) {
    free_trigger(OLC_TRIG(d));
    OLC_STORAGE(d) = NULL;
  }
#endif
  /*
   * Restore descriptor playing status.
   */
  if (d->character) {
    REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING);
    STATE(d) = CON_PLAYING;
    act("$n stops using OLC.", TRUE, d->character, NULL, NULL, TO_ROOM);
    mudlog(CMP, LVL_SAINT, TRUE, "OLC: %s stops editing zone %d allowed zone %d", GET_NAME(d->character), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(d->character));
  }

  free(d->olc);
  d->olc = NULL;
}
