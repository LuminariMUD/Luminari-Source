# Racial Casting Speed Plan

Status: plan, written 2026-09-24; nothing here is implemented.

Change `FEAT_FAST_CASTING` (1316) and `FEAT_SLOW_CASTING` (1317) from one casting tick per rank to
10 percent of the spell's casting time per rank, so a race's ranks reproduce its Duris spellcast
multiplier. The [Duris race feat proposal](DURIS_RACE_FEAT_PROPOSAL.md#notes) assigns ranks on
this rule. Both feats came from #182; see
[DURIS_RACIAL_IMPORTS.md](DURIS_RACIAL_IMPORTS.md#new-innate-feats).

## Why the current rule does not convert

- Duris multiplies every spell's cast time, counted in beats, by `spellcast.pulse.racial.<Race>`
  (`src/net/sparser.c:942` in Duris). The values used here were read at Duris `e1357a30a` and are
  unchanged since `a71fdfee7`.
- LuminariMUD counts casting in whole ticks of one second (`casting_activity_step()` reschedules
  every 10 pulses). The current rule, `src/magic/spell_parser.c:2846`, adds one tick per slow rank
  and removes one per fast rank, so its effect depends on the spell's length:
  - `spellcasting_time_mode` 0 (standard action, the compiled default): every non-ritual cast is
    one tick, so one rank doubles the cast or makes it instant.
  - `spellcasting_time_mode` 1 (per-spell seconds, set in this dev server's local
    `lib/etc/config`): the 546 registered spells take 0 to 16 ticks, mostly 2 to 9. One rank is
    1.1x on a 10-tick spell and 2x on a 1-tick spell, and slow casting delays spells that should
    be instant.
- Production's mode is not recorded in the repository. The new rule is correct in both modes.

## The rule

`rate = 100 + 10 x (slow ranks - fast ranks)`, floored at 0. The cast takes `base x rate / 100`
ticks: the whole ticks are kept, and the fractional remainder becomes one more tick with that
percent chance. No roll is made when the remainder is zero.

| Ranks (multiplier) | Mode 0, one-tick cast | Mode 1, five-tick spell |
| -- | -- | -- |
| 3 fast (0.7) | Instant 30 percent, else 1 tick | 3 ticks, 50 percent chance of 4 |
| 1 slow (1.1) | 1 tick, 10 percent chance of 2 | 5 ticks, 50 percent chance of 6 |
| 5 slow (1.5) | 1 tick, 50 percent chance of 2 | 7 ticks, 50 percent chance of 8 |
| 9 slow (1.9) | 2 ticks 90 percent, else 1 | 9 ticks, 50 percent chance of 10 |
| 10 fast (0.0) | Instant | Instant |

Kept from the current rule: the adjustment runs at the same point in `cast_spell()`, before the
sorcerer metamagic surcharge, quicken, and the other instant-cast overrides, so a quickened cast
stays instant; the two feats net rank for rank; a zero-time spell stays instant; the NPC two-tick
minimum still applies afterwards. A fast cast that reaches zero ticks is not a quickened cast: in
mode 0 it still costs a standard and a move action (`src/magic/spell_parser.c:3068`).

No race grants either feat yet, so no existing character changes.

## Steps

1. Code. Replace the adjustment at `src/magic/spell_parser.c:2846` with one function,
   `racial_casting_time(ch, casting_time)`, in the same file and declared in
   `src/magic/spells.h`, that applies the rule with `rand_number(1, 100) <= remainder`.
   `cast_spell()` is its only caller; the declaration lets the tests reach it.
2. Feat text. Rewrite both `feato()` rows in `src/character/feats.c` (short and long text) for
   10 percent per rank.
3. Help. Rewrite `FAST-CASTING` and `SLOW-CASTING` in `lib/text/help/help.hlp` and
   `sql/components/help_other_racial_innate_entries.sql`, then update the two rows in the dev help
   database.
4. Tests, in `unittests/CuTest/test_racial_innate_feats.c`:
   - Replace `Test_racial_casting_feats_shift_a_timed_cast_by_one_tick_per_rank` with whole-tick
     cases through `cast_spell()` on a 10-tick spell: none 10, 1 slow 11, 3 fast 7, 1 fast and
     1 slow 10, 10 fast 0 (completes inside `cast_spell()`), 12 fast 0.
   - Keep `Test_racial_casting_feats_apply_in_standard_action_mode` with whole-tick cases: none
     1, 10 fast 0.
   - Add one test of `racial_casting_time()`: over 2000 one-tick casts at 3 fast ranks, every
     result is 0 or 1 and the count of 0 falls within 600 +/- 110 (about five standard
     deviations, so it holds for any recorded seed).
5. Docs, once the code lands:
   - `docs/systems/GAME_MECHANICS_SYSTEMS.md`, the racial casting speed paragraph.
   - `docs/guides/PLAYER_RACES_REFERENCE.md`, the Fast Casting and Slow Casting rows of the race
     point tables, re-priced per 10 percent rank. Start from the retired
     [race conversion study](https://github.com/LuminariMUD/Luminari-Source/blob/dba4ca2de4afdbe47fc0d1f6831a1a1f75db1e7e/docs/ongoing-projects/DURIS_RACE_CONVERSION.md)
     rates: 0.5 RP per fast rank (up to 10 ranks) and -0.25 RP per slow rank (refund counted for
     up to 5 ranks).
   - Delete this plan and its links from [DURIS_RACIAL_IMPORTS.md](DURIS_RACIAL_IMPORTS.md) and
     [DURIS_RACE_FEAT_PROPOSAL.md](DURIS_RACE_FEAT_PROPOSAL.md); the proposal's casting note then
     points to `GAME_MECHANICS_SYSTEMS.md`.
6. Verify. `CUTEST_FILTER=racial_casting ./cutest` while iterating, then `make -j$(nproc) test`
   and `make install`. On the dev MUD (port 4100), `help fast-casting`, `help slow-casting`, and
   `feat info fast casting` show the new text.

## Left out

- A setting for the rank size: every non-creation Duris value is a multiple of 0.1, and the
  creation races land within 0.045 of their Duris multiplier at 10 percent ranks.
- Separate rules per casting mode: one rule is correct in both.
- New feats or IDs, and any save or schema migration: the two feats keep their IDs and no race
  holds them.
- Combat pulse (Duris `damage.pulse.racial.<Race>`), which #164 declined; see
  [Not imported](DURIS_RACIAL_IMPORTS.md#not-imported).
