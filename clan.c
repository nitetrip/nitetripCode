/* ***********************************************************************
 * File: clan.c                 From the Guild patch by Sean Mountcastle *
 * Usage: Main source code for clans.                                    *
 * Turned into its own file by Catherine Gore (Cheron)          May 2001 *
 * Modified by Cyric (StrifeMud)                                         *
 * For use with CircleMUD 3.1/OasisOLC 2.06 and ascii pfiles             *
 *********************************************************************** */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "interpreter.h" 
#include "constants.h"
#include "screen.h"
#include "comm.h"
#include "handler.h"
#include "clan.h"
#include "oasis.h"
#include "pfdefaults.h"
#include "genolc.h"

/* clan globals */
struct clan_type *clan_info = NULL;    /* queue of clans  */
int    cnum  = 0;                      /* number of clans */
int  newclan = 0;                      /* number of next new clan */

/* external functions */
long asciiflag_conv(char *);
void sprintbits(long bits, char *s);
void store_mail(long to, long from, char *message_pointer);
int add_to_save_list(zone_vnum, int type);
zone_rnum real_zone_by_thing(room_vnum vznum);
ACMD(do_say);

/* external globals */
extern struct descriptor_data *descriptor_list;
extern int top_of_p_table;
extern struct player_index_element *player_table;
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct zone_data *zone_table;

void free_clan(struct clan_type *c)
{
  int    i;
  
  if (c != NULL) {
    free(c->name);
    free(c->leadersname);
    for(i=0; i < NUM_CLAN_RANKS; i++) {
      free(c->rank_name[i]);
    }
    if (c->applicants[0] != NULL) {
      for (i = 0; i < MAX_CLAN_APPLICANTS; i++) {
        if (c->applicants[i] != NULL) {
          free(c->applicants[i]);
        }
      }
    }
    free(c);
  }
}

struct clan_type *enqueue_clan(void)
{
  struct clan_type *cptr;
  
  /* This is the first clan loaded if true */
  if (clan_info == NULL) {
    if ((clan_info = (struct clan_type *) malloc(sizeof(struct clan_type))) == NULL) {
      fprintf(stderr, "SYSERR: Out of memory for clans!");
      exit(1);
    } else {
      clan_info->next = NULL;
      return (clan_info);
    }
  } else { /* clan_info is not NULL */
    for (cptr = clan_info; cptr->next != NULL; cptr = cptr->next)
      /* Loop does the work */;
      if ((cptr->next = (struct clan_type *) malloc(sizeof(struct clan_type))) == NULL) {
        fprintf(stderr, "SYSERR: Out of memory for clans!");
        exit(1);
      } else {
        cptr->next->next = NULL;
        return (cptr->next);
      }
  }
  return NULL;
}

void dequeue_clan(int clannum)
{
  struct clan_type *cptr = NULL, *temp;
  
  if (clannum < 0 || clannum > cnum) {
    log("SYSERR: Attempting to dequeue invalid clan!\r\n");
    exit(1);
  } else {
    if (clan_info->number != clannum) {
      for (cptr = clan_info; cptr->next && cptr->next->number != clannum; cptr = cptr->next)
        ;
      if (cptr->next != NULL && cptr->next->number==clannum) {
        temp       = cptr->next;
        cptr->next = temp->next;
        free_clan(temp);
      }
    } else {
      /* The first one is the one being removed */
      cptr = clan_info;
      clan_info = clan_info->next;
      free_clan(cptr);
    }
  }
}

/* Puts clans in numerical order */
void order_clans(void)
{
  struct clan_type *cptr = NULL, *temp = NULL;

  if (cnum > 2) {
    for (cptr = clan_info; (cptr->next != NULL) && (cptr->next->next != NULL); cptr = cptr->next) {
      if (cptr->next->number > cptr->next->next->number) {
        temp = cptr->next;
        cptr->next = temp->next;
        temp->next = cptr->next->next;
        cptr->next->next = temp;
      }
    }
  }

  return;
}

void create_clanfile(void)
{
  FILE   *cfile;
  
  /* Should be no reason it can't open... */
  if ((cfile = fopen(CLAN_FILE, "wt")) == NULL) {
    fprintf(stderr, "SYSERR: Cannot save clans!\n");
    exit(0);
  }

  /* The total number of clans */
  fprintf(cfile, "1\r\n");
  /* The Void Clan */
  fprintf(cfile,  "#0\r\n"
    "The Void Clan~\r\n"
    "Clanless~\r\n"
    "0 0 0\r\n"
    "0 0 0 0\r\n"
    "NoOne\r\n"
    "Leader~\r\n"
    "Lieutenant~\r\n"
    "Sergeant~\r\n"
    "Member~\r\n"
    "Novice~\r\n"
    "Trainee~\r\n"
    "$\r\n");

  fclose(cfile);
}

/* Loads the clans from the text file */
void load_clans(void)
{
  FILE   *fl = NULL;
  int    clannum = 0, line_num = 0, i = 0, tmp, tmp1, tmp2, p_kill = FALSE;
  long clangold = 0;
  room_vnum clan_entr, clan_recall;
  char   name[80], buf[MAX_INPUT_LENGTH];
  char   *ptr;
  struct clan_type *cptr = NULL;
  bool   newc = FALSE;
  
  if ((fl = fopen(CLAN_FILE, "rt")) == NULL) {
    fprintf(stderr, "INFO: No clan file! New file will be created.");
    create_clanfile();
    fl = fopen(CLAN_FILE, "rt");
  }
  
  line_num += get_line(fl, buf);
  if (sscanf(buf, "%d", &clannum) != 1) {
    fprintf(stderr, "Format error in clan, line %d (number of clans)\n", line_num);
    exit(0);
  }
  /* Setup the global total number of clans */
  cnum = clannum;
  
  /* process each clan in order */
  for (clannum = 0; clannum < cnum; clannum++) {
    /* Get the info for the individual clans */
    line_num += get_line(fl, buf);
    if (sscanf(buf, "#%d", &tmp) != 1) {
      fprintf(stderr, "Format error in clan (No Unique GID), line %d\n", line_num);
      exit(0);
    }
    /* create some clan shaped memory space */
    if ((cptr = enqueue_clan()) != NULL) {
      cptr->number = tmp;
      
      /* setup the global number of next new clan number */
      if (!newc) {
        if (cptr->number != clannum) {
          newclan = clannum;
          newc = TRUE;
        }
      }

      if (newc) {
        if (newclan == cptr->number) {
          newclan = cnum;
          newc = FALSE;
        }
      } else
        newclan = cptr->number + 1;

      for(i = 0; i < NUM_CLAN_RANKS; i++) {
        cptr->rank_name[i] = NULL;
      }
     
      /* allocate space for applicants */
      for (i=0; i < MAX_CLAN_APPLICANTS; i++) 
        cptr->applicants[i] = NULL;

      /* Now get the name of the clan */
      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
        *ptr = '\0';
      cptr->name = strdup(buf);
      
      /* Now get the who string */
      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
        *ptr = '\0';
      cptr->member_look_str = strdup(buf);

      /* Now get guard and direction*/
      line_num += get_line(fl, buf);
      if (sscanf(buf, "%d %d %d", &tmp, &tmp1, &tmp2) != 3) {
        fprintf(stderr, "Format error in clan, line %d (guard, guard, direction)\n", line_num);
        exit(0);
      }
      cptr->guard[0] = tmp;
      cptr->guard[1] = tmp1;
      cptr->direction = tmp2;

      /* Now get war kills & entrance room */
      line_num += get_line(fl, buf);
      if (sscanf(buf, "%d %ld %d %d", &p_kill, &clangold, &clan_entr, &clan_recall) != 4) {
        fprintf(stderr, "Format error in clan, line %d (pkill, clan_gold, entrance room, recall)\n", line_num);
        exit(0);
      }
      cptr->pkill = p_kill;
      cptr->clan_gold = clangold;
      cptr->clan_entr_room = clan_entr;
      cptr->clan_recall = clan_recall;

      /* Skip this line it's just a header */
      line_num += get_line(fl, buf);
 
      /* Leader's name */     
      sscanf(buf, "%s", name);
      cptr->leadersname = strdup(name);

      /* get the ranks */
      for( i = (NUM_CLAN_RANKS-1); i >= 0; i--) {
        line_num += get_line(fl, buf);
        if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
          *ptr = '\0';
        cptr->rank_name[i] = strdup(buf);
      }
      
      /* Okay we have the rank names ... now for the applicants */
      for (i = 0; i < MAX_CLAN_APPLICANTS; i++) {
        line_num += get_line(fl, buf);
        /* We're done when we hit the $ character */
        if (*buf == '$')
          break;
        else if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
          *ptr = '\0';
        cptr->applicants[i] = strdup(buf);
      }
    } else break;
    /* process the next clan */
  }
  /* done processing clans -- close the file */
  fclose(fl);
}


void save_clans(void)
{
  FILE   *cfile;
  int    clannum = 0, i;
  struct clan_type *cptr = clan_info;
  
  if (cptr == NULL) {
    fprintf(stderr, "SYSERR: No clans to save!!\n");
    return;
  }
  if ((cfile = fopen(CLAN_FILE, "wt")) == NULL) {
    fprintf(stderr, "SYSERR: Cannot save clans!\n");
    exit(0);
  }
     
  /* Put the clans in order */
  order_clans();

  /* The total number of clans */
  fprintf(cfile, "%d\r\n", cnum);

  /* Save each clan */
  while (clannum < cnum && cptr != NULL) {
    fprintf(cfile,  "#%d\r\n"
		    "%s~\r\n"
        "%s~\r\n"
        "%d %d %d\r\n"
        "%d %ld %d %d\r\n"
        "%s\r\n", /* leader */
        cptr->number,   cptr->name, /* Applicants */
        cptr->member_look_str,
        cptr->guard[0], cptr->guard[1], cptr->direction,
        cptr->pkill, cptr->clan_gold, cptr->clan_entr_room, cptr->clan_recall, 
        cptr->leadersname);

    for(i = (NUM_CLAN_RANKS-1); i >= 0; i--) {
      fprintf(cfile, "%s~\r\n", cptr->rank_name[i]);
    }

    if (cptr->applicants[0] != NULL) {
      for (i = 0; i < MAX_CLAN_APPLICANTS; i++) {
        if (cptr->applicants[i] == NULL) 
          break;
        else
          fprintf(cfile, "%s~\r\n", cptr->applicants[i]);
      }
    }
    fprintf(cfile, "$\r\n");
    /* set the guard */
    for(i = 0; i<NUM_CLAN_GUARDS; i++) {
      if(mob_proto[real_mobile(cptr->guard[i])].player.clan != cptr->number) {
        mob_proto[real_mobile(cptr->guard[i])].player.clan = cptr->number;
        add_to_save_list(zone_table[real_zone_by_thing(cptr->guard[i])].number, SL_MOB);
      }
    }
    /* process the next clan */
    cptr = cptr->next;
    //clannum++;
  }
  /* done processing clans */
  fclose(cfile);
}

/* Apply to a clan for membership */
void do_apply( struct char_data *ch, struct clan_type *cptr )
{
  int i;
  
  if (IS_NPC(ch))
    return;
  
  if (!strcmp(cptr->leadersname, "NoOne")) {
    send_to_char(ch, "The clan is currently without a leader.\r\n");
    return;
  }

  for (i = 0; i < MAX_CLAN_APPLICANTS; i++) {
    if (cptr->applicants[i] == NULL)
      break;
    if (!strcmp(cptr->applicants[i], GET_NAME(ch))) {
      send_to_char(ch, "You have already applied to this clan.\r\n");
      return;
    }
  }
  if ((i < MAX_CLAN_APPLICANTS) && (cptr->applicants[i] == NULL)) {
    cptr->applicants[i] = strdup(GET_NAME(ch));
    send_to_char(ch, "You need to talk to  %s to notify them of your application.\r\n",
        cptr->leadersname);
  } else
    send_to_char(ch, "This clan is already being applied to by %d other players, try again later.\r\n",
      MAX_CLAN_APPLICANTS);

  save_clans(); 
  return;    
}

/* Accept an applicant into the clan */
void do_accept( struct char_data *ch, struct clan_type *cptr, struct char_data *vict )
{
  int i=0;
  
  if (vict != NULL && (GET_CLAN(vict) <= CLAN_NONE)) {
    GET_CLAN(vict) = GET_CLAN(ch);
    GET_CLAN_RANK(vict) = CLAN_NONE;

    GET_HOME(vict) = GET_HOME(ch);
    save_char(vict, NOWHERE);

    /* now remove the player from the petition list */
    for (i = 0; i < (MAX_CLAN_APPLICANTS-1); i++) {
      if (cptr->applicants[i+1]) {
        if (!strcmp(GET_NAME(vict), cptr->applicants[i]) )
          cptr->applicants[i] = strdup(cptr->applicants[i+1]);
        else 
          continue;
      } else {
        free(cptr->applicants[i]);
        cptr->applicants[i] = NULL;
      }
    }
    save_clans();
  }
  return;
}

/* Reject an applicant from a clan/Withdraw an application */
void do_reject( struct char_data *ch, struct clan_type *cptr, struct char_data *vict)
{
  int i=0;

  if (vict != ch) {
    if (vict != NULL) {
      send_to_char(vict, "You have been rejected in your application to join %s!\r\n",
         cptr->name);
    }
  } else {
      for (i=0; i < MAX_CLAN_APPLICANTS; i++) {
        if (!cptr->applicants[i]) {
          send_to_char(ch, "You are not applying to the clan...\r\n");
          break;
        }
        if (strcmp(cptr->applicants[i], GET_NAME(ch))) {
          send_to_char(ch, "You aren't applying to the clan...\r\n");
          break;
        }
      }
  }
  GET_CLAN(vict) = CLAN_NONE;
  GET_CLAN_RANK(vict) = 0;
  save_char(vict, NOWHERE);
  /* now remove the player from the petition list */
  for (i = 0; i < (MAX_CLAN_APPLICANTS-1); i++) {
    if (cptr->applicants[i+1]) {
      if (!strcmp(GET_NAME(vict), cptr->applicants[i]) )
        cptr->applicants[i] = strdup(cptr->applicants[i+1]);
      else 
        continue;
    } else {
      free(cptr->applicants[i]);
      cptr->applicants[i] = NULL;
    }
  }
  save_clans();
  return;
}

/* Dismiss a member from the clan/Resign from the clan */
void do_dismiss( struct char_data *ch, struct clan_type *cptr, struct char_data *vict )
{
  if (vict != ch){
    if (vict != NULL && GET_CLAN(vict) == GET_CLAN(ch)) {
      if (GET_CLAN_RANK(ch) <= GET_CLAN_RANK(vict)) {
        send_to_char(ch, "You may not dismiss those of equal or greater clan status than yourself!\r\n");
        return;
      } else 
        send_to_char(vict, "You have been dismissed from the clan of %s.\r\n", cptr->name);
    }
  } else if (GET_CLAN_RANK(vict) == CLAN_LEADER) {
    GET_CLAN_RANK(vict)--;
    send_to_char(ch, "You have resigned as leader.\r\n");
    cptr->leadersname = strdup("NoOne");
    save_clans();
    return;
  }  /* ok it's not the leader resigning, so we're good to go */
  GET_CLAN(vict) = CLAN_NONE;
  GET_CLAN_RANK(vict) = CLAN_NONE;
  GET_HOME(vict) = 1;
  save_char(vict, NOWHERE);
  save_clans();
  return;
}

/* 
 * Find the char, if not online, load char up. Currently unused. If you can get this to work
 * for you, please send me an update!  Where you get_char_vis in do_clan, substitute this
 * and theoretically you can do anything with the victim offline.
 * Theoretically.  Can also be used in cedit when saving the clan to set leader and sponsor
 * at clan creation/edit.
 */
struct char_data *find_clan_char( struct char_data *ch, char *arg )
{
  struct char_data *vict = NULL, *cbuf = NULL;

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    /* we need to load the file up :( */
   CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    if (load_char(arg, vict) > -1) {
      vict = cbuf;
    }
  }
  return(vict);
}

/* Check to see if victim is applying to the clan */
int app_check( struct clan_type *cptr, struct char_data *applicant )
{
  int i;
  
  for (i = 0; i < MAX_CLAN_APPLICANTS; i++) {
    if (cptr->applicants[i] != NULL)
      if (!strcmp(GET_NAME(applicant), cptr->applicants[i]) || is_abbrev(GET_NAME(applicant), cptr->applicants[i])) {
        return(TRUE);
      }
  }

  return(FALSE);
}

/* List members of the clan online */
void show_clan_who(struct char_data *ch, struct clan_type *clan)
{
  struct descriptor_data *d;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  snprintf(buf, sizeof(buf),
     "\r\nMembers of %s Online\r\n"
     "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\r\n",
    clan->name);

  for (d = descriptor_list; d; d = d->next ) {
    if (!(d->connected) && (GET_CLAN(d->character) == GET_CLAN(ch))) {
      if(!CAN_SEE(ch, d->character)) 
        continue;
      snprintf(buf2, sizeof(buf2), "%s %s (%s)\r\n", GET_NAME(d->character), GET_TITLE(d->character), 
        (GET_CLAN_RANK(d->character) <= CLAN_LEADER) ? clan->rank_name[GET_CLAN_RANK(d->character)] : "error!");
      strncat(buf, buf2, sizeof(buf));
    }
  }
  page_string(ch->desc, buf, TRUE); 
  return;
}

/* Send message to all members of clan */
void perform_ctell( struct char_data *ch, const char *messg, ... )
{
  struct descriptor_data *d;
  va_list args;

  if (messg == NULL)
    return;

  for ( d = descriptor_list; d; d = d->next ) {
    if (STATE(d) != CON_PLAYING)
      continue;

    if ( GET_CLAN(d->character) == GET_CLAN(ch) || 
( GET_LEVEL(d->character) >= LVL_DEITY && PRF_FLAGGED(d->character, 
PRF_ALLCTELL))) {
      send_to_char(d->character, "%s[%s] %s: ", CCGRN(d->character, C_NRM),
        get_clan_name(GET_CLAN(ch)), GET_NAME(ch)); 
      va_start(args, messg);
      vwrite_to_output(d, messg, args);
      va_end(args);
      send_to_char(d->character, "%s\r\n", CCNRM(d->character, C_NRM));
    }
  }
  return;
}

/* Send clan infochannel message to all members of clan */
void perform_cinfo( int clan_number, const char *messg, ... )
{
  struct descriptor_data *d;
  va_list args;

  if (messg == NULL)
    return;

  for ( d = descriptor_list; d; d = d->next ) {
    if (STATE(d) != CON_PLAYING) 
      continue;

    if ( GET_CLAN(d->character) == clan_number || GET_LEVEL(d->character) >= LVL_DEITY) {
      send_to_char( d->character, "%s[Clan Announcement (%s)] ",  
        CCCYN(d->character, C_SPR), get_blank_clan_name(clan_number));

      va_start(args, messg);
      vwrite_to_output(d, messg, args);
      va_end(args);
      send_to_char( d->character, " ]%s\r\n", CCNRM(d->character, C_NRM));
    }
  }
  return;
}

/* ctell for high level imms */
void perform_imm_ctell( struct char_data *ch, int clan, const char *messg, ... )
{
  struct descriptor_data *d;
  va_list args;

  if (messg == NULL)
    return;

  if (clan == 0) {
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING)
        continue;
      if ( GET_CLAN(d->character) > CLAN_NONE ) {
        send_to_char( d->character, "%s%s tells all clans, '",
          CCRED(d->character, C_NRM), GET_NAME(ch));
        va_start(args, messg);
        vwrite_to_output(d, messg, args);
        va_end(args);
        send_to_char( d->character, "'%s\r\n", CCNRM(d->character, C_NRM));
      }
    }
  } else {
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING)
        continue;
      if (GET_CLAN(d->character) == clan || GET_LEVEL(d->character) >= LVL_DEITY) {
        send_to_char( d->character, "%s%s tells your clan, '",
          CCYEL(d->character, C_NRM), GET_NAME(ch));
        va_start(args, messg);
        vwrite_to_output(d, messg, args);
        va_end(args);
        send_to_char( d->character, "'%s\r\n", CCNRM(d->character, C_NRM));
      }
    }
  }

  return;
}

/* The main function */
ACMD(do_clan)
{

 send_to_char(ch, "CLAN is not yet implemented!\r\n");
   return;
  struct clan_type *cptr = NULL;
  struct char_data *victim = NULL;
  int i, clan;
  long amount = 0;
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];
  bool app = FALSE;

  if (IS_NPC(ch))
    return;

  if ( (GET_CLAN(ch) == CLAN_NONE || GET_CLAN(ch) == CLAN_UNDEFINED) &&
(subcmd != SCMD_CLAN_APPLY) && (subcmd != SCMD_CLAN_REVOKE) ) {
    send_to_char(ch, "You don't belong to any clan.\r\n");
    return;
  }

  for (cptr = clan_info; cptr && cptr->number != GET_CLAN(ch); cptr = cptr->next);

  if (cptr == NULL) {
    send_to_char(ch, "That clan does not exist.\r\n");
    return;
  }

  switch(subcmd){

/*** cwho ***/
  case SCMD_CLAN_WHO:
    show_clan_who(ch, cptr);
    break;

/*** ctell ***/

  case SCMD_CLAN_TELL:
    if (!PRF_FLAGGED(ch, PRF_CLANTALK)) {
      send_to_char(ch, "You have turned off your ability to clan talk.\r\n");
      return;
    }

    skip_spaces(&argument);
    if (!*argument)
      send_to_char(ch, "Yes... but what do you want to ctell?\r\n");
    else {
      if (GET_LEVEL(ch) < LVL_DEITY)
        perform_ctell( ch, argument );
      else {
        argument = one_argument(argument, arg1);
        skip_spaces(&argument);
        if (is_number(arg1) && !*argument) {
          send_to_char(ch, "Yes... but what do you want to ctell?\r\n");
          return;
        }
        if (is_number(arg1)) {
          clan = atoi(arg1);
          for (cptr = clan_info; cptr && cptr->number != clan; cptr = cptr->next);
          if (cptr == NULL) {
            send_to_char(ch, "That clan does not exist.\r\n");
            return;
          }
          perform_imm_ctell(ch, clan, argument);
        } else if (!str_cmp("all", arg1)) {
          clan = 0;
          perform_imm_ctell(ch, clan, argument);
        } else {
          perform_ctell(ch, "%s %s", arg1, argument);
        }
      }
    }
    break;

/*** capply ***/

  case SCMD_CLAN_APPLY:
    if (GET_CLAN(ch) > CLAN_NONE) {
      send_to_char(ch, "You already belong to a clan.\r\n");
      return;
    }
    skip_spaces(&argument);
    if (!*argument) {
      send_to_char(ch, "Which clan do you want to apply to for membership?\r\n");
      for (cptr = 0; cptr; cptr = cptr->next) {
        send_to_char(ch, "%2d %s\r\n", cptr->number, cptr->name);
      }
      return;
    }
    i = atoi(argument);
    for (cptr = clan_info; cptr && cptr->number != i; cptr = cptr->next);
    if (cptr == NULL) {
      send_to_char(ch, "That clan does not exist.\r\n");
      return;
    }
    do_apply( ch, cptr );
    break;

/*** creject ***/

  case SCMD_CLAN_REJECT:
    if (GET_CLAN_RANK(ch) != CLAN_LEADER) {
      send_to_char(ch, "Only the clan leader can reject applicants.\r\n");
      return;
    }
    skip_spaces(&argument);
    if (!*argument) {
      send_to_char(ch, "Current applicants are:\r\n");
      for (i = 0; i < MAX_CLAN_APPLICANTS; i++) {
        if (cptr->applicants[i] != NULL) {
          send_to_char(ch, "%s\r\n", cptr->applicants[i]);
          app = TRUE;
        }
      }
      if (!app)
        send_to_char(ch, "None.\r\n");
      send_to_char(ch, "Whom do you wish to reject from the clan?\r\n");
      return;
    }
    if (!(victim = get_char_vis(ch, argument, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, NOPERSON);
      return;
    } else {
      if (!app_check(cptr, victim)) {
        send_to_char(ch, "There is no one applying your clan by that name.\r\n");
        return;
      } 
      do_reject( ch, cptr, victim );
      send_to_char(ch, "Rejected.\r\n");
    }
    break;

/*** cwithdraw ***/

  case SCMD_CLAN_REVOKE:
    skip_spaces(&argument);
    if (!*argument) {
      send_to_char(ch, "Revoke application to which clan?\r\n");
      return;
    }
    if (!is_number(argument)) {
      send_to_char(ch, "You cannot revoke someone else's application.\r\n");
      return;
    }
    for (cptr = clan_info; cptr && cptr->number != atoi(argument); cptr = cptr->next);
    if (cptr == NULL) {
      send_to_char(ch, "That clan does not exist.\r\n");
      return;
    }
    do_reject( ch, cptr, ch );
    return;
    break;

/*** caccept ***/

  case SCMD_CLAN_ACCEPT: 
    if (GET_CLAN_RANK(ch) < CLAN_ADVISOR) {
      send_to_char(ch, "Only those at or above %s can accept new members!\r\n", cptr->rank_name[CLAN_ADVISOR]);
      return;
    }
    skip_spaces(&argument);
    if (!*argument) {
      send_to_char(ch, "Current applicants are:\r\n");
      for (i = 0; i < MAX_CLAN_APPLICANTS; i++) {
        if (cptr->applicants[i] != NULL) {
          send_to_char(ch, "%s\r\n", cptr->applicants[i]);
          app = TRUE;
        }
      }
      if (!app)
        send_to_char(ch, "None.\r\n");
      send_to_char(ch, "Whom do you wish to accept into the clan?\r\n");
      return;
    }
    if (!(victim = get_char_vis(ch, argument, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, NOPERSON);
      return;
    } else {
      if (!app_check(cptr, victim)) {
        send_to_char(ch, "There is no one applying to your clan by that name.\r\n");
        return;
      }
      do_accept( ch, cptr, victim );
      perform_cinfo(GET_CLAN(ch), "%s is now a member of the clan", GET_NAME(victim));
      send_to_char(ch, "New member accepted.\r\n");
    }
    break;

/*** cdismiss ***/

  case SCMD_CLAN_DISMISS:  
      if (GET_CLAN_RANK(ch) != CLAN_LEADER) {
        send_to_char(ch, "Only the clan leader can dismiss members.\r\n");
        return;
      }
      skip_spaces(&argument);
      if (!*argument) {
        send_to_char(ch, "Whom do you wish to dismiss?\r\n");
        return;
      }
      if (!(victim = get_char_vis(ch, argument, NULL, FIND_CHAR_WORLD))) {
        send_to_char(ch, NOPERSON);
        return;
      } else {
        do_dismiss( ch, cptr, victim );
        send_to_char(ch, "Dismissed.\r\n");
      }
      break;

/*** cresign ***/

  case SCMD_CLAN_RESIGN:  
    skip_spaces(&argument);
    if (*argument) {
      send_to_char(ch, "You cannot force other members to resign.\r\n");
      return;
    }
    if (GET_CLAN_RANK(ch) == CLAN_LEADER) {
      perform_cinfo(GET_CLAN(ch), "%s has resigned as leader of the clan.", GET_NAME(ch));
    } else {
      perform_cinfo(GET_CLAN(ch), "%s has resigned from the clan.", GET_NAME(ch));
    }
    do_dismiss( ch, cptr, ch );
    break;

  case SCMD_CLAN_PROMOTE:
    if (GET_CLAN_RANK(ch) < CLAN_ADVISOR) {
      send_to_char(ch, "You have to be rank %s or higher to promote someone.\r\n", cptr->rank_name[CLAN_ADVISOR]);
      return;
    }
    skip_spaces(&argument);
    if (!*argument) {
      send_to_char(ch, "Whom do you wish to promote?\r\n");
      return;
    }
    if (!(victim = get_char_vis(ch, argument, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, NOPERSON);
      return;
    } 
    if (GET_CLAN(ch) != GET_CLAN(victim)) {
      send_to_char(ch, "They aren't in your clan!\r\n");
      return;
    }
    if (GET_CLAN_RANK(ch) <= GET_CLAN_RANK(victim)) {
      send_to_char(ch, "You cannot promote someone of higher or equal rank.\r\n");
      return;
    }
    if (GET_CLAN_RANK(victim) >= CLAN_ADVISOR) {
      send_to_char(ch, "Leaders must be set by an immortal.\r\n");
      return;
    }
    GET_CLAN_RANK(victim)++;
    perform_cinfo(GET_CLAN(ch), "%s has been promoted to %s by %s", GET_NAME(victim), 
        get_rank_name(GET_CLAN(victim), GET_CLAN_RANK(victim)), GET_NAME(ch));
    break;

  case SCMD_CLAN_DEMOTE:
    if (GET_CLAN_RANK(ch) < CLAN_ADVISOR) {
      send_to_char(ch, "You have to be rank %s or higher to demote someone.\r\n", cptr->rank_name[CLAN_ADVISOR]);
      return;
    }
    skip_spaces(&argument);
    if (!*argument) {
      send_to_char(ch, "Whom do you wish to demote?\r\n");
      return;
    }
    if (!(victim = get_char_vis(ch, argument, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, NOPERSON);
      return;
    } 
    if (GET_CLAN(ch) != GET_CLAN(victim)) {
      send_to_char(ch, "They aren't in your clan!\r\n");
      return;
    }
    if (GET_CLAN_RANK(ch) <= GET_CLAN_RANK(victim)) {
      send_to_char(ch, "You cannot demote someone of higher or equal rank.\r\n");
      return;
    }
    if (GET_CLAN_RANK(victim) == CLAN_NONE) {
      send_to_char(ch, "They are already at the lowest rank.\r\n");
      return;
    }
    GET_CLAN_RANK(victim)--;
    perform_cinfo(GET_CLAN(ch), "%s has been demoted to %s by %s", GET_NAME(victim), 
        get_rank_name(GET_CLAN(victim), GET_CLAN_RANK(victim)), GET_NAME(ch));
    break;

  case SCMD_CLAN_DEPOSIT:
    skip_spaces(&argument);
    if (!*argument) {
      send_to_char(ch, "How much do you wish to deposit?\r\n");
      return;
    }
    one_argument(argument, arg);
    if((amount = atol(arg)) < 0) {
      send_to_char(ch, "You cannot deposit a negative amount in your clan's account.\r\n");
      return;
    }
    if(amount > GET_GOLD(ch)) {
      send_to_char(ch, "You do not have that much gold in your personal account.\r\n");
      return;
    }
    if(amount > 2000000000 || (amount + cptr->clan_gold) > 2000000000) {
      send_to_char(ch, "The clan account cannot hold more than 2 billion coins.\r\n");
      return;
    }
    send_to_char(ch, "You deposit %ld coins into your clan account (%s%s%s).\r\n", amount, CCCYN(ch, C_SPR), cptr->name, CCNRM(ch, C_SPR));
    mudlog(NRM, LVL_DEITY, TRUE, "CLAN GOLD: %s deposits %ld coins to %s.", GET_NAME(ch), amount, cptr->name);
    GET_GOLD(ch) -= amount;
       cptr->clan_gold += amount;
    save_clans();
    save_char(ch, NOWHERE);
    break;

  case SCMD_CLAN_WITHDRAW_GOLD:
    if(GET_CLAN_RANK(ch) < CLAN_LEADER) {
      send_to_char(ch, "Only clan leaders can withdraw clan funds.\r\n");
      return;
    }
    skip_spaces(&argument);
    if (!*argument) {
      send_to_char(ch, "How much do you wish to withdraw?\r\n");
      return;
    }
    one_argument(argument, arg);
    if((amount = atol(arg)) < 0) {
      send_to_char(ch, "You cannot withdraw a negative amount from your clan's account.\r\n");
      return;
    }
    if(amount > cptr->clan_gold) {
      send_to_char(ch, "You do not have that much gold in your clan's account.\r\n");
      return;
    }
    if(amount > 2000000000 || (amount + GET_GOLD(ch)) > 2000000000) {
      send_to_char(ch, "You cannot have over 2 billion coins on hand.\r\n");
      return;
    }
    send_to_char(ch, "You withdraw %ld coins from your clan account (%s).\r\n", amount, cptr->name);
    mudlog(NRM, LVL_DEITY, TRUE, "CLAN GOLD: %s withdraws %ld coins from %s.", GET_NAME(ch), amount, cptr->name);
    GET_GOLD(ch) += amount;
    cptr->clan_gold -= amount;
    save_clans();
    save_char(ch, NOWHERE);
    break;
 
  case SCMD_CLAN_BALANCE:
    send_to_char(ch, "%s has %ld coins deposited in the clan account.\r\n", cptr->name, cptr->clan_gold);
    break;

  default:
    return;
  }
}

/* Helper functions for clan information */
char *get_clan_name(int clan)
{
  struct clan_type *cptr;
  char *clan_name;

  for (cptr = clan_info; cptr && cptr->number != clan; cptr = cptr->next);
  if(cptr != NULL)
    clan_name = strdup(cptr->member_look_str);
  else
    clan_name = strdup("Invalid Clan");

  return (clan_name);
}

char *get_blank_clan_name(int clan)
{
  struct clan_type *cptr;
  char *clan_name;

  for (cptr = clan_info; cptr && cptr->number != clan; cptr = cptr->next);
  clan_name = strdup(cptr->name);

  return (clan_name);
}

char *get_rank_name(int clan, int rank)
{
  char *rank_name;
  struct clan_type *cptr;

  for (cptr = clan_info; cptr && cptr->number != clan; cptr = cptr->next);
  if(cptr != NULL) 
    rank_name = strdup(cptr->rank_name[rank]);
  else
    rank_name = strdup("Invalid Rank");

  return(rank_name);
}


/* procedure to show clan info to morts */
ACMD(do_show_clan)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int i = 0;
  struct clan_type *cptr;

  snprintf(buf, sizeof(buf), "Clans\r\n"
    "---------------------------------------------------------------\r\n");
  for (cptr = clan_info; cptr; cptr = cptr->next) {
    if (cptr->number == 0)
      continue;
    snprintf(buf2, sizeof(buf2), "Clan #%2d %s%s%s (%s%s%s)\r\n%s%s: %s%s\r\n", 
      cptr->number, CCCYN(ch, C_SPR), cptr->name, 
CCNRM(ch, C_SPR), CCCYN(ch, C_SPR), cptr->member_look_str,
CCNRM(ch, C_SPR), CCCYN(ch, C_SPR),
cptr->rank_name[CLAN_LEADER], cptr->leadersname,CCNRM(ch, C_SPR));
    strncat(buf, buf2, sizeof(buf));
    for(i=(CLAN_ADVISOR); i>=0; i--) {
      snprintf(buf2, sizeof(buf2), "Rank %d: %s\r\n", i+1, cptr->rank_name[i]);
      strncat(buf, buf2, sizeof(buf));
    }
    strncat(buf, "\r\n", sizeof(buf));
  }
  page_string(ch->desc, buf, 1);
}

/* Special for clan guards */
SPECIAL(clan_guard)
{
  struct char_data *guard = (struct char_data *) me;
  struct clan_type *cptr;
  char *buf2 = "This is a restricted area.  Keep out!";
  char buf[MAX_STRING_LENGTH];

  for (cptr = clan_info; cptr->number != GET_CLAN(guard); cptr = cptr->next);

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
    return (FALSE);
  
  if ((cmd) == (cptr->direction + 1)) {
    if (GET_CLAN(ch) != GET_CLAN(guard)) {
      act("The guard blocks your way.", FALSE, ch, 0, 0, TO_CHAR);
      act("The guard blocks $n's way.", FALSE, ch, 0, 0, TO_ROOM);
      do_say(me, buf2, 0, 0);
      return (TRUE);
    } else {
      snprintf(buf, sizeof(buf), "Welcome home, %s!", GET_NAME(ch));
      do_say(me, buf, 0, 0);
    }
  }

  return (FALSE);
}
