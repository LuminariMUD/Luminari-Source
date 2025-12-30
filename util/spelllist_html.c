/* Utility to generate HTML documentation of all spells */

#include "../src/conf.h"
#include "../src/sysdep.h"
#include "../src/structs.h"
#include "../src/utils.h"
#include "../src/spells.h"
#include "../src/db.h"
#include <stdio.h>
#include <string.h>

/* External declarations */
extern struct spell_info_type spell_info[];
extern const char *class_names[];

/* School names - matching the order in constants.c */
const char *school_names[] = {"None",          "Abjuration", "Conjuration", "Divination",
                              "Enchantment",   "Evocation",  "Illusion",    "Necromancy",
                              "Transmutation", "\n"};

#define NUM_SCHOOLS 9

/* Position names */
const char *position_types[] = {
    "Dead",    "Mortally wounded", "Incapacitated", "Stunned",  "Sleeping",
    "Resting", "Sitting",          "Fighting",      "Standing", "\n"};

/* Target types */
const char *target_names[] = {"Ignore",    "Char Room", "Char World", "Fight Self", "Fight Vict",
                              "Self Only", "Not Self",  "Obj Inv",    "Obj Room",   "Obj World",
                              "Obj Equip", "All",       "\n"};

/* Declare extern functions we need */
extern const char *skill_name(int num);

int main(void)
{
  FILE *fl;
  int i, j;
  char filename[256];

  /* Initialize class data if needed */

  snprintf(filename, sizeof(filename), "../docs/spells_reference.html");

  if (!(fl = fopen(filename, "w")))
  {
    fprintf(stderr, "Cannot open output file: %s\n", filename);
    return 1;
  }

  /* Write HTML header */
  fprintf(fl, "<!DOCTYPE html>\n");
  fprintf(fl, "<html lang=\"en\">\n");
  fprintf(fl, "<head>\n");
  fprintf(fl, "    <meta charset=\"UTF-8\">\n");
  fprintf(fl, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
  fprintf(fl, "    <title>Luminari MUD - Spell Reference</title>\n");
  fprintf(fl, "    <style>\n");
  fprintf(fl, "        body { font-family: Arial, sans-serif; margin: 20px; background-color: "
              "#f5f5f5; }\n");
  fprintf(fl,
          "        h1 { color: #333; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }\n");
  fprintf(fl, "        h2 { color: #555; margin-top: 30px; }\n");
  fprintf(fl, "        .spell-block { background-color: white; margin: 20px 0; padding: 20px; "
              "border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n");
  fprintf(fl, "        .spell-name { font-size: 24px; font-weight: bold; color: #2196F3; "
              "margin-bottom: 10px; }\n");
  fprintf(fl, "        .spell-number { color: #999; font-size: 14px; }\n");
  fprintf(fl, "        .info-grid { display: grid; grid-template-columns: 200px 1fr; gap: 10px; "
              "margin-top: 15px; }\n");
  fprintf(fl, "        .info-label { font-weight: bold; color: #666; }\n");
  fprintf(fl, "        .info-value { color: #333; }\n");
  fprintf(fl, "        .class-levels { margin-top: 10px; }\n");
  fprintf(fl, "        .class-row { padding: 5px 0; }\n");
  fprintf(fl, "        .class-name { display: inline-block; width: 150px; font-weight: bold; }\n");
  fprintf(fl, "        .unavailable { color: #ccc; }\n");
  fprintf(fl,
          "        .quest-spell { background-color: #fff3cd; border-left: 4px solid #ffc107; }\n");
  fprintf(fl,
          "        .ritual-spell { background-color: #e8f5e9; border-left: 4px solid #4caf50; }\n");
  fprintf(fl, "        .navbar { position: sticky; top: 0; background-color: #333; padding: 10px; "
              "margin: -20px -20px 20px -20px; }\n");
  fprintf(fl, "        .navbar a { color: white; text-decoration: none; margin: 0 15px; }\n");
  fprintf(fl, "        .navbar a:hover { color: #4CAF50; }\n");
  fprintf(fl, "        .alpha-index { background-color: white; padding: 15px; border-radius: 8px; "
              "margin-bottom: 20px; }\n");
  fprintf(fl, "        .alpha-index a { margin: 0 10px; text-decoration: none; font-weight: bold; "
              "color: #2196F3; }\n");
  fprintf(fl, "        .alpha-header { font-size: 32px; font-weight: bold; color: #4CAF50; "
              "margin-top: 40px; padding: 10px; background-color: white; border-radius: 8px; }\n");
  fprintf(fl, "    </style>\n");
  fprintf(fl, "</head>\n");
  fprintf(fl, "<body>\n\n");

  fprintf(fl, "<div class=\"navbar\">\n");
  fprintf(fl, "    <a href=\"#top\">Top</a>\n");
  for (i = 'A'; i <= 'Z'; i++)
  {
    fprintf(fl, "    <a href=\"#letter-%c\">%c</a>\n", i, i);
  }
  fprintf(fl, "</div>\n\n");

  fprintf(fl, "<h1 id=\"top\">Luminari MUD - Spell Reference Guide</h1>\n");
  fprintf(fl, "<p>Generated: %s</p>\n", __DATE__);
  fprintf(fl, "<p>Total Spells: ");

  /* Count actual spells (not skills) */
  int spell_count = 0;
  for (i = 1; i < TOP_SPELL_DEFINE; i++)
  {
    if (spell_info[i].name && strlen(spell_info[i].name) > 0 &&
        spell_info[i].min_position != POS_DEAD)
    {
      spell_count++;
    }
  }
  fprintf(fl, "%d</p>\n\n", spell_count);

  /* Create alphabetical index */
  fprintf(fl, "<div class=\"alpha-index\">\n");
  fprintf(fl, "<strong>Quick Navigation:</strong> ");
  for (i = 'A'; i <= 'Z'; i++)
  {
    fprintf(fl, "<a href=\"#letter-%c\">%c</a> ", i, i);
  }
  fprintf(fl, "\n</div>\n\n");

  /* Group spells by first letter */
  for (char letter = 'A'; letter <= 'Z'; letter++)
  {
    int found = 0;

    /* Check if any spells start with this letter */
    for (i = 1; i < TOP_SPELL_DEFINE; i++)
    {
      if (spell_info[i].name && strlen(spell_info[i].name) > 0 &&
          spell_info[i].min_position != POS_DEAD && toupper(spell_info[i].name[0]) == letter)
      {
        if (!found)
        {
          fprintf(fl, "<div class=\"alpha-header\" id=\"letter-%c\">%c</div>\n\n", letter, letter);
          found = 1;
        }

        /* Determine special spell types */
        char extra_class[100] = "";
        if (spell_info[i].quest)
        {
          strcat(extra_class, " quest-spell");
        }
        if (spell_info[i].ritual_spell)
        {
          strcat(extra_class, " ritual-spell");
        }

        fprintf(fl, "<div class=\"spell-block%s\">\n", extra_class);
        fprintf(fl,
                "    <div class=\"spell-name\">%s <span class=\"spell-number\">(Spell "
                "#%d)</span></div>\n",
                spell_info[i].name, i);

        if (spell_info[i].quest)
        {
          fprintf(fl,
                  "    <div style=\"color: #856404; font-weight: bold;\">⚠ Quest Spell</div>\n");
        }
        if (spell_info[i].ritual_spell)
        {
          fprintf(fl,
                  "    <div style=\"color: #2e7d32; font-weight: bold;\">🕯 Ritual Spell</div>\n");
        }

        fprintf(fl, "    <div class=\"info-grid\">\n");

        /* School of Magic */
        if (spell_info[i].schoolOfMagic >= 0 && spell_info[i].schoolOfMagic < NUM_SCHOOLS)
        {
          fprintf(fl, "        <div class=\"info-label\">School:</div>\n");
          fprintf(fl, "        <div class=\"info-value\">%s</div>\n",
                  school_names[spell_info[i].schoolOfMagic]);
        }

        /* Minimum Position */
        fprintf(fl, "        <div class=\"info-label\">Min Position:</div>\n");
        fprintf(fl, "        <div class=\"info-value\">%s</div>\n",
                position_types[spell_info[i].min_position]);

        /* PSP Costs */
        if (spell_info[i].psp_max > 0 || spell_info[i].psp_min > 0)
        {
          fprintf(fl, "        <div class=\"info-label\">PSP Cost:</div>\n");
          fprintf(fl, "        <div class=\"info-value\">Min: %d, Max: %d</div>\n",
                  spell_info[i].psp_min, spell_info[i].psp_max);
        }

        /* Casting Time */
        if (spell_info[i].time > 0)
        {
          fprintf(fl, "        <div class=\"info-label\">Casting Time:</div>\n");
          fprintf(fl, "        <div class=\"info-value\">%d rounds</div>\n", spell_info[i].time);
        }

        /* Memorization Time */
        if (spell_info[i].memtime > 0)
        {
          fprintf(fl, "        <div class=\"info-label\">Mem Time:</div>\n");
          fprintf(fl, "        <div class=\"info-value\">%d</div>\n", spell_info[i].memtime);
        }

        /* Violent flag */
        fprintf(fl, "        <div class=\"info-label\">Violent:</div>\n");
        fprintf(fl, "        <div class=\"info-value\">%s</div>\n",
                spell_info[i].violent ? "Yes" : "No");

        /* Wear-off message */
        if (spell_info[i].wear_off_msg && strlen(spell_info[i].wear_off_msg) > 0)
        {
          fprintf(fl, "        <div class=\"info-label\">Wear-off Message:</div>\n");
          fprintf(fl, "        <div class=\"info-value\">%s</div>\n", spell_info[i].wear_off_msg);
        }

        fprintf(fl, "    </div>\n");

        /* Class Levels */
        fprintf(fl, "    <div class=\"class-levels\">\n");
        fprintf(fl, "        <div class=\"info-label\">Available to Classes:</div>\n");
        int has_class = 0;
        for (j = 0; j < NUM_CLASSES; j++)
        {
          if (spell_info[i].min_level[j] < LVL_IMMORT)
          {
            fprintf(fl,
                    "        <div class=\"class-row\"><span class=\"class-name\">%s:</span> Level "
                    "%d</div>\n",
                    class_names[j], spell_info[i].min_level[j]);
            has_class = 1;
          }
        }
        if (!has_class)
        {
          fprintf(
              fl,
              "        <div class=\"class-row unavailable\">Not available to any class</div>\n");
        }
        fprintf(fl, "    </div>\n");

        fprintf(fl, "</div>\n\n");
      }
    }
  }

  /* Footer */
  fprintf(fl, "<div style=\"margin-top: 50px; padding: 20px; background-color: white; "
              "border-radius: 8px; text-align: center;\">\n");
  fprintf(fl, "    <p><strong>Luminari MUD Spell Reference</strong></p>\n");
  fprintf(fl, "    <p>For more information, visit the game or contact the administrators.</p>\n");
  fprintf(fl, "    <p><a href=\"#top\">Back to Top</a></p>\n");
  fprintf(fl, "</div>\n\n");

  fprintf(fl, "</body>\n");
  fprintf(fl, "</html>\n");

  fclose(fl);

  printf("Spell reference HTML generated: %s\n", filename);
  printf("Total spells documented: %d\n", spell_count);

  return 0;
}
