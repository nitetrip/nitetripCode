#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"


const char *race_abbrevs[] = {
        "Hum",
        "Elf",
        "Gno",
        "Dwa",
	"Haf",
	"Min",
	"Pix",
	"Uld",
	"Tri",
	"Ogr",
	"Vam",
	"Shi",
	"Kar",
	"Vis",
        "\n"
};

const char *pc_race_types[] = {
        "Human",
        "Elf",
        "Gnome",
        "Dwarf",
	"Halfling",
	"Minotaur",
	"Pixie",
	"Uldra",
	"Triton",
	"Ogre",
	"Vampire",
	"Shintari",
	"Karadal",
	"Visreal",
        "\n"
};

const char *mob_race_abbrevs[] = {
        "Non",
        "Hum",
        "Elf",
        "Gno",
        "Dwa",
	"Haf",
	"Min",
	"Pix",
	"Uld",
	"Tri",
	"Ogr",
	"Vam",
	"Shi",
	"Kar",
	"Vis",
	"Dra",
        "Dva",
        "Dvl",
        "Dem",
        "Ele",
        "Und",
        "Pla",
        "Ani",
        "Rep",
        "Aqu",
        "Aer",
        "Ins",
        "Ara",
        "Gol",
        "Bea",
        "Hmd",
        "Drc",
        "Gia",
        "Din",
        "\n"
};

const char *mob_race_types[] = {
        "None",
        "Human",
        "Elf",
        "Gnome",
        "Dwarf",
	"Halfling",
	"Minotaur",
	"Pixie",
	"Uldra",
	"Triton",
	"Ogre",
	"Vampire",
	"Shintari",
	"Karadal",
	"Visreal",
	"Dragon",
        "Deva",
        "Devil",
        "Demon",
        "Elemental",
        "Undead",
        "Plant",
        "Animal",
        "Reptile",
        "Aquatic",
        "Aerial",
        "Insect",
        "Arachnid",
        "Golem",
        "Beast",
        "Humanoid",
        "Draconian",
        "Giant",
        "Dinosaur",
        "\n"
};

/* The menu for choosing a race in interpreter.c: */
const char *race_menu =
"\r\n"
"Select a race:\r\n"
"  [H]uman\r\n"
"  [E]lf\r\n"
"  [G]nome\r\n"
"  [D]warf\r\n"
"  Hal[f]ling\r\n"
"  [M]inotaur\r\n"
"  [P]ixie\r\n"
"  [U]ldra\r\n"
"  [T]riton\r\n"
"  [O]gre\r\n"
"  [V]ampire\r\n"
"  [S]hintari\r\n"
"  [K]aradal\r\n"
"  V[i]sreal\r\n";
/*
 * The code to interpret a race letter (used in interpreter.c when a
 * new character is selecting a race).
 */
int parse_race(char arg)
{
        arg = LOWER(arg);

        switch (arg) {
                case 'h': return RACE_HUMAN;
                case 'e': return RACE_ELF;
                case 'g': return RACE_GNOME;
                case 'd': return RACE_DWARF;
		case 'f': return RACE_HALFLING;
		case 'm': return RACE_MINOTAUR;
		case 'p': return RACE_PIXIE;
		case 'u': return RACE_ULDRA;
		case 't': return RACE_TRITON;
		case 'o': return RACE_OGRE;
		case 'v': return RACE_VAMPIRE;
		case 's': return RACE_SHINTARI;
		case 'k': return RACE_KARADAL;
		case 'i': return RACE_VISRAEL;

                default:
                        return RACE_UNDEFINED;
        }
}

bitvector_t find_race_bitvector(const char *arg)
{
        size_t rpos, ret = 0;

        for (rpos = 0; rpos < strlen(arg); rpos++)
                ret |= (1 << parse_race(arg[rpos]));

        return (ret);
}

void racial_ability_modifiers(struct char_data *ch)
{
        switch (GET_RACE(ch)) {

                default:
                case RACE_HUMAN:
                        break;

                        break;

                case RACE_ELF:
			ch->real_abils.str -= 2;
			ch->real_abils.intel += 1;
			ch->real_abils.wis += 2;
                        ch->real_abils.dex += 2;
                        ch->real_abils.con -= 2;
                        break;

                case RACE_GNOME:
                        ch->real_abils.intel += 2;
                        ch->real_abils.dex += 1;
			ch->real_abils.con += 1;
			ch->real_abils.cha -= 1;
                        break;

                case RACE_DWARF:
			ch->real_abils.str += 1;
			ch->real_abils.wis -= 1;
			ch->real_abils.dex -= 1;
                        ch->real_abils.con += 1;
                        ch->real_abils.cha -= 1;
                        break;

		case RACE_HALFLING:
			ch->real_abils.str -= 1;
			ch->real_abils.dex += 3;
			ch->real_abils.cha += 1;
			break;

		case RACE_MINOTAUR:
			ch->real_abils.str += 2;
			ch->real_abils.intel -= 1;
			ch->real_abils.dex -= 2;
			ch->real_abils.con += 2;
			ch->real_abils.cha -= 3;
			break;

		case RACE_PIXIE:
			ch->real_abils.intel += 1;
			ch->real_abils.wis += 1;
			ch->real_abils.dex += 2;
			ch->real_abils.con -= 2;
			break;

		case RACE_ULDRA:
			ch->real_abils.wis += 2;
			ch->real_abils.con += 1;
			ch->real_abils.cha -= 1;
			break;

		case RACE_TRITON:
			ch->real_abils.str += 1;
			ch->real_abils.dex += 1;
			ch->real_abils.con += 1;
			break;

		case RACE_OGRE:
			ch->real_abils.str += 4;
			ch->real_abils.intel -= 4;
			ch->real_abils.wis -= 3;
			ch->real_abils.con += 3;
			ch->real_abils.cha -= 3;
			break;

		case RACE_VAMPIRE:
			ch->real_abils.str += 2;
			ch->real_abils.intel += 1;
			ch->real_abils.wis += 1;
			ch->real_abils.con += 4;
			ch->real_abils.cha += 2;
			break;

		case RACE_SHINTARI:
			ch->real_abils.str += 2;
			ch->real_abils.intel += 2;
			ch->real_abils.cha -= 3;
			break;

		case RACE_KARADAL:
			ch->real_abils.str -= 2;
			ch->real_abils.intel += 3;
			ch->real_abils.wis += 2;
			ch->real_abils.con += 1;
			break;

		case RACE_VISRAEL:
			ch->real_abils.str -= 1;
			ch->real_abils.dex += 3;
			ch->real_abils.intel += 2;
			break;

			
   }
}


void set_height_by_race(struct char_data *ch)
{
        if (GET_SEX(ch) == SEX_MALE)
        {
                if (IS_DWARF(ch))
                        GET_HEIGHT(ch) = 43 + dice(1, 10);
                else if (IS_ELF(ch))
                        GET_HEIGHT(ch) = 60 + dice(3, 10);
                else if (IS_GNOME(ch))
                        GET_HEIGHT(ch) = 38 + dice(1, 6);
		else if (IS_HALFLING(ch))
			GET_HEIGHT(ch) = 34 + dice(1, 10);
		else if (IS_MINOTAUR(ch))
			GET_HEIGHT(ch) = 66 + dice(3, 6);
		else if (IS_PIXIE(ch))
			GET_HEIGHT(ch) = 8 + dice(1, 24);
		else if (IS_ULDRA(ch))
			GET_HEIGHT(ch) = 36 + dice(1, 12);
		else if (IS_OGRE(ch))
			GET_HEIGHT(ch) = 66 + dice(1, 18);
		else if (IS_SHINTARI(ch))
			GET_HEIGHT(ch) = 66 + dice(1, 18);
		else if (IS_KARADAL(ch))
			GET_HEIGHT(ch) = 63 + dice(1, 12);
		else if (IS_VISRAEL(ch))
			GET_HEIGHT(ch) = 45 + dice(1, 8);  
                else /* if (IS_HUMAN(ch)) */
                        GET_HEIGHT(ch) = 60 + dice(2, 10);
        } else /* if (IS_FEMALE(ch)) */ {
                if (IS_DWARF(ch))
                        GET_HEIGHT(ch) = 41 + dice(1, 10);
                else if (IS_ELF(ch))
                        GET_HEIGHT(ch) = 57 + dice(1, 10);
                else if (IS_GNOME(ch))
                        GET_HEIGHT(ch) = 36 + dice(1, 6);
		else if (IS_HALFLING(ch))
			GET_HEIGHT(ch) = 30 + dice(1, 10);
		else if (IS_MINOTAUR(ch))
			GET_HEIGHT(ch) = 60 + dice(3, 6);
		else if (IS_PIXIE(ch))
			GET_HEIGHT(ch) = 6 + dice(1, 24);
		else if (IS_ULDRA(ch))
			GET_HEIGHT(ch) = 32 + dice(1, 12);
		else if (IS_OGRE(ch))
			GET_HEIGHT(ch) = 62 + dice(1, 18);
		else if (IS_SHINTARI(ch))
			GET_HEIGHT(ch) = 62 + dice(1, 18);
		else if (IS_KARADAL(ch))
			GET_HEIGHT(ch) = 60 + dice(1, 12);
		else if (IS_VISRAEL(ch))
			GET_HEIGHT(ch) = 40 + dice(1, 8);  
                else /* if (IS_HUMAN(ch)) */
                        GET_HEIGHT(ch) = 59 + dice(2, 10);
        }

        return;
}

void set_weight_by_race(struct char_data *ch)
{
        if (GET_SEX(ch) == SEX_MALE)
        {
                if (IS_DWARF(ch))
                        GET_WEIGHT(ch) = 130 + dice(4, 10);
                else if (IS_ELF(ch))
                        GET_WEIGHT(ch) = 90 + dice(3, 10);
                else if (IS_GNOME(ch))
                        GET_WEIGHT(ch) = 72 + dice(5, 4);
		else if (IS_HALFLING(ch))
			GET_WEIGHT(ch) = 40 + dice(6, 10);
		else if (IS_MINOTAUR(ch))
			GET_WEIGHT(ch) = 200 + dice(6, 10);
		else if (IS_PIXIE(ch))
			GET_WEIGHT(ch) = dice(6, 10);
		else if (IS_ULDRA(ch))
			GET_WEIGHT(ch) = 40 + dice(6, 10);
		else if (IS_OGRE(ch))
			GET_WEIGHT(ch) = 275 + dice(6, 10);
		else if (IS_SHINTARI(ch))
			GET_WEIGHT(ch) = 275 + dice(6, 10);
		else if (IS_KARADAL(ch))
			GET_WEIGHT(ch) = 80 + dice(6, 10);
		else if (IS_VISRAEL(ch))
			GET_HEIGHT(ch) = 110 + dice(6, 10); 
                else /* if (IS_HUMAN(ch)) */
                        GET_WEIGHT(ch) = 140 + dice(6, 10);
        } else /* if (IS_FEMALE(ch)) */ {
                if (IS_DWARF(ch))
                        GET_WEIGHT(ch) = 105 + dice(4, 10);
                else if (IS_ELF(ch))
                        GET_WEIGHT(ch) = 55 + dice(3, 10);
                else if (IS_GNOME(ch))
                        GET_WEIGHT(ch) = 47 + dice(5, 4);
		else if (IS_HALFLING(ch))
			GET_WEIGHT(ch) = 25 + dice(6, 10);
		else if (IS_MINOTAUR(ch))
			GET_WEIGHT(ch) = 185 + dice(6, 10);
		else if (IS_PIXIE(ch))
			GET_WEIGHT(ch) = dice(4, 10);
		else if (IS_ULDRA(ch))
			GET_WEIGHT(ch) = 25 + dice(6, 10);
		else if (IS_OGRE(ch))
			GET_WEIGHT(ch) = 245 + dice(6, 10);
		else if (IS_SHINTARI(ch))
			GET_WEIGHT(ch) = 245 + dice(6, 10);
		else if (IS_KARADAL(ch))
			GET_WEIGHT(ch) = 50 + dice(6, 10);
		else if (IS_VISRAEL(ch))
			GET_HEIGHT(ch) = 75 + dice(6, 10);
                else /* if (IS_HUMAN(ch)) */
                        GET_WEIGHT(ch) = 100 + dice(6, 10);
        }

        return;
}


int invalid_race(struct char_data *ch, struct obj_data *obj)
{
        if (OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch))
                return (TRUE);

        if (OBJ_FLAGGED(obj, ITEM_ANTI_ELF) && IS_ELF(ch))
                return (TRUE);

        if (OBJ_FLAGGED(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch))
                return (TRUE);

        if (OBJ_FLAGGED(obj, ITEM_ANTI_GNOME) && IS_GNOME(ch))
                return (TRUE);

  return (FALSE);
}

