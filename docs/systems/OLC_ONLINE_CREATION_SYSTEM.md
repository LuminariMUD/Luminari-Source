# LuminariMUD Online Level Creation (OLC) System

## Overview

The Online Level Creation (OLC) system in LuminariMUD provides in-game tools
for builders to create and modify rooms, objects, mobiles, shops, zones,
triggers, numbered quests, and mobile-hosted high-level quests. Editing occurs
against live in-memory copies; saving writes the applicable flat-file package.

> **About the code samples in this document.** Most C listings below are
> *illustrative*: they show the shape of the system, not the literal source.
> Several named functions (`olc_save_to_disk()`, `validate_room_data()`,
> `create_new_room()`, `trigedit_save_to_disk()`, and others) do not exist under
> `src/`, and the `massroomset` command described near the end of this document
> is not registered in `cmd_info[]`. Read them as pseudocode. For the actual
> implementation, start at `src/olc/oasis.c` and the individual `*edit.c` files.

## Command Reference

Every editor and listing command below is registered in `cmd_info[]`
(`src/interpreter.c`). All are `LVL_BUILDER` unless noted.

### Editors

| Command | Edits | Notes |
|---------|-------|-------|
| `redit` | Rooms | Also `dig` to create and link a room in one step |
| `oedit` | Objects | See the [OEDIT Guide](../world_game-data/OEDIT_GUIDE.md) |
| `medit` | Mobiles | |
| `zedit` | Zones and reset commands | |
| `sedit` | Shops | |
| `trigedit` | DG Script triggers | See [SCRIPTING_SYSTEM_DG.md](SCRIPTING_SYSTEM_DG.md) |
| `qedit` | Numbered quests | Uses a quest VNUM; see the [QST format](../world_game-data/QUEST_FILE_FORMAT.md) |
| `hlqedit` | High-level quests | Uses an existing host mobile VNUM; see the [HLQ format](../world_game-data/HLQUEST_FILE_FORMAT.md) |
| `hedit` | Help entries | Database-backed; see [HELP_SYSTEM.md](HELP_SYSTEM.md) |
| `aedit` | Socials | `LVL_STAFF`. See [docs/utilities/README.md](../utilities/README.md) |
| `iedit` | A live object instance | `LVL_STAFF`. Scripting is disabled in this mode |
| `bedit` | Message boards | Database-backed; `blist` shows existing boards |

### Listing Commands

These are how you discover which vnums are already in use before creating
anything. Skipping this step is the most common cause of vnum collisions.

| Command | Lists |
|---------|-------|
| `rlist` | Rooms |
| `mlist` | Mobiles |
| `olist` | Objects |
| `slist` | Shops |
| `tlist` | Triggers |
| `qlist` | Quests |
| `zlist` | Zones (no level restriction) |
| `blist` | Message boards |
| `pathlist` | Wilderness paths; only usable while in the wilderness |

All of them except `blist` and `pathlist` are the same function
(`do_oasis_list()` in `src/olc/oasis_list.c`) and take the same arguments:

```
rlist              List everything in the zone you are standing in
rlist .            Same as above
rlist 30           List everything in zone 30
rlist 3000 3099    List everything in the vnum range 3000-3099
```

`zlist` additionally accepts a builder name instead of a zone number, which
lists the zones that builder is assigned to.

Note that `plist` is **not** part of this family - it lists *players* and lives
in `src/act.wizard.c`.

## OLC Architecture

### Core Components

#### 1. OLC States (`olc_state_t`)
```c
typedef enum {
  CON_PLAYING = 0,
  CON_REDIT,     // Room editing
  CON_OEDIT,     // Object editing  
  CON_ZEDIT,     // Zone editing
  CON_MEDIT,     // Mobile editing
  CON_SEDIT,     // Shop editing
  CON_TEDIT,     // Text editing
  CON_QEDIT,     // Quest editing
  CON_HEDIT,     // Help editing
  CON_AEDIT,     // Action editing
  CON_TRIGEDIT,  // Trigger editing
  CON_HLQEDIT    // High-level quest editing
} olc_state_t;
```

#### 2. OLC Data Structure
```c
struct olc_data {
  int mode;                    // Current editing mode
  room_rnum real_num;          // Real number being edited
  int number;                  // Virtual number
  int max_num;                 // Maximum allowed number
  char *storage;               // Temporary storage buffer
  struct char_data *ch;        // Character doing the editing
  int script_mode;             // Script editing mode
  int trigger_position;        // Current trigger position
  int item_type;               // Type of item being edited
  struct extra_descr_data **desc; // Extra descriptions
  struct social_messg **action;   // Social actions
  union {
    struct room_data *room;    // Room data
    struct obj_data *obj;      // Object data
    struct char_data *mob;     // Mobile data
    struct shop_data *shop;    // Shop data
    struct zone_data *zone;    // Zone data
    struct aq_data *quest;     // Numbered QEDIT quest data
    struct quest_entry *hlquest; // HLQEDIT host entry list
    struct quest_entry *entry; // HLQEDIT entry under construction
    struct quest_command *qcom; // HLQEDIT command under construction
    struct help_index_element *help; // Help data
    struct trig_data *trig;    // Trigger data
  } data;
};
```

The actual `struct oasis_olc_data` is not a union and contains additional
editor state. See `src/olc/oasis.h`; the listing above shows only the ownership
shape relevant to the editors discussed here.

## Quest Editors (QEDIT and HLQEDIT)

The two quest editors are separate systems and must not be treated as aliases.

### QEDIT: Numbered Quests

Start QEDIT with a quest VNUM owned by an editable zone:

```text
qedit 3001
```

QEDIT edits one `struct aq_data` record. Its menu covers the questmaster,
source-defined quest type and flags, type-dependent target, prerequisite,
quantity, level range, time limit, return recipient, rewards, previous/next
links, and dialogue fields. A new quest starts with type `AQ_UNDEFINED`; set a
real type and questmaster before expecting players to discover it.

Saving updates the in-memory quest table and writes the owning `.qst` package
through `save_quests()` in `src/olc/genqst.c`. The writer emits the current
seven-field reward row and a `D` block. The loader also accepts the historical
three-field reward row and an omitted `D` block, as documented in the
[Quest File Format](../world_game-data/QUEST_FILE_FORMAT.md).

After answering `Q` and `Y`, validate and inspect the same VNUM:

```sh
python3 scripts/world/wtool.py validate --zone 30
python3 scripts/world/wtool.py show quest 3001
python3 scripts/world/wtool.py refs quest 3001
```

QEDIT can verify many direct VNUM choices while editing, but it does not make
chain topology, every persisted scalar, or every cross-package condition safe.
The validator supplies those checks before boot and playtesting.

### HLQEDIT: Mobile-Hosted Entries

HLQEDIT's numeric argument is an existing host mobile VNUM, not a quest VNUM:

```text
hlqedit 3000
```

The editor refuses a host that does not exist and prevents simultaneous MEDIT
or HLQEDIT work on that mobile. One host can contain:

- ASK entries with keywords and a reply;
- GIVE entries with coin/item inputs, a reply, and output commands; and
- ROOM entries with a room trigger, a reply, and output commands.

Approval is a normal workflow state. An unapproved entry is retained and is
not a validation error; the canonical approved disk marker is `!`.

The output-command menu supports coins, item delivery, object/mobile loads,
attack, disappear, teach/cast spell, open door, follow, kit/lich transition,
and church changes. Only coin and item commands are consumed as GIVE inputs.
ROOM inputs and other input command types can exist in hand-edited files but
are ignored by the runtime and are validator errors.

Saving writes both the zone's `.hlq` package and its mobile package. The disk
loader prepends entries and input commands but appends output commands, so
physical file order and effective runtime order differ. Inspect both views
after saving:

```sh
python3 scripts/world/wtool.py validate --zone 30
python3 scripts/world/wtool.py show hlquest 3000
python3 scripts/world/wtool.py refs hlquest 3000
```

The [High-Level Quest File Format](../world_game-data/HLQUEST_FILE_FORMAT.md)
lists every command code, legal direction, scalar bound, reference role, and
loader hazard.

### Required Runtime Check

Static validation cannot execute either quest system. After a clean structural
pass, boot only a development server and play through the changed behavior.
For QEDIT, test join gates, target progress, completion, quit behavior,
rewards, and chain/dialogue transitions. For HLQEDIT, test the intended ASK,
GIVE, and ROOM matches and verify input consumption and output effects in the
runtime order shown by `wtool`.

## Room Editor (REDIT)

### Basic Room Editing
```c
// Enter room editing mode
ACMD(do_redit) {
  room_vnum vnum;
  room_rnum real_num;

  one_argument(argument, buf);

  if (!*buf) {
    vnum = world[IN_ROOM(ch)].number;
  } else {
    vnum = atoi(buf);
  }

  if ((real_num = real_room(vnum)) == NOWHERE) {
    // Create new room
    if (can_edit_zone(ch, zone_table[vnum / 100].number)) {
      real_num = create_new_room(vnum);
    } else {
      send_to_char(ch, "You don't have permission to edit that zone.\r\n");
      return;
    }
  }

  // Enter editing mode
  STATE(ch->desc) = CON_REDIT;
  act("$n starts using OLC.", TRUE, ch, 0, 0, TO_ROOM);
  SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

  // Initialize OLC data
  CREATE(OLC(ch->desc), struct olc_data, 1);
  OLC_ROOM(ch->desc) = &world[real_num];
  OLC_NUM(ch->desc) = vnum;
  OLC_VAL(ch->desc) = 0;

  redit_disp_menu(ch->desc);
}
```

### Room Editing Menu
```c
void redit_disp_menu(struct descriptor_data *d) {
  struct room_data *room = OLC_ROOM(d);

  clear_screen(d);
  write_to_output(d,
    "-- Room Number: [%s%d%s]  Room Zone: [%s%d%s]\r\n"
    "%s1%s) Name        : %s%s\r\n"
    "%s2%s) Description :\r\n%s%s"
    "%s3%s) Room flags  : %s%s\r\n"
    "%s4%s) Sector type : %s%s\r\n"
    "%s5%s) Exit north  : %s%d\r\n"
    "%s6%s) Exit east   : %s%d\r\n"
    "%s7%s) Exit south  : %s%d\r\n"
    "%s8%s) Exit west   : %s%d\r\n"
    "%s9%s) Exit up     : %s%d\r\n"
    "%sA%s) Exit down   : %s%d\r\n"
    "%sB%s) Extra descriptions menu\r\n"
    "%sC%s) Script      : %s%s\r\n"
    "%sS%s) Script      : %s%s\r\n"
    "%sW%s) Copy room\r\n"
    "%sX%s) Delete room\r\n"
    "%sQ%s) Quit\r\n"
    "Enter choice : ",

    cyn, OLC_NUM(d), nrm,
    cyn, zone_table[room->zone].number, nrm,
    grn, nrm, yel, room->name ? room->name : "Undefined",
    grn, nrm, yel, room->description ? room->description : "Undefined\r\n",
    grn, nrm, cyn, room_bits[room->room_flags],
    grn, nrm, cyn, sector_types[room->sector_type],
    grn, nrm, cyn, room->dir_option[NORTH] ? room->dir_option[NORTH]->to_room : -1,
    grn, nrm, cyn, room->dir_option[EAST] ? room->dir_option[EAST]->to_room : -1,
    grn, nrm, cyn, room->dir_option[SOUTH] ? room->dir_option[SOUTH]->to_room : -1,
    grn, nrm, cyn, room->dir_option[WEST] ? room->dir_option[WEST]->to_room : -1,
    grn, nrm, cyn, room->dir_option[UP] ? room->dir_option[UP]->to_room : -1,
    grn, nrm, cyn, room->dir_option[DOWN] ? room->dir_option[DOWN]->to_room : -1,
    grn, nrm,
    grn, nrm, cyn, room->proto_script ? "Set" : "Not Set",
    grn, nrm, cyn, room->script ? "Set" : "Not Set",
    grn, nrm,
    grn, nrm,
    grn, nrm
  );

  OLC_MODE(d) = REDIT_MAIN_MENU;
}
```

### Exit Editing
```c
void redit_disp_exit_menu(struct descriptor_data *d) {
  struct room_direction_data *exit = OLC_EXIT(d);

  write_to_output(d,
    "%s-- Exit info --\r\n"
    "%s1%s) Exit to room : %s%d\r\n"
    "%s2%s) Description  : %s%s\r\n"
    "%s3%s) Door name    : %s%s\r\n"
    "%s4%s) Key          : %s%d\r\n"
    "%s5%s) Door flags   : %s%s\r\n"
    "%s6%s) Purge exit.\r\n"
    "Enter choice, 0 to quit : ",

    cyn,
    grn, nrm, cyn, exit && exit->to_room != NOWHERE ? world[exit->to_room].number : -1,
    grn, nrm, yel, exit && exit->general_description ? exit->general_description : "None",
    grn, nrm, yel, exit && exit->keyword ? exit->keyword : "None",
    grn, nrm, cyn, exit && exit->key != NOTHING ? exit->key : -1,
    grn, nrm, cyn, exit ? exit_bits[exit->exit_info] : "None",
    grn, nrm
  );
}
```

## Object Editor (OEDIT)

### Object Creation and Editing
```c
ACMD(do_oedit) {
  obj_vnum vnum;
  obj_rnum real_num;
  struct obj_data *obj;

  one_argument(argument, buf);

  if (!*buf) {
    send_to_char(ch, "Specify an object VNUM to edit.\r\n");
    return;
  }

  vnum = atoi(buf);

  if ((real_num = real_object(vnum)) == NOTHING) {
    // Create new object
    if (!can_edit_zone(ch, vnum / 100)) {
      send_to_char(ch, "You don't have permission to edit that zone.\r\n");
      return;
    }

    real_num = create_new_object(vnum);
  }

  obj = &obj_proto[real_num];

  // Enter editing mode
  STATE(ch->desc) = CON_OEDIT;
  act("$n starts using OLC.", TRUE, ch, 0, 0, TO_ROOM);
  SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

  // Initialize OLC data
  CREATE(OLC(ch->desc), struct olc_data, 1);
  OLC_OBJ(ch->desc) = obj;
  OLC_NUM(ch->desc) = vnum;
  OLC_VAL(ch->desc) = 0;

  oedit_disp_menu(ch->desc);
}
```

### Object Properties Menu
```c
void oedit_disp_menu(struct descriptor_data *d) {
  struct obj_data *obj = OLC_OBJ(d);

  clear_screen(d);
  write_to_output(d,
    "-- Object Number: [%s%d%s]\r\n"
    "%s1%s) Namelist    : %s%s\r\n"
    "%s2%s) S-Desc      : %s%s\r\n"
    "%s3%s) L-Desc      :\r\n%s%s\r\n"
    "%s4%s) A-Desc      :\r\n%s%s\r\n"
    "%s5%s) Type        : %s%s\r\n"
    "%s6%s) Extra flags : %s%s\r\n"
    "%s7%s) Wear flags  : %s%s\r\n"
    "%s8%s) Weight      : %s%d\r\n"
    "%s9%s) Cost        : %s%d\r\n"
    "%sA%s) Cost/Day    : %s%d\r\n"
    "%sB%s) Timer       : %s%d\r\n"
    "%sC%s) Values      : %s%s\r\n"
    "%sD%s) Applies menu\r\n"
    "%sE%s) Extra descriptions menu\r\n"
    "%sM%s) Min Level   : %s%d\r\n"
    "%sP%s) Perm Affects: %s%s\r\n"
    "%sS%s) Script      : %s%s\r\n"
    "%sW%s) Copy object\r\n"
    "%sX%s) Delete object\r\n"
    "%sQ%s) Quit\r\n"
    "Enter choice : ",

    cyn, OLC_NUM(d), nrm,
    grn, nrm, yel, obj->name ? obj->name : "undefined",
    grn, nrm, yel, obj->short_description ? obj->short_description : "undefined",
    grn, nrm, yel, obj->description ? obj->description : "undefined",
    grn, nrm, yel, obj->action_description ? obj->action_description : "Not Set",
    grn, nrm, cyn, item_types[GET_OBJ_TYPE(obj)],
    grn, nrm, cyn, extra_bits[GET_OBJ_EXTRA(obj)],
    grn, nrm, cyn, wear_bits[GET_OBJ_WEAR(obj)],
    grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
    grn, nrm, cyn, GET_OBJ_COST(obj),
    grn, nrm, cyn, GET_OBJ_RENT(obj),
    grn, nrm, cyn, GET_OBJ_TIMER(obj),
    grn, nrm, cyn, oedit_get_values(obj),
    grn, nrm,
    grn, nrm,
    grn, nrm, cyn, GET_OBJ_LEVEL(obj),
    grn, nrm, cyn, affected_bits[obj->obj_flags.bitvector],
    grn, nrm, cyn, obj->proto_script ? "Set" : "Not Set",
    grn, nrm,
    grn, nrm,
    grn, nrm
  );

  OLC_MODE(d) = OEDIT_MAIN_MENU;
}
```

### Object Values Editing
```c
void oedit_disp_val1_menu(struct descriptor_data *d) {
  struct obj_data *obj = OLC_OBJ(d);

  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_WEAPON:
      write_to_output(d, "Weapon type (0-13): ");
      break;
    case ITEM_ARMOR:
      write_to_output(d, "AC-apply: ");
      break;
    case ITEM_CONTAINER:
      write_to_output(d, "Max weight: ");
      break;
    case ITEM_DRINKCON:
      write_to_output(d, "Max drink units: ");
      break;
    case ITEM_FOOD:
      write_to_output(d, "Hours to fill stomach: ");
      break;
    case ITEM_MONEY:
      write_to_output(d, "Number of coins: ");
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      write_to_output(d, "Spell level: ");
      break;
    default:
      write_to_output(d, "Value: ");
      break;
  }
}
```

## Mobile Editor (MEDIT)

### Mobile Creation
```c
ACMD(do_medit) {
  mob_vnum vnum;
  mob_rnum real_num;
  struct char_data *mob;

  one_argument(argument, buf);

  if (!*buf) {
    send_to_char(ch, "Specify a mobile VNUM to edit.\r\n");
    return;
  }

  vnum = atoi(buf);

  if ((real_num = real_mobile(vnum)) == NOBODY) {
    // Create new mobile
    if (!can_edit_zone(ch, vnum / 100)) {
      send_to_char(ch, "You don't have permission to edit that zone.\r\n");
      return;
    }

    real_num = create_new_mobile(vnum);
  }

  mob = &mob_proto[real_num];

  // Enter editing mode
  STATE(ch->desc) = CON_MEDIT;
  act("$n starts using OLC.", TRUE, ch, 0, 0, TO_ROOM);
  SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

  // Initialize OLC data
  CREATE(OLC(ch->desc), struct olc_data, 1);
  OLC_MOB(ch->desc) = mob;
  OLC_NUM(ch->desc) = vnum;
  OLC_VAL(ch->desc) = 0;

  medit_disp_menu(ch->desc);
}
```

### Mobile Stats Menu
```c
void medit_disp_stats_menu(struct descriptor_data *d) {
  struct char_data *mob = OLC_MOB(d);

  write_to_output(d,
    "%s-- Mob Number:  [%s%d%s]\r\n"
    "%s1%s) Level       : %s%d\r\n"
    "%s2%s) Hitroll     : %s%d\r\n"
    "%s3%s) Damroll     : %s%d\r\n"
    "%s4%s) NumDamDice  : %s%d\r\n"
    "%s5%s) SizeDamDice : %s%d\r\n"
    "%s6%s) Num HP Dice : %s%d\r\n"
    "%s7%s) Size HP Dice: %s%d\r\n"
    "%s8%s) HP Bonus    : %s%d\r\n"
    "%s9%s) Armor Class : %s%d\r\n"
    "%sA%s) Exp         : %s%d\r\n"
    "%sB%s) Gold        : %s%d\r\n"
    "%sC%s) Strength    : %s%d\r\n"
    "%sD%s) Intelligence: %s%d\r\n"
    "%sE%s) Wisdom      : %s%d\r\n"
    "%sF%s) Dexterity   : %s%d\r\n"
    "%sG%s) Constitution: %s%d\r\n"
    "%sH%s) Charisma    : %s%d\r\n"
    "%sQ%s) Quit\r\n"
    "Enter choice : ",

    cyn, cyn, OLC_NUM(d), nrm,
    grn, nrm, cyn, GET_LEVEL(mob),
    grn, nrm, cyn, GET_HITROLL(mob),
    grn, nrm, cyn, GET_DAMROLL(mob),
    grn, nrm, cyn, GET_NDD(mob),
    grn, nrm, cyn, GET_SDD(mob),
    grn, nrm, cyn, GET_HIT(mob),
    grn, nrm, cyn, GET_MANA(mob),
    grn, nrm, cyn, GET_MOVE(mob),
    grn, nrm, cyn, GET_AC(mob),
    grn, nrm, cyn, GET_EXP(mob),
    grn, nrm, cyn, GET_GOLD(mob),
    grn, nrm, cyn, GET_STR(mob),
    grn, nrm, cyn, GET_INT(mob),
    grn, nrm, cyn, GET_WIS(mob),
    grn, nrm, cyn, GET_DEX(mob),
    grn, nrm, cyn, GET_CON(mob),
    grn, nrm, cyn, GET_CHA(mob),
    grn, nrm
  );

  OLC_MODE(d) = MEDIT_STATS_MENU;
}
```

## Zone Editor (ZEDIT)

### Zone Commands

Zone reset commands are stored as `struct reset_com` (`src/db.h`):

```c
struct reset_com
{
  char command; /* current command                      */

  signed char if_flag; /* if TRUE: exe only if preceding exe'd */
  int arg1;            /*                                      */
  int arg2;            /* Arguments to the command             */
  int arg3;            /*                                      */
  int arg4;            /* probability of command executing     */
  int line;            /* line number this command appears on  */
  char *sarg1;         /* string argument                      */
  char *sarg2;         /* string argument                      */
};
```

There are no `ZCMD_*` constants - the command is stored and compared as a bare
character literal throughout `src/db.c` and `src/olc/zedit.c`. The full command
set, argument arities, and file syntax are documented in the
[Zone File Format Reference](../world_game-data/ZONE_FILE_FORMAT.md).

| Char | Meaning |
|------|---------|
| `M` | Load a mobile into a room |
| `O` | Load an object into a room |
| `P` | Put an object into another object |
| `G` | Give an object to the last mobile loaded |
| `E` | Equip the last mobile loaded with an object |
| `D` | Set the state of a door |
| `R` | Remove an object from a room |
| `T` | Attach a trigger |
| `V` | Assign a DG script variable |
| `J` | Load a mobile with a load probability |
| `I` | Load random treasure on a mobile |
| `L` | Load random treasure into a container |
| `S` | Terminates the command list (required) |
| `*` | Comment line |

### Zone Reset Menu
```c
void zedit_disp_menu(struct descriptor_data *d) {
  struct zone_data *zone = OLC_ZONE(d);
  int i = 0;

  clear_screen(d);
  write_to_output(d,
    "Zone number: %s%d%s        Zone name: %s%s%s\r\n"
    "Builders: %s%s%s\r\n"
    "Top of zone: %s%d%s        Lifespan: %s%d%s minutes\r\n"
    "Reset Mode: %s%s%s        Zone flags: %s%s%s\r\n"
    "Min Level: %s%d%s         Max Level: %s%d%s\r\n\r\n",

    cyn, zone->number, nrm,
    yel, zone->name ? zone->name : "Undefined", nrm,
    yel, zone->builders ? zone->builders : "None", nrm,
    cyn, zone->top, nrm,
    cyn, zone->lifespan, nrm,
    cyn, zone->reset_mode ? ((zone->reset_mode == 1) ? "Reset when empty" : "Normal reset") : "Never reset", nrm,
    cyn, zone->zone_flags ? "Set" : "None", nrm,
    cyn, zone->min_level, nrm,
    cyn, zone->max_level, nrm
  );

  // Display zone commands
  write_to_output(d, "Zone commands:\r\n");
  for (i = 0; zone->cmd[i].command != 'S'; i++) {
    write_to_output(d, "%s%2d%s - %s\r\n",
                    cyn, i, nrm,
                    zedit_get_command_desc(&zone->cmd[i]));
  }

  write_to_output(d,
    "\r\n%sN%s) New command.    %sE%s) Edit a command.    %sD%s) Delete a command.\r\n"
    "%sZ%s) Zone info.       %sQ%s) Quit\r\n"
    "Enter choice : ",
    grn, nrm, grn, nrm, grn, nrm,
    grn, nrm, grn, nrm
  );

  OLC_MODE(d) = ZEDIT_MAIN_MENU;
}
```

## Script Editor (TRIGEDIT)

### Trigger Editing
```c
void trigedit_disp_menu(struct descriptor_data *d) {
  struct trig_data *trig = OLC_TRIG(d);

  clear_screen(d);
  write_to_output(d,
    "Trigger Editor [%s%d%s]\r\n"
    "%s1%s) Name         : %s%s\r\n"
    "%s2%s) Intended for : %s%s\r\n"
    "%s3%s) Trigger types: %s%s\r\n"
    "%s4%s) Numeric Arg  : %s%d\r\n"
    "%s5%s) Arguments    : %s%s\r\n"
    "%s6%s) Commands:\r\n%s%s\r\n"
    "%sQ%s) Quit\r\n"
    "Enter choice : ",

    cyn, OLC_NUM(d), nrm,
    grn, nrm, yel, trig->name ? trig->name : "Undefined",
    grn, nrm, cyn, attach_types[trig->attach_type],
    grn, nrm, cyn, trigedit_get_type_desc(trig),
    grn, nrm, cyn, trig->narg,
    grn, nrm, yel, trig->arglist ? trig->arglist : "None",
    grn, nrm, yel, trigedit_get_command_list(trig),
    grn, nrm
  );

  OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
}
```

## Permission System

### Builder Permissions
```c
bool can_edit_zone(struct char_data *ch, zone_vnum zone_num) {
  struct zone_data *zone;
  char *builders, *builder_name;

  // Implementors can edit anything
  if (GET_LEVEL(ch) >= LVL_IMPL) {
    return TRUE;
  }

  // Find the zone
  if ((zone = real_zone(zone_num)) == NULL) {
    return FALSE;
  }

  // Check if character is listed as a builder
  if (!zone->builders || !*zone->builders) {
    return FALSE; // No builders specified
  }

  builders = strdup(zone->builders);
  builder_name = strtok(builders, " ");

  while (builder_name) {
    if (!str_cmp(builder_name, GET_NAME(ch))) {
      free(builders);
      return TRUE;
    }
    builder_name = strtok(NULL, " ");
  }

  free(builders);
  return FALSE;
}
```

### OLC Security
```c
bool olc_security_check(struct char_data *ch, int vnum) {
  zone_vnum zone = vnum / 100;

  // Check basic permissions
  if (!can_edit_zone(ch, zone)) {
    send_to_char(ch, "You don't have permission to edit zone %d.\r\n", zone);
    return FALSE;
  }

  // Check if zone is locked
  if (zone_table[zone].locked) {
    send_to_char(ch, "Zone %d is currently locked for editing.\r\n", zone);
    return FALSE;
  }

  // Check if vnum is in valid range
  if (vnum < zone * 100 || vnum > zone_table[zone].top) {
    send_to_char(ch, "VNUM %d is outside the valid range for zone %d.\r\n",
                 vnum, zone);
    return FALSE;
  }

  return TRUE;
}
```

## OLC Utilities

### Save Functions
```c
void olc_save_to_disk(struct descriptor_data *d) {
  switch (STATE(d)) {
    case CON_REDIT:
      redit_save_to_disk(d);
      break;
    case CON_OEDIT:
      oedit_save_to_disk(d);
      break;
    case CON_MEDIT:
      medit_save_to_disk(d);
      break;
    case CON_ZEDIT:
      zedit_save_to_disk(d);
      break;
    case CON_SEDIT:
      sedit_save_to_disk(d);
      break;
    case CON_TRIGEDIT:
      trigedit_save_to_disk(d);
      break;
  }

  send_to_char(d->character, "Saved to disk.\r\n");
}
```

### Validation Functions
```c
bool validate_room_data(struct room_data *room) {
  // Check required fields
  if (!room->name || !*room->name) {
    return FALSE;
  }

  if (!room->description || !*room->description) {
    return FALSE;
  }

  // Validate exits
  int dir;
  for (dir = 0; dir < NUM_OF_DIRS; dir++) {
    if (room->dir_option[dir]) {
      if (room->dir_option[dir]->to_room == NOWHERE) {
        return FALSE;
      }
    }
  }

  return TRUE;
}

bool validate_object_data(struct obj_data *obj) {
  // Check required fields
  if (!obj->name || !*obj->name) {
    return FALSE;
  }

  if (!obj->short_description || !*obj->short_description) {
    return FALSE;
  }

  // Validate object values based on type
  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_WEAPON:
      if (GET_OBJ_VAL(obj, 1) <= 0 || GET_OBJ_VAL(obj, 2) <= 0) {
        return FALSE; // Invalid weapon dice
      }
      break;
    case ITEM_ARMOR:
      if (GET_OBJ_VAL(obj, 0) < 0) {
        return FALSE; // Negative AC bonus
      }
      break;
  }

  return TRUE;
}
```

## Advanced OLC Features

### Copy and Paste
```c
ACMD(do_copy) {
  char from_arg[MAX_INPUT_LENGTH], to_arg[MAX_INPUT_LENGTH];
  int from_vnum, to_vnum;

  two_arguments(argument, from_arg, to_arg);

  if (!*from_arg || !*to_arg) {
    send_to_char(ch, "Usage: copy <from_vnum> <to_vnum>\r\n");
    return;
  }

  from_vnum = atoi(from_arg);
  to_vnum = atoi(to_arg);

  // Validate permissions for both vnums
  if (!olc_security_check(ch, from_vnum) || !olc_security_check(ch, to_vnum)) {
    return;
  }

  // Perform the copy based on current editing mode
  switch (STATE(ch->desc)) {
    case CON_REDIT:
      copy_room(from_vnum, to_vnum);
      break;
    case CON_OEDIT:
      copy_object(from_vnum, to_vnum);
      break;
    case CON_MEDIT:
      copy_mobile(from_vnum, to_vnum);
      break;
  }

  send_to_char(ch, "Copied %d to %d.\r\n", from_vnum, to_vnum);
}
```

### Batch Operations
```c
ACMD(do_massroomset) {
  char field_arg[MAX_INPUT_LENGTH], value_arg[MAX_INPUT_LENGTH];
  char range_arg[MAX_INPUT_LENGTH];
  int start_vnum, end_vnum, i;

  three_arguments(argument, range_arg, field_arg, value_arg);

  if (!*range_arg || !*field_arg || !*value_arg) {
    send_to_char(ch, "Usage: massroomset <start-end> <field> <value>\r\n");
    return;
  }

  // Parse range
  sscanf(range_arg, "%d-%d", &start_vnum, &end_vnum);

  // Apply changes to all rooms in range
  for (i = start_vnum; i <= end_vnum; i++) {
    room_rnum real_num = real_room(i);
    if (real_num == NOWHERE) continue;

    if (!can_edit_zone(ch, i / 100)) continue;

    // Apply the field change
    apply_room_field_change(&world[real_num], field_arg, value_arg);
  }

  send_to_char(ch, "Applied changes to rooms %d-%d.\r\n", start_vnum, end_vnum);
}
```

---

*This documentation covers the core OLC system functionality. For specific editor commands and advanced features, refer to the individual OLC source files and builder documentation.*
