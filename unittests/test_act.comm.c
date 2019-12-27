#include "minunit.h"

#include "../act.h"
#include "../structs.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>


// testable
void do_spec_comm(struct char_data *ch, char *argument, int cmd, int subcmd);

// mocks
char *act(const char *str, int hide_invisible, struct char_data *ch,
        struct obj_data *obj, void *vict_obj, int type)
{
  static int call = 0;
  if (++call == 1)
    mu_equals("$n asks you, 'Text?'", str);

  return NULL;
}

size_t send_to_char(struct char_data *ch, const char *messg, ...)
{
  char dest[2048];
  va_list args;
  va_start(args, messg);
  vsnprintf(dest, sizeof(dest), messg, args);
  va_end(args);

  static int call = 0;
  if (++call == 1)
    mu_equals("You ask TargetName, 'Text?'\r\n", dest);
  return 0;
}

struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where)
{
  static struct char_data target = {0};
  target.player.name = "TargetName";
  return &target;
}

// test
int main()
{
  // arrange
  struct player_special_data specials = {0};
  struct char_data sayer = {0};
  sayer.player_specials = &specials;

  // act
  char argument[] = "target text";
  do_spec_comm(&sayer, argument, 0, SCMD_ASK);

  // assert in mocks
}
