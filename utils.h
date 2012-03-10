/* 
************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/* external declarations and prototypes **********************************/

extern struct weather_data weather_info;
extern FILE *logfile;

#define log			basic_mud_log

#define READ_SIZE	256

/* public functions in utils.c */
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void	basic_mud_vlog(const char *format, va_list args);
int	touch(const char *path);
void	mudlog(int type, int level, int file, const char *str, ...) __attribute__ ((format (printf, 4, 5)));
void	log_death_trap(struct char_data *ch);
int	rand_number(int from, int to);
int	dice(int number, int size);
size_t	sprintbit(bitvector_t vektor, const char *names[], char *result, size_t reslen);
size_t	sprinttype(int type, const char *names[], char *result, size_t reslen);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);
time_t	mud_time_to_secs(struct time_info_data *now);
struct time_info_data *age(struct char_data *ch);
int	num_pc_in_room(struct room_data *room);
void	core_dump_real(const char *, int);
int	room_is_dark(room_rnum room);
int get_weapon_dam(struct obj_data *obj);
int get_room_light(room_rnum room);
int get_outside_light();
bool is_daytime();
int can_see_room(struct char_data *ch, room_rnum room);
int get_ave_dam(struct obj_data *obj);
void infochan(const char *str, ...);
int get_max_damage_per_hit(struct char_data *ch, bool use_held);
int get_total_hitbonus(struct char_data *ch);
int get_total_dambonus(struct char_data *ch);


#define core_dump()		core_dump_real(__FILE__, __LINE__)

/*
 * Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a strcasecmp or stricmp exists.
 */
#ifndef str_cmp
int	str_cmp(const char *arg1, const char *arg2);
#endif
#ifndef strn_cmp
int	strn_cmp(const char *arg1, const char *arg2, int n);
#endif

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
char *CAP(char *txt);

/* Followers */
int	num_followers_charmed(struct char_data *ch);
void	die_follower(struct char_data *ch);
void	add_follower(struct char_data *ch, struct char_data *leader);
void	stop_follower(struct char_data *ch);
bool	circle_follow(struct char_data *ch, struct char_data *victim);

/* in act.informative.c */
void	look_at_room(room_rnum target_room, struct char_data *ch, int mode);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* in limits.c */
int	mana_gain(struct char_data *ch);
int	hit_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
void	advance_level(struct char_data *ch);
void    check_max_on_level(struct char_data *ch);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	check_idling(struct char_data *ch);
void	point_update(void);
void	update_pos(struct char_data *victim);


/* various constants *****************************************************/

/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	0
#define ETEXT_FILE	1
#define ALIAS_FILE	2
#define SCRIPT_VARS_FILE 3

/* breadth-first searching */
#define BFS_ERROR		(-1)
#define BFS_ALREADY_THERE	(-2)
#define BFS_NO_PATH		(-3)

/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time */
#define SECS_PER_MUD_HOUR	75
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(17*SECS_PER_MUD_MONTH)
#define IS_DAYTIME              ((time_info.hours>=DAWN_HOUR) && (time_info.hours<DUSK_HOUR))
#define MOONLIGHT_SCALOR        ((time_info.moon_phase <(MUD_MOON_PHASES>>1)) ? (time_info.moon_phase*0.2222+0.3333) : (((MUD_MOON_PHASES-1-time_info.moon_phase)*0.2222)+0.3333))
 /* real-life time (remember Real Life?) */ 
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)



/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 

/* See also: ANA, SANA */
#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")


/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("SYSERR: realloc failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) ^= (bit))
#define IN_ZONE(ch) (world[(ch)->in_room].zone)
/* zonenumb should be IN_ZONE(ch) */


/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define MOB_FLAGS(ch)	((ch)->char_specials.saved.act)
#define PLR_FLAGS(ch)	((ch)->char_specials.saved.act)
#define PRF_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.pref))
#define AFF_FLAGS(ch)	((ch)->char_specials.saved.affected_by)
#define AFF2_FLAGS(ch)	((ch)->char_specials.saved.affected_by2)
#define ROOM_FLAGS(loc)	(world[(loc)].room_flags)
#define SPELL_ROUTINES(spl)	(spell_info[spl].routines)
#define MSC_FLAGS(ch)   ((ch)->char_specials.misc_flags_bitvector)

#define IS_FIGHTING(ch)          (GET_POS(ch) == POS_FIGHTING)
#define WIELDED_WEAPON(ch)       (GET_EQ(ch, WEAR_WIELD))
#define IS_USING_HELD_WEAPON(ch) (GET_EQ(ch, WEAR_HOLD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON))
#define IS_USING_WEAPON(ch)      (WIELDED_WEAPON(ch) || IS_USING_HELD_WEAPON(ch))
#define GET_DAMNODICE(ch)        ((ch)->player.damnodice)
#define GET_DAMSIZEDICE(ch)      ((ch)->player.damsizedice)
#define GET_DAMBONUS(ch)         ((ch)->player.dambonus)
#define IS_USING_SHIELD(ch)      (GET_EQ(ch, WEAR_SHIELD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_SHIELD)) == ITEM_WEAR_SHIELD))
#define GET_AP(ch)               ((ch)->points.armor)

/*
 * See http://www.circlemud.org/~greerga/todo/todo.009 to eliminate MOB_ISNPC.
 * IS_MOB() acts as a VALID_MOB_RNUM()-like function.
 */
#define IS_NPC(ch)	(IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)	(IS_NPC(ch) && GET_MOB_RNUM(ch) <= top_of_mobt && \
				GET_MOB_RNUM(ch) != NOBODY)
#define IS_CLANNED(ch)  (!IS_NPC(ch) && ch->player_specials->saved.clan_id > 0)

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET(AFF_FLAGS(ch), (flag)))
#define AFF2_FLAGGED(ch, flag) (IS_SET(AFF2_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET(PRF_FLAGS(ch), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))
#define OBJAFF_FLAGGED(obj, flag) (IS_SET(GET_OBJ_AFFECT(obj), (flag)))
#define OBJAFF2_FLAGGED(obj, flag) (IS_SET(GET_OBJ_AFFECT2(obj), (flag)))
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, flag) (IS_SET(GET_OBJ_WEAR(obj), (flag)))
#define OBJ_FLAGGED(obj, flag) (IS_SET(GET_OBJ_EXTRA(obj), (flag)))
#define OBJ_CLASS(obj, flag) (IS_SET(GET_OBJ_CLASS(obj), (flag)))
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))
#define IS_AFFECTED2(ch, skill) (AFF2_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))


/* room utils ************************************************************/


#define SECT(room)	(VALID_ROOM_RNUM(room) ? \
				world[(room)].sector_type : SECT_INSIDE)

#define SIZE(room)	(VALID_ROOM_RNUM(room) ? \
				world[(room)].size : SIZE_SPECIAL)

#define ROOM_MIN_LEVEL(room)  (world[(room)].min_level)
#define ROOM_MAX_LEVEL(room)  (world[(room)].max_level)

#define IS_DARK(room)	room_is_dark((room))
#define IS_LIGHT(room)  (!IS_DARK(room))

#define VALID_ROOM_RNUM(rnum)	((rnum) != NOWHERE && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) \
	(VALID_ROOM_RNUM(room) ? world[(room)].func : NULL)


/* char utils ************************************************************/


#define IN_ROOM(ch)	((ch)->in_room)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch)->year)

#define GET_PC_NAME(ch)	((ch)->player.name)
#define GET_NAME(ch)    (IS_NPC(ch) ? \
			 (ch)->player.short_descr : GET_PC_NAME(ch))
#define GET_PC_NAME(ch)          ((ch)->player.name)

#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)
#define GET_DIFFICULTY(ch)   ((ch)->player.difficulty)
#define GET_PASSWD(ch)	((ch)->player.passwd)
#define GET_PFILEPOS(ch)((ch)->pfilepos)
#define GET_AGGRESSION(ch, i)    ((ch)->player.aggression[i])


/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define GET_CLASS(ch)   ((ch)->player.chclass)
#define GET_RACE(ch)    ((ch)->player.race)
#define GET_HOME(ch)	((ch)->player.hometown)
#define GET_HEIGHT(ch)	((ch)->player.height)
#define GET_WEIGHT(ch)	((ch)->player.weight)
#define GET_SEX(ch)	((ch)->player.sex)
#define GET_SIZE(ch)    ((ch)->player.size)

#define GET_STR(ch)     ((ch)->aff_abils.str)
#define GET_ADD(ch)     ((ch)->aff_abils.str_add)
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)
#define GET_TOT_STR(ch)          ((ch)->aff_abils.str)
#define GET_TOT_ADD(ch)          ((ch)->aff_abils.str_add)
#define GET_TOT_DEX(ch)          ((ch)->aff_abils.dex)
#define GET_TOT_INT(ch)          ((ch)->aff_abils.intel)
#define GET_TOT_WIS(ch)          ((ch)->aff_abils.wis)
#define GET_TOT_CON(ch)          ((ch)->aff_abils.con)
#define GET_TOT_CHA(ch)          ((ch)->aff_abils.cha)
#define GET_NAT_STR(ch)          ((ch)->real_abils.str)
#define GET_NAT_ADD(ch)          ((ch)->real_abils.str_add)
#define GET_NAT_DEX(ch)          ((ch)->real_abils.dex)
#define GET_NAT_INT(ch)          ((ch)->real_abils.intel)
#define GET_NAT_WIS(ch)          ((ch)->real_abils.wis)
#define GET_NAT_CON(ch)          ((ch)->real_abils.con)
#define GET_NAT_CHA(ch)          ((ch)->real_abils.cha)


#define GET_EXP(ch)	  ((ch)->points.exp)
#define GET_AC(ch)        ((ch)->points.armor)
#define GET_RESIST(ch, i)        ((ch)->points.resistance[i])
#define GET_HIT(ch)	  ((ch)->points.hit)
#define GET_MAX_HIT(ch)	  ((ch)->points.max_hit)
#define GET_MOVE(ch)	  ((ch)->points.move)
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
#define GET_MANA(ch)	  ((ch)->points.mana)
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
#define GET_GOLD(ch)	  ((ch)->points.gold)
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold)
#define GET_HITROLL(ch)	  ((ch)->points.hitroll)
#define GET_DAMROLL(ch)   ((ch)->points.damroll)
#define GET_DISHONOR(ch)         ((ch)->player_specials->saved.dishonor)



#define GET_POS(ch)	  ((ch)->char_specials.position)
#define GET_IDNUM(ch)	  ((ch)->char_specials.saved.idnum)
#define GET_ID(x)         ((x)->id)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define FIGHTING(ch)	  ((ch)->char_specials.fighting)
#define HUNTING(ch)	  ((ch)->char_specials.hunting)
#define GET_SAVE(ch, i)	  ((ch)->char_specials.saved.apply_saving_throw[i])
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)
#define GET_ATTACKS(ch)   ((ch)->char_specials.extra_attack)
#define FORTDAM(ch)       ((ch)->char_specials.damback)
#define GET_STUN_RECOVER_CHANCE(ch) ((ch)->char_specials.stun_recovery_chance)
#define GET_STUN_DURATION(ch)    ((ch)->char_specials.stun_min_duration)


#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.conditions[(i)]))
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))
#define GET_PRACTICES(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.spells_to_learn))
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.wimp_level))
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.freeze_level))
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.talks[i]))
#define POOFIN(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))

#define GET_MOD_SKILL(ch, i)        (IS_NPC(ch) ? ((ch)->mob_specials.mod_mskills[i]) : \
                                    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.mod_skills[i])))

#define GET_TOT_SKILL(ch, i)        MIN(MAX_PROFICIENCY,MAX(MIN_PROFICIENCY,(GET_NAT_SKILL(ch, i) + GET_MOD_SKILL(ch, i))))



#define GET_CLAN(ch)		((ch)->player.clan)
#define GET_CLAN_RANK(ch) 	((ch)->player.rank)

#define GET_SKILL(ch, i)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[i]))
#define SET_SKILL(ch, i, pct)	do { CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.skills[i]) = pct; } while(0)


#define SET_NAT_PSKILL(ch, i, pct)   do { CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.nat_skills[i]) = pct; } while(0)
#define SET_NAT_MSKILL(ch, i, pct)  ((ch)->mob_specials.nat_mskills[i] = pct)
#define GET_NAT_SKILL(ch, i)        (IS_NPC(ch) ? ((ch)->mob_specials.nat_mskills[i]) : \
                                    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.nat_skills[i])))

#define SET_MOD_PSKILL(ch, i, pct)   do { CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.mod_skills[i]) = pct; } while(0)
#define SET_MOD_MSKILL(ch, i, pct)  ((ch)->mob_specials.mod_mskills[i] = pct)
#define GET_MOD_SKILL(ch, i)        (IS_NPC(ch) ? ((ch)->mob_specials.mod_mskills[i]) : \
                                    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.mod_skills[i])))


#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_MOB_SPEC(ch)	(IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].vnum : NOBODY)
#define GET_MOB_ATTACKS(ch)			((ch)->mob_specials.num_attacks)

#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define MEMORY(ch)		((ch)->mob_specials.memory)

#define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch)==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        )

#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w)
#define CAN_CARRY_N(ch) (5 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

#define IS_GRGOD (!IS_NPC(d) && (GET_LEVEL(d) >= LVL_GOD))
#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_INVIS(ch)                     (AFF_FLAGGED(ch, AFF_IMPROVED_INVIS) || AFF_FLAGGED(ch, AFF_INVISIBLE))
#define SENDOK(ch)    (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
                      (to_sleeping || AWAKE(ch)) && \
                      !PLR_FLAGGED((ch), PLR_WRITING))
#define IS_MOBILE(ch) (GET_POS(ch)>POS_SLEEPING)

/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define CHECK_WAIT(ch)                ((ch)->wait > 0)
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)


/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define STATE(d)	((d)->connected)

#define IS_PLAYING(d)   (STATE(d) == CON_TEDIT || STATE(d) == CON_REDIT || \
                        STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT || \
                        STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT || \
                        STATE(d) == CON_TRIGEDIT || STATE(d) == CON_PLAYING)

#define IS_EDITING(d)   (STATE(d) == CON_TEDIT || STATE(d) == CON_REDIT ||      \
                        STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT ||       \
                        STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT ||       \
                        STATE(d) == CON_TRIGEDIT)

/* object utils **********************************************************/

/*
 * Check for NOWHERE or the top array index?
 * If using unsigned types, the top array index will catch everything.
 * If using signed types, NOTHING will catch the majority of bad accesses.
 */
#define VALID_OBJ_RNUM(obj)	(GET_OBJ_RNUM(obj) <= top_of_objt && \
				 GET_OBJ_RNUM(obj) != NOTHING)

#define GET_OBJ_LEVEL(obj)      ((obj)->obj_flags.level)
#define GET_OBJ_MAX_LEVEL(obj)  ((obj)->obj_flags.max_level)
#define GET_OBJ_PERM(obj)       ((obj)->obj_flags.bitvector)
#define GET_OBJ_TYPE(obj)	((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)	((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)	((obj)->obj_flags.cost_per_day)
#define GET_OBJ_AFFECT(obj)	((obj)->obj_flags.bitvector)
#define GET_OBJ_AFFECT2(obj)	((obj)->obj_flags.bitvector2)
#define GET_OBJ_EXTRA(obj)	((obj)->obj_flags.extra_flags)
#define GET_OBJ_CLASS(obj)	((obj)->obj_flags.class_flags)
#define GET_OBJ_WEAR(obj)	((obj)->obj_flags.wear_flags)
#define GET_OBJ_VAL(obj, val)	((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)	((obj)->obj_flags.timer)
#define GET_OBJ_SIZE(obj)       ((obj)->size)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].vnum : NOTHING)
#define GET_OBJ_SPEC(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].func : NULL)

#define IS_CORPSE(obj)		(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
					GET_OBJ_VAL((obj), 3) == 1)

#define CAN_WEAR(obj, part)	OBJWEAR_FLAGGED((obj), (part))


/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a")


/* Various macros building up to CAN_SEE */

#define IS_GOOD_LIGHT_FOR_SHADOWS(rnum) ((get_room_light(rnum)>=MIN_ROOM_BRIGHTNESS_FOR_CLOAK) && \
                                         (get_room_light(rnum)<=MAX_ROOM_BRIGHTNESS_FOR_CLOAK) && !ROOM_FLAGGED(rnum, ROOM_LIT))

#define IS_IN_HEALING_DREAM(ch)          (AFF_FLAGGED(ch, AFF_HEALING_DREAM) && (GET_POS(ch)==POS_SLEEPING))
#define IS_FULLY_HEALED(ch)              ((GET_HIT(ch)==GET_MAX_HIT(ch)) && (GET_MOVE(ch)==GET_MAX_MOVE(ch)) && \
                                         (GET_MANA(ch) == GET_MAX_MANA(ch)))
#define IS_BLIND(ch)                     (AFF_FLAGGED(ch, AFF_BLIND) && !AFF_FLAGGED(ch, AFF_BAT_SONAR))
#define IS_BLINDABLE(ch)                 (!AFF_FLAGGED(ch, AFF_BLIND) && !AFF_FLAGGED(ch, AFF_BAT_SONAR) && \
                                         !MOB_FLAGGED(ch, MOB_NOBLIND))
#define CAN_SEE_ASLEEP(ch)               (AFF_FLAGGED(ch, AFF_DREAMSIGHT) || AFF_FLAGGED(ch, AFF_SLEEPWALK))

#define IS_NOMAGIC                        (ROOM_FLAGGED(room, ROOM_NOMAGIC))

#define IS_SLEEPWALKING(ch)              (AFF_FLAGGED(ch, AFF_SLEEPWALK) && (GET_POS(ch)==POS_SLEEPING))
#define IS_CLOAKED(ch)                   (AFF_FLAGGED(ch, AFF_CLOAK_OF_SHADOWS) && IS_GOOD_LIGHT_FOR_SHADOWS(IN_ROOM(ch)))
#define HAS_INFRAVISION(ch)              (AFF_FLAGGED(ch, AFF_INFRAVISION) || AFF_FLAGGED(ch, AFF_SECOND_SIGHT))
#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT(IN_ROOM(sub)) || AFF_FLAGGED((sub), AFF_INFRAVISION)))
#define INVIS_OK(sub, obj) \
 (((((!IS_INVIS(obj) && (!AFF_FLAGGED(obj,AFF_INVIS_TO_ENEMIES) || (IS_NPC(obj)==IS_NPC(sub)))) || AFF_FLAGGED(sub,AFF_DETECT_INVIS)) && (!AFF_FLAGGED((obj), AFF_HIDE) || AFF_FLAGGED(sub, AFF_SENSE_LIFE)) && !IS_CLOAKED(obj)) || AFF_FLAGGED(sub, AFF_BAT_SONAR)) && (GET_POS(obj) != POS_BURIED))


#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
   IMM_CAN_SEE(sub, obj)))

/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  (!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

/*modified to make traps invisible to morts by anubis */
#define MORT_CAN_SEE_OBJ(sub, obj) \
  (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj) \
  && GET_OBJ_TYPE(obj) != ITEM_TRAP)

#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || (GET_LEVEL(ch) == LVL_IMPL) || \
   (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT) && GET_OBJ_TYPE(obj) != ITEM_TRAP) )

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define PERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_NAME(ch) : "someone")

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")


#define EXIT(ch, door)  (world[IN_ROOM(ch)].dir_option[door])
#define W_EXIT(room, num)     (world[(room)].dir_option[(num)])
#define R_EXIT(room, num)     ((room)->dir_option[(num)])
#define IS_VALID_EXIT(ch, door)  (EXIT(ch,door) && (EXIT(ch,door)->to_room != NOWHERE))

#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))


#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])
#define RACE_ABBR(ch) (IS_NPC(ch) ? "--" : race_abbrevs[(int)GET_RACE(ch)])


/* For PCs */
#define IS_MAGIC_USER(ch)	(GET_CLASS(ch) == CLASS_MAGIC_USER)
#define IS_CLERIC(ch)	(GET_CLASS(ch) == CLASS_CLERIC)
#define IS_THIEF(ch)	(GET_CLASS(ch) == CLASS_THIEF)
#define IS_WARRIOR(ch)	  (GET_CLASS(ch) == CLASS_WARRIOR)
#define IS_SKNIGHT(ch)    (GET_CLASS(ch) == CLASS_SKNIGHT)
#define IS_PALADIN(ch)	 (GET_CLASS(ch) == CLASS_PALADIN)
#define IS_ASSASSIN(ch)	(GET_CLASS(ch) == CLASS_ASSASSIN)
#define IS_CHAOSMAGE(ch)	(GET_CLASS(ch) == CLASS_CHAOSMAGE)
#define IS_SHAMAN(ch)	(GET_CLASS(ch) == CLASS_SHAMAN)
#define IS_DRUID(ch)	(GET_CLASS(ch) == CLASS_DRUID)												
#define IS_RANGER(ch)	(GET_CLASS(ch) == CLASS_RANGER)												
#define IS_PRIEST(ch)	(GET_CLASS(ch) == CLASS_PRIEST)												
#define IS_DISCIPLE(ch)	 (GET_CLASS(ch) == CLASS_DISCIPLE)												
#define IS_CRUSADER(ch)	(GET_CLASS(ch) == CLASS_CRUSADER)												
#define IS_FIGHTER(ch)	(GET_CLASS(ch) == CLASS_FIGHTER)												
#define IS_BARBARIAN(ch)	(GET_CLASS(ch) == CLASS_BARBARIAN)
#define IS_MONK(ch)	(GET_CLASS(ch) == CLASS_MONK)												
#define IS_KNIGHT(ch)	(GET_CLASS(ch) == CLASS_KNIGHT)												
#define IS_ROGUE(ch)	(GET_CLASS(ch) == CLASS_ROGUE)												
#define IS_BARD(ch)	(GET_CLASS(ch) == CLASS_BARD)											
#define IS_JESTER(ch)	(GET_CLASS(ch) == CLASS_JESTER)												
#define IS_BLADE(ch)	(GET_CLASS(ch) == CLASS_BLADE)												
#define IS_BOUNTYHUNTER(ch)	(GET_CLASS(ch) == CLASS_BOUNTYHUNTER)
#define IS_BATTLEMAGE(ch)	(GET_CLASS(ch) == CLASS_BATTLEMAGE)												
#define IS_SORCEROR(ch)	(GET_CLASS(ch) == CLASS_SORCEROR)
#define IS_ENCHANTER(ch)	  (GET_CLASS(ch) == CLASS_ENCHANTER)												
#define IS_NECROMANCER(ch)  (GET_CLASS(ch) == CLASS_NECROMANCER)
#define IS_ALCHEMIST(ch)	   (GET_CLASS(ch) == CLASS_ALCHEMIST)
#define IS_FIGHTER_TYPE(ch)  (IS_FIGHTER(ch) || IS_WARRIOR(ch) || IS_KNIGHT(ch) || IS_SKNIGHT(ch) || IS_PALADIN(ch) || IS_MONK(ch) || IS_BARBARIAN(ch))
#define IS_PRIEST_TYPE(ch)   (IS_PRIEST(ch) || IS_SHAMAN(ch) || IS_CLERIC(ch) || IS_DRUID(ch) || IS_RANGER(ch) || IS_DISCIPLE(ch) || IS_CRUSADER(ch))
#define IS_MAGE_TYPE(ch)  (IS_MAGIC_USER(ch) || IS_BATTLEMAGE(ch) || IS_ENCHANTER(ch) || IS_CHAOSMAGE(ch) || IS_SORCEROR(ch) || IS_NECROMANCER(ch) || IS_ALCHEMIST(ch))
#define IS_ROGUE_TYPE(ch)  (IS_ROGUE(ch) || IS_THIEF(ch) || IS_BARD(ch) || IS_BOUNTYHUNTER(ch) || IS_JESTER(ch) || IS_BLADE(ch) || IS_ASSASSIN(ch))


/* For NPCs */
#define IS_MOB_MAGIC_USER(ch)	(GET_CLASS(ch) == MOB_CLASS_MAGIC_USER)
#define IS_MOB_CLERIC(ch)	(GET_CLASS(ch) == MOB_CLASS_CLERIC)
#define IS_MOB_THIEF(ch)	(GET_CLASS(ch) == MOB_CLASS_THIEF)
#define IS_MOB_WARRIOR(ch)	  (GET_CLASS(ch) == MOB_CLASS_WARRIOR)
#define IS_MOB_SKNIGHT(ch)    (GET_CLASS(ch) == MOB_CLASS_SKNIGHT)
#define IS_MOB_PALADIN(ch)	 (GET_CLASS(ch) == MOB_CLASS_PALADIN)
#define IS_MOB_ASSASSIN(ch)	(GET_CLASS(ch) == MOB_CLASS_ASSASSIN)
#define IS_MOB_CHAOSMAGE(ch)	(GET_CLASS(ch) == MOB_CLASS_CHAOSMAGE)
#define IS_MOB_SHAMAN(ch)	(GET_CLASS(ch) == MOB_CLASS_SHAMAN)
#define IS_MOB_DRUID(ch)	(GET_CLASS(ch) == MOB_CLASS_DRUID)												
#define IS_MOB_RANGER(ch)	(GET_CLASS(ch) == MOB_CLASS_RANGER)												
#define IS_MOB_PRIEST(ch)	(GET_CLASS(ch) == MOB_CLASS_PRIEST)												
#define IS_MOB_DISCIPLE(ch)	 (GET_CLASS(ch) == MOB_CLASS_DISCIPLE)												
#define IS_MOB_CRUSADER(ch)	(GET_CLASS(ch) == MOB_CLASS_CRUSADER)												
#define IS_MOB_FIGHTER(ch)	(GET_CLASS(ch) == MOB_CLASS_FIGHTER)												
#define IS_MOB_BARBARIAN(ch)	(GET_CLASS(ch) == MOB_CLASS_BARBARIAN)												
#define IS_MOB_MONK(ch)	(GET_CLASS(ch) == MOB_CLASS_MONK)												
#define IS_MOB_KNIGHT(ch)	(GET_CLASS(ch) == MOB_CLASS_KNIGHT)												
#define IS_MOB_ROGUE(ch)	(GET_CLASS(ch) == MOB_CLASS_ROGUE)												
#define IS_MOB_BARD(ch)	(GET_CLASS(ch) == MOB_CLASS_BARD)											
#define IS_MOB_JESTER(ch)	(GET_CLASS(ch) == MOB_CLASS_JESTER)
#define IS_MOB_BLADE(ch)	(GET_CLASS(ch) == MOB_CLASS_BLADE)												
#define IS_MOB_BOUNTYHUNTER(ch)	(GET_CLASS(ch) == MOB_CLASS_BOUNTYHUNTER)
#define IS_MOB_BATTLEMAGE(ch)	(GET_CLASS(ch) == MOB_CLASS_BATTLEMAGE)												
#define IS_MOB_SORCEROR(ch)	(GET_CLASS(ch) == MOB_CLASS_SORCEROR)
#define IS_MOB_ENCHANTER(ch)	  (GET_CLASS(ch) == MOB_CLASS_ENCHANTER)												
#define IS_MOB_NECROMANCER(ch)  (GET_CLASS(ch) == MOB_CLASS_NECROMANCER)
#define IS_MOB_ALCHEMIST(ch)	   (GET_CLASS(ch) == MOB_CLASS_ALCHEMIST)
#define IS_MOB_FIGHTER_TYPE(ch)  (IS_MOB_FIGHTER(ch) || IS_MOB_WARRIOR(ch) || IS_MOB_KNIGHT(ch) || IS_MOB_SKNIGHT(ch) || IS_MOB_PALADIN(ch) || IS_MOB_MONK(ch) || IS_MOB_BARBARIAN(ch))
#define IS_MOB_PRIEST_TYPE(ch)   (IS_MOB_PRIEST(ch) || IS_MOB_SHAMAN(ch) || IS_MOB_CLERIC(ch) || IS_MOB_DRUID(ch) || IS_MOB_RANGER(ch) || IS_MOB_DISCIPLE(ch) || IS_MOB_CRUSADER(ch))
#define IS_MOB_MAGE_TYPE(ch)  (IS_MOB_MAGIC_USER(ch) || IS_MOB_BATTLEMAGE(ch) || IS_MOB_ENCHANTER(ch) || IS_MOB_CHAOSMAGE(ch) || IS_MOB_SORCEROR(ch) || IS_MOB_NECROMANCER(ch) || IS_MOB_ALCHEMIST(ch))
#define IS_MOB_ROGUE_TYPE(ch)  (IS_MOB_ROGUE(ch) || IS_MOB_THIEF(ch) || IS_MOB_BARD(ch) || IS_MOB_BOUNTYHUNTER(ch) || IS_MOB_JESTER(ch) || IS_MOB_BLADE(ch) || IS_MOB_ASSASSIN(ch))   



/*For PCs */
#define IS_HUMAN(ch)  	(GET_RACE(ch) == RACE_HUMAN)
#define IS_DWARF(ch)  	(GET_RACE(ch) == RACE_DWARF)
#define IS_ELF(ch)    	(GET_RACE(ch) == RACE_ELF)
#define IS_GNOME(ch)  	(GET_RACE(ch) == RACE_GNOME)
#define IS_HALFLING(ch)	(GET_RACE(ch) == RACE_HALFLING)
#define IS_MINOTAUR(ch)	(GET_RACE(ch) == RACE_MINOTAUR)
#define IS_PIXIE(ch)	(GET_RACE(ch) == RACE_PIXIE)
#define IS_ULDRA(ch)	(GET_RACE(ch) == RACE_ULDRA)
#define IS_TRITON(ch)	(GET_RACE(ch) == RACE_TRITON)
#define IS_OGRE(ch)	(GET_RACE(ch) == RACE_OGRE)
#define IS_VAMPIRE(ch) 	(GET_RACE(ch) == RACE_VAMPIRE)
#define IS_SHINTARI(ch)	(GET_RACE(ch) == RACE_SHINTARI)
#define IS_KARADAL(ch)	(GET_RACE(ch) == RACE_KARADAL)
#define IS_VISRAEL(ch)	(GET_RACE(ch) == RACE_VISRAEL)


// For Mobs:

#define IS_MOB_HUMAN(ch)  	(GET_RACE(ch) == MOB_RACE_HUMAN)
#define IS_MOB_DWARF(ch) 	(GET_RACE(ch) == MOB_RACE_DWARF)
#define IS_MOB_ELF(ch)  	(GET_RACE(ch) == MOB_RACE_ELF)
#define IS_MOB_GNOME(ch)  	(GET_RACE(ch) == MOB_RACE_GNOME)
#define IS_MOB_HALFLING(ch)	(GET_RACE(ch) == MOB_RACE_HALFLING)
#define IS_MOB_MINOTAUR(ch)	(GET_RACE(ch) == MOB_RACE_MINOTAUR)
#define IS_MOB_PIXIE(ch)	(GET_RACE(ch) == MOB_RACE_PIXIE)
#define IS_MOB_ULDRA(ch)	(GET_RACE(ch) == MOB_RACE_ULDRA)
#define IS_MOB_TRITON(ch)	(GET_RACE(ch) == MOB_RACE_TRITON)
#define IS_MOB_OGRE(ch)		(GET_RACE(ch) == MOB_RACE_OGRE)
#define IS_MOB_VAMPIRE(ch) 	(GET_RACE(ch) == MOB_RACE_VAMPIRE)
#define IS_MOB_SHINTARI(ch)	(GET_RACE(ch) == MOB_RACE_SHINTARI)
#define IS_MOB_KARADAL(ch)	(GET_RACE(ch) == MOB_RACE_KARADAL)
#define IS_MOB_VISRAEL(ch)	(GET_RACE(ch) == MOB_RACE_VISRAEL)
#define IS_MOB_DRAGON(ch)	(GET_RACE(ch) == MOB_RACE_DRAGON)
#define IS_MOB_DEVA(ch)         (GET_RACE(ch) == MOB_RACE_DEVA)
#define IS_MOB_DEVIL(ch)        (GET_RACE(ch) == MOB_RACE_DEVIL)
#define IS_MOB_DEMON(ch)        (GET_RACE(ch) == MOB_RACE_DEMON)
#define IS_MOB_ELEMENTAL(ch)    (GET_RACE(ch) == MOB_RACE_ELEMENTAL)
#define IS_MOB_UNDEAD(ch)       (GET_RACE(ch) == MOB_RACE_UNDEAD)
#define IS_MOB_PLANT(ch)        (GET_RACE(ch) == MOB_RACE_PLANT)
#define IS_MOB_ANIMAL(ch)       (GET_RACE(ch) == MOB_RACE_ANIMAL)
#define IS_MOB_REPTILE(ch)      (GET_RACE(ch) == MOB_RACE_REPTILE)
#define IS_MOB_AQUATIC(ch)      (GET_RACE(ch) == MOB_RACE_AQUATIC)
#define IS_MOB_AERIAL(ch)       (GET_RACE(ch) == MOB_RACE_AERIAL)
#define IS_MOB_INSECT(ch)       (GET_RACE(ch) == MOB_RACE_INSECT)
#define IS_MOB_ARACHNID(ch)     (GET_RACE(ch) == MOB_RACE_ARACHNID)
#define IS_MOB_GOLEM(ch)        (GET_RACE(ch) == MOB_RACE_GOLEM)
#define IS_MOB_BEAST(ch)        (GET_RACE(ch) == MOB_RACE_BEAST)
#define IS_MOB_HUMANOID(ch)     (GET_RACE(ch) == MOB_RACE_HUMANOID)
#define IS_MOB_DRACONIAN(ch)    (GET_RACE(ch) == MOB_RACE_DRACONIAN)
#define IS_MOB_GIANT(ch)        (GET_RACE(ch) == MOB_RACE_GIANT)
#define IS_MOB_DINOSAUR(ch)     (GET_RACE(ch) == MOB_RACE_DINOSAUR)


#define OUTSIDER(room)   ((SECT(room) != SECT_INSIDE) && !ROOM_FLAGGED(room, ROOM_INDOORS))
#define OUTSIDE(ch) (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
#if defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
#define CRYPT(a,b) (a)
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

#define CONFIG_NOEFFECT         config_info.play.NOEFFECT


