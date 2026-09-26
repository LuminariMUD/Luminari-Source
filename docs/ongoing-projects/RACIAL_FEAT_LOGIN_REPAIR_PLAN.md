# Racial Feat Login Repair Plan

Status: plan, not started. Written 2026-09-26 and traced against master `a845fd41e`; item 2 was
done by the Duris races work.

## Problem

A character receives racial feats only from `process_race_level_feats()` (`src/character/class.c`),
which `advance_level()` runs for the level just reached. A feat added to an existing race therefore
reaches only characters created or respecced afterwards, because only a respec reruns `do_start()`.

Commit `0831ef644` (2026-09-12) turned several race checks into feat checks and, to keep existing
races unchanged, granted `FEAT_STABILITY` to Crystal Dwarf and `FEAT_BODYSLAM` to Half-Troll. Older
characters never received those grants: their Crystal Dwarves lost knockdown resistance and their
Half-Trolls the bodyslam skill. In the local development player files, all 143 Half-Trolls lack
Bodyslam and 31 of 33 Crystal Dwarves lack Stability. `racefix` cannot restore them, because it
reads the deprecated `level_feats` table, which has neither row.

## Fix

When a character enters the game, grant each single-rank racial feat that its real race assigns at
or below its level and that it does not hold, through the same code the level-up path uses.

1. Shared grant. Move the per-assignment body of `process_race_level_feats()` (special handling or
   the gained or improved message, then the rank increment) into one static helper in
   `src/character/class.c`, so a repaired feat behaves exactly like a level-up grant.
2. Real race. Done by the Duris races work: `process_race_level_feats()` reads
   `GET_REAL_RACE()`. The repair must read it too.
3. Repair. Add `grant_missing_race_feats()` beside it, declared in `src/character/class.h`. It skips
   NPCs and invalid races, then for each assignment in the real race's `featassign_list` whose
   `level_received` is at most `GET_LEVEL(ch)`, whose feat has `can_stack` false in `feat_list[]`,
   and whose `HAS_REAL_FEAT()` is zero, logs the character, race, and feat and grants it through the
   shared helper. `HAS_REAL_FEAT()` ignores gear, so an item that grants the feat does not hide the
   gap. It returns the number granted.
4. Hook. Call it in `enter_player_game()` (`src/core/interpreter.c`) right after the class-spell
   top-up ("make sure we assign any new spells"). The menu login, copyover recovery, and the
   description-completion entry all pass through there. A new character enters at level 0 and gets
   nothing until `do_start()` runs. The next save persists a repaired feat; a login before that save
   repeats the same grant.

Multi-rank feats stay out: a saved rank does not show which ranks came from the race and which from
class or study, so a top-up could over- or under-grant. Races grant only two, Armor Skin and Fast
Movement; 277 of the 294 current racial grants are single-rank. No migration script, `racefix`
change, or new command is needed.

## Tests

In `unittests/CuTest/test_racial_innate_feats.c`, beside `TestBucketBRacesStillHoldTheirWiredFeats`,
which checks only the race lists:

- a saved Crystal Dwarf without Stability and a Half-Troll without Bodyslam each gain it once, and a
  second call grants nothing;
- a character that already holds the feat keeps its rank;
- an assignment above the character's level is skipped;
- a stackable racial feat (Armor Skin on a Half-Ogre) is not topped up;
- the repair reads the real race while a disguise race is set;
- an NPC is untouched.

No CuTest drives `enter_player_game()`, so the hook gets a live check on port 4100
(`MUD_PORT=4100 ./scripts/autorun/autorun.sh`): remove a racial feat with
`featset <name> '<feat>' -1` (for example Bodyslam on a Half-Troll), reconnect, see the
"[race] You have gained" line, use the feat, and find it in the player file after the next save;
then repeat through a copyover. Finish with `make -j$(nproc)`, `make test`, and `make install`.

## Docs and rollout

- `docs/systems/GAME_MECHANICS_SYSTEMS.md`, racial innate section: saved characters receive missing
  single-rank racial feats when they enter the game.
- `docs/guides/ADDING_NEW_RACE_GUIDE.md`, racial feats section: a single-rank feat added to an
  existing race reaches saved characters at their next login; a multi-rank grant still reaches only
  new or respecced characters.
- Production picks the repair up with the next restart, which needs its own approval; the log lines
  count the repaired characters. A rollback leaves granted feats in place, and they are what those
  characters should hold.

Then delete this plan and its index entry.

The Duris races work kept race checks beside `FEAT_PSIONIC_RESISTANCE` (Trelux) and
`FEAT_COLD_IMMUNITY` (Lich) in `compute_damtype_reduction()` because saved characters lack those
feats. Once this repair grants them at login, those two race checks can go (Lich already assigns
Cold Immunity at level 1).
