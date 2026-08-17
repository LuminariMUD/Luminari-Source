#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "mob/mob_autoroll.h"

#include <stdio.h>
#include <string.h>

#define PROTOCOL_VERSION 1

static void write_stats(const struct mob_autoroll_stats *stats)
{
  printf(" %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", stats->hit_points,
         stats->hitroll, stats->armor_class, stats->damage_dice_count, stats->damage_dice_size,
         stats->damage_bonus, stats->experience, stats->gold, stats->strength, stats->strength_add,
         stats->intelligence, stats->wisdom, stats->dexterity, stats->constitution, stats->charisma,
         stats->saving_fortitude, stats->saving_reflex, stats->saving_will, stats->saving_poison,
         stats->saving_death, stats->spell_resistance);
}

int main(int argc, char **argv)
{
  struct mob_autoroll_config config;
  struct mob_autoroll_input input;
  struct mob_autoroll_result result;
  char line[1024];
  char command[32];
  int identifier;
  int version;

  if (argc == 2 && strcmp(argv[1], "--version") == 0)
  {
    printf("rol_mob_calculator protocol=%d profile=%d config=%d\n", PROTOCOL_VERSION,
           MOB_AUTOROLL_PROFILE_V1, MOB_AUTOROLL_CONFIG_V1);
    return 0;
  }
  if (argc != 1)
  {
    fprintf(stderr, "usage: rol_mob_calculator [--version]\n");
    return 2;
  }

  mob_autoroll_default_config(&config);
  if (!fgets(line, sizeof(line), stdin) || sscanf(line, "%31s %d", command, &version) != 2 ||
      strcmp(command, "ROL_MOB_CALCULATOR") != 0 || version != PROTOCOL_VERSION)
  {
    fprintf(stderr, "invalid or unsupported protocol header\n");
    return 2;
  }
  printf("ROL_MOB_CALCULATOR %d\n", PROTOCOL_VERSION);
  if (fflush(stdout) != 0)
    return 1;

  while (fgets(line, sizeof(line), stdin))
  {
    if (strcmp(line, "END\n") == 0 || strcmp(line, "END\r\n") == 0)
    {
      printf("END\n");
      return fflush(stdout) == 0 ? 0 : 1;
    }
    if (sscanf(line, "%31s %d %d %d %d %d %d", command, &identifier, &input.level, &input.race,
               &input.ch_class, &input.tier, &input.custom_profile) != 7 ||
        strcmp(command, "MOB") != 0)
    {
      fprintf(stderr, "invalid request row: %s", line);
      return 2;
    }
    if (!mob_autoroll_calculate(&input, &config, &result))
    {
      fprintf(stderr, "calculator rejected mobile %d\n", identifier);
      return 2;
    }
    printf("MOB %d %d %d %d %d %d %d %d", identifier, result.profile_version, result.category,
           input.level, input.race, input.ch_class, input.tier, result.custom_profile);
    write_stats(&result.persisted);
    write_stats(&result.expected_post_load);
    putchar('\n');
    if (fflush(stdout) != 0)
      return 1;
  }

  fprintf(stderr, "request stream ended before END\n");
  return 2;
}
