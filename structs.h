/* 
************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x030100 /* Major/Minor/Patchlevel - MMmmPP */

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ	1	/* TRUE/FALSE aren't defined yet. */


/* preamble *************************************************************/

/*
 * As of bpl20, it should be safe to use unsigned data types for the
 * various virtual and real number data types.  There really isn't a
 * reason to use signed anymore so use the unsigned types and get
 * 65,535 objects instead of 32,768.
 *
 * NOTE: This will likely be unconditionally unsigned later.
 */
#define CIRCLE_UNSIGNED_INDEX	0	/* 0 = signed, 1 = unsigned */

#if CIRCLE_UNSIGNED_INDEX
# define IDXTYPE	int
# define NOWHERE	((IDXTYPE)~0)
# define NOTHING	((IDXTYPE)~0)
# define NOBODY		((IDXTYPE)~0)
#else
# define IDXTYPE	sh_int
# define NOWHERE	(-1)	/* nil reference for rooms	*/
# define NOTHING	(-1)	/* nil reference for objects	*/
# define NOBODY		(-1)	/* nil reference for mobiles	*/
#endif

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)

//dan clan system
/* Clan ranks */
#define CLAN_UNDEFINED     -1
#define CLAN_NONE           0

#define NUM_CLAN_RANKS      6 /* 0,1,2... leader is top number */
#define CLAN_LEADER         (NUM_CLAN_RANKS-1)
#define CLAN_ADVISOR        (CLAN_LEADER-1)
#define MAX_CLAN_APPLICANTS 20
#define NUM_CLAN_GUARDS     2

#define NUM_STARTROOMS      3



/* This is for the Class Branching stuff - Frenzy */
struct char_data *guildmaster1;

/* room-related defines *************************************************/

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5


/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK	        (1 << 0)   /* Dark			*/
#define ROOM_DEATH	      	(1 << 1)   /* Death trap		*/
#define ROOM_NOMOB	      	(1 << 2)   /* MOBs not allowed		*/
#define ROOM_INDOORS	    	(1 << 3)   /* Indoors			*/
#define ROOM_PEACEFUL	    	(1 << 4)   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF         (1 << 5)   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK	    	(1 << 6)   /* Track won't go through	*/
#define ROOM_NOMAGIC	    	(1 << 7)   /* Magic not allowed		*/
#define ROOM_TUNNEL	        (1 << 8)   /* room for only 1 pers	*/
#define ROOM_PRIVATE	    	(1 << 9)   /* Can't teleport in		*/
#define ROOM_GODROOM	    	(1 << 10)  /* LVL_DEITY+ only allowed	*/
#define ROOM_HOUSE	        (1 << 11)  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH        (1 << 12)  /* (R) House needs saving	*/
#define ROOM_ATRIUM	        (1 << 13)  /* (R) The door to a house	*/
#define ROOM_OLC	        (1 << 14)  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK	    	(1 << 15)  /* (R) breath-first srch mrk	*/
#define ROOM_IMPROOM            (1 << 16)  /* Room is only for the imps */
#define ROOM_DUMP               (1 << 17)  /* Room that DOES NOT reward for objs dropped */
#define ROOM_REWARDDUMP         (1 << 18)  /* Dump room that rewards for objs dropped */
#define ROOM_DUMPONTICK         (1 << 19)  /* Dump room that dumps on tick */
#define ROOM_LIT                (1 << 20) 
#define ROOM_PAIN               (1 << 21)  /* Pain Room  */
#define ROOM_NOMAGIC_MSG        (1 << 22)  /* custom no magic message   */
#define ROOM_NOSUMMON           (1 << 23)  /* summoning not allowed     */
#define ROOM_NOSCRY             (1 << 24)  /* no scrying into the room  */
#define ROOM_NOPORTAL           (1 << 25)  /* no portaling into room    */
#define ROOM_GOOD               (1 << 26)  /* pain room for evil pcs    */
#define ROOM_EVIL               (1 << 27)  /* pain room for good pcs    */
#define ROOM_FIRE               (1 << 28)  /* fire-based pain room      */
#define ROOM_ICE                (1 << 29)  /* ice-based pain room       */
#define ROOM_WATER              (1 << 30)  /* room is under water       */
#define ROOM_AIR                (1 << 31)  /* room requires flight      */
#define ROOM_ASTRAL             (1 << 32)  /* on/connected to astral plane */
#define ROOM_ETHEREAL           (1 << 33)  /* on/connected to ethereal plane */


/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		    (1 << 0)   /* Exit is a door	*/
#define EX_CLOSED		    (1 << 1)   /* The door is closed	*/
#define EX_LOCKED		    (1 << 2)   /* The door is locked	*/
#define EX_PICKPROOF		    (1 << 3)   /* Lock can't be picked	*/
#define EX_HIDDEN		    (1 << 4)   /* Exit is hidden	*/


/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_FLYING	     8		   /* Wheee!			*/
#define SECT_UNDERWATER	     9		   /* Underwater		*/


/* char and mob-related defines *****************************************/

/* Resistance, Immunity, and vulnerability types these should never be changed
*  new types can be added at the end  Anubis */
#define ATTACK_PHYSICAL 0
#define ATTACK_PIERCE 1
#define ATTACK_BLUDGEON 2
#define ATTACK_SLASH  3
#define ATTACK_WHIP 4
#define ATTACK_MAGIC  5
#define ATTACK_FIRE 6
#define ATTACK_COLD 7
#define ATTACK_ELECTRIC 8
#define ATTACK_LIGHT 9
#define ATTACK_POISON 10
#define MAX_ATTACK_TYPES  11  /* total number of res, imm, and vuln...*/

/* Room/mob/obj/pc sizes */
#define SIZE_SPECIAL    0
#define SIZE_TINY       1
#define SIZE_SMALL      2
#define SIZE_NORMAL     3
#define SIZE_LARGE      4
#define SIZE_GIANT      5
#define SIZE_BEHEMOTH   6

/* Abilities */
#define ABILITY_STR   0
#define ABILITY_DEX   1
#define ABILITY_INT   2
#define ABILITY_WIS   3
#define ABILITY_CON   4
#define ABILITY_CHA   5
#define ABILITY_ADD   6
#define NUM_ABILITIES 7

#define BASE_HITS               15
#define BASE_MOVE               82
#define BASE_MANA               100
#define BASE_MANA_LAYHANDS      10
#define BASE_MANA_WHIRLWIND     100
#define MAX_AP                  100
#define MIN_AP                 -1000
#define MAX_AGGRESSION          100
#define MIN_AGGRESSION          0
#define MAX_RESIST              100
#define MAX_PHYSICAL_RESIST     75
#define MIN_RESIST             -MAX_RESIST
#define MAX_STAT_ATTRIBUTE      25
#define MIN_STAT_ATTRIBUTE      0
#define MAX_ALIGNMENT           1000
#define MIN_ALIGNMENT          -MAX_ALIGNMENT
#define MAX_NUM_ATTACKS         20
#define MIN_NUM_ATTACKS         2
#define MAX_DAMAGE_PER_HIT      500
#define MAX_SAVING_THROW        0
#define NUM_OF_SAVE_THROWS      5
#define MIN_HIT_POINTS         -10
#define MAX_HIT                 200000
#define MAX_MANA                10000
#define MAX_MOVE                1000
#define MIN_HIT_GAIN           -10
#define MIN_MANA_GAIN          -10
#define MIN_MOVE_GAIN          -10
#define MAX_HIT_GAIN            1000
#define MAX_MANA_GAIN           1000
#define MAX_MOVE_GAIN           1000
#define MIN_GOLD_GAIN          -10000
#define MAX_GOLD_GAIN           100000
#define MAX_GOLD                2000000000
#define MIN_EXP_GAIN           -1000000
#define MAX_EXP_GAIN            100000
#define DISHONOR_THRESHOLD      100
#define PIXIE_MAX_WIELD_WEIGHT  15
#define HASTE_MULTIPLIER        2
#define ALIGN_FLAG_RATE         30
#define MOB_HP_RANDOMIZATION    0.1
#define MOB_GP_RANDOMIZATION    0.1
#define DAWN_HOUR               6
#define DUSK_HOUR               21
#define LIGHT_ABSORBED_BY_CLOAK_OF_DARKNESS 100
#define LIGHT_ABSORBED_BY_CLOAK_OF_THE_NIGHT 25
#define MIN_ROOM_BRIGHTNESS_FOR_CLOAK  -15
#define MAX_ROOM_BRIGHTNESS_FOR_CLOAK   15
#define MAX_ROOM_LIGHT          100
#define MIN_ROOM_LIGHT         -100
#define DEFAULT_NORM_ROOM_LIGHT  0
/* if room light falls below the normal room light, it becomes dark */
#define DEFAULT_DARK_ROOM_LIGHT -5
/* normal "dark rooms" have light intensity of -5 */
/* NOTE: the use of the ROOM_DARK flag has changed */
#define HEAL_ROOM_RATE           1
#define PAIN_ROOM_RATE           1
#define MAX_BAG_ROWS             5


/* PC classes */
#define CLASS_UNDEFINED	  (-1)
#define CLASS_MAGIC_USER  0
#define CLASS_CLERIC      1
#define CLASS_THIEF       2
#define CLASS_WARRIOR     3
#define CLASS_SKNIGHT     4
#define CLASS_PALADIN     5
#define CLASS_ASSASSIN    6
#define CLASS_CHAOSMAGE   7
#define CLASS_SHAMAN      8
#define CLASS_DRUID       9
#define CLASS_RANGER      10
#define CLASS_PRIEST      11
#define CLASS_DISCIPLE    12
#define CLASS_CRUSADER    13
#define CLASS_FIGHTER     14
#define CLASS_BARBARIAN   15
#define CLASS_MONK        16
#define CLASS_KNIGHT      17
#define CLASS_ROGUE       18
#define CLASS_BARD        19
#define CLASS_JESTER      20
#define CLASS_BLADE       21
#define CLASS_BOUNTYHUNTER 22
#define CLASS_BATTLEMAGE  23
#define CLASS_SORCEROR    24
#define CLASS_ENCHANTER   25
#define CLASS_NECROMANCER 26
#define CLASS_ALCHEMIST   27

#define NUM_CLASSES	  28  /* This must be the number of classes!! */

/* NPC classes */
#define MOB_CLASS_NONE       0
#define MOB_CLASS_MAGIC_USER 1
#define MOB_CLASS_CLERIC     2
#define MOB_CLASS_THIEF      3
#define MOB_CLASS_WARRIOR    4
#define MOB_CLASS_SKNIGHT    5
#define MOB_CLASS_PALADIN    6
#define MOB_CLASS_ASSASSIN   7
#define MOB_CLASS_CHAOSMAGE  8
#define MOB_CLASS_SHAMAN      9
#define MOB_CLASS_DRUID       10
#define MOB_CLASS_RANGER      11
#define MOB_CLASS_PRIEST      12
#define MOB_CLASS_DISCIPLE    13
#define MOB_CLASS_CRUSADER    14
#define MOB_CLASS_FIGHTER     15
#define MOB_CLASS_BARBARIAN   16
#define MOB_CLASS_MONK        17
#define MOB_CLASS_KNIGHT      18
#define MOB_CLASS_ROGUE       19
#define MOB_CLASS_BARD        20
#define MOB_CLASS_JESTER      21
#define MOB_CLASS_BLADE       22
#define MOB_CLASS_BOUNTYHUNTER 23
#define MOB_CLASS_BATTLEMAGE  24
#define MOB_CLASS_SORCEROR    25
#define MOB_CLASS_ENCHANTER   26
#define MOB_CLASS_NECROMANCER 27
#define MOB_CLASS_ALCHEMIST   28

#define NUM_MOB_CLASSES	  29  /* This must be the number of mob classes!! */

#define RACE_UNDEFINED   -1
#define RACE_HUMAN        0
#define RACE_ELF          1
#define RACE_GNOME        2
#define RACE_DWARF        3
#define RACE_HALFLING	  4
#define RACE_MINOTAUR	  5
#define RACE_PIXIE	  6
#define RACE_ULDRA	  7
#define RACE_TRITON	  8
#define RACE_OGRE	  9
#define RACE_VAMPIRE	  10
#define RACE_SHINTARI	  11
#define RACE_KARADAL	  12
#define RACE_VISRAEL	  13


#define NUM_RACES         15

#define MOB_RACE_NONE     -1
#define MOB_RACE_HUMAN        0
#define MOB_RACE_ELF          1
#define MOB_RACE_GNOME        2
#define MOB_RACE_DWARF        3
#define MOB_RACE_HALFLING	  4
#define MOB_RACE_MINOTAUR	  5
#define MOB_RACE_PIXIE	  6
#define MOB_RACE_ULDRA	  7
#define MOB_RACE_TRITON	  8
#define MOB_RACE_ORGE	  9
#define MOB_RACE_VAMPIRE	  10
#define MOB_RACE_SHINTARI	  11
#define MOB_RACE_KARADAL	  12
#define MOB_RACE_VISRAEL	  13
#define MOB_RACE_DRAGON		14
#define MOB_RACE_DEVA           15
#define MOB_RACE_DEVIL          16
#define MOB_RACE_DEMON          17
#define MOB_RACE_ELEMENTAL      18
#define MOB_RACE_UNDEAD         19
#define MOB_RACE_PLANT          20
#define MOB_RACE_ANIMAL         21
#define MOB_RACE_REPTILE        22
#define MOB_RACE_AQUATIC        23
#define MOB_RACE_AERIAL         24
#define MOB_RACE_INSECT         25
#define MOB_RACE_ARACHNID       26
#define MOB_RACE_GOLEM          27
#define MOB_RACE_BEAST          28
#define MOB_RACE_HUMANOID       29
#define MOB_RACE_DRACONIAN      30
#define MOB_RACE_GIANT          31
#define MOB_RACE_DINOSAUR       32

#define NUM_MOB_RACES     34

/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2


/* Positions */
#define POS_DEAD          0     /* dead                 */
#define POS_MORTALLYW     1     /* mortally wounded     */
#define POS_INCAP         2     /* incapacitated        */
#define POS_PARALYZED     3     /* paralyzed            */
#define POS_STUNNED       4     /* stunned              */
#define POS_BURIED        5     /* buried (for RIP spell) */
#define POS_FAKEDEAD      6     /* feigning death       */
#define POS_SLEEPING      7     /* sleeping             */
#define POS_MEDITATING    8     /* meditating           */
#define POS_RESTING       9     /* resting              */
#define POS_SITTING      10     /* sitting              */
#define POS_FIGHTING     11     /* fighting             */
#define POS_STANDING     12     /* standing             */
#define POS_FLYING       13     /* flying               */


/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER	(1 << 0)   /* Player is a player-killer		*/
#define PLR_THIEF	(1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN	(1 << 2)   /* Player is frozen			*/
#define PLR_DONTSET     (1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING	(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING	(1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH	(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK	(1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT	(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE	(1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED	(1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_NOWIZLIST	(1 << 12)  /* Player shouldn't be on wizlist	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO	(1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_NOTDEADYET	(1 << 16)  /* (R) Player being extracted.	*/
#define PLR_JAIL        (1 << 17) /* Player is JAILED */
#define PLR_NOEXPGAIN   (1 << 18) /* Player CANNOT gain xp */
#define PLR_PROGRESS    (1 << 19) /* Player needs to progress to the next class */

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         (1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE	       (1 << 4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE   (1 << 5)  /* Mob auto-attacks everybody nearby	*/
#define MOB_STAY_ZONE    (1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (1 << 7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_EVIL	   (1 << 8)  /* Auto-attack any evil PC's		*/
#define MOB_AGGR_GOOD	   (1 << 9)  /* Auto-attack any good PC's		*/
#define MOB_AGGR_NEUTRAL (1 << 10) /* Auto-attack any neutral PC's	*/
#define MOB_MEMORY	     (1 << 11) /* remember attackers if attacked	*/
#define MOB_HELPER	     (1 << 12) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM	     (1 << 13) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	   (1 << 14) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	     (1 << 15) /* Mob can't be slept		*/
#define MOB_NOBASH	     (1 << 16) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND	     (1 << 17) /* Mob can't be blinded		*/
#define MOB_NOTDEADYET   (1 << 18) /* (R) Mob being extracted.		*/


/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF       (1 << 0)  /* Room descs won't normally be shown	*/
#define PRF_COMPACT     (1 << 1)  /* No extra CRLF pair before prompts	*/
#define PRF_DEAF	(1 << 2)  /* Can't hear shouts			*/
#define PRF_NOTELL	(1 << 3)  /* Can't receive tells		*/
#define PRF_DISPHP	(1 << 4)  /* Display hit points in prompt	*/
#define PRF_DISPMANA	(1 << 5)  /* Display mana points in prompt	*/
#define PRF_DISPMOVE	(1 << 6)  /* Display move points in prompt	*/
#define PRF_AUTOEXIT	(1 << 7)  /* Display exits in a room		*/
#define PRF_NOHASSLE	(1 << 8)  /* Aggr mobs won't attack		*/
#define PRF_QUEST	(1 << 9)  /* On quest				*/
#define PRF_SUMMONABLE	(1 << 10) /* Can be summoned			*/
#define PRF_NOREPEAT	(1 << 11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(1 << 12) /* Can see in dark			*/
#define PRF_COLOR_1	(1 << 13) /* Color (low bit)			*/
#define PRF_COLOR_2	(1 << 14) /* Color (high bit)			*/
#define PRF_NOWIZ	(1 << 15) /* Can't hear wizline			*/
#define PRF_LOG1	(1 << 16) /* On-line System Log (low bit)	*/
#define PRF_LOG2	(1 << 17) /* On-line System Log (high bit)	*/
#define PRF_NOAUCT	(1 << 18) /* Can't hear auction channel		*/
#define PRF_NOGOSS	(1 << 19) /* Can't hear gossip channel		*/
#define PRF_NOGRATZ	(1 << 20) /* Can't hear grats channel		*/
#define PRF_ROOMFLAGS	(1 << 21) /* Can see room flags (ROOM_x)	*/
#define PRF_DISPAUTO	(1 << 22) /* Show prompt HP, MP, MV when < 30%.	*/
#define PRF_CLS         (1 << 23) /* Clear screen in OasisOLC 		*/
#define PRF_BUILDWALK   (1 << 23) /* Build new rooms when walking       */
#define PRF_AFK         (1 << 24) /* Player is AFK */
#define PRF_DISPTARGET  (1 << 25) /* Player is displaying target */
#define PRF_CLANTALK    (1 << 26) /* Can't hear clan channel            */
#define PRF_ALLCTELL    (1 << 27) /* Can't hear all clan channels(imm)  */
#define PRF_DISPEXP     (1 << 28) /* Exp in the display prompt          */
#define PRF_AUTOSPLIT   (1 << 29) /* Autosplit gold amonst the group    */

/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */

#define AFF_BLIND             (1ULL << 0)   /* (R) Char is blind	*/
#define AFF_INVISIBLE         (1ULL << 1)   /* Char is invisible	*/
#define AFF_DETECT_ALIGN      (1ULL << 2)   /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      (1ULL << 3)   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (1ULL << 4)   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (1ULL << 5)   /* Char can sense hidden life*/
#define AFF_WATERWALK	      (1ULL << 6)   /* Char can walk on water	*/
#define AFF_SANCTUARY         (1ULL << 7)   /* Char protected by sanct.	*/
#define AFF_GROUP             (1ULL << 8)   /* (R) Char is grouped	*/
#define AFF_CURSE             (1ULL << 9)   /* Char is cursed		*/
#define AFF_INFRAVISION       (1ULL << 10)  /* Char can see in dark	*/
#define AFF_POISON            (1ULL << 11)  /* (R) Char is poisoned	*/
#define AFF_PROTECT_EVIL      (1ULL << 12)  /* Char protected from evil  */
#define AFF_PROTECT_GOOD      (1ULL << 13)  /* Char protected from good  */
#define AFF_SLEEP             (1ULL << 14)  /* (R) Char magically asleep*/
#define AFF_NOTRACK	      (1ULL << 15)  /* Char can't be tracked	*/
#define AFF_SUSTAIN	      (1ULL << 16)  /* char is sustained	*/
#define AFF_AIRWALK	      (1ULL << 17)  /* Char has airwalk        	*/
#define AFF_SNEAK             (1ULL << 18)  /* Char can move quietly	*/
#define AFF_HIDE              (1ULL << 19)  /* Char is hidden		*/
#define AFF_HASTE	      (1ULL << 20)  /* Char is hasted     	*/
#define AFF_CHARM             (1ULL << 21)  /* Char is charmed		*/
#define AFF_FURY              (1ULL << 22)  /* Char is in fury		*/
#define AFF_DIST_SIGHT        (1ULL << 23)  /* Char has doubled scan range */
#define AFF_IMM_POISON        (1ULL << 24)  /* Char is IMM poison */
#define AFF_IMM_LIGHT         (1ULL << 25)  /* Char is IMM light */
#define AFF_ANGELIC           (1ULL << 26)  /* Char is an angelic being  */
#define AFF_BAT_SONAR         (1ULL << 27)  /* Char can see cloaked persons and while blinded */
#define AFF_BENEFICENCE       (1ULL << 28)  /* Char is affected by beneficence */
#define AFF_CLOAK_OF_DARKNESS (1ULL << 29)  /* Char is affected by cloak of darkness */
#define AFF_CLOAK_OF_SHADOWS  (1ULL << 30)  /* Char is hidden through cloak of shadows */
#define AFF_CLOAK_OF_THE_NIGHT (1ULL << 31) /* Char is affected by cloak of the night */
#define AFF_DEATHS_DOOR       (1ULL << 32)  /* Char is affected by deaths door */
#define AFF_DETECT_TRAPS      (1ULL << 33)  /* char sees all traps */
#define AFF_DREAMSIGHT        (1ULL << 34)  /* Char can see while sleeping */
#define AFF_ETHEREAL          (1ULL << 35)  /* Char is ethereal          */
#define AFF_FEATHER_FALL      (1ULL << 36)  /* Char will float down to the ground when they jump */
#define AFF_FLEET_FEET        (1ULL << 37)  /* Char movement cost is halved */
#define AFF_FLYING            (1ULL << 38)  /* Char is flying         	*/
#define AFF_FREE_ACTION       (1ULL << 39)  /* Char is immune to paralyzation */
#define AFF_HEALING_DREAM     (1ULL << 40)  /* Char heals faster while sleeping */
#define AFF_FORT              (1ULL << 41)  /* Char is forted           */
#define AFF_DERVISH_SPIN      (1ULL << 42)  /* Char is in defensive spin */
#define AFF_DECREPIFY         (1ULL << 43)  /* Char has been cursed by decrepify */
#define AFF_SECOND_SIGHT      (1ULL << 44)  /* Reserved bit thats auto-set when no flags are selected */
#define AFF_IMPROVED_INVIS    (1ULL << 45)  /* Char is affected by improved invisibility */
#define AFF_INVIS_TO_ENEMIES  (1ULL << 46)  /* Char is invisible to enemies */
#define AFF_MAGICONLY         (1ULL << 47)  /* Char only hurt by magic   */
#define AFF_PARALYZE          (1ULL << 48)  /* Char is paralized		*/
#define AFF_PARALYZING_TOUCH  (1ULL << 49)  /* Char may paralyze the enemy if he/she touches it */
#define AFF_PARRY             (1ULL << 50)  /* Char will try to parry blows during combat */
#define AFF_REFLECT_DAMAGE    (1ULL << 51)  /* Char protected by reflect damage  */
#define AFF_RESERVED          (1ULL << 52)  /* Char is affected by regeneration */
#define AFF_REGENERATION      (1ULL << 53)  /* Char can see in the dark and twice as far */
#define AFF_SHADOW_ARMOR      (1ULL << 54)  /* Char is protected by shadow armor */
#define AFF_SILENCE           (1ULL << 55)  /* Char is unable to speak or cast spells */
#define AFF_SLEEPWALK         (1ULL << 56)  /* Char can walk around while sleeping */
#define AFF_UNDEAD            (1ULL << 57)  /* Char has become undead */
#define AFF_WATERBREATH       (1ULL << 58)  /* Char can breath non O2    */
#define AFF_DETECT_EVIL       (1ULL << 59)  /* char can detect evil */
#define AFF_DETECT_GOOD       (1ULL << 60)     /* char can detect good */
#define AFF_DETECT_NEUTRAL    (1ULL << 61)     /* char can detect neutral */

/* Affect2 bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF2_UNUSED             (1ULL << 0)	   /* 		*/


/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0	/* Playing - Nominal state		*/
#define CON_CLOSE	 1	/* User disconnect, remove character.	*/
#define CON_GET_NAME	 2	/* By what name ..?			*/
#define CON_NAME_CNFRM	 3	/* Did I get that right, x?		*/
#define CON_PASSWORD	 4	/* Password:				*/
#define CON_NEWPASSWD	 5	/* Give me a password for x		*/
#define CON_CNFPASSWD	 6	/* Please retype password:		*/
#define CON_QSEX	 7	/* Sex?					*/
#define CON_QCLASS	 8	/* Class?				*/
#define CON_RMOTD	 9	/* PRESS RETURN after MOTD		*/
#define CON_MENU	 10	/* Your choice: (main menu)		*/
#define CON_EXDESC	 11	/* Enter a new description:		*/
#define CON_CHPWD_GETOLD 12	/* Changing passwd: get old		*/
#define CON_CHPWD_GETNEW 13	/* Changing passwd: get new		*/
#define CON_CHPWD_VRFY   14	/* Verify new password			*/
#define CON_DELCNF1	 15	/* Delete confirmation 1		*/
#define CON_DELCNF2	 16	/* Delete confirmation 2		*/
#define CON_DISCONNECT	 17	/* In-game link loss (leave character)	*/
#define CON_OEDIT	 18	/* OLC mode - object editor		*/
#define CON_REDIT	 19	/* OLC mode - room editor		*/
#define CON_ZEDIT	 20	/* OLC mode - zone info editor		*/
#define CON_MEDIT	 21	/* OLC mode - mobile editor		*/
#define CON_SEDIT	 22	/* OLC mode - shop editor		*/
#define CON_TEDIT	 23	/* OLC mode - text editor		*/
#define CON_TRIGEDIT     24	/* OLC mode - trigger edit              */
#define CON_QRACE        25     /* Race? 				*/
#define CON_QSTATS       26 /* for choosing your starting stats   Anubis */
#define CON_BRANCH1      27 /* First Class Branch - Frenzy */
#define CON_BRANCH2      28 /* Second Class Branch - Frenzy */
#define CON_CLANEDIT     29     /* OLC mode - clan editor               */

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_HOLD      17
#define NUM_WEARS      18	/* This must be the # of eq positions!! */


/* object-related defines ********************************************/


/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON 6		/* Unimplemented		*/
#define ITEM_MISSILE    7		/* Unimplemented		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_WORN      11		/* Unimplemented		*/
#define ITEM_OTHER     12		/* Misc object			*/
#define ITEM_TRASH     13		/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP      14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_BOAT      22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/


/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  /* Item can be takes		*/
#define ITEM_WEAR_FINGER	(1 << 1)  /* Can be worn on finger	*/
#define ITEM_WEAR_NECK		(1 << 2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(1 << 3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(1 << 4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(1 << 5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(1 << 6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(1 << 7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(1 << 8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(1 << 9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(1 << 10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(1 << 11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(1 << 12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(1 << 13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(1 << 14) /* Can be held		*/


/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW           (1ULL << 0)	 /* Item is glowing		*/
#define ITEM_HUM            (1ULL << 1)	 /* Item is humming		*/
#define ITEM_NORENT         (1ULL << 2)	 /* Item cannot be rented	*/
#define ITEM_NODONATE       (1ULL << 3)	 /* Item cannot be donated	*/
#define ITEM_NOINVIS	    (1ULL << 4)	 /* Item cannot be made invis	*/
#define ITEM_INVISIBLE      (1ULL << 5)	 /* Item is invisible		*/
#define ITEM_MAGIC          (1ULL << 6)	 /* Item is magical		*/
#define ITEM_NODROP         (1ULL << 7)	 /* Item is cursed: can't drop	*/
#define ITEM_BLESS          (1ULL << 8)	 /* Item is blessed		*/
#define ITEM_ANTI_GOOD      (1ULL << 9)	 /* Not usable by good people	*/
#define ITEM_ANTI_EVIL      (1ULL << 10) /* Not usable by evil people	*/
#define ITEM_ANTI_NEUTRAL   (1ULL << 11) /* Not usable by neutral people	*/
#define ITEM_NO_EQ_MSG      (1ULL << 12) /* Item will not display normal eq msg */
#define ITEM_RADIATE        (1UL  << 13) /* Item radiates a bright light */
#define ITEM_NOSELL	    (1ULL << 16) /* Shopkeepers won't touch it	*/
#define ITEM_ANTI_HUMAN     (1ULL << 17) /* Not usable by humans         */
#define ITEM_ANTI_DWARF     (1ULL << 18) /* Not usable by dwarves        */
#define ITEM_ANTI_ELF       (1ULL << 19) /* Not usable by elves          */
#define ITEM_ANTI_GNOME     (1ULL << 20) /* Not usable by gnomes         */
#define ITEM_NO_PURGE       (1ULL << 21) /* Item cannot be purged */
#define ITEM_NO_DISARM      (1ULL << 22) /* Item cannot be disarmed */
#define ITEM_NO_CLAIM       (1ULL << 23) /* Can't be claimed Anubis */
#define ITEM_GOOD_FLAGGED   (1ULL << 24) /* item is good flagged */
#define ITEM_EVIL_FLAGGED   (1ULL << 25) /* item is evil flagged */
#define ITEM_ANGELIC        (1ULL << 26) /* item is ang flagged, +100 align */
#define ITEM_DEMONIC        (1ULL << 27) /* item is dem flagged -100 align */

#define ITEM_GREATER_HIDDEN (1ULL << 51) /* Item will not lose hidden if picked up */
#define ITEM_RESERVED       (1ULL << 52) /* Reserved bit thats auto-set when no flags are selected */
#define ITEM_LESSER_HIDDEN  (1ULL << 53) /* Item will lose hidden if picked up */


/* =-=-=-=-=-=-=-=-=-= Class restrictions =-=-=-=-=-=-=-=-=-= */


/* Priest Types */
#define ITEM_ANTI_PRIEST             (1ULL << 0)
#define ITEM_ANTI_SHAMAN             (1ULL << 1)
#define ITEM_ANTI_CLERIC             (1ULL << 2)	
#define ITEM_ANTI_RANGER             (1ULL << 3)
#define ITEM_ANTI_DRUID              (1ULL << 4)
#define ITEM_ANTI_DISCIPLE           (1ULL << 5)
#define ITEM_ANTI_CRUSADER           (1ULL << 6)
#define ITEM_PRIEST_ONLY             (1ULL << 7)
#define ITEM_SHAMAN_ONLY             (1ULL << 8)
#define ITEM_CLERIC_ONLY             (1ULL << 9)
#define ITEM_RANGER_ONLY             (1ULL << 10)
#define ITEM_DRUID_ONLY              (1ULL << 11)
#define ITEM_DISCIPLE_ONLY           (1ULL << 12)
#define ITEM_CRUSADER_ONLY           (1ULL << 13)
#define ITEM_PRIESTS_ONLY            (1ULL << 14)

/* Fighter Types */
#define ITEM_ANTI_FIGHTER            (1ULL << 15)
#define ITEM_ANTI_WARRIOR            (1ULL << 16)
#define ITEM_ANTI_KNIGHT             (1ULL << 17)
#define ITEM_ANTI_BARBARIAN          (1ULL << 18)
#define ITEM_ANTI_MONK               (1ULL << 19)
#define ITEM_ANTI_PALADIN            (1ULL << 20)
#define ITEM_ANTI_SKNIGHT            (1ULL << 21)
#define ITEM_FIGHTER_ONLY            (1ULL << 22)
#define ITEM_WARRIOR_ONLY            (1ULL << 23)
#define ITEM_KNIGHT_ONLY             (1ULL << 24)
#define ITEM_BARBARIAN_ONLY          (1ULL << 25)
#define ITEM_MONK_ONLY               (1ULL << 26)
#define ITEM_PALADIN_ONLY            (1ULL << 27)
#define ITEM_SKNIGHT_ONLY            (1ULL << 28)
#define ITEM_FIGHTERS_ONLY           (1ULL << 29)

/* Rogue Types */
#define ITEM_ANTI_ROGUE              (1ULL << 30)
#define ITEM_ANTI_THIEF	             (1ULL << 31)
#define ITEM_ANTI_BARD               (1ULL << 32)
#define ITEM_ANTI_ASSASSIN           (1ULL << 33)
#define ITEM_ANTI_BOUNTYHUNTER       (1ULL << 34)
#define ITEM_ANTI_JESTER             (1ULL << 35)
#define ITEM_ANTI_BLADE              (1ULL << 36)
#define ITEM_ROGUE_ONLY              (1ULL << 37)
#define ITEM_THIEF_ONLY              (1ULL << 38)
#define ITEM_BARD_ONLY               (1ULL << 39)
#define ITEM_ASSASSIN_ONLY           (1ULL << 40)
#define ITEM_BOUNTYHUNTER_ONLY       (1ULL << 41)
#define ITEM_JESTER_ONLY             (1ULL << 42)
#define ITEM_BLADE_ONLY              (1ULL << 43)
#define ITEM_ROGUES_ONLY             (1ULL << 44)

/* Mage Types */
#define ITEM_ANTI_MAGE               (1ULL << 45)
#define ITEM_ANTI_BATTLEMAGE         (1ULL << 46)
#define ITEM_ANTI_ENCHANTER          (1ULL << 47)
#define ITEM_ANTI_CHAOSMAGE          (1ULL << 48)
#define ITEM_ANTI_SORCEROR           (1ULL << 49)
#define ITEM_ANTI_NECROMANCER        (1ULL << 50)
#define ITEM_ANTI_ALCHEMIST          (1ULL << 51)
#define ITEM_CLASSRESERVED           (1ULL << 52)
#define ITEM_MAGE_ONLY               (1ULL << 53)
#define ITEM_BATTLEMAGE_ONLY         (1ULL << 54)
#define ITEM_ENCHANTER_ONLY          (1ULL << 55)
#define ITEM_CHAOSMAGE_ONLY          (1ULL << 56)
#define ITEM_SORCEROR_ONLY           (1ULL << 57)
#define ITEM_NECROMANCER_ONLY        (1ULL << 58)
#define ITEM_ALCHEMIST_ONLY          (1ULL << 59)
#define ITEM_MAGES_ONLY              (1ULL << 60)


/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to intelligence	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA	            	6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/
#define APPLY_SAVING_PARA      20	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       21	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     22	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    23	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     24	/* Apply to save throw: spells	*/
#define APPLY_ATTACKS	       25       /* Apply to attacks             */
#define APPLY_RACE             26       /* Apply to race                */
#define APPLY_SIZE             27       /* Apply to size                */
#define APPLY_HIT_GAIN         28       /* Apply to hit gain            */
#define APPLY_MANA_GAIN        29       /* Apply to mana gain           */
#define APPLY_MOVE_GAIN        30       /* Apply to move gain           */
#define APPLY_PRACTICE         31       /* Apply to practices           */
#define APPLY_SONC_RESIST      32      /* Apply to sonic resistance    */
#define APPLY_AP               33       /* Apply to Armor Points        */
/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15


/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2


/* Sun state for weather_data */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3


/* Sky conditions for weather_data */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3


/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5


/* other #defined constants **********************************************/

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_SAINT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_SAINT - 1.
 */
#define LVL_IMPL	47
#define LVL_GOD	        46
#define LVL_DEITY	45
#define LVL_COUNT	44
#define LVL_LIEGE	43
#define LVL_BUILDER     42
#define LVL_SAINT	41
#define LVL_NEWBIE       0
#define LVL_APPRENTICE   1
#define LVL_JOURNEYMAN  10
#define LVL_MASTER      25

/* Level of the 'freeze' command */
#define LVL_FREEZE	LVL_DEITY




#define NUM_OF_DIRS	6	/* number of directions in a room (nsewud) */
#define MAGIC_NUMBER	(0x06)	/* Arbitrary number that won't be in a string */

/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 */
#define OPT_USEC	100000		/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_MOBILE    (10 RL_SEC)
#define PULSE_VIOLENCE  ( 1 RL_SEC)
#define PULSE_AUTOSAVE	(60 RL_SEC)
#define PULSE_IDLEPWD	(15 RL_SEC)
#define PULSE_SANITY	(30 RL_SEC)
#define PULSE_USAGE	(30 * 60 RL_SEC)	/* 30 mins */
#define PULSE_TIMESAVE	(30 * 60 RL_SEC) /* should be >= SECS_PER_MUD_HOUR */
#define PULSE_PAIN_ROOM (1 RL_SEC)

#define MUD_HOURS_PER_DAY      24
#define MUD_DAYS_PER_MONTH     35
#define MUD_MONTHS_PER_YEAR    17
#define MUD_MOON_PHASES        25
#define BANK_INTEREST_RATE     0.1  //this is the annual rate

/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (12 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       200          /* Max length of prompt        */
#define GARBAGE_SPACE		32          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE		1024        /* Static output buffer size   */
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE	   (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define HISTORY_SIZE		5	/* Keep last 5 commands. */
#define MAX_STRING_LENGTH	32768  /*increased by mak */
#define MAX_INPUT_LENGTH	256	/* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	512	/* Max size of *raw* input */
#define MAX_MESSAGES		400
#define MAX_NAME_LENGTH		20  /* Used in char_file_u *DO*NOT*CHANGE* */
/* ** MAX_PWD_LENGTH changed from 10 to 30 for ascii test - Sam ** */
#define MAX_PWD_LENGTH		30  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH	80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH		80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH		240 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TONGUE		3   /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS		2000 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT		64  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT		6 /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define MAX_NOTE_LENGTH		1000	/* arbitrary */

/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */
#if defined(HAVE_UNSAFE_CRYPT) && MAX_PWD_LENGTH == 10
#error You need to increase MAX_PWD_LENGTH to at least 20.
#error See the comment near these errors for more explanation.
#endif

/**********************************************************************
* Structures                                                          *
**********************************************************************/

typedef signed char		sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;
#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char			bool;
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)	/* Hm, sysdep.h? */
typedef signed char			byte;
#endif

/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum;
typedef IDXTYPE obj_vnum;
typedef IDXTYPE mob_vnum;
typedef IDXTYPE zone_vnum;
typedef IDXTYPE shop_vnum;

/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum;
typedef IDXTYPE obj_rnum;
typedef IDXTYPE mob_rnum;
typedef IDXTYPE zone_rnum;
typedef IDXTYPE shop_rnum;


/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 * 'unsigned long long' will give you at least 64 bits if you have GCC.
 *
 * Since we don't want to break the pfiles, you'll have to search throughout
 * the code for "bitvector_t" and change them yourself if you'd like this
 * extra flexibility.
 */
typedef unsigned long long	bitvector_t;

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char	*keyword;                 /* Keyword in look/examine          */
   char	*description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};

 

//dan clan system 

struct clan_type {
  int  number;            /* clan's UNIQUE ID Number      */
  char *name;             /* No color name of clan (string)  */
  char *applicants[MAX_CLAN_APPLICANTS];/* Pointer to strings            */
  char *leadersname;      /* Leader's (Player's) Name     */
  char *rank_name[NUM_CLAN_RANKS]; /* Rank names                      */
  char *member_look_str;  /* The clan's colored name      */
  room_vnum clan_entr_room;    /* VNUM of clan Entrance Room */
  room_vnum clan_recall;           /* VNUM of clan recall room        */
  mob_rnum guard[NUM_CLAN_GUARDS]; /* RNUM of clan guard              */
  int  direction;         /* Direction of clan entrance   */
  int  pkill;             /* TRUE if pkill desired        */
  long  clan_gold;         /* clan gold                    */
  obj_vnum clan_eq[NUM_CLAN_RANKS];/* clan equipment                  */
  struct clan_type *next;
};

/* ====================================================================== */

/* object-related structures ******************************************/
#define NUM_OBJ_VAL_POSITIONS 4

/* object flags; used in obj_data */
struct obj_flag_data {
   int	value[NUM_OBJ_VAL_POSITIONS];	/* Values of the item (see list)    */
   byte type_flag;	/* Type of item			    */
   int level;		/* Minimum level of object.		*/
   int max_level;       /* Maximum level of object          */
   int /*bitvector_t*/	wear_flags;	/* Where you can wear it	    */
   bitvector_t	extra_flags;	/* If it hums, glows, etc.	    */
   bitvector_t  class_flags;    /* class restrictions --seymour */
   int	weight;		/* Weigt what else                  */
   int	cost;		/* Value when sold (gp.)            */
   int	cost_per_day;	/* Cost to keep pr. real day        */
   int	timer;		/* Timer for object                 */
   bitvector_t	bitvector;	/* To set chars bits                */
   bitvector_t	bitvector2;	/* To set chars bits                */
   
};


/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
   byte location;      /* Which ability to change (APPLY_XXX) */
   sbyte modifier;     /* How much it changes by              */
};


/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_vnum item_number;	/* Where in data-base			*/
   room_rnum in_room;		/* In what room -1 when conta/carr	*/

   struct obj_flag_data obj_flags;/* Object information               */
   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */
   struct obj_flag_data class_flags;

   char	*name;                    /* Title of object :get etc.        */
   char	*description;		  /* When in room                     */
   char	*short_description;       /* when worn/carry/in cont.         */
   char	*action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *worn_by;	  /* Worn by?			      */
   sh_int worn_on;		  /* Worn where?		      */
   sh_int size;                   /* Size of the object */
   long corpse_id;      /* Anubis for claim     */
   long killer;         /*     claim            */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */

   long id;                       /* used by DG triggers              */
   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;    /* script info for the object       */

   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */
   sh_int resist[MAX_ATTACK_TYPES];           /* Anubis  */
   sh_int immune[MAX_ATTACK_TYPES];
   sh_int vulnerable[MAX_ATTACK_TYPES];
};
/* ======================================================================= */


/* ====================== File Element for Objects ======================= */
/*                 BEWARE: Changing it will ruin rent files		   */
struct obj_file_elem {
   obj_vnum item_number;

#if USE_AUTOEQ
   sh_int location;
#endif
   int	value[NUM_OBJ_VAL_POSITIONS];
   bitvector_t	extra_flags;
   int	weight;
   int	timer;
   bitvector_t	bitvector;
   bitvector_t	bitvector2;
   struct obj_affected_type affected[MAX_OBJ_AFFECT];
};


/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct rent_info {
   int	time;
   int	rentcode;
   int	net_cost_per_diem;
   int	gold;
   int	account;
   int	nitems;
   int	spare0;
   int	spare1;
   int	spare2;
   int	spare3;
   int	spare4;
   int	spare5;
   int	spare6;
   int	spare7;
};
/* ======================================================================= */


/* room-related structures ************************************************/


struct room_direction_data {
   char	*general_description;       /* When look DIR.			*/

   char	*keyword;		/* for open/close			*/

   sh_int /*bitvector_t*/ exit_info;	/* Exit info			*/
   obj_vnum key;		/* Key's number (-1 for no key)		*/
   room_rnum to_room;		/* Where direction leads (NOWHERE)	*/
};


/* ================== Memory Structure for room ======================= */
struct room_data {
   room_vnum number;		/* Rooms number	(vnum)		      */
   zone_rnum zone;              /* Room zone (for resetting)          */
   int	sector_type;            /* sector type (move/hide)            */
   int  size;			/* Size of the room		      */
   char	*name;                  /* Rooms name 'You are ...'           */
   char	*description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   bitvector_t room_flags;	/* DEATH,DARK ... etc */

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;  /* script info for the object         */
   int nat_light;                  /* Amount of natural light in room on a -100 to 100 scale */
   int mod_light;                  /* Amount of added light to a room */
   char *pain_message;             /* message for the pain room       */
   int pain_damage;                /* how much damage per rate?       */
   int pain_rate;                  /* whats the pain room rate?       */
   int pain_check;                 /* saves the heartbeats passed     */
   byte light;                     /* Number of lightsources in room  */
   SPECIAL(*func);                 
   char *nomagic_message_caster;   /* message used in no magic rooms to caster */
   char *nomagic_message_room;     /* message used in no magic rooms to room   */
   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */
   int max_level;               /* max level of the room              */
   int min_level;               /* min level of the room              */
   char *max_level_message;      /* max level message for people trying to enter the room */
   char *min_level_message;      /* min level message for people trying to enter the room */

};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   long	id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month, moon_phase;
   sh_int year;
};



/* These data contain information about a players time data */
struct time_data {
   time_t birth;    /* This represents the characters age                */
   time_t logon;    /* Time of the last logon (used to calculate played) */
   int	played;     /* This is the total accumulated time played in secs */
};


/* The pclean_criteria_data is set up in config.c and used in db.c to
   determine the conditions which will cause a player character to be
   deleted from disk if the automagic pwipe system is enabled (see config.c).
*/
struct pclean_criteria_data {
  int level;		/* max level for this time limit	*/
  int days;		/* time limit in days			*/
}; 


/* general player-related info, usually PC's and NPC's */
struct char_player_data {
   char	passwd[MAX_PWD_LENGTH+1]; /* character's password      */
   char	*name;	       /* PC / NPC s name (kill ...  )         */
   char	*short_descr;  /* for NPC 'actions'                    */
   char	*long_descr;   /* for 'look'			       */
   char	*description;  /* Extra descriptions                   */
   char	*title;        /* PC / NPC's title                     */
   byte size;           /* PC / NPC's size                      */
   byte sex;           /* PC / NPC's sex                       */
   byte chclass;       /* PC / NPC's class		       */
   byte level;         /* PC / NPC's level                     */
   int difficulty;     /* NPCs difficulty level                */
   sh_int hometown;    /* PC s Hometown (zone)                 */
   struct time_data time;  /* PC's AGE in days                 */
   ubyte weight;       /* PC / NPC's weight                    */
   ubyte height;       /* PC / NPC's height                    */
   byte race;          /* PC / NPC's race                      */
   int clan;           /* PC / NPC's clan                      */
   int rank;           /* PC / NPC's clan rank                 */
   };


/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data {
   sbyte str;
   sbyte str_add;      /* 000 - 100 if strength 18             */
   sbyte intel;
   sbyte wis;
   sbyte dex;
   sbyte con;
   sbyte cha;
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {
   sh_int mana;
   sh_int max_mana;     /* Max mana for PC/NPC			   */
   int hit;
   sh_int max_hit;      /* Max hit for PC/NPC                      */
   sh_int move;
   sh_int max_move;     /* Max move for PC/NPC                     */

   sh_int armor;        /* Internal -100..100, external -10..10 AC */
   int	gold;           /* Money carried                           */
   int	bank_gold;	/* Gold the char has in a bank account	   */
   int	exp;            /* The experience of the player            */
   
   int hitroll;       /* Any bonus or penalty to the hit roll    */
   int damroll;       /* Any bonus or penalty to the damage roll */
};


/* 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
   int	alignment;		/* +-1000 for alignments                */
   long	idnum;			/* player's idnum; -1 for mobiles	*/
   long /*bitvector_t*/ act;	/* act flag for NPC's; player flag for PC's */

   bitvector_t	affected_by;
   bitvector_t	affected_by2;
				/* Bitvector for spells/skills affected by */
   sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)		*/ 
   
};


/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
   struct char_data *fighting;	/* Opponent				*/
   struct char_data *hunting;	/* Char hunted by this char		*/

   byte position;		/* Standing, fighting, sleeping, etc.	*/

   int	carry_weight;		/* Carried weight			*/
   byte carry_items;		/* Number of items carried		*/
   int	timer;			/* Timer for update			*/
   sbyte hitgain;               /* Hit gain                             */
   sbyte managain;              /* Mana gain                            */
   sbyte movegain;              /* Move gain                            */
   sh_int resist[MAX_ATTACK_TYPES];           /* Anubis  */
   sh_int immune[MAX_ATTACK_TYPES];
   sh_int vulnerable[MAX_ATTACK_TYPES];
   sh_int extra_attack;  /* extra attacks?? seymour */
   int damback; /*fort damage back*/ 

   struct char_special_data_saved saved; /* constants saved in plrfile	*/
};


/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
   byte skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
   byte PADDING0;		/* used to be spells_to_learn		*/
   bool talks[MAX_TONGUE];	/* PC s Tongues 0 for NPC		*/
   int	wimp_level;		/* Below this # of hit points, flee!	*/
   byte freeze_level;		/* Level of god who froze char, if any	*/
   sh_int invis_level;		/* level of invisibility		*/
   room_vnum load_room;		/* Which room to place char in		*/
   long /*bitvector_t*/	pref;	/* preference flags for PC's.		*/
   ubyte bad_pws;		/* number of bad password attemps	*/
   sbyte conditions[3];         /* Drunk, full, thirsty			*/

   /* spares below for future expansion.  You can change the names from
      'sparen' to something meaningful, but don't change the order.  */

   ubyte spare0;
   ubyte spare1;
   ubyte spare2;
   ubyte spare3;
   ubyte spare4;
   ubyte spare5;
   int spells_to_learn;		/* How many can you learn yet this level*/
   int olc_zone;
   int clan;         //dan clan system
 //  int clan_id; REMOVE
 //  int clan_rank; REMOVE       /*                  */
   int spare10;
   int spare11;
   int spare12;
   int spare13;
   int spare14;
   int spare15;
   int spare16;
   long	spare17;
   long	spare18;
   long	spare19;
   long	spare20;
   long	spare21;
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data {
   struct player_special_data_saved saved;

   char	*poofin;		/* Description on arrival of a god.     */
   char	*poofout;		/* Description upon a god's exit.       */
   struct alias_data *aliases;	/* Character's aliases			*/
   long last_tell;		/* idnum of last tell from		*/
   long last_kill;		/* idnum of pc killed to claim from  Anubis 3/5/03 */
   void *last_olc_targ;		/* olc control				*/
   int last_olc_mode;		/* olc control				*/
   char *host;			/* player host				*/
};


/* Specials used by NPCs, not PCs */
struct mob_special_data {
   memory_rec *memory;	    /* List of attackers to remember	       */
   byte	attack_type;        /* The Attack Type Bitvector for NPC's     */
   byte default_pos;        /* Default position for NPC                */
   byte damnodice;          /* The number of damage dice's	       */
   byte damsizedice;        /* The size of the damage dice's           */
   int num_attacks;
};


/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
   sh_int type;          /* The type of spell that caused this      */
   sh_int duration;      /* For how long its effects will last      */
   sbyte modifier;       /* This is added to apropriate ability     */
   byte location;        /* Tells which ability to change(APPLY_XXX)*/
   bitvector_t	bitvector; /* Tells which bits to set (AFF_XXX) */

   struct affected_type *next;
};


/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};


/* ================== Structure for player/non-player ===================== */
struct char_data {
   int pfilepos;			 /* playerfile pos		  */
   mob_rnum nr;                          /* Mob's rnum			  */
   room_rnum in_room;                    /* Location (real room number)	  */
   room_rnum was_in_room;		 /* location for linkdead people  */
   int wait;				 /* wait for how many loops	  */

   struct char_player_data player;       /* Normal data                   */
   struct char_ability_data real_abils;	 /* Abilities without modifiers   */
   struct char_ability_data aff_abils;	 /* Abils with spells/stones/etc  */
   struct char_point_data points;        /* Points                        */
   struct char_special_data char_specials;	/* PC/NPC specials	  */
   struct player_special_data *player_specials; /* PC specials		  */
   struct mob_special_data mob_specials;	/* NPC specials		  */

   struct affected_type *affected;       /* affected by what spells       */
   struct affected_type *affected2;      /* affected by what second set of spells  Anubis */
   struct obj_data *equipment[NUM_WEARS];/* Equipment array               */

   struct obj_data *carrying;            /* Head of list                  */
   struct descriptor_data *desc;         /* NULL for mobiles              */

   long id;                            /* used by DG triggers             */
   struct trig_proto_list *proto_script; /* list of default triggers      */
   struct script_data *script;         /* script info for the object      */
   struct script_memory *memory;       /* for mob memory triggers         */

   struct char_data *next_in_room;     /* For room->people - list         */
   struct char_data *next;             /* For either monster or ppl-list  */
   struct char_data *next_fighting;    /* For fighting list               */

   struct follow_type *followers;        /* List of chars followers       */
   struct char_data *master;             /* Who is char following?        */
};
/* ====================================================================== */


/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct char_file_u {
   /* char_player_data */
   char	name[MAX_NAME_LENGTH+1];
   char	description[EXDSCR_LENGTH];
   char	title[MAX_TITLE_LENGTH+1];
   byte sex;
   byte chclass;
   byte level;
   sh_int hometown;
   time_t birth;   /* Time of birth of character     */
   int	played;    /* Number of secs played in total */
   ubyte weight;
   ubyte height;
   byte race;

   char	pwd[MAX_PWD_LENGTH+1];    /* character's password */

   struct char_special_data_saved char_specials_saved;
   struct player_special_data_saved player_specials_saved;
   struct char_ability_data abilities;
   struct char_point_data points;
   struct affected_type affected[MAX_AFFECT];
   struct affected_type affected2[MAX_AFFECT];

   time_t last_logon;		/* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];	/* host of last logon */
};
/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};


struct descriptor_data {
   socket_t	descriptor;	/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   byte idle_tics;		/* tics idle at password prompt		*/
   int	connected;		/* mode of 'connectedness'		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   char *showstr_head;		/* for keeping track of an internal str	*/
   char **showstr_vector;	/* for paging through texts		*/
   int  showstr_count;		/* number of pages to page through	*/
   int  showstr_page;		/* which page are we currently showing?	*/
   char	**str;			/* for the modify-str system		*/
   char *backstr;		/* backup string for modify-str system	*/
   size_t max_str;	        /* maximum size of string in modify-str	*/
   long	mail_to;		/* name for mail system			*/
   int	has_prompt;		/* is the user at a prompt?             */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   char **history;		/* History of commands, for ! mostly.	*/
   int	history_pos;		/* Circular array position.		*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   struct oasis_olc_data *olc;   /* OLC info                            */
};


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};


struct dex_skill_type {
   sh_int p_pocket;
   sh_int p_locks;
   sh_int traps;
   sh_int sneak;
   sh_int hide;
};


struct dex_app_type {
   sh_int reaction;
   sh_int miss_att;
   sh_int defensive;
};


struct str_app_type {
   sh_int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
   sh_int todam;    /* Damage Bonus/Penalty                */
   sh_int carry_w;  /* Maximum weight that can be carrried */
   sh_int wield_w;  /* Maximum weight that can be wielded  */
};


struct wis_app_type {
   byte bonus;       /* how many practices player gains per lev */
};


struct int_app_type {
   byte learn;       /* how many % a player learns a spell/skill */
};


struct con_app_type {
   sh_int hitp;
   sh_int shock;
};


struct weather_data {
   int	pressure;	/* How is the pressure ( Mb ) */
   int	change;	/* How fast and what way does it change. */
   int	sky;	/* How is the sky. */
   int	sunlight;	/* And how much sun. */
};


/*
 * Element in monster and object index-tables.
 *
 * NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum)
 */
struct index_data {
   mob_vnum	vnum;	/* virtual number of this mob/obj		*/
   int		number;	/* number of existing units of this mob/obj	*/
   SPECIAL(*func);

   char *farg;         /* string argument for special function     */
   struct trig_data *proto;     /* for triggers... the trigger     */
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
  int vnum;                             /* vnum of the trigger   */
  struct trig_proto_list *next;         /* next trigger          */
};

struct guild_info_type {
  int pc_class;
  room_vnum guild_room;
  int direction;
};