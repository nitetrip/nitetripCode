/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define DEFAULT_STAFF_LVL	12
#define DEFAULT_WAND_LVL	12

#define CAST_UNDEFINED	(-1)
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4

#define MAG_DAMAGE	(1 << 0)
#define MAG_AFFECTS	(1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	(1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	(1 << 5)
#define MAG_MASSES	(1 << 6)
#define MAG_AREAS	(1 << 7)
#define MAG_SUMMONS	(1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	(1 << 10)
#define MAG_MATERIALS   (1 << 11)
#define MAG_AFFECTSV	(1 << 12)

#define TYPE_UNDEFINED               (-1)
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define SPELL_ARMOR                   1 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_TELEPORT                2 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLESS                   3 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLINDNESS               4 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BURNING_HANDS           5 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CALL_LIGHTNING          6 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHARM                   7 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHILL_TOUCH             8 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CLONE                   9 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_COLOR_SPRAY            10 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CONTROL_WEATHER        11 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_FOOD            12 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_WATER           13 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_BLIND             14 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_CRITIC            15 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_LIGHT             16 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURSE                  17 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_ALIGN           18 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_INVIS           19 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_MAGIC           20 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_POISON          21 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_EVIL            22 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EARTHQUAKE             23 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENCHANT_WEAPON         24 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENERGY_DRAIN           25 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FIREBALL               26 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HARM                   27 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL                   28 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INVISIBLE              29 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LIGHTNING_BOLT         30 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LOCATE_OBJECT          31 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MAGIC_MISSILE          32 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_POISON                 33 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PROT_FROM_EVIL         34 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_CURSE           35 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SANCTUARY              36 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SHOCKING_GRASP         37 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SLEEP                  38 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STRENGTH               39 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUMMON                 40 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VENTRILOQUATE          41 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WORD_OF_RECALL         42 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_POISON          43 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SENSE_LIFE             44 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ANIMATE_DEAD	     45 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_GOOD	     46 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_ARMOR	     47 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_HEAL	     48 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_RECALL	     49 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INFRAVISION	     50 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WATERWALK	      	     51 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUSTAIN		     52 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SPOOK                  53 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_AIRWALK                54 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FORT                   55 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HASTE                  56 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLOODLUST 	     57 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SOVEREIGN_HEAL         58 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL_SERIOUS           59 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ICE_STORM		     60 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHI_FIST               61 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BOULDER                62 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_METEOR                 63 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_METEOR_SHOWER          64 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_THUNDER_SWARM          65 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ROAR                   66 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SONIC_BLAST            67 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ICE_LANCE              68 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HORNET_SWARM           69 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PILLAR_OF_FLAME        70 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FINGER_OF_DEATH        71 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DERVISH_SPIN           72 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL_CRITICAL          73 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DIVINE_HEAL            74 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MASS_HEAL              75 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEALING_WIND           76 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VIGORIZE_LIGHT         77 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VIGORIZE_SERIOUS       78 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VIGORIZE_CRITICAL      79 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VIGORIZE_GROUP         80 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VITALIZE_MANA          81 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EXCOMMUNICATE          82 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DECREPIFY              83
#define SPELL_TELEVIEW_MINOR         84
#define SPELL_TELEVIEW_MAJOR         85
#define SPELL_CLAN_RECALL            86
#define SPELL_ACID_ARROW	     87
#define SPELL_BALL_LIGHTNING         88
#define SPELL_BOLT_OF_STEEL          89
#define SPELL_CAUSE_MINOR            90
#define SPELL_CAUSE_MAJOR            91
#define SPELL_CHAIN_LIGHTNING        92
#define SPELL_PRISMATIC_SPRAY        93
#define SPELL_CLENCHED_FIST          94
#define SPELL_ELEMENTAL_BURST        95
#define SPELL_ELEMENTAL_BLAST        96
#define SPELL_ELEMENTAL_HANDS        97
#define SPELL_ELEMENTAL_STRIKE       98
#define SPELL_FIREBOLT               99
#define SPELL_FLAILING_FISTS         100
#define SPELL_FLAMESTRIKE            101
#define SPELL_FLAMING_ARROW          102
#define SPELL_HAIL_OF_ARROWS         103
#define SPELL_HORNETS_DART           104
#define SPELL_SMITE_EVIL             105
#define SPELL_SMITE_GOOD             106
#define SPELL_SEARING_ORB            107
#define SPELL_HOLY_WORD              108
#define SPELL_UNHOLY_WORD            109
#define SPELL_WAIL_OF_THE_BANSHEE    110
#define SPELL_BAT_SONAR              111
#define SPELL_BENEFICENCE            112
#define SPELL_CLOAK_OF_DARKNESS      113
#define SPELL_CLOAK_OF_SHADOWS       114
#define SPELL_CLOAK_OF_THE_NIGHT     115
#define SPELL_DEATHS_DOOR            116
#define SPELL_DREAMSIGHT             117
#define SPELL_FEATHER_FALL           118
#define SPELL_FLEET_FEET             119
#define SPELL_FREE_ACTION            120
#define SPELL_HEALING_DREAM          121
#define SPELL_IMPROVED_INVISIBILITY  122
#define SPELL_INVISIBILITY_TO_ENEMIES  123
#define SPELL_PARALYZE		     124
#define SPELL_REFLECT_DAMAGE         125
#define SPELL_REGENERATION           126
#define SPELL_SECOND_SIGHT           127
#define SPELL_SHADOW_ARMOR           128
#define SPELL_SILENCE                129
#define SPELL_SLEEPWALK              130
#define SPELL_GRANT_BAT_SONAR        131
#define SPELL_BLOOD_REVEL            132
#define SPELL_RECALL_TO_SORIN        133
#define SPELL_GROUP_SORIN_RECALL     134
#define SPELL_ARCANE_LORE            135
#define SPELL_TALES_OF_ARCANE_LORE   136
#define SPELL_ARBOREAL_FORM          137
#define SPELL_FORESTATION            138
#define SPELL_ETHEREAL_PROJECTION    139
#define SPELL_ETHEREAL_SPHERE        140
#define SPELL_ASTRAL_PROJECTION      141
#define SPELL_ASTRAL_ASCENSION       142
#define SPELL_HANG                   143
#define SPELL_MASS_SUICIDE           144
#define SPELL_DETECT_EVIL            145
#define SPELL_DETECT_GOOD            146
#define SPELL_DETECT_NEUTRAL         147
#define SPELL_SUSTAIN_GROUP          148
#define SPELL_PHASE_DOOR	     149
#define SPELL_ACCURACY		     150
#define SPELL_AID		     151
#define SPELL_ANIMAL_FRIENDSHIP		152
#define SPELL_BARKSKIN			153
#define SPELL_BEFRIEND_DRYAD		154
#define SPELL_BIND_PORTAL_MAJOR		155
#define SPELL_BIND_PORTAL_MINOR		156
#define SPELL_CALM			157
#define SPELL_CANNIBALIZE		158
#define SPELL_CHAMPIONS_STRENGTH	159
#define SPELL_CHARM_BEAST		160
#define SPELL_CHARM_MONSTER		161
#define SPELL_CHARM_PERSON		162
#define SPELL_CONJURE_ELEMENTAL		163
#define SPELL_CONJURE_UNDEAD		164
#define SPELL_CONTROL_PLANT		165
#define SPELL_CONTROL_UNDEAD		166
#define SPELL_CURE_SERIOUS		167
#define SPELL_DIMENSION_SHIFT		168
#define SPELL_DIMENSION_WALK		169
#define SPELL_DIMENSION_DOOR		170
#define SPELL_DISPEL_MAGIC		171
#define SPELL_DISPEL_SILENCE		172
#define SPELL_DRAW_UPON_HOLY_MIGHT	173
#define SPELL_ELEMENTAL_AURA		174
#define SPELL_RECHARGE    		175
#define SPELL_EMBALM			176
#define SPELL_ENFEEBLEMENT		177
#define SPELL_ENLARGE			178
#define SPELL_FEIGN_DEATH		179
#define SPELL_FLAMEWALK			180
#define SPELL_FUMBLE			181
#define SPELL_GATE			182
#define SPELL_GHOUL_GAUNTLET		183
#define SPELL_GHOUL_TOUCH		184
#define SPELL_HEAL_LIGHT		185
#define SPELL_HEROES_FEAST		186
#define SPELL_HOLD_ANIMAL		187
#define SPELL_HOLD_BEAST		188
#define SPELL_HOLD_MONSTER		189
#define SPELL_HOLD_PERSON		190
#define SPELL_HOLD_PLANT		191
#define SPELL_HOLD_UNDEAD		192
#define SPELL_IMMUNITY_TO_COLD		193
#define SPELL_IMMUNITY_TO_ELEC		194
#define SPELL_INTIMIDATE		195
#define SPELL_LEGEND_LORE		196
#define SPELL_LIFE_LEECH		197
#define SPELL_LOCATE_SHADOW_PLANE	198
#define SPELL_PACIFY			199
#define SPELL_PASS_WITHOUT_TRACE	200
#define SPELL_PLANAR_TRAVEL		201
#define SPELL_PORTAL			202
#define SPELL_POWER_STRIKE		203
#define SPELL_PROTECTION_FROM_GOOD	204
#define SPELL_REMOVE_PARALYSIS		205
#define SPELL_RESISTANCE_TO_COLD	206
#define SPELL_RESISTANCE_TO_ELEC	207
#define SPELL_REST_IN_PEACE		208
#define SPELL_SCRY_GREATER		209
#define SPELL_SCRY_LESSER		210
#define SPELL_SHADOW_DOOR		211
#define SPELL_SHADOW_WALK		212
#define SPELL_SHIELD			213
#define SPELL_SHIELD_AGAINST_EVIL	214
#define SPELL_SHIELD_AGAINST_GOOD	215
#define SPELL_SHRINK			216
#define SPELL_SKELETAL_GUISE		217
#define SPELL_SOMNOLENT_GAZE		218
#define SPELL_STRENGTH_BURST		219
#define SPELL_STUN			220
#define SPELL_SUCCOR			221
#define SPELL_SUMMON_AVENGER		222
#define SPELL_SUMMON_BEAST		223
#define SPELL_SUMMON_LESSER		224
#define SPELL_SUMMON_GREATER		225
#define SPELL_SUNBURST			226
#define SPELL_SUNRAY			227
#define SPELL_SYNOSTODWEOMER		228
#define SPELL_TELEPORT_MAJOR		229
#define SPELL_TELEPORT_MINOR		230
#define SPELL_TOWER_OF_STRENGTH		231
#define SPELL_TRAIL_OF_WOODLANDS	232
#define SPELL_VAMPIRIC_GAZE		233
#define SPELL_VAMPIRIC_TOUCH		234
#define SPELL_VITALITY			235
#define SPELL_WINDWALK			236
#define SPELL_WITHER			237
#define SPELL_DEATH_STRIKE              238
#define SPELL_MAGICAL_VESTMANTS         239
#define SPELL_ELEMENTAL_SHIELD          240
#define SPELL_ASPHYXIATE		241
#define SPELL_PROTECTION_FROM_EVIL    	242
#define SPELL_KNOCK			243
#define SPELL_BLOOD_QUENCH		244
#define SPELL_ELEMENTAL_SHARD		245
#define SPELL_MOON_MOTE			246
/* Insert new spells here, up to MAX_SPELLS */

// NEXT SPELL SHOULD BE ASSIGNED TO 0 TO STOP PROF FROM GIVING THE FIRST SPELL !UNUSED!

#define SPELL_BREATH_FIRE            324
#define SPELL_BREATH_GAS             325
#define SPELL_BREATH_FROST           326
#define SPELL_BREATH_ACID            327
#define SPELL_BREATH_LIGHTNING       328


#define MAX_SPELLS		    1000

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB              1031 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH                  1032 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE                  1033 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK                  1034 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK             1035 /* Reserved Skill[] DO NOT CHANGE */
/* Undefined */
#define SKILL_RESCUE                1037 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK                 1038 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL                 1039 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_TRACK		    1040 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_DETECT_TRAPS          1041 /* Anubis */
#define SKILL_PROF_PIERCE           1042 /* skills for weapon profs        */
#define SKILL_PROF_BLUDGEON         1043
#define SKILL_PROF_SLASH            1044
#define SKILL_PROF_WHIP             1045 /*                                */
#define SKILL_FURY                  1046 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_DISARM                1047 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_DIST_SIGHT            1048 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_CRIT_HIT              1049 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_CIRCLE                1050
#define SKILL_SHIELD_SPEC           1051
#define SKILL_PARRY		    1052
#define SKILL_RAGE		    1053
#define SKILL_WHIRLWIND             1054
#define SKILL_ARMOR_SPEC        	1055
#define SKILL_DISABLE_TRAP		1056
#define SKILL_ENDURANCE			1057
#define SKILL_ENVENOM			1058
#define SKILL_ESTATES			1059
#define SKILL_IAIJUTSU			1060
#define SKILL_LAYHANDS			1061
#define SKILL_MEDITATE			1062
#define SKILL_RETREAT			1063
#define SKILL_SHIELDPUNCH		1064
#define SKILL_SHIELDRUSH		1065
#define SKILL_SEARCH			1066
#define SKILL_TURNING			1067
#define SKILL_LANG_COMMON		1068
#define SKILL_LANG_ELVEN		1069
#define SKILL_LANG_GNOME		1070
#define SKILL_LANG_DWARVEN		1071
#define SKILL_PROF_UNARMED		1072
#define SKILL_PROF_SPECIAL		1073


// #define SKILL_WP_PIERCE             1300 /* Weapon prof - form CWE March 7, 2012*/
//#define SKILL_WP_SLASH              1301
//#define SKILL_WP_BLUDGEON           1302
#define SKILL_WP_SPECIAL            1303
#define SKILL_WP_UNARMED            1304 /* Barehanded weapon group        */


/* New skills may be added here up to MAX_SKILLS (2000) */


/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */

#define SPELL_IDENTIFY               2001
#define SPELL_FIRE_BREATH            2002
#define SPELL_GAS_BREATH             2003
#define SPELL_FROST_BREATH           2004
#define SPELL_ACID_BREATH            2005
#define SPELL_LIGHTNING_BREATH       2006




#define SPELL_DG_AFFECT              2098  /* to make an affect induced by dg_affect
                                           * look correct on 'stat' we need to define
                                           * it with a 'spellname'.
                                           */
#define TOP_SPELL_DEFINE	     2299
/* NEW NPC/OBJECT SPELLS can be inserted here up to 2099 */


/* WEAPON ATTACK TYPES */

#define TYPE_HIT                     2300
#define TYPE_STING                   2301
#define TYPE_WHIP                    2302
#define TYPE_SLASH                   2303
#define TYPE_BITE                    2304
#define TYPE_BLUDGEON                2305
#define TYPE_CRUSH                   2306
#define TYPE_POUND                   2307
#define TYPE_CLAW                    2308
#define TYPE_MAUL                    2309
#define TYPE_THRASH                  2310
#define TYPE_PIERCE                  2311
#define TYPE_BLAST		     2312
#define TYPE_PUNCH		     2313
#define TYPE_STAB		     2314
#define TYPE_KICK                    2315
#define TYPE_POIS                    2316
#define TYPE_RANDOM		     2317

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING		     2399
#define TYPE_SUNDAM                  2400
#define TYPE_LASTATTACK              2401 /* This is internal only */



#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4


#define TAR_IGNORE      (1 << 0)
#define TAR_CHAR_ROOM   (1 << 1)
#define TAR_CHAR_WORLD  (1 << 2)
#define TAR_FIGHT_SELF  (1 << 3)
#define TAR_FIGHT_VICT  (1 << 4)
#define TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF   	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     (1 << 7)
#define TAR_OBJ_ROOM    (1 << 8)
#define TAR_OBJ_WORLD   (1 << 9)
#define TAR_OBJ_EQUIP	(1 << 10)
#define TAR_CHAR_ZONE   (1 << 11)
#define TAR_DIRECTION   (1 << 12)
#define TAR_ATTRIBUTE   (1 << 13) //
#define TAR_PORTAL_CODE      (1 << 14) //target = a valid portal code
#define TAR_ROOM_IN_ZONE     (1 << 15) //target = a room in the same zone as caster
#define TAR_WEATHER          (1 << 16) //target = better or worse weather



struct spell_info_type {
   byte min_position;	/* Position for caster	 */
   int mana_min;	/* Min amount of mana used by a spell (highest lev) */
   int mana_max;	/* Max amount of mana used by a spell (lowest lev) */
   int mana_change;	/* Change in mana used by spell from lev to lev */

   int min_level[NUM_CLASSES];
   int routines;
   byte violent;
   int targets;         /* See below for use with TAR_XXX  */
   const char *name;	/* Input size not limited. Originates from string constants. */
   const char *wear_off_msg;	/* Input size not limited. Originates from string constants. */
   int first_prereq[NUM_CLASSES];          /* prerequisite skill/spell one */
   int second_prereq[NUM_CLASSES];          /* prerequisite skill/spell two */
};

/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4


/* Attacktypes with grammar */

struct attack_hit_type {
   const char	*singular;
   const char	*plural;
};


#define ASPELL(spellname) void spellname(int level, struct char_data *ch, struct char_data *victim, struct obj_data *obj, int param1, int spellnum)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict, param1, spellnum);
ASPELL(spell_arboreal_form);
ASPELL(spell_arcane_lore);
ASPELL(spell_astral_projection);
ASPELL(spell_ethereal_form);
ASPELL(spell_ethereal_projection);
ASPELL(spell_hang);
ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_sorin_recall);
ASPELL(spell_teleport);
ASPELL(spell_teleview);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(spell_spook);
ASPELL(spell_clan_recall);
ASPELL(spell_phase_door);
ASPELL(spell_vitality);
ASPELL(spell_bind_portal);
ASPELL(spell_portal);
ASPELL(spell_teleportm);
ASPELL(spell_succor);
ASPELL(spell_stun);
ASPELL(spell_scry);
ASPELL(spell_rest_in_peace);
ASPELL(spell_knock);
ASPELL(spell_fumble);
ASPELL(spell_feign_death);
ASPELL(spell_control_weather);
ASPELL(spell_cannibalize);
ASPELL(spell_calm);
ASPELL(spell_blood_quench);
ASPELL(spell_recharge);

/* basic magic calling functions */

int find_skill_num(char *name);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int savetype);

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
  int param1, int spellnum, int savetype);

void mag_groups(int level, struct char_data *ch, int param1, int spellnum, int savetype);

void mag_masses(int level, struct char_data *ch, int spellnum, int savetype);

void mag_areas(int level, struct char_data *ch, int spellnum, int savetype);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
 int spellnum, int savetype);

void mag_points(int level, struct char_data *ch, struct char_data *victim,
 int param1, int spellnum, int savetype);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int type);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
  int spellnum, int type);

void mag_creations(int level, struct char_data *ch, int spellnum);

int	call_magic(struct char_data *caster, struct char_data *cvict,
  struct obj_data *ovict, int param1, int spellnum, int level, int casttype);

void	mag_objectmagic(struct char_data *ch, struct obj_data *obj,
			char *argument);

int	cast_spell(struct char_data *ch, struct char_data *tch,  struct obj_data *tobj, int param1, int spellnum);


/* other prototypes */
void spell_level(int first_prereq,int second_prereq, int spell, int chclass, int level);
void init_spell_levels(void);
const char *skill_name(int num);
