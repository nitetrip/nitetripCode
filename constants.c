/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h"	/* alias_data */

cpp_extern const char *circlemud_version =
	"CircleMUD, version 3.1";

/* strings corresponding to ordinals/bitvectors in structs.h ***********/


const char *NOEFFECT_RACE = "This spell doesn't affect the victim's race.\r\n";
const char *NOEFFECT_PC = "You cannot do that to PCs or their charmies.\r\n";
// "Use 'murder' to attack another player.\r\n"
const char *VICTIM_RESISTS = "Your victim resists your puny attempt!\r\n";
const char *NOPROFICIENCY = "You have no idea how to do that.\r\n";
const char *PEACE_ROOM_WARNING = "This room just has such a peaceful, easy feeling...\r\n";
const char *FEIGNING_DEATH = "You cannot do that while you're feigning death! (type 'wake' to get up)\r\n";
const char *PC_IS_BURIED = "You cannot do that while you're resting underground! (type 'wake' to get up)\r\n";
const char *PS_RESISTS_POISON = "You resist the deadly toxin... this time.\r\n";



/* (Note: strings for class definitions in class.c instead of here) */



/* PC/mob/room/obj sizes */
const char *sizes[] =
{
  "Special",
  "Tiny",
  "Small",
  "Normal",
  "Large",
  "Giant",
  "Behemoth",
  "\n"
};

/* cardinal directions */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "\n"
};

const char *abbr_dirs[] =
{
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "\n"
};

/* ROOM_x */
const char *room_bits[] = {
  "DARK",
  "DEATH",
  "NO_MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "NO_TRACK",
  "NO_MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HCRSH",
  "ATRIUM",
  "OLC",
  "*",				/* BFS MARK */
  "IMPROOM",
  "DUMP",
  "REWARDDUMP",
  "DUMPONTICK",
  "LIT",  
  "PAIN",
  "NOMAGIC_MSG",
  "NO_SUMMON",
  "NO_SCRY",
  "NO_PORTAL",
  "GOOD",
  "EVIL",
  "FIRE",
  "ICE",
  "WATER",
  "AIR",
  "ASTRAL",
  "ETHEREAL",
  "\n"
};

/* for use in OLC menus */
/* strings must match values in room_bits */
const char *sorted_room_bits[] = {
  "AIR",
  "ASTRAL",
  "ATRIUM",
  "DARK",
  "DEATH",
  "DUMP",
  "DUMPONTICK",
  "ETHEREAL",
  "FIRE",
  "GODROOM",
  "GOOD",
  "EVIL",
  "HCRSH",
  "HOUSE",
  "ICE",
  "IMPROOM",
  "INDOORS",
  "LIT",  
  "NO_MAGIC",
  "NOMAGIC_MSG",
  "NO_MOB",
  "NO_PORTAL",
  "NO_SCRY",
  "NO_SUMMON",
  "NO_TRACK",
  "OLC",
  "PAIN",
  "PEACEFUL",
  "PRIVATE",
  "REWARDDUMP",
  "SOUNDPROOF",
  "TUNNEL",
  "WATER",
  "*",
  "\n"
};


/* EX_x */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "HIDDEN",
  "\n"
};


/* SECT_ */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "In Flight",
  "Underwater",
  "\n"
};

/*
 * SEX_x
 * Not used in sprinttype() so no \n.
 */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
};


/* POS_x */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Paralysed",
  "Stunned",
  "Buried",
  "Feigning Death",
  "Sleeping",
  "Meditating",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "Flying",
  "\n"
};


/* PLR_x */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",
  "DEAD",    /* You should never see this. */
  "JAIL",
  "NOEXPGAIN",
  "PROGRESS",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "\n"
};


/* MOB_x */
const char *action_bits[] = {
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "NO_CHARM",
  "NO_SUMMN",
  "NO_SLEEP",
  "NO_BASH",
  "NO_BLIND",
  "DEAD",    /* You should never see this. */
  "\n"
};


/* PRF_x */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "DEAF",
  "NO_TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "NO_HASS",
  "QUEST",
  "SUMN",
  "NO_REP",
  "LIGHT",
  "C1",
  "C2",
  "NO_WIZ",
  "L1",
  "L2",
  "NO_AUC",
  "NO_GOS",
  "NO_GTZ",
  "RMFLG",
  "CLS",
  "BLDWLK",
  "AFK",
  "D_TAR",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "AUTOSPLIT",
  "\n"
};


/* Spells for Applies menu */
const char *spells[] =
{
"ACID ARROW",
"AIRWALK",
"ANIDEAD",
"ARBOREAL FORM",
"ARCANE LORE",
"ARMOR",
"ASTRAL ASCENSION",
"ASTRAL PROJECTION",
"BALL LIGHTNING",
"BAT SONAR",
"BLESS",
"BLOODLUST",
"BLINDNESS",
"BOLT OF STEEL",
"BOULDER",
"BURNHANDS",
"CALLLIGHT",
"CAUSE MINOR",
"CAUSE MAJOR", 
"CHAIN LIGHTNING",
"CHARM",
"CHILLTOUCH",
"CHIFIST",
"CLENCHED FIST",
"CLONE",
"COLORSPRAY",
"CTRLWEATHR",
"CREATEFOOD",
"CREATEWTR",
"CUREBLIND",
"CURECRIT",
"CURELIGHT",
"CURSE",
"DIVINE HEAL",
"DECREPIFY",
"DETALIGN",
"DETEVIL",
"DETGOOD",
"DETINVIS",
"DETMAGIC",
"DETNEUT",
"DETPOISON",
"DETEVIL",
"DISPGOOD",
"EARTHQUAKE",
"ELEMENTAL BLAST",
"ELEMENTAL BURST",
"ELEMENTAL HANDS",
"ELEMENTAL STRIKE",
"ENCHNTWEAP",
"ENRGYDRAIN",
"ETHEREAL PROJECTION",
"ETHEREAL SPHERE",
"EXCOMMUNICATE",
"FIREBALL",
"FIREBOLT",
"FLAILING FISTS",
"FLAMESTRIKE",
"FLAMING ARROW",
"FORESTATION",
"FORT",
"GRANT BAT SONAR",
"GROUPARMOR",
"GROUPHEAL",
"GROUPRECALL",
"GROUP SORIN RECALL",
"HAIL OF ARROWS",
"HANG",
"HARM",
"HASTE",
"HEAL",
"HEAL CRITICAL",
"HEAL SERIOUS",
"HEALING WIND",
"HOLY WORD",
"HORNETS DART",
"ICESTORM",
"INFRA",
"INVISIBLE",
"LIGHTBOLT",
"LOCATEOBJ",
"MAGMISSILE",
"MASS HEAL",
"MASS SUICIDE",
"METEOR", 
"POISON",
"PRISMATIC SPRAY",
"PFE",
"RECALL",
"RECALL TO SORIN",
"REMCURSE",
"REMPOISON",
"SANCT",
"SEARING ORB",
"SENSELIFE",
"SHOCKGRSP",
"SLEEP",
"SMITE EVIL",
"SMITE GOOD",
"SOVERIEGN HEAL",
"SPOOK",
"STR",
"SUMMON",
"SUSTAIN",
"SUSTAIN GROUP",
"TALES OF ARCANE LORE",
"TELEPORT",
"UNHOLY WORD",
"VIGORIZE CRITICAL",
"VIGORIZE GROUP",
"VIGORIZE LIGHT",
"VIGORIZE SERIOUS",
"VITALIZE MANA",
"VNTRILOQTE",
"WAIL OF THE BANSHEE",
"WATERWALK",
"\n"
};

/* Skills for Applies menu */
const char *skills[]=
{
"BACKSTAB",
"BASH",
"CIRCLE", 
"HIDE",
"KICK",
"PICK_LOCK",
"UNDEFINED",
"RESCUE",
"SNEAK",
"STEAL",
"TRACK",
"DETTRAPS",
"PIERCE",
"BLUDGEON",
"SLASH",
"WHIP",
"FURY",
"DISARM",
"DISTSIGHT",
"CRITHIT",
"\n"
};

/* AFF_x */
const char *affected_bits[] =
{
  "BLIND",
  "INVISIBLE",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "WATERWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "NO_TRACK",
  "SUSTAIN",
  "AIRWALK",
  "SNEAK",
  "HIDE",
  "HASTE",
  "CHARM",
  "FURY",
  "DIST-SIGHT",
  "IMM_POISON",
  "IMM_LIGHT",
  "ANGELIC",
  "BAT_SONAR",
  "BENEFICENCE",
  "CLOAK_OF_DARKNESS",
  "CLOAK_OF_SHADOWS",
  "CLOAK_OF_THE_NIGHT",
  "DEATHS_DOOR",
  "DETECT_TRAPS",
  "DREAMSIGHT",
  "ETHEREAL",
  "FEATHER_FALL",
  "FLEET_FEET",
  "FLYING",
  "FREE_ACTION",
  "HEALING_DREAM",
  "FORT",  
  "DERVISH_SPIN",
  "DECREPIFY",
  "SECOND_SIGHT",
  "IMPROVED_INVIS",
  "INVIS_TO_ENEMIES",
  "MAGICONLY",
  "PARALYZE",
  "PARALYZING_TOUCH",
  "PARRY",
  "REFLECT_DAMAGE",
  "RESERVED",
  "REGENERATION",
  "SHADOW_ARMOR",
  "SILENCE",
  "SLEEPWALK",
  "UNDEAD",
  "WATERBREATH",
  "DETECT_EVIL",
  "DETECT_GOOD",
  "DETECT_NEUTRAL",
  "\n"
};

/* AFF_x */
/* for use in OLC menus */
/* strings must match values in affected_bits */
/* count does not need to match count of affected_bits, but must match
 * NUM_AFF_FLAGS value in oasis.h */
const char *sorted_affected_bits[] =
{
  "AIRWALK",
  "ANGELIC",
  "BAT_SONAR",
  "BENEFICENCE",
  "BLIND",
  "CHARM",
  "CLOAK_OF_DARKNESS",
  "CLOAK_OF_SHADOWS",
  "CLOAK_OF_THE_NIGHT",
  "CURSE",
  "DEATHS_DOOR",
  "DECREPIFY",
  "DERVISH_SPIN",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "DETECT_EVIL",
  "DETECT_GOOD",
  "DETECT_NEUTRAL",
  "DETECT_TRAPS",
  "DIST-SIGHT",
  "DREAMSIGHT",
  "ETHEREAL",
  "FEATHER_FALL",
  "FLEET_FEET",
  "FLYING",
  "FORT",  
  "FREE_ACTION",
  "FURY",
  "GROUP",
  "HASTE",
  "HEALING_DREAM",
  "HIDE",
  "IMM_LIGHT",
  "IMM_POISON",
  "IMPROVED_INVIS",
  "INFRA",
  "INVIS_TO_ENEMIES",
  "INVISIBLE",
  "MAGICONLY",
  "NO_TRACK",
  "PARALYZE",
  "PARALYZING_TOUCH",
  "PARRY",
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "REFLECT_DAMAGE",
  "REGENERATION",
  "SANCT",
  "SECOND_SIGHT",
  "SENSE-LIFE",
  "SHADOW_ARMOR",
  "SILENCE",
  "SLEEP",
  "SLEEPWALK",
  "SNEAK",
  "SUSTAIN",
  "UNDEAD",
  "WATERBREATH",
  "WATERWALK",
  "\n"
};

/* AFF2_x */
const char *affected2_bits[] =
{
  "\n"
};

/* ATTACK_x */
const char *res_types[] =
{
  "PHYSICAL",
  "PIERCE",
  "BLUDGEON",
  "SLASH",
  "WHIP",
  "MAGIC",
  "FIRE",
  "COLD",
  "ELECTRIC",
  "LIGHT",
  "POISON",
  "\n"
};

/* for use in OLC menus */
/* strings must match values in res_types */
const char *sorted_res_types[] =
{
  "BLUDGEON",
  "COLD",
  "ELECTRIC",
  "FIRE",
  "LIGHT",
  "MAGIC",
  "PHYSICAL",
  "PIERCE",
  "POISON",
  "SLASH",
  "WHIP",
  "\n"
};

/* CON_x */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Trigger Edit",
  "Select race",
  "Select stats",
  "Class Branch 1",
  "Class Branch 2",
  "\n"
};


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
const char *wear_where[] = {
  "<used as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn as shield>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<wielded>            ",
  "<held>               "
};


/* WEAR_x - for stat */
const char *equipment_types[] = {
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "\n"
};


/* ITEM_x (ordinal object types) */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FIRE WEAPON",
  "MISSILE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "\n"
};


/* for use in OLC menus */
/* strings must match values in item_types */
/* "UNDEFINED" is always the first in the list, so that it can't
 * be selected in OLC */
const char *sorted_item_types[] = {
  "UNDEFINED",
  "ARMOR",
  "BOAT",
  "CONTAINER",
  "FIRE WEAPON",
  "FOOD",
  "FOUNTAIN",
  "KEY",
  "LIGHT",
  "LIQ CONTAINER",
  "MISSILE",
  "MONEY",
  "NOTE",
  "OTHER",
  "PEN",
  "POTION",
  "SCROLL",
  "STAFF",
  "TRAP",
  "TRASH",
  "TREASURE",
  "WAND",
  "WEAPON",
  "WORN",
  "\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "NO_EQ_MSG",
  "RADIATE",
  "UNUSED14",
  "UNUSED15",
  "NO_SELL",
  "ANTI_HUMAN",
  "ANTI_DWARF",
  "ANTI_ELF",
  "ANTI_GNOME",
  "NO_PURGE",
  "NO_DISARM",
  "NO_CLAIM",
  "GOOD_FLAG",
  "EVIL_FLAG",
  "ANGELIC",
  "DEMONIC",
  "UNUSED28",
  "UNUSED29",
  "UNUSED30",
  "UNUSED31",
  "UNUSED32",
  "UNUSED33",
  "UNUSED34",
  "UNUSED35",
  "UNUSED36",
  "UNUSED37",
  "UNUSED38",
  "UNUSED39",
  "UNUSED40",
  "UNUSED41",
  "UNUSED42",
  "UNUSED43",
  "UNUSED44",
  "UNUSED45",
  "UNUSED46",
  "UNUSED47",
  "UNUSED48",
  "UNUSED49",
  "UNUSED50",
  "GREATER_HIDDEN",
  "NOBITS", 
  "LESSER_HIDDEN",
  "UNUSED55",
  "UNUSED56",
  "UNUESD57",
  "UNUSED58",
  "UNUSED59",
  "UNUSED60",
  "UNUESD61",
  "UNUSED62",
  "UNUSED63",
  "UNUSED64",
  "\n"
};

/* for use in OLC menus */
/* strings must match values in extra_bits */
const char *sorted_extra_bits[] = {
  "ANGELIC",
  "ANTI_DWARF",
  "ANTI_ELF",
  "ANTI_EVIL",
  "ANTI_GNOME",
  "ANTI_GOOD",
  "ANTI_HUMAN",
  "ANTI_NEUTRAL",
  "BLESS",
  "DEMONIC",
  "EVIL_FLAG",
  "GLOW",
  "GOOD_FLAG",
  "GREATER_HIDDEN",
  "HUM",
  "INVISIBLE",
  "LESSER_HIDDEN",
  "MAGIC",
  "NO_CLAIM",
  "NO_DISARM",
  "NO_DONATE",
  "NO_DROP",
  "NO_EQ_MSG",
  "NO_INVIS",
  "NO_PURGE",
  "NO_RENT",
  "NO_SELL",
  "NOBITS", 
  "RADIATE",
  "UNUSED14",
  "UNUSED15",
  "UNUSED28",
  "UNUSED29",
  "UNUSED30",
  "UNUSED31",
  "UNUSED32",
  "UNUSED33",
  "UNUSED34",
  "UNUSED35",
  "UNUSED36",
  "UNUSED37",
  "UNUSED38",
  "UNUSED39",
  "UNUSED40",
  "UNUSED41",
  "UNUSED42",
  "UNUSED43",
  "UNUSED44",
  "UNUSED45",
  "UNUSED46",
  "UNUSED47",
  "UNUSED48",
  "UNUSED49",
  "UNUSED50",
  "UNUSED55",
  "UNUSED56",
  "UNUESD57",
  "UNUSED58",
  "UNUSED59",
  "UNUSED60",
  "UNUESD61",
  "UNUSED62",
  "UNUSED63",
  "UNUSED64",
  "\n"
};


const char *class_bits[] = {
  "ANTI_PRIEST",
  "ANTI_SHAMAN",
  "ANTI_CLERIC",
  "ANTI_RANGER",
  "ANTI_DRUID",
  "ANTI_DISCIPLE",
  "ANTI_CRUSADER",
  "PRIEST",
  "SHAMAN",
  "CLERIC",
  "RANGER",
  "DRUID",
  "DISCIPLE",
  "CRUSADER",
  "PRIEST_CLASS",
  "ANTI_FIGHTER",
  "ANTI_WARRIOR",
  "ANTI_KNIGHT",
  "ANTI_BARBARIAN",
  "ANTI_MONK",
  "ANTI_PALADIN",
  "ANTI_SKNIGHT",
  "FIGHTER",
  "WARRIOR",
  "KNIGHT",
  "BARBARIAN",
  "MONK",
  "PALADIN",
  "SKNIGHT",
  "FIGHTER_CLASS",
  "ANTI_ROGUE",
  "ANTI_THIEF",
  "ANTI_BARD",
  "ANTI_ASSASSIN",
  "ANTI_BOUNTYHUNTER",
  "ANTI_JESTER",
  "ANTI_BLADE",
  "ROGUE",
  "THIEF",
  "BARD",
  "ASSASSIN",
  "BOUNTYHUNTER",
  "JESTER",
  "BLADE",
  "ROGUE_CLASS",
  "ANTI_MAGE",
  "ANTI_BATTLEMAGE",
  "ANTI_ENCHANTER",
  "ANTI_CHAOSMAGE",
  "ANTISORCEROR",
  "ANTI_NECROMANCER",
  "ANTI_ALCHEMIST",
  "NOBITS",
  "MAGE",
  "BATTLEMAGE",
  "ENCHANTER",
  "CHAOSMAGE",
  "SORCEROR",
  "NECROMANCER",
  "ALCHEMIST",
  "MAGE_CLASS",
  "\n"
};

/* APPLY_x */
const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "ATTACKS",
  "RACE",
  "SIZE",  
  "HIT_GAIN",
  "MANA_GAIN",
  "MOVE_GAIN",
  "\n"
};

/* APPLY_x */
/* for use in OLC menus */
/* strings must match values in apply_types */
/* "NONE" must always be the first in the list */
const char *sorted_apply_types[] = {
  "NONE",
  "AGE",
  "ARMOR",
  "ATTACKS",
  "DAMROLL",
  "HITROLL",
  "MAXMANA",
  "MANA_GAIN",
  "MAXHIT",
  "HIT_GAIN",
  "MAXMOVE",
  "MOVE_GAIN",
  "RACE",
  "SAVING_BREATH",
  "SAVING_PARA",
  "SAVING_PETRI",
  "SAVING_ROD",
  "SAVING_SPELL",
  "STR",
  "INT",
  "DEX",
  "WIS",
  "CON",
  "CHA",
  "SIZE",  
  "GOLD",
  "EXP",
  "CLASS",
  "LEVEL",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "\n"
};


/* CONT_x */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};

/* trap types */
const char *traps[] =
{
  "Damage trap no save",
  "Damage trap no save - room",
  "Damage trap save for 1/2 dam",
  "Damage trap save for 1/2 dam - room",
  "Damage trap save for no dam",
  "Damage trap save for no dam - room",
  "Teleport trap",
  "Teleport trap - room",   
  "Sleep trap - room",    
  "Acid trap - room",  
  "Death trap",
  "Death trap - room",  
  "\n"
}; 


/* LIQ_x */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "tequila",
  "\n"
};


/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "tequila",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13},
  {10, 0, 0},
};


/* color of the various drinks */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "clear",
  "\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* str, int, wis, dex, con applies **************************************/


/* [ch] strength apply (all) */
cpp_extern const struct str_app_type str_app[] = {
  {-5, -4, 0, 0},	/* str = 0 */
  {-5, -4, 3, 1},	/* str = 1 */
  {-3, -2, 3, 2},
  {-3, -1, 10, 3},
  {-2, -1, 25, 4},
  {-2, -1, 55, 5},	/* str = 5 */
  {-1, 0, 80, 6},
  {-1, 0, 90, 7},
  {0, 0, 100, 8},
  {0, 0, 100, 9},
  {0, 0, 115, 10},	/* str = 10 */
  {0, 0, 115, 11},
  {0, 0, 140, 12},
  {0, 0, 140, 13},
  {0, 0, 170, 14},
  {0, 0, 170, 15},	/* str = 15 */
  {0, 1, 195, 16},
  {1, 1, 220, 18},
  {1, 2, 255, 20},	/* str = 18 */
  {3, 7, 640, 40},
  {3, 8, 700, 40},	/* str = 20 */
  {4, 9, 810, 40},
  {4, 10, 970, 40},
  {5, 11, 1130, 40},
  {6, 12, 1440, 40},
  {7, 14, 1750, 40},	/* str = 25 */
  {1, 3, 280, 22},	/* str = 18/0 - 18-50 */
  {2, 3, 305, 24},	/* str = 18/51 - 18-75 */
  {2, 4, 330, 26},	/* str = 18/76 - 18-90 */
  {2, 5, 380, 28},	/* str = 18/91 - 18-99 */
  {3, 6, 480, 30}	/* str = 18/100 */
};



/* [dex] skill apply (thieves only) */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
  {-99, -99, -90, -99, -60},	/* dex = 0 */
  {-90, -90, -60, -90, -50},	/* dex = 1 */
  {-80, -80, -40, -80, -45},
  {-70, -70, -30, -70, -40},
  {-60, -60, -30, -60, -35},
  {-50, -50, -20, -50, -30},	/* dex = 5 */
  {-40, -40, -20, -40, -25},
  {-30, -30, -15, -30, -20},
  {-20, -20, -15, -20, -15},
  {-15, -10, -10, -20, -10},
  {-10, -5, -10, -15, -5},	/* dex = 10 */
  {-5, 0, -5, -10, 0},
  {0, 0, 0, -5, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},		/* dex = 15 */
  {0, 5, 0, 0, 0},
  {5, 10, 0, 5, 5},
  {10, 15, 5, 10, 10},		/* dex = 18 */
  {15, 20, 10, 15, 15},
  {15, 20, 10, 15, 15},		/* dex = 20 */
  {20, 25, 10, 15, 20},
  {20, 25, 15, 20, 20},
  {25, 25, 15, 20, 20},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}		/* dex = 25 */
};

cpp_extern const struct dex_skill_type race_app_skill[NUM_RACES] =
{
        { 0,  0,  0,  0,  0, }, /* HUMAN     */
        { 5, -5,  0,  5, 10, }, /* ELF       */
        { 0,  5, 10,  5,  5, }, /* GNOME     */
        { 0, 10, 15,  0,  0, }, /* DWARF     */
};

/* [dex] apply (all) */
cpp_extern const struct dex_app_type dex_app[] = {
  {-7, -7, 6},		/* dex = 0 */
  {-6, -6, 5},		/* dex = 1 */
  {-4, -4, 5},
  {-3, -3, 4},
  {-2, -2, 3},
  {-1, -1, 2},		/* dex = 5 */
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},		/* dex = 10 */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 1},		/* dex = 15 */
  {1, 1, 2},
  {2, 2, 3},
  {2, 2, 4},		/* dex = 18 */
  {3, 3, 4},
  {3, 3, 4},		/* dex = 20 */
  {4, 4, 5},
  {4, 4, 5},
  {4, 4, 5},
  {5, 5, 6},
  {5, 5, 6}		/* dex = 25 */
};



/* [con] apply (all) */
cpp_extern const struct con_app_type con_app[] = {
  {-4, 20},		/* con = 0 */
  {-3, 25},		/* con = 1 */
  {-2, 30},
  {-2, 35},
  {-1, 40},
  {-1, 45},		/* con = 5 */
  {-1, 50},
  {0, 55},
  {0, 60},
  {0, 65},
  {0, 70},		/* con = 10 */
  {0, 75},
  {0, 80},
  {0, 85},
  {0, 88},
  {1, 90},		/* con = 15 */
  {2, 95},
  {2, 97},
  {3, 99},		/* con = 18 */
  {3, 99},
  {4, 99},		/* con = 20 */
  {5, 99},
  {5, 99},
  {5, 99},
  {6, 99},
  {6, 99}		/* con = 25 */
};



/* [int] apply (all) */
cpp_extern const struct int_app_type int_app[] = {
  {3},		/* int = 0 */
  {5},		/* int = 1 */
  {7},
  {8},
  {9},
  {10},		/* int = 5 */
  {11},
  {12},
  {13},
  {15},
  {17},		/* int = 10 */
  {19},
  {22},
  {25},
  {30},
  {35},		/* int = 15 */
  {40},
  {45},
  {50},		/* int = 18 */
  {53},
  {55},		/* int = 20 */
  {56},
  {57},
  {58},
  {59},
  {60}		/* int = 25 */
};


/* [wis] apply (all) */
cpp_extern const struct wis_app_type wis_app[] = {
  {0},	/* wis = 0 */
  {0},  /* wis = 1 */
  {0},
  {0},
  {0},
  {0},  /* wis = 5 */
  {0},
  {0},
  {0},
  {0},
  {0},  /* wis = 10 */
  {0},
  {2},
  {2},
  {3},
  {3},  /* wis = 15 */
  {3},
  {4},
  {5},	/* wis = 18 */
  {6},
  {6},  /* wis = 20 */
  {6},
  {6},
  {7},
  {7},
  {7}  /* wis = 25 */
};


int rev_dir[] =
{
  2,
  3,
  0,
  1,
  5,
  4
};


int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  3,	/* Swimming   */
  1,	/* Unswimmable*/
  1,	/* Flying     */
  4     /* Underwater */
};

/* Not used in sprinttype(). */
const char *weekdays[] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};


/* Not used in sprinttype(). */
const char *month_name[] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};

/* mob trigger types */
const char *trig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "\n"
};


/* obj trigger types */
const char *otrig_types[] = {
  "Global",
  "Random",
  "Command",
  "UNUSED",
  "UNUSED",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED",
  "Remove",
  "UNUSED",
  "Load",
  "UNUSED",
  "UNUSED",
  "Leave",
  "UNUSED",
  "\n"
};


/* wld trigger types */
const char *wtrig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "Cast",
  "Leave",
  "Door",
  "\n"
};

#if defined(CONFIG_OASIS_MPROG)
/*
 * Definitions necessary for MobProg support in OasisOLC
 */
const char *mobprog_types[] = {
  "INFILE",
  "ACT",
  "SPEECH",
  "RAND",
  "FIGHT",
  "DEATH",
  "HITPRCNT",
  "ENTRY",
  "GREET",
  "ALL_GREET",
  "GIVE",
  "BRIBE",
  "\n"
};
#endif


/* --- End of constants arrays. --- */

/*
 * Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared.
 */
size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
	sorted_room_bits_count = sizeof(sorted_room_bits) / sizeof(sorted_room_bits[0]) - 1,
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	apply_types_count = sizeof(apply_types) / sizeof(apply_types[0]) - 1,
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;

