/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
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
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "constants.h"

#define SINFO spell_info[spellnum]

/* local globals */
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

/* local functions */
void say_spell(struct char_data *ch, int spellnum, struct char_data *tch, struct obj_data *tobj);
void spello(int spl, const char *name, int max_mana, int min_mana, int mana_change, int minpos, int targets, int violent, int routines, const char *wearoff);
int mag_manacost(struct char_data *ch, int spellnum);
ACMD(do_cast);
void unused_spell(int spl);
void mag_assign_spells(void);

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

struct syllable {
  const char *org;
  const char *news;
};


struct syllable syls[] = {
  {" ", " "},
  {"ar", "abra"},
  {"ate", "i"},
  {"cau", "kada"},
  {"blind", "nose"},
  {"bur", "mosa"},
  {"cu", "judi"},
  {"de", "oculo"},
  {"dis", "mar"},
  {"ect", "kamina"},
  {"en", "uns"},
  {"gro", "cra"},
  {"light", "dies"},
  {"lo", "hi"},
  {"magi", "kari"},
  {"mon", "bar"},
  {"mor", "zak"},
  {"move", "sido"},
  {"ness", "lacri"},
  {"ning", "illa"},
  {"per", "duda"},
  {"ra", "gru"},
  {"re", "candus"},
  {"son", "sabru"},
  {"tect", "infra"},
  {"tri", "cula"},
  {"ven", "nofo"},
  {"word of", "inset"},
  {"ess", "zxk"},
  {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"}, {"g", "t"},
  {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"},
  {"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"}, {"u", "e"},
  {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""}
};

const char *unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */

int mag_manacost(struct char_data *ch, int spellnum)
{
  return MAX(SINFO.mana_max - (SINFO.mana_change *
		    (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])),
	     SINFO.mana_min);
}


void say_spell(struct char_data *ch, int spellnum, struct char_data *tch,
	            struct obj_data *tobj)
{
  char lbuf[256], buf[256], buf1[256], buf2[256];	/* FIXME */
  const char *format;

  struct char_data *i;
  int j, ofs = 0;

  *buf = '\0';
  strlcpy(lbuf, skill_name(spellnum), sizeof(lbuf));

  while (lbuf[ofs]) {
    for (j = 0; *(syls[j].org); j++) {
      if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
	strcat(buf, syls[j].news);	/* strcat: BAD */
	ofs += strlen(syls[j].org);
        break;
      }
    }
    /* i.e., we didn't find a match in syls[] */
    if (!*syls[j].org) {
      log("No entry in syllable table for substring of '%s'", lbuf);
      ofs++;
    }
  }

  if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch)) {
    if (tch == ch)
      format = "$n closes $s eyes and utters the words, '%s'.";
    else
      format = "$n stares at $N and utters the words, '%s'.";
  } else if (tobj != NULL &&
	     ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
    format = "$n stares at $p and utters the words, '%s'.";
  else
    format = "$n utters the words, '%s'.";

  snprintf(buf1, sizeof(buf1), format, skill_name(spellnum));
  snprintf(buf2, sizeof(buf2), format, buf);

  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
    if (i == ch || i == tch || !i->desc || !AWAKE(i))
      continue;
    if (GET_CLASS(ch) == GET_CLASS(i))
      perform_act(buf1, ch, tobj, tch, i);
    else
      perform_act(buf2, ch, tobj, tch, i);
  }

  if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch)) {
    snprintf(buf1, sizeof(buf1), "$n stares at you and utters the words, '%s'.",
	    GET_CLASS(ch) == GET_CLASS(tch) ? skill_name(spellnum) : buf);
    act(buf1, FALSE, ch, NULL, tch, TO_VICT);
  }
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */
const char *skill_name(int num)
{
  if (num > 0 && num <= TOP_SPELL_DEFINE)
    return (spell_info[num].name);
  else if (num == -1)
    return ("UNUSED");
  else
    return ("UNDEFINED");
}

	 
int find_skill_num(char *name)
{
  int skindex, ok;
  char *temp, *temp2;
  char first[256], first2[256], tempbuf[256];

  for (skindex = 1; skindex <= TOP_SPELL_DEFINE; skindex++) {
    if (is_abbrev(name, spell_info[skindex].name))
      return (skindex);

    ok = TRUE;
    strlcpy(tempbuf, spell_info[skindex].name, sizeof(tempbuf));	/* strlcpy: OK */
    temp = any_one_arg(tempbuf, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
	ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2)
      return (skindex);
  }

  return (-1);
}



/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(struct char_data *caster, struct char_data *cvict, struct obj_data *ovict, int param1, int spellnum, int level, int casttype)
{
  int savetype;

  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
    return (0);

  if (!cast_wtrigger(caster, cvict, ovict, spellnum))
    return 0;
  if (!cast_otrigger(caster, ovict, spellnum))
    return 0;
  if (!cast_mtrigger(caster, cvict, spellnum))
    return 0;

  if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC) && (GET_LEVEL(caster) < LVL_SAINT) ) {
    if (world[IN_ROOM(caster)].nomagic_message_caster != NULL && 
        world[IN_ROOM(caster)].nomagic_message_room != NULL){
       send_to_char(caster, "%s\r\n", world[IN_ROOM(caster)].nomagic_message_caster);
       act(world[IN_ROOM(caster)].nomagic_message_room, FALSE, caster, 0, 0, 
TO_ROOM);
    } else {
    send_to_char(caster, "Your magic fizzles out and dies.\r\n");
    act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
    }
    return (0);
  }
  if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
    send_to_char(caster, "A flash of white light fills the room, dispelling your violent magic!\r\n");
    act("White light from no particular source suddenly fills the room, then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }
  /* determine the type of saving throw */
  switch (casttype) {
  case CAST_STAFF:
  case CAST_SCROLL:
  case CAST_POTION:
  case CAST_WAND:
    savetype = SAVING_ROD;
    break;
  case CAST_SPELL:
    savetype = SAVING_SPELL;
    break;
  default:
    savetype = SAVING_BREATH;
    break;
  }


  if (IS_SET(SINFO.routines, MAG_DAMAGE))
    if (mag_damage(level, caster, cvict, spellnum, savetype) == -1)
      return (-1);	/* Successful and target died, don't cast again. */

  if (IS_SET(SINFO.routines, MAG_AFFECTS))
    mag_affects(level, caster, cvict, param1, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
    mag_unaffects(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_POINTS))
    mag_points(level, caster, cvict, param1, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
    mag_alter_objs(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_GROUPS))
    mag_groups(level, caster, param1, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_MASSES))
    mag_masses(level, caster, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_AREAS))
    mag_areas(level, caster, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_SUMMONS))
    mag_summons(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_CREATIONS))
    mag_creations(level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_MANUAL))
    switch (spellnum) {
    case SPELL_ARBOREAL_FORM:   MANUAL_SPELL(spell_arboreal_form); break;
    case SPELL_ARCANE_LORE:     MANUAL_SPELL(spell_arcane_lore); break;
    case SPELL_ASTRAL_PROJECTION:MANUAL_SPELL(spell_astral_projection); break;
    case SPELL_ANIMAL_FRIENDSHIP:
    case SPELL_BLOOD_QUENCH:          MANUAL_SPELL(spell_blood_quench); break;
    case SPELL_CALM:                  MANUAL_SPELL(spell_calm); break;
    case SPELL_CANNIBALIZE:           MANUAL_SPELL(spell_cannibalize); break;
    case SPELL_CHARM_BEAST:
    case SPELL_CHARM_MONSTER:
    case SPELL_CHARM_PERSON:
    case SPELL_CONTROL_PLANT:
    case SPELL_CONTROL_UNDEAD:
    case SPELL_VAMPIRIC_GAZE:
    case SPELL_CHARM:		MANUAL_SPELL(spell_charm); break;

   //  Spells to get portal names
    case SPELL_BEFRIEND_DRYAD:
    case SPELL_BIND_PORTAL_MAJOR:
    case SPELL_BIND_PORTAL_MINOR:
    case SPELL_LOCATE_SHADOW_PLANE:   MANUAL_SPELL(spell_bind_portal); break;

   case SPELL_CONTROL_WEATHER:       MANUAL_SPELL(spell_control_weather); break;

    case SPELL_CREATE_WATER:	MANUAL_SPELL(spell_create_water); break;
    case SPELL_DETECT_POISON:	MANUAL_SPELL(spell_detect_poison); break;
    case SPELL_ENCHANT_WEAPON:  MANUAL_SPELL(spell_enchant_weapon); break;
    case SPELL_FEIGN_DEATH:           MANUAL_SPELL(spell_feign_death); break;
    case SPELL_FUMBLE:                MANUAL_SPELL(spell_fumble); break;

    case SPELL_ETHEREAL_PROJECTION: MANUAL_SPELL(spell_ethereal_projection);break;
    case SPELL_HANG:            MANUAL_SPELL(spell_hang); break;
    case SPELL_IDENTIFY:	MANUAL_SPELL(spell_identify); break;
    case SPELL_KNOCK:                 MANUAL_SPELL(spell_knock); break;
   case SPELL_RECHARGE:              MANUAL_SPELL(spell_recharge); break;
    case SPELL_REST_IN_PEACE:         MANUAL_SPELL(spell_rest_in_peace); break;

    case SPELL_LOCATE_OBJECT:   MANUAL_SPELL(spell_locate_object); break;
    case SPELL_SUMMON:		MANUAL_SPELL(spell_summon); break;
    case SPELL_WORD_OF_RECALL:  MANUAL_SPELL(spell_recall); break;
    case SPELL_RECALL_TO_SORIN: MANUAL_SPELL(spell_sorin_recall); break;
    case SPELL_PHASE_DOOR:      MANUAL_SPELL(spell_phase_door); break;
    case SPELL_TELEPORT:	MANUAL_SPELL(spell_teleport); break;
    case SPELL_SUCCOR:                MANUAL_SPELL(spell_succor); break;
 
    // Spells to travel through portals
    case SPELL_DIMENSION_SHIFT:
    case SPELL_DIMENSION_WALK:
    case SPELL_SHADOW_WALK:
    case SPELL_PASS_WITHOUT_TRACE:
         MANUAL_SPELL(spell_portal);
/*    case SPELL_DIMENSION_DOOR:
    case SPELL_PLANAR_TRAVEL:
    case SPELL_SHADOW_DOOR:
    case SPELL_TRAIL_OF_WOODLANDS:
         MAG_GROUPS(spell_portal);*/
         break;


    case SPELL_CLAN_RECALL:	MANUAL_SPELL(spell_clan_recall); break;

    case SPELL_TELEPORT_MAJOR:
    case SPELL_TELEPORT_MINOR:        MANUAL_SPELL(spell_teleportm); break;

    case SPELL_TELEVIEW_MAJOR:
    case SPELL_TELEVIEW_MINOR:	MANUAL_SPELL(spell_teleview); break;
    case SPELL_SPOOK:           MANUAL_SPELL(spell_spook); break;
    case SPELL_VITALITY:              MANUAL_SPELL(spell_vitality); break;
    }


  return (1);
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */
void mag_objectmagic(struct char_data *ch, struct obj_data *obj,
		          char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  int i, k;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;

  one_argument(argument, arg);

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		   FIND_OBJ_EQUIP, ch, &tch, &tobj);

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_STAFF:
    act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
    else
      act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);

    if (GET_OBJ_VAL(obj, 2) <= 0) {
	  extract_obj(obj);
      act("$p loses its magical energy and disintegrates.", FALSE, ch, obj, 0, TO_CHAR);
      act("$p loses its magical energy and crumbles in $n's hands.", FALSE, ch, obj, 0, TO_ROOM);
    } else {
      GET_OBJ_VAL(obj, 2)--;
      WAIT_STATE(ch, PULSE_VIOLENCE);
      /* Level to cast spell at. */
      k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;

      /*
       * Problem : Area/mass spells on staves can cause crashes.
       * Solution: Remove the special nature of area/mass spells on staves.
       * Problem : People like that behavior.
       * Solution: We special case the area/mass spells here.
       */
      if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), MAG_MASSES | MAG_AREAS)) {
        for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
	  i++;
	while (i-- > 0 )
	  call_magic(ch, NULL, NULL, NOWHERE, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
      } else {
	for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
	  next_tch = tch->next_in_room;
	  if (ch != tch)
	    call_magic(ch, tch, NULL, NOWHERE, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
	}
      }
    }
    break;
  case ITEM_WAND:
    if (GET_LEVEL(tch) < LVL_SAINT) {
    if (k == FIND_CHAR_ROOM) {
      if (tch == ch) {
	act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
	act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
      } else {
	act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
	if (obj->action_description)
	  act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
	else
	  act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
      }
    } else if (tobj != NULL) {
      act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
      if (obj->action_description)
	act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
      else
	act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
    } else if (IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES)) {
      /* Wands with area spells don't need to be pointed. */
      act("You point $p outward.", FALSE, ch, obj, NULL, TO_CHAR);
      act("$n points $p outward.", TRUE, ch, obj, NULL, TO_ROOM);
    } else {
      act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
      return;
    }

    if (GET_OBJ_VAL(obj, 2) <= 0) {
	  extract_obj(obj);
      act("$p loses its magical energy and disintegrates.", FALSE, ch, obj, 0, TO_CHAR);
      act("$p loses its magical energy and crumbles in $n's hands.", FALSE, ch, obj, 0, TO_ROOM);
      return;
    }
    GET_OBJ_VAL(obj, 2)--;
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (GET_OBJ_VAL(obj, 0))
      call_magic(ch, tch, tobj, NOWHERE, GET_OBJ_VAL(obj, 3),
		 GET_OBJ_VAL(obj, 0), CAST_WAND);
    else
      call_magic(ch, tch, tobj, NOWHERE, GET_OBJ_VAL(obj, 3),
		 DEFAULT_WAND_LVL, CAST_WAND);
 }   
else
act("You can't use $p on $N.", FALSE, ch, obj, tch, TO_CHAR);
   break;
  case ITEM_SCROLL:
    if (*arg) {
      if (!k) {
	act("There is nothing to here to affect with $p.", FALSE,
	    ch, obj, NULL, TO_CHAR);
	return;
      }
    } else
      tch = ch;

    act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
      if (call_magic(ch, tch, tobj, NOWHERE, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_SCROLL) <= 0)
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  case ITEM_POTION:
    tch = ch;
    act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
      if (call_magic(ch, ch, NULL, NOWHERE, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_POTION) <= 0)
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  default:
    log("SYSERR: Unknown object_type %d in mag_objectmagic.",
	GET_OBJ_TYPE(obj));
    break;
  }
}


/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */
int cast_spell(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int param1, int spellnum)
{
  if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE) {
    log("SYSERR: cast_spell trying to call spellnum %d/%d.", spellnum,
	TOP_SPELL_DEFINE);
    return (0);
  }
    
  if (GET_POS(ch) < SINFO.min_position) {
    switch (GET_POS(ch)) {
      case POS_SLEEPING:
      send_to_char(ch, "You dream about great magical powers.\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "You cannot concentrate while resting.\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "You can't do this sitting!\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "Impossible!  You can't concentrate enough!\r\n");
      break;
    default:
      send_to_char(ch, "You can't do much of anything like this!\r\n");
      break;
    }
    return (0);
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
    send_to_char(ch, "You are afraid you might hurt your master!\r\n");
    return (0);
  }
  if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY)) {
    send_to_char(ch, "You can only cast this spell upon yourself!\r\n");
    return (0);
  }
  if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF)) {
    send_to_char(ch, "You cannot cast this spell upon yourself!\r\n");
    return (0);
  }
  if (IS_SET(SINFO.routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "You can't cast this spell if you're not in a group!\r\n");
    return (0);
  }
  
   /* send_to_char(ch, "%s", OK); <-- that's just dumb */
  
  say_spell(ch, spellnum, tch, tobj);

  return (call_magic(ch, tch, tobj, param1, spellnum, GET_LEVEL(ch), CAST_SPELL));
}


/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */
ACMD(do_cast)
{
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  char *s, *t;
  int mana, spellnum, i, param1=NOTHING, target = 0;

  if (IS_NPC(ch))
    return;

  /* get: blank, spell name, target name */
  s = strtok(argument, "'");

  if (s == NULL) {
    send_to_char(ch, "Cast what where?\r\n");
    return;
  }
  s = strtok(NULL, "'");
  if (s == NULL) {
    send_to_char(ch, "Spell names must be enclosed in the Holy Magic Symbols: '\r\n");
    return;
  }
  t = strtok(NULL, "\0");

  /* spellnum = search_block(s, spells, 0); */
  spellnum = find_skill_num(s);

  if ((spellnum < 1) || (spellnum > MAX_SPELLS)) {
    send_to_char(ch, "Cast what?!?\r\n");           
    return;
  }
  if ((GET_LEVEL(ch) < SINFO.min_level[(int) GET_CLASS(ch)]) && (GET_SKILL(ch, spellnum) == 0)) {
    send_to_char(ch, "You do not know that spell!\r\n");
    return;
  }
//  if (GET_SKILL(ch, spellnum) == 0) {
//    send_to_char(ch, "You are unfamiliar with that spell.\r\n");
//    return;
//  }
  /* Find the target */
  if (t != NULL) {
    char arg[MAX_INPUT_LENGTH];

    strlcpy(arg, t, sizeof(arg));
    one_argument(arg, t);
    skip_spaces(&t);
  }

  if (spellnum == SPELL_LOCATE_OBJECT) {
    /* use the CREATE macro instead of the create_obj func so we don't
     * add the temp obj to the global list, avoiding the overhead of
     * adding and removing it.
     */
    CREATE(tobj, struct obj_data, 1);
    tobj->name = (t && *t ? strdup(t) : NULL);
    /* could get fancy here and support multiple arguments, but the code in
     * spells.c would have to be updated too.  Anyone want to write it? :-)
     */                       
    target = TRUE;
  }

  if (IS_SET(SINFO.targets, TAR_IGNORE)) {
    target = TRUE;
  } else if (t != NULL && *t) {
    if (!target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
      if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_ROOM)) != NULL)
	target = TRUE;
    }
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_ZONE))
      if (((tch = get_char_vis(ch, t, NULL, FIND_CHAR_WORLD)) != NULL) &&
         (world[IN_ROOM(ch)].zone == world[IN_ROOM(tch)].zone))
        target = TRUE;
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
      if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_WORLD)) != NULL)
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_INV))
      if ((tobj = get_obj_in_list_vis(ch, t, NULL, ch->carrying)) != NULL)
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
      for (i = 0; !target && i < NUM_WEARS; i++)
	if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->name)) {
	  tobj = GET_EQ(ch, i);
	  target = TRUE;
	}
    }
    if (!target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
      if ((tobj = get_obj_in_list_vis(ch, t, NULL, world[IN_ROOM(ch)].contents)) != NULL)
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
      if ((tobj = get_obj_vis(ch, t, NULL)) != NULL)
	target = TRUE;
    if (!target && IS_SET(SINFO.targets, TAR_DIRECTION))
      if (((param1 = search_block(t, dirs, FALSE)) > -1) || ((param1 = search_block(t, abbr_dirs, FALSE)) > -1))
        target = TRUE;

 if (!target && IS_SET(SINFO.targets, TAR_PORTAL_CODE)) {
      param1 = portal_code_decrypt(ch, t, spellnum);
      if (VALID_ROOM_RNUM(param1)) {
        if (CAN_USE_ROOM(ch, param1)) {
          target = TRUE;
          if ((IS_SET(SINFO.targets, TAR_ROOM_IN_ZONE)) && (world[IN_ROOM(ch)].zone != world[param1].zone))
            target = FALSE;
        }
      }
    }




  } else {			/* if target string is empty */
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
      if (FIGHTING(ch) != NULL) {
	tch = ch;
	target = TRUE;
      }
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
      if (FIGHTING(ch) != NULL) {
	tch = FIGHTING(ch);
	target = TRUE;
      }
    /* if no target specified, and the spell isn't violent, default to self */
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
	!SINFO.violent) {
      tch = ch;
      target = TRUE;
    }
    if (!target && IS_SET(SINFO.targets, TAR_PORTAL_CODE)) {
      send_to_char(ch, "What is the portal name?\r\n");
      return;
    }

    if (!target && IS_SET(SINFO.targets, TAR_DIRECTION)) {
      send_to_char(ch, "In which direction should the spell be cast?\r\n");
      return;           
    }

    if (!target) {
      send_to_char(ch, "Upon %s should the spell be cast?\r\n",
		IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? "what" : "who");
      return;
    }
  }
 
  if (target && (tch == ch) && SINFO.violent) {
    send_to_char(ch, "You shouldn't cast that on yourself -- could be bad for your health!\r\n");
    return;
  }

 if (!target) {
    if IS_SET(SINFO.targets, TAR_PORTAL_CODE) {
      send_to_char(ch, "Invalid portal name. What is the correct portal name?\r\n");
      return;
    }
 }



  if (!target) {
    send_to_char(ch, "Cannot find the target of your spell!\r\n");
    return;
    if IS_SET(SINFO.targets, TAR_DIRECTION) {
      send_to_char(ch, "Invalid direction.  What direction should the spell be cast?\r\n");
      return;
    }

  }
  mana = mag_manacost(ch, spellnum);
  if ((mana > 0) && (GET_MANA(ch) < mana) && (GET_LEVEL(ch) < LVL_SAINT)) {
    send_to_char(ch, "You haven't the energy to cast that spell!\r\n");
    return;
  }

  /* You throws the dice and you takes your chances.. 101% is total failure */
  if (rand_number(0, 101) > GET_SKILL(ch, spellnum)) {
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (!tch || !skill_message(0, ch, tch, spellnum))
      send_to_char(ch, "You lost your concentration!\r\n");
    if (mana > 0)
      GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana / 2)));
    if (SINFO.violent && tch && IS_NPC(tch))
      hit(tch, ch, TYPE_UNDEFINED);
  } else { /* cast spell returns 1 on success; subtract mana & set waitstate */
    if (cast_spell(ch, tch, tobj, param1, spellnum)) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
      if (mana > 0)
	GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
    }
  }                            

  /* send a warning message when mana is getting low */
  if ( GET_MANA(ch) < (GET_MAX_MANA(ch) / 10) )
  {
    send_to_char(ch, "Your power is fading fast!\r\n");
  }
 }



void spell_level(int first_prereq, int second_prereq, int spell, int chclass, int level)
{
  int bad = 0;

  if (spell < 0 || spell > TOP_SPELL_DEFINE) {
    log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (chclass < 0 || chclass >= NUM_CLASSES) {
    log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
		chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (level < 1 || level > LVL_IMPL) {
    log("SYSERR: assigning '%s' to illegal level %d/%d.", skill_name(spell),
		level, LVL_IMPL);
    bad = 1;
  }

  if ( (first_prereq < 0 && first_prereq != TYPE_UNDEFINED) || first_prereq > TOP_SPELL_DEFINE) {
    log("SYSERR: attempting to assign illegal first pre-requisite spellnum of %d to spellnum %d/%d", first_prereq, spell, TOP_SPELL_DEFINE);
    bad =1;
  }  
  
  if ( (second_prereq < 0 && second_prereq != TYPE_UNDEFINED) || second_prereq > TOP_SPELL_DEFINE) {
    log("SYSERR: attempting to assign illegal second pre-requiste spellnum of %d to spellnum %d/%d", second_prereq, spell, TOP_SPELL_DEFINE);
    bad =1;
  }

  if (!bad){
    spell_info[spell].min_level[chclass] = level;
    spell_info[spell].first_prereq[chclass] = first_prereq;
    spell_info[spell].second_prereq[chclass] = second_prereq;
    }
}


/* Assign the spells on boot up */
void spello(int spl, const char *name, int max_mana, int min_mana,
	int mana_change, int minpos, int targets, int violent, int routines, const char *wearoff)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
  spell_info[spl].min_level[i] = LVL_SAINT;
  spell_info[spl].mana_max = max_mana;
  spell_info[spl].mana_min = min_mana;
  spell_info[spl].mana_change = mana_change;
  spell_info[spl].min_position = minpos;
  spell_info[spl].targets = targets;
  spell_info[spl].violent = violent;
  spell_info[spl].routines = routines;
  spell_info[spl].name = name;
  spell_info[spl].wear_off_msg = wearoff;
  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].first_prereq[i] = TYPE_UNDEFINED;
  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].second_prereq[i] = TYPE_UNDEFINED;

}


void unused_spell(int spl)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].min_level[i] = LVL_IMPL + 1;
  spell_info[spl].mana_max = 0;
  spell_info[spl].mana_min = 0;
  spell_info[spl].mana_change = 0;
  spell_info[spl].min_position = 0;
  spell_info[spl].targets = 0;
  spell_info[spl].violent = 0;
  spell_info[spl].routines = 0;
  spell_info[spl].name = unused_spellname;
  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].first_prereq[i] = TYPE_UNDEFINED;
  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].second_prereq[i] = TYPE_UNDEFINED;
}

#define skillo(skill, name) spello(skill, name, 0, 0, 0, 0, 0, 0, 0, NULL);


/*
 * Arguments for spello calls:
 *
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL).
 *
 * spellname: The name of the spell.
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

/*
 * NOTE: SPELL LEVELS ARE NO LONGER ASSIGNED HERE AS OF Circle 3.0 bpl9.
 * In order to make this cleaner, as well as to make adding new classes
 * much easier, spell levels are now assigned in class.c.  You only need
 * a spello() call to define a new spell; to decide who gets to use a spell
 * or skill, look in class.c.  -JE 5 Feb 1996
 */

void mag_assign_spells(void)
{
  int i;

  /* Do not change the loop below. */
  for (i = 0; i <= TOP_SPELL_DEFINE; i++)
    unused_spell(i);
  /* Do not change the loop above. */
  spello(SPELL_ACCURACY, "accuracy", 55, 25, 3, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "Your attacks hit with less precision now.");

  spello(SPELL_ACID_ARROW, "acid arrow", 50, 20, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);
  spello(SPELL_AID, "aid", 40, 10, 3, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel less fit.");

  spello(SPELL_AIRWALK, "airwalk", 50, 15, 3, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feet feel heavier and began to drag as you walk.");
  spello(SPELL_ANIMAL_FRIENDSHIP, "animal friendship", 75, 35, 2, POS_STANDING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, "You feel more self-confident.");

  spello(SPELL_ANIMATE_DEAD, "animate dead", 35, 10, 3, POS_STANDING,
	TAR_OBJ_ROOM, FALSE, MAG_SUMMONS,
	NULL);
  spello(SPELL_ARBOREAL_FORM, "arboreal form", 200, 150, 10, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL,
	NULL);
  spello(SPELL_ARCANE_LORE, "arcane lore", 200, 150, 10, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_ARMOR, "armor", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less protected.");
  
 spello(SPELL_ASPHYXIATE, "asphyxiate", 100, 60, 4, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, "You finally inhale some fresh air and catch your breath.");
  spello(SPELL_ASTRAL_ASCENSION, "astral ascension", 300, 300, 0, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);
                    
  spello(SPELL_ASTRAL_PROJECTION, "astral projection", 200, 150, 10, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL,
	NULL);
        
spello(SPELL_BALL_LIGHTNING, "ball lightning", 100, 70, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);
 spello(SPELL_BARKSKIN, "barkskin", 25, 5, 2, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel your skin revert back to flesh.");

spello(SPELL_BAT_SONAR, "bat sonar", 50, 10, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You notice that sound is no longer as sharp.");
spello(SPELL_BEFRIEND_DRYAD, "befriend dryad", 30, 10, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, NULL);

 spello(SPELL_BENEFICENCE, "beneficence", 50, 35, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You no longer feel the mystical aura of peace and harmony.");
 spello(SPELL_BIND_PORTAL_MAJOR, "bind portal major", 40, 20, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, NULL);
  spello(SPELL_BIND_PORTAL_MINOR, "bind portal minor", 20, 10, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, NULL);

  spello(SPELL_BLESS, "bless", 35, 5, 3, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel less righteous.");

  spello(SPELL_BLINDNESS, "blindness", 35, 25, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS,
	"You feel a cloak of blindness dissolve.");

  spello(SPELL_BLOODLUST, "bloodlust", 110, 80, 3, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS,
	"You ache as the surge of power you once felt leaves your body.");
  spello(SPELL_BLOOD_REVEL, "blood revel", 50, 30, 5, POS_STANDING,
            TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_POINTS, NULL);
 spello(SPELL_BLOOD_QUENCH, "blood quench", 30, 5, 4, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL, NULL);

spello(SPELL_BOLT_OF_STEEL, "bolt of steel", 35, 15, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);

   spello(SPELL_BOULDER, "boulder", 75, 30, 10, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
        NULL);

  spello(SPELL_BURNING_HANDS, "burning hands", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_CALL_LIGHTNING, "call lightning", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);
 spello(SPELL_CANNIBALIZE, "cannibalize", 30, 5, 4, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL, NULL);

  spello(SPELL_CAUSE_MAJOR, "cause major wounds", 40, 15, 2, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);
  spello(SPELL_CAUSE_MINOR, "cause minor wounds", 30, 10, 2, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);
  spello(SPELL_CALM, "calm", 150, 100, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_AREAS, NULL);

  spello(SPELL_CHAIN_LIGHTNING, "chain lightning", 140, 100, 10, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL);

  spello(SPELL_CHAMPIONS_STRENGTH, "champions strength", 75, 50, 5, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "Your armor grows heavy as the strength of ancient heroes vacate your body.");

  spello(SPELL_CHARM_BEAST, "charm beast", 75, 35, 2, POS_STANDING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, "You feel more self-confident.");
  spello(SPELL_CHARM_MONSTER, "charm monster", 75, 35, 2, POS_STANDING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, "You feel more self-confident.");
  spello(SPELL_CHARM_PERSON, "charm person", 75, 35, 2, POS_STANDING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, "You feel more self-confident.");
	

  spello(SPELL_CHILL_TOUCH, "chill touch", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
	"You feel your strength return.");

  spello(SPELL_CLAN_RECALL, "clan recall", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_CLENCHED_FIST, "clenched fist", 80, 50, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);

  spello(SPELL_CLOAK_OF_DARKNESS, "cloak of darkness", 70, 35, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "The darkness dissipates letting the light shine in.");
  spello(SPELL_CLOAK_OF_SHADOWS, "cloak of shadows", 50, 15, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "The shadows leave, leaving you exposed.");
  spello(SPELL_CLOAK_OF_THE_NIGHT, "cloak of the night", 60, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "The darkness dissipates letting the light shine in.");
  

  spello(SPELL_CHI_FIST, "chi fist", 30, 15, 5, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
        NULL);

  spello(SPELL_CLONE, "clone", 80, 65, 5, POS_STANDING,
	TAR_SELF_ONLY, FALSE, MAG_SUMMONS,
	NULL);

  spello(SPELL_COLOR_SPRAY, "color spray", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);
 spello(SPELL_CONJURE_ELEMENTAL, "conjure elemental", 80, 50, 3, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL);
  spello(SPELL_CONJURE_UNDEAD, "conjure undead", 100, 70, 3, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL);

  spello(SPELL_CONTROL_PLANT, "control plant", 75, 35, 2, POS_STANDING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, "You feel more self-confident.");
  spello(SPELL_CONTROL_UNDEAD, "control undead", 75, 35, 2, POS_STANDING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, "You feel more self-confident.");
  spello(SPELL_CONTROL_WEATHER, "control weather", 75, 25, 5, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_CREATE_FOOD, "create food", 30, 5, 4, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_CREATIONS,
	NULL);

  spello(SPELL_CREATE_WATER, "create water", 30, 5, 4, POS_STANDING,
	TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_CURE_BLIND, "cure blind", 30, 5, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS,
	NULL);

  spello(SPELL_CURE_CRITIC, "cure critic", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	NULL);

  spello(SPELL_CURE_LIGHT, "cure light", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	NULL);
 spello(SPELL_CURE_SERIOUS, "cure serious", 50, 20, 5, POS_FIGHTING,
        TAR_CHAR_ROOM, FALSE, MAG_POINTS, NULL);

  spello(SPELL_CURSE, "curse", 80, 50, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, TRUE, MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel more optimistic.");

 spello(SPELL_DEATH_STRIKE, "death strike", 200, 150, 10, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You are tired after exerting yourself so much.");

  spello(SPELL_DEATHS_DOOR, "deaths door", 40, 20, 4, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel death closer at hand.");
 
  spello(SPELL_DECREPIFY, "decrepify", 60, 40, 5, POS_STANDING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS | MAG_MATERIALS,
        "You feel less decrepid");

spello(SPELL_DERVISH_SPIN, "dervish spin", 52, 50, 33, POS_STANDING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
        "You warily jump out of your dervish spin, very dizzy.");


  spello(SPELL_DETECT_ALIGN, "detect alignment", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You feel less aware.");
        
  spello(SPELL_DETECT_EVIL, "detect evil", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You feel less aware of the evil of the world.");
        
  spello(SPELL_DETECT_GOOD, "detect good", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You lose sight of the good in the world.");

  spello(SPELL_DETECT_INVIS, "detect invisibility", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"Your eyes stop tingling.");

  spello(SPELL_DETECT_MAGIC, "detect magic", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"The detect magic wears off.");
  
  spello(SPELL_DETECT_NEUTRAL, "detect neutral", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You feel less aware of the balance of the world.");      

  spello(SPELL_DETECT_POISON, "detect poison", 15, 5, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
	"The detect poison wears off.");
  spello(SPELL_DIMENSION_DOOR, "dimension door", 120, 75, 5, POS_STANDING,
        TAR_PORTAL_CODE, FALSE, MAG_GROUPS, NULL);
  spello(SPELL_DIMENSION_SHIFT, "dimension shift", 35, 15, 4, POS_STANDING,
        TAR_PORTAL_CODE | TAR_ROOM_IN_ZONE, FALSE, MAG_MANUAL, NULL);
spello(SPELL_DIMENSION_DOOR, "dimension door", 120, 75, 5, POS_STANDING,
        TAR_PORTAL_CODE, FALSE, MAG_GROUPS, NULL);
 spello(SPELL_DIMENSION_WALK, "dimension walk", 60, 30, 5, POS_STANDING,
        TAR_PORTAL_CODE, FALSE, MAG_MANUAL, NULL);

  spello(SPELL_DISPEL_EVIL, "dispel evil", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);
  spello(SPELL_DISPEL_GOOD, "dispel good", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);
 spello(SPELL_DISPEL_MAGIC, "dispel magic", 40, 10, 5, POS_STANDING,
        TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS, NULL);
 spello(SPELL_DISPEL_SILENCE, "dispel silence", 25, 5, 5, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS, NULL);

  spello(SPELL_DIVINE_HEAL, "divine heal", 165, 150, 5, POS_FIGHTING,
        TAR_CHAR_ROOM, FALSE, MAG_POINTS,
        NULL);
 spello(SPELL_DRAW_UPON_HOLY_MIGHT, "draw upon holy might", 30, 10, 2, POS_FIGHTING,
        TAR_ATTRIBUTE, FALSE, MAG_AFFECTS, "You feel physically weak as the energy of your god drains from your body.");
 spello(SPELL_DREAMSIGHT, "dreamsight", 30, 10, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "Your heightened awareness dissipates.");

  spello(SPELL_EARTHQUAKE, "earthquake",40, 25, 3, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL);
  spello(SPELL_ELEMENTAL_AURA, "elemental aura", 80, 50, 5, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "The aura of elemental power surrounding you fades away.");
  spello(SPELL_ELEMENTAL_BLAST, "elemental blast", 75, 50, 5, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);
  spello(SPELL_ELEMENTAL_BURST, "elemental burst", 100, 60, 8, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL);
  spello(SPELL_ELEMENTAL_HANDS, "elemental hands", 25, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);
  spello(SPELL_ELEMENTAL_SHARD, "elemental shard", 60, 20, 4, POS_STANDING, TAR_IGNORE, FALSE, MAG_CREATIONS, NULL);

 spello(SPELL_ELEMENTAL_SHIELD, "elemental shield", 50, 20, 5, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "The shield of elemental power dissipates into nothingness.");

  spello(SPELL_ELEMENTAL_STRIKE, "elemental strike", 50, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);

  spello(SPELL_EMBALM, "embalm", 50, 20, 3, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_ALTER_OBJS, NULL);

  spello(SPELL_ENCHANT_WEAPON, "enchant weapon", 150, 100, 10, POS_STANDING,
	TAR_OBJ_INV, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_ENERGY_DRAIN, "energy drain", 40, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_MANUAL,
	NULL);
  spello(SPELL_ENFEEBLEMENT, "enfeeblement", 50, 20, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, MAG_AFFECTS, "You feel your strength return.");

 spello(SPELL_ENLARGE, "enlarge", 50, 20, 3, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You shrink back to your natural size.");

  spello(SPELL_ETHEREAL_PROJECTION, "ethereal projection", 200, 150, 10, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL,
	NULL);
        
  spello(SPELL_ETHEREAL_SPHERE, "ethereal sphere", 300, 300, 0, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);
        
  spello(SPELL_EXCOMMUNICATE, "excommunicate", 100, 75, 25, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_POINTS,
        NULL);
  spello(SPELL_FEATHER_FALL, "feather fall", 20, 5, 3, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel more substantial in the air.");
 spello(SPELL_FEIGN_DEATH, "feign death", 50, 30, 2, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_MANUAL, NULL);

  spello(SPELL_FLAMEWALK, "flamewalk", 40, 20, 4, POS_STANDING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You no longer feel resistant to the element of fire.");

  spello(SPELL_FLEET_FEET, "fleet feet", 40, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "Your feet and legs feel their normal weight again.");
        
  spello(SPELL_FORESTATION, "forestation", 300, 300, 0, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);
              
  spello(SPELL_FREE_ACTION, "free action", 50, 20, 5, POS_FIGHTING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You find it slightly more difficult to move.");
 spello(SPELL_FUMBLE, "fumble", 50, 20, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, NULL);
  spello(SPELL_GATE, "gate", 80, 50, 3, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL);

 spello(SPELL_GHOUL_GAUNTLET, "ghoul gauntlet", 80, 40, 4, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "Your hands no longer itch.");
 spello(SPELL_GHOUL_TOUCH, "ghoul touch", 25, 10, 1, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS, "Your feel that your limbs will move again.");

  spello(SPELL_GRANT_BAT_SONAR, "grant bat sonar", 60, 20, 4, POS_STANDING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS, "You notice that sound is no longer as sharp.");


  spello(SPELL_GROUP_ARMOR, "group armor", 50, 30, 2, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);
  spello(SPELL_GROUP_RECALL, "group recall", 50, 30, 2, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);
  spello(SPELL_GROUP_SORIN_RECALL, "group sorin recall", 50, 30, 2, POS_STANDING,
        TAR_IGNORE, FALSE, MAG_GROUPS,
        NULL);

  spello(SPELL_HASTE, "haste", 60, 20, 3, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Suddenly the world seems to sloooowww back down again.");

  spello(SPELL_HEALING_DREAM, "healing dream", 50, 20, 3, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "Peaceful dreams end as you wake up.");

  spello(SPELL_HORNET_SWARM, "hornet swarm", 75, 25, 25, POS_FIGHTING,
            TAR_IGNORE, TRUE, MAG_AREAS,
            NULL);
 spello(SPELL_IMMUNITY_TO_COLD, "immunity to cold", 60, 35, 5, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel a cold chill creep back into your body.");
  spello(SPELL_IMMUNITY_TO_ELEC, "immunity to electricity", 60, 35, 5, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You no longer feel insulated against electricity.");

  spello(SPELL_IMPROVED_INVISIBILITY, "improved invisibility", 50, 30, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel yourself exposed.");
  
  spello(SPELL_INVISIBILITY_TO_ENEMIES, "invis to enemies", 40, 20, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You feel exposed.");
 
  spello(SPELL_FIREBALL, "fireball", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_FIREBOLT, "firebolt", 65, 35, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);

  spello(SPELL_FINGER_OF_DEATH, "finger of death", 75, 20, 15, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
        NULL);
 
 spello(SPELL_FLAILING_FISTS, "flailing fists", 100, 75, 5, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL);
  spello(SPELL_FLAMESTRIKE, "flamestrike", 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);
    spello(SPELL_FLAMING_ARROW, "flaming arrow", 55, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);

   spello(SPELL_FORT, "fortress of hate", 52, 50, 33, POS_STANDING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
        "The aura of hatred surrounding your body fades.");

 spello(SPELL_SCRY_GREATER, "greater scry", 70, 30, 4, POS_STANDING, TAR_PORTAL_CODE, FALSE, MAG_MANUAL, NULL);


  spello(SPELL_GROUP_HEAL, "group heal", 90, 75, 5, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);

    spello(SPELL_HAIL_OF_ARROWS, "hail of arrows", 80, 50, 3, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL);
    
    spello(SPELL_HANG, "hang", 200, 150, 10, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL,
	NULL);

    spello(SPELL_HARM, "harm", 75, 45, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_HEAL, "heal", 45, 30, 5, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	NULL);

  spello(SPELL_HEAL_LIGHT, "heal light", 45, 30, 5, POS_FIGHTING,
        TAR_CHAR_ROOM, FALSE, MAG_POINTS,
        NULL);


  spello(SPELL_HEAL_CRITICAL, "heal critical", 75, 60, 5, POS_FIGHTING,
        TAR_CHAR_ROOM, FALSE, MAG_POINTS,
        NULL);
    
  spello(SPELL_HEALING_WIND, "healing wind", 165, 150, 5, POS_FIGHTING,
        TAR_CHAR_ROOM, FALSE, MAG_MASSES,
        NULL);

  spello(SPELL_HEAL_SERIOUS, "heal serious", 65, 50, 5, POS_FIGHTING,
        TAR_CHAR_ROOM, FALSE, MAG_POINTS,
        NULL);

  spello(SPELL_HEROES_FEAST, "heroes feast", 60, 40, 4, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS, NULL);
 spello(SPELL_HOLD_BEAST, "hold beast", 25, 10, 1, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS, "Your feel that your limbs will move again.");
  spello(SPELL_HOLD_MONSTER, "hold monster", 25, 10, 1, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS, "Your feel that your limbs will move again.");
  spello(SPELL_HOLD_PERSON, "hold person", 25, 10, 1, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS, "Your feel that your limbs will move again.");
  spello(SPELL_HOLD_PLANT, "hold plant", 25, 10, 1, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS, "Your feel that your limbs will move again.");
  spello(SPELL_HOLD_UNDEAD, "hold undead", 25, 10, 1, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS, "Your feel that your limbs will move again.");

 spello(SPELL_HOLY_WORD, "holy word", 100, 60, 8, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL);
  spello(SPELL_HORNETS_DART, "hornets dart", 75, 45, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);

  spello(SPELL_ICE_LANCE, "ice lance", 35, 10, 5, POS_FIGHTING,
            TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
            NULL);

  spello(SPELL_ICE_STORM, "ice storm", 60, 25, 5, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS,
	NULL);
   spello(SPELL_INFRAVISION, "infravision", 25, 10, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"Your night vision seems to fade.");
  spello(SPELL_INTIMIDATE, "intimidate", 45, 25, 2, POS_FIGHTING, TAR_FIGHT_VICT, FALSE, MAG_AFFECTS, "You feel more confident.");

  spello(SPELL_INVISIBLE, "invisibility", 35, 25, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel yourself exposed.");

 spello(SPELL_KNOCK, "knock", 40, 20, 2, POS_STANDING, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_DIRECTION, FALSE, MAG_MANUAL, NULL);
  spello(SPELL_SCRY_LESSER, "lesser scry", 50, 20, 3, POS_STANDING, TAR_PORTAL_CODE | TAR_ROOM_IN_ZONE, FALSE, MAG_MANUAL, NULL);


 spello(SPELL_LEGEND_LORE, "legend lore", 100, 50, 5, POS_STANDING,
        TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL, NULL);

  spello(SPELL_LIFE_LEECH, "life leech", 50, 20, 3, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL);

  spello(SPELL_LIGHTNING_BOLT, "lightning bolt", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_LOCATE_OBJECT, "locate object", 25, 20, 1, POS_STANDING,
	TAR_OBJ_WORLD, FALSE, MAG_MANUAL,
	NULL);
 spello(SPELL_LOCATE_SHADOW_PLANE, "locate shadow plane", 20, 10, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, NULL);


  spello(SPELL_MAGIC_MISSILE, "magic missile", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);
  spello(SPELL_MAGICAL_VESTMANTS, "magical vestmants", 40, 10, 3, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You feel the protection of your prayers unravel from your body.");


   spello(SPELL_MASS_HEAL, "mass heal", 165, 150, 5, POS_FIGHTING,
        TAR_CHAR_ROOM, FALSE, MAG_MASSES,
        NULL);

   spello(SPELL_MASS_SUICIDE, "mass suicide", 300, 300, 0, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);

   spello(SPELL_METEOR, "meteor", 150, 50, 25, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
        NULL);

   spello(SPELL_METEOR_SHOWER, "meteor shower", 150, 50, 50, POS_FIGHTING,
            TAR_IGNORE, TRUE, MAG_AREAS,
            NULL);
  spello(SPELL_MOON_MOTE, "moon mote", 50, 30, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);
  spello(SPELL_PACIFY, "pacify", 30, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS, "You feel more aggresive.");

  spello(SPELL_PARALYZE, "paralyze", 25, 10, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS, "Your feel that your limbs will move again.");
 spello(SPELL_PASS_WITHOUT_TRACE, "pass without trace", 35, 15, 4, POS_STANDING,
        TAR_PORTAL_CODE, FALSE, MAG_MANUAL, NULL);

 spello(SPELL_PHASE_DOOR, "phase door", 60, 30, 3, POS_STANDING, TAR_DIRECTION, FALSE, MAG_MANUAL, NULL);

    spello(SPELL_PILLAR_OF_FLAME, "pillar of flame", 35, 10, 5, POS_FIGHTING,
            TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
            NULL);
 spello(SPELL_PLANAR_TRAVEL, "planar travel", 125, 100, 5, POS_STANDING, TAR_PORTAL_CODE, FALSE, MAG_GROUPS, NULL);

  spello(SPELL_POISON, "poison", 50, 20, 3, POS_STANDING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE,
	MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel less sick.");
  spello(SPELL_PORTAL, "portal", 75, 75, 0, POS_STANDING,
        TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, NULL);
 spello(SPELL_POWER_STRIKE, "power strike", 150, 100, 5, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You are tired after exerting yourself so much.");

  spello(SPELL_PRISMATIC_SPRAY, "prismatic spray", 120, 80, 8, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);

  spello(SPELL_PROT_FROM_EVIL, "protection from evil", 40, 10, 3, POS_STANDING, // one of these has to go
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You feel less protected.");
spello(SPELL_PROTECTION_FROM_EVIL, "protection from evil", 60, 20, 4, POS_STANDING, // one of these has to go
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel less protected.");
   spello(SPELL_PROTECTION_FROM_GOOD, "protection from good", 60, 20, 4, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel less protected.");

  spello(SPELL_RECALL_TO_SORIN, "recall to sorin", 20, 10, 2, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL,
        NULL);      
  spello(SPELL_RECHARGE, "recharge", 150, 100, 10, POS_STANDING, TAR_OBJ_INV, FALSE, MAG_MANUAL, NULL);
  spello(SPELL_REFLECT_DAMAGE, "reflect damage", 80, 50, 5, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "The reflecting shield protecting you dissipates.");
   spello(SPELL_REGENERATION, "regeneration", 50, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel death closer at hand.");
 

  spello(SPELL_REMOVE_CURSE, "remove curse", 45, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
	MAG_UNAFFECTS | MAG_ALTER_OBJS,
	NULL);
 spello(SPELL_REMOVE_PARALYSIS, "remove paralysis", 40, 10, 3, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS, NULL);


  spello(SPELL_REMOVE_POISON, "remove poison", 40, 8, 4, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS,
	NULL);
 spello(SPELL_RESISTANCE_TO_COLD, "resistance to cold", 50, 25, 5, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel a cold chill creep back into your body.");
  spello(SPELL_RESISTANCE_TO_ELEC, "resistance to electricity", 50, 25, 5, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You no longer feel insulated against electricity.");
 spello(SPELL_REST_IN_PEACE, "rest in peace", 80, 50, 3, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL, NULL);
 spello(SPELL_ROAR, "roar", 30, 5, 5, POS_FIGHTING,
            TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
            NULL);
  spello(SPELL_SANCTUARY, "sanctuary", 110, 85, 5, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"The white aura around your body fades.");
  spello(SPELL_SECOND_SIGHT, "second sight", 30, 15, 3, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "Your can no longer well see in the dark or from afar.");
  spello(SPELL_SENSE_LIFE, "sense life", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You feel less aware of your surroundings.");
  spello(SPELL_SEARING_ORB, "searing orb", 100, 70, 5, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL);      
  spello(SPELL_SHADOW_ARMOR, "shadow armor", 35, 15, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "The shadows dissipates from around your body leaving you vulnerable.");
  spello(SPELL_SHADOW_DOOR, "shadow door", 120, 75, 5, POS_STANDING, TAR_PORTAL_CODE, FALSE, MAG_GROUPS, NULL);

  spello(SPELL_SHADOW_WALK, "shadow walk", 35, 15, 4, POS_STANDING, TAR_PORTAL_CODE, FALSE, MAG_MANUAL, NULL);

  spello(SPELL_SILENCE, "silence", 50, 15, 5, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, MAG_AFFECTS, "You feel your vocal cords loosen up.");
 spello(SPELL_SHIELD, "shield", 25, 10, 3, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You feel less protected.");
 
spello(SPELL_SHIELD_AGAINST_EVIL, "shield against evil", 40, 10, 3, POS_STANDING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You feel less protected.");
 spello(SPELL_SHIELD_AGAINST_GOOD, "shield against good", 40, 10, 3, POS_STANDING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You feel less protected.");

  spello(SPELL_SHOCKING_GRASP, "shocking grasp", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);

 spello(SPELL_SHRINK, "shrink", 50, 20, 3, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You grow back to your natural size.");

 spello(SPELL_SKELETAL_GUISE, "skeletal guise", 80, 50, 5, POS_FIGHTING,
        TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel your skin stretch and expand as skeletal guise expires.");

  spello(SPELL_SLEEP, "sleep", 40, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM, TRUE, MAG_AFFECTS,
	"You feel less tired.");
 spello(SPELL_SLEEPWALK, "sleepwalk", 35, 15, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You can no longer walk while you're asleep.");

  spello(SPELL_SMITE_EVIL, "smite evil", 50, 35, 5, POS_FIGHTING, TAR_CHAR_ROOM, TRUE, MAG_DAMAGE, NULL);
  spello(SPELL_SMITE_GOOD, "smite good", 50, 35, 5, POS_FIGHTING, TAR_CHAR_ROOM, TRUE, MAG_DAMAGE, NULL);
  spello(SPELL_SOMNOLENT_GAZE, "somnolent gaze", 50, 30, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel less lethargic.");
  spello(SPELL_SONIC_BLAST, "sonic blast", 60, 20, 20, POS_FIGHTING,
            TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
            NULL);
            
  spello(SPELL_SOVEREIGN_HEAL, "sovereign heal", 90, 75, 5, POS_FIGHTING,
            TAR_CHAR_ROOM, FALSE, MAG_POINTS,
            NULL);
            
  spello(SPELL_SPOOK, "spook", 40, 20, 5, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_MANUAL, NULL);

  spello(SPELL_STRENGTH, "strength", 35, 30, 1, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel weaker.");
  spello(SPELL_STRENGTH_BURST, "strength burst", 75, 50, 5, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "Your armor grows heavy as the strength of ancient heroes vacate your body.");
  spello(SPELL_SYNOSTODWEOMER, "synostodweomer", 65, 35, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_POINTS, NULL);
  spello(SPELL_STUN, "stun", 80, 50, 3, POS_FIGHTING, TAR_FIGHT_VICT, TRUE, MAG_MANUAL, NULL);


  spello(SPELL_SUCCOR, "succor", 25, 15, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, NULL);
  spello(SPELL_SUMMON, "summon", 75, 50, 3, POS_STANDING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
	NULL);
 spello(SPELL_SUMMON_GREATER, "greater summon", 100, 75, 5, POS_STANDING,
        TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, NULL);
 spello(SPELL_SUMMON_LESSER, "lesser summon", 80, 50, 3, POS_STANDING,
        TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, NULL);
 spello(SPELL_SUMMON_AVENGER, "summon avenger", 100, 70, 3, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL);
  spello(SPELL_SUMMON_BEAST, "summon beast", 100, 70, 3, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL);

 spello(SPELL_SUNBURST, "sunburst", 40, 20, 2, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, "You feel a cloak of blindness dissolve.");
 spello(SPELL_SUNRAY, "sunray", 30, 10, 4, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS, "You feel a cloak of blindness dissolve.");


  spello(SPELL_SUSTAIN, "sustain", 60, 30, 5, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You start to feel hungry again.");

  spello(SPELL_SUSTAIN_GROUP, "sustain group", 120, 60, 10, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	"You start to feel hungry again.");      
        
  spello(SPELL_TALES_OF_ARCANE_LORE, "tales of arcane lore", 300, 300, 0, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);      

  spello(SPELL_TELEPORT, "teleport", 75, 50, 3, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
	NULL);
  spello(SPELL_TELEPORT_MINOR, "teleport minor", 50, 25, 5, POS_STANDING, TAR_CHAR_ZONE, FALSE, MAG_MANUAL, NULL);
spello(SPELL_TELEPORT_MAJOR, "teleport major", 70, 40, 3, POS_STANDING, TAR_CHAR_WORLD, FALSE, MAG_MANUAL, NULL);

  spello(SPELL_TELEVIEW_MAJOR, "teleview major", 60, 30, 3, POS_STANDING,
        TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, NULL);
  spello(SPELL_TELEVIEW_MINOR, "teleview minor", 35, 15, 2, POS_STANDING,
        TAR_CHAR_ZONE | TAR_NOT_SELF, FALSE, MAG_MANUAL, NULL);

  spello(SPELL_THUNDER_SWARM, "thunder swarm", 75, 15, 25, POS_FIGHTING,
            TAR_IGNORE, TRUE, MAG_AREAS,
            NULL);
   spello(SPELL_TOWER_OF_STRENGTH, "tower of strength", 70, 50, 4, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You feel vulnerable as your tower of strength disappears.");

 spello(SPELL_TRAIL_OF_WOODLANDS, "trail of the woodlands", 120, 75, 5, POS_STANDING, TAR_PORTAL_CODE, FALSE, MAG_GROUPS, NULL);

   spello(SPELL_UNHOLY_WORD, "unholy word", 100, 60, 8, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL);  

   spello(SPELL_VAMPIRIC_GAZE, "vampiric gaze", 75, 35, 2, POS_STANDING,
        TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_MANUAL, "You feel more self-confident.");

   spello(SPELL_VAMPIRIC_TOUCH, "vampiric touch", 35, 15, 2, POS_FIGHTING,
            TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL);

   spello(SPELL_VIGORIZE_CRITICAL, "vigorize critical", 40, 25, 5, POS_FIGHTING,
            TAR_CHAR_ROOM, FALSE, MAG_POINTS,
            NULL);
   spello(SPELL_VIGORIZE_GROUP, "vigorize group", 65, 50, 5, POS_STANDING, 
            TAR_IGNORE, FALSE, MAG_GROUPS,
            NULL);
   spello(SPELL_VIGORIZE_LIGHT, "vigorize light", 25, 10, 5, POS_FIGHTING,
            TAR_CHAR_ROOM, FALSE, MAG_POINTS,
            NULL);
   spello(SPELL_VIGORIZE_SERIOUS, "vigorize serious", 35, 20, 5, POS_FIGHTING,
            TAR_CHAR_ROOM, FALSE, MAG_POINTS,
            NULL);
  
   spello(SPELL_VITALIZE_MANA, "vitalize mana", 33, 33, 0, POS_STANDING,
            TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_POINTS,
            NULL);
   spello(SPELL_VITALITY, "vitality", 30, 10, 3, POS_STANDING,
        TAR_CHAR_ROOM, FALSE, MAG_MANUAL, NULL);

   spello(SPELL_WAIL_OF_THE_BANSHEE, "wail of the banshee", 150, 100, 10, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS, NULL);
   spello(SPELL_WATERWALK, "waterwalk", 40, 20, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your feet seem less buoyant.");
   spello(SPELL_WINDWALK, "windwalk", 50, 25, 5, POS_STANDING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "The wind stops swirling about your body.");


   spello(SPELL_WITHER, "wither", 55, 25, 3, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, MAG_AFFECTS, "Your shriveled arm grows back to normal.");

   spello(SPELL_WORD_OF_RECALL, "word of recall", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL,
	NULL);
  



  /* NON-castable spells should appear below here. */

  spello(SPELL_IDENTIFY, "identify", 0, 0, 0, 0,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
	NULL);

  /*
   * These spells are currently not used, not implemented, and not castable.
   * Values for the 'breath' spells are filled in assuming a dragon's breath.
   */

  spello(SPELL_FIRE_BREATH, "fire breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0,
	NULL);

  spello(SPELL_GAS_BREATH, "gas breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0,
	NULL);

  spello(SPELL_FROST_BREATH, "frost breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0,
	NULL);

  spello(SPELL_ACID_BREATH, "acid breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0,
	NULL);

  spello(SPELL_LIGHTNING_BREATH, "lightning breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0,
	NULL);

  /* you might want to name this one something more fitting to your theme -Welcor*/
  spello(SPELL_DG_AFFECT, "Script-inflicted", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0,
	NULL);

    
  /*
   * Declaration of skills - this actually doesn't do anything except
   * set it up so that immortals can use these skills by default.  The
   * min level to use the skill for other classes is set up in class.c.
   */


  skillo(SKILL_ARMOR_SPEC, "armor specialization");
  skillo(SKILL_BACKSTAB, "backstab");
  skillo(SKILL_BASH, "bash");

  skillo(SKILL_CIRCLE, "circle");
  skillo(SKILL_CRIT_HIT, "critical hit");

  skillo(SKILL_DETECT_TRAPS, "detect traps");
  skillo(SKILL_DISABLE_TRAP, "disable trap");
  skillo(SKILL_DISARM, "disarm");
  skillo(SKILL_DIST_SIGHT,"distant sight");

  skillo(SKILL_ENDURANCE, "endurance");
  skillo(SKILL_ENVENOM, "envenom");
  skillo(SKILL_ESTATES, "estates");

  skillo(SKILL_FURY, "fury");

  skillo(SKILL_HIDE, "hide");

  skillo(SKILL_IAIJUTSU, "iaijutsu");

  skillo(SKILL_KICK, "kick");

  skillo(SKILL_LAYHANDS, "layhands");

  skillo(SKILL_MEDITATE, "meditate");

  skillo(SKILL_PARRY, "parry");
  skillo(SKILL_PICK_LOCK, "pick lock");

  skillo(SKILL_RAGE, "rage");
  skillo(SKILL_RESCUE, "rescue");
  skillo(SKILL_RETREAT, "retreat");

  skillo(SKILL_SNEAK, "sneak");
  skillo(SKILL_STEAL, "steal");
  skillo(SKILL_SHIELD_SPEC, "shield specialization");
  skillo(SKILL_SHIELDPUNCH, "shield punch");
  skillo(SKILL_SHIELDRUSH, "shield rush");
  skillo(SKILL_SEARCH, "search");


  skillo(SKILL_TRACK, "track");
  skillo(SKILL_TURNING, "turning");


  skillo(SKILL_WHIRLWIND, "whirlwind");

//Languages
  skillo(SKILL_LANG_COMMON, "common");
  skillo(SKILL_LANG_ELVEN, "elven");
  skillo(SKILL_LANG_GNOME, "gnomish");
  skillo(SKILL_LANG_DWARVEN, "dwarven");


//Attack Types

 skillo(SKILL_PROF_PIERCE, "weap type pierce");
  skillo(SKILL_PROF_BLUDGEON, "weap type bludgeon");
  skillo(SKILL_PROF_SLASH, "weap type slash");
  skillo(SKILL_PROF_WHIP, "weap type whip");
  skillo(SKILL_PROF_UNARMED, "unarmed attack");
  skillo(SKILL_PROF_SPECIAL, "special attack");


}

