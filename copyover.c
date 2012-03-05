/* 
 Here is what my logf looks like: 
 I like those variable argument functions, and have printf_to_char,
 bugf, etc. as well :) 
 
 Oh, and MSL = MAX_STRING_LENGTH (MIL = MAX_INPUT_LENGTH).
 
*/

void logf (char * fmt, ...)
{
	char buf [2*MSL];
	va_list args;
	va_start (args, fmt);
	vsprintf (buf, fmt, args);
	va_end (args);
	
	log_string (buf);
}


/* This is the handy CH() macro. I think that it was Tom Adriansen (sp?) */

#define CH(descriptor)  ((descriptor)->original ? \
(descriptor)->original : (descriptor)->character)



/*

You need to define:

COPYOVER_FILE - temporary data file used
EXE_FILE      - file to be exec'ed (i.e. the MUD)


Note that I advance level 1 chars to level 2 - this is necessary in MERC and
Envy, but I think that ROM saves level 1 characters too.

Note that you might want to change your close_socket() a bit. I have changed
the connected state so that negative states represent logging-in, while as
positive ones represent states where the character is already inside the game.
close_socket() frees that chararacters with negative state, but just loses
link on those with a positive state. I believe that idea comes from Elwyn
originally.

Things to note: This corresponds to a reboot, followed by the characters
logging in again. This means that stuff like corpses, disarmed weapons etc.
are lost, unless you save those to the pfile. You should probably give the
players some warning before doing a copyover.

The command was inspired by the discussion on merc-l about how Fusion's MUD++
could reboot without players having to re-login :)

*/


extern int port,control; /* db.c */

void do_copyover (CHAR_DATA *ch, char * argument)
{
	FILE *fp;
	DESCRIPTOR_DATA *d, *d_next;
	char buf [100], buf2[100];
	
	fp = fopen (COPYOVER_FILE, "w");
	
	if (!fp)
	{
		send_to_char ("Copyover file not writeable, aborted.\n\r",ch);
		logf ("Could not write to copyover file: %s", COPYOVER_FILE);
		perror ("do_copyover:fopen");
		return;
	}
	
	/* Consider changing all saved areas here, if you use OLC */
	
	/* do_asave (NULL, ""); - autosave changed areas */
	
	
	sprintf (buf, "\n\r *** COPYOVER by %s - please remain seated!\n\r", ch->name);
	
	/* For each playing descriptor, save its state */
	for (d = descriptor_list; d ; d = d_next)
	{
		CHAR_DATA * och = CH (d);
		d_next = d->next; /* We delete from the list , so need to save this */
		
		if (!d->character || d->connected > CON_PLAYING) /* drop those logging on */
		{
			write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0);
			close_socket (d); /* throw'em out */
		}
		else
		{
			fprintf (fp, "%d %s %s\n", d->descriptor, och->name, d->host);
			if (och->level == 1)
			{
				write_to_descriptor (d->descriptor, "Since you are level one, and level one characters do not save, you gain a free level!\n\r", 0);
				advance_level (och);
				och->level++; /* Advance_level doesn't do that */
			}
			save_char_obj (och);
			write_to_descriptor (d->descriptor, buf, 0);
		}
	}
	
	fprintf (fp, "-1\n");
	fclose (fp);
	
	/* Close reserve and other always-open files and release other resources */
	
	fclose (fpReserve);
	
	/* exec - descriptors are inherited */
	
	sprintf (buf, "%d", port);
	sprintf (buf2, "%d", control);
	execl (EXE_FILE, "EnvyMUD", buf, "copyover", buf2, (char *) NULL);

	/* Failed - sucessful exec will not return */
	
	perror ("do_copyover: execl");
	send_to_char ("Copyover FAILED!\n\r",ch);
	
	/* Here you might want to reopen fpReserve */
}

/* Recover from a copyover - load players */
void copyover_recover ()
{
	DESCRIPTOR_DATA *d;
	FILE *fp;
	char name [100];
	char host[MSL];
	int desc;
	bool fOld;
	
	logf ("Copyover recovery initiated");
	
	fp = fopen (COPYOVER_FILE, "r");
	
	if (!fp) /* there are some descriptors open which will hang forever then ? */
	{
		perror ("copyover_recover:fopen");
		logf ("Copyover file not found. Exitting.\n\r");
		exit (1);
	}

	unlink (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading	*/
	
	for (;;)
	{
		fscanf (fp, "%d %s %s\n", &desc, name, host);
		if (desc == -1)
			break;

		/* Write something, and check if it goes error-free */		
		if (!write_to_descriptor (desc, "\n\rRestoring from copyover...\n\r",0))
		{
			close (desc); /* nope */
			continue;
		}
		
		d = alloc_perm (sizeof(DESCRIPTOR_DATA));
		init_descriptor (d,desc); /* set up various stuff */
		
		d->host = str_dup (host);
		d->next = descriptor_list;
		descriptor_list = d;
		d->connected = CON_COPYOVER_RECOVER; /* -15, so close_socket frees the char */
		
	
		/* Now, find the pfile */
		
		fOld = load_char_obj (d, name);
		
		if (!fOld) /* Player file not found?! */
		{
			write_to_descriptor (desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r", 0);
			close_socket (d);			
		}
		else /* ok! */
		{
			write_to_descriptor (desc, "\n\rCopyover recovery complete.\n\r",0);
	
			/* Just In Case */
			if (!d->character->in_room)
				d->character->in_room = get_room_index (ROOM_VNUM_TEMPLE);

			/* Insert in the char_list */
			d->character->next = char_list;
			char_list = d->character;

			char_to_room (d->character, d->character->in_room);
			do_look (d->character, "");
			act ("$n materializes!", d->character, NULL, NULL, TO_ROOM);
			d->connected = CON_PLAYING;
		}
		
	}
	
	fclose (fp);
}




PATCHES to comm.c and db.c


Here's what I had to change to allow copyover to work:


comm.c
=======

int port and control need to be global (move them out of main).


Add this variable:

  
  int     main (int argc, char **argv)
  {
  	struct timeval now_time;
! 	bool fCopyOver = FALSE;


  

After  check for port number, check for copyover parameter. Number of
'control' descriptor is passed as 3rd. parameter. Note that this will not
work if you do not explicitly give a port number on the command like (i.e.
you have to type mud-name 1234 if you want to start up at port 1234). I was
too lazy to check for this :)


+ 	if (argv[2] && argv[2][0])
+ 	{
+ 		fCopyOver = TRUE;
+ 		control = atoi(argv[3]);
+ 	}
+ 		
+ 	else
+ 		fCopyOver = FALSE;


Here, init_socket (which does the bind'ing) only if we not already have
control. The call boot_db, passing a bool parameter that is TRUE if copyover
recover is needed

  #if defined( unix )
! 
! 	if (!fCopyOver) /* We have already the port if copyover'ed */
! 		control = init_socket (port);
! 	boot_db (fCopyOver);



In new descriptor, move the initialization part to a seperate procedure:

  
  #if defined( unix )
  void    new_descriptor (int control)
  {
  	BAN_DATA *pban;
- 	static DESCRIPTOR_DATA d_zero;
  	DESCRIPTOR_DATA *dnew;
  	struct sockaddr_in sock;
  	struct hostent *from;
--- 765,800 ----
  }
  #endif
  

This is the procedure:


+ void init_descriptor (DESCRIPTOR_DATA *dnew, int desc)
+ {
+ 	static DESCRIPTOR_DATA d_zero;
+ 
+ 	*dnew = d_zero;
+ 	dnew->descriptor = desc;
+ 	dnew->character = NULL;
+ 	dnew->connected = CON_GET_NAME;
+ 	dnew->showstr_head = str_dup ("");
+ 	dnew->showstr_point = 0;
+ 	dnew->pEdit = NULL;			/* OLC */
+ 	dnew->pString = NULL;		/* OLC */
+ 	dnew->editor = 0;			/* OLC */
+ 	dnew->outsize = 2000;
+ 	dnew->outbuf = alloc_mem (dnew->outsize);
+ 	
+     /* Initialize command history list. */
+     {
+         int  i;
+         for ( i = 0; i < MAX_HISTORY; i++ )
+             dnew->hist_cmd[i][0] = '\0';
+     }
+ 
+ }
  
  
In new descriptor, remove the intialization, and call init_descriptor
instead.


*************** void    new_descriptor (int control)
*** 802,825 ****
  		descriptor_free = descriptor_free->next;
  	}
  	
! 	*dnew = d_zero;
! 	dnew->descriptor = desc;
! 	dnew->character = NULL;
! 	dnew->connected = CON_GET_NAME;
! 	dnew->showstr_head = str_dup ("");
! 	dnew->showstr_point = 0;
! 	dnew->pEdit = NULL;			/* OLC */
! 	dnew->pString = NULL;		/* OLC */
! 	dnew->editor = 0;			/* OLC */
! 	dnew->outsize = 2000;
! 	dnew->outbuf = alloc_mem (dnew->outsize);
! 	
!     /* Initialize command history list. */
!     {
!         int  i;
!         for ( i = 0; i < MAX_HISTORY; i++ )
!             dnew->hist_cmd[i][0] = '\0';
!     }
  	
  
  	size = sizeof (sock);
--- 837,843 ----
  		descriptor_free = descriptor_free->next;
  	}
  	
! 	init_descriptor (dnew, desc);



in db.c, boot_db needs a bool flag:

*** 1.27	1996/06/04 20:02:09
--- db.c	1996/06/08 12:09:52

--- 231,237 ----
  /*
   * Big mama top level function.
   */
! void    boot_db ( bool fCopyOver)
  {
  	/*
  	 * Init some data space stuff.

And then just call copyover_recover, if needed:


*************** void    boot_db (void)
*** 419,424 ****
--- 419,427 ----
  		auction = alloc_mem (sizeof(struct auction_data));
  		arena = alloc_mem (sizeof(struct ArenaData));
  	}
+ 	
+ 	if (fCopyOver)
+ 		copyover_recover();
  
  	return;
  }

