# Inquisitor Domain System - Investigation Findings and Resolution

> Investigation record and implementation summary. Source and tests remain
> authoritative for current behavior.

Date of investigation: 2026-08-13
Reported symptom: an inquisitor can select a single domain in the `study` menu,
but none of the domain's granted powers are usable. Reproduced by the reporter
with a freshly created character selecting the Healing domain, then finding
`healingtouch` unavailable.

Resolution date: 2026-08-13
Status: fixed in the development worktree and covered by production-linked tests.

The investigation sections through "Reproduction" describe the pre-fix source
state. "Implemented fix" records the current behavior.

## Summary

The original report was confirmed by code inspection. Domain *selection* and
domain *spells* were wired for inquisitors; domain *granted powers* were not.
Three independent layers blocked an inquisitor from using a domain power, and
fixing only the first would still have left the powers scaled to zero.

| Original layer | Pre-fix effect for an inquisitor | Original location |
|----------------|----------------------------------|-------------------|
| 1. Feat grant | Domain feats were never set on the character | `src/magic/domains_schools.c:227` |
| 2. Feat wipe | Any domain feat present was cleared on every study finalize | `src/magic/domains_schools.c:212` |
| 3. Power scaling | Every power's magnitude/duration used cleric level | `src/magic/domain_powers.c`, `src/combat/fight.c`, `src/magic/magic.c` |

## What already works for inquisitors

These paths explicitly test for `CLASS_INQUISITOR` and behave correctly:

- Domain selection is offered. `CAN_SET_DOMAIN` (`src/utils.h:1636`) is true at
  cleric level 1 **or** inquisitor level 1, so study option 8 is live.
- Exactly one domain is allowed. `src/character/study.c:4766` rejects a 2nd
  domain for anything that is not a cleric ("Only clerics can choose a 2nd
  domain."), which matches the intended inquisitor rule.
- Domain bonus spells are granted. `assign_domain_spells()`
  (`src/magic/domains_schools.c:310`) accepts cleric or inquisitor, and
  `init_class()` (`src/character/class.c:2839`) calls it for both classes.
- Domain spell circles resolve for inquisitors:
  `src/magic/spell_parser.c:174`, `src/magic/spell_parser.c:3492`,
  `src/magic/spell_lists.c:283`, `src/act.other.c:7970`.
- Favored weapon proficiency from the domain is inquisitor-aware:
  `src/combat/assign_wpn_armor.c:63`.
- `has_domain_power()` (`src/magic/domains_schools.c:247`) already accepts
  inquisitors - it returns TRUE for a healing-domain inquisitor asking about
  `DOMAIN_POWER_HEALING_TOUCH`.

So the reporter's character genuinely has the domain, its spells, and its
favored weapon. Only the granted powers are missing.

## Root cause 1 - domain feats are never granted to inquisitors

Domain powers are implemented as feats of type `FEAT_TYPE_DOMAIN_ABILITY`
(registered in `src/character/feats.c`, e.g. `FEAT_HEALING_TOUCH` at
`src/character/feats.c:3591`). The only routine that converts a selected domain
into those feats is `add_domain_feats()`:

```c
/* src/magic/domains_schools.c:227 */
void add_domain_feats(struct char_data *ch)
{
  if (!CLASS_LEVEL(ch, CLASS_CLERIC))
    return;
  ...
}
```

The early return fires for a pure inquisitor, so no domain feat is ever set.
Note the contrast with the two neighbouring functions in the same file, both of
which were updated for inquisitors while this one was not:

- `has_domain_power()` - `!CLASS_CLERIC && !CLASS_INQUISITOR` (line 249)
- `assign_domain_spells()` - `!CLASS_CLERIC && !CLASS_INQUISITOR` (line 312)
- `add_domain_feats()` - `!CLASS_CLERIC` only (line 229)

This is the direct cause of the reported symptom. `do_healingtouch`
(`src/magic/domain_powers.c:272`) performs two checks in sequence:

```c
  if (!has_domain_power(ch, DOMAIN_POWER_HEALING_TOUCH))  /* passes  */
  if (!HAS_FEAT(ch, FEAT_HEALING_TOUCH))                  /* fails   */
    send_to_char(ch, "You do not have that feat!\r\n");
```

So the expected player-visible message on the reporter's character is
"You do not have that feat!" - the domain is recognised, the feat is absent.
Every other domain power command in `src/magic/domain_powers.c` has the same
two-check shape (lines 32, 120, 197, 278, 356, 393, 430, 536, 699, 739, 769,
856, 941, 1026, 1109), so the failure is uniform across all domains, not
specific to Healing.

The two callers of `add_domain_feats()` are:

- `finalize_study()` - `src/character/study.c:519-520`
- `levelup_cleric()` premade build - `src/character/premadebuilds.c:1527-1528`
  (cleric-only by construction; there is no inquisitor premade equivalent that
  sets a domain)

Neither can help an inquisitor while the guard stands.

## Root cause 2 - domain feats are unconditionally cleared for every class

```c
/* src/magic/domains_schools.c:212 */
void clear_domain_feats(struct char_data *ch)
{
  for (i = 1; i < NUM_FEATS; i++)
    if (feat_list[i].feat_type == FEAT_TYPE_DOMAIN_ABILITY)
      SET_FEAT(ch, i, 0);
}
```

This has no class guard, and `finalize_study()` calls it immediately before
`add_domain_feats()`. The pairing is asymmetric: the clear applies to everyone,
the re-grant applies to clerics only. Consequently, even if an inquisitor
obtained a domain feat by some other route (staff `set`, a future feat grant,
a multiclass cleric level that is later respecced away), the next trip through
study would strip it and not restore it. Any fix that only relaxes the guard in
`add_domain_feats()` should be checked against this ordering.

## Root cause 3 - power effects are scaled by cleric level

Even with the feat granted, every domain power computes its effect from
`CLASS_LEVEL(ch, CLASS_CLERIC)`, which is 0 for a pure inquisitor. This is a
second, independent defect: the abilities would appear on the character but do
degenerate or nothing.

Active powers in `src/magic/domain_powers.c`:

| Line | Power | Cleric-level dependency |
|------|-------|-------------------------|
| 95 | evil touch | `SPELL_EYEBITE` cast at cleric level |
| 183 | blessed touch | `SPELL_AID` cast at cleric level |
| 254-255 | good touch | `remove poison` / `remove disease` at cleric level |
| 343 | healing touch | `20 + 1d4 + CLASS_LEVEL(CLERIC)/2` |
| 380 | eye of knowledge | `SPELL_WIZARD_EYE` at cleric level |
| 417 | copycat | `SPELL_MIRROR_IMAGE` at cleric level |
| 460 | mass invis | `SPELL_INVISIBILITY_SPHERE` at cleric level |
| 490-497 | aura of protection | `MAX(1, CLASS_LEVEL(CLERIC)/6)` on four modifiers |
| 603 | (destructive aura bonus) | `CLASS_LEVEL(CLERIC)/4` |
| 659 | destructive smite | `MAX(1, CLASS_LEVEL(CLERIC)/2)` |
| 834, 920, 1005, 1090 | lightning arc / acid dart / fire bolt / icicle | `10 + 1d6 + CLASS_LEVEL(CLERIC)/2` |
| 1176 | curse touch | `SPELL_CURSE` at cleric level |

Because these use `MAX(1, ...)` or a flat base, most would still "work" for an
inquisitor but at minimum magnitude - healing touch would heal `20 + 1d4`
forever, aura of protection would give +1, and so on.

Passive powers fail harder, because they are gated behind a cleric-level
threshold rather than scaled:

- `src/combat/fight.c:3914-3919` fire resistance - requires cleric level 6/12/20
- `src/combat/fight.c:3966-3970` cold resistance - same
- `src/combat/fight.c:4041-4045` acid resistance - same
- `src/combat/fight.c:4093-4097` electric resistance - same
- `src/combat/fight.c:3865-3867` `FEAT_RESISTANCE` - `CLASS_LEVEL(CLERIC)/6`, i.e. 0
- `src/magic/magic.c:461-462` `FEAT_SAVES` - `CLASS_LEVEL(CLERIC)/6`, i.e. 0

For a pure inquisitor these grant exactly nothing at any level. Some powers are
class-agnostic and would work immediately once the feat exists:
`FEAT_LAWFUL_WEAPON` / `FEAT_CHAOTIC_WEAPON` (`src/combat/fight.c:9144`,
`9162`), `FEAT_WEAPON_EXPERT` (`src/combat/assign_wpn_armor.c:47-51`),
`FEAT_KNOWLEDGE` (`src/act.other.c:5591`), `FEAT_DECEPTION`
(`src/character/class.c:1742`), `FEAT_ETH_SHIFT` (`src/act.other.c:963`),
`FEAT_EMPOWERED_HEALING` (`src/magic/magic.c:12795`).

Also relevant: daily uses are already correct for inquisitors.
`daily_uses_remaining()` (`src/utils.c:5222-5234`) bases domain-power uses on
`GET_WIS_BONUS(ch)`, not on cleric level, and inquisitors are wisdom-based.

Note also the dead code at `src/magic/domain_powers.c:781-786`, `868-873`,
`953-958`, `1038-1043`, `1121-1126`: an explicit
`if (!CLASS_LEVEL(ch, CLASS_CLERIC)) "You do not have any clerical powers!"`
block, commented out in each of the five direct-damage powers. Someone
previously started removing cleric-only gating from this file and stopped
before reaching the feat-grant path.

## Secondary / cosmetic issues found

- The study main menu labels option 8 "Cleric Domain Selection"
  (`src/character/study.c:2632`), which reads as a mistake to an inquisitor
  player who is nonetheless allowed to use it.
- The `FEAT_HEALING_TOUCH` help text (`src/character/feats.c:3591`) and the
  other domain feat descriptions all say "cleric level" / "your cleric level".
  If inquisitors are meant to have these, the text needs a class-neutral
  wording.
- `CAN_SET_DOMAIN` is `CLASS_LEVEL(...) == 1`, so the domain choice is
  permanently locked after the first level. If an inquisitor created a character
  before a fix lands, they will have a domain recorded but no way to re-trigger
  the study path that would grant the feats; a fix should consider a one-time
  re-grant on login or level-up rather than relying on `finalize_study()` alone.
- `clear_domain_feats()` uses `SET_FEAT(ch, i, 0)`, while `add_domain_feats()`
  guards on `!HAS_REAL_FEAT(ch, featnum)`. Worth confirming these two agree on
  the real-vs-effective feat storage before any change.

## Reproduction

1. Create an inquisitor, take it to level 1.
2. `study` -> option 8 -> 1 -> choose Healing. The menu accepts it and prints
   the granted powers (`healing touch`, `empowered healing`), because
   `print_domain_info()` reads the static `domain_list` table and never consults
   the character.
3. Quit study to finalize. `finalize_study()` calls `assign_domain_spells()`
   (works), then `clear_domain_feats()` + `add_domain_feats()` (the latter
   returns immediately).
4. `feats` shows no healing touch. `healingtouch` reports "You do not have that
   feat!".

Comparison control: the same steps on a cleric produce a working `healingtouch`.

## Implemented fix

The completed fix covers every layer identified above:

1. `add_domain_feats()` accepts clerics and inquisitors, preserving the existing
   clear-then-reconcile behavior when study finalizes.
2. `get_domain_power_level()` provides one scaling rule for active powers,
   elemental resistance, general resistance, and saving throw bonuses. It uses
   full inquisitor level or cleric level, whichever is higher, without stacking
   the two classes.
3. `init_class()` re-grants missing domain feats on login, migrating existing
   inquisitors without changing their saved domain choice.
4. The study menu, feat descriptions, and player help use class-neutral domain
   wording and document the scaling rule.
5. Production-linked CuTest coverage verifies inquisitor feat reconciliation,
   login re-grant behavior, scaling, and passive resistance/save effects.
