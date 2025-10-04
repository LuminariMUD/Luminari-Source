/***********************************************************************
** TALENTS.C                                                          **
** Implementation of Crafting & Harvesting Talent System              **
***********************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "modify.h"
#include "feats.h"
#include "class.h"
#include "mud_event.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "spell_prep.h"
#include "talents.h"
#include "helpers.h" /* for two_arguments prototype */

struct talent_info talent_list[TALENT_MAX];

/* Helper to initialize a talent definition */
static void talento(int talent, const char *name, int base_point_cost, int base_gold_cost, int max_ranks,
                    const char *short_desc, const char *long_desc, bool in_game) {
  if (talent < 0 || talent >= TALENT_MAX) return;
  talent_list[talent].name = name;
  talent_list[talent].base_point_cost = base_point_cost;
  talent_list[talent].base_gold_cost = base_gold_cost;
  talent_list[talent].max_ranks = MAX(1, max_ranks);
  talent_list[talent].short_desc = short_desc;
  talent_list[talent].long_desc = long_desc;
  talent_list[talent].in_game = in_game;
}

void init_talents(void) {
  memset(talent_list, 0, sizeof(talent_list));
  talento(TALENT_NONE, "(none)", 0, 0, 1, "", "", FALSE);
  /* Default design: base point cost 1, base gold 100 per rank, up to 3 ranks unless noted */
  talento(TALENT_EFFICIENT_CRAFTING, "efficient crafting", 1, 100, 3,
          "Reduce material consumption slightly.",
          "Reduces material consumption by approximately 5% (placeholder effect).", TRUE);
  talento(TALENT_PRECISE_CRAFTING, "precise crafting", 1, 100, 3,
          "Improves crafting precision.",
          "Adds a small bonus to success and quality rolls when crafting.", TRUE);
  talento(TALENT_RAPID_HARVEST, "rapid harvest", 1, 100, 3,
          "Faster harvesting actions.",
          "Reduces time required for harvesting actions by about 10% (placeholder).", TRUE);
  talento(TALENT_MASTER_REFINER, "master refiner", 2, 150, 3,
          "Improved refine yields.",
          "Increases refined material yield by roughly 10% (placeholder).", TRUE);
  talento(TALENT_RESOURCE_INSIGHT, "resource insight", 2, 150, 3,
          "Better rare resource chance.",
          "Slightly increases the chance to obtain rare harvesting results.", TRUE);
  talento(TALENT_SUPERIOR_SUPPLY, "superior supply", 2, 150, 3,
          "Improves supply order rewards.",
          "Adds a small percentage bonus to supply order experience / rewards.", TRUE);
  talento(TALENT_ARTISAN_TOUCH, "artisan's touch", 2, 150, 3,
          "Improves quality tier chances.",
          "Grants a bonus to the internal roll that determines item quality tier.", TRUE);
  talento(TALENT_BULK_SPECIALIST, "bulk specialist", 2, 150, 3,
          "Better results on bulk orders.",
          "Adds bonus quantity for bulk contract crafting.", TRUE);
  talento(TALENT_ALCHEMICAL_FOCUS, "alchemical focus", 1, 100, 3,
          "Enhanced alchemy potency.",
          "Provides a small potency boost to alchemy creations.", TRUE);
}

/* Player data integration helpers */
int get_talent_rank(struct char_data *ch, int talent) {
  if (!ch || IS_NPC(ch)) return 0;
  if (talent <= 0 || talent >= TALENT_MAX) return 0;
  return GET_TALENT_RANK(ch, talent);
}

int has_talent(struct char_data *ch, int talent) {
  return get_talent_rank(ch, talent) > 0;
}

static int current_rank(struct char_data *ch, int talent) {
  return get_talent_rank(ch, talent);
}

int talent_max_ranks(int talent) {
  if (talent <= 0 || talent >= TALENT_MAX) return 0;
  return MAX(1, talent_list[talent].max_ranks);
}

/* Cost progression: next rank n costs base + (n-1) increment.
 * For now: point cost grows by +1 per rank; gold cost grows by +50% per rank (rounded up). */
int talent_next_point_cost(struct char_data *ch, int talent) {
  if (!ch || talent <= 0 || talent >= TALENT_MAX) return 0;
  int rank = current_rank(ch, talent);
  if (rank >= talent_max_ranks(talent)) return 0;
  return talent_list[talent].base_point_cost + rank; /* 0->+0, 1->+1, etc */
}

int talent_next_gold_cost(struct char_data *ch, int talent) {
  if (!ch || talent <= 0 || talent >= TALENT_MAX) return 0;
  int rank = current_rank(ch, talent);
  if (rank >= talent_max_ranks(talent)) return 0;
  /* geometric-ish growth: base * (1.5^rank) using integer math with ceil */
  int cost = talent_list[talent].base_gold_cost;
  int i;
  for (i = 0; i < rank; i++) {
    /* ceil(cost * 3 / 2) */
    cost = (cost * 3 + 1) / 2;
  }
  return cost;
}

int can_learn_talent(struct char_data *ch, int talent) {
  if (!ch || IS_NPC(ch)) return 0;
  if (talent <= 0 || talent >= TALENT_MAX) return 0;
  if (!talent_list[talent].in_game) return 0;
  int rank = current_rank(ch, talent);
  if (rank >= talent_max_ranks(talent)) return 0;
  int p_cost = talent_next_point_cost(ch, talent);
  int g_cost = talent_next_gold_cost(ch, talent);
  if (p_cost <= 0) return 0;
  if (GET_TALENT_POINTS(ch) < p_cost) return 0;
  if (g_cost > 0 && GET_GOLD(ch) < g_cost) return 0;
  return 1;
}

int learn_talent(struct char_data *ch, int talent) {
  if (!can_learn_talent(ch, talent)) return 0;
  int p_cost = talent_next_point_cost(ch, talent);
  int g_cost = talent_next_gold_cost(ch, talent);
  /* Deduct costs */
  if (p_cost > 0) GET_TALENT_POINTS(ch) -= p_cost;
  if (g_cost > 0) {
    if (GET_GOLD(ch) < g_cost) return 0; /* double-check */
    GET_GOLD(ch) -= g_cost;
  }
  /* Increase rank */
  int rank = current_rank(ch, talent);
  (ch)->player_specials->saved.talent_ranks[talent] = MIN(rank + 1, talent_max_ranks(talent));
  if (rank == 0)
    send_to_char(ch, "You learn the talent: %s (rank %d/%d)\r\n", talent_list[talent].name, rank+1, talent_max_ranks(talent));
  else
    send_to_char(ch, "You improve %s to rank %d/%d\r\n", talent_list[talent].name, rank+1, talent_max_ranks(talent));
  return 1;
}

void list_talents(struct char_data *ch) {
  int i;
  send_to_char(ch, "\r\nCrafting Talents (ranked):\r\n--------------------------\r\n");
  send_to_char(ch, "You have %d unspent talent point%s.  Gold: %d\r\n\r\n", GET_TALENT_POINTS(ch), GET_TALENT_POINTS(ch)==1?"":"s", GET_GOLD(ch));
  for (i = 1; i < TALENT_MAX; i++) {
    if (!talent_list[i].in_game) continue;
    int rank = get_talent_rank(ch, i);
    int maxr = talent_max_ranks(i);
    int p_cost = talent_next_point_cost(ch, i);
    int g_cost = talent_next_gold_cost(ch, i);
    if (rank >= maxr)
      send_to_char(ch, "%2d) %-22s Rank:%d/%d  [MAX]\r\n", i, talent_list[i].name, rank, maxr);
    else
      send_to_char(ch, "%2d) %-22s Rank:%d/%d  Next Cost: %d pt%s, %d gold %s\r\n", i, talent_list[i].name, rank, maxr,
                   p_cost, p_cost==1?"":"s", g_cost, can_learn_talent(ch, i)?"(available)":"");
  }
  send_to_char(ch, "\r\nUse 'talents learn <number>' to learn or rank up a talent.\r\n");
}

void gain_talent_point(struct char_data *ch, int amount) {
  if (!ch || IS_NPC(ch) || amount <= 0) return;
  GET_TALENT_POINTS(ch) += amount;
  send_to_char(ch, "\tGYou gain %d crafting talent point%s!\tn\r\n", amount, amount==1?"":"s");
}

/* Commands */
ACMD(do_talents) {
  char arg1[100];
  char arg2[100];
  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
  if (!*arg1) {
    list_talents(ch);
    return;
  }
  if (!strcasecmp(arg1, "learn")) {
    if (!*arg2) {
      send_to_char(ch, "Specify a talent number to learn.\r\n");
      return;
    }
    int num = atoi(arg2);
    if (num <= 0 || num >= TALENT_MAX) {
      send_to_char(ch, "Invalid talent number.\r\n");
      return;
    }
    int p_cost = talent_next_point_cost(ch, num);
    int g_cost = talent_next_gold_cost(ch, num);
    if (!can_learn_talent(ch, num)) {
      if (get_talent_rank(ch, num) >= talent_max_ranks(num))
        send_to_char(ch, "That talent is already at maximum rank.\r\n");
      else if (GET_TALENT_POINTS(ch) < p_cost)
        send_to_char(ch, "You need %d talent point%s to learn the next rank.\r\n", p_cost, p_cost==1?"":"s");
      else if (g_cost > 0 && GET_GOLD(ch) < g_cost)
        send_to_char(ch, "You need %d gold to learn the next rank.\r\n", g_cost);
      else
        send_to_char(ch, "You cannot learn that talent.\r\n");
      return;
    }
    if (learn_talent(ch, num)) return;
    send_to_char(ch, "Learning failed.\r\n");
    return;
  }
  send_to_char(ch, "Unknown option. Use: talents | talents learn <number>\r\n");
}
