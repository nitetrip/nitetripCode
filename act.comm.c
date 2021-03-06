/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
//  a test for git

#include "conf.h"
#include "sysdep.h"
// Testing git repo

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"

/* Stuff for gen_comm */
char buf1[MAX_INPUT_LENGTH];
char buf2[MAX_INPUT_LENGTH];
char buf3[MAX_INPUT_LENGTH];
char buf4[MAX_INPUT_LENGTH];
char buf5[MAX_INPUT_LENGTH];


/* extern variables */
extern int level_can_shout;
extern int shout_move_cost;
extern char *clan_names_output[];

/* local functions */

void clanlog(struct char_data *ch, const char *str, ...);
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
int is_tell_ok(struct char_data *ch, struct char_data *vict);
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);
ACMD(do_beep);

ACMD(do_beep)
{
  struct descriptor_data *d;
  struct char_data *vict;
  char buf2[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  half_chop(argument, arg, buf2);

   if (!*arg)
    send_to_char(ch, "Whom do you wish to beep?\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "\007\007%s", buf2);
    if (!str_cmp(arg, "all")) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
    }
    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(ch, "%s", OK);
    }
  }
}


ACMD(do_say)
{
  struct char_data *i;
  struct descriptor_data *j;
  skip_spaces(&argument);
  if (AFF_FLAGGED(ch, AFF_SILENCE)){
     send_to_char(ch, "Your vocal chords are too tight to move!!\r\n");
     return;
     }

  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
  else {

  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
  {
//    if (STATE(i->desc) == CON_SWITCHED) FIXME needs to be like perform_act()
    if (!PLR_FLAGGED(i, PLR_WRITING) && (AWAKE(i) || CAN_SEE_ASLEEP(i))) {
	  if (IN_ROOM(i) == IN_ROOM(ch))
		  {
	  if (CAN_SEE(i, ch) && (ch != i))
		  {
		  snprintf(buf4, sizeof(buf4), "%s", GET_NAME(ch));  /* Grab char name */
                  send_to_char(i, "%s says, '%s'\r\n", buf4, argument);
		  }
	  else
			  {
	  if (ch != i)
             send_to_char(i, "Someone says, '%s'\r\n", argument);
			  }
		  }
		}
	  }

/* See say while writing */
 for (j = descriptor_list; j; j = j->next) 
 {
   if (IS_EDITING(j) && j != ch->desc && j->character && (GET_LEVEL(j->character) >= LVL_BUILDER) && (IN_ROOM(j->character) == IN_ROOM(ch)) && AWAKE(j->character))
	  {
      if (CAN_SEE(j->character, ch))
		  {
		  snprintf(buf4, sizeof(buf4), "%s", GET_NAME(ch));  /* Grab char name */
          send_to_char(j->character, "%s says, '%s'\r\n", buf4, argument);
		  }
	  else
	  	  send_to_char(j->character, "Someone says, '%s'\r\n", argument);
	   }
  }
/* End of See say while writing */

   if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", OK);
    else {
      delete_doubledollar(argument);
      send_to_char(ch, "You say, '%s'\r\n", argument);
    }
  }
  /* trigger check */
  speech_mtrigger(ch, argument);
  speech_wtrigger(ch, argument);
}


ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

  skip_spaces(&argument);

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not the member of a group!\r\n");
    return;
  }
  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to group-say?\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    if (ch->master)
      k = ch->master;
    else
      k = ch;

    snprintf(buf, sizeof(buf), "%s$n%s tells the group, '%s'%s",
        KBCN,
        KBBL,
        argument,
        KNRM);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
      act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch))
	act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", OK);
    else
      send_to_char(ch, "%sYou tell the group, '%s'%s\r\n",
        KBBL,
        argument,
        KNRM);
  }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  char buf[MAX_STRING_LENGTH];
  snprintf(buf4, sizeof(buf4), "%s", GET_NAME(ch));  /* Grab char name */


  if CAN_SEE(vict, ch)
  snprintf(buf, sizeof(buf), "%s%s%s%s tells you, '%s'%s", CCBGR(vict, C_SPR), buf4, CCNRM(vict, C_SPR), CCGRN(vict, C_SPR), arg, CCNRM(vict, C_SPR));
  else
  snprintf(buf, sizeof(buf), "%sSomeone%s%s tells you, '%s'%s", CCBGR(vict, C_SPR), CCNRM(vict, C_SPR), CCGRN(vict, C_SPR), arg, CCNRM(vict, C_SPR));
  send_to_char(vict, "%s\r\n", buf);
/*    mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "%s told %s to %s while INVIS.", GET_NAME(ch), arg, GET_NAME(vict)); */

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", OK);
  else {
    snprintf(buf, sizeof(buf), "%sYou tell $N, '%s'%s", CCGRN(ch, C_SPR), arg, CCNRM(ch, C_SPR));
    act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  }
  if (!IS_NPC(vict) && !IS_NPC(ch))
    GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
  struct descriptor_data *l;
  if (ch == vict)
    send_to_char(ch, "You try to tell yourself something.\r\n");
  else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char(ch, "You can't tell other people while you have notell on.\r\n");
  else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF))
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
	{
	  if (GET_LEVEL(vict) < LVL_BUILDER)
      act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	  else
/* This should be trimmed down in case of a resource hogging MUD or if tells go slow */
	  {
		for (l = descriptor_list ; GET_IDNUM(l->character) != GET_IDNUM(vict) ; l = l->next)
		;
		if (IS_EDITING(l))
		return (TRUE);
		else
        act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
		}
	}
  else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF))
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    return (TRUE);

  return (FALSE);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict = NULL;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  half_chop(argument, buf, buf2);
  if (AFF_FLAGGED(ch, AFF_SILENCE)){
    send_to_char(ch, "Your vocal chords are too tight to move!!\r\n");
    return;
    }
  if (!*buf || !*buf2)
    send_to_char(ch, "Who do you wish to tell what??\r\n");
  else if (GET_LEVEL(ch) < LVL_SAINT && !(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", NOPERSON);
  else if (GET_LEVEL(ch) >= LVL_SAINT && !(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", NOPERSON);
  else if (is_tell_ok(ch, vict))
    perform_tell(ch, vict, buf2);
}


ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
    return;
  if (AFF_FLAGGED(ch, AFF_SILENCE)) {
    send_to_char(ch, "Your vocal chords are too tight to move!!\r\n");
    return;
    }
  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char(ch, "You have nobody to reply to!\r\n");
  else if (!*argument)
    send_to_char(ch, "What is your reply?\r\n");
  else {
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again.
     *
    *
     * XXX: A descriptor list based search would be faster although
     *      we could not find link dead people.  Not that they can
     *      hear tells anyway. :) -gg 2/24/98
     */
    while (tch != NULL && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)) && (CAN_SEE(ch, tch)))
      tch = tch->next;

    if (tch == NULL)
      send_to_char(ch, "They are no longer playing.\r\n");
      else if ((GET_INVIS_LEV(tch)) > (GET_LEVEL(ch))){
        send_to_char(ch, "You must be hearing voices!\r\n");
        send_to_char(tch, "%s tried to reply %s to you, but you are invis.\r\n", GET_NAME(ch), argument);
        mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "%s tried to REPLY %s to an INVIS %s", GET_NAME(tch), argument, GET_NAME(ch));
        return;
        }
       else if (is_tell_ok(ch, tch))
      perform_tell(ch, tch, argument);
  }
}


ACMD(do_spec_comm)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  switch (subcmd) {
  case SCMD_WHISPER:
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
    break;

  case SCMD_ASK:
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
    break;

  default:
    action_sing = "oops";
    action_plur = "oopses";
    action_others = "$n is tongue-tied trying to speak with $N.";
    break;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "You can't get your mouth close enough to your ear...\r\n");
  else {
    char buf1[MAX_STRING_LENGTH];

    snprintf(buf1, sizeof(buf1), "$n %s you, '%s'", action_plur, buf2);
    act(buf1, FALSE, ch, 0, vict, TO_VICT);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", OK);
    else
      send_to_char(ch, "You %s %s, '%s'\r\n", action_sing, GET_NAME(vict), buf2);
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
  }
}


/*
 * buf1, buf2 = MAX_OBJECT_NAME_LENGTH
 *	(if it existed)
 */
ACMD(do_write)
{
  struct obj_data *paper, *pen = NULL;
  char *papername, *penname;
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* nothing was delivered */
    send_to_char(ch, "Write?  With what?  ON what?  What are you trying to do?!?\r\n");
    return;
  }
  if (*penname) {		/* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", papername);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", penname);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
      pen = paper;
      paper = NULL;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char(ch, "That thing has nothing to do with writing.\r\n");
      return;
    }
    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_HOLD)) {
      send_to_char(ch, "You can't write with %s %s alone.\r\n", AN(papername), papername);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
      send_to_char(ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_HOLD);
    else
      pen = GET_EQ(ch, WEAR_HOLD);
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else {
    char *backstr = NULL;

    /* Something on it, display it as that's in input buffer. */
    if (paper->action_description) {
      backstr = strdup(paper->action_description);
      send_to_char(ch, "There's something written on it already:\r\n");
      send_to_char(ch, paper->action_description);
    }

    /* we can write - hooray! */
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    send_editor_help(ch->desc);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
  }
}



ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;
  char buf2[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char(ch, "Monsters can't page.. go away.\r\n");
  else if (!*arg)
    send_to_char(ch, "Whom do you wish to page?\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "\007\007*$n* %s", buf2);
    if (!str_cmp(arg, "all")) {
      if (GET_LEVEL(ch) > LVL_DEITY) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char(ch, "You will never be godly enough to do that!\r\n");
      return;
    }
    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(ch, "%s", OK);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(ch, "There is no such person in the game!\r\n");
  }
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];
  char buf1[MAX_INPUT_LENGTH];
  
  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    0
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */
  const char *com_msgs[][4] = {
    {"You cannot yell!!\r\n",
      "Yell",
      "",
    KBMG},

    {"You cannot shout!!\r\n",
      "Shout",
      "Turn off your noshout flag first!\r\n",
    KBRD},

    {"You cannot gossip!!\r\n",
      "Gossip",
      "You aren't even on the channel!\r\n",
    KMAG},

    {"You cannot auction!!\r\n",
      "Auction",
      "You aren't even on the channel!\r\n",
    KYEL},
    
    {"You cannot ctell!!\r\n",
      "Clan",
      "You aren't even on the channel!\r\n",
    KBBL}
  };

  /* to keep pets, etc from being ordered to shout
  if (!ch->desc)
    return;                                       */
 if (AFF_FLAGGED(ch, AFF_SILENCE)) {
    send_to_char(ch, "Your vocal chords are too tight to move!!\r\n");
    return; }

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "%s", com_msgs[subcmd][0]);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF)) {
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
    return;
  }
  
  
  
  /* level_can_shout defined in config.c */
  if (GET_LEVEL(ch) < level_can_shout) {
    send_to_char(ch, "You must be at least level %d before you can %s.\r\n", level_can_shout, com_msgs[subcmd][1]);
    return;
  }
  /* make sure the char is on the channel */
  if (PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(ch, "%s", com_msgs[subcmd][2]);
    return;
  }
  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument) {
    send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
    return;
  }
  if (subcmd == SCMD_SHOUT) {
    if (GET_MOVE(ch) < shout_move_cost) {
      send_to_char(ch, "You're too exhausted to shout.\r\n");
      return;
    } else
      GET_MOVE(ch) -= shout_move_cost;
  }
  /* set up the color on code */
  strlcpy(color_on, com_msgs[subcmd][3], sizeof(color_on));

  /* first, set up strings to be given to the communicator */
  snprintf(buf4, sizeof(buf4), "%s", GET_NAME(ch));  /* Grab char name */
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", OK);
  else
     {
  if (subcmd != SCMD_YELL && subcmd != SCMD_SHOUT && subcmd != SCMD_CTELL)
    {
    send_to_char(ch, "%s[%s] %s: %s %s\r\n", COLOR_LEV(ch) >= C_CMP ? color_on : "", com_msgs[subcmd][1], GET_NAME(ch), argument, CCNRM(ch, C_CMP));
    }  
  else 
      {
      if (subcmd == SCMD_YELL)
      send_to_char(ch, "%sYou yell, '%s'%s\r\n", COLOR_LEV(ch) >= C_CMP ? color_on : "", argument, CCNRM(ch, C_CMP)); 
      if (subcmd == SCMD_SHOUT)
	  send_to_char(ch, "%sYou shout, '%s'%s\r\n", COLOR_LEV(ch) >= C_CMP ? color_on : "", argument, CCNRM(ch, C_CMP)); 
      }
      }
  if (subcmd != SCMD_YELL && subcmd != SCMD_SHOUT && subcmd != SCMD_CTELL) 
  {
  snprintf(buf1, sizeof(buf1), "[%s] $n: %s", com_msgs[subcmd][1], argument);
  snprintf(buf2, sizeof(buf2), "[%s] Someone: %s", com_msgs[subcmd][1], argument);
  snprintf(buf3, sizeof(buf3), "[%s] %s: %s", com_msgs[subcmd][1], buf4, argument);
  }
  else
  {
  if (subcmd == SCMD_YELL)
  {
  snprintf(buf1, sizeof(buf1), "$n yells, '%s'", argument);
  snprintf(buf2, sizeof(buf2), "Someone yells, '%s'", argument);
  snprintf(buf3, sizeof(buf3), "%s yells, '%s'", buf4, argument);

  }
  if (subcmd == SCMD_SHOUT)
  {
  snprintf(buf1, sizeof(buf1), "$n shouts, '%s'", argument);
  snprintf(buf2, sizeof(buf2), "Someone shouts, '%s'", argument);
  snprintf(buf3, sizeof(buf3), "%s shouts, '%s'", buf4, argument);
  }

  }
  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if ((IS_PLAYING(i)) && i != ch->desc && i->character &&
	!PRF_FLAGGED(i->character, channels[subcmd]) &&
	!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_YELL && (GET_LEVEL(i->character) < 43 && ((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone) || (!AWAKE(i->character) && !CAN_SEE_ASLEEP(i->character)))))
      	continue;
	  if (CAN_SEE(i->character, ch))
		{
	  if (IS_EDITING(i) && GET_LEVEL(i->character) >= LVL_BUILDER)
	  			{
	  if (COLOR_LEV(i->character) >= C_NRM)
	  send_to_char(i->character, "%s", color_on);
	  send_to_char(i->character, "%s%s", buf3, "\r\n");
 	  if (COLOR_LEV(i->character) >= C_NRM)
      send_to_char(i->character, "%s", KNRM);
				}
	  else	  
			{
      if (!PLR_FLAGGED(i->character, PLR_WRITING))
			{
	  if (COLOR_LEV(i->character) >= C_NRM)
	  send_to_char(i->character, "%s", color_on);
	  act(buf1, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
 	  if (COLOR_LEV(i->character) >= C_NRM)
      send_to_char(i->character, "%s", KNRM);
				}
			}
		}
	  else
		{
	  if (IS_EDITING(i) && GET_LEVEL(i->character) >= LVL_BUILDER)
			{
	  if (COLOR_LEV(i->character) >= C_NRM)
	  send_to_char(i->character, "%s", color_on);
	  send_to_char(i->character, "%s%s", buf2, "\r\n");
 	  if (COLOR_LEV(i->character) >= C_NRM)
      send_to_char(i->character, "%s", KNRM);
				}
	  else	  
			{
      if (!PLR_FLAGGED(i->character, PLR_WRITING))
			{
	  if (COLOR_LEV(i->character) >= C_NRM)
	  send_to_char(i->character, "%s", color_on);
	  act(buf2, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
	  if (COLOR_LEV(i->character) >= C_NRM)
      send_to_char(i->character, "%s", KNRM);
				}
			}
		}

	}
  }
}

ACMD(do_qcomm)
{
  snprintf(buf4, sizeof(buf4), "%s", GET_NAME(ch));  /* Grab char name */
  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char(ch, "You aren't even part of the quest!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "%c%s?  Yes, fine, %s we must, but WHAT??\r\n", UPPER(*CMD_NAME), CMD_NAME + 1, CMD_NAME);
  else {
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *i;

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", OK);
    else if (subcmd == SCMD_QSAY) {
	snprintf(buf, sizeof(buf), "[Quest] %s: %s", buf4, argument);
		if (COLOR_LEV(ch) >= C_NRM)
		{
  			  snprintf(buf3, sizeof(buf3), "%s%s%s", KBYL, buf, KNRM);
		  	  send_to_char(ch, buf3);
		}
		else
		 {
		 if (COLOR_LEV(ch) >= C_NRM)
			  {
			  snprintf(buf3, sizeof(buf3), "%s%s%s", KBYL, buf, KNRM);
		  	  send_to_char(ch, buf3);
			  }
		 else
		 send_to_char(ch, buf);
	  }

    } else
      act(argument, FALSE, ch, 0, argument, TO_CHAR);

    if (subcmd == SCMD_QSAY)
	  {
	  snprintf(buf, sizeof(buf), "[Quest] %s: %s", buf4, argument);
      snprintf(buf2, sizeof(buf2), "[Quest] Someone: %s", argument);
	  }
	else
      strlcpy(buf, argument, sizeof(buf));

    for (i = descriptor_list; i; i = i->next)
      if ((IS_PLAYING(i)) && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST))
	  {
	  if ((IS_EDITING(i)) && (GET_LEVEL(i->character) > LVL_BUILDER))
		  {
	  	  if (CAN_SEE(i->character, ch))
		  {
          if (COLOR_LEV(i->character) >= C_NRM)
			  {
			  snprintf(buf3, sizeof(buf3), "%s%s%s\r\n", KBYL, buf, KNRM);
		  	  send_to_char(i->character, buf3);
			  }
           else
           send_to_char(i->character, buf);				
		  }
	  else
		  {
		 if (COLOR_LEV(i->character) >= C_NRM)
			  {
			  snprintf(buf3, sizeof(buf3), "%s%s%s", KBYL, buf2, KNRM);
		  	  send_to_char(i->character, buf3);
			  }
		 else
		 send_to_char(i->character, buf2);
		  }
	  }
      if (!PLR_FLAGGED(i->character, PLR_WRITING))
		  {
	  	  if (CAN_SEE(i->character, ch))
		  {
          if (COLOR_LEV(i->character) >= C_NRM)
			  {
			  snprintf(buf3, sizeof(buf3), "%s%s%s", KBYL, buf, KNRM);
		  	  send_to_char(i->character, buf3);
			  }
           else
           send_to_char(i->character, buf);				
		  }
	  else
		  {
		 if (COLOR_LEV(i->character) >= C_NRM)
			  {
			  snprintf(buf3, sizeof(buf3), "%s%s%s", KBYL, buf2, KNRM);
		  	  send_to_char(i->character, buf3);
			  }
		 else
		 send_to_char(i->character, buf2);
		  }
		  }
 
  }
  }
}


void clanlog(struct char_data *ch, const char *str, ...)
{
char buf[MAX_STRING_LENGTH];
va_list args;
struct descriptor_data *i;
  va_start(args, str);
  vsnprintf(buf, sizeof(buf) - 6, str, args);
  va_end(args);

for (i = descriptor_list; i; i = i->next) {

if (IS_PLAYING(i) && i != ch->desc && i->character) {

  } 
 }

}
