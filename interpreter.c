/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "tedit.h"
#include "dg_scripts.h"
#include "constants.h"
#include "pfdefaults.h" //dan clan system
/* external variables */
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern room_rnum r_newbie_start_room;
extern room_rnum r_sorin_start_room;
extern const char *class_menu;
extern const char *race_menu;
extern char *motd;
extern char *imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct player_index_element *player_table;
extern const char *pc_class_types[];
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern int selfdelete_fastwipe;
extern struct clan_type *clan_info;

/* external functions */
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
void do_start(struct char_data *ch);
int parse_class(char arg);
int parse_race(char arg);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void read_aliases(struct char_data *ch);
void delete_aliases(const char *charname);
void remove_player(int pfilepos);
void read_saved_vars(struct char_data *ch);
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
void clanlog(struct char_data *ch, const char *str, ...);
void perform_cinfo( int clan_number, const char *messg, ... );

/* local functions */
int class_branch_1_guild_work(struct char_data *ch, struct char_data *guildmaster, int cmd); /* Doesn't actually return anything */
int perform_dupe_check(struct descriptor_data *d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen);
int reserved_word(char *argument);
int _parse_name(char *arg, char *name);
int parse_stat_menu(struct descriptor_data *d, char *arg);
int parse_stats(struct descriptor_data *d, char *arg);
int stats_disp_menu(struct descriptor_data *d);
void init_stats(struct descriptor_data *d);
int stats_assign_stat(int abil, char *arg, struct descriptor_data *d);

/* prototypes for all do_x functions. */
ACMD(do_action);
ACMD(do_advance);
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_beep);
ACMD(do_cast);
ACMD(do_circle);
ACMD(do_color);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_diagnose);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_edit);		/* Mainly intended as a test function. */
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_file);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_jail);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_load);
ACMD(do_look);
/* ACMD(do_move); -- interpreter.h */
ACMD(do_not_here);
ACMD(do_oasis);
//ACMD(do_offer);
ACMD(do_olc);
ACMD(do_order);
ACMD(do_page);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_prereq);
ACMD(do_proficiencies);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_rage);
ACMD(do_reboot);
ACMD(do_release);
ACMD(do_remove);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_save);
ACMD(do_saveall);
ACMD(do_say);
ACMD(do_score);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_show_clan);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);
ACMD(do_clan);  /* non-stock commands start here */
ACMD(do_scan);
ACMD(do_attributes);
ACMD(do_peace);
ACMD(do_seize);
ACMD(do_detect_traps);
ACMD(do_claim);
ACMD(do_godnet);
ACMD(do_impnet);
ACMD(do_deitynet);
ACMD(do_countnet);
ACMD(do_liegenet);
ACMD(do_progress);
ACMD(do_fury);
ACMD(do_disarm);
ACMD(do_distant_sight);
ACMD(do_zone);
ACMD(do_zcheck);
/* DG Script ACMD's */
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_tlist);
ACMD(do_tstat);
ACMD(do_masound);
ACMD(do_mkill);
ACMD(do_mjunk);
ACMD(do_mdoor);
ACMD(do_mechoaround);
ACMD(do_msend);
ACMD(do_mecho);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mdamage);
ACMD(do_mteleport);
ACMD(do_mforce);
ACMD(do_mhunt);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mzoneecho);
ACMD(do_vdelete);

ACMD(do_builder_list);
ACMD(do_dig);
ACMD(do_room_copy);

SPECIAL(class_branch_guildmaster);

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

cpp_extern const struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH },
  { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST },
  { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH },
  { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST },
  { "up"       , POS_STANDING, do_move     , 0, SCMD_UP },
  { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN },

  /* now, the main list */
  { "at"       , POS_DEAD    , do_at       , LVL_SAINT, 0 },
  { "attributes", POS_SLEEPING, do_attributes, 0, 0},
  { "advance"  , POS_DEAD    , do_advance  , LVL_GOD, 0 },
  { "alias"    , POS_DEAD    , do_alias    , 0, 0 },
  { "accuse"   , POS_SITTING , do_action   , 0, 0 },
  { "afk"      , POS_DEAD    , do_gen_tog  , 0, SCMD_AFK },
  { "applaud"  , POS_RESTING , do_action   , 0, 0 },
  { "assist"   , POS_FIGHTING, do_assist   , 1, 0 },
  { "ask"      , POS_RESTING , do_spec_comm, 0, SCMD_ASK },
  { "auction"  , POS_SLEEPING, do_gen_comm , 0, SCMD_AUCTION },
  { "autoexits" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOEXIT },
  { "autosplit", POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOSPLIT },
  { "ack"      , POS_RESTING , do_action   , 0, 0 },
  { "admire"   , POS_RESTING , do_action   , 0, 0 },
  { "backstab" , POS_STANDING, do_backstab , 1, 0 },
  { "ban"      , POS_DEAD    , do_ban      , LVL_GOD, 0 },
  { "balance"  , POS_STANDING, do_not_here , 1, 0 },
  { "bash"     , POS_FIGHTING, do_bash     , 1, 0 },
  { "beep"     , POS_DEAD    , do_beep     , LVL_IMPL, 0 },
  { "beg"      , POS_RESTING , do_action   , 0, 0 },
  { "buildwalk", POS_STANDING, do_gen_tog,   LVL_BUILDER, SCMD_BUILDWALK },
  { "bleed"    , POS_RESTING , do_action   , 0, 0 },
  { "blush"    , POS_RESTING , do_action   , 0, 0 },
  { "bow"      , POS_STANDING, do_action   , 0, 0 },
  { "brb"      , POS_RESTING , do_action   , 0, 0 },
  { "brief"    , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF },
  { "brow"     , POS_RESTING , do_action   , 0, 0 },
  { "burp"     , POS_RESTING , do_action   , 0, 0 },
  { "buy"      , POS_STANDING, do_not_here , 0, 0 },
  { "bug"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG },
  { "bark"     , POS_RESTING , do_action   , 0, 0 },
  { "bearhug"  , POS_RESTING , do_action   , 0, 0 },
  { "beam"     , POS_RESTING , do_action   , 0, 0 },
  { "bite"     , POS_RESTING , do_action   , 0, 0 },
  { "blerg"    , POS_RESTING , do_action   , 0, 0 },
  { "blink"    , POS_RESTING , do_action   , 0, 0 },
  { "boo"      , POS_RESTING , do_action   , 0, 0 },
  { "boogie"   , POS_RESTING , do_action   , 0, 0 },
  { "bonk"     , POS_RESTING , do_action   , 0, 0 },
  { "bird"     , POS_RESTING , do_action   , 0, 0 },
  { "bounce"   , POS_STANDING, do_action   , 0, 0 },
  { "cast"     , POS_SITTING , do_cast     , 1, 0 },
  { "cackle"   , POS_RESTING , do_action   , 0, 0 },
  { "caress"   , POS_RESTING , do_action   , 0, 0 },
  { "cane"     , POS_RESTING , do_action   , 0, 0 },
  { "circle"   , POS_STANDING, do_circle   , 1, 0 },
  { "clan"     , POS_RESTING , do_clan     , LVL_IMPL, 0 },
  { "claim"    , POS_RESTING , do_claim    , 1, 0 },
  { "clear"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "close"    , POS_SITTING , do_gen_door , 0, SCMD_CLOSE },
  { "caccept"  , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_ACCEPT },
  { "capply"   , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_APPLY },
  { "clanedit" , POS_DEAD    , do_oasis    , LVL_DEITY, SCMD_OASIS_CLANEDIT },
  { "cbalance" , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_BALANCE },
  { "cdemote"  , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_DEMOTE},
  { "cdeposit" , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_DEPOSIT },
  { "cdismiss" , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_DISMISS },
  { "cpromote" , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_PROMOTE},
  { "clans"    , POS_SLEEPING, do_show_clan, 0, 0 },
  { "clantalk" , POS_SLEEPING, do_gen_tog  , 0, SCMD_CLANTALK },
  { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "consider" , POS_RESTING , do_consider , 0, 0 },
  { "color"    , POS_DEAD    , do_color    , 0, 0 },
  { "comfort"  , POS_RESTING , do_action   , 0, 0 },
  { "comb"     , POS_RESTING , do_action   , 0, 0 },
  { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS },
  { "compact"  , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT },
  { "cough"    , POS_RESTING , do_action   , 0, 0 },
  { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS },
  { "cringe"   , POS_RESTING , do_action   , 0, 0 },
  { "cry"      , POS_RESTING , do_action   , 0, 0 },
  { "csay"     , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_TELL },
  { "ctell"    , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_TELL },
  { "cwho"     , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_WHO },
  { "crevoke"  , POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_REVOKE },
  { "cwithdraw", POS_SLEEPING, do_clan     , LVL_IMPL, SCMD_CLAN_WITHDRAW_GOLD },
  { "cuddle"   , POS_RESTING , do_action   , 0, 0 },
  { "curse"    , POS_RESTING , do_action   , 0, 0 },
  { "curtsey"  , POS_STANDING, do_action   , 0, 0 },
  { "cower"    , POS_RESTING , do_action   , 0, 0 },
  { "cheer"    , POS_STANDING, do_action   , 0, 0 },
/*{ "check"    , POS_STANDING, do_not_here , 1, 0 }, Removed Mail System */
  { "chuckle"  , POS_RESTING , do_action   , 0, 0 },
  { "clap"     , POS_RESTING , do_action   , 0, 0 },
  { "countnet" , POS_SLEEPING, do_countnet   , LVL_COUNT, 0 },
  { "deitynet" , POS_SLEEPING, do_deitynet   , LVL_DEITY, 0 },
  { "destroy"     , POS_RESTING , do_drop  , 0, SCMD_JUNK },
  { "dc"       , POS_DEAD    , do_dc       , LVL_DEITY, 0 },
  { "detect traps", POS_STANDING, do_detect_traps, 1, 0},
  { "deposit"  , POS_STANDING, do_not_here , 1, 0 },
  { "disarm"   , POS_FIGHTING, do_disarm   , 0, 0 },
  { "diagnose" , POS_RESTING , do_diagnose , 0, 0 },
  { "dig"      , POS_DEAD    , do_dig      , LVL_BUILDER, 0 },
  { "display"  , POS_DEAD    , do_display  , 0, 0 },
  { "distant sight"  , POS_STANDING    , do_distant_sight  , 1, 0 },
/*{ "donate"   , POS_RESTING , do_drop     , 0, SCMD_DONATE }, Command removed */
  { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK },
  { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP },
  { "drool"    , POS_RESTING , do_action   , 0, 0 },
  { "doh"      , POS_RESTING , do_action   , 0, 0 },
  { "duck"     , POS_RESTING , do_action   , 0, 0 },
  { "dance"    , POS_STANDING, do_action   , 0, 0 },
  { "date"     , POS_DEAD    , do_date     , LVL_SAINT, SCMD_DATE },
  { "daydream" , POS_SLEEPING, do_action   , 0, 0 },
  { "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT },
  { "echo"     , POS_SLEEPING, do_echo     , LVL_LIEGE, SCMD_ECHO },
  { "emote"    , POS_RESTING , do_echo     , 1, SCMD_EMOTE },
  { ","        , POS_RESTING, do_echo      , 1, SCMD_EMOTE },
  { "embrace"  , POS_STANDING, do_action   , 0, 0 },
  { "enter"    , POS_STANDING, do_enter    , 0, 0 },
  { "equipment", POS_SLEEPING, do_equipment, 0, 0 },
  { "exits"    , POS_RESTING , do_exits    , 0, 0 },
  { "examine"  , POS_SITTING , do_examine  , 0, 0 },
  { "edit"     , POS_DEAD    , do_edit	   ,LVL_IMPL, 0 },	/* Testing! */
  { "eyeroll"  , POS_RESTING , do_action   , 0, 0 },
  { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL },
  { "file"     , POS_SLEEPING, do_file     , LVL_GOD, 0 },
  { "flee"     , POS_FIGHTING, do_flee     , 1, 0 },
  { "flip"     , POS_STANDING, do_action   , 0, 0 },
  { "flirt"    , POS_RESTING , do_action   , 0, 0 },
  { "follow"   , POS_RESTING , do_follow   , 0, 0 },
  { "fondle"   , POS_RESTING , do_action   , 0, 0 },
  { "freeze"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE },
  { "french"   , POS_RESTING , do_action   , 0, 0 },
  { "frown"    , POS_RESTING , do_action   , 0, 0 },
  { "fume"     , POS_RESTING , do_action   , 0, 0 },
  { "flap"     , POS_RESTING , do_action   , 0, 0 },
  { "flex"     , POS_RESTING , do_action   , 0, 0 },
  { "flutter"  , POS_RESTING , do_action   , 0, 0 },
  { "faint"    , POS_RESTING , do_action   , 0, 0 },
  { "force"    , POS_SLEEPING, do_force    , LVL_BUILDER, 0 }, /* change back to leige */
  { "fart"     , POS_RESTING , do_action   , 0, 0 },
  { "fury"     , POS_FIGHTING, do_fury     , 1, 0 },
  { "gecho"    , POS_DEAD    , do_gecho    , LVL_COUNT, 0 },
  { "give"     , POS_RESTING , do_give     , 0, 0 },
  { "giggle"   , POS_RESTING , do_action   , 0, 0 },
  { "glare"    , POS_RESTING , do_action   , 0, 0 },
  { "goto"     , POS_SLEEPING, do_goto     , LVL_BUILDER, 0 },
  { "godnet"   , POS_SLEEPING, do_godnet   , LVL_GOD, 0 },
  { "gold"     , POS_RESTING , do_gold     , 0, 0 },
  { "gossip"   , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP },
  { "group"    , POS_RESTING , do_group    , 1, 0 },
  { "grab"     , POS_RESTING , do_grab     , 0, 0 },
  { "greet"    , POS_RESTING , do_action   , 0, 0 },
  { "grin"     , POS_RESTING , do_action   , 0, 0 },
  { "groan"    , POS_RESTING , do_action   , 0, 0 },
  { "grope"    , POS_RESTING , do_action   , 0, 0 },
  { "grovel"   , POS_RESTING , do_action   , 0, 0 },
  { "growl"    , POS_RESTING , do_action   , 0, 0 },
  { "gsay"     , POS_SLEEPING, do_gsay     , 0, 0 },
  { "gtell"    , POS_SLEEPING, do_gsay     , 0, 0 },
  { "gag"      , POS_RESTING , do_action   , 0, 0 },
  { "grumble"  , POS_RESTING , do_action   , 0, 0 },
  { "gloat"    , POS_RESTING , do_action   , 0, 0 },
  { "grimace"  , POS_RESTING , do_action   , 0, 0 },
  { "get"      , POS_RESTING , do_get      , 0, 0 },
  { "gasp"     , POS_RESTING , do_action   , 0, 0 },
  { "help"     , POS_DEAD    , do_help     , 0, 0 },
  { "handbook" , POS_DEAD    , do_gen_ps   , LVL_SAINT, SCMD_HANDBOOK },
  { "hcontrol" , POS_DEAD    , do_hcontrol , LVL_COUNT, 0 },
  { "hiccup"   , POS_RESTING , do_action   , 0, 0 },
  { "hide"     , POS_RESTING , do_hide     , 1, 0 },
  { "hit"      , POS_FIGHTING, do_hit      , 0, SCMD_HIT },
  { "hold"     , POS_RESTING , do_grab     , 1, 0 },
  { "holylight", POS_DEAD    , do_gen_tog  , LVL_SAINT, SCMD_HOLYLIGHT },
  { "hop"      , POS_RESTING , do_action   , 0, 0 },
  { "house"    , POS_RESTING , do_house    , 0, 0 },
  { "hug"      , POS_RESTING , do_action   , 0, 0 },
  { "headbutt" , POS_RESTING , do_action   , 0, 0 },
  { "high5"    , POS_RESTING , do_action   , 0, 0 },
  { "hiss"     , POS_RESTING , do_action   , 0, 0 },
  { "howl"     , POS_RESTING , do_action   , 0, 0 },
  { "har"      , POS_RESTING , do_action   , 0, 0 },
  { "inventory", POS_DEAD    , do_inventory, 0, 0 },
  { "idea"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA },
  { "impnet"   , POS_DEAD    , do_impnet, LVL_IMPL, 0 },
  { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_SAINT, SCMD_IMOTD },
  { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
  { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO },
  { "insult"   , POS_RESTING , do_insult   , 0, 0 },
  { "invis"    , POS_DEAD    , do_invis    , LVL_LIEGE, 0 },
  { "jail"    , POS_DEAD    , do_jail    , LVL_GOD, 0 },
  { "kill"     , POS_FIGHTING, do_kill     , 0, 0 },
  { "kick"     , POS_FIGHTING, do_kick     , 1, 0 },
  { "kiss"     , POS_RESTING , do_action   , 0, 0 },
  { "look"     , POS_SLEEPING , do_look     , 0, SCMD_LOOK },
  { "laugh"    , POS_RESTING , do_action   , 0, 0 },
  { "last"     , POS_DEAD    , do_last     , LVL_DEITY, 0 },
  { "leave"    , POS_STANDING, do_leave    , 0, 0 },
  { "levels"   , POS_DEAD    , do_levels   , 0, 0 },
  { "liegenet" , POS_SLEEPING, do_liegenet   , LVL_LIEGE, 0 },
  { "list"     , POS_STANDING, do_not_here , 0, 0 },
  { "lick"     , POS_RESTING , do_action   , 0, 0 },
  { "lock"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK },
  { "load"     , POS_DEAD    , do_load     , LVL_BUILDER, 0 }, /* change back to liege */
  { "love"     , POS_RESTING , do_action   , 0, 0 },
  { "motd"     , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD },
/*{ "mail"     , POS_STANDING, do_not_here , 1, 0 }, Removed Mail System */
  { "massage"  , POS_RESTING , do_action   , 0, 0 },
  { "medit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_MEDIT },
  { "mlist"    , POS_DEAD    , do_builder_list, LVL_BUILDER, SCMD_MLIST },
  { "mute"     , POS_DEAD    , do_wizutil  , LVL_DEITY, SCMD_SQUELCH },
  { "murder"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER },
  { "mourn"    , POS_RESTING , do_action   , 0, 0 },
  { "mumble"   , POS_RESTING , do_action   , 0, 0 },
  { "moon"     , POS_STANDING, do_action   , 0, 0 },
  { "moan"     , POS_RESTING , do_action   , 0, 0 },
  { "news"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS },
  { "nibble"   , POS_RESTING , do_action   , 0, 0 },
  { "nod"      , POS_RESTING , do_action   , 0, 0 },
  { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION },
  { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP },
  { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_SAINT, SCMD_NOHASSLE },
  { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT },
  { "noshout"  , POS_SLEEPING, do_gen_tog  , 1, SCMD_DEAF },
  { "nosummon" , POS_DEAD    , do_gen_tog  , 1, SCMD_NOSUMMON },
  { "notell"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL },
  { "notitle"  , POS_DEAD    , do_wizutil  , LVL_DEITY, SCMD_NOTITLE },
  { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_SAINT, SCMD_NOWIZ },
  { "nudge"    , POS_RESTING , do_action   , 0, 0 },
  { "nuzzle"   , POS_RESTING , do_action   , 0, 0 },
  { "noogie"   , POS_RESTING , do_action   , 0, 0 },
  { "order"    , POS_RESTING , do_order    , 1, 0 },
  { "offer"    , POS_STANDING, do_not_here , 1, 0 },
  { "open"     , POS_SITTING , do_gen_door , 0, SCMD_OPEN },
  { "olc"      , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OLC_SAVEINFO },
  { "olist"    , POS_DEAD    , do_builder_list, LVL_BUILDER, SCMD_OLIST },
  { "oedit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_OEDIT },
  { "page"     , POS_DEAD    , do_page     , LVL_COUNT, 0 },
  { "pardon"   , POS_DEAD    , do_wizutil  , LVL_DEITY, SCMD_PARDON },
  { "peer"     , POS_RESTING , do_action   , 0, 0 },
  { "peace"    , POS_DEAD    , do_peace    , LVL_DEITY, 0},
  { "pick"     , POS_STANDING, do_gen_door , 1, SCMD_PICK },
  { "point"    , POS_RESTING , do_action   , 0, 0 },
  { "poke"     , POS_RESTING , do_action   , 0, 0 },
  { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
  { "ponder"   , POS_RESTING , do_action   , 0, 0 },
  { "poofin"   , POS_DEAD    , do_poofset  , LVL_SAINT, SCMD_POOFIN },
  { "poofout"  , POS_DEAD    , do_poofset  , LVL_SAINT, SCMD_POOFOUT },
  { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR },
  { "pout"     , POS_RESTING , do_action   , 0, 0 },
  { "prompt"   , POS_DEAD    , do_display  , 0, 0 },
  { "progress" , POS_DEAD    , do_progress , 0, 0 },
  { "practice" , POS_RESTING , do_practice , 1, 0 },
  { "pray"     , POS_SITTING , do_action   , 0, 0 },
  { "prereq"   , POS_SLEEPING, do_prereq   , 0, 0 },
  { "proficiencies" , POS_SLEEPING, do_proficiencies, 1, 0 },
  { "puke"     , POS_RESTING , do_action   , 0, 0 },
  { "punch"    , POS_RESTING , do_action   , 0, 0 },
  { "purr"     , POS_RESTING , do_action   , 0, 0 },
  { "purge"    , POS_DEAD    , do_purge    , LVL_BUILDER, 0 }, /* change back to leige */
  { "pant"     , POS_RESTING , do_action   , 0, 0 },
  { "pb"       , POS_RESTING , do_action   , 0, 0 },
  { "plot"     , POS_RESTING , do_action   , 0, 0 },
  { "pace"     , POS_RESTING , do_action   , 0, 0 },
  { "put"      , POS_RESTING , do_put      , 0, 0 },
  { "pat"      , POS_RESTING , do_action   , 0, 0 },
  { "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF },
  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_BUILDER, SCMD_QECHO },
  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST },
  { "qui"      , POS_DEAD    , do_quit     , 0, 0 },
  { "quit"     , POS_DEAD    , do_quit     , 0, SCMD_QUIT },
  { "qsay"     , POS_RESTING , do_qcomm    , 0, SCMD_QSAY },
  { "reply"    , POS_SLEEPING, do_reply    , 0, 0 },
  { "rage"     , POS_FIGHTING, do_rage     , 1, 0},
  { "rest"     , POS_RESTING , do_rest     , 0, 0 },
  { "read"     , POS_SLEEPING , do_look     , 0, SCMD_READ },
  { "reload"   , POS_DEAD    , do_reboot   , LVL_DEITY, 0 },
  { "recite"   , POS_RESTING , do_use      , 0, SCMD_RECITE },
/*{ "receive"  , POS_STANDING, do_not_here , 1, 0 }, Removed Mail System */
  { "remove"   , POS_RESTING , do_remove   , 0, 0 },
  { "rent"     , POS_STANDING, do_not_here , 1, 0 },
  { "report"   , POS_RESTING , do_report   , 0, 0 },
  { "release"   , POS_DEAD    , do_release    , LVL_GOD, 0 },
  { "rescue"   , POS_FIGHTING, do_rescue   , 1, 0 },
  { "restore"  , POS_DEAD    , do_restore  , LVL_DEITY, 0 },
  { "return"   , POS_DEAD    , do_return   , 0, 0 },
  { "redit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_REDIT },
  { "rlist"    , POS_DEAD    , do_builder_list, LVL_BUILDER, SCMD_RLIST },
  { "rclone"   , POS_DEAD    , do_room_copy, LVL_BUILDER, 0 },
  { "roll"     , POS_RESTING , do_action   , 0, 0 },
  { "roomflags", POS_DEAD    , do_gen_tog  , LVL_SAINT, SCMD_ROOMFLAGS },
  { "ruffle"   , POS_STANDING, do_action   , 0, 0 },
  { "raise"    , POS_RESTING , do_action   , 0, 0 },
  { "roar"     , POS_RESTING , do_action   , 0, 0 },
  { "say"      , POS_RESTING , do_say      , 0, 0 },
  { "'"        , POS_RESTING , do_say      , 0, 0 },
  { "save"     , POS_SLEEPING, do_save     , 0, 0 },
  { "saveall"  , POS_DEAD    , do_saveall  , LVL_BUILDER, 0},
  { "scan"     , POS_STANDING, do_scan     , 0, 0 },
  { "score"    , POS_DEAD    , do_score    , 0, 0 },
  { "scream"   , POS_RESTING , do_action   , 0, 0 },
  { "sell"     , POS_STANDING, do_not_here , 0, 0 },
  { "send"     , POS_SLEEPING, do_send     , LVL_DEITY, 0 },
  { "set"      , POS_DEAD    , do_set      , LVL_DEITY, 0 },
  { "sedit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_SEDIT },
  { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT },
  { "seize"    , POS_DEAD    , do_seize    , LVL_GOD, 0},
  { "shake"    , POS_RESTING , do_action   , 0, 0 },
  { "shiver"   , POS_RESTING , do_action   , 0, 0 },
  { "show"     , POS_DEAD    , do_show     , LVL_BUILDER, 0 },
  { "shrug"    , POS_RESTING , do_action   , 0, 0 },
  { "shutdow"  , POS_DEAD    , do_shutdown , LVL_DEITY, 0 },
  { "shutdown" , POS_DEAD    , do_shutdown , LVL_DEITY, SCMD_SHUTDOWN },
  { "sigh"     , POS_RESTING , do_action   , 0, 0 },
  { "sing"     , POS_RESTING , do_action   , 0, 0 },
  { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP },
  { "sit"      , POS_RESTING , do_sit      , 0, 0 },
  { "skillset" , POS_SLEEPING, do_skillset , LVL_GOD, 0 },
  { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0 },
  { "slap"     , POS_RESTING , do_action   , 0, 0 },
  { "slist"    , POS_DEAD    , do_builder_list, LVL_BUILDER, SCMD_SLIST },
  { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_SLOWNS },
  { "smile"    , POS_RESTING , do_action   , 0, 0 },
  { "smirk"    , POS_RESTING , do_action   , 0, 0 },
  { "snicker"  , POS_RESTING , do_action   , 0, 0 },
  { "snap"     , POS_RESTING , do_action   , 0, 0 },
  { "snarl"    , POS_RESTING , do_action   , 0, 0 },
  { "sneeze"   , POS_RESTING , do_action   , 0, 0 },
  { "sneak"    , POS_STANDING, do_sneak    , 1, 0 },
  { "sniff"    , POS_RESTING , do_action   , 0, 0 },
  { "snore"    , POS_SLEEPING, do_action   , 0, 0 },
  { "snowball" , POS_STANDING, do_action   , LVL_SAINT, 0 },
  { "snoop"    , POS_DEAD    , do_snoop    , LVL_DEITY, 0 },
  { "snuggle"  , POS_RESTING , do_action   , 0, 0 },
  { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS },
  { "split"    , POS_SITTING , do_split    , 1, 0 },
  { "spank"    , POS_RESTING , do_action   , 0, 0 },
  { "spit"     , POS_STANDING, do_action   , 0, 0 },
  { "squeeze"  , POS_RESTING , do_action   , 0, 0 },
  { "stand"    , POS_RESTING , do_stand    , 0, 0 },
  { "stare"    , POS_RESTING , do_action   , 0, 0 },
  { "stat"     , POS_DEAD    , do_stat     , LVL_BUILDER, 0 },
  { "steal"    , POS_STANDING, do_steal    , 1, 0 },
  { "steam"    , POS_RESTING , do_action   , 0, 0 },
  { "stroke"   , POS_RESTING , do_action   , 0, 0 },
  { "strut"    , POS_STANDING, do_action   , 0, 0 },
  { "sulk"     , POS_RESTING , do_action   , 0, 0 },
  { "switch"   , POS_DEAD    , do_switch   , LVL_BUILDER, 0 }, /* change back to count */
  { "syslog"   , POS_DEAD    , do_syslog   , LVL_BUILDER, 0 },
  { "sock"     , POS_RESTING , do_action   , 0, 0 },
  { "salute"   , POS_RESTING , do_action   , 0, 0 },
  { "scratch"  , POS_RESTING , do_action   , 0, 0 },
  { "stomp"    , POS_RESTING , do_action   , 0, 0 },
  { "tell"     , POS_DEAD    , do_tell     , 0, 0 },
  { "tackle"   , POS_RESTING , do_action   , 0, 0 },
  { "take"     , POS_RESTING , do_get      , 0, 0 },
  { "tango"    , POS_STANDING, do_action   , 0, 0 },
  { "taunt"    , POS_RESTING , do_action   , 0, 0 },
  { "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE },
  { "teleport" , POS_DEAD    , do_teleport , LVL_DEITY, 0 },
  { "tedit"    , POS_DEAD    , do_tedit    , LVL_GOD, 0 },  /* XXX: Oasisify */
  { "thank"    , POS_RESTING , do_action   , 0, 0 },
  { "think"    , POS_RESTING , do_action   , 0, 0 },
  { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW },
  { "title"    , POS_DEAD    , do_title    , 0, 0 },
  { "tickle"   , POS_RESTING , do_action   , 0, 0 },
  { "time"     , POS_DEAD    , do_time     , 0, 0 },
  { "tlist"    , POS_DEAD    , do_builder_list, LVL_BUILDER, SCMD_TLIST },
  { "toggle"   , POS_DEAD    , do_toggle   , 0, 0 },
  { "track"    , POS_STANDING, do_track    , 0, 0 },
  { "trackthru", POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_TRACK },
  { "transfer" , POS_SLEEPING, do_trans    , LVL_BUILDER, 0 },
  { "trigedit" , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_TRIGEDIT},
  { "twiddle"  , POS_RESTING , do_action   , 0, 0 },
  { "typo"     , POS_DEAD    , do_gen_write, 0, SCMD_TYPO },
  { "trip"     , POS_RESTING , do_action   , 0, 0 },
  { "tap"      , POS_RESTING , do_action   , 0, 0 },
  { "tease"    , POS_RESTING , do_action   , 0, 0 },
  { "twitch"   , POS_RESTING , do_action   , 0, 0 },
  { "unlock"   , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK },
  { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0 },
  { "unban"    , POS_DEAD    , do_unban    , LVL_GOD, 0 },
  { "unaffect" , POS_DEAD    , do_wizutil  , LVL_DEITY, SCMD_UNAFFECT },
  { "uptime"   , POS_DEAD    , do_date     , LVL_SAINT, SCMD_UPTIME },
  { "use"      , POS_SITTING , do_use      , 1, SCMD_USE },
  { "users"    , POS_DEAD    , do_users    , LVL_DEITY, 0 },
  { "value"    , POS_STANDING, do_not_here , 0, 0 },
  { "version"  , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION },
  { "visible"  , POS_RESTING , do_visible  , 1, 0 },
  { "vnum"     , POS_DEAD    , do_vnum     , LVL_BUILDER, 0 },
  { "vstat"    , POS_DEAD    , do_vstat    , LVL_BUILDER, 0 },
  { "wake"     , POS_SLEEPING, do_wake     , 0, 0 },
  { "wave"     , POS_RESTING , do_action   , 0, 0 },
  { "wear"     , POS_RESTING , do_wear     , 0, 0 },
  { "weather"  , POS_RESTING , do_weather  , 0, 0 },
  { "who"      , POS_DEAD    , do_who      , 0, 0 },
  { "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },
  { "where"    , POS_RESTING , do_where    , 1, 0 },
  { "whap"     , POS_RESTING , do_action   , 0, 0 },
  { "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },
  { "whine"    , POS_RESTING , do_action   , 0, 0 },
  { "whistle"  , POS_RESTING , do_action   , 0, 0 },
  { "wield"    , POS_RESTING , do_wield    , 0, 0 },
  { "wiggle"   , POS_STANDING, do_action   , 0, 0 },
  { "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0 },
  { "wink"     , POS_RESTING , do_action   , 0, 0 },
  { "withdraw" , POS_STANDING, do_not_here , 1, 0 },
  { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_SAINT, 0 },
  { ";"        , POS_DEAD    , do_wiznet   , LVL_SAINT, 0 },
  { "wizhelp"  , POS_SLEEPING, do_commands , LVL_SAINT, SCMD_WIZHELP },
  { "wizlist"  , POS_DEAD    , do_gen_ps   , LVL_BUILDER, SCMD_WIZLIST },
  { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_IMPL, 0 },
  { "worship"  , POS_RESTING , do_action   , 0, 0 },
  { "write"    , POS_STANDING, do_write    , 1, 0 },
  { "yell"   , POS_RESTING , do_gen_comm , 1, SCMD_YELL },
  { "yawn"     , POS_RESTING , do_action   , 0, 0 },
  { "yodel"    , POS_RESTING , do_action   , 0, 0 },
  { "zreset"   , POS_DEAD    , do_zreset   , LVL_BUILDER, 0 },
  { "zone"     , POS_DEAD    , do_zone     , 0, 0 },
  { "zedit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_ZEDIT },
  /* DG trigger commands */
  { "attach"   , POS_DEAD    , do_attach   , LVL_LIEGE, 0 },
  { "detach"   , POS_DEAD    , do_detach   , LVL_LIEGE, 0 },
  { "tlist"    , POS_DEAD    , do_tlist    , LVL_BUILDER, 0 },
  { "tstat"    , POS_DEAD    , do_tstat    , LVL_BUILDER, 0 },
  { "masound"  , POS_DEAD    , do_masound  , -1, 0 },
  { "mkill"    , POS_STANDING, do_mkill    , -1, 0 },
  { "mjunk"    , POS_SITTING , do_mjunk    , -1, 0 },
  { "mdamage"  , POS_DEAD    , do_mdamage  , -1, 0 },
  { "mdoor"    , POS_DEAD    , do_mdoor    , -1, 0 },
  { "mecho"    , POS_DEAD    , do_mecho    , -1, 0 },
  { "mechoaround" , POS_DEAD , do_mechoaround, -1, 0 },
  { "msend"    , POS_DEAD    , do_msend    , -1, 0 },
  { "mload"    , POS_DEAD    , do_mload    , -1, 0 },
  { "mpurge"   , POS_DEAD    , do_mpurge   , -1, 0 },
  { "mgoto"    , POS_DEAD    , do_mgoto    , -1, 0 },
  { "mat"      , POS_DEAD    , do_mat      , -1, 0 },
  { "mteleport", POS_DEAD    , do_mteleport, -1, 0 },
  { "mforce"   , POS_DEAD    , do_mforce   , -1, 0 },
  { "mhunt"    , POS_DEAD    , do_mhunt    , -1, 0 },
  { "mremember", POS_DEAD    , do_mremember, -1, 0 },
  { "mforget"  , POS_DEAD    , do_mforget  , -1, 0 },
  { "mtransform",POS_DEAD    , do_mtransform,-1, 0 },
  { "mzoneecho", POS_DEAD    , do_mzoneecho, -1, 0 },
  { "vdelete"  , POS_DEAD    , do_vdelete  , LVL_IMPL, 0 },
  { "zcheck"   , POS_DEAD    , do_zcheck   , LVL_GOD, 0 },


  { "\n", 0, 0, 0, 0 } };	/* this must be last */


const char *fill[] =
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd, length;
  char *line;
  char arg[MAX_INPUT_LENGTH];

  /* Removed by Anubis - hide stays in affect until you leave a room -
  REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);  */

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;

  /*
   * special case to handle one-character, non-alphanumeric commands;
   * requested by many people so "'hi" or ";godnet test" is possible.
   * Patch sent by Eric Green and Stefan Wasilewski.
   */
  if (!isalpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);

  /* Since all command triggers check for valid_dg_target before acting, the levelcheck
   * here has been removed. 
   */
  /* otherwise, find the command */
  {
  int cont;                                            /* continue the command checks */
  cont = command_wtrigger(ch, arg, line);              /* any world triggers ? */
  if (!cont) cont = command_mtrigger(ch, arg, line);   /* any mobile triggers ? */
  if (!cont) cont = command_otrigger(ch, arg, line);   /* any object triggers ? */
  if (cont) return;                                    /* yes, command trigger took over */
  }
  for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level)
	break;

  if (*cmd_info[cmd].command == '\n')
    send_to_char(ch, "Huh?!?\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
    send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
 else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_JAIL) && GET_LEVEL(ch) < LVL_IMPL && *cmd_info[cmd].command_pointer != do_say && *cmd_info[cmd].command_pointer != do_look)
    send_to_char(ch, "You can't do a thing.... You're JAILED!!!\r\n");
  else if (cmd_info[cmd].command_pointer == NULL)
    send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n");
  else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_SAINT)
    send_to_char(ch, "You can't use immortal commands while switched.\r\n");
  else if (GET_POS(ch) < cmd_info[cmd].minimum_position)
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char(ch, "You are in a pretty bad shape, unable to do anything!\r\n");
      break;
    case POS_STUNNED:
      send_to_char(ch, "All you can do right now is think about the stars!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "In your dreams, or what?\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "Nah... You feel too relaxed to do that..\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "Maybe you should get on your feet first?\r\n");
      break;
    case POS_PARALYZED:
      send_to_char(ch, "You are too stiff to do anything!\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "No way!  You're fighting for your life!\r\n");
      break;
  } else if (no_specials || !special(ch, cmd, line))
    ((*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd));
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}


void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char arg[MAX_INPUT_LENGTH];
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char(ch, "Currently defined aliases:\r\n");
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(ch, " None.\r\n");
    else {
      while (a != NULL) {
	send_to_char(ch, "%-15s %s\r\n", a->alias, a->replacement);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char(ch, "No such alias.\r\n");
      else
	send_to_char(ch, "Alias deleted.\r\n");
    } else {			/* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
	send_to_char(ch, "You can't alias 'alias'.\r\n");
	return;
      }
      CREATE(a, struct alias_data, 1);
      a->alias = strdup(arg);
      delete_doubledollar(repl);
      a->replacement = strdup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      send_to_char(ch, "Alias added.\r\n");
    }
  }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH];	/* raw? */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  strcpy(buf2, orig);	/* strcpy: OK (orig:MAX_INPUT_LENGTH < buf2:MAX_RAW_INPUT_LENGTH) */
  temp = strtok(buf2, " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);	/* strcpy: OK */
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);		/* strcpy: OK */
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strlcpy(orig, a->replacement, maxlen);
    return (0);
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
  int i, l;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae.
   */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}


int is_number(const char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return (0);

  return (1);
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}


/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
  char *ddread, *ddwrite;

  /* If the string has no dollar signs, return immediately */
  if ((ddwrite = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  ddread = ddwrite;


  while (*ddread)   /* Until we reach the end of the string... */
    if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
      if (*ddread == '$')
	ddread++; /* skip if we saw 2 $'s in a row */

  *ddwrite = '\0';

  return (string);
}


int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}


int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    log("SYSERR: one_argument received a NULL pointer!");
    *first_arg = '\0';
    return (NULL);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg)
{
  char *begin = first_arg;

  do {
    skip_spaces(&argument);

    first_arg = begin;

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}



/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returns 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}



/*
 * Return first space-delimited token in arg1; remainder of string in arg2.
 *
 * NOTE: Requires sizeof(arg2) >= sizeof(string)
 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);	/* strcpy: OK (documentation) */
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(cmd_info[cmd].command, command))
      return (cmd);

  return (-1);
}


int special(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *i;
  struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
    if (GET_ROOM_SPEC(IN_ROOM(ch)) (ch, world + IN_ROOM(ch), cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  /* special in mobile present? */
  for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
    if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
      if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return (1);

  /* special in object present? */
  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  return (0);
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* This function needs to die. */
int _parse_name(char *arg, char *name)
{
  int i;

  skip_spaces(&arg);
  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return (1);

  if (!i)
    return (1);

  return (0);
}


#define RECON		1
#define USURP		2
#define UNSWITCH	3

/* This function seems a bit over-extended. */
int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;

  int id = GET_IDNUM(d->character);

  /*
   * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number.
   */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {
      /* Original descriptor was switched, booting it and restoring normal body control. */

      write_to_output(d, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && GET_IDNUM(k->character) == id && k->original) {
      /* Character taking over their own body, while an immortal was switched to it. */

      do_return(k->character, NULL, 0, 0);
    } else if (k->character && GET_IDNUM(k->character) == id) {
      /* Character taking over their own body. */

      if (!target && STATE(k) == CON_PLAYING) {
	write_to_output(k, "\r\nThis body has been usurped!\r\n");
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
    }
  }

 /*
  * now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist).
  */

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (IN_ROOM(ch) != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for switching into was found - allow login to continue */
  if (!target)
    return (0);

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->char_specials.timer = 0;
  REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
  REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
  STATE(d) = CON_PLAYING;

  switch (mode) {
  case RECON:
    write_to_output(d, "Reconnecting.\r\n");
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), GET_LEVEL(d->character) >= LVL_GOD ? "Hostname Protected" : d->host);
    break;
  case USURP:
    write_to_output(d, "You take over your own body, already in use!\r\n");
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
	"$n's body has been taken over by a new spirit!",
	TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(d->character)), TRUE,
	"%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
     break;
  case UNSWITCH:
    write_to_output(d, "Reconnecting to unswitched char.");
    mudlog(NRM, MAX(LVL_DEITY, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    break;
  }

  return (1);
}



/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  int load_result;	/* Overloaded variable */
  int player_i;
  struct clan_type *cptr = NULL;

  /* OasisOLC states */
  struct {
    int state;
    void (*func)(struct descriptor_data *, char*);
  } olc_functions[] = {
    { CON_OEDIT, oedit_parse },
    { CON_ZEDIT, zedit_parse },
    { CON_SEDIT, sedit_parse },
    { CON_MEDIT, medit_parse },
    { CON_REDIT, redit_parse },
    { CON_TRIGEDIT, trigedit_parse },
    { CON_CLANEDIT, clanedit_parse },
    { -1, NULL }
  };

  skip_spaces(&arg);

  /*
   * Quick check for the OLC states.
   */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state) {
      /* send context-sensitive help if need be */
      if (context_help(d, arg)) return;
      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /* Not in OLC. */
  switch (STATE(d)) {
  case CON_GET_NAME:		/* wait for input of name */
    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
    }
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      char buf[MAX_INPUT_LENGTH], tmp_name[MAX_INPUT_LENGTH];

      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {	/* strcpy: OK (mutual MAX_INPUT_LENGTH) */
	write_to_output(d, "Invalid name, please try another.\r\nName: ");
	return;
      }
      if ((player_i = load_char(tmp_name, d->character)) > -1) {
	GET_PFILEPOS(d->character) = player_i;

	if (PLR_FLAGGED(d->character, PLR_DELETED)) {

	  /* make sure old files are removed so the new player doesn't get
	     the deleted player's equipment (this should probably be a
	     stock behavior)
	  */
	  if((player_i = get_ptable_by_name(tmp_name)) >= 0)
	    remove_player(player_i);

	  /* We get a false positive from the original deleted character. */
	  free_char(d->character);
	  /* Check for multiple creations... */
	  if (!Valid_Name(tmp_name)) {
	    write_to_output(d, "Invalid name, please try another.\r\nName: ");
	    return;
	  }
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  d->character->desc = d;
	  CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));	/* strcpy: OK (size checked above) */
	  GET_PFILEPOS(d->character) = player_i;
	  write_to_output(d, "Did I get that right, %s (Y/N)? ", tmp_name);
	  STATE(d) = CON_NAME_CNFRM;
	} else {
	  /* undo it just in case they are set */
	  REMOVE_BIT(PLR_FLAGS(d->character),
		     PLR_WRITING | PLR_MAILING | PLR_CRYO);
	  REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
	  write_to_output(d, "Password: ");
	  echo_off(d);
	  d->idle_tics = 0;
	  STATE(d) = CON_PASSWORD;
	}
      } else {
	/* player unknown -- make new character */

	/* Check for multiple creations of a character. */
	if (!Valid_Name(tmp_name)) {
	  write_to_output(d, "Invalid name, please try another.\r\nName: ");
	  return;
	}
	CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	strcpy(d->character->player.name, CAP(tmp_name));	/* strcpy: OK (size checked above) */

	write_to_output(d, "Did I get that right, %s (Y/N)? ", tmp_name);
	STATE(d) = CON_NAME_CNFRM;
      }
    }
    break;

  case CON_NAME_CNFRM:		/* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	mudlog(NRM, LVL_DEITY, TRUE, "Request for new char %s denied from [%s] (siteban)", GET_PC_NAME(d->character), d->host);
/*	write_to_output(d, "Sorry, new characters are not allowed from your site!\r\n"); */
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
/*	write_to_output(d, "Sorry, new players can't be created at the moment.\r\n"); */
	mudlog(NRM, LVL_DEITY, TRUE, "Request for new char %s denied from [%s] (wizlock)", GET_PC_NAME(d->character), d->host);
	STATE(d) = CON_CLOSE;
	return;
      }
      write_to_output(d, "New character.\r\nGive me a password for %s: ", GET_PC_NAME(d->character));
      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      write_to_output(d, "Okay, what IS it, then? ");
      free_char(d->character);
      STATE(d) = CON_GET_NAME;
    } else
      write_to_output(d, "Please type Yes or No: ");
    break;

  case CON_PASSWORD:		/* get pwd for known player      */
    /*
     * To really prevent duping correctly, the player's record should
     * be reloaded from disk at this point (after the password has been
     * typed).  However I'm afraid that trying to load a character over
     * an already loaded character is going to cause some problem down the
     * road that I can't see at the moment.  So to compensate, I'm going to
     * (1) add a 15 or 20-second time limit for entering a password, and (2)
     * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
     */

    echo_on(d);    /* turn echo back on */

    /* New echo_on() eats the return on telnet. Extra space better than none. */
    write_to_output(d, "\r\n");

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
	mudlog(BRF, LVL_IMPL, TRUE, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);/* GET_LEVEL(d->character))  >= LVL_GOD ? "Hostname Protected" : d->host); */
	GET_BAD_PWS(d->character)++;
	save_char(d->character, GET_LOADROOM(d->character));
	if (++(d->bad_pws) >= max_bad_pws) {	/* 3 strikes and you're out. */
	  write_to_output(d, "Wrong password... disconnecting.\r\n");
	  STATE(d) = CON_CLOSE;
	} else {
	  write_to_output(d, "Wrong password.\r\nPassword: ");
	  echo_off(d);
	}
	return;
      }

      /* Password was correct. */
      load_result = GET_BAD_PWS(d->character);
      GET_BAD_PWS(d->character) = 0;
      d->bad_pws = 0;

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
/*	write_to_output(d, "Sorry, this char has not been cleared for login from your site!\r\n"); */
	STATE(d) = CON_CLOSE;
	mudlog(NRM, LVL_DEITY, TRUE, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
	return;
      }
      if (GET_LEVEL(d->character) < circle_restrict) {
/*	write_to_output(d, "The game is temporarily restricted.. try again later.\r\n"); */
	STATE(d) = CON_CLOSE;
	mudlog(NRM, LVL_DEITY, TRUE, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
	return;
      }
      /* check and make sure no other copies of this player are logged in */
      if (perform_dupe_check(d))
	return;

      if (GET_LEVEL(d->character) >= LVL_SAINT)
	write_to_output(d, "%s", imotd);
      else
	write_to_output(d, "%s", motd);

      mudlog(BRF, MAX(LVL_DEITY, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has connected.", GET_NAME(d->character), GET_LEVEL(d->character) >= LVL_GOD ? "Hostname Protected" : d->host);

      if (load_result) {
        write_to_output(d, "\r\n\r\n\007\007\007"
		"%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
		CCRED(d->character, C_SPR), load_result,
		(load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
	GET_BAD_PWS(d->character) = 0;
      }
      write_to_output(d, "\r\n*** PRESS RETURN: ");
      STATE(d) = CON_RMOTD;
    }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, GET_PC_NAME(d->character))) {
      write_to_output(d, "\r\nIllegal password.\r\nPassword: ");
      return;
    }
    strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)), MAX_PWD_LENGTH);	/* strncpy: OK (G_P:MAX_PWD_LENGTH+1) */
    *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

    write_to_output(d, "\r\nPlease retype password: ");
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;
    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nPasswords don't match... start over.\r\nPassword: ");
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    echo_on(d);

    if (STATE(d) == CON_CNFPASSWD) {
      write_to_output(d, "\r\nWhat is your sex (M/F)? ");
      STATE(d) = CON_QSEX;
    } else {
      save_char(d->character, NOWHERE);
      write_to_output(d, "\r\nDone.\r\n%s", MENU);
      STATE(d) = CON_MENU;
    }
    break;

  case CON_QSEX:		/* query sex of new user         */
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->player.sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->player.sex = SEX_FEMALE;
      break;
    default:
      write_to_output(d, "That is not a sex..\r\n"
		"What IS your sex? ");
      return;
    }

    write_to_output(d, "%s\r\nRace: ", race_menu);
    STATE(d) = CON_QRACE;
    break;

    case CON_QRACE:
      load_result = parse_race(*arg);
      if (load_result == RACE_UNDEFINED) {
        write_to_output(d, "\r\nThat's not a race.\r\nRace: ");
        return;
      } else
        GET_RACE(d->character) = load_result;

    write_to_output(d, "%s\r\nClass: ", class_menu);
    STATE(d) = CON_QCLASS;
    break;

  case CON_QCLASS:
    load_result = parse_class(*arg);
    if (load_result == CLASS_UNDEFINED) {
      write_to_output(d, "\r\nThat's not a class.\r\nClass: ");
      return;
    } else
      GET_CLASS(d->character) = load_result;

    if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
    /* Now GET_NAME() will work properly. */
    init_char(d->character);
     /* --relistan for statedit -- Some code moved into CON_QSTATS 
         See below for details */
    if (!d->olc)
      CREATE(d->olc, struct oasis_olc_data, 1);

    STATE(d) = CON_QSTATS;
    init_stats(d);
    break;

 /* --relistan 2/22/99 for configurable stats */
  case CON_QSTATS:
    if (parse_stats(d, arg)) {
      if(d->olc) free(d->olc);
      save_char(d->character, NOWHERE);
      save_player_index();
      write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
      STATE(d) = CON_RMOTD;
      mudlog(NRM, LVL_DEITY, TRUE, "%s [%s] new player.", GET_NAME(d->character), d->host);   
    }
    break;   

  case CON_RMOTD:		/* read CR after printing motd   */
    write_to_output(d, "%s", MENU);
    STATE(d) = CON_MENU;
    break;

  case CON_MENU: {		/* get selection from main menu  */
    room_vnum load_room;

    switch (*arg) {
    case '0':
      write_to_output(d, "Goodbye.\r\n");
      STATE(d) = CON_CLOSE;
      break;

    case '1':
      reset_char(d->character);
      read_aliases(d->character);
 
      if (PLR_FLAGGED(d->character, PLR_INVSTART))
	GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);

      /* If you have the copyover patch installed, the following goes in enter_player_game as well */
      /* Check for new clans for leader */
      if (GET_CLAN(d->character) == PFDEF_CLAN) {
        for (cptr = clan_info; cptr; cptr = cptr->next) {
         if (!strcmp(GET_NAME(d->character), cptr->leadersname))
            GET_CLAN(d->character) = cptr->number;
        }
      }
                                                                                                         
      /* can't do an 'else' here, cuz they might have a clan now. */
      if (GET_CLAN(d->character) != PFDEF_CLAN) {
        /* Now check to see if person's clan still exists */
        for (cptr = clan_info; cptr && cptr->number != GET_CLAN(d->character); cptr = cptr->next);
                                                                                                         
        if (cptr == NULL) {  /* Clan no longer exists */
          GET_CLAN(d->character) = PFDEF_CLAN;
          GET_CLAN_RANK(d->character) = PFDEF_CLANRANK;
          GET_HOME(d->character) = 1;
        } else {  /* Was there a change of leadership? */
          if (!strcmp(GET_NAME(d->character), cptr->leadersname))
            GET_CLAN_RANK(d->character) = CLAN_LEADER;
        }
      }


      /*
       * We have to place the character in a room before equipping them
       * or equip_char() will gripe about the person in NOWHERE.
       */
      if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
	load_room = real_room(load_room);

      /* If char was saved with NOWHERE, or real_room above failed... */
      if (load_room == NOWHERE) {
	if (GET_LEVEL(d->character) >= LVL_SAINT){
	  load_room = r_immort_start_room;}
	else if (GET_LEVEL(d->character) == LVL_NEWBIE){
            load_room = r_newbie_start_room;}
        else
           load_room = r_mortal_start_room;
       /* Add other start rooms here - mak */   
      }

      if (PLR_FLAGGED(d->character, PLR_FROZEN))
	load_room = r_frozen_start_room;

      send_to_char(d->character, "%s", WELC_MESSG);
      d->character->next = character_list;
      character_list = d->character;
      char_to_room(d->character, load_room);
      load_result = Crash_load(d->character);

      /* with the copyover patch, this next line goes in enter_player_game() */
      GET_ID(d->character) = GET_IDNUM(d->character);

      /* Clear their load room if it's not persistant. */
      if (!PLR_FLAGGED(d->character, PLR_LOADROOM))
        GET_LOADROOM(d->character) = NOWHERE;
      d->character->player.time.logon = time(0);
      save_char(d->character, GET_LOADROOM(d->character));

      act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

      /* with the copyover patch, this next line goes in enter_player_game() */
      read_saved_vars(d->character);

      greet_mtrigger(d->character, -1);
      greet_memory_mtrigger(d->character);

      STATE(d) = CON_PLAYING;
      if (GET_LEVEL(d->character) == 0) {
	do_start(d->character);
	send_to_char(d->character, "%s", START_MESSG);
      }
      look_at_room(IN_ROOM(d->character), d->character, 0);
      if (has_mail(GET_IDNUM(d->character)))
	send_to_char(d->character, "You have mail waiting.\r\n");
      if (load_result == 2) {	/* rented items lost */
	send_to_char(d->character, "\r\n\007You could not afford your rent!\r\n"
		"Your possesions have been donated to the Salvation Army!\r\n");
      }
      d->has_prompt = 0;
      break;

    case '2':
      if (d->character->player.description) {
	write_to_output(d, "Current description:\r\n%s", d->character->player.description);
	/*
	 * Don't free this now... so that the old description gets loaded
	 * as the current buffer in the editor.  Do setup the ABORT buffer
	 * here, however.
	 *
	 * free(d->character->player.description);
	 * d->character->player.description = NULL;
	 */
	d->backstr = strdup(d->character->player.description);
      }
      write_to_output(d, "Enter the new text you'd like others to see when they look at you.\r\n"
		         "(/s saves /h for help)\r\n");
      d->str = &d->character->player.description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_EXDESC;
      break;

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      write_to_output(d, "\r\nEnter your old password: ");
      echo_off(d);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      write_to_output(d, "\r\nEnter your password for verification: ");
      echo_off(d);
      STATE(d) = CON_DELCNF1;
      break;

    default:
      write_to_output(d, "\r\nThat's not a menu choice!\r\n%s", MENU);
      break;
    }
    break;
  }

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      echo_on(d);
      write_to_output(d, "\r\nIncorrect password.\r\n%s", MENU);
      STATE(d) = CON_MENU;
    } else {
      write_to_output(d, "\r\nEnter a new password: ");
      STATE(d) = CON_CHPWD_GETNEW;
    }
    return;

  case CON_DELCNF1:
    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nIncorrect password.\r\n%s", MENU);
      STATE(d) = CON_MENU;
    } else {
      write_to_output(d, "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		"ARE YOU ABSOLUTELY SURE?\r\n\r\n"
		"Please type \"yes\" to confirm: ");
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
	write_to_output(d, "You try to kill yourself, but the ice stops you.\r\n"
		"Character not deleted.\r\n\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (GET_LEVEL(d->character) < LVL_GOD)
	SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character, GET_LOADROOM(d->character));
      Crash_delete_file(GET_NAME(d->character));
      /* If the selfdelete_fastwipe flag is set (in config.c), remove all
         the player's immediately
      */
      if(selfdelete_fastwipe)
        if((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0) {
          SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
          remove_player(player_i);
        }
      delete_aliases(GET_NAME(d->character));
      save_char(d->character, GET_LOADROOM(d->character));
      write_to_output(d, "Character '%s' deleted!\r\n"
	      "Goodbye.\r\n", GET_NAME(d->character));
      mudlog(NRM, LVL_DEITY, TRUE, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GET_LEVEL(d->character));
      STATE(d) = CON_CLOSE;
      return;
    } else {
      write_to_output(d, "\r\nCharacter not deleted.\r\n%s", MENU);
      STATE(d) = CON_MENU;
    }
    break;

  /*
   * It's possible, if enough pulses are missed, to kick someone off
   * while they are at the password prompt. We'll just defer to let
   * the game_loop() axe them.
   */
  case CON_CLOSE:
    break;

/*
 * CLASS BRANCHING(1) 
 */
  case CON_BRANCH1:
  switch (d->character->player.chclass)
	  {
  /* Priest Part */
   case CLASS_PRIEST:
   switch (*arg) 
	  {
    case 's':
    case 'S':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);
	  d->character->player.chclass = CLASS_SHAMAN;
      break;
	case 'c':
    case 'C':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);
      d->character->player.chclass = CLASS_CLERIC;
      break;
    case 'h':
      write_to_output(d, "Priest Branch 1 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
    break;
  /* Rogue Part */
  case CLASS_ROGUE:
   switch (*arg) 
	  {
    case 't':
    case 'T':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);
	  d->character->player.chclass = CLASS_THIEF;
      break;
	case 'b':
    case 'B':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);
      d->character->player.chclass = CLASS_BARD;
      break;
    case 'h':
      write_to_output(d, "Rogue Branch 1 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
    break;
  /* Fighter Part */
  case CLASS_FIGHTER:
   switch (*arg) 
	  {
    case 'w':
    case 'W':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
      d->character->player.chclass = CLASS_WARRIOR;
      break;
	case 'k':
    case 'K':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);
      d->character->player.chclass = CLASS_KNIGHT;
      break;
    case 'h':
      write_to_output(d, "Fighter Branch 1 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
    break;
  /* Magic User Part */
  case CLASS_MAGIC_USER:
   switch (*arg) 
	  {
    case 'b':
    case 'B':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);
      d->character->player.chclass = CLASS_BATTLEMAGE;
      break;
	case 'e':
    case 'E':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
      d->character->player.chclass = CLASS_ENCHANTER;
      break;
    case 'h':
      write_to_output(d, "Magic User Branch 1 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
    break;
	default:
	   /* Well, this shouldn't happen */ 
       log("SYSERR: Erroneous exit of Branch One Sub in Interp.c.");
	  }
    STATE(d) = CON_PLAYING;
    break;
  
 /*
 * FOR CLASS BRANCHING(2)
 */
  case CON_BRANCH2:
  switch (d->character->player.chclass)
	  {
  case CLASS_SHAMAN:
   switch (*arg) 
	  {
    case 'r':
    case 'R':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
      d->character->player.chclass = CLASS_RANGER;
      break;
	case 'd':
    case 'D':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);
      d->character->player.chclass = CLASS_DRUID;
      break;
    case 'h':
      write_to_output(d, "Shaman Branch 2 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
      break;
  case CLASS_CLERIC:
   switch (*arg) 
	  {
    case 'd':
    case 'D':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
      d->character->player.chclass = CLASS_DISCIPLE;
      break;
	case 'c':
    case 'C':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
      d->character->player.chclass = CLASS_CRUSADER;
      break;
    case 'h':
      write_to_output(d, "Cleric Branch 2 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
	  break;
  case CLASS_THIEF:
   switch (*arg) 
	  {
    case 'a':
    case 'A':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
      d->character->player.chclass = CLASS_ASSASSIN;
      break;
	case 'b':
    case 'B':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
      d->character->player.chclass = CLASS_BOUNTYHUNTER;
      break;
    case 'h':
      write_to_output(d, "Thief Branch 2 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
	  break;
  case CLASS_BARD:
   switch (*arg) 
	  {
    case 'j':
    case 'J':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);      
      d->character->player.chclass = CLASS_JESTER;
      break;
	case 'b':
    case 'B':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
      d->character->player.chclass = CLASS_BLADE;
      break;
    case 'h':
      write_to_output(d, "Bard Branch 2 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
	break;
  case CLASS_WARRIOR:
   switch (*arg) 
	  {
    case 'b':
    case 'B':
	  REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
	  d->character->player.chclass = CLASS_BARBARIAN;
      break;
	case 'm':
    case 'M':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);    
      d->character->player.chclass = CLASS_MONK;
      break;
    case 'h':
      write_to_output(d, "Warrior Branch 2 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
	break;
  case CLASS_KNIGHT:
   switch (*arg) 
	  {
    case 'p':
    case 'P':
	  REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
	  d->character->player.chclass = CLASS_PALADIN;
      break;
	case 's':
    case 'S':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);    
	  d->character->player.chclass = CLASS_SKNIGHT;
      break;
    case 'h':
      write_to_output(d, "Knight Branch 2 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
	break;
  case CLASS_BATTLEMAGE:
   switch (*arg) 
	  {
    case 'c':
    case 'C':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
	  d->character->player.chclass = CLASS_CHAOSMAGE;
      break;
	case 's':
    case 'S':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
	  d->character->player.chclass = CLASS_SORCEROR;
      break;
    case 'h':
      write_to_output(d, "Battle Mage Branch 2 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
	  break;
  case CLASS_ENCHANTER:
   switch (*arg) 
	  {
    case 'n':
    case 'N':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
	  d->character->player.chclass = CLASS_NECROMANCER;
      break;
	case 'a':
    case 'A':
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PROGRESS);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOEXPGAIN);  
	  d->character->player.chclass = CLASS_ALCHEMIST;
      break;
    case 'h':
      write_to_output(d, "Enchanter Branch 2 Help\r\n");
	  return;
    case 'q':
	case 'Q':
      write_to_output(d, "\r\n");
      perform_tell(guildmaster1, d->character, "You have delayed your choice but must choose your path before continuing on your adventure.");
      write_to_output(d, "\r\n");
	  break;
	default:
      write_to_output(d, "That is not a class..\r\n"
		"Which do you choose? ");
      return;
      }
	  break;
	default:
	   /* Well, this shouldn't happen */ 
       log("SYSERR: Erroneous exit of Branch Two Sub in Interp.c.");
	  }
    STATE(d) = CON_PLAYING;
    break;
    
  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
	STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT;	/* Safest to do. */
    break;
  }
}


/************************************************************************
 *  --Statedit  Part of UrathMud                                v1.0    *
 *  Copyright 1999 Karl N. Matthias.  All rights Reserved.              *
 *  You may freely distribute, modify, or sell this code                *
 *  as long as this copyright remains intact.                           *
 *                                                                      *
 *  Based on code by Jeremy Elson, Harvey Gilpin, and George Greer.     *
 ************************************************************************/

/* --relistan 2/22/99 - 2/24/99 */
/* --relistan 2/22/99 for player configurable stats */
int parse_stats(struct descriptor_data *d, char *arg)
{
  struct char_data *ch;

  ch = d->character;

  switch(OLC_MODE(d)) {

    case STAT_QUIT: return 1;

    case STAT_PARSE_MENU:
      if(parse_stat_menu(d, arg)) return 1;
      break;

    case STAT_GET_STR:
      ch->real_abils.str = stats_assign_stat(ch->real_abils.str, arg, d);
      stats_disp_menu(d);
    break;

    case STAT_GET_INT:
      ch->real_abils.intel = stats_assign_stat(ch->real_abils.intel, arg, d);
      stats_disp_menu(d);
    break;

    case STAT_GET_WIS:
      ch->real_abils.wis = stats_assign_stat(ch->real_abils.wis, arg, d);
      stats_disp_menu(d);
    break;

    case STAT_GET_DEX:
      ch->real_abils.dex = stats_assign_stat(ch->real_abils.dex, arg, d);
      stats_disp_menu(d);
    break;

    case STAT_GET_CON:
      ch->real_abils.con = stats_assign_stat(ch->real_abils.con, arg, d);
      stats_disp_menu(d);
    break;

    case STAT_GET_CHA:
      ch->real_abils.cha = stats_assign_stat(ch->real_abils.cha, arg, d);
      stats_disp_menu(d);
    break;

    default:
      OLC_MODE(d) = stats_disp_menu(d); break;
  }

  return 0;
}

/* Roll up the initial stats and assign them to OLC_VAL(d) 
   This isn''t nearly as efficient as it could be, but this code
   is horked from the original roll_real_abils() code */
void init_stats(struct descriptor_data *d)
{
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = rand_number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
        temp ^= table[k];
        table[k] ^= temp;
        temp ^= table[k];
      }
  }

/*  removed to set a STANDARD bonus arangement
  OLC_VAL(d) = table[0] + table[1] + table[2] + table[3]
          + table[4] + table[5];
*/
  d->character->real_abils.str_add = 0;
srand( time(0) );
  /* Minimum stats 8 */
  d->character->real_abils.str = 8;
  d->character->real_abils.intel = 8;
  d->character->real_abils.cha = 8;
  d->character->real_abils.dex = 8;
  d->character->real_abils.con = 8;
  d->character->real_abils.wis = 8;

  OLC_VAL(d) = 30;

  stats_disp_menu(d);

}

int stats_disp_menu(struct descriptor_data *d)
{

send_to_char(d->character,"\r\n");
send_to_char(d->character,"-<[========[ STATS ]=======]>-\r\n");
send_to_char(d->character," <| Total Points Left: %3d |>     You should select the letter of\r\n",OLC_VAL(d));
send_to_char(d->character," <|                        |>     the score you wish to adjust.\r\n");
send_to_char(d->character," <| = Select a stat:       |>     When prompted, enter the new\r\n");
send_to_char(d->character," <| S) Strength     : %2d   |>     score, NOT the amount to add.\r\n",d->character->real_abils.str);
send_to_char(d->character," <| I) Intelligence : %2d   |>     NOTE: If you quit before you\r\n",   d->character->real_abils.intel);
send_to_char(d->character," <| W) Wisdom       : %2d   |>     assign all the points, you\r\n",d->character->real_abils.wis);
send_to_char(d->character," <| D) Dexterity    : %2d   |>     will lose them forever.\r\n",d->character->real_abils.dex);
send_to_char(d->character," <| N) Constitution : %2d   |>     \r\n",d->character->real_abils.con);
send_to_char(d->character," <| C) Charisma     : %2d   |>     \r\n",d->character->real_abils.cha);
send_to_char(d->character," <| Q) Quit                |>     \r\n");
send_to_char(d->character,"-<[========================]>-    \r\n");
send_to_char( d->character,"\r\nEnter Letter to Change: ");

OLC_MODE(d) = STAT_PARSE_MENU;

return 1;
}

int parse_stat_menu(struct descriptor_data *d, char *arg)
{
  /* Main parse loop */
  *arg = LOWER(*arg);
	switch (*arg) {
	/* we don't want random stats   -Anubis
		case 'r':
			d->character->real_abils.str   = (rand() / (RAND_MAX / 15 + 1)+2);
			d->character->real_abils.intel = (rand() / (RAND_MAX / 15 + 1)+2);
			d->character->real_abils.cha   = (rand() / (RAND_MAX / 15 + 1)+2);
			d->character->real_abils.dex   = (rand() / (RAND_MAX / 15 + 1)+2);
			d->character->real_abils.con   = (rand() / (RAND_MAX / 15 + 1)+2);
			d->character->real_abils.wis   = (rand() / (RAND_MAX / 15 + 1)+2);

if (d->character->real_abils.str > 18)
		d->character->real_abils.str = 18;

if (d->character->real_abils.intel > 18)
		d->character->real_abils.intel = 18;

if (d->character->real_abils.cha > 18)
		d->character->real_abils.cha = 18;

if (d->character->real_abils.dex > 18)
		d->character->real_abils.dex = 18;

if (d->character->real_abils.con > 18)
		d->character->real_abils.con = 18;

if (d->character->real_abils.wis > 18)
		d->character->real_abils.wis = 18;

			

			OLC_VAL(d) = (10 + rand() / (RAND_MAX / 10 + 1)+3);
			stats_disp_menu(d);
			break;    */
	  case 'S':
		case 's':
			OLC_MODE(d) = STAT_GET_STR; 
			send_to_char( d->character,"Enter New value: ");
			break;
		case 'I':
		case 'i': 
			OLC_MODE(d) = STAT_GET_INT; 
			send_to_char( d->character,"Enter New value: ");
			break;
		case 'W':
		case 'w': 
			OLC_MODE(d) = STAT_GET_WIS;  
			send_to_char( d->character,"Enter New value: ");
			break;
		case 'D':
		case 'd': 
			OLC_MODE(d) = STAT_GET_DEX;  
			send_to_char( d->character,"Enter New value: ");
			break;
		case 'N':
		case 'n': 
			OLC_MODE(d) = STAT_GET_CON;  
			send_to_char( d->character,"Enter New value: ");
			break; 
		case 'C':
		case 'c': 
			OLC_MODE(d) = STAT_GET_CHA;  
			send_to_char( d->character,"Enter New value: ");
			break;
		case 'Q':
		case 'q': 
			OLC_MODE(d) = STAT_QUIT; 
      			free(d->olc);
      			d->olc = NULL;
			return 1;
  
    default: stats_disp_menu(d);
  }

  return 0;
}

int stats_assign_stat(int abil, char *arg, struct descriptor_data *d)
{
  int temp;

  if (abil > 0) {
      OLC_VAL(d) = OLC_VAL(d) 
          + abil; 
      abil = 0;
  }

  if (atoi(arg) > OLC_VAL(d)) 
    temp = OLC_VAL(d);
  else
    temp = atoi(arg);
  
  /* FIXME WILL HAVE TO CHANGE THIS WHEN RACES HAVE DIFFEREING ABILS */
  if (temp > 18) {
    if (OLC_VAL(d) < 18)
      temp = OLC_VAL(d);
    else temp = 18;
  }
  /* WILL HAVE TO MAKE SURE THEY DON'T LOWER BELOW PREVIOUS VALUES WHEN THEY PROGRESS */
  if (temp < 8) {
    send_to_char(d->character, "Can not lower values below 8.\r\n");
    temp = 8;
  }  
  /* This should throw an error! */
  if (OLC_VAL(d) <= 0) {
    temp = 0;
    OLC_VAL(d) = 0;
    mudlog(BRF, LVL_BUILDER, TRUE, "Stat total below 0: possible code error");

  }
  abil = temp;
  OLC_VAL(d) -= temp;

  return abil;
}

/*              end of fixed stats code                                          */
/*-------------------------------------------------------------------------------*/


/************************************************************
* Special procedures for the Class Branch Guildmaster(s)        *
************************************************************/
/* This doesn't actually return anything that's used.. HA .. just can't be a void */

int class_branch_guild(struct char_data *ch, struct char_data *guildmaster, int cmd)
{
  
  guildmaster1 = guildmaster;

  if (IS_NPC(ch) || !CMD_IS("progress"))
    return (FALSE);

  if (!PLR_FLAGGED(ch, PLR_PROGRESS))
	{
  if (GET_LEVEL(ch) == 10)
		{
	  send_to_char(ch, "You have already progressed at this level.\r\n");
      return (TRUE);
		}
  if (GET_LEVEL(ch) == 25)
		{
	  send_to_char(ch, "You have already progressed at this level.\r\n");
	  return (TRUE);
	  		}
  if (GET_LEVEL(ch) < 10)
		{
	  send_to_char(ch, "You are not quite ready for that yet.\r\n");
	  return (TRUE);
		}
  if ((GET_LEVEL(ch) > 10) && (GET_LEVEL(ch) < 25))
		{
	  send_to_char(ch, "You are not quite ready for that yet.\r\n");
	  return (TRUE);
	    }
  if (GET_LEVEL(ch) > 25)
		{
	  send_to_char(ch, "You have already achieved your last class.\r\n");
	  return (TRUE);
		}
   }
   
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	
	sprinttype(ch->player.chclass, pc_class_types, buf, sizeof(buf));
	send_to_char(ch, "\r\n");
	
	snprintf(buf2, sizeof(buf2), "You have been persuing the path of a %s, but now it is time to take your studies further.", buf); 
	perform_tell(guildmaster, ch, buf2);
    send_to_char(ch, "\r\n");

	/*
    * This is the first branch
    */
	if (GET_LEVEL(ch) == 10)
	{
	switch(GET_CLASS(ch))
	{
	case CLASS_PRIEST:
		perform_tell(guildmaster, ch, "From the path of the Priest you may continue your studies as a Shaman, or a Cleric.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH1;
        send_to_char(ch, " (S)haman\r\n");
		send_to_char(ch, " (C)leric\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter S or C for selection, h for information, or q to come back later: ");
		break;
	case CLASS_ROGUE:
		perform_tell(guildmaster, ch, "From the path of the Rogue you may continue your studies as a Thief, or a Bard.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH1;
        send_to_char(ch, " (T)hief\r\n");
		send_to_char(ch, " (B)ard\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter T or B for selection, h for information, or q to come back later: ");
		break;
	case CLASS_FIGHTER:
		perform_tell(guildmaster, ch, "From the path of the Fighter you may continue your studies as a Warrior, or a Knight.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH1;
        send_to_char(ch, " (W)arrior\r\n");
		send_to_char(ch, " (K)night\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter W or K for selection, h for information, or q to come back later: ");
		break;
	case CLASS_MAGIC_USER:
		perform_tell(guildmaster, ch, "From the path of the Magic User you may continue your studies as a Battle Mage, or an Enchanter.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH1;
        send_to_char(ch, " (B)attle Mage\r\n");
		send_to_char(ch, " (E)nchanter\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter B or E for selection, h for information, or q to come back later: ");
		break;
	default:
    /* Well, this shouldn't happen */
	log("SYSERR: Erroneous exit of Branch One Sub in Interp.c.");
	}
	}

    /*
	 * Here goes Branch 2
	 */

	else if (GET_LEVEL(ch) == 25)
	{
	switch(GET_CLASS(ch))
	{
	case CLASS_SHAMAN:
		perform_tell(guildmaster, ch, "From the path of the Shaman you may continue your studies as a Ranger, or a Druid.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH2;
        send_to_char(ch, " (R)anger\r\n");
		send_to_char(ch, " (D)ruid\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter R or D for selection, h for information, or q to come back later: ");
		break;
    case CLASS_CLERIC:
		perform_tell(guildmaster, ch, "From the path of the Cleric you may continue your studies as a Disciple, or a Crusader.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH2;	
        send_to_char(ch, " (D)isciple\r\n");
		send_to_char(ch, " (C)crusader\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter D or C for selection, h for information, or q to come back later: ");
		break;
	case CLASS_THIEF:
		perform_tell(guildmaster, ch, "From the path of the Thief you may continue your studies as an Assassin, or a Bounty Hunter.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH2;	
        send_to_char(ch, " (A)ssassin\r\n");
		send_to_char(ch, " (B)ounty Hunter\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter A or B for selection, h for information, or q to come back later: ");
		break;
	case CLASS_BARD:
		perform_tell(guildmaster, ch, "From the path of the Bard you may continue your studies as a Jester, or a Blade.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH2;	
        send_to_char(ch, " (J)ester\r\n");
		send_to_char(ch, " (B)lade\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter J or B for selection, h for information, or q to come back later: ");
		break;
	case CLASS_WARRIOR:
		perform_tell(guildmaster, ch, "From the path of the Warrior you may continue your studies as a Barbarian, or a Monk.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH2;	
        send_to_char(ch, " (B)arbarian\r\n");
		send_to_char(ch, " (M)onk\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter B or M for selection, h for information, or q to come back later: ");
		break;
	case CLASS_KNIGHT:
		perform_tell(guildmaster, ch, "From the path of the Knight you may continue your studies as a Paladin, or a Shadow Knight.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH2;	
        send_to_char(ch, " (P)aladin\r\n");
		send_to_char(ch, " (S)hadow Knight\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter P or S for selection, h for information, or q to come back later: ");
		break;
	case CLASS_BATTLEMAGE:
		perform_tell(guildmaster, ch, "From the path of the Battle Mage you may continue your studies as a Chaos Mage, or a Sorceror.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH2;	
        send_to_char(ch, " (C)haos Mage\r\n");
		send_to_char(ch, " (S)orceror\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter C or S for selection, h for information, or q to come back later: ");
		break;
	case CLASS_ENCHANTER:
		perform_tell(guildmaster, ch, "From the path of the Enchanter you may continue your studies as a Necromancer, or an Alchemist.");
        send_to_char(ch, "\r\n");
		STATE(ch->desc) = CON_BRANCH2;	
        send_to_char(ch, " (N)ecromancer\r\n");
		send_to_char(ch, " (A)lchemist\r\n");
        send_to_char(ch, "\r\n");
		send_to_char(ch, "Enter N or A for selection, h for information, or q to come back later: ");
		break;
	default:
    /* Well, this shouldn't happen */ 
	log("SYSERR: Erroneous exit of Branch Two Sub in Interp.c.");
	}	
	}
    
	else /* Well, this shouldn't happen */ 
	log("SYSERR: Erroneous exit of Branch If Sub in Interp.c.");


 return (TRUE);

}

SPECIAL(class_branch_guildmaster)
{
	return (class_branch_guild(ch, (struct char_data *)me, cmd));
}
