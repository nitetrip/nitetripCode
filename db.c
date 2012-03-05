/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "diskio.h"
#include "pfdefaults.h"
#include "dg_scripts.h"
#include "diskio.h"
#include "clan.h" //dan clan system
/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

struct room_data *world = NULL;	/* array of rooms		 */
room_rnum top_of_world = 0;	/* ref to top element of world	 */

struct char_data *character_list = NULL; /* global linked list of chars	 */
struct index_data *mob_index;	/* index table for mobile file	 */
struct char_data *mob_proto;	/* prototypes for mobs		 */
mob_rnum top_of_mobt = 0;	/* top of mobile index table	 */

struct obj_data *object_list = NULL;	/* global linked list of objs	 */
struct index_data *obj_index;	/* index table for object file	 */
struct obj_data *obj_proto;	/* prototypes for objs		 */
obj_rnum top_of_objt = 0;	/* top of object index table	 */

struct zone_data *zone_table;	/* zone table			 */
zone_rnum top_of_zone_table = 0;/* top element of zone tab	 */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages	 */

struct player_index_element *player_table = NULL;	/* index to plr file	 */
int top_of_p_table = 0;		/* ref to top of table		 */
int top_of_p_file = 0;  /* ref of size of pfile   */
long top_idnum = 0;		/* highest idnum in use		 */

struct index_data **trig_index; /* index table for triggers      */
struct trig_data *trigger_list = NULL;  /* all attached triggers */
int top_of_trigt = 0;           /* top of trigger index table    */
long max_mob_id = MOB_ID_BASE;  /* for unique mob id's       */
long max_obj_id = OBJ_ID_BASE;  /* for unique obj id's       */
int dg_owner_purged;            /* For control of scripts */

int no_mail = 0;		/* mail disabled?		 */
int mini_mud = 0;		/* mini-mud mode?		 */
int no_rent_check = 0;		/* skip rent check on boot?	 */
time_t boot_time = 0;		/* time of mud boot		 */
int circle_restrict = 0;	/* level of game restriction	 */
room_rnum r_mortal_start_room;	/* rnum of mortal start room	 */
room_rnum r_recall_room;	/* Recall room */
room_rnum r_immort_start_room;	/* rnum of immort start room	 */
room_rnum r_frozen_start_room;	/* rnum of frozen start room	 */
room_rnum r_sorin_recall_room;  /* rnum of sorin recall room -mak 5.14.05 */
room_rnum r_newbie_start_room;  /* rnum of newbie start room '' */
room_rnum r_sorin_start_room;   /* rnum of sorin start room '' */
room_rnum r_arcane_library_room;/* rnum of tp zones -mak 5.14.05 */
room_rnum r_first_forest_room;  /*                               */
room_rnum r_ethereal_plane_room;/*        superfluous? oh well   */
room_rnum r_astral_plane_room;  /*                               */
room_rnum r_shadow_plane_room;  /*                               */

char *credits = NULL;		/* game credits			 */
char *news = NULL;		/* mud news			 */
char *motd = NULL;		/* message of the day - mortals */
char *imotd = NULL;		/* message of the day - immorts */
char *GREETINGS = NULL;		/* opening credits screen	*/
char *help = NULL;		/* help screen			 */
char *info = NULL;		/* info page			 */
char *wizlist = NULL;		/* list of higher gods		 */
char *immlist = NULL;		/* list of peon gods		 */
char *background = NULL;	/* background story		 */
char *handbook = NULL;		/* handbook for new immortals	 */
char *policies = NULL;		/* policies page		 */
char *zonelist = NULL;     /* zones list */

struct help_index_element *help_table = 0;	/* the help table	 */
int top_of_helpt = 0;		/* top of help index table	 */

struct zone_list *zone_info_table = 0; /* the zone able */
int top_of_zonet = 0;       /* top of zone list index */

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;	/* the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs	*/
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

/* local functions */
int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits);
int check_object_spell_number(struct obj_data *obj, int val);
int check_object_level(struct obj_data *obj, int val);
void setup_dir(FILE *fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE *fl, int mode, char *filename);
int check_object(struct obj_data *);
void parse_room(FILE *fl, int virtual_nr);
void parse_mobile(FILE *mob_f, int nr);
char *parse_object(FILE *obj_f, int nr);
void parse_trigger(FILE *fl, int virtual_nr);
void load_zones(FILE *fl, char *zonename);
void load_help(FILE *fl);
void load_zonelist(FILE *fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void build_player_index(void);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void reboot_wizlists(void);
ACMD(do_reboot);
void boot_world(void);
int count_alias_records(FILE *fl);
int count_hash_records(FILE *fl);
bitvector_t asciiflag_conv(char *flag);
bitvector_t asciiflaglong_conv(char *flag);
void parse_simple_mob(FILE *mob_f, int i, int nr);
void interpret_espec(const char *keyword, const char *value, int i, int nr);
void parse_espec(char *buf, int i, int nr);
void parse_enhanced_mob(FILE *mob_f, int i, int nr);
void get_one_line(FILE *fl, char *buf);
void save_etext(struct char_data *ch);
void save_char_vars(struct char_data *ch);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);
long get_ptable_by_name(const char *name);

/* external functions */
void paginate_string(char *str, struct descriptor_data *d);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void free_alias(struct alias_data *a);
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void boot_social_messages(void);
void update_obj_file(void);	/* In objsave.c */
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE *shop_f, char *filename, int rec_count);
int hsort(const void *a, const void *b);
int zsort(const void *a, const void *b);
void prune_crlf(char *txt);
void sprintbits(long vektor, char *outstring);
void tag_argument(char *argument, char *tag);
void clean_pfiles(void);
void destroy_shops(void);
void free_object_strings(struct obj_data *obj);
void free_object_strings_proto(struct obj_data *obj);
void boot_context_help(void);
void free_context_help(void);
/* external vars */
extern int no_specials;
extern int scheck;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum newbie_start_room;
extern room_vnum sorin_start_room;
extern room_vnum recall_room;
extern room_vnum sorin_recall_room;
extern room_vnum arcane_library_room; /* tp zone rooms - mak 5.14.05 */
extern room_vnum first_forest_room;
extern room_vnum ethereal_plane_room;
extern room_vnum astral_plane_room;
extern room_vnum shadow_plane_room;
extern struct descriptor_data *descriptor_list;
extern const char *unused_spellname;	/* spell_parser.c */

/* external ascii pfile vars */
extern struct pclean_criteria_data pclean_criteria[];
extern int selfdelete_fastwipe;
extern int auto_pwipe;

/* ascii pfiles - set this TRUE if you want poofin/poofout
   +    strings saved in the pfiles
   + */
#define ASCII_SAVE_POOFS  FALSE

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}


/* Wipe out all the loaded text files, for shutting down. */
void free_text_files(void)
{
  char **textfiles[] = {
	&wizlist, &immlist, &news, &credits, &motd, &imotd, &help, &info,
	&policies, &handbook, &background, &zonelist, &GREETINGS, NULL
  };
  int rf;

  for (rf = 0; textfiles[rf]; rf++)
    if (*textfiles[rf]) {
      free(*textfiles[rf]);
      *textfiles[rf] = NULL;
    }
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*') {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
    file_to_string_alloc(NEWS_FILE, &news);
    file_to_string_alloc(CREDITS_FILE, &credits);
    file_to_string_alloc(MOTD_FILE, &motd);
    file_to_string_alloc(IMOTD_FILE, &imotd);
    file_to_string_alloc(HELP_PAGE_FILE, &help);
    file_to_string_alloc(INFO_FILE, &info);
    file_to_string_alloc(POLICIES_FILE, &policies);
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
    file_to_string_alloc(BACKGROUND_FILE, &background);
    file_to_string_alloc(ZONELIST_FILE, &zonelist);
  } else if (!str_cmp(arg, "wizlist"))
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
  else if (!str_cmp(arg, "immlist"))
    file_to_string_alloc(IMMLIST_FILE, &immlist);
  else if (!str_cmp(arg, "news"))
    file_to_string_alloc(NEWS_FILE, &news);
  else if (!str_cmp(arg, "credits"))
    file_to_string_alloc(CREDITS_FILE, &credits);
  else if (!str_cmp(arg, "motd"))
    file_to_string_alloc(MOTD_FILE, &motd);
  else if (!str_cmp(arg, "imotd"))
    file_to_string_alloc(IMOTD_FILE, &imotd);
  else if (!str_cmp(arg, "help"))
    file_to_string_alloc(HELP_PAGE_FILE, &help);
  else if (!str_cmp(arg, "info"))
    file_to_string_alloc(INFO_FILE, &info);
  else if (!str_cmp(arg, "policy"))
    file_to_string_alloc(POLICIES_FILE, &policies);
  else if (!str_cmp(arg, "handbook"))
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
  else if (!str_cmp(arg, "background"))
      file_to_string_alloc(BACKGROUND_FILE, &background);
  else if (!str_cmp(arg, "zonelist"))
    file_to_string_alloc(ZONELIST_FILE, &zonelist);
  else if (!str_cmp(arg, "greetings")) {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
  } else if (!str_cmp(arg, "xhelp")) {
    if (help_table)
      free_help();
    index_boot(DB_BOOT_HLP);
  } else if (!str_cmp(arg, "xzoneinfo")) {
    if (zone_info_table)
      free_zone_list();
    index_boot(DB_BOOT_ZNLST);
  }
  else {
    send_to_char(ch, "Unknown reload option.\r\n");
    return;
  }

  send_to_char(ch, "%s", OK);
}


void boot_world(void)
{
  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading triggers and generating index.");
  index_boot(DB_BOOT_TRG);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  log("Renumbering zone table.");
  renum_zone_table();

  if (!no_specials) {
    log("Loading shops.");
    index_boot(DB_BOOT_SHP);
  }
}


void free_extra_descriptions(struct extra_descr_data *edesc)
{
  struct extra_descr_data *enext;

  for (; edesc; edesc = enext) {
    enext = edesc->next;

    free(edesc->keyword);
    free(edesc->description);
    free(edesc);
  }
}


/* Free the world, in a memory allocation sense. */
void destroy_db(void)
{
  ssize_t cnt, itr;
  struct char_data *chtmp;
  struct obj_data *objtmp;

  /* Active Mobiles & Players */
  while (character_list) {
    chtmp = character_list;
    character_list = character_list->next;
    free_char(chtmp);
  }

  /* Active Objects */
  while (object_list) {
    objtmp = object_list;
    object_list = object_list->next;
    free_obj(objtmp);
  }

  /* Rooms */
  for (cnt = 0; cnt <= top_of_world; cnt++) {
    if (world[cnt].name)
      free(world[cnt].name);
    if (world[cnt].description)
      free(world[cnt].description);
    free_extra_descriptions(world[cnt].ex_description);

    for (itr = 0; itr < NUM_OF_DIRS; itr++) {
      if (!world[cnt].dir_option[itr])
        continue;

      if (world[cnt].dir_option[itr]->general_description)
        free(world[cnt].dir_option[itr]->general_description);
      if (world[cnt].dir_option[itr]->keyword)
        free(world[cnt].dir_option[itr]->keyword);
      free(world[cnt].dir_option[itr]);
    }
  }
  free(world);

  /* Objects */
  for (cnt = 0; cnt <= top_of_objt; cnt++) {
    if (obj_proto[cnt].name)
      free(obj_proto[cnt].name);
    if (obj_proto[cnt].description)
      free(obj_proto[cnt].description);
    if (obj_proto[cnt].short_description)
      free(obj_proto[cnt].short_description);
    if (obj_proto[cnt].action_description)
      free(obj_proto[cnt].action_description);
    free_extra_descriptions(obj_proto[cnt].ex_description);
  }
  free(obj_proto);
  free(obj_index);

  /* Mobiles */
  for (cnt = 0; cnt <= top_of_mobt; cnt++) {
    if (mob_proto[cnt].player.name)
      free(mob_proto[cnt].player.name);
    if (mob_proto[cnt].player.title)
      free(mob_proto[cnt].player.title);
    if (mob_proto[cnt].player.short_descr)
      free(mob_proto[cnt].player.short_descr);
    if (mob_proto[cnt].player.long_descr)
      free(mob_proto[cnt].player.long_descr);
    if (mob_proto[cnt].player.description)
      free(mob_proto[cnt].player.description);

    while (mob_proto[cnt].affected)
      affect_remove(&mob_proto[cnt], mob_proto[cnt].affected);
  }
  free(mob_proto);
  free(mob_index);

  /* Shops */
  destroy_shops();

  /* Zones */
  for (cnt = 0; cnt <= top_of_zone_table; cnt++) {
    if (zone_table[cnt].name)
      free(zone_table[cnt].name);
    if (zone_table[cnt].cmd)
      free(zone_table[cnt].cmd);
  }
  free(zone_table);
  
  free_context_help();
}


/* body of the booting system */
void boot_db(void)
{
  zone_rnum i;

  log("Boot db -- BEGIN.");

  log("Resetting the game time:");
  reset_time();

  log("Reading news, credits, help, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  file_to_string_alloc(ZONELIST_FILE, &zonelist);
  if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
    prune_crlf(GREETINGS);

  log("Loading spell definitions.");
  mag_assign_spells();

  boot_world();

  log("Loading help entries.");
  index_boot(DB_BOOT_HLP);
 
  log("Loading zone info entries.");
  index_boot(DB_BOOT_ZNLST);
  
  log("Setting up context sensitive help system for OLC");
  boot_context_help();

  log("Generating player index.");
  build_player_index();

  if(auto_pwipe) {
    log("Cleaning out the pfiles.");
    clean_pfiles();
  }

  log("Loading fight messages.");
  load_messages();

  log("Loading social messages.");
  boot_social_messages();

  log("Assigning function pointers:");

  if (!no_specials) {
    log("   Mobiles.");
    assign_mobiles();
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
  }

  log("Assigning spell and skill levels.");
  init_spell_levels();

  log("Sorting command list and spells.");
  sort_commands();
  sort_spells();

  log("Booting mail system.");
  if (!scan_file()) {
    log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }
  log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check) {
    log("Deleting timed-out crash and rent files:");
    update_obj_file();
    log("   Done.");
  }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!mini_mud) {
    log("Booting houses.");
    House_boot();
  }

//dan clan system
  log("Loading clans.");
  load_clans();

  for (i = 0; i <= top_of_zone_table; i++) {
    log("Resetting #%d: %s (rooms %d-%d).", zone_table[i].number,
	zone_table[i].name, zone_table[i].bot, zone_table[i].top);
    reset_zone(i);
  }

  reset_q.head = reset_q.tail = NULL;

  boot_time = time(0);

  log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
  time_t beginning_of_time = 0;
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "r")) == NULL)
    log("SYSERR: Can't read from '%s' time file.", TIME_FILE);
  else {
    fscanf(bgtime, "%ld\n", &beginning_of_time);
    fclose(bgtime);
  }
  if (beginning_of_time == 0)
    beginning_of_time = 650336715;

  time_info = *mud_time_passed(time(0), beginning_of_time);

  if (time_info.hours <= 4)
    weather_info.sunlight = SUN_DARK;
  else if (time_info.hours == 5)
    weather_info.sunlight = SUN_RISE;
  else if (time_info.hours <= 20)
    weather_info.sunlight = SUN_LIGHT;
  else if (time_info.hours == 21)
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
	  time_info.day, time_info.month, time_info.year);

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  weather_info.change = 0;

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
}


/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(struct time_info_data *when)
{
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "w")) == NULL)
    log("SYSERR: Can't write to '%s' time file.", TIME_FILE);
  else {
    fprintf(bgtime, "%ld\n", mud_time_to_secs(when));
    fclose(bgtime);
  }
}


void free_player_index(void)
{
  int tp;

  if (!player_table)
    return;

  for (tp = 0; tp <= top_of_p_table; tp++)
    if (player_table[tp].name)
      free(player_table[tp].name);

  free(player_table);
  player_table = NULL;
  top_of_p_table = 0;
}

/* new version to build player index for ascii pfiles */
/* generate index table for the player file */
void build_player_index(void)
{
   int rec_count = 0, i;
   FBFILE *plr_index;
   char index_name[40], line[256], bits[64];
   char arg2[80];

   sprintf(index_name, "%s", PLR_INDEX_FILE);
   if(!(plr_index = fbopen(index_name, FB_READ))) {
     top_of_p_table = -1;
     log("No player index file!  First new char will be IMP!");
     return;
  }
  while(fbgetline(plr_index, line))
    if(*line != '~')
      rec_count++;
  fbrewind(plr_index);

  if(rec_count == 0) {
    player_table = NULL;
    top_of_p_table = -1;
    return;
  }
  CREATE(player_table, struct player_index_element, rec_count);
  for(i = 0; i < rec_count; i++) {
    fbgetline(plr_index, line);
    sscanf(line, "%ld %s %d %s %d", &player_table[i].id, arg2,
      &player_table[i].level, bits, (int *)&player_table[i].last);
    CREATE(player_table[i].name, char, strlen(arg2) + 1);
    strcpy(player_table[i].name, arg2);
    player_table[i].flags = asciiflag_conv(bits);
    top_idnum = MAX(top_idnum, player_table[i].id);
  }

  fbclose(plr_index);
  top_of_p_file = top_of_p_table = i - 1;
}

/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *	-gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$') {
    /* skip the text */
    do {
      get_one_line(fl, line);
      if (feof(fl))
	goto ackeof;
    } while (*line != '#');

    /* now count keywords */
    scan = key;
    do {
      scan = one_word(scan, next_key);
      if (*next_key)
        ++total_keywords;
    } while (*next_key);

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if (feof(fl))
      goto ackeof;
  }

  return (total_keywords);

  /* No, they are not evil. -gg 6/24/98 */
ackeof:	
  log("SYSERR: Unexpected end of help file.");
  exit(1);	/* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE *fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}

void index_boot(int mode)
{
  const char *index_filename, *prefix = NULL;	/* NULL or egcs 1.1 complains */
  FILE *db_index, *db_file;
  int rec_count = 0, size[2];
  char buf2[PATH_MAX], buf1[MAX_STRING_LENGTH];

  switch (mode) {
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  case DB_BOOT_HLP:
    prefix = HLP_PREFIX;
    break;
  case DB_BOOT_ZNLST:
    prefix = ZNLST_PREFIX;
    break;
  case DB_BOOT_TRG:
    prefix = TRG_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand %d to index_boot!", mode);
    exit(1);
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  snprintf(buf2, sizeof(buf2), "%s%s", prefix, index_filename);
  if (!(db_index = fopen(buf2, "r"))) {
    log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$') {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix,
	  index_filename, strerror(errno));
      fscanf(db_index, "%s\n", buf1);
      continue;
    } else {
      if (mode == DB_BOOT_ZON)
	rec_count++;
      else if (mode == DB_BOOT_HLP || mode == DB_BOOT_ZNLST)
	rec_count += count_alias_records(db_file);
      else
	rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
  }

  /* Exit if 0 records, unless this is shops */

  if (!rec_count) {
    if (mode == DB_BOOT_SHP)
      return;
    log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
	index_filename);
    exit(1);
  } 

  /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
  switch (mode) {
  case DB_BOOT_TRG:
    CREATE(trig_index, struct index_data *, rec_count);
    break;
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    size[0] = sizeof(struct room_data) * rec_count;
    log("   %d rooms, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct char_data) * rec_count;
    log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct obj_data) * rec_count;
    log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    size[0] = sizeof(struct zone_data) * rec_count;
    log("   %d zones, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count);
    size[0] = sizeof(struct help_index_element) * rec_count;
    log("   %d entries, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_ZNLST:
    CREATE(zone_info_table, struct zone_list, rec_count);
    size[0] = sizeof(struct zone_list) * rec_count;
    log("   %d entries, %d bytes.", rec_count, size[0]);
    break;  
  }

  rewind(db_index);
  fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$') {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      log("SYSERR: %s: %s", buf2, strerror(errno));
      exit(1);
    }
    switch (mode) {
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
    case DB_BOOT_TRG:
      discrete_load(db_file, mode, buf2);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      /*
       * If you think about it, we have a race here.  Although, this is the
       * "point-the-gun-at-your-own-foot" type of race.
       */
      load_help(db_file);
      break;
    case DB_BOOT_ZNLST:
      load_zonelist(db_file);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    }

    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
  }
  fclose(db_index);

  /* sort the help index */
  if (mode == DB_BOOT_HLP) {
    qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
    top_of_helpt--;
  }
  if (mode == DB_BOOT_ZNLST) {
    qsort(zone_info_table, top_of_zonet, sizeof(struct zone_list), zsort);
    top_of_zonet--;
  }
}


void discrete_load(FILE *fl, int mode, char *filename)
{
  int nr = -1, last;
  char line[READ_SIZE];

  const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg"};
  /* modes positions correspond to DB_BOOT_xxx in db.h */

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
	if (nr == -1) {
	  log("SYSERR: %s file %s is empty!", modes[mode], filename);
	} else {
	  log("SYSERR: Format error in %s after %s #%d\n"
	      "...expecting a new %s, but file ended!\n"
	      "(maybe the file is not terminated with '$'?)", filename,
	      modes[mode], nr, modes[mode]);
	}
	exit(1);
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	log("SYSERR: Format error after %s #%d", modes[mode], last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	switch (mode) {
	case DB_BOOT_WLD:
	  parse_room(fl, nr);
	  break;
	case DB_BOOT_MOB:
	  parse_mobile(fl, nr);
	  break;
        case DB_BOOT_TRG:
          parse_trigger(fl, nr);
          break;
	case DB_BOOT_OBJ:
	  strlcpy(line, parse_object(fl, nr), sizeof(line));
	  break;
	}
    } else {
      log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
	  filename, modes[mode], nr);
      log("SYSERR: ... offending line: '%s'", line);
      exit(1);
    }
  }
}

char fread_letter(FILE *fp)
{
  char c;
  do {
    c = getc(fp);  
  } while (isspace(c));
  return c;
}

bitvector_t asciiflag_conv(char *flag)
{
  bitvector_t flags = 0;
  int is_num = TRUE;
  char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1ULL << (*p - 'a');
    else if (isupper(*p))
      flags |= 1ULL << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}

bitvector_t asciiflaglong_conv( char *flag )
{
  bitvector_t flags = 0;
  char *p;

  for( p = flag; *p; p++ )
  {    
    /* 0 (zero) is a special flag that cannot be set; it is used as the
     * field placeholder when no bits have been set.  Just ignore it.  */
    if (*p == '0')
      // flags |= 1ULL << 52;
      ;
    else if (*p == '1')
      flags |= 1ULL << 53;
    else if (*p == '2')
      flags |= 1ULL << 54;
    else if (*p == '3')
      flags |= 1ULL << 55;
    else if (*p == '4')
      flags |= 1ULL << 56;
    else if (*p == '5')
      flags |= 1ULL << 57;
    else if (*p == '6')
      flags |= 1ULL << 58;
    else if (*p == '7')
      flags |= 1ULL << 59;
    else if (*p == '8')
      flags |= 1ULL << 60;
    else if (*p == '9')
      flags |= 1ULL << 61;
    else if (*p == '@')
      flags |= 1ULL << 62;
    else if (*p == '$')
      flags |= 1ULL << 63;        
    else if (islower(*p))
      flags |= 1ULL << (*p - 'a');
    else if (isupper(*p))
      flags |= 1ULL << (26 + (*p - 'A'));    
  }
 
  return (flags);
}


/* load the rooms */
void parse_room(FILE *fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i, retval;
  char line[READ_SIZE], flags[128], buf2[MAX_STRING_LENGTH], buf[128];
  struct extra_descr_data *new_descr;
  char letter; 

  /* This really had better fit or there are other problems. */
  snprintf(buf2, sizeof(buf2), "room #%d", virtual_nr);

  if (virtual_nr < zone_table[zone].bot) {
    log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      log("SYSERR: Room %d is outside of any zone.", virtual_nr);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
	virtual_nr);
    exit(1);
  }

  if ((retval = sscanf(line, " %d %s %d %d %d %d %d", t, flags, t + 2, t + 3, 
t + 4, t + 5, t + 6)) != 7) {

   if (retval == 3)
    t[3] = 0;
   else if (retval == 4)
    t[4] = 0;
   else if (retval == 5)
    t[5] = 0;
   else if (retval == 6)
    t[6] = LVL_SAINT - 1;
   else {
    log("SYSERR: Format error in roomflags/sector type/size of room #%d",
	virtual_nr);
    exit(1);
    }
  }
  /* t[0] is the zone number; ignored with the zone-file system */

  world[room_nr].room_flags = asciiflaglong_conv(flags);
  sprintf(flags, "room #%d", virtual_nr);	/* sprintf: OK (until 399-bit integers) */
  check_bitvector_names(world[room_nr].room_flags, room_bits_count, flags, "room");

  world[room_nr].sector_type = t[2];
  world[room_nr].size = t[3];
  world[room_nr].nat_light = t[4];  /* amount of natural light */
  if (t[6] == 0)
     t[6] = LVL_SAINT-1;
  world[room_nr].min_level = t[5];
  world[room_nr].max_level = t[6];
  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0;	/* Zero light sources */

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;
  world[room_nr].pain_message = NULL;
  world[room_nr].pain_rate = 0;
  world[room_nr].pain_damage = 0;
  snprintf(buf, sizeof(buf), "SYSERR: Format error in room #%d (expecting D/E/P/S)", virtual_nr);

top_of_world = room_nr;

  for (;;) {
    if (!get_line(fl, line)) {
      log("%s", buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      break;
    case 'P':   /* is this a pain room? */
      world[room_nr].pain_message = fread_string(fl, line);
      world[room_nr].pain_damage = atoi(fread_string(fl, line + 1));
      world[room_nr].pain_rate = atoi(fread_string(fl, line +2));
      break;
    case 'N':   /* is there a no magic room message */
      world[room_nr].nomagic_message_caster = fread_string(fl, line);
      world[room_nr].nomagic_message_room = fread_string(fl, line + 2);
      break;
    case 'F':
      world[room_nr].min_level_message = fread_string(fl, line);
      world[room_nr].max_level_message = fread_string(fl, line + 1);
      break;
    case 'S':			/* end of room */
      /* DG triggers -- script is defined after the end of the room */
      letter = fread_letter(fl);
      ungetc(letter, fl);
      while (letter=='T') {
        dg_read_trigger(fl, &world[room_nr], WLD_TRIGGER);
        letter = fread_letter(fl);
        ungetc(letter, fl);
      }
      room_nr++;
      return;
     default:
      log("%s", buf);
      exit(1);
    }
  }
}



/* read direction data */
void setup_dir(FILE *fl, int room, int dir)
{
  int t[5];
  char line[READ_SIZE], buf2[128];

  snprintf(buf2, sizeof(buf2), "room #%d, direction D%d", GET_ROOM_VNUM(room), dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (t[0] == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else if (t[0] == 3)
	world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_HIDDEN;
  else if (t[0] == 4)
	world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_HIDDEN | EX_PICKPROOF;
  else
    world[room].dir_option[dir]->exit_info = 0;

  world[room].dir_option[dir]->key = t[1];
  world[room].dir_option[dir]->to_room = t[2];
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  if ((r_mortal_start_room = real_room(mortal_start_room)) == NOWHERE) {
    log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(immort_start_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(frozen_start_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
  if ((r_newbie_start_room = real_room(newbie_start_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Newbie start room does not exist.  Change in config.c.");
    r_newbie_start_room = r_mortal_start_room;
  }
  if ((r_sorin_recall_room = real_room(sorin_recall_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Sorin recall room does not exist.  Change in config.c.");
    r_sorin_recall_room = r_mortal_start_room;
  }
  if ((r_recall_room = real_room(recall_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Recall room does not exist.  Change in config.c.");
    r_recall_room = r_mortal_start_room;
  }
  if ((r_sorin_start_room = real_room(sorin_start_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Sorin start room does not exist.  Change in config.c.");
    r_sorin_start_room = r_mortal_start_room;
  }
  if ((r_arcane_library_room = real_room(arcane_library_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Arcane Library start room does not exist.  Change in config.c.");
    r_arcane_library_room = r_mortal_start_room;
  }
  if ((r_first_forest_room = real_room(first_forest_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: First Forest start room does not exist.  Change in config.c.");
    r_first_forest_room = r_mortal_start_room;
  }
  if ((r_ethereal_plane_room = real_room(ethereal_plane_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Ethereal Plane start room does not exist.  Change in config.c.");
    r_ethereal_plane_room = r_mortal_start_room;
  }
  
  if ((r_astral_plane_room = real_room(astral_plane_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Astral Plane start room does not exist.  Change in config.c.");
    r_astral_plane_room = r_mortal_start_room;
  }
  if ((r_shadow_plane_room = real_room(shadow_plane_room)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Shadow Plane start room does not exist.  Change in config.c.");
    r_shadow_plane_room = r_mortal_start_room;
  }
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  int room, door;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door])
	if (world[room].dir_option[door]->to_room != NOWHERE)
	  world[room].dir_option[door]->to_room =
	    real_room(world[room].dir_option[door]->to_room);
}


#define ZCMD zone_table[zone].cmd[cmd_no]

/*
 * "resulve vnums into rnums in the zone reset tables"
 *
 * Or in English: Once all of the zone reset tables have been loaded, we
 * resolve the virtual numbers into real numbers all at once so we don't have
 * to do it repeatedly while the game is running.  This does make adding any
 * room, mobile, or object a little more difficult while the game is running.
 *
 * NOTE 1: Assumes NOWHERE == NOBODY == NOTHING.
 * NOTE 2: Assumes sizeof(room_rnum) >= (sizeof(mob_rnum) and sizeof(obj_rnum))
 */
void renum_zone_table(void)
{
  int cmd_no;
  room_rnum a, b, c, olda, oldb, oldc;
  zone_rnum zone;
  char buf[128];

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
      a = b = c = 0;
      olda = ZCMD.arg1;
      oldb = ZCMD.arg2;
      oldc = ZCMD.arg3;
      switch (ZCMD.command) {
      case 'M':
	a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
	c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'O':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	if (ZCMD.arg3 != NOWHERE)
	  c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'G':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'E':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'P':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	c = ZCMD.arg3 = real_object(ZCMD.arg3);
	break;
      case 'D':
	a = ZCMD.arg1 = real_room(ZCMD.arg1);
	break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
	b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      case 'T': /* a trigger */
#if 0  /* triggers are stored as vnums! */
        b = ZCMD.arg2 = real_trigger(ZCMD.arg2);
#endif
        b = real_trigger(ZCMD.arg2);
        c = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'V': /* trigger variable assignment */
        b = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      }
      if (a == NOWHERE || b == NOWHERE || c == NOWHERE) {
	if (!mini_mud) {
	  snprintf(buf, sizeof(buf), "Invalid vnum %d, cmd disabled",
			 a == NOWHERE ? olda : b == NOWHERE ? oldb : oldc);
	  log_zone_error(zone, cmd_no, buf);
	}
	ZCMD.command = '*';
      }
    }
}



void parse_simple_mob(FILE *mob_f, int i, int nr)
{
  int j, t[10], retval;
  char line[READ_SIZE];

  mob_proto[i].real_abils.str = 11;
  mob_proto[i].real_abils.intel = 11;
  mob_proto[i].real_abils.wis = 11;
  mob_proto[i].real_abils.dex = 11;
  mob_proto[i].real_abils.con = 11;
  mob_proto[i].real_abils.cha = 11;

  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    exit(1);
  }

  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
	  t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    log("SYSERR: Format error in mob #%d, first line after S flag\n"
	"...expecting line of form '# # # #d#+# #d#+#'", nr);
    exit(1);
  }

  GET_LEVEL(mob_proto + i) = t[0];
  GET_HITROLL(mob_proto + i) = t[1];
  GET_AC(mob_proto + i) = t[2];

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  GET_MAX_HIT(mob_proto + i) = 0;
  GET_HIT(mob_proto + i) = t[3];
  GET_MANA(mob_proto + i) = t[4];
  GET_MOVE(mob_proto + i) = t[5];

  GET_MAX_MANA(mob_proto + i) = 10;
  GET_MAX_MOVE(mob_proto + i) = 50;

  mob_proto[i].mob_specials.damnodice = t[6];
  mob_proto[i].mob_specials.damsizedice = t[7];
  GET_DAMROLL(mob_proto + i) = t[8];

  if (!get_line(mob_f, line)) {
      log("SYSERR: Format error in mob #%d, second line after S flag\n"
	  "...expecting line of form '# #', but file ended!", nr);
      exit(1);
    }

  if (sscanf(line, " %d %d ", t, t + 1) != 2) {
    log("SYSERR: Format error in mob #%d, second line after S flag\n"
	"...expecting line of form '# #'", nr);
    exit(1);
  }

  GET_GOLD(mob_proto + i) = t[0];
  GET_EXP(mob_proto + i) = t[1];

  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in last line of mob #%d\n"
	"...expecting line of form '# # #', but file ended!", nr);
    exit(1);
  }

  if ((retval = sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3)) != 4) {
    if (retval == 3)
      t[3] = 0;
    else {
    log("SYSERR: Format error in last line of mob #%d\n"
	"...expecting line of form '# # # #'", nr);
    exit(1);
    }
  }

  GET_POS(mob_proto + i) = t[0];
  GET_DEFAULT_POS(mob_proto + i) = t[1];
  GET_SEX(mob_proto + i) = t[2];
  mob_proto[i].player.size = t[3];
  GET_CLASS(mob_proto + i) = 0;
  GET_RACE(mob_proto + i) = 0;
  GET_WEIGHT(mob_proto + i) = 200;
  GET_HEIGHT(mob_proto + i) = 198;

  /*
   * these are now save applies; base save numbers for MOBs are now from
   * the warrior save table.
   */
  for (j = 0; j < 5; j++)
    GET_SAVE(mob_proto + i, j) = 0;
}


/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 *
 * CASE		: Requires a parameter through 'value'.
 * BOOL_CASE	: Being specified at all is its value.
 */

#define CASE(test)	\
	if (value && !matched && !str_cmp(keyword, test) && (matched = TRUE))

#define BOOL_CASE(test)	\
	if (!value && !matched && !str_cmp(keyword, test) && (matched = TRUE))

#define RANGE(low, high)	\
	(num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(const char *keyword, const char *value, int i, int nr)
{
  int num_arg = 0, matched = FALSE; 
  char *value2;
  
  /*
   * If there isn't a colon, there is no value.  While Boolean options are
   * possible, we don't actually have any.  Feel free to make some.
  */
  if (value) {
    num_arg = atoi(value);         
    (const char *)value2;
     value2 = value;
  }

  CASE("Aff2") {    
    AFF2_FLAGS(mob_proto + i) = asciiflaglong_conv(value2);
  }  
  CASE("BareHandAttack") {
    RANGE(0, 99);
    mob_proto[i].mob_specials.attack_type = num_arg;
  }
  
  CASE("Numattacks") {
    RANGE(0, 10);
    mob_proto[i].mob_specials.num_attacks = num_arg;
  }
  CASE("Difficulty") {
    RANGE(0, 6);
    GET_DIFFICULTY(mob_proto + i) = num_arg;
  }
  CASE("Race") {
    RANGE(0, 5);
    GET_RACE(mob_proto + i) = num_arg;
  }
  
  CASE("Class") {
    RANGE(0, NUM_MOB_CLASSES);
    GET_CLASS(mob_proto + i) = num_arg;
  }
  
  CASE("Str") {
    RANGE(3, 25);
    mob_proto[i].real_abils.str = num_arg;
  }

  CASE("StrAdd") {
    RANGE(0, 100);
    mob_proto[i].real_abils.str_add = num_arg;    
  }

  CASE("Int") {
    RANGE(3, 25);
    mob_proto[i].real_abils.intel = num_arg;
  }

  CASE("Wis") {
    RANGE(3, 25);
    mob_proto[i].real_abils.wis = num_arg;
  }

  CASE("Dex") {
    RANGE(3, 25);
    mob_proto[i].real_abils.dex = num_arg;
  }

  CASE("Con") {
    RANGE(3, 25);
    mob_proto[i].real_abils.con = num_arg;
  }

  CASE("Cha") {
    RANGE(3, 25);
    mob_proto[i].real_abils.cha = num_arg;
  } 
//clan system dan
  CASE("Clan") {
    GET_CLAN(mob_proto + i) = num_arg;
  }

 
  /*resistance   Anubis*/
  CASE("Res0") {
    RANGE(0, 1);
    mob_proto[i].char_specials.resist[0] = num_arg;
  } 
  
  CASE("Res1") {
    RANGE(0, 1);
    mob_proto[i].char_specials.resist[1] = num_arg;
  } 
  CASE("Res2") {
    RANGE(0, 1);
    mob_proto[i].char_specials.resist[2] = num_arg;
  } 
  
  CASE("Res3") {
    RANGE(0, 1);
    mob_proto[i].char_specials.resist[3] = num_arg;
  }  
  
  CASE("Res4") {
    RANGE(0, 1);
    mob_proto[i].char_specials.resist[4] = num_arg;
  } 
  
  CASE("Res5") {
    RANGE(0, 1);
    mob_proto[i].char_specials.resist[5] = num_arg;
  } 
  CASE("Res6") {
    RANGE(0, 1);
    mob_proto[i].char_specials.resist[6] = num_arg;
  } 
  
  CASE("Res7") {
    RANGE(0, 1);
    mob_proto[i].char_specials.resist[7] = num_arg;
  } 
  
  CASE("Res8") {
    RANGE(0, 1);
    mob_proto[i].char_specials.resist[8] = num_arg;
  } 
  /* Vulnerabilities   Anubis */
  CASE("Vul0") {
    RANGE(0, 1);
    mob_proto[i].char_specials.vulnerable[0] = num_arg;
  } 
  
  CASE("Vul1") {
    RANGE(0, 1);
    mob_proto[i].char_specials.vulnerable[1] = num_arg;
  } 
  CASE("Vul2") {
    RANGE(0, 1);
    mob_proto[i].char_specials.vulnerable[2] = num_arg;
  } 
  
  CASE("Vul3") {
    RANGE(0, 1);
    mob_proto[i].char_specials.vulnerable[3] = num_arg;
  }  
  
  CASE("Vul4") {
    RANGE(0, 1);
    mob_proto[i].char_specials.vulnerable[4] = num_arg;
  } 
  
  CASE("Vul5") {
    RANGE(0, 1);
    mob_proto[i].char_specials.vulnerable[5] = num_arg;
  } 
  CASE("Vul6") {
    RANGE(0, 1);
    mob_proto[i].char_specials.vulnerable[6] = num_arg;
  } 
  
  CASE("Vul7") {
    RANGE(0, 1);
    mob_proto[i].char_specials.vulnerable[7] = num_arg;
  } 
  
  CASE("Vul8") {
    RANGE(0, 1);
    mob_proto[i].char_specials.vulnerable[8] = num_arg;
  } 

  /* Immunities   Anubis */
  CASE("Imm0") {
    RANGE(0, 1);
    mob_proto[i].char_specials.immune[0] = num_arg;
  } 
  
  CASE("Imm1") {
    RANGE(0, 1);
    mob_proto[i].char_specials.immune[1] = num_arg;
  } 
  CASE("Imm2") {
    RANGE(0, 1);
    mob_proto[i].char_specials.immune[2] = num_arg;
  } 
  
  CASE("Imm3") {
    RANGE(0, 1);
    mob_proto[i].char_specials.immune[3] = num_arg;
  }  
  
  CASE("Imm4") {
    RANGE(0, 1);
    mob_proto[i].char_specials.immune[4] = num_arg;
  } 
  
  CASE("Imm5") {
    RANGE(0, 1);
    mob_proto[i].char_specials.immune[5] = num_arg;
  } 
  CASE("Imm6") {
    RANGE(0, 1);
    mob_proto[i].char_specials.immune[6] = num_arg;
  } 
  
  CASE("Imm7") {
    RANGE(0, 1);
    mob_proto[i].char_specials.immune[7] = num_arg;
  } 
  
  CASE("Imm8") {
    RANGE(0, 1);
    mob_proto[i].char_specials.immune[8] = num_arg;
  } 

  if (!matched) {
    log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
	    keyword, nr);
  }    
}

#undef CASE
#undef BOOL_CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  }
  interpret_espec(buf, ptr, i, nr);
}


void parse_enhanced_mob(FILE *mob_f, int i, int nr)
{
  char line[READ_SIZE];

  parse_simple_mob(mob_f, i, nr);

  while (get_line(mob_f, line)) {
    if (!strcmp(line, "E"))	/* end of the enhanced section */
      return;
    else if (*line == '#') {	/* we've hit the next mob, maybe? */
      log("SYSERR: Unterminated E section in mob #%d", nr);
      exit(1);
    } else
      parse_espec(line, i, nr);
  }

  log("SYSERR: Unexpected end of file reached after mob #%d", nr);
  exit(1);
}


void parse_mobile(FILE *mob_f, int nr)
{
  static int i = 0;
  int j, t[10];
  char line[READ_SIZE], *tmpptr, letter;
  char f1[128], f2[128], buf2[128];

  mob_index[i].vnum = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  /*
   * Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg 2/25/98
   */
  mob_proto[i].player_specials = &dummy_mob;
  sprintf(buf2, "mob vnum %d", nr);	/* sprintf: OK (for 'buf2 >= 19') */

  /***** String data *****/
  mob_proto[i].player.name = fread_string(mob_f, buf2);
  tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
  mob_proto[i].player.description = fread_string(mob_f, buf2);
  GET_TITLE(mob_proto + i) = NULL;

  /* *** Numeric data *** */
  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}', but file ended!", nr);
    exit(1);
  }

#ifdef CIRCLE_ACORN	/* Ugh. */
  if (sscanf(line, "%s %s %d %s", f1, f2, t + 2, &letter) != 4) {
#else
  if (sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter) != 4) {
#endif
    log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}'", nr);
    exit(1);
  }

  MOB_FLAGS(mob_proto + i) = asciiflaglong_conv(f1);
  SET_BIT(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
  if (MOB_FLAGGED(mob_proto + i, MOB_NOTDEADYET)) {
    /* Rather bad to load mobiles with this bit already set. */
    log("SYSERR: Mob #%d has reserved bit MOB_NOTDEADYET set.", nr);
    REMOVE_BIT(MOB_FLAGS(mob_proto + i), MOB_NOTDEADYET);
  }
  check_bitvector_names(MOB_FLAGS(mob_proto + i), action_bits_count, buf2, "mobile");

  AFF_FLAGS(mob_proto + i) = asciiflaglong_conv(f2);
  check_bitvector_names(AFF_FLAGS(mob_proto + i), affected_bits_count, buf2, "mobile affect");  

  GET_ALIGNMENT(mob_proto + i) = t[2];

  /* AGGR_TO_ALIGN is ignored if the mob is AGGRESSIVE. */
  if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL))
    log("SYSERR: Mob #%d both Aggressive and Aggressive_to_Alignment.", nr);

  switch (UPPER(letter)) {
  case 'S':	/* Simple monsters */
    parse_simple_mob(mob_f, i, nr);
    break;
  case 'E':	/* Circle3 Enhanced monsters */
    parse_enhanced_mob(mob_f, i, nr);
    break;
  /* add new mob types here.. */
  default:
    log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
    exit(1);
  }

  /* DG triggers -- script info follows mob S/E section */
  letter = fread_letter(mob_f);
  ungetc(letter, mob_f);
  while (letter=='T') {
    dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
  }

  mob_proto[i].aff_abils = mob_proto[i].real_abils;

  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  top_of_mobt = i++;
}




/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE *obj_f, int nr)
{
  static int i = 0;
  static char line[READ_SIZE];
  int t[10], j, retval;
  char *tmpptr;
  char f1[READ_SIZE], f2[READ_SIZE], buf2[128], f3[READ_SIZE];
  struct extra_descr_data *new_descr;
  char aff[READ_SIZE];
  obj_index[i].vnum = nr;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;

  clear_object(obj_proto + i);
  obj_proto[i].item_number = i;

  sprintf(buf2, "object #%d", nr);	/* sprintf: OK (for 'buf2 >= 19') */

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    log("SYSERR: Null obj name or format error at or near %s", buf2);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    CAP(tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, " %d %s %s %s %s", t, f1, f2, aff, f3)) != 5) {
    if (retval == 3)
      t[3] = 0;
    else
    if (retval == 4)
      *f3 = 0;
    else {
      log("SYSERR: Format error in first numeric line (expecting 4 args, got %d), %s", retval, buf2);
    exit(1);
  }
  }

  /* Object flags checked in check_object(). */
  GET_OBJ_TYPE(obj_proto + i) = t[0];
  GET_OBJ_EXTRA(obj_proto + i) = asciiflaglong_conv(f1);
  GET_OBJ_WEAR(obj_proto + i) = asciiflag_conv(f2);
  GET_OBJ_CLASS(obj_proto + i) = asciiflaglong_conv(f3);
  GET_OBJ_PERM(obj_proto + i) = asciiflaglong_conv(aff);

  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4)) != 5) {
     if (retval == 4)
       t[4] = 0;
     else {
    log("SYSERR: Format error in second numeric line (expecting 5 args, got %d), %s", retval, buf2);
    exit(1);
      }
  }
  GET_OBJ_VAL(obj_proto + i, 0) = t[0];
  GET_OBJ_VAL(obj_proto + i, 1) = t[1];
  GET_OBJ_VAL(obj_proto + i, 2) = t[2];
  GET_OBJ_VAL(obj_proto + i, 3) = t[3];
  GET_OBJ_SIZE(obj_proto + i) = t[4];

  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4)) != 5) {
    if (retval == 3)
      t[3] = 0;
    else if (retval == 4)
      t[4] = 42;
    else {
      log("SYSERR: Format error in third numeric line (expecting 4 args, got %d), %s", retval, buf2);
    exit(1);
  }
  }
  GET_OBJ_WEIGHT(obj_proto + i) = t[0];
  GET_OBJ_COST(obj_proto + i) = t[1];
  GET_OBJ_RENT(obj_proto + i) = t[2];
  GET_OBJ_LEVEL(obj_proto + i) = t[3];
  GET_OBJ_MAX_LEVEL(obj_proto + i) = t[4];
  /* check to make sure that weight of containers exceeds curr. quantity */
  if (GET_OBJ_TYPE(obj_proto + i) == ITEM_DRINKCON ||
      GET_OBJ_TYPE(obj_proto + i) == ITEM_FOUNTAIN) {
    if (GET_OBJ_WEIGHT(obj_proto + i) < GET_OBJ_VAL(obj_proto + i, 1))
      GET_OBJ_WEIGHT(obj_proto + i) = GET_OBJ_VAL(obj_proto + i, 1) + 5;
  }

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
  }

  strcat(buf2, ", after numeric constants\n"	/* strcat: OK (for 'buf2 >= 87') */
	 "...expecting 'E', 'A', '$', or next object number");
  j = 0;

  for (;;) {
    if (!get_line(obj_f, line)) {
      log("SYSERR: Format error in %s", buf2);
      exit(1);
    }
    switch (*line) {
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      new_descr->next = obj_proto[i].ex_description;
      obj_proto[i].ex_description = new_descr;
      break;
    case 'A':
      if (j >= MAX_OBJ_AFFECT) {
	log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
	exit(1);
      }
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }

      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
	log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric arguments, got %d\n"
	    "...offending line: '%s'", buf2, retval, line);
	exit(1);
      }
      obj_proto[i].affected[j].location = t[0];
      obj_proto[i].affected[j].modifier = t[1];
      j++;
      break;
    case 'R':
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'R' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }
      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
	log("SYSERR: Format error in 'R' field, %s\n"
	    "...expecting 2 numeric arguments, got %d\n"
	    "...offending line: '%s'", buf2, retval, line);
	exit(1);
    }
      obj_proto[i].resist[t[0]] = t[1];
      break;
    case 'I':
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'I' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }
      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
	log("SYSERR: Format error in 'I' field, %s\n"
	    "...expecting 2 numeric arguments, got %d\n"
	    "...offending line: '%s'", buf2, retval, line);
	exit(1);
    }
      obj_proto[i].immune[t[0]] = t[1];
      break;
    case 'V':
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'V' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }
      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
	log("SYSERR: Format error in 'V' field, %s\n"
	    "...expecting 2 numeric arguments, got %d\n"
	    "...offending line: '%s'", buf2, retval, line);
	exit(1);
    }
      obj_proto[i].vulnerable[t[0]] = t[1];
      break;
    case 'T':  /* DG triggers */
      dg_obj_trigger(line, &obj_proto[i]);
      break;
    case '$':
    case '#':
      top_of_objt = i;
      check_object(obj_proto + i);
      i++;
      return (line);
    default:
      log("SYSERR: Format error in (%c): %s", *line, buf2);
      exit(1);
    }
  }
}


#define Z	zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE *fl, char *zonename)
{
  static zone_rnum zone = 0;
  int cmd_no = 0, num_of_cmds = 0, line_num = 0, tmp, error, arg_num;
  char *ptr, buf[READ_SIZE], zname[READ_SIZE], buf2[MAX_STRING_LENGTH];
/*  char t1[80], t2[80]; */

  strlcpy(zname, zonename, sizeof(zname));

  /* Skip first 3 lines lest we mistake the zone name for a command. */
  for (tmp = 0; tmp < 3; tmp++)
    get_line(fl, buf);

  /*  More accurate count. Previous was always 4 or 5 too high. -gg 2001/1/17
   *  Note that if a new zone command is added to reset_zone(), this string
   *  will need to be updated to suit. - ae.
   */
  while (get_line(fl, buf))
    if ((strchr("MOPGERDTV", buf[0]) && buf[1] == ' ') || (buf[0] == 'S' && buf[1] == '\0'))
      num_of_cmds++;

  rewind(fl);

  if (num_of_cmds == 0) {
    log("SYSERR: %s is empty!", zname);
    exit(1);
  } else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  if (sscanf(buf, "#%hd", &Z.number) != 1) {
    log("SYSERR: Format error in %s, line %d", zname, line_num);
    exit(1);
  }
  snprintf(buf2, sizeof(buf2), "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = strdup(buf);

  line_num += get_line(fl, buf);
  if (sscanf(buf, " %hd %hd %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4) {
    log("SYSERR: Format error in numeric constant line of %s", zname);
    exit(1);
  }
  if (Z.bot > Z.top) {
    log("SYSERR: Zone %d bottom (%d) > top (%d).", Z.number, Z.bot, Z.top);
    exit(1);
  }

  cmd_no = 0;

  for (;;) {
    if ((tmp = get_line(fl, buf)) == 0) {
      log("SYSERR: Format error in %s - premature end of file", zname);
      exit(1);
    }
    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;

    ptr++;

    if (ZCMD.command == 'S' || ZCMD.command == '$') {
      ZCMD.command = 'S';
      break;
    }
    error = 0;
     if (strchr("D", ZCMD.command) != NULL) { /* ### */
        if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                  &ZCMD.arg3) != 4)
         error = 1;
      }
     else if (strchr("R", ZCMD.command) != NULL) { /* ### */
       if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1,
            &ZCMD.arg2) != 3)
         error = 1;
     }
     else if (strchr("G", ZCMD.command) != NULL) { /* ### */
       if ((arg_num = sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1,
            &ZCMD.arg2, &ZCMD.arg3)) != 4) {
         if (arg_num != 3)
           error = 1;
         else
           ZCMD.arg3 = 0;
       }
     }
     else { /* ### */
       if ((arg_num = sscanf(ptr, " %d %d %d %d %d ", &tmp, &ZCMD.arg1,
            &ZCMD.arg2, &ZCMD.arg3, &ZCMD.arg4)) != 5) {
         if (arg_num != 4)
           error = 1;
         else
           ZCMD.arg4 = 0;
       }
     }

    ZCMD.if_flag = tmp;

    if (error) {
      log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
      exit(1);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }

  if (num_of_cmds != cmd_no + 1) {
    log("SYSERR: Zone command count mismatch for %s. Estimated: %d, Actual: %d", zname, num_of_cmds, cmd_no + 1);
    exit(1);
  }

  top_of_zone_table = zone++;
}

#undef Z


void get_one_line(FILE *fl, char *buf)
{
  if (fgets(buf, READ_SIZE, fl) == NULL) {
    log("SYSERR: error reading help file: not terminated with $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}


void free_help(void)
{
  int hp;

  if (!help_table)
     return;

  for (hp = 0; hp <= top_of_helpt; hp++) {
    if (help_table[hp].keyword)
      free(help_table[hp].keyword);
    if (help_table[hp].entry && !help_table[hp].duplicate)
      free(help_table[hp].entry);
  }

  free(help_table);
  help_table = NULL;
  top_of_helpt = 0;
}

void free_zone_list(void)
{
  int zl;

  if (!zone_info_table)
     return;

  for (zl = 0; zl <= top_of_zonet; zl++) {
    if (zone_info_table[zl].num)
      free(zone_info_table[zl].num);
    if (zone_info_table[zl].desc && !zone_info_table[zl].duplicate)
      free(zone_info_table[zl].desc);
  }

  free(zone_info_table);
  zone_info_table = NULL;
  top_of_zonet = 0;
}

void load_help(FILE *fl)
{
#if defined(CIRCLE_MACINTOSH)
  static char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384]; /* too big for stack? */
#else
  char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
#endif
  size_t entrylen;
  char line[READ_SIZE + 1], *scan;
  struct help_index_element el;

  /* get the first keyword line */
  get_one_line(fl, key);
  while (*key != '$') {
    strcat(key, "\r\n");	/* strcat: OK (READ_SIZE - "\n" + "\r\n" == READ_SIZE + 1) */
    entrylen = strlcpy(entry, key, sizeof(entry));

    /* read in the corresponding help entry */
    get_one_line(fl, line);
    while (*line != '#' && entrylen < sizeof(entry) - 1) {
      entrylen += strlcpy(entry + entrylen, line, sizeof(entry) - entrylen);

      if (entrylen + 2 < sizeof(entry) - 1) {
        strcpy(entry + entrylen, "\r\n");	/* strcpy: OK (size checked above) */
        entrylen += 2;
      }
      get_one_line(fl, line);
    }

    if (entrylen >= sizeof(entry) - 1) {
      int keysize;
      const char *truncmsg = "\r\n*TRUNCATED*\r\n";

      strcpy(entry + sizeof(entry) - strlen(truncmsg) - 1, truncmsg);	/* strcpy: OK (assuming sane 'entry' size) */

      keysize = strlen(key) - 2;
      log("SYSERR: Help entry exceeded buffer space: %.*s", keysize, key);

      /* If we ran out of buffer space, eat the rest of the entry. */
      while (*line != '#')
        get_one_line(fl, line);
    }

    /* now, add the entry to the index with each keyword on the keyword line */
    el.duplicate = 0;
    el.entry = strdup(entry);
    scan = one_word(key, next_key);
    while (*next_key) {
      el.keyword = strdup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }

    /* get next keyword line (or $) */
    get_one_line(fl, key);
  }
}


int hsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *) a;
  b1 = (const struct help_index_element *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}

int zsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *) a;
  b1 = (const struct help_index_element *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}


void load_zonelist(FILE *fl)
{
#if defined(CIRCLE_MACINTOSH)
  static char num[READ_SIZE + 1], next_num[READ_SIZE + 1], desc[32384]; /* too big for stack? */
#else
  char num[READ_SIZE + 1], next_num[READ_SIZE + 1], desc[32384];
#endif
  size_t desclen;
  char line[READ_SIZE + 1], *scan;
  struct zone_list zl;

  /* get the first num line */
  get_one_line(fl, num);
  while (*num != '$') {
    strcat(num, "\r\n");	/* strcat: OK (READ_SIZE - "\n" + "\r\n" == READ_SIZE + 1) */
    desclen = strlcpy(desc, num, sizeof(desc));

    /* read in the corresponding help desc */
    get_one_line(fl, line);
    while (*line != '#' && desclen < sizeof(desc) - 1) {
      desclen += strlcpy(desc + desclen, line, sizeof(desc) - desclen);

      if (desclen + 2 < sizeof(desc) - 1) {
        strcpy(desc + desclen, "\r\n");	/* strcpy: OK (size checked above) */
        desclen += 2;
      }
      get_one_line(fl, line);
    }

    if (desclen >= sizeof(desc) - 1) {
      int numsize;
      const char *truncmsg = "\r\n*TRUNCATED*\r\n";

      strcpy(desc + sizeof(desc) - strlen(truncmsg) - 1, truncmsg);	/* strcpy: OK (assuming sane 'desc' size) */

      numsize = strlen(num) - 2;
      log("SYSERR: Help desc exceeded buffer space: %.*s", numsize, num);

      /* If we ran out of buffer space, eat the rest of the desc. */
      while (*line != '#')
        get_one_line(fl, line);
    }

    /* now, add the desc to the index with each num on the num line */
    zl.duplicate = 0;
    zl.desc = strdup(desc);
    scan = one_word(num, next_num);
    while (*next_num) {
      zl.num = strdup(next_num);
      zone_info_table[top_of_zonet++] = zl;
      zl.duplicate++;
      scan = one_word(scan, next_num);
    }

    /* get next num line (or $) */
    get_one_line(fl, num);
  }
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/


int vnum_mobile(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_mobt; nr++)
    if (isname(searchname, mob_proto[nr].player.name))
      send_to_char(ch, "%3d. [%5d] %s\r\n", ++found, mob_index[nr].vnum, mob_proto[nr].player.short_descr);

  return (found);
}



int vnum_object(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
    if (isname(searchname, obj_proto[nr].name))
      send_to_char(ch, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].vnum, obj_proto[nr].short_description);

  return (found);
}

int vnum_weapon(int attacktype, struct char_data * ch)
{
  int nr, found = 0;

  char buf[MAX_INPUT_LENGTH];

  for (nr = 0; nr <= top_of_objt; nr++) {
    if (obj_proto[nr].obj_flags.type_flag == ITEM_WEAPON && obj_proto[nr].obj_flags.value[3] == attacktype) {
      sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
	      obj_index[nr].vnum,
	      obj_proto[nr].short_description);
      send_to_char(ch, buf);
    }
  }
  return (found);
}



/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;
  GET_ID(ch) = max_mob_id++;

  return (ch);
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
  mob_rnum i;
  struct char_data *mob;

  if (type == VIRTUAL) {
    if ((i = real_mobile(nr)) == NOBODY) {
      log("WARNING: Mobile vnum %d does not exist in database.", nr);
      return (NULL);
    }
  } else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;

  if (!mob->points.max_hit) {
    mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
      mob->points.move;
  } else
    mob->points.max_hit = rand_number(mob->points.hit, mob->points.mana);

  mob->points.hit = mob->points.max_hit;
  mob->points.mana = mob->points.max_mana;
  mob->points.move = mob->points.max_move;

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);

  mob_index[i].number++;

  GET_ID(mob) = max_mob_id++;
  assign_triggers(mob, MOB_TRIGGER);

  return (mob);
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;

  GET_ID(obj) = max_obj_id++;
  assign_triggers(obj, OBJ_TRIGGER);

  return (obj);
}


/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
  struct obj_data *obj;
  obj_rnum i = type == VIRTUAL ? real_object(nr) : nr;

  if (i == NOTHING || i > top_of_objt) {
    log("Object (%c) %d does not exist in database.", type == VIRTUAL ? 'V' : 'R', nr);
    return (NULL);
  }

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  obj_index[i].number++;

  GET_ID(obj) = max_obj_id++;
  assign_triggers(obj, OBJ_TRIGGER);

  return (obj);
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
	  zone_table[i].reset_mode)
	(zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan &&
	  zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
	/* enqueue zone */

	CREATE(update_u, struct reset_q_element, 1);

	update_u->zone_to_reset = i;
	update_u->next = 0;

	if (!reset_q.head)
	  reset_q.head = reset_q.tail = update_u;
	else {
	  reset_q.tail->next = update_u;
	  reset_q.tail = update_u;
	}

	zone_table[i].age = ZO_DEAD;
      }
    }
  }	/* end - one minute has passed */


  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
	is_empty(update_u->zone_to_reset)) {
      reset_zone(update_u->zone_to_reset);
      mudlog(CMP, LVL_DEITY, FALSE, "Auto zone reset: %s", zone_table[update_u->zone_to_reset].name);
      /* dequeue */
      if (update_u == reset_q.head)
	reset_q.head = reset_q.head->next;
      else {
	for (temp = reset_q.head; temp->next != update_u;
	     temp = temp->next);

	if (!update_u->next)
	  reset_q.tail = temp;

	temp->next = update_u->next;
      }

      free(update_u);
      break;
    }
}

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
  mudlog(NRM, LVL_DEITY, TRUE, "SYSERR: zone file: %s", message);
  mudlog(NRM, LVL_DEITY, TRUE, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
	ZCMD.command, zone_table[zone].number, ZCMD.line);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
  int cmd_no, last_cmd = 0;
   int mob_load = FALSE; /* ### */
   int obj_load = FALSE; /* ### */
  struct char_data *mob = NULL;
  struct obj_data *obj, *obj_to;
  room_vnum rvnum;
  room_rnum rrnum;
  struct char_data *tmob=NULL; /* for trigger assignment */
  struct obj_data *tobj=NULL;  /* for trigger assignment */

  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
    if (ZCMD.if_flag && !last_cmd && !mob_load && !obj_load)
      continue;

     if (!ZCMD.if_flag) { /* ### */
       mob_load = FALSE;
       obj_load = FALSE;
     }
 

    /*  This is the list of actual zone commands.  If any new
     *  zone commands are added to the game, be certain to update
     *  the list of commands in load_zone() so that the counting
     *  will still be correct. - ae.
     */
    switch (ZCMD.command) {
    case '*':			/* ignore command */
      last_cmd = 0;
      break;

     case 'M':			/* read a mobile ### */
       if ((mob_index[ZCMD.arg1].number < ZCMD.arg2) &&
           (rand_number(1, 100) >= ZCMD.arg4)) {
  	mob = read_mobile(ZCMD.arg1, REAL);
  	char_to_room(mob, ZCMD.arg3);
  	last_cmd = 1;
         mob_load = TRUE;
      } else
	last_cmd = 0;
      tobj = NULL;
      break;

     case 'O':			/* read an object ### */
       if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) &&
           (rand_number(1, 100) >= ZCMD.arg4)) {
  	if (ZCMD.arg3 >= 0) {
  	  obj = read_object(ZCMD.arg1, REAL);
  	  obj_to_room(obj, ZCMD.arg3);
  	  last_cmd = 1;
           obj_load = TRUE;
  	} else {
  	  obj = read_object(ZCMD.arg1, REAL);
  	  obj->in_room = NOWHERE;
  	  last_cmd = 1;
           obj_load = TRUE;
  	}
        } else
  	last_cmd = 0;
        break;

     case 'P':			/* object to object ### */
       if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) &&
           obj_load && (rand_number(1, 100) >= ZCMD.arg4)) {
  	if (!(obj_to = get_obj_num(ZCMD.arg3))) {
  	  ZONE_ERROR("target obj not found");
	  ZCMD.command = '*';
	  break;
	}
  	obj = read_object(ZCMD.arg1, REAL);
	obj_to_obj(obj, obj_to);
	last_cmd = 1;
        load_otrigger(obj);
        tobj = obj;
      } else
	last_cmd = 0;
      tmob = NULL;
      break;

     case 'G':			/* obj_to_char ### */
        if (!mob) {
  	ZONE_ERROR("attempt to give obj to non-existant mob");
  	break;
        }
       if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) &&
           mob_load && (rand_number(1, 100) >= ZCMD.arg3)) {
  	obj = read_object(ZCMD.arg1, REAL);
  	obj_to_char(obj, mob);
        if ( GET_OBJ_TYPE(obj) == ITEM_CONTAINER )
            obj_load = TRUE;
        else
            obj_load = FALSE;
  	last_cmd = 1;
      } else
	last_cmd = 0;
      tmob = NULL;
      break;

     case 'E':			/* object to equipment list ### */
        if (!mob) {
  	ZONE_ERROR("trying to equip non-existant mob");
  	break;
        }
       if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) &&
           mob_load && (rand_number(1, 100) >= ZCMD.arg4)) {
  	if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
  	  ZONE_ERROR("invalid equipment pos number");
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
          IN_ROOM(obj) = IN_ROOM(mob);
          load_otrigger(obj);
          if (wear_otrigger(obj, mob, ZCMD.arg3)) {
            IN_ROOM(obj) = NOWHERE;
	  equip_char(mob, obj, ZCMD.arg3);
          } else
            obj_to_char(obj, mob);
          if ( GET_OBJ_TYPE(obj) == ITEM_CONTAINER )
              obj_load = TRUE;
          else
              obj_load = FALSE;
          tobj = obj;
	  last_cmd = 1;
	}
      } else
	last_cmd = 0;
      tmob = NULL;
      break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL && (rand_number(1, 100) >= ZCMD.arg4) )
        extract_obj(obj);
      last_cmd = 1;
      tmob = NULL;
      tobj = NULL;
      break;


    case 'D':			/* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
	  (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
	ZONE_ERROR("door does not exist, command disabled");
	ZCMD.command = '*';
      } else
	switch (ZCMD.arg3) {
	case 0:
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_CLOSED);
	  break;
	case 1:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  break;
	case 2:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_LOCKED);
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  break;
	}
      last_cmd = 1;
      tmob = NULL;
      tobj = NULL;
      break;

    case 'T': /* trigger command */
      if (ZCMD.arg1==MOB_TRIGGER && tmob) {
        if (!SCRIPT(tmob))
          CREATE(SCRIPT(tmob), struct script_data, 1);
        add_trigger(SCRIPT(tmob), read_trigger(real_trigger(ZCMD.arg2)), -1);
        last_cmd = 1;
      } else if (ZCMD.arg1==OBJ_TRIGGER && tobj) {
        if (!SCRIPT(tobj))
          CREATE(SCRIPT(tobj), struct script_data, 1);
        add_trigger(SCRIPT(tobj), read_trigger(real_trigger(ZCMD.arg2)), -1);
        last_cmd = 1;
      } else if (ZCMD.arg1==WLD_TRIGGER) {
        if (ZCMD.arg3 == NOWHERE || ZCMD.arg3>top_of_world) {
          ZONE_ERROR("Invalid room number in trigger assignment");
        }
        if (!world[ZCMD.arg3].script)
          CREATE(world[ZCMD.arg3].script, struct script_data, 1);
        add_trigger(world[ZCMD.arg3].script, read_trigger(real_trigger(ZCMD.arg2)), -1);
        last_cmd = 1;
      }

      break;

    case 'V':
      if (ZCMD.arg1==MOB_TRIGGER && tmob) {
        if (!SCRIPT(tmob)) {
          ZONE_ERROR("Attempt to give variable to scriptless mobile");
        } else
          add_var(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                  ZCMD.arg3);
        last_cmd = 1;
      } else if (ZCMD.arg1==OBJ_TRIGGER && tobj) {
        if (!SCRIPT(tobj)) {
          ZONE_ERROR("Attempt to give variable to scriptless object");
        } else
          add_var(&(SCRIPT(tobj)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                  ZCMD.arg3);
        last_cmd = 1;
      } else if (ZCMD.arg1==WLD_TRIGGER) {
        if (ZCMD.arg3 == NOWHERE || ZCMD.arg3>top_of_world) {
          ZONE_ERROR("Invalid room number in variable assignment");
        } else {
          if (!(world[ZCMD.arg3].script)) {
            ZONE_ERROR("Attempt to give variable to scriptless object");
          } else
            add_var(&(world[ZCMD.arg3].script->global_vars),
                    ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg2);
          last_cmd = 1;
        }
      }
      break;

   	default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD.command = '*';
      break;
    }
  }

  zone_table[zone].age = 0;

  /* handle reset_wtrigger's */
  rvnum = zone_table[zone].bot;
  while (rvnum <= zone_table[zone].top) {
    rrnum = real_room(rvnum);
    if (rrnum != NOWHERE) reset_wtrigger(&world[rrnum]);
    rvnum++;
  }
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;
    if (IN_ROOM(i->character) == NOWHERE)
      continue;
    if (world[IN_ROOM(i->character)].zone != zone_nr)
      continue;
    /*
     * if an immortal has nohassle off, he counts as present 
     * added for testing zone reset triggers - Welcor 
     */
    if ((GET_LEVEL(i->character) >= LVL_SAINT) && (PRF_FLAGGED(i->character, PRF_NOHASSLE)))
      continue;

    return (0);
  }

  return (1);
}





/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/


long get_ptable_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (i);

  return (-1);
}


long get_id_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (player_table[i].id);

  return (-1);
}


char *get_name_by_id(long id)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == id)
      return (player_table[i].name);

  return (NULL);
}

/* ziz */
 #define NUM_OF_SAVE_THROWS 5

 /* new load_char reads ascii pfiles */
  /* Load a char, TRUE if loaded, FALSE if not */
 int load_char(char *name, struct char_data *ch)
  {
   int id, num = 0, num2 = 0, num3 = 0, num4 = 0, i;
   bitvector_t num5 = 0;
   FBFILE *fl;
   char filename[40];
   char buf[128], line[MAX_INPUT_LENGTH + 1], tag[6];
   struct affected_type af;

   if((id = get_ptable_by_name(name)) < 0)
      return (-1);
   else {
     sprintf(filename, "%s%s%c%s%s%s", PLR_PREFIX, SLASH, *player_table[id].name,
       SLASH, player_table[id].name, PLR_SUFFIX);
     if(!(fl = fbopen(filename, FB_READ))) {
       mudlog(NRM, LVL_DEITY, TRUE, "SYSERR: Couldn't open player file %s", filename);
       return (-1);
     }

     /* character initializations */
     /* initializations necessary to keep some things straight */
     ch->affected = NULL;
     ch->affected2 = NULL;
     for(i = 1; i <= MAX_SKILLS; i++)
     GET_SKILL(ch, i) = 0;
     GET_SEX(ch) = PFDEF_SEX;
     GET_CLASS(ch) = PFDEF_CLASS;
     GET_RACE(ch) = PFDEF_RACE;
     GET_LEVEL(ch) = PFDEF_LEVEL;
     GET_SIZE(ch)  = PFDEF_SIZE;
     GET_HOME(ch) = PFDEF_HOMETOWN;
     GET_HEIGHT(ch) = PFDEF_HEIGHT;
     GET_WEIGHT(ch) = PFDEF_WEIGHT;
     GET_ALIGNMENT(ch) = PFDEF_ALIGNMENT;
     PLR_FLAGS(ch) = PFDEF_PLRFLAGS;
     AFF_FLAGS(ch) = PFDEF_AFFFLAGS;
     AFF2_FLAGS(ch) = PFDEF_AFFFLAGS;
     for(i = 0; i < NUM_OF_SAVE_THROWS; i++)
       GET_SAVE(ch, i) = PFDEF_SAVETHROW;
     GET_LOADROOM(ch) = PFDEF_LOADROOM;
     GET_INVIS_LEV(ch) = PFDEF_INVISLEV;
     GET_FREEZE_LEV(ch) = PFDEF_FREEZELEV;
     GET_WIMP_LEV(ch) = PFDEF_WIMPLEV;
     GET_COND(ch, FULL) = PFDEF_HUNGER;
     GET_COND(ch, THIRST) = PFDEF_THIRST;
     GET_COND(ch, DRUNK) = PFDEF_DRUNK;
     GET_BAD_PWS(ch) = PFDEF_BADPWS;
     PRF_FLAGS(ch) = PFDEF_PREFFLAGS;
     GET_PRACTICES(ch) = PFDEF_PRACTICES;
     GET_GOLD(ch) = PFDEF_GOLD;
     GET_BANK_GOLD(ch) = PFDEF_BANK;
     GET_EXP(ch) = PFDEF_EXP;
     GET_HITROLL(ch) = PFDEF_HITROLL;
     GET_DAMROLL(ch) = PFDEF_DAMROLL;
     GET_AC(ch) = PFDEF_AC;
     ch->real_abils.str = PFDEF_STR;
     ch->real_abils.str_add = PFDEF_STRADD;
     ch->real_abils.dex = PFDEF_DEX;
     ch->real_abils.intel = PFDEF_INT;
     ch->real_abils.wis = PFDEF_WIS;
     ch->real_abils.con = PFDEF_CON;
     ch->real_abils.cha = PFDEF_CHA;
     GET_HIT(ch) = PFDEF_HIT;
     GET_MAX_HIT(ch) = PFDEF_MAXHIT;
     GET_MANA(ch) = PFDEF_MANA;
     GET_MAX_MANA(ch) = PFDEF_MAXMANA;
     GET_MOVE(ch) = PFDEF_MOVE;
     GET_MAX_MOVE(ch) = PFDEF_MAXMOVE;
     GET_CLAN(ch) = PFDEF_CLAN;
     GET_CLAN_RANK(ch) = PFDEF_CLANRANK;
     ch->player_specials->saved.olc_zone = 0;
     

     while(fbgetline(fl, line)) {
       tag_argument(line, tag);
       num = atoi(line);

       switch (*tag) {
       case 'A':
       if(!strcmp(tag, "Ac  "))
         GET_AC(ch) = num;
       else if(!strcmp(tag, "Act "))
         PLR_FLAGS(ch) = num;
        else if(!strcmp(tag, "Af2 "))
         AFF2_FLAGS(ch) = asciiflag_conv(line);
       else if(!strcmp(tag, "Afs2")) {
         i = 0;
         do {
           fbgetline(fl, line);
           sscanf(line, "%d %d %d %d %lld", &num, &num2, &num3, &num4, &num5);
           if(num > 0) {
             af.type = num;
             af.duration = num2;
             af.modifier = num3;
             af.location = num4;
             af.bitvector = num5;             
             affect2_to_char(ch, &af);
             i++;
           }
         } while (num != 0);
       } 
       else if(!strcmp(tag, "Aff "))
         AFF_FLAGS(ch) = asciiflag_conv(line);      
       else if(!strcmp(tag, "Affs")) {
         i = 0;
         do {
           fbgetline(fl, line);
           sscanf(line, "%d %d %d %d %lld", &num, &num2, &num3, &num4, &num5);
           if(num > 0) {
             af.type = num;
             af.duration = num2;
             af.modifier = num3;
             af.location = num4;
             af.bitvector = num5;             
             affect_to_char(ch, &af);
             i++;
           }
         } while (num != 0);
       }             
       else if(!strcmp(tag, "Alin"))
         GET_ALIGNMENT(ch) = num;
       break;

       case 'B':
       if(!strcmp(tag, "Badp"))
         GET_BAD_PWS(ch) = num;
       else if(!strcmp(tag, "Bank"))
         GET_BANK_GOLD(ch) = num;
       else if(!strcmp(tag, "Brth"))
         ch->player.time.birth = num;
       break;

       case 'C':
       if(!strcmp(tag, "Cha "))
         ch->real_abils.cha = num;
       else if(!strcmp(tag, "Clas"))
         GET_CLASS(ch) = num;
       else if(!strcmp(tag, "Con "))
         ch->real_abils.con = num;
        else if(!strcmp(tag, "Clan"))
          GET_CLAN(ch) = num;
       break;

       case 'D':
       if(!strcmp(tag, "Desc")) {
         ch->player.description = fbgetstring(fl);
       } else if(!strcmp(tag, "Dex "))
         ch->real_abils.dex = num;
       else if(!strcmp(tag, "Drnk"))
         GET_COND(ch, DRUNK) = num;
       else if(!strcmp(tag, "Drol"))
         GET_DAMROLL(ch) = num;
       break;

       case 'E':
       if(!strcmp(tag, "Exp "))
         GET_EXP(ch) = num;
       break;

       case 'F':
       if(!strcmp(tag, "Frez"))
         GET_FREEZE_LEV(ch) = num;
       break;

       case 'G':
       if(!strcmp(tag, "Gold"))
         GET_GOLD(ch) = num;
       break;

       case 'H':
       if(!strcmp(tag, "Hit ")) {
         sscanf(line, "%d/%d", &num, &num2);
         GET_HIT(ch) = num;
         GET_MAX_HIT(ch) = num2;
       } else if(!strcmp(tag, "Hite"))
         GET_HEIGHT(ch) = num;
       else if(!strcmp(tag, "Home"))
         GET_HOME(ch) = num;
       else if(!strcmp(tag, "Host"))
         ch->player_specials->host = strdup(line);

       else if(!strcmp(tag, "Hrol"))
         GET_HITROLL(ch) = num;
       else if(!strcmp(tag, "Hung"))
        GET_COND(ch, FULL) = num;
       break;

       case 'I':
       if(!strcmp(tag, "Id  "))
         GET_IDNUM(ch) = num;
       else if(!strcmp(tag, "Int "))
         ch->real_abils.intel = num;
       else if(!strcmp(tag, "Invs"))
         GET_INVIS_LEV(ch) = num;
       break;

       case 'L':
       if(!strcmp(tag, "Last"))
         ch->player.time.logon = num;
       else if(!strcmp(tag, "Lern"))
         GET_PRACTICES(ch) = num;
       else if(!strcmp(tag, "Levl"))
         GET_LEVEL(ch) = num;
       break;

       case 'M':
       if(!strcmp(tag, "Mana")) {
         sscanf(line, "%d/%d", &num, &num2);
         GET_MANA(ch) = num;
         GET_MAX_MANA(ch) = num2;
       } else if(!strcmp(tag, "Move")) {
         sscanf(line, "%d/%d", &num, &num2);
         GET_MOVE(ch) = num;
         GET_MAX_MOVE(ch) = num2;
       }
       break;

       case 'N':
       if(!strcmp(tag, "Name"))
         GET_PC_NAME(ch) = strdup(line);
       break;       
      
       case 'P':
       if(!strcmp(tag, "Pass"))
         strcpy(GET_PASSWD(ch), line);
       else if(!strcmp(tag, "Plyd"))
         ch->player.time.played = num;
 #ifdef ASCII_SAVE_POOFS
       else if(!strcmp(tag, "PfIn"))
         POOFIN(ch) = strdup(line);
       else if(!strcmp(tag, "PfOt"))
         POOFOUT(ch) = strdup(line);
 #endif
       else if(!strcmp(tag, "Pref"))
         PRF_FLAGS(ch) = asciiflag_conv(line);
       break;

       case 'R':
       if(!strcmp(tag, "Room"))
         GET_LOADROOM(ch) = num;
        else if (!strcmp(tag, "Rank"))
          GET_CLAN_RANK(ch) = num;
       else if(!strcmp(tag, "Race"))
         GET_RACE(ch) = num;
       break;

       case 'S':
       if(!strcmp(tag, "Sex "))
         GET_SEX(ch) = num;
       if(!strcmp(tag, "Size"))
         GET_SIZE(ch) = num;
       else if(!strcmp(tag, "Solc"))
         ch->player_specials->saved.olc_zone = num;
       else if(!strcmp(tag, "Skil")) {
         do {
           fbgetline(fl, line);
           sscanf(line, "%d %d", &num, &num2);
             if(num != 0)
               GET_SKILL(ch, num) = num2;
         } while (num != 0);
       } else if(!strcmp(tag, "Str ")) {
         sscanf(line, "%d/%d", &num, &num2);
         ch->real_abils.str = num;
         ch->real_abils.str_add = num2;
       }
       break;

       case 'T':
       if(!strcmp(tag, "Thir"))
         GET_COND(ch, THIRST) = num;
       else if(!strcmp(tag, "Thr1"))
         GET_SAVE(ch, 0) = num;
       else if(!strcmp(tag, "Thr2"))
         GET_SAVE(ch, 1) = num;
       else if(!strcmp(tag, "Thr3"))
         GET_SAVE(ch, 2) = num;
       else if(!strcmp(tag, "Thr4"))
         GET_SAVE(ch, 3) = num;
       else if(!strcmp(tag, "Thr5"))
         GET_SAVE(ch, 4) = num;
       else if(!strcmp(tag, "Titl"))
         GET_TITLE(ch) = strdup(line);
       break;

       case 'W':
       if(!strcmp(tag, "Wate"))
         GET_WEIGHT(ch) = num;
       else if(!strcmp(tag, "Wimp"))
         GET_WIMP_LEV(ch) = num;
       else if(!strcmp(tag, "Wis "))
         ch->real_abils.wis = num;
       break;

       default:
       sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
       }
     }
    }

   affect_total(ch);

   /* initialization for imms */
   if(GET_LEVEL(ch) >= LVL_SAINT) {
     for(i = 1; i <= MAX_SKILLS; i++)
       GET_SKILL(ch, i) = 100;
     GET_COND(ch, FULL) = -1;
     GET_COND(ch, THIRST) = -1;
     GET_COND(ch, DRUNK) = -1;
   }
   fbclose(fl);
   return(id);
 }

 /* remove ^M's from file output */
 /* There may be a similar function in Oasis (and I'm sure
    it's part of obuild).  Remove this if you get a
    multiple definition error or if it you want to use a
    substitute
 */
 void kill_ems(char *str)
 {
   char *ptr1, *ptr2, *tmp;

   tmp = str;
   ptr1 = str;
   ptr2 = str;

   while(*ptr1) {
     if((*(ptr2++) = *(ptr1++)) == '\r')
       if(*ptr1 == '\r')
       ptr1++;
    }
   *ptr2 = '\0';
 }


 /*
  * write the vital data of a player to the player file
  *
  * NOTE: load_room should be an *RNUM* now.  It is converted to a vnum here.
  */
 /* This is the ascii pfiles save routine */
 void save_char(struct char_data * ch, room_rnum load_room)
  {
   FBFILE *fl;
   char outname[40], bits[127], buf[MAX_STRING_LENGTH];
   int i, id, save_index = FALSE;
   struct affected_type *aff, tmp_aff[MAX_AFFECT], tmp_aff2[MAX_AFFECT];
   struct obj_data *char_eq[NUM_WEARS];

   if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;
   /* This version of save_char allows ch->desc to be null (to allow
      "set file" to use it).  This causes the player's last host
      and probably last login to null out.
   */

   if (!PLR_FLAGGED(ch, PLR_LOADROOM)) {
     if (load_room == NOWHERE)
       GET_LOADROOM(ch) = NOWHERE;
     else
       GET_LOADROOM(ch) = 4121;
    }

   /*strcpy(player.pwd, GET_PASSWD(ch));*/

   {
     for (i = 0;
       (*(bits + i) = LOWER(*(GET_NAME(ch) + i))); i++);
     sprintf(outname, "%s%s%c%s%s%s", PLR_PREFIX, SLASH, *bits,
                       SLASH, bits, PLR_SUFFIX);

     if (!(fl = fbopen(outname, FB_WRITE))) {
       mudlog(NRM, LVL_DEITY, TRUE, "SYSERR: Couldn't open player file %s for write", outname);
       return;
     }

     /* remove affects from eq and spells (from char_to_store) */
     /* Unaffect everything a character can be affected by */

     for (i = 0; i < NUM_WEARS; i++) {
       if (GET_EQ(ch, i))
         char_eq[i] = unequip_char(ch, i);
       else
         char_eq[i] = NULL;
     }

     for (aff = ch->affected, i = 0; i < MAX_AFFECT; i++) {
       if (aff) {
         tmp_aff[i] = *aff;
         tmp_aff[i].next = 0;
         aff = aff->next;
       } else {
         tmp_aff[i].type = 0;  /* Zero signifies not used */
         tmp_aff[i].duration = 0;
         tmp_aff[i].modifier = 0;
         tmp_aff[i].location = 0;
         tmp_aff[i].bitvector = 0;
         tmp_aff[i].next = 0;
       }
     }
     
     for (aff = ch->affected2, i = 0; i < MAX_AFFECT; i++) {
       if (aff) {
         tmp_aff2[i] = *aff;
         tmp_aff2[i].next = 0;
         aff = aff->next;
       } else {
         tmp_aff2[i].type = 0;  /* Zero signifies not used */
         tmp_aff2[i].duration = 0;
         tmp_aff2[i].modifier = 0;
         tmp_aff2[i].location = 0;
         tmp_aff2[i].bitvector = 0;
         tmp_aff2[i].next = 0;
       }
     }

     /*
      * remove the affections so that the raw values are stored; otherwise the
      * effects are doubled when the char logs back in.
      */

     while (ch->affected)
       affect_remove(ch, ch->affected);
     
     while (ch->affected2)
       affect2_remove(ch, ch->affected2);

     if ((i >= MAX_AFFECT) && aff && aff->next)
       log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

     ch->aff_abils = ch->real_abils;

     /* end char_to_store code */

     if(GET_NAME(ch))
       fbprintf(fl, "Name: %s\n", GET_NAME(ch));
     if(GET_PASSWD(ch))
       fbprintf(fl, "Pass: %s\n", GET_PASSWD(ch));
     if(GET_TITLE(ch))
       fbprintf(fl, "Titl: %s\n", GET_TITLE(ch));
     if(ch->player.description && *ch->player.description) {
       strcpy(buf, ch->player.description);
       kill_ems(buf);
       fbprintf(fl, "Desc:\n%s~\n", buf);
     }
     
 #ifdef ASCII_SAVE_POOFS
     if(POOFIN(ch))
       fbprintf(fl, "PfIn: %s\n", POOFIN(ch));
     if(POOFOUT(ch))
       fbprintf(fl, "PfOt: %s\n", POOFOUT(ch));
 #endif
     if(GET_SEX(ch) != PFDEF_SEX)
       fbprintf(fl, "Sex : %d\n", GET_SEX(ch));
     if(GET_SIZE(ch) != PFDEF_SIZE)
       fbprintf(fl, "Size: %d\n", GET_SIZE(ch));
     if(GET_CLASS(ch) != PFDEF_CLASS)
       fbprintf(fl, "Clas: %d\n", GET_CLASS(ch));
     if(GET_RACE(ch) != PFDEF_RACE)
       fbprintf(fl, "Race: %d\n", GET_RACE(ch));
     if(GET_LEVEL(ch) != PFDEF_LEVEL)
       fbprintf(fl, "Levl: %d\n", GET_LEVEL(ch));
     if(GET_HOME(ch) != PFDEF_HOMETOWN)
       fbprintf(fl, "Home: %d\n", GET_HOME(ch));
     fbprintf(fl, "Brth: %d\n", ch->player.time.birth);
     fbprintf(fl, "Plyd: %d\n", ch->player.time.played);
     fbprintf(fl, "Last: %d\n", ch->player.time.logon);
     if(ch->player_specials->host)
       fbprintf(fl, "Host: %s\n", ch->player_specials->host);
     if(GET_HEIGHT(ch) != PFDEF_HEIGHT)
       fbprintf(fl, "Hite: %d\n", GET_HEIGHT(ch));
     if(GET_WEIGHT(ch) != PFDEF_HEIGHT)
       fbprintf(fl, "Wate: %d\n", GET_WEIGHT(ch));

     if(GET_ALIGNMENT(ch) != PFDEF_ALIGNMENT)
       fbprintf(fl, "Alin: %d\n", GET_ALIGNMENT(ch));
     fbprintf(fl, "Id  : %d\n", GET_IDNUM(ch));
     if(PLR_FLAGS(ch) != PFDEF_PLRFLAGS)
       fbprintf(fl, "Act : %d\n", PLR_FLAGS(ch));
     if(AFF_FLAGS(ch) != PFDEF_AFFFLAGS) {
       sprintbits(AFF_FLAGS(ch), bits);
       fbprintf(fl, "Aff : %s\n", bits);
     }
     if(AFF2_FLAGS(ch) != PFDEF_AFFFLAGS) {
       sprintbits(AFF2_FLAGS(ch), bits);
       fbprintf(fl, "Af2 : %s\n", bits);
     }
     if(GET_SAVE(ch, 0) != PFDEF_SAVETHROW)
       fbprintf(fl, "Thr1: %d\n", GET_SAVE(ch, 0));
     if(GET_SAVE(ch, 1) != PFDEF_SAVETHROW)
       fbprintf(fl, "Thr2: %d\n", GET_SAVE(ch, 1));
     if(GET_SAVE(ch, 2) != PFDEF_SAVETHROW)
       fbprintf(fl, "Thr3: %d\n", GET_SAVE(ch, 2));
     if(GET_SAVE(ch, 3) != PFDEF_SAVETHROW)
       fbprintf(fl, "Thr4: %d\n", GET_SAVE(ch, 3));
     if(GET_SAVE(ch, 4) != PFDEF_SAVETHROW)
       fbprintf(fl, "Thr5: %d\n", GET_SAVE(ch, 4));

     if(GET_LEVEL(ch) < LVL_SAINT) {
       fbprintf(fl, "Skil:\n");
       for(i = 1; i <= MAX_SKILLS; i++) {
       if(GET_SKILL(ch, i))
         fbprintf(fl, "%d %d\n", i, GET_SKILL(ch, i));
       }
       fbprintf(fl, "0 0\n");
     }
     if(GET_WIMP_LEV(ch) != PFDEF_WIMPLEV)
       fbprintf(fl, "Wimp: %d\n", GET_WIMP_LEV(ch));
     if(GET_FREEZE_LEV(ch) != PFDEF_FREEZELEV)
       fbprintf(fl, "Frez: %d\n", GET_FREEZE_LEV(ch));
     if(GET_INVIS_LEV(ch) != PFDEF_INVISLEV)
       fbprintf(fl, "Invs: %d\n", GET_INVIS_LEV(ch));
     if(GET_LOADROOM(ch) != PFDEF_LOADROOM)
       fbprintf(fl, "Room: %d\n", GET_LOADROOM(ch));
     if(PRF_FLAGS(ch) != PFDEF_PREFFLAGS) {
       sprintbits(PRF_FLAGS(ch), bits);
       fbprintf(fl, "Pref: %s\n", bits);
     }
     if(GET_BAD_PWS(ch) != PFDEF_BADPWS)
       fbprintf(fl, "Badp: %d\n", GET_BAD_PWS(ch));
     if(GET_COND(ch, FULL) != PFDEF_HUNGER && GET_LEVEL(ch) < LVL_SAINT)
       fbprintf(fl, "Hung: %d\n", GET_COND(ch, FULL));
     if(GET_COND(ch, THIRST) != PFDEF_THIRST && GET_LEVEL(ch) < LVL_SAINT)
       fbprintf(fl, "Thir: %d\n", GET_COND(ch, THIRST));
     if(GET_COND(ch, DRUNK) != PFDEF_DRUNK && GET_LEVEL(ch) < LVL_SAINT)
       fbprintf(fl, "Drnk: %d\n", GET_COND(ch, DRUNK));
     if(GET_PRACTICES(ch) != PFDEF_PRACTICES)
       fbprintf(fl, "Lern: %d\n", GET_PRACTICES(ch));

     if(GET_STR(ch) != PFDEF_STR || GET_ADD(ch) != PFDEF_STRADD)
       fbprintf(fl, "Str : %d/%d\n", GET_STR(ch), GET_ADD(ch));
     if(GET_INT(ch) != PFDEF_INT)
       fbprintf(fl, "Int : %d\n", GET_INT(ch));
     if(GET_WIS(ch) != PFDEF_WIS)
       fbprintf(fl, "Wis : %d\n", GET_WIS(ch));
     if(GET_DEX(ch) != PFDEF_DEX)
       fbprintf(fl, "Dex : %d\n", GET_DEX(ch));
     if(GET_CON(ch) != PFDEF_CON)
       fbprintf(fl, "Con : %d\n", GET_CON(ch));
     if(GET_CHA(ch) != PFDEF_CHA)
       fbprintf(fl, "Cha : %d\n", GET_CHA(ch));

     if(GET_HIT(ch) != PFDEF_HIT || GET_MAX_HIT(ch) != PFDEF_MAXHIT)
       fbprintf(fl, "Hit : %d/%d\n", GET_HIT(ch), GET_MAX_HIT(ch));

     if(GET_MANA(ch) != PFDEF_MANA || GET_MAX_MANA(ch) != PFDEF_MAXMANA)
       fbprintf(fl, "Mana: %d/%d\n", GET_MANA(ch), GET_MAX_MANA(ch));
     if(GET_MOVE(ch) != PFDEF_MOVE || GET_MAX_MOVE(ch) != PFDEF_MAXMOVE)
       fbprintf(fl, "Move: %d/%d\n", GET_MOVE(ch), GET_MAX_MOVE(ch));
     if(GET_AC(ch) != PFDEF_AC)
       fbprintf(fl, "Ac  : %d\n", GET_AC(ch));
     if(GET_GOLD(ch) != PFDEF_GOLD)
       fbprintf(fl, "Gold: %d\n", GET_GOLD(ch));
     if(GET_BANK_GOLD(ch) != PFDEF_BANK)
       fbprintf(fl, "Bank: %d\n", GET_BANK_GOLD(ch));
     if(GET_EXP(ch) != PFDEF_EXP)
       fbprintf(fl, "Exp : %d\n", GET_EXP(ch));
     if(GET_HITROLL(ch) != PFDEF_HITROLL)
       fbprintf(fl, "Hrol: %d\n", GET_HITROLL(ch));
     if(GET_DAMROLL(ch))
       fbprintf(fl, "Drol: %d\n", GET_DAMROLL(ch));   
     if(ch->player_specials->saved.olc_zone != 0)
       fbprintf(fl, "Solc: %d\n", ch->player_specials->saved.olc_zone );
  if(GET_CLAN(ch) != PFDEF_CLAN)
    fbprintf(fl, "Clan: %d\n", GET_CLAN(ch));
  if(GET_CLAN_RANK(ch) != PFDEF_CLANRANK)
    fbprintf(fl, "Rank: %d\n", GET_CLAN_RANK(ch));

     /* affected_type */
     if(tmp_aff[0].type > 0) {
       fbprintf(fl, "Affs:\n");
       for(i = 0; i < MAX_AFFECT; i++) {
       aff = &tmp_aff[i];
       if(aff->type)
         fbprintf(fl, "%d %d %d %d %lld\n", aff->type, aff->duration,
           aff->modifier, aff->location, aff->bitvector);
       }
       fbprintf(fl, "0 0 0 0 0\n");
      }
      
       /* affected2_type */
     if(tmp_aff2[0].type > 0) {
       fbprintf(fl, "Afs2:\n");
       for(i = 0; i < MAX_AFFECT; i++) {
       aff = &tmp_aff2[i];
       if(aff->type)
         fbprintf(fl, "%d %d %d %d %lld\n", aff->type, aff->duration,
           aff->modifier, aff->location, aff->bitvector);
       }
       fbprintf(fl, "0 0 0 0 0\n");
      }

     fbclose(fl);

     /* more char_to_store code to restore affects */

     /* add spell and eq affections back in now */
     for (i = 0; i < MAX_AFFECT; i++) {
       if (tmp_aff[i].type)
         affect_to_char(ch, &tmp_aff[i]);
     }     
     
     for (i = 0; i < MAX_AFFECT; i++) {
       if (tmp_aff2[i].type)
         affect2_to_char(ch, &tmp_aff2[i]);
     }

     for (i = 0; i < NUM_WEARS; i++) {
       if (char_eq[i])
          equip_char(ch, char_eq[i], i);
      }
     /* end char_to_store code */

     if((id = get_ptable_by_name(GET_NAME(ch))) < 0)
       return;

     /* update the player in the player index */
     if(player_table[id].level != GET_LEVEL(ch)) {
       save_index = TRUE;
       player_table[id].level = GET_LEVEL(ch);
     }
     if(player_table[id].last != ch->player.time.logon) {
       save_index = TRUE;
       player_table[id].last = ch->player.time.logon;
     }
     i = player_table[id].flags;
     if(PLR_FLAGGED(ch, PLR_DELETED))
       SET_BIT(player_table[id].flags, PINDEX_DELETED);
     else
       REMOVE_BIT(player_table[id].flags, PINDEX_DELETED);
     if(PLR_FLAGGED(ch, PLR_NODELETE) || PLR_FLAGGED(ch, PLR_CRYO))
       SET_BIT(player_table[id].flags, PINDEX_NODELETE);
     else
       REMOVE_BIT(player_table[id].flags, PINDEX_NODELETE);
     if(player_table[id].flags != i || save_index)
       save_player_index();
   }
 }

/* ziz */

void save_etext(struct char_data *ch)
{
/* this will be really cool soon */
}


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(char *name)
{
  int i, pos;

  if (top_of_p_table == -1) {	/* no table */
    CREATE(player_table, struct player_index_element, 1);
    pos = top_of_p_table = 0;
  } else if ((pos = get_ptable_by_name(name)) == -1) {	/* new name */
    i = ++top_of_p_table + 1;

    RECREATE(player_table, struct player_index_element, i);
    pos = top_of_p_table;
  }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (player_table[pos].name[i] = LOWER(name[i])); i++)
	/* Nothing */;

  return (pos);
}



/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[513];
  char *point;
  int done = 0, length = 0, templength;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      log("SYSERR: fread_string: format error at or near %s", error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if (length + templength >= MAX_STRING_LENGTH) {
      log("SYSERR: fread_string: string too large (db.c)");
      log("%s", error);
      exit(1);
    } else {
      strcat(buf + length, tmp);	/* strcat: OK (size checked above) */
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  return (strlen(buf) ? strdup(buf) : NULL);
}


/* release memory allocated for a char struct */
void free_char(struct char_data *ch)
{
  int i;
  struct alias_data *a;

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
    while ((a = GET_ALIASES(ch)) != NULL) {
      GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
      free_alias(a);
    }
    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
    if (ch->player_specials->host)
      free(ch->player_specials->host);
    free(ch->player_specials);
    if (IS_NPC(ch))
      log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
  }
  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == NOBODY)) {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (GET_NAME(ch))
      free(GET_NAME(ch));
    if (ch->player.title)
      free(ch->player.title);
    if (ch->player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description)
      free(ch->player.description);
  } else if ((i = GET_MOB_RNUM(ch)) != NOBODY) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->player.name && ch->player.name != mob_proto[i].player.name)
      free(ch->player.name);
    if (ch->player.title && ch->player.title != mob_proto[i].player.title)
      free(ch->player.title);
    if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description && ch->player.description != mob_proto[i].player.description)
      free(ch->player.description);
  }
  while (ch->affected)
    affect_remove(ch, ch->affected);

  if (ch->desc)
    ch->desc->character = NULL;

  free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(struct obj_data *obj)
{
  if (GET_OBJ_RNUM(obj) == NOWHERE)
    free_object_strings(obj);
  else
    free_object_strings_proto(obj);

  free(obj);
}


/*
 * Steps:
 *   1: Read contents of a text file.
 *   2: Make sure no one is using the pointer in paging.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * strdup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 *
 * If someone is reading a global copy we're trying to
 * replace, give everybody using it a different copy so
 * as to avoid special cases.
 */
int file_to_string_alloc(const char *name, char **buf)
{
  int temppage;
  char temp[MAX_STRING_LENGTH];
  struct descriptor_data *in_use;

  for (in_use = descriptor_list; in_use; in_use = in_use->next)
    if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
      return (-1);

  /* Lets not free() what used to be there unless we succeeded. */
  if (file_to_string(name, temp) < 0)
    return (-1);

  for (in_use = descriptor_list; in_use; in_use = in_use->next) {
    if (!in_use->showstr_count || *in_use->showstr_vector != *buf)
      continue;

    /* Let's be nice and leave them at the page they were on. */
    temppage = in_use->showstr_page;
    paginate_string((in_use->showstr_head = strdup(*in_use->showstr_vector)), in_use);
    in_use->showstr_page = temppage;
  }

  if (*buf)
    free(*buf);

  *buf = strdup(temp);
  return (0);
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE + 3];
  int len;

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    log("SYSERR: reading %s: %s", name, strerror(errno));
    return (-1);
  }

  for (;;) {
    if (!fgets(tmp, READ_SIZE, fl))	/* EOF check */
      break;
    if ((len = strlen(tmp)) > 0)
      tmp[len - 1] = '\0'; /* take off the trailing \n */
    strcat(tmp, "\r\n");	/* strcat: OK (tmp:READ_SIZE+3) */

    if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
      log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
      *buf = '\0';
      fclose(fl);
      return (-1);
    }
    strcat(buf, tmp);	/* strcat: OK (size checked above) */
  }

  fclose(fl);

  return (0);
}



/* clear some of the the working variables of a char */
void reset_char(struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    GET_EQ(ch, i) = NULL;

  ch->followers = NULL;
  ch->master = NULL;
  IN_ROOM(ch) = NOWHERE;
  ch->carrying = NULL;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  ch->char_specials.hitgain = 0;
  ch->char_specials.managain = 0;
  ch->char_specials.movegain = 0;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;

  GET_LAST_TELL(ch) = NOBODY;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  IN_ROOM(ch) = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_RNUM(ch) = NOBODY;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;

  GET_AC(ch) = 10;		/* Basic Armor */
  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;
}


void clear_object(struct obj_data *obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  IN_ROOM(obj) = NOWHERE;
  obj->worn_on = NOWHERE;
}




/*
 * Called during character creation after picking character class
 * (and then never again for that character).
 */
void init_char(struct char_data *ch)
{
  int i;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  /* *** if this is our first player --- he be God *** */
  if (top_of_p_table == 0) {
    GET_LEVEL(ch) = LVL_IMPL;
    GET_EXP(ch) = 7000000;

    /* The implementor never goes through do_start(). */
    GET_MAX_HIT(ch) = 500;
    GET_MAX_MANA(ch) = 100;
    GET_MAX_MOVE(ch) = 82;
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
  }

  set_title(ch, NULL);
  ch->player.short_descr = NULL;
  ch->player.long_descr = NULL;
  ch->player.description = NULL;

  ch->player.time.birth = time(0);
  ch->player.time.logon = time(0);
  ch->player.time.played = 0;

  GET_HOME(ch) = 1;
  GET_AC(ch) = 10;

  for (i = 0; i < MAX_TONGUE; i++)
    GET_TALK(ch, i) = 0;

  /*
   * make favors for sex -- or in English, we bias the height and weight of the
   * character depending on what gender they've chosen for themselves. While it
   * is possible to have a tall, heavy female it's not as likely as a male.
   *
   * Height is in centimeters. Weight is in pounds.  The only place they're
   * ever printed (in stock code) is SPELL_IDENTIFY.
   */
  if (GET_SEX(ch) == SEX_MALE) {
    GET_WEIGHT(ch) = rand_number(120, 180);
    GET_HEIGHT(ch) = rand_number(160, 200); /* 5'4" - 6'8" */
  } else {
    GET_WEIGHT(ch) = rand_number(100, 160);
    GET_HEIGHT(ch) = rand_number(150, 180); /* 5'0" - 6'0" */
  }

   /* SET SIZE BASED ON RACE for now it's normal */

   switch(GET_RACE(ch))
   {
     case RACE_HUMAN:
       GET_SIZE(ch) = SIZE_NORMAL;
       break;
     case RACE_DWARF:
       GET_SIZE(ch) = SIZE_SMALL;
       break;
     case RACE_ELF:
       GET_SIZE(ch) = SIZE_NORMAL;
       break;
     case RACE_GNOME:
       GET_SIZE(ch) = SIZE_SMALL;
       break;
     case RACE_HALFLING:
       GET_SIZE(ch) = SIZE_SMALL;
       break;
     case RACE_MINOTAUR:
       GET_SIZE(ch) = SIZE_LARGE;
       break;
     case RACE_PIXIE:
       GET_SIZE(ch) = SIZE_TINY;
       break;
     case RACE_ULDRA:
       GET_SIZE(ch) = SIZE_SMALL;
       break;
     case RACE_TRITON:
       GET_SIZE(ch) = SIZE_SMALL;
       break;
     case RACE_OGRE:
       GET_SIZE(ch) = SIZE_LARGE;
       break;
     case RACE_VAMPIRE:
       GET_SIZE(ch) = SIZE_LARGE;
       break;
     case RACE_SHINTARI:
       GET_SIZE(ch) = SIZE_LARGE;
       break;
     case RACE_KARADAL:
       GET_SIZE(ch) = SIZE_NORMAL;
       break;
     case RACE_VISRAEL:
       GET_SIZE(ch) = SIZE_SMALL;
       break;
     default:
       GET_SIZE(ch) = SIZE_NORMAL;
       log("SYSERR: init char: Character '%s' could not have size set", GET_NAME(ch));
   }


  if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
    player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
  else
    log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));

  for (i = 1; i <= MAX_SKILLS; i++) {
    if (GET_LEVEL(ch) < LVL_IMPL)
      SET_SKILL(ch, i, 0);
    else
      SET_SKILL(ch, i, 100);
  }

  AFF_FLAGS(ch) = 0;
  AFF2_FLAGS(ch) = 0;

  for (i = 0; i < 5; i++)
    GET_SAVE(ch, i) = 0;

  ch->real_abils.intel = 25;
  ch->real_abils.wis = 25;
  ch->real_abils.dex = 25;
  ch->real_abils.str = 25;
  ch->real_abils.str_add = 100;
  ch->real_abils.con = 25;
  ch->real_abils.cha = 25;

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);

  GET_LOADROOM(ch) = NOWHERE;
}



/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum)
{
  room_rnum bot, top, mid;

  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((world + mid)->number == vnum)
      return (mid);
    if (bot >= top)
      return (NOWHERE);
    if ((world + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
  mob_rnum bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (NOBODY);
    if ((mob_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}


/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  obj_rnum bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (NOTHING);
    if ((obj_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}


/* returns the real number of the zone with given virtual number */
zone_rnum real_zone(zone_vnum vnum)
{
  zone_rnum bot, top, mid;

  bot = 0;
  top = top_of_zone_table;

  /* perform binary search on zone-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((zone_table + mid)->number == vnum)
      return (mid);
    if (bot >= top)
      return (NOWHERE);
    if ((zone_table + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}


/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
int check_object(struct obj_data *obj)
{
  char objname[MAX_INPUT_LENGTH + 32];
  int error = FALSE;

  if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative weight (%d).",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));

  if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));

  snprintf(objname, sizeof(objname), "Object #%d (%s)", GET_OBJ_VNUM(obj), obj->short_description);
  error |= check_bitvector_names(GET_OBJ_WEAR(obj), wear_bits_count, objname, "object wear");
  error |= check_bitvector_names(GET_OBJ_EXTRA(obj), extra_bits_count, objname, "object extra");
  error |= check_bitvector_names(GET_OBJ_AFFECT(obj), affected_bits_count, objname, "object affect");

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_DRINKCON:
  {
    char onealias[MAX_INPUT_LENGTH], *space = strrchr(obj->name, ' ');

    strlcpy(onealias, space ? space + 1 : obj->name, sizeof(onealias));
    if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE))
      log("SYSERR: Object #%d (%s) doesn't have drink type as last alias. (%s)",
		GET_OBJ_VNUM(obj), obj->short_description, obj->name);
  }
  /* Fall through. */
  case ITEM_FOUNTAIN:
    if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
      log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 1);
    error |= check_object_spell_number(obj, 2);
    error |= check_object_spell_number(obj, 3);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 3);
    if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
      log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
    break;
 }

  return (error);
}


/* new functions used by ascii pfiles */

/* Separate a 4-character id tag from the data it precedes */
void tag_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i;

  for(i = 0; i < 4; i++)
    *(ttag++) = *(tmp++);
  *ttag = '\0';
  
  while(*tmp == ':' || *tmp == ' ')
    tmp++;

  while(*tmp)
    *(wrt++) = *(tmp++);
  *wrt = '\0';
}


/* This function necessary to save a seperate ascii player index */
void save_player_index(void)
{
  int i;
  char bits[64];
  FBFILE *index_file;

  if(!(index_file = fbopen(PLR_INDEX_FILE, FB_WRITE))) {
    log("SYSERR:  Could not write player index file");
    return;
  }

  for(i = 0; i <= top_of_p_table; i++)
    if(*player_table[i].name) {
      sprintbits(player_table[i].flags, bits);
      fbprintf(index_file, "%d %s %d %s %d\n", player_table[i].id,
	player_table[i].name, player_table[i].level, *bits ? bits : "0",
  player_table[i].last);
    }
  fbprintf(index_file, "~\n");

  fbclose(index_file);
}


/* This is a general purpose function originally from obuild OLC,
   and there is probably a similar function in Oasis.  Feel free to
   remove this and use another
*/
/*thanks to Luis Carvalho for sending this my way..it's probably a
  lot shorter than the one I would have made :)  */
void sprintbits(long vektor,char *outstring)
{
  int i;
  char flags[53]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  strcpy(outstring,"");
  for (i=0;i<53;i++)
  {
    if (vektor & 1) {
      *outstring=flags[i];
      outstring++;
    };
    vektor>>=1;
  };
  *outstring=0;
};

/* remove_player removes all files associated with a player who is
   self-deleted, deleted by an immortal, or deleted by the auto-wipe
   system (if enabled).  If you add more character files, you'll want
   to put a remover here.
*/ 
void remove_player(int pfilepos)
{
  char pfile_name[128]/*, rent_name[128]*/;

  if(!*player_table[pfilepos].name)
    return;

  unlink(pfile_name);

  /* Unlink any other player-owned files here if you have them  */

  log("PCLEAN: %s Lev: %d Last: %s",
	  player_table[pfilepos].name, player_table[pfilepos].level,
    asctime(localtime(&player_table[pfilepos].last)));
  player_table[pfilepos].name[0] = '\0';
  save_player_index();
}


void clean_pfiles(void)
{
  int i, ci, timeout;

  for(i = 0; i <= top_of_p_table; i++) {
    if(IS_SET(player_table[i].flags, PINDEX_NODELETE))
      continue;
    timeout = -1;
    for(ci = 0; ci == 0 || (pclean_criteria[ci].level > 
      pclean_criteria[ci - 1].level); ci++) {
      if((pclean_criteria[ci].level == -1 && IS_SET(player_table[i].flags,
        PINDEX_DELETED)) || player_table[i].level <=
        pclean_criteria[ci].level) {
          timeout = pclean_criteria[ci].days;
          break;
      }
    }
    if(timeout >= 0) {
      timeout *= SECS_PER_REAL_DAY;
      if((time(0) - player_table[i].last) > timeout)
        remove_player(i);
    }
  }  
}

/* end of ascii pfile added functions */


int check_object_spell_number(struct obj_data *obj, int val)
{
  int error = FALSE;
  const char *spellname;

  if (GET_OBJ_VAL(obj, val) == -1)	/* i.e.: no spell */
    return (error);

  /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
  if (GET_OBJ_VAL(obj, val) < 0)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > MAX_SPELLS && GET_OBJ_VAL(obj, val) <= MAX_SKILLS)
    error = TRUE;
  if (error)
    log("SYSERR: Object #%d (%s) has out of range spell #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
  if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
    log("... '%s' (#%d) uses %s spell '%s'.",
	obj->short_description,	GET_OBJ_VNUM(obj),
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
	skill_name(GET_OBJ_VAL(obj, val)));
#endif

  if (scheck)		/* Spell names don't exist in syntax check mode. */
    return (error);

  /* Now check for unnamed spells. */
  spellname = skill_name(GET_OBJ_VAL(obj, val));

  if ((spellname == unused_spellname || !str_cmp("UNDEFINED", spellname)) && (error = TRUE))
    log("SYSERR: Object #%d (%s) uses '%s' spell #%d.",
		GET_OBJ_VNUM(obj), obj->short_description, spellname,
		GET_OBJ_VAL(obj, val));

  return (error);
}

int check_object_level(struct obj_data *obj, int val)
{
  int error = FALSE;

  if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL) && (error = TRUE))
    log("SYSERR: Object #%d (%s) has out of range level #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  return (error);
}

int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits)
{
  unsigned int flagnum;
  bool error = FALSE;

  /* See if any bits are set above the ones we know about. */
  if (bits <= (~(bitvector_t)0 >> (sizeof(bitvector_t) * 8 - namecount)))
    return (FALSE);

  for (flagnum = namecount; flagnum < sizeof(bitvector_t) * 8; flagnum++)
    if ((1ULL << flagnum) & bits) {
      log("SYSERR: %s has unknown %s flag, bit %d (0 through %d known).", whatami, whatbits, flagnum, namecount - 1);
      error = TRUE;
    }

  return (error);
}
