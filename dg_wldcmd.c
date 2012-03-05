/**************************************************************************
*  File: wldcmd.c                                                         *
*  Usage: contains the command_interpreter for rooms,                     *
*         room commands.                                                  *
*                                                                         *
*                                                                         *
*  $Author: galion $
*  $Date: 1996/08/05 03:27:07 $
*  $Revision: 3.12 $
**************************************************************************/

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "screen.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"

extern struct room_data *world;
extern const char *dirs[];
extern struct zone_data *zone_table;

void send_char_pos(struct char_data *ch, int dam);
void die(struct char_data * ch, struct char_data * killer);
void sub_write(char *arg, char_data *ch, byte find_invis, int targets);
void send_to_zone(char *messg, zone_rnum zrnum);
zone_rnum real_zone_by_thing(room_vnum vznum);
bitvector_t asciiflag_conv(char *flag);
char_data *get_char_by_room(room_data *room, char *name);
room_data *get_room(char *name);
obj_data *get_obj_by_room(room_data *room, char *name);
void wld_command_interpreter(room_data *room, char *argument);
void clanlog(struct char_data *ch, const char *str, ...);

#define WCMD(name)  \
    void (name)(room_data *room, char *argument, int cmd, int subcmd)


struct wld_command_info {
    char *command;
    void (*command_pointer)
           (room_data *room, char *argument, int cmd, int subcmd);
    int        subcmd;
};


/* do_wsend */
#define SCMD_WSEND        0
#define SCMD_WECHOAROUND  1



/* attaches room vnum to msg and sends it to script_log */
void wld_log(room_data *room, const char *format, ...)
{
  va_list args;
  char output[MAX_STRING_LENGTH];
    
  snprintf(output, sizeof(output), "Room %d :: %s", room->number, format);

  va_start(args, format);
  script_vlog(output, args);
  va_end(args);
}

/* sends str to room */
void act_to_room(char *str, room_data *room)
{
    /* no one is in the room */
    if (!room->people)
        return;

    /*
     * since you can't use act(..., TO_ROOM) for an room, send it
     * TO_ROOM and TO_CHAR for some char in the room.
     * (just dont use $n or you might get strange results)
     */
    act(str, FALSE, room->people, 0, 0, TO_ROOM);
    act(str, FALSE, room->people, 0, 0, TO_CHAR);
}



/* World commands */

/* prints the argument to all the rooms aroud the room */
WCMD(do_wasound)
{
    int  door;
 
    skip_spaces(&argument);

    if (!*argument) {
        wld_log(room, "wasound called with no argument");
        return;
    }
  
    for (door = 0; door < NUM_OF_DIRS; door++) {
        struct room_direction_data *newexit;
        
        if ((newexit = room->dir_option[door]) && (newexit->to_room != NOWHERE) &&
            room != &world[newexit->to_room])
            act_to_room(argument, &world[newexit->to_room]);
    }
}


WCMD(do_wecho)
{
    skip_spaces(&argument);

    if (!*argument) 
        wld_log(room, "wecho called with no args");

    else 
        act_to_room(argument, room);
}


WCMD(do_wsend)
{
    char buf[MAX_INPUT_LENGTH], *msg;
    char_data *ch;
  
    msg = any_one_arg(argument, buf);

    if (!*buf)
    {
        wld_log(room, "wsend called with no args");
        return;
    }

    skip_spaces(&msg);

    if (!*msg)
    {
        wld_log(room, "wsend called without a message");
        return;
    }

    if ((ch = get_char_by_room(room, buf)))
    {
        if (subcmd == SCMD_WSEND)
            sub_write(msg, ch, TRUE, TO_CHAR);
        else if (subcmd == SCMD_WECHOAROUND)
            sub_write(msg, ch, TRUE, TO_ROOM);
    }

    else
        wld_log(room, "no target found for wsend");
}

WCMD(do_wzoneecho)
{
    zone_rnum zone;
    char room_num[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;
  
    msg = any_one_arg(argument, room_num);
    skip_spaces(&msg);

    if (!*room_num || !*msg)
        wld_log(room, "wzoneecho called with too few args");

    else if ((zone = real_zone_by_thing(atoi(room_num))) == NOWHERE)
        wld_log(room, "wzoneecho called for nonexistant zone");

    else { 
        sprintf(buf, "%s\r\n", msg);
        send_to_zone(buf, zone);
    }
}


WCMD(do_wdoor)
{
    char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
    char field[MAX_INPUT_LENGTH], *value;
    room_data *rm;
    struct room_direction_data *newexit;
    int dir, fd, to_room;

    const char *door_field[] = {
        "purge",
        "description",
        "flags",
        "key",
        "name",
        "room",
        "\n"
    };


    argument = two_arguments(argument, target, direction);
    value = one_argument(argument, field);
    skip_spaces(&value);

    if (!*target || !*direction || !*field) {
        wld_log(room, "wdoor called with too few args");
        return;
    }
  
    if ((rm = get_room(target)) == NULL) {
        wld_log(room, "wdoor: invalid target");
        return;
    }
  
    if ((dir = search_block(direction, dirs, FALSE)) == -1) {
        wld_log(room, "wdoor: invalid direction");
        return;
    }

    if ((fd = search_block(field, door_field, FALSE)) == -1) {
        wld_log(room, "wdoor: invalid field");
        return;
    }

    newexit = rm->dir_option[dir];

    /* purge exit */
    if (fd == 0) {
        if (newexit) {
            if (newexit->general_description)
                free(newexit->general_description);
            if (newexit->keyword)
                free(newexit->keyword);
            free(newexit);
            rm->dir_option[dir] = NULL;
        }
    }

    else {
        if (!newexit) {
            CREATE(newexit, struct room_direction_data, 1);
            rm->dir_option[dir] = newexit; 
        }
    
        switch (fd) {
        case 1:  /* description */
            if (newexit->general_description)
                free(newexit->general_description);
            CREATE(newexit->general_description, char, strlen(value) + 3);
            strcpy(newexit->general_description, value);
            strcat(newexit->general_description, "\r\n");
            break;
        case 2:  /* flags       */
            newexit->exit_info = (sh_int)asciiflag_conv(value);
            break;
        case 3:  /* key         */
            newexit->key = atoi(value);
            break;
        case 4:  /* name        */
            if (newexit->keyword)
                free(newexit->keyword);
            CREATE(newexit->keyword, char, strlen(value) + 1);
            strcpy(newexit->keyword, value);
            break;
        case 5:  /* room        */
            if ((to_room = real_room(atoi(value))) != NOWHERE)
                newexit->to_room = to_room;
            else
                wld_log(room, "wdoor: invalid door target");
            break;
        }
    }
}


WCMD(do_wteleport)
{
    char_data *ch, *next_ch;
    sh_int target, nr;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);
  
    if (!*arg1 || !*arg2) {
        wld_log(room, "wteleport called with too few args");
        return;
    }

    nr = atoi(arg2);
    target = real_room(nr);
  
    if (target == NOWHERE) 
        wld_log(room, "wteleport target is an invalid room");
  
    else if (!str_cmp(arg1, "all")) {
        if (nr == room->number) {
            wld_log(room, "wteleport all target is itself");
            return;
        }

        for (ch = room->people; ch; ch = next_ch)
        {
            next_ch = ch->next_in_room;
            if (!valid_dg_target(ch, DG_ALLOW_GODS)) 
              continue;
            char_from_room(ch);
            char_to_room(ch, target);
        }
    }
  
    else
    {
        if ((ch = get_char_by_room(room, arg1))) {
          if (valid_dg_target(ch, DG_ALLOW_GODS)) {
            char_from_room(ch);
            char_to_room(ch, target);
          }
        }
        
        else
            wld_log(room, "wteleport: no target found");
    }
}


WCMD(do_wforce)
{
    char_data *ch, *next_ch;
    char arg1[MAX_INPUT_LENGTH], *line;

    line = one_argument(argument, arg1);
  
    if (!*arg1 || !*line) {
        wld_log(room, "wforce called with too few args");
        return;
    }
  
    if (!str_cmp(arg1, "all"))
    {
        for (ch = room->people; ch; ch = next_ch)
        {
            next_ch = ch->next_in_room;
      
            if (valid_dg_target(ch, 0))
            {
                command_interpreter(ch, line);
            }
        }
    }
  
    else
    {
        if ((ch = get_char_by_room(room, arg1)))
        {
            if (valid_dg_target(ch, 0))
            {
                command_interpreter(ch, line);
            }
        }
    
        else
            wld_log(room, "wforce: no target found");
    }
}


/* increases the target's exp */
WCMD(do_wexp)
{
    char_data *ch;
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

    two_arguments(argument, name, amount);

    if (!*name || !*amount) {
        wld_log(room, "wexp: too few arguments");
        return;
    }

    if ((ch = get_char_by_room(room, name))) 
        gain_exp(ch, atoi(amount));
    else {
        wld_log(room, "wexp: target not found");
        return;
    }
}


/* purge all objects an npcs in room, or specified object or mob */
WCMD(do_wpurge)
{
    char arg[MAX_INPUT_LENGTH];
    char_data *ch, *next_ch;
    obj_data *obj, *next_obj;

    one_argument(argument, arg);
  
    if (!*arg) {
      /* purge all */
      for (ch = room->people; ch; ch = next_ch ) {
        next_ch = ch->next_in_room;
        if (IS_NPC(ch))
          extract_char(ch);
      }
        
      for (obj = room->contents; obj; obj = next_obj ) {
        next_obj = obj->next_content;
        extract_obj(obj);
      }
    
      return;
    }
  
    ch = get_char_by_room(room, arg);
    if (!ch) {
      obj = get_obj_by_room(room, arg);
      if (obj) {
        extract_obj(obj);
      } else 
        wld_log(room, "wpurge: bad argument");
    
      return;
    }
  
    if (!IS_NPC(ch)) {
      wld_log(room, "wpurge: purging a PC");
      return;
    }
    
    extract_char(ch);
}


/* loads a mobile or object into the room */
WCMD(do_wload)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number = 0;
    char_data *mob;
    obj_data *object;
  

    two_arguments(argument, arg1, arg2);
  
    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
        wld_log(room, "wload: bad syntax");
        return;
    }

    if (is_abbrev(arg1, "mob")) {
        if ((mob = read_mobile(number, VIRTUAL)) == NULL) {
            wld_log(room, "wload: bad mob vnum");
            return;
        }
        char_to_room(mob, real_room(room->number));
        load_mtrigger(mob);
    }
  
    else if (is_abbrev(arg1, "obj")) {
        if ((object = read_object(number, VIRTUAL)) == NULL) {
            wld_log(room, "wload: bad object vnum");
            return;
        }

        obj_to_room(object, real_room(room->number)); 
        load_otrigger(object);
    }

    else
        wld_log(room, "wload: bad type");
}

WCMD(do_wdamage) {
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
  int dam = 0;
  char_data *ch;

  two_arguments(argument, name, amount);

  if (!*name || !*amount || !is_number(amount)) {
    wld_log(room, "wdamage: bad syntax");
    return;
  }

  dam = atoi(amount);
  ch = get_char_by_room(room, name);
    
  if (!ch) {
    wld_log(room, "wdamage: target not found");      
    return;
  }

  if (GET_LEVEL(ch)>=LVL_SAINT && (dam > 0)) {
    send_to_char(ch, "Being a god, you carefully avoid a trap.");
    return;
  }
 
  GET_HIT(ch) -= dam;
  update_pos(ch);
  send_char_pos(ch, dam);
 
  if (GET_POS(ch) == POS_DEAD) {
    if (!IS_NPC(ch))
      mudlog(BRF, 0, TRUE,  "%s killed by a trap at %s", GET_NAME(ch),
                            world[ch->in_room].name);
      clanlog(ch, "%s killed by a trap", GET_NAME(ch));
    die(ch, NULL);
  }
}


WCMD(do_wat) {
    char location[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int vnum = 0;    
    room_data *r2;
 
    half_chop(argument, location, arg2);

    if (!*location || !*arg2 || !isdigit(*location)) {
        wld_log(room, "wat: bad syntax");
        return;
    }
    vnum = atoi(location);
    if (NOWHERE == real_room(vnum)) {
        wld_log(room, "wat: location not found");
        return;
    }

    r2 = &world[real_room(vnum)];
    wld_command_interpreter(r2, arg2);
}

const struct wld_command_info wld_cmd_info[] = {
    { "RESERVED", 0, 0 },/* this must be first -- for specprocs */
    
    { "wasound "    , do_wasound   , 0 },
    { "wdoor "      , do_wdoor     , 0 },
    { "wecho "      , do_wecho     , 0 },
    { "wechoaround ", do_wsend     , SCMD_WECHOAROUND },
    { "wexp "       , do_wexp      , 0 },
    { "wforce "     , do_wforce    , 0 },
    { "wload "      , do_wload     , 0 },
    { "wpurge "     , do_wpurge    , 0 },
    { "wsend "      , do_wsend     , SCMD_WSEND },
    { "wteleport "  , do_wteleport , 0 },
    { "wzoneecho "  , do_wzoneecho , 0 },
    { "wdamage "    , do_wdamage,    0 },
    { "wat "        , do_wat,        0 },
    { "\n", 0, 0 }        /* this must be last */
};


/*
 *  This is the command interpreter used by rooms, called by script_driver.
 */
void wld_command_interpreter(room_data *room, char *argument)
{
    int cmd, length;
    char *line, arg[MAX_INPUT_LENGTH];
  
    skip_spaces(&argument);
  
    /* just drop to next line for hitting CR */
    if (!*argument)
        return;

    line = any_one_arg(argument, arg);


    /* find the command */
    for (length = strlen(arg), cmd = 0;
         *wld_cmd_info[cmd].command != '\n'; cmd++)
        if (!strncmp(wld_cmd_info[cmd].command, arg, length))
            break;
  
    if (*wld_cmd_info[cmd].command == '\n')
        wld_log(room, "Unknown world cmd: '%s'", argument);
    else
      ((*wld_cmd_info[cmd].command_pointer) 
       (room, line, cmd, wld_cmd_info[cmd].subcmd));
}
