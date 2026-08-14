# Production Playtest: New Account to Level 2

Date verified: 2026-07-27

This is a record of a live, player-facing playtest against the production server. The run
used the normal TCP game interface on local port 4100. It did not use database edits,
administrator commands, or code changes.

## Result

- Disposable account: `Pathcheck`
- Character: `Rellanor`
- Build: male human, true neutral, premade warrior
- Final class and level: level 2 warrior
- Final score evidence: 2,990 total XP, 8,210 XP to next level, 46 maximum HP
- Final quest evidence: 5 quests completed and 5 quest points
- Final gold: 810
- Final location: Ijale's vegetable stand in Mosswood
- Remaining active quest: `Hunt Piglets`

The password is intentionally not recorded here.

## Connecting

The production game process listens on port 4100. This run connected locally:

```text
nc 127.0.0.1 4100
```

`nc` works, but a real MUD client is easier to read because the server sends ANSI color
and Telnet negotiation bytes.

## Account and Character Creation

At the account-name prompt, entering a name that does not exist starts account creation.
The observed sequence was:

1. Enter `Pathcheck` as the account name.
2. Confirm with `y`.
3. Enter and retype a password.
4. At the account menu, enter `c` to create a character.
5. Enter `Rellanor` and confirm with `y`.
6. Choose `m` for sex.
7. Choose `human`, read the race information, and confirm with `y`.
8. Choose `warrior`, read the class information, and confirm with `y`.
9. Choose `premade`.
10. Choose `4` for true neutral.
11. Answer `y` to enable the recommended preference flags.
12. Choose `1` to enter as a non-role-player.
13. Press Return at the welcome screen.
14. Choose `1` at the character menu to enter the game.

An 11-character account-name attempt was rejected, while the 9-character `Pathcheck`
was accepted. That observation alone does not prove the exact length limit.

The character entered at level 1 with 1 total XP and 1,999 XP to the next level. The
premade warrior already had its ability scores, feats, and skills assigned.

## Initial Equipment

The useful initial equipment commands for this warrior were:

```text
wield sword
wear scalemail
wear shield
wear leggings
wear sleeves
wear quiver
wear backpack
equipment
```

The worn backpack starts with important supplies that do not appear in `inventory`.
Use this instead:

```text
look in backpack
```

The starting backpack included a crafting kit, magical torch, cup of water, waybread,
shortbow, and a teleporter.

The game recommends `study` before moving. A premade character cannot edit most study
options, but opening and closing the menu satisfies the introduction:

```text
study
q
y
```

The final `y` saves and finalizes the study choices.

## Training Halls Walkthrough

From the starting beach, go east into the training hall. Equip the starting gear, then
go north to the key tutorial:

```text
east
north
open box
get key box
unlock door north
open door north
north
```

Commands should be entered one at a time. Sending several actions too quickly can put
them in the command queue and make the resulting messages confusing.

### Feats and Combat

At the first four-way junction:

- West is the feats trainer. Use `say ready`, then follow the dialogue. When asked, use
  `study`, then `q` and `y`.
- East is the combat trainer. Use `say ready`.
- Return to the junction and go north for the next tutorials.

The feats dialogue contains stale menu numbers. It says feats are menu 1 and ability
scores are menu 6, while the observed study menu showed feats as 2 and ability scores
as 1. The premade build locks both options, so this did not block progress.

### Communication and Preferences

At the next four-way junction, go west:

```text
ask francesca help
examine board
ask francesca next
open door west
west
ask vicril francesca
say ready
prefedit
q
```

Return east twice to the junction.

### Crafting

Go east from the second junction. The room is labeled "Under Construction", but the
observed tutorial completed successfully.

First clear carried items into the backpack and start:

```text
put all backpack
say ready
```

The critical detail is that the kit, mold, and materials may be inside the worn
backpack. If `put ring kit` says no container was specified, inspect and retrieve them:

```text
look in backpack
get kit backpack
get ring backpack
put ring kit
get material backpack
put material kit
get all.material backpack
put all.material kit
create copper ring
```

The craft took about 66 seconds and completed without further input. It awarded small
amounts of XP as it progressed. Afterward:

```text
wear ring
```

### Shop

Go north from the crafting room:

```text
list
say next
get coins
buy potion
quaff potion
```

The tutorial supplied 100 coins. The potion cost 30.

### Group Tutorial

Return south, west, then north to the narrow hall. The optional group tutorial is up:

```text
up
group new
group
```

The trainer explains `group join`, `report`, `greport`, and follower orders. Go down
when the dialogue finishes.

## Quest Tutorial

Go north from the narrow hall to Demic, then:

```text
say ready
quest list
quest join 1
```

The four quests unlock dungeon sections in sequence.

### Quest 1: Find The Water Source

From Demic:

```text
down
get torch backpack
hold torch
west
south
west
north
north
```

Entering `Leaking Wall` completes the quest. The displayed reward was 100 XP, 75 gold,
and 1 quest point.

Return to Demic:

```text
south
south
east
north
east
up
```

### Quest 2: Kill The Thief

```text
quest join 2
down
south
consider thief
kill thief
north
up
```

The thief was a "perfect match" and died in two automatic combat rounds. The displayed
quest reward was 125 XP, 75 gold, and 1 quest point, in addition to kill XP and gold.

### Quest 3: Obtain The Treasure Key

```text
quest join 3
down
east
examine lever
pull lever
east
south
consider skirker
kill skirker
```

Recommended preferences enabled autoloot, so the treasure key was taken automatically.
The displayed quest reward was 145 XP, 100 gold, and 1 quest point.

Return to Demic:

```text
north
west
west
up
```

### Quest 4: Retrieve The Amulet of Ashenport

```text
quest join 4
down
north
unlock chest
open chest
get amulet chest
south
up
give amulet demic
```

The displayed reward was 200 XP, 250 gold, 1 quest point, and the amulet. The character
had 1,740 total XP and was 260 XP short of level 2 after all four training quests.

The amulet can be equipped with:

```text
wear amulet
```

## Mosswood and Level 2

From Demic, the verified route back to the starting beach is five rooms south and one
room west:

```text
south
south
south
south
south
west
enter portal
```

The portal arrives on the road to Mosswood. The portal at that destination labeled as
the more in-depth tutorial returns to the Training Halls, so do not enter it again after
finishing the halls.

Continue south twice to the Mosswood elder:

```text
south
south
ask elder hi
ask elder next
get ration backpack
give ration elder
quest list
quest join 1
west
```

Entering Ijale's vegetable stand completes `Find Ijale`. The displayed reward was 500
XP, 50 gold, and 1 quest point. The server announced that the character could advance.

For this premade, single-class character, no argument was required:

```text
gain
```

The command advanced Rellanor to level 2 warrior, added 13 maximum HP and 10 maximum
movement, learned Power Attack, and improved the premade skills.

Verify and save:

```text
score
save
```

The final `score` showed level 2, 2,990 total XP, 8,210 XP to next level, 46 maximum HP,
5 completed quests, 5 quest points, and 810 gold.

## Persistence Check

After saving, exit the character and account menus:

```text
quit
```

The first quit displayed an optional new-player exit survey. Pressing Return skipped
it and opened the character menu. From there, use `0` to return to the account menu and
`q` to disconnect.

A completely new TCP connection was then opened and the account was loaded again. The
account menu listed Rellanor as level 2. Entering the game returned to Ijale's vegetable
stand, and `score` still showed level 2, 2,990 total XP, 8,210 XP to next level, 46
maximum HP, 5 completed quests, 5 quest points, and 810 gold.

## Live-Server Observations

- The prompt's `XP` value is XP to next level, not total XP. `score` shows both values.
- The prompt became negative when the character had enough XP to advance.
- On this run, XP changes in `score` were 2.5 times the reward text. For example, the
  displayed 500 XP from `Find Ijale` increased total XP from 1,740 to 2,990.
- `inventory` does not show items inside a worn backpack. Use `look in backpack`.
- The crafting tutorial warning says it may be broken, but it worked after retrieving
  the hidden kit and materials from the backpack.
- `enter portal` on the Mosswood road returns to the Training Halls. It is not a new
  tutorial after the Training Halls have already been completed.
- The elder/Ijale chain automatically started `Hunt Piglets` after `Find Ijale`.
- `save` produced `Saving Rellanor.` on the live server.

## Playthrough Impressions

The game made a strong first impression. The world felt live: other players welcomed
the new character, congratulated the level gain, and appeared naturally throughout the
tutorial. Trainers reacted to the character's class and progress, which made the
onboarding feel connected to the world rather than like a separate command manual.

Character creation offered meaningful mechanical identity without requiring a new
player to understand the entire Pathfinder-based system. The premade build was a good
default, while the custom-build and respec options made it clear that deeper character
building remains available.

The Training Halls were unusually ambitious for a MUD tutorial. They introduced:

- Equipment, containers, doors, keys, and movement
- Feats, skills, preferences, and level advancement
- Communication channels and NPC interaction
- Crafting and shops
- Groups and followers
- Combat, quests, exploration, hidden passages, and quest items

The colored descriptions, automap, contextual hints, quest markers, and `consider`
command also made the traditional text interface more approachable. Reaching level 2
felt earned because it involved exploration, crafting, combat, a small environmental
puzzle, and several kinds of quest objective.

The main weakness was onboarding friction. A good tutorial was sometimes obscured by
long pauses, dense NPC dialogue, ambient spellcasting, hints, and other output. Specific
points of confusion included:

- Stale study-menu numbers in the feats tutorial
- Important starting items hidden inside the backpack but absent from `inventory`
- A crafting room labeled as broken even though its tutorial worked
- Actions becoming confusing when commands entered too quickly were queued
- The Mosswood tutorial portal returning to the area that had just been completed
- Displayed XP rewards differing from the actual XP changes shown by `score`

The combat tutorial explained Pathfinder combat maneuvers, but the two required enemies
were almost defeated by the warrior's first attack. A controlled encounter where a
maneuver such as trip provides a visible advantage would demonstrate the combat system
more effectively.

Overall, the playthrough suggested a deep, carefully built MUD with a friendly community
and many years of accumulated systems. The underlying game was compelling enough to
make further exploration appealing. The largest opportunity is to streamline the first
hour: shorten passive dialogue, reduce unrelated output during instructions, update
stale commands, expose backpack contents more clearly, and make tutorial objectives and
XP feedback unambiguous.
