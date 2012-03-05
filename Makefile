# Generated automatically from Makefile.in by configure.
# CircleMUD Makefile.in - Makefile template used by 'configure'

# C compiler to use
CC = gcc

# Path to cxref utility
CXREF = cxref

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE)

LIBS =  -lcrypt 

OBJFILES = 	act.comm.o act.informative.o act.item.o act.movement.o \
	act.offensive.o act.other.o act.social.o act.wizard.o alias.o ban.o \
	boards.o clanedit.o clan.o castle.o class.o comm.o config.o constants.o db.o fight.o \
	genmob.o genobj.o genolc.o genshp.o genwld.o genzon.o graph.o handler.o \
	house.o improved-edit.o interpreter.o limits.o magic.o mail.o medit.o \
	mobact.o modify.o oasis.o objsave.o oedit.o olc.o random.o redit.o \
	sedit.o shop.o spec_assign.o spec_procs.o spell_parser.o spells.o \
	tedit.o utils.o weather.o zedit.o bsd-snprintf.o \
	dg_comm.o dg_db_scripts.o dg_event.o dg_handler.o dg_mobcmd.o \
	dg_misc.o dg_objcmd.o dg_scripts.o dg_triggers.o dg_wldcmd.o dg_olc.o \
	context_help.o diskio.o races.o zcheck.o color.o


CXREF_FILES = act.comm.c act.informative.c act.item.c act.movement.c \
	act.offensive.c act.other.c act.social.c act.wizard.c alias.c ban.c \
	boards.c clanedit.c clan.c castle.c class.c comm.c config.c constants.c db.c fight.c \
	genmob.c genobj.c genolc.c genshp.c genwld.c genzon.c graph.c handler.c \
	house.c improved-edit.c interpreter.c limits.c magic.c mail.c medit.c \
	mobact.c modify.c oasis.c objsave.c oedit.c olc.c random.c redit.c \
	sedit.c shop.c spec_assign.c spec_procs.c spell_parser.c spells.c \
	tedit.c utils.c weather.c zedit.c bsd-snprintf.c \
	dg_comm.c dg_db_scripts.c dg_event.c dg_handler.c dg_mobcmd.c \
	dg_misc.c dg_objcmd.c dg_scripts.c dg_triggers.c dg_wldcmd.c dg_olc.c \
	context_help.c diskio.c races.c zcheck.c color.c

default: all

all: .accepted
	$(MAKE) $(BINDIR)/circle
	$(MAKE) utils

.accepted:
	@./licheck less

utils: .accepted
	(cd util; $(MAKE) all)
circle:
	$(MAKE) $(BINDIR)/circle

$(BINDIR)/circle : $(OBJFILES)
	$(CC) -o $(BINDIR)/circle $(PROFILE) $(OBJFILES) $(LIBS)

clean:
	rm -f *.o
ref:
#
# Create the cross reference files
# Note, this is not meant to be used unless you've installed cxref...
#
	@for file in $(CXREF_FILES) ; do \
	  echo Cross referencing $$file ; \
	  $(CXREF) -D__CXREF__ -xref -Odoc -Ncircle $$file ; \
	done
#
# Create the source files using cxref
#
	@for file in $(CXREF_FILES) ; do \
	   echo Documenting $$file ; \
	   ( cd . ; $(CXREF) -D__CXREF__ -warn-xref -xref -Odoc -Ncircle -html $$file ) ; \
	   rm -f $(DOCS) ; \
	done
#
# Create the index using cxref
#
	@echo Indexing
	@( cd . ; $(CXREF) -D__CXREF__ -index-all -Odoc -Ncircle -html )
	@rm -f $(DOCS)
#
# Make html files for the .h files
#
	@echo Creating .h.html files...
	@for file in *.h ; do \
	  echo $$file ; \
	  cat htmlh-head $$file htmlh-tail > doc/$$file.html ; \
	done
# Copy over to the html directory
	#cp doc/*.html $(HOME)/www/cxref
	#chmod 644 $(HOME)/www/cxref/*.html

# Dependencies for the object files (automagically generated with
# gcc -MM)

depend:
	$(CC) -MM *.c > depend

-include depend
