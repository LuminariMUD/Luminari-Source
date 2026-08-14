/**
 * @file spec_menu.c
 * Owner-aware special-procedure filtering, selection, and builder presentation.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

#include "comm.h"
#include "oasis.h"
#include "spec_menu.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

static bool spec_olc_definition_selectable(const struct spec_definition *definition,
                                           spec_owner_mask owner)
{
  if (definition == NULL || definition->builder_visibility != SPEC_BUILDER_VISIBLE ||
      spec_definition_callback(definition) == NULL)
    return false;

  return spec_definition_supports_owner(definition, owner) &&
         spec_definition_allows_binding(definition, SPEC_BINDING_SOURCE_WORLD);
}

size_t spec_olc_menu_count(spec_owner_mask owner)
{
  const struct spec_definition *definition;
  size_t definition_index;
  size_t count;

  if (spec_owner_name(owner) == NULL)
    return 0;

  count = 0;
  for (definition_index = 0; definition_index < spec_registry_count(); definition_index++)
  {
    definition = spec_registry_get((int)definition_index);
    if (spec_olc_definition_selectable(definition, owner))
      count++;
  }

  return count;
}

const struct spec_definition *spec_olc_menu_get(spec_owner_mask owner, int index)
{
  const struct spec_definition *definition;
  size_t definition_index;
  size_t filtered_index;

  if (index < 0 || spec_owner_name(owner) == NULL)
    return NULL;

  filtered_index = 0;
  for (definition_index = 0; definition_index < spec_registry_count(); definition_index++)
  {
    definition = spec_registry_get((int)definition_index);
    if (!spec_olc_definition_selectable(definition, owner))
      continue;
    if (filtered_index == (size_t)index)
      return definition;
    filtered_index++;
  }

  return NULL;
}

enum spec_olc_selection_result spec_olc_parse_selection(spec_owner_mask owner, const char *argument,
                                                        const struct spec_definition **definition)
{
  const struct spec_definition *selected_definition;
  char *end;
  long choice;

  if (definition == NULL)
    return SPEC_OLC_SELECTION_INVALID;
  *definition = NULL;

  if (argument == NULL || spec_owner_name(owner) == NULL)
    return SPEC_OLC_SELECTION_INVALID;

  errno = 0;
  end = NULL;
  choice = strtol(argument, &end, 10);
  if (end == argument || errno == ERANGE)
    return SPEC_OLC_SELECTION_INVALID;

  while (*end != '\0' && isspace((unsigned char)*end))
    end++;
  if (*end != '\0')
    return SPEC_OLC_SELECTION_INVALID;

  if (choice == 0)
    return SPEC_OLC_SELECTION_CLEAR;
  if (choice < 1 || choice - 1 > INT_MAX)
    return SPEC_OLC_SELECTION_INVALID;

  selected_definition = spec_olc_menu_get(owner, (int)(choice - 1));
  if (selected_definition == NULL)
    return SPEC_OLC_SELECTION_INVALID;

  *definition = selected_definition;
  return SPEC_OLC_SELECTION_DEFINITION;
}

static const char *spec_olc_owner_title(spec_owner_mask owner)
{
  switch (owner)
  {
  case SPEC_OWNER_MOBILE:
    return "Mobile";
  case SPEC_OWNER_OBJECT:
    return "Object";
  case SPEC_OWNER_ROOM:
    return "Room";
  default:
    return "Unsupported";
  }
}

static void spec_olc_display_prototype_flags(struct descriptor_data *d,
                                             spec_prototype_flag_mask flags)
{
  bool needs_separator;

  needs_separator = false;
  if ((flags & SPEC_PROTOTYPE_MOB_SPEC) != 0)
  {
    write_to_output(d, "MOB_SPEC");
    needs_separator = true;
  }
  if ((flags & SPEC_PROTOTYPE_ITEM_AUTOPROC) != 0)
  {
    if (needs_separator)
      write_to_output(d, ", ");
    write_to_output(d, "ITEM_AUTOPROC");
  }
}

static void spec_olc_display_placements(struct descriptor_data *d, spec_placement_mask placements)
{
  static const struct
  {
    spec_placement_mask flag;
    const char *name;
  } placement_names[] = {
      {SPEC_PLACEMENT_CARRIED, "carried"},         {SPEC_PLACEMENT_EQUIPPED, "equipped"},
      {SPEC_PLACEMENT_COMBAT, "combat"},           {SPEC_PLACEMENT_MOUNTED, "mounted"},
      {SPEC_PLACEMENT_MOVING_ROOM, "moving room"},
  };
  bool needs_separator;
  size_t placement_index;

  needs_separator = false;
  for (placement_index = 0; placement_index < sizeof(placement_names) / sizeof(placement_names[0]);
       placement_index++)
  {
    if ((placements & placement_names[placement_index].flag) == 0)
      continue;
    if (needs_separator)
      write_to_output(d, ", ");
    write_to_output(d, "%s", placement_names[placement_index].name);
    needs_separator = true;
  }
}

static void spec_olc_display_event(struct descriptor_data *d,
                                   const struct spec_event_contract *event)
{
  const char *event_name;
  bool has_flags;
  bool has_placement;

  event_name = spec_event_name(event->event);
  has_flags = event->required_prototype_flags != SPEC_PROTOTYPE_NONE;
  has_placement = event->required_placement != SPEC_PLACEMENT_NONE;

  write_to_output(
      d, "       - %s (prerequisites: ", event_name != NULL ? event_name : "unknown event");
  if (!has_flags && !has_placement)
  {
    write_to_output(d, "none");
  }
  else
  {
    if (has_flags)
    {
      write_to_output(d, "flags ");
      spec_olc_display_prototype_flags(d, event->required_prototype_flags);
    }
    if (has_placement)
    {
      if (has_flags)
        write_to_output(d, "; ");
      write_to_output(d, "placement ");
      spec_olc_display_placements(d, event->required_placement);
    }
  }
  write_to_output(d, ")\r\n");
}

void spec_olc_display_menu(struct descriptor_data *d, spec_owner_mask owner)
{
  const struct spec_definition *definition;
  size_t event_index;
  size_t filtered_index;
  size_t count;

  if (d == NULL)
    return;

  clear_screen(d);
  count = spec_olc_menu_count(owner);
  write_to_output(d, "Special Procedures for %s Prototypes (0 = None)\r\n\r\n",
                  spec_olc_owner_title(owner));

  if (count == 0)
  {
    write_to_output(d, "No compatible builder-visible procedures are available.\r\n");
  }
  else
  {
    for (filtered_index = 0; filtered_index < count; filtered_index++)
    {
      definition = spec_olc_menu_get(owner, (int)filtered_index);
      if (definition == NULL)
        continue;

      write_to_output(d, "%3zu) %s [%s]\r\n", filtered_index + 1, definition->display_name,
                      definition->category);
      write_to_output(d, "     %s\r\n", definition->description);
      write_to_output(d, "     Events:\r\n");
      for (event_index = 0; event_index < definition->event_count; event_index++)
      {
        if (spec_definition_supports_event(definition, owner,
                                           definition->events[event_index].event))
          spec_olc_display_event(d, &definition->events[event_index]);
      }
    }
  }

  write_to_output(d, "\r\nEnter selection (0 to clear, Q to quit): ");
}
