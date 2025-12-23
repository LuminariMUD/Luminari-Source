# Blackguard (Antipaladin) Perk Trees

Design goal: three thematic trees, each with 4 tiers; Tiers 1–3 have 4 perks each, Tier 4 has 2 capstones. Emphasize fear/control, profane offense, and unholy resilience. Prefer profane typing, alignment DR interactions, and daily/encounter flags consistent with existing perk cadence. Avoid introducing new resources.

## Tree A: Tyranny & Fear (Debuff/Control)

- **Tier 1 (4 perks)**
  - Dread Presence: Aura penalties to Will vs fear; small demoralize bonus.
  - Intimidating Smite: Smite applies shaken on hit; scaling DC with level/CHA.
  - Cruel Edge: Bonus damage vs frightened/panicked; minor temp hp on kill.
  - Command the Weak: Demoralize as move/swift (1/enc); modest duration bump.
- **Tier 2 (4 perks)**
  - Aura of Cowardice: Suppress fear immunity; fear saves in aura take a penalty.
  - Terror Tactics: Demoralize can splash to adjacent foes on high roll.
  - Black Seraph Step: On forced position change (self) such as knockdown or trip, auto-demoralize.
  - Nightmarish Visage: Intimidate AoE instead of one target.
- **Tier 3 (4 perks)**
  - Paralyzing Dread: Shaken -> frightened on fail; frightened -> cower on big fail.
  - Despair Harvest: Temp hp or dark focus when foes fail fear saves (per-round cap).
  - Shackles of Awe: Fear effects also reduce speed and attack bonus.
  - Profane Dominion: Fearful foes take extra profane damage each round.
- **Tier 4 (2 capstones)**
  - Sovereign of Terror: Fear immunity becomes resistance; aura escalates fear one step each round (cap at cower).
  - Midnight Edict: 1/day decree: all enemies in aura save vs mass fright/panic; failures also stagger.

## Tree B: Profane Might (Damage/Offense)

- **Tier 1 (4 perks)**
  - Vile Strike: Bonus profane damage; higher vs good/holy.
  - Cruel Momentum: On kill/crit, gain stacking damage buff for short duration.
  - Dark Channel: Smite adds a damage die and bypasses DR/good.
  - Brutal Oath: Choose favored foe type; bonus hit/damage vs that type.
- **Tier 2 (4 perks)**
  - Ravaging Smite: Smite adds bleed or ability-damage rider (pick on use).
  - Profane Weapon Bond: Short buff grants magic + alignment + minor on-hit rider.
  - Relentless Assault: Extra attack on charge or after dropping a foe (1/round gate).
  - Sanguine Barrier: Portion of damage dealt grants temp hp (per-round cap).
- **Tier 3 (4 perks)**
  - Doom Cleave: On kill, free cleave-like strike (1/round).
  - Soul Rend: Extra dice vs good outsiders/undead; may suppress resist.
  - Blackened Precision: Threat range or crit damage bump while bonded.
  - Unholy Blitz: Brief haste-like burst after smite hit (limited uses).
- **Tier 4 (2 capstones)**
  - Avatar of Profanity: Long-cd self-buff: big profane damage, DR/—, resist, and auto-bypass alignment DR.
  - Cataclysmic Smite: 1/day smite detonates in dark burst: AoE damage + save vs sickened/staggered.

## Tree C: Unholy Resilience (Defense/Utility)

- **Tier 1 (4 perks)**
  - Profane Fortitude: Bonus saves vs holy effects/positive energy.
  - Dark Aegis: Small DR/— while not flat-footed; light scaling with level.
  - Graveborn Vigor: Temp hp trigger when under threshold (cooldown).
  - Sinister Recovery: Limited self-heal that harms adjacent good foes for half.
- **Tier 2 (4 perks)**
  - Aura of Desecration: Allies’ negative energy boosted; enemies’ healing impeded.
  - Fell Ward: Reactive save bonus after being targeted by a divine spell.
  - Defiant Hide: Bonus AC/DR vs smite/good-aligned weapons.
  - Shade Step: Short shadow step as move/swift (limited uses).
- **Tier 3 (4 perks)**
  - Soul Carapace: Convert portion of incoming damage to temp hp (per-round cap).
  - Death Denied: 1/rest, prevent drop to 0; stay at 1 with brief DR.
  - Blackguard’s Reprisal: After you save vs a spell, next attack gains bonus damage/DC rider.
  - Warding Malice: Enemies in aura take penalty to caster level checks vs your saves/wards.
- **Tier 4 (2 capstones)**
  - Umbral Immortality: 1/day: incorporeal-like defenses briefly; heal on kills during it.
  - Pact of the Grave: Auto-revive 1/day at low hp with burst of negative energy damage to enemies/heal to allies.

## Implementation guardrails
- Use profane typing; align DR bypass with existing alignment flags.
- Fear stacking: cap at cower; only specified perks erode immunity.
- Swift-action economy: enforce 1/round; avoid free stacking with other conversions.
- Temp hp: cap by level or CHA to prevent loops.
- Aura radii: match existing paladin/antipaladin auras.
- Smiting hooks: reuse smite framework; add riders via perk checks.
- Use daily/per-encounter mud events; no new resources.
