/************************************************************************
 * OasisOLC - General / oasis.h					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#define _OASISOLC	0x201   /* 2.0.1 */

/*
 * Used to determine what version of OasisOLC is installed.
 *
 * Ex: #if _OASISOLC >= OASIS_VERSION(2,0,0)
 */
#define OASIS_VERSION(x,y,z)	(((x) << 8 | (y) << 4 | (z))

#define OLC_AUTO_SAVE 1

/*
 * Set this to 1 to enable MobProg support.  MobProgs are available on
 * the CircleMUD FTP site in the "contrib/scripting/" directory.
 *
 * -- THIS WILL NOT WORK WITHOUT MobProgs INSTALLED. --
 * -- OasisOLC DOES NOT COME WITH THEM. -- Loud enough for you?
 *
 * It might work with DG Scripts (successor to MobProgs) but I haven't
 * tried, nor have I heard of anyone trying.
 */
#define CONFIG_OASIS_MPROG	0

/*
 * Submodes of CLANEDIT connectedness
 */
#define CLANEDIT_MAIN_MENU      0
#define CLANEDIT_CONFIRM_SAVE   1
#define CLANEDIT_NAME           2
#define CLANEDIT_LEADERSNAME    3
#define CLANEDIT_RANK6          4
#define CLANEDIT_RANK5          5
#define CLANEDIT_RANK4          6
#define CLANEDIT_RANK3          7
#define CLANEDIT_RANK2          8
#define CLANEDIT_RANK1          9
#define CLANEDIT_MBR_LOOK_STR   10
#define CLANEDIT_ENTR_ROOM      11
#define CLANEDIT_GUARD1         12
#define CLANEDIT_GUARD2         13
#define CLANEDIT_DIRECTION      14
#define CLANEDIT_PKILL          15
#define CLANEDIT_CLAN_GOLD      16
#define CLANEDIT_RECALL         17

/* -------------------------------------------------------------------------- */

/*
 * Macros, defines, structs and globals for the OLC suite.  You will need
 * to adjust these numbers if you ever add more.
 */

#define NUM_OBJ_SIZES           7

#define NUM_ROOM_FLAGS 		22
#define NUM_ROOM_SECTORS	10
#define NUM_ROOM_SIZES          7

#define NUM_MOB_FLAGS		23
#define NUM_MOB_CLASSES         29
#define NUM_MOB_RACES           34
#define NUM_MOB_SIZES           7
#define NUM_AFF_FLAGS		63
#define NUM_AFF2_FLAGS		1
#define NUM_ATTACK_TYPES	15

#define NUM_ITEM_TYPES		24
#define NUM_ITEM_FLAGS		41
#define NUM_ITEMCLASS_FLAGS     61
#define NUM_ITEM_WEARS 		15
#define NUM_APPLIES		31
#define NUM_LIQ_TYPES 		17
#define NUM_TRAP_TYPES    12
#define NUM_POSITIONS		15
#define NUM_SPELLS		148

#define NUM_GENDERS		3
#define NUM_SHOP_FLAGS 		2
#define NUM_TRADERS 		7

#if CONFIG_OASIS_MPROG
/*
 * Define this to how many MobProg scripts you have.
 */
#define NUM_PROGS		12
#endif


/*
 * Statedit Connectedness
 * --relistan 2/23/99
 */

#define STAT_GET_STR  0
#define STAT_GET_INT  1
#define STAT_GET_WIS  2
#define STAT_GET_DEX  3
#define STAT_GET_CON  4
#define STAT_GET_CHA  5
#define STAT_QUIT     6
#define STAT_PARSE_MENU 7

/* -------------------------------------------------------------------------- */

/*
 * Limit information.
 */
#define MAX_ROOM_NAME	75
#define MAX_MOB_NAME	50
#define MAX_OBJ_NAME	50
#define MAX_ROOM_DESC	2048
#define MAX_EXIT_DESC	512
#define MAX_EXTRA_DESC  1024
#define MAX_MOB_DESC    1024
#define MAX_OBJ_DESC	1024

/* -------------------------------------------------------------------------- */

extern int list_top;

/*
 * Utilities exported from olc.c.
 */
void cleanup_olc(struct descriptor_data *d, byte cleanup_type);
void get_char_colors(struct char_data *ch);

/*
 * OLC structures.
 */

struct oasis_olc_data {
  int mode;
  int zone_num;
  int number;
  int value;
  int total_mprogs;
  struct char_data *mob;
  struct room_data *room;
  struct obj_data *obj;
  struct zone_data *zone;
  struct shop_data *shop;
  struct extra_descr_data *desc;
 struct clan_type *clan; 
#if CONFIG_OASIS_MPROG
  struct mob_prog_data *mprog;
  struct mob_prog_data *mprogl;
#endif
  struct trig_data *trig;
  int script_mode;
  int trigger_position;
  int item_type;
  struct trig_proto_list *script;
  char *storage; /* for holding commands etc.. */
};

/*
 * Exported globals.
 */
extern const char *nrm, *grn, *cyn, *yel;

/*
 * Descriptor access macros.
 */
#define OLC(d)		((struct oasis_olc_data *)(d)->olc)
#define OLC_MODE(d) 	(OLC(d)->mode)		/* Parse input mode.	*/
#define OLC_NUM(d) 	(OLC(d)->number)	/* Room/Obj VNUM.	*/
#define OLC_VAL(d) 	(OLC(d)->value)		/* Scratch variable.	*/
#define OLC_ZNUM(d) 	(OLC(d)->zone_num)	/* Real zone number.	*/
#define OLC_ROOM(d) 	(OLC(d)->room)		/* Room structure.	*/
#define OLC_OBJ(d) 	(OLC(d)->obj)		/* Object structure.	*/
#define OLC_ZONE(d)     (OLC(d)->zone)		/* Zone structure.	*/
#define OLC_MOB(d)	(OLC(d)->mob)		/* Mob structure.	*/
#define OLC_SHOP(d) 	(OLC(d)->shop)		/* Shop structure.	*/
#define OLC_DESC(d) 	(OLC(d)->desc)		/* Extra description.	*/
#define OLC_CLAN(d)     (OLC(d)->clan)          /* clan structure       */
#if CONFIG_OASIS_MPROG
#define OLC_MPROG(d)	(OLC(d)->mprog)		/* Temporary MobProg.	*/
#define OLC_MPROGL(d)	(OLC(d)->mprogl)	/* MobProg list.	*/
#define OLC_MTOTAL(d)	(OLC(d)->total_mprogs)	/* Total mprog number.	*/
#endif
#define OLC_TRIG(d)     (OLC(d)->trig)        /* Trigger structure.   */
#define OLC_STORAGE(d)  (OLC(d)->storage)    /* For command storage  */

/*
 * Other macros.
 */
#define OLC_EXIT(d)		(OLC_ROOM(d)->dir_option[OLC_VAL(d)])
#define GET_OLC_ZONE(c)		((c)->player_specials->saved.olc_zone)

/*
 * Cleanup types.
 */
#define CLEANUP_ALL		1	/* Free the whole lot.	*/
#define CLEANUP_STRUCTS 	2	/* Don't free strings.	*/

/*
 * Submodes of OEDIT connectedness.
 */
#define OEDIT_MAIN_MENU              	1
#define OEDIT_EDIT_NAMELIST          	2
#define OEDIT_SHORTDESC              	3
#define OEDIT_LONGDESC               	4
#define OEDIT_ACTDESC                	5
#define OEDIT_TYPE                   	6
#define OEDIT_EXTRAS                 	7
#define OEDIT_WEAR                  	8
#define OEDIT_WEIGHT                	9
#define OEDIT_COST                  	10
#define OEDIT_COSTPERDAY            	11
#define OEDIT_TIMER                 	12
#define OEDIT_VALUE_1               	13
#define OEDIT_VALUE_2               	14
#define OEDIT_VALUE_3               	15
#define OEDIT_VALUE_4               	16
#define OEDIT_APPLY                 	17
#define OEDIT_APPLYMOD              	18
#define OEDIT_EXTRADESC_KEY         	19
#define OEDIT_CONFIRM_SAVEDB        	20
#define OEDIT_CONFIRM_SAVESTRING    	21
#define OEDIT_PROMPT_APPLY          	22
#define OEDIT_EXTRADESC_DESCRIPTION 	23
#define OEDIT_EXTRADESC_MENU        	24
#define OEDIT_LEVEL                 	25
#define OEDIT_MAX_LEVEL	                26
#define OEDIT_PERM			27
#define OEDIT_RESIST                  28
#define OEDIT_IMMUNE                  29
#define OEDIT_VULNERABLE              30
#define OEDIT_CLASS                   31
#define OEDIT_SIZE                    32

/*
 * Submodes of REDIT connectedness.
 */
#define REDIT_MAIN_MENU 		1
#define REDIT_NAME 			2
#define REDIT_DESC 			3
#define REDIT_FLAGS 			4
#define REDIT_SECTOR 			5
#define REDIT_EXIT_MENU 		6
#define REDIT_CONFIRM_SAVEDB 		7
#define REDIT_CONFIRM_SAVESTRING 	8
#define REDIT_EXIT_NUMBER 		9
#define REDIT_EXIT_DESCRIPTION 		10
#define REDIT_EXIT_KEYWORD 		11
#define REDIT_EXIT_KEY 			12
#define REDIT_EXIT_DOORFLAGS 		13
#define REDIT_EXTRADESC_MENU 		14
#define REDIT_EXTRADESC_KEY 		15
#define REDIT_EXTRADESC_DESCRIPTION 	16
#define REDIT_SIZE                      17
#define REDIT_ROOM_LIGHT                18
#define REDIT_ROOM_PAINDAM              19
#define REDIT_ROOM_PAINRATE             20
#define REDIT_ROOM_PAINMSG              21
#define REDIT_ROOM_NOMAGIC_MSG_CASTER   22
#define REDIT_ROOM_NOMAGIC_MSG_ROOM     23
#define REDIT_ROOM_MIN_LEVEL            24
#define REDIT_ROOM_MAX_LEVEL            25
#define REDIT_ROOM_MAX_LEVEL_MSG        26
#define REDIT_ROOM_MIN_LEVEL_MSG        27

/*
 * Submodes of ZEDIT connectedness.
 */
#define ZEDIT_MAIN_MENU              	0
#define ZEDIT_DELETE_ENTRY		1
#define ZEDIT_NEW_ENTRY			2
#define ZEDIT_CHANGE_ENTRY		3
#define ZEDIT_COMMAND_TYPE		4
#define ZEDIT_IF_FLAG			5
#define ZEDIT_ARG1			6
#define ZEDIT_ARG2			7
#define ZEDIT_ARG3			8
#define ZEDIT_ZONE_NAME			9
#define ZEDIT_ZONE_LIFE			10
#define ZEDIT_ZONE_BOT			11
#define ZEDIT_ZONE_TOP			12
#define ZEDIT_ZONE_RESET		13
#define ZEDIT_CONFIRM_SAVESTRING	14
#define ZEDIT_PROB	                15
#define ZEDIT_PROB2                     16
#define ZEDIT_SARG1			20
#define ZEDIT_SARG2			21
#define ZEDIT_FLAG_MENU 22
#define ZEDIT_FLAGS  23

/*
 * Submodes of MEDIT connectedness.
 */
#define MEDIT_MAIN_MENU              	0
#define MEDIT_ALIAS			1
#define MEDIT_S_DESC			2
#define MEDIT_L_DESC			3
#define MEDIT_D_DESC			4
#define MEDIT_NPC_FLAGS			5
#define MEDIT_AFF_FLAGS			6
#define MEDIT_CONFIRM_SAVESTRING	7
#define MEDIT_RESIST    8
#define MEDIT_IMMUNE  9
/*
 * Numerical responses.
 */
#define MEDIT_NUMERICAL_RESPONSE	10
#define MEDIT_SEX			11
#define MEDIT_HITROLL			12
#define MEDIT_DAMROLL			13
#define MEDIT_NDD			14
#define MEDIT_SDD			15
#define MEDIT_NUM_HP_DICE		16
#define MEDIT_SIZE_HP_DICE		17
#define MEDIT_ADD_HP			18
#define MEDIT_AC			19
#define MEDIT_EXP			20
#define MEDIT_GOLD			21
#define MEDIT_POS			22
#define MEDIT_DEFAULT_POS		23
#define MEDIT_ATTACK			24
#define MEDIT_LEVEL			25
#define MEDIT_ALIGNMENT			26
#define MEDIT_SIZE			27
#define MEDIT_DIFFICULTY                28

#if CONFIG_OASIS_MPROG
#define MEDIT_MPROG                     27
#define MEDIT_CHANGE_MPROG              28
#define MEDIT_MPROG_COMLIST             29
#define MEDIT_MPROG_ARGS                30
#define MEDIT_MPROG_TYPE                31
#define MEDIT_PURGE_MPROG               32
#endif
#define MEDIT_NUM_ATTACKS               33
#define MEDIT_VULNERABLE                34
#define MEDIT_AFF2_FLAGS                35
#define MEDIT_RACE                      36
#define MEDIT_CLASS                     37
/*
 * Submodes of SEDIT connectedness.
 */
#define SEDIT_MAIN_MENU              	0
#define SEDIT_CONFIRM_SAVESTRING	1
#define SEDIT_NOITEM1			2
#define SEDIT_NOITEM2			3
#define SEDIT_NOCASH1			4
#define SEDIT_NOCASH2			5
#define SEDIT_NOBUY			6
#define SEDIT_BUY			7
#define SEDIT_SELL			8
#define SEDIT_PRODUCTS_MENU		11
#define SEDIT_ROOMS_MENU		12
#define SEDIT_NAMELIST_MENU		13
#define SEDIT_NAMELIST			14
/*
 * Numerical responses.
 */
#define SEDIT_NUMERICAL_RESPONSE	20
#define SEDIT_OPEN1			21
#define SEDIT_OPEN2			22
#define SEDIT_CLOSE1			23
#define SEDIT_CLOSE2			24
#define SEDIT_KEEPER			25
#define SEDIT_BUY_PROFIT		26
#define SEDIT_SELL_PROFIT		27
#define SEDIT_TYPE_MENU			29
#define SEDIT_DELETE_TYPE		30
#define SEDIT_DELETE_PRODUCT		31
#define SEDIT_NEW_PRODUCT		32
#define SEDIT_DELETE_ROOM		33
#define SEDIT_NEW_ROOM			34
#define SEDIT_SHOP_FLAGS		35
#define SEDIT_NOTRADE			36

/*
 * Prototypes to keep.
 */

#ifndef ACMD

void clanedit_setup_new(struct descriptor_data *d);

#define ACMD(name)  \
   void name(struct char_data *ch, char *argument, int cmd, int subcmd)

#endif

void clear_screen(struct descriptor_data *);
ACMD(do_oasis);
//dan clan system
void clanedit_parse(struct descriptor_data *d, char *arg);
ACMD(do_oasis_clanedit);

/*
 * Prototypes, to be moved later.
 */
void medit_free_mobile(struct char_data *mob);
void medit_setup_new(struct descriptor_data *d);
void medit_setup_existing(struct descriptor_data *d, int rmob_num);
void init_mobile(struct char_data *mob);
void medit_save_internally(struct descriptor_data *d);
void medit_save_to_disk(zone_vnum zone_num);
void medit_disp_positions(struct descriptor_data *d);
void medit_disp_mprog(struct descriptor_data *d);
void medit_change_mprog(struct descriptor_data *d);
void medit_disp_mprog_types(struct descriptor_data *d);
void medit_disp_sex(struct descriptor_data *d);
void medit_disp_attack_types(struct descriptor_data *d);
void medit_disp_mob_flags(struct descriptor_data *d);
void medit_disp_aff_flags(struct descriptor_data *d);
void medit_disp_aff2_flags(struct descriptor_data *d);
void medit_disp_menu(struct descriptor_data *d);
void medit_parse(struct descriptor_data *d, char *arg);
void medit_string_cleanup(struct descriptor_data *d, int terminator);
void medit_disp_resist(struct descriptor_data *d);
void medit_disp_immune(struct descriptor_data *d);
void medit_disp_vulnerable(struct descriptor_data *d);
void medit_disp_race(struct descriptor_data *d);
void medit_disp_class(struct descriptor_data *d);



void oedit_setup_new(struct descriptor_data *d);
void oedit_setup_existing(struct descriptor_data *d, int real_num);
void oedit_save_internally(struct descriptor_data *d);
void oedit_save_to_disk(zone_rnum zone_num);
void oedit_disp_container_flags_menu(struct descriptor_data *d);
void oedit_disp_extradesc_menu(struct descriptor_data *d);
void oedit_disp_prompt_apply_menu(struct descriptor_data *d);
void oedit_liquid_type(struct descriptor_data *d);
void oedit_trap_type(struct descriptor_data *d);
void oedit_trap_range1(struct descriptor_data *d);
void oedit_trap_range2(struct descriptor_data *d);
void oedit_trap_location(struct descriptor_data *d);
void oedit_disp_apply_menu(struct descriptor_data *d);
void oedit_disp_weapon_menu(struct descriptor_data *d);
void oedit_disp_spells_menu(struct descriptor_data *d);
void oedit_disp_val1_menu(struct descriptor_data *d);
void oedit_disp_val2_menu(struct descriptor_data *d);
void oedit_disp_val3_menu(struct descriptor_data *d);
void oedit_disp_val4_menu(struct descriptor_data *d);
void oedit_disp_type_menu(struct descriptor_data *d);
void oedit_disp_extra_menu(struct descriptor_data *d);
void oedit_disp_wear_menu(struct descriptor_data *d);
void oedit_disp_menu(struct descriptor_data *d);
void oedit_parse(struct descriptor_data *d, char *arg);
void oedit_disp_perm_menu(struct descriptor_data *d);
void oedit_string_cleanup(struct descriptor_data *d, int terminator);
void oedit_disp_resist(struct descriptor_data *d);
void oedit_disp_immune(struct descriptor_data *d);
void oedit_disp_vulnerable(struct descriptor_data *d);

void redit_string_cleanup(struct descriptor_data *d, int terminator);
void redit_setup_new(struct descriptor_data *d);
void redit_setup_existing(struct descriptor_data *d, int real_num);
void redit_save_internally(struct descriptor_data *d);
void redit_save_to_disk(zone_vnum zone_num);
void redit_disp_extradesc_menu(struct descriptor_data *d);
void redit_disp_exit_menu(struct descriptor_data *d);
void redit_disp_exit_flag_menu(struct descriptor_data *d);
void redit_disp_flag_menu(struct descriptor_data *d);
void redit_disp_sector_menu(struct descriptor_data *d);
void redit_disp_menu(struct descriptor_data *d);
void redit_parse(struct descriptor_data *d, char *arg);
void free_room(struct room_data *room);

void sedit_setup_new(struct descriptor_data *d);
void sedit_setup_existing(struct descriptor_data *d, int rshop_num);
void sedit_save_internally(struct descriptor_data *d);
void sedit_save_to_disk(int zone_num);
void sedit_products_menu(struct descriptor_data *d);
void sedit_compact_rooms_menu(struct descriptor_data *d);
void sedit_rooms_menu(struct descriptor_data *d);
void sedit_namelist_menu(struct descriptor_data *d);
void sedit_shop_flags_menu(struct descriptor_data *d);
void sedit_no_trade_menu(struct descriptor_data *d);
void sedit_types_menu(struct descriptor_data *d);
void sedit_disp_menu(struct descriptor_data *d);
void sedit_parse(struct descriptor_data *d, char *arg);

void zedit_setup(struct descriptor_data *d, int room_num);
void zedit_new_zone(struct char_data *ch, zone_vnum vzone_num, room_vnum bottom, room_vnum top);
void zedit_create_index(int znum, char *type);
void zedit_save_internally(struct descriptor_data *d);
void zedit_disp_menu(struct descriptor_data *d);
void zedit_disp_comtype(struct descriptor_data *d);
void zedit_disp_arg1(struct descriptor_data *d);
void zedit_disp_arg2(struct descriptor_data *d);
void zedit_disp_arg3(struct descriptor_data *d);
void zedit_parse(struct descriptor_data *d, char *arg);

void trigedit_parse(struct descriptor_data *d, char *arg);
void trigedit_setup_existing(struct descriptor_data *d, int rtrg_num);
void trigedit_setup_new(struct descriptor_data *d);

#define CONTEXT_HELP_STRING "help"

#define CONTEXT_OEDIT_MAIN_MENU              	1
#define CONTEXT_OEDIT_EDIT_NAMELIST          	2
#define CONTEXT_OEDIT_SHORTDESC              	3
#define CONTEXT_OEDIT_LONGDESC               	4
#define CONTEXT_OEDIT_ACTDESC                	5
#define CONTEXT_OEDIT_TYPE                   	6
#define CONTEXT_OEDIT_EXTRAS                 	7
#define CONTEXT_OEDIT_WEAR                  	8
#define CONTEXT_OEDIT_WEIGHT                	9
#define CONTEXT_OEDIT_COST                  	10
#define CONTEXT_OEDIT_COSTPERDAY            	11
#define CONTEXT_OEDIT_TIMER                 	12
#define CONTEXT_OEDIT_VALUE_1               	13
#define CONTEXT_OEDIT_VALUE_2               	14
#define CONTEXT_OEDIT_VALUE_3               	15
#define CONTEXT_OEDIT_VALUE_4               	16
#define CONTEXT_OEDIT_APPLY                 	17
#define CONTEXT_OEDIT_APPLYMOD              	18
#define CONTEXT_OEDIT_EXTRADESC_KEY         	19
#define CONTEXT_OEDIT_CONFIRM_SAVEDB        	20
#define CONTEXT_OEDIT_CONFIRM_SAVESTRING    	21
#define CONTEXT_OEDIT_PROMPT_APPLY          	22
#define CONTEXT_OEDIT_EXTRADESC_DESCRIPTION 	23
#define CONTEXT_OEDIT_EXTRADESC_MENU        	24
#define CONTEXT_OEDIT_LEVEL                 	25
#define CONTEXT_OEDIT_PERM			26
#define CONTEXT_REDIT_MAIN_MENU 		27
#define CONTEXT_REDIT_NAME 			28
#define CONTEXT_REDIT_DESC 			29
#define CONTEXT_REDIT_FLAGS 			30
#define CONTEXT_REDIT_SECTOR 			31
#define CONTEXT_REDIT_EXIT_MENU 		32
#define CONTEXT_REDIT_CONFIRM_SAVEDB 		33
#define CONTEXT_REDIT_CONFIRM_SAVESTRING 	34
#define CONTEXT_REDIT_EXIT_NUMBER 		35
#define CONTEXT_REDIT_EXIT_DESCRIPTION 		36
#define CONTEXT_REDIT_EXIT_KEYWORD 		37
#define CONTEXT_REDIT_EXIT_KEY 			38
#define CONTEXT_REDIT_EXIT_DOORFLAGS 		39
#define CONTEXT_REDIT_EXTRADESC_MENU 		40
#define CONTEXT_REDIT_EXTRADESC_KEY 		41
#define CONTEXT_REDIT_EXTRADESC_DESCRIPTION 	42
#define CONTEXT_ZEDIT_MAIN_MENU              	43
#define CONTEXT_ZEDIT_DELETE_ENTRY		44
#define CONTEXT_ZEDIT_NEW_ENTRY			45
#define CONTEXT_ZEDIT_CHANGE_ENTRY		46
#define CONTEXT_ZEDIT_COMMAND_TYPE		47
#define CONTEXT_ZEDIT_IF_FLAG			48
#define CONTEXT_ZEDIT_ARG1			49
#define CONTEXT_ZEDIT_ARG2			50
#define CONTEXT_ZEDIT_ARG3			51
#define CONTEXT_ZEDIT_ZONE_NAME			52
#define CONTEXT_ZEDIT_ZONE_LIFE			53
#define CONTEXT_ZEDIT_ZONE_BOT			54
#define CONTEXT_ZEDIT_ZONE_TOP			55
#define CONTEXT_ZEDIT_ZONE_RESET		56
#define CONTEXT_ZEDIT_CONFIRM_SAVESTRING	57
#define CONTEXT_ZEDIT_SARG1			58
#define CONTEXT_ZEDIT_SARG2			59
#define CONTEXT_MEDIT_MAIN_MENU              	60
#define CONTEXT_MEDIT_ALIAS			61
#define CONTEXT_MEDIT_S_DESC			62
#define CONTEXT_MEDIT_L_DESC			63
#define CONTEXT_MEDIT_D_DESC			64
#define CONTEXT_MEDIT_NPC_FLAGS			65
#define CONTEXT_MEDIT_AFF_FLAGS			66
#define CONTEXT_MEDIT_CONFIRM_SAVESTRING	67
#define CONTEXT_MEDIT_SEX			68
#define CONTEXT_MEDIT_HITROLL			69
#define CONTEXT_MEDIT_DAMROLL			70
#define CONTEXT_MEDIT_NDD			71
#define CONTEXT_MEDIT_SDD			72
#define CONTEXT_MEDIT_NUM_HP_DICE		73
#define CONTEXT_MEDIT_SIZE_HP_DICE		74
#define CONTEXT_MEDIT_ADD_HP			75
#define CONTEXT_MEDIT_AC			76
#define CONTEXT_MEDIT_EXP			77
#define CONTEXT_MEDIT_GOLD			78
#define CONTEXT_MEDIT_POS			79
#define CONTEXT_MEDIT_DEFAULT_POS		80
#define CONTEXT_MEDIT_ATTACK			81
#define CONTEXT_MEDIT_LEVEL			82
#define CONTEXT_MEDIT_ALIGNMENT			83
#define CONTEXT_SEDIT_MAIN_MENU              	84
#define CONTEXT_SEDIT_CONFIRM_SAVESTRING	85
#define CONTEXT_SEDIT_NOITEM1			86
#define CONTEXT_SEDIT_NOITEM2			87
#define CONTEXT_SEDIT_NOCASH1			88
#define CONTEXT_SEDIT_NOCASH2			89
#define CONTEXT_SEDIT_NOBUY			90
#define CONTEXT_SEDIT_BUY			91
#define CONTEXT_SEDIT_SELL			92
#define CONTEXT_SEDIT_PRODUCTS_MENU		93
#define CONTEXT_SEDIT_ROOMS_MENU		94
#define CONTEXT_SEDIT_NAMELIST_MENU		95
#define CONTEXT_SEDIT_NAMELIST			96
#define CONTEXT_SEDIT_OPEN1			97
#define CONTEXT_SEDIT_OPEN2			98
#define CONTEXT_SEDIT_CLOSE1			99
#define CONTEXT_SEDIT_CLOSE2			100
#define CONTEXT_SEDIT_KEEPER			101
#define CONTEXT_SEDIT_BUY_PROFIT		102
#define CONTEXT_SEDIT_SELL_PROFIT		103
#define CONTEXT_SEDIT_TYPE_MENU			104
#define CONTEXT_SEDIT_DELETE_TYPE		105
#define CONTEXT_SEDIT_DELETE_PRODUCT		106
#define CONTEXT_SEDIT_NEW_PRODUCT		107
#define CONTEXT_SEDIT_DELETE_ROOM		108
#define CONTEXT_SEDIT_NEW_ROOM			109
#define CONTEXT_SEDIT_SHOP_FLAGS		110
#define CONTEXT_SEDIT_NOTRADE			111
#define CONTEXT_TRIGEDIT_MAIN_MENU              112
#define CONTEXT_TRIGEDIT_TRIGTYPE               113
#define CONTEXT_TRIGEDIT_CONFIRM_SAVESTRING	114
#define CONTEXT_TRIGEDIT_NAME			115
#define CONTEXT_TRIGEDIT_INTENDED		116
#define CONTEXT_TRIGEDIT_TYPES			117
#define CONTEXT_TRIGEDIT_COMMANDS		118
#define CONTEXT_TRIGEDIT_NARG			119
#define CONTEXT_TRIGEDIT_ARGUMENT		120
#define CONTEXT_SCRIPT_MAIN_MENU		121
#define CONTEXT_SCRIPT_NEW_TRIGGER		122
#define CONTEXT_SCRIPT_DEL_TRIGGER		123
#define CONTEXT_ZEDIT_FLAG_MENU 124

/* includes number 0 */
#define NUM_CONTEXTS 124

/* Prototypes for the context sensitive help system */
int find_context(struct descriptor_data *d);
int find_context_oedit(struct descriptor_data *d);
int find_context_redit(struct descriptor_data *d);
int find_context_zedit(struct descriptor_data *d);
int find_context_medit(struct descriptor_data *d);
int find_context_sedit(struct descriptor_data *d);
int find_context_trigedit(struct descriptor_data *d);
int find_context_script_edit(struct descriptor_data *d);
int context_help(struct descriptor_data *d, char *arg);
void boot_context_help(void);
void free_context_help(void);

