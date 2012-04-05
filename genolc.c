/************************************************************************
 * Generic OLC Library - General / genolc.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "comm.h"
#include "shop.h"
#include "genolc.h"
#include "genwld.h"
#include "genmob.h"
#include "genshp.h"
#include "genzon.h"
#include "genobj.h"
#include "dg_olc.h"
#include "oasis.h"
#include "constants.h"

extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct room_data *world;
extern struct char_data *mob_proto;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct shop_data *shop_index;
extern struct index_data **trig_index;
extern int top_shop;
extern int top_of_trigt;

/* Adjustment for top_shop change between bpl15 and bpl16. */
#if _CIRCLEMUD < CIRCLEMUD_VERSION(3,0,16)
int top_shop_offset = 1;
#else
int top_shop_offset = 0;
#endif

/*
 * List of zones to be saved.
 */
struct save_list_data *save_list;

/*
 * Structure defining all known save types.
 */
struct {
  int save_type;
  int (*func)(zone_rnum rnum);
  const char *message;
} save_types[] = {
  { SL_MOB, save_mobiles , "mobile" },
  { SL_OBJ, save_objects, "object" },
  { SL_SHP, save_shops, "shop" },
  { SL_WLD, save_rooms, "room" },
  { SL_ZON, save_zone, "zone" },
  { -1, NULL, NULL },
};

/* -------------------------------------------------------------------------- */

int genolc_checkstring(struct descriptor_data *d, const char *arg)
{
  if (strchr(arg, STRING_TERMINATOR)) {
    write_to_output(d, "Sorry, you cannot use '%c' in your descriptions.\r\n", STRING_TERMINATOR);
    return FALSE;
  }
  return TRUE;
}

char *str_udup(const char *txt)
{
 return strdup((txt && *txt) ? txt : STRING_UNDEFINED);
}

/*
 * Original use: to be called at shutdown time.
 */
int save_all(void)
{
  while (save_list) {
    if (save_list->type < 0 || save_list->type > SL_MAX)
      log("SYSERR: GenOLC: Invalid save type %d in save list.\n", save_list->type);
    else if ((*save_types[save_list->type].func)(real_zone(save_list->zone)) < 0)
      save_list = save_list->next;	/* Fatal error, skip this one. */
  }

  return TRUE;
}

/* -------------------------------------------------------------------------- */

/*
 * NOTE: This changes the buffer passed in.
 */
void strip_cr(char *buffer)
{
  int rpos, wpos;

  if (buffer == NULL)
    return;

  for (rpos = 0, wpos = 0; buffer[rpos]; rpos++) {
    buffer[wpos] = buffer[rpos];
    wpos += (buffer[rpos] != '\r');
  }
  buffer[wpos] = '\0';
}

/* -------------------------------------------------------------------------- */

void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from)
{
  struct extra_descr_data *wpos;

  CREATE(*to, struct extra_descr_data, 1);
  wpos = *to;

  for (; from; from = from->next, wpos = wpos->next) {
    wpos->keyword = strdup(from->keyword ? from->keyword : "<null>");
    wpos->description = strdup(from->description ? from->description : "You see nothing special.");
    if (from->next)
      CREATE(wpos->next, struct extra_descr_data, 1);
  }
}

/* -------------------------------------------------------------------------- */

void free_ex_descriptions(struct extra_descr_data *head)
{
  struct extra_descr_data *thised, *next_one;

  if (!head) {
    log("free_ex_descriptions: NULL pointer or NULL data.");
    return;
  }

  for (thised = head; thised; thised = next_one) {
    next_one = thised->next;
    if (thised->keyword)
      free(thised->keyword);
    if (thised->description)
      free(thised->description);
    free(thised);
  }
}

/* -------------------------------------------------------------------------- */

int remove_from_save_list(zone_vnum zone, int type)
{
  struct save_list_data *ritem, *temp;

  for (ritem = save_list; ritem; ritem = ritem->next)
    if (ritem->zone == zone && ritem->type == type)
      break;

  if (ritem == NULL) {
    log("SYSERR: remove_from_save_list: Saved item not found. (%d/%d)", zone, type);
    return -1;
  }
  REMOVE_FROM_LIST(ritem, save_list, next);
  free(ritem);
  return TRUE;
}

/* -------------------------------------------------------------------------- */

int add_to_save_list(zone_vnum zone, int type)
{
  struct save_list_data *nitem;
  zone_rnum rznum = real_zone(zone);

  if (rznum == NOWHERE || rznum > top_of_zone_table) {
    log("SYSERR: add_to_save_list: Invalid zone number passed. (%d => %d, 0-%d)", zone, rznum, top_of_zone_table);
    return -1;
  }

  for (nitem = save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return FALSE;

  CREATE(nitem, struct save_list_data, 1);
  nitem->zone = zone;
  nitem->type = type;
  nitem->next = save_list;
  save_list = nitem;
  return TRUE;
}

/* -------------------------------------------------------------------------- */

/*
 * Used from do_show(), ideally.
 */
void do_show_save_list(struct char_data *ch)
{
  if (save_list == NULL)
    send_to_char(ch, "All world files are up to date.\r\n");
  else {
    struct save_list_data *item;

    send_to_char(ch, "The following files need saving:\r\n");
    for (item = save_list; item; item = item->next) {
      send_to_char(ch, " - %s data for zone %d.\r\n", save_types[item->type].message, item->zone);
    }
  }
}

/* -------------------------------------------------------------------------- */

/*
 * TEST FUNCTION! Not meant to be pretty!
 *
 * edit q	- Query unsaved files.
 * edit a	- Save all.
 * edit r1204 c	- Copies current room to 1204.
 * edit r1204 d	- Deletes room 1204.
 * edit r12 s	- Saves rooms in zone 12.
 * edit m3000 c3001	- Copies mob 3000 to 3001.
 * edit m3000 d	- Deletes mob 3000.
 * edit m30 s	- Saves mobiles in zone 30.
 * edit o186 s	- Saves objects in zone 186.
 * edit s25 s	- Saves shops in zone 25.
 * edit z31 s	- Saves zone 31.
 */
#include "interpreter.h"
ACMD(do_edit);
ACMD(do_edit)
{
  int idx, num, mun;
  char a[MAX_INPUT_LENGTH], b[MAX_INPUT_LENGTH];

  two_arguments(argument, a, b);
  num = atoi(a + 1);
  mun = atoi(b + 1);
  switch (a[0]) {
  case 'a':
    save_all();
    break;
  case 'm':
    switch (b[0]) {
    case 'd':
      if ((idx = real_mobile(num)) != NOBODY) {
	delete_mobile(idx);	/* Delete -> Real */
	send_to_char(ch, OK);
      } else
	send_to_char(ch, "What mobile?\r\n");
      break;
    case 's':
      save_mobiles(num);
      break;
    case 'c':
      if ((num = real_mobile(num)) == NOBODY)
	send_to_char(ch, "What mobile?\r\n");
      else if ((mun = real_mobile(mun)) == NOBODY)
	send_to_char(ch, "Can only copy over an existing mob.\r\n");
      else {
        /* Otherwise the old ones have dangling string pointers. */
        extract_mobile_all(mob_index[mun].vnum);
        /* To <- From */
	copy_mobile(mob_proto + mun, mob_proto + num);
	send_to_char(ch, OK);
      }
      break;
    }
    break;
  case 's':
    switch (b[0]) {
    case 's':
      save_objects(real_zone(num));
      break;
    }
    break;
  case 'z':
    switch (b[0]) {
    case 's':
      save_zone(real_zone(num));
      break;
    }
    break;
  case 'o':
    switch (b[0]) {
    case 's':
      save_objects(real_zone(num));
      break;
    }
    break;
  case 'r':
    switch (b[0]) {
    case 'd':
      if ((idx = real_room(num)) != NOWHERE) {
	delete_room(idx);
	send_to_char(ch, OK);
      } else
	send_to_char(ch, "What room?\r\n");
      break;
    case 's':
      save_rooms(real_zone(num));
      break;
    case 'c':
      duplicate_room(num, ch->in_room);	/* To -> Virtual, From -> Real */
      break;
    }
  case 'q':
    do_show_save_list(ch);
    break;
  default:
    send_to_char(ch, "What?\r\n");
    break;
  }
}

/* -------------------------------------------------------------------------- */

room_vnum genolc_zonep_bottom(struct zone_data *zone)
{
#if _CIRCLEMUD < CIRCLEMUD_VERSION(3,0,21)
  return zone->number * 100;
#else
  return zone->bot;
#endif
}

zone_vnum genolc_zone_bottom(zone_rnum rznum)
{
#if _CIRCLEMUD < CIRCLEMUD_VERSION(3,0,21)
  return zone_table[rznum].number * 100;
#else /* bpl21+ */
  return zone_table[rznum].bot;
#endif
}

/* -------------------------------------------------------------------------- */

int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

  for (i = 0; flags[i] != '\0'; i++)
    if (bits & (1 << i))
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* NUL terminate the output string. */
  out[j++] = '\0';
  return j;
}

/* -------------------------------------------------------------------------- */

int sprintasciilong(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 64 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  char *flags = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@$";

  for (i = 0; flags[i] != '\0'; i++)
    if (bits & (1ULL << i))
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* NUL terminate the output string. */
  out[j++] = '\0';
  return j;
}

/* Can they edit a zone?  This takes a zone's vnum.   */
int can_edit_zone(struct char_data *ch, zone_vnum number, int subcmd)
{
  if (GET_LEVEL(ch) >= LVL_LIEGE) /* counts+ can edit everything */
    return TRUE;

  if (GET_LEVEL(ch) < LVL_BUILDER) /* non-builders can't edit anything */
    return FALSE;
  
  if (GET_OLC_ZONE(ch) == number) /* correct permission set */
    return TRUE;

  return FALSE;
  
#if 0
  /* some sanity checking */
  switch (subcmd) {
    case SCMD_OASIS_HEDIT:
    case SCMD_OASIS_GEDIT: /* only ok if permission is set */
      break; 
    case SCMD_OASIS_AEDIT:
       /* Social editor can edit actions */
      if (PLR_FLAGGED(ch, PLR_SOCED))
        return TRUE;
      break;
    default: 
      if (is_name(GET_NAME(ch), zone_table[real_zone(number)].builders))
        return TRUE;
  }
#endif   
}

/* list code */

void perform_slist(struct char_data *ch, zone_rnum first)
{ 
  char pagebuf[MAX_STRING_LENGTH], temp[44], buf[MAX_STRING_LENGTH];
  shop_rnum nr;
  int found = 0;
  extern char *customer_string(int shop_nr, int detailed);
  
  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s used slist for zone %d.",
		   GET_NAME(ch), first);
  
  strcpy(pagebuf, " ##   Virtual   Where    Keeper    Buy   Sell   Customers\r\n"
                  "---------------------------------------------------------\r\n");
               
  for (nr = 0; nr <= top_shop; nr++)  {
    if(real_zone_by_thing(SHOP_NUM(nr)) == first) {

      if (SHOP_KEEPER(nr) == NOBODY)
        strcpy(temp, "<NONE>");
      else
        sprintf(temp, "%6d", mob_index[SHOP_KEEPER(nr)].vnum);

      sprintf(buf, "%3d   %6d   %6d    %s   %3.2f   %3.2f    %s\r\n",
              nr + 1,
              SHOP_NUM(nr),
              SHOP_ROOM(nr, 0),
              temp,
              SHOP_SELLPROFIT(nr),
              SHOP_BUYPROFIT(nr),
              customer_string(nr, FALSE)
              );
      found++;
      strcat(pagebuf, buf);
      if (strlen(pagebuf)>MAX_STRING_LENGTH-100) {
        strcat(pagebuf, "\r\n!!!!OVERFLOW!!!!\r\n");
        page_string(ch->desc, pagebuf, TRUE);
        return;
      }
    }
  }

  if (!found)
    send_to_char(ch, "No shops in that zone.\n\r");
  else page_string(ch->desc, pagebuf, TRUE);
}


void perform_tlist(struct char_data *ch, int first, int last)
{ 
  char pagebuf[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int nr, found = 0;

  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s used tlist from %d to %d.",
		   GET_NAME(ch), first, last);
  
  strcpy(pagebuf, "Vnum   Trigger name\r\n"
                  "-----------------------------------------------------------------\r\n");
               
  for (nr = 0; nr < top_of_trigt && (trig_index[nr]->vnum <= last); nr++)  {
    if (trig_index[nr]->vnum >= first) {
      sprintf(buf, "[%5d] %s\r\n",
              trig_index[nr]->vnum,
              trig_index[nr]->proto->name);
      found++;
      strcat(pagebuf, buf);
      if (strlen(pagebuf)>MAX_STRING_LENGTH-100) {
        strcat(pagebuf, "\r\n!!!!OVERFLOW!!!!\r\n");
        page_string(ch->desc, pagebuf, TRUE);
        return;
      }
    }
  }

  if (!found)
    send_to_char(ch, "No triggers were found in those parameters.\n\r");
  else page_string(ch->desc, pagebuf, TRUE);
}

void perform_mlist(struct char_data *ch, mob_vnum first, mob_vnum last)
{ 
  char pagebuf[MAX_STRING_LENGTH], temp[44], buf[MAX_STRING_LENGTH];
  mob_rnum nr;
  int found = 0;

  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s used mlist from %d to %d.",
		   GET_NAME(ch), first, last);
  
  strcpy(pagebuf, "Vnum    Short description                         Level\r\n"
                  "-----------------------------------------------------------------\r\n");
               
  for (nr = 0; nr <= top_of_mobt && (mob_index[nr].vnum <= last); nr++)  {
    if (mob_index[nr].vnum >= first) {
      strncpy(temp, mob_proto[nr].player.short_descr, 43);
      temp[43] = '\0';
      sprintf(buf, "[%5d] %-43s %5d\r\n",
              mob_index[nr].vnum,
              temp,
              mob_proto[nr].player.level);
      found++;
      strcat(pagebuf, buf);
      if (strlen(pagebuf)>MAX_STRING_LENGTH-100) {
        strcat(pagebuf, "\r\n!!!!OVERFLOW!!!!\r\n");
        page_string(ch->desc, pagebuf, TRUE);
        return;
      }
    }
  }

  if (!found)
    send_to_char(ch, "No mobiles were found in those parameters.\n\r");
  else page_string(ch->desc, pagebuf, TRUE);
}

void perform_rlist(struct char_data *ch, room_vnum first, room_vnum last)
{ 
  char pagebuf[MAX_STRING_LENGTH],
       temp[40],
       temp2[MAX_INPUT_LENGTH],
       temp3[MAX_INPUT_LENGTH],
       buf[MAX_STRING_LENGTH];
  room_rnum nr;
  int found = 0;
  
  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s used rlist from %d to %d.",
		   GET_NAME(ch), first, last);

  strcpy(pagebuf, "Vnum    Name                                Sector          Flags\r\n"
                  "-----------------------------------------------------------------\r\n");
               
  for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++) {
    if (GET_ROOM_VNUM(nr) >= first) {
      strncpy(temp, world[nr].name, 35);
      temp[35] = '\0';
      sprinttype(world[nr].sector_type, sector_types, temp2, sizeof(temp2));
      sprintbit(world[nr].room_flags, room_bits, temp3, sizeof(temp3));
      sprintf(buf, "[%5d] %-35s %-15s %s\r\n",
              GET_ROOM_VNUM(nr),
              temp,
              temp2,
              temp3);
      found++;
      strcat(pagebuf, buf);
      if (strlen(pagebuf)>MAX_STRING_LENGTH-100) {
        strcat(pagebuf, "\r\n!!!!OVERFLOW!!!!\r\n");
        page_string(ch->desc, pagebuf, TRUE);
        return;
      }
    }
  }

  if (!found)
    send_to_char(ch, "No rooms were found in those parameters.\n\r");
  else page_string(ch->desc, pagebuf, TRUE);
}

void perform_olist(struct char_data *ch, obj_vnum first, obj_vnum last)
{ 
  char pagebuf[MAX_STRING_LENGTH], temp[40], buf[MAX_STRING_LENGTH];
  obj_rnum nr;
  int found = 0;

  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s used olist from %d to %d.",
		   GET_NAME(ch), first, last);
  
  strcpy(pagebuf,
            " Vnum   Short description                            Level\r\n"
            "----------------------------------------------------------\r\n");
               
  for (nr = 0; nr <= top_of_objt && (obj_index[nr].vnum <= last); nr++) {
    if (obj_index[nr].vnum >= first) {
      strncpy(temp, obj_proto[nr].short_description, 33);
      temp[33] = '\0';
      sprintf(buf, "[%5d] %-33s   %2d\r\n",
              obj_index[nr].vnum,
              temp,
              obj_proto[nr].obj_flags.level );
      found++;
      strcat(pagebuf, buf);
      if (strlen(pagebuf)>MAX_STRING_LENGTH-100) {
        strcat(pagebuf, "\r\n!!!!OVERFLOW!!!!\r\n");
        page_string(ch->desc, pagebuf, TRUE);
        return;
      }
    }
  }

  if (!found)
    send_to_char(ch, "No objects were found in those parameters.\n\r");
  else page_string(ch->desc, pagebuf, TRUE);
}

ACMD(do_builder_list)
{
  int first, last, max;
  zone_rnum rzone = 0;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf, buf2);

  if (!*buf) {
    send_to_char(ch, "Usage:\r\n" 
                     "Objects:  olist <beginning number or zone> [<ending number>]\r\n"
                     "Mobiles:  mlist <beginning number or zone> [<ending number>]\r\n"
                     "Rooms:    rlist <beginning number or zone> [<ending number>]\r\n"
                     "Triggers: tlist <beginning number or zone> [<ending number>]\r\n"
                     "Shops:    slist <zone>\r\n"
                );
    return;
  }

  if (*buf != '.')
    first = atoi(buf);
  else
    first = GET_OLC_ZONE(ch);
    
  if (subcmd == SCMD_SLIST) {
    last = first;
  } else {
    if (*buf2)
      last = atoi(buf2);
    else {
      if ((rzone = real_zone(first)) != NOWHERE) {
        last = zone_table[rzone].top;
        first = zone_table[rzone].bot;
      } else {
        send_to_char(ch, "Nothing was found within those parameters.\n\r");
        return;
      }
    }
  }
  switch (subcmd) {
    case SCMD_RLIST: max = world[top_of_world].number; break;
    case SCMD_OLIST: max = obj_index[top_of_objt].vnum; break;
    case SCMD_MLIST: max = mob_index[top_of_mobt].vnum; break;
    case SCMD_TLIST: max = zone_table[top_of_zone_table].top; break;
    case SCMD_SLIST: max = real_zone_by_thing(SHOP_NUM(top_shop)); break;
    default: return;
  }
  if (first < 0 || first > max || last < 0) {
    send_to_char(ch, "Values must be between 0 and %d.\r\n", max);
    return;
  }

  if (first > last) {
    send_to_char(ch, "Second value must be greater than first.\n\r");
    return;
  }

  switch (subcmd) {
    case SCMD_RLIST:   perform_rlist(ch, first, last); break;
    case SCMD_OLIST:   perform_olist(ch, first, last); break;
    case SCMD_MLIST:   perform_mlist(ch, first, last); break;
    case SCMD_TLIST:   perform_tlist(ch, first, last); break;
    case SCMD_SLIST:   perform_slist(ch, real_zone(first)); break;
    default: return;
  }
}
