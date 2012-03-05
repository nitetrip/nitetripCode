/* ************************************************************************
*   File: screen.h                                      Part of CircleMUD *
*  Usage: header file with ANSI color codes for online color              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* Added new colors!  Hacket Tobitts Dec 8 1994 */

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37;1m" /* really brite grey */
#define KGRY  "\x1B[37m"   /* old white */
#define KBGR  "\x1B[32;1m" /* brite green */
#define KBRD  "\x1B[31;1m" /* brite red */
#define KBYL  "\x1B[33;1m" /* brite yellow */
#define KBBL  "\x1B[34;1m" /* brite blue */
#define KBMG  "\x1B[35;1m" /* brite magenta */
#define KBCN  "\x1B[36;1m" /* brite cyan */
#define KNUL  ""           /* ya i know brite is spelt bright :] */

/* conditional color.  pass it a pointer to a char_data and a color level. */
#define C_OFF	0
#define C_SPR	1
#define C_NRM	2
#define C_CMP	3
#define _clrlevel(ch) ((PRF_FLAGGED((ch), PRF_COLOR_1) ? 1 : 0) + \
		       (PRF_FLAGGED((ch), PRF_COLOR_2) ? 2 : 0))
#define clr(ch,lvl) (_clrlevel(ch) >= (lvl))
#define CCNRM(ch,lvl)  (clr((ch),(lvl))?KNRM:KNUL)
#define CCRED(ch,lvl)  (clr((ch),(lvl))?KRED:KNUL)
#define CCGRN(ch,lvl)  (clr((ch),(lvl))?KGRN:KNUL)
#define CCYEL(ch,lvl)  (clr((ch),(lvl))?KYEL:KNUL)
#define CCBLU(ch,lvl)  (clr((ch),(lvl))?KBLU:KNUL)
#define CCMAG(ch,lvl)  (clr((ch),(lvl))?KMAG:KNUL)
#define CCCYN(ch,lvl)  (clr((ch),(lvl))?KCYN:KNUL)
#define CCWHT(ch,lvl)  (clr((ch),(lvl))?KWHT:KNUL)

/* new colors for CC      Hacket Tobitts Dec 8 1994 */

#define CCBRD(ch,lvl)  (clr((ch),(lvl))?KBRD:KNUL)
#define CCBYL(ch,lvl)  (clr((ch),(lvl))?KBYL:KNUL)
#define CCBGR(ch,lvl)  (clr((ch),(lvl))?KBGR:KNUL)
#define CCBBL(ch,lvl)  (clr((ch),(lvl))?KBBL:KNUL)
#define CCBMG(ch,lvl)  (clr((ch),(lvl))?KBMG:KNUL)
#define CCBCN(ch,lvl)  (clr((ch),(lvl))?KBCN:KNUL)
#define CCGRY(ch,lvl)  (clr((ch),(lvl))?KGRY:KNUL)

#define COLOR_LEV(ch) (_clrlevel(ch))

#define QNRM CCNRM(ch,C_SPR)
#define QRED CCRED(ch,C_SPR)
#define QGRN CCGRN(ch,C_SPR)
#define QYEL CCYEL(ch,C_SPR)
#define QBLU CCBLU(ch,C_SPR)
#define QMAG CCMAG(ch,C_SPR)
#define QCYN CCCYN(ch,C_SPR)
#define QWHT CCWHT(ch,C_SPR)

/* and so on   Hacket Tobitts Dec 8 1994 */
#define QBRD CCBRD(ch,C_SPR)
#define QBYL CCBYL(ch,C_SPR)
#define QBGR CCBGR(ch,C_SPR)
#define QBBL CCBBL(ch,C_SPR)
#define QBMG CCBMG(ch,C_SPR)
#define QBCN CCBCN(ch,C_SPR)
#define QGRY CCGRY(ch,C_SPR)

