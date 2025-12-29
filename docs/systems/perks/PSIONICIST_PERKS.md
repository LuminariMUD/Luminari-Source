# Psionicist Perk Trees

Concrete mechanics aligned to this codebase (power points, augments, schools, DCs). Three trees, four tiers; tiers 1–3: four perks each; tier 4: two capstones.

## Tree A: Telepathic Control (Telepathy / Saves / Debuffs)

### Tier 1
- **Mind Spike I** — +1 DC to Telepathy powers (e.g., mind thrust, demoralize, slumber).
- **Suggestion Primer** — Telepathy powers that impose daze/sleep/fear gain +1 round duration on failed save (non-boss only).
- **Psionic Disruptor I** — +1 to caster level for Telepathy powers when rolling to penetrate power resistance.
- **Focus Channeling** — When you manifest a Telepathy power, reclaim 1 PSP if it hits at least one target (1/round).

### Tier 2
- **Mind Spike II** — Total +2 Telepathy DCs; Telepathy damage powers add +1 die if augmented by ≥2 PSP.
- **Overwhelm** — First Telepathy power you manifest each encounter forces targets to save twice and take the worse (once per combat).
- **Psionic Disruptor II** — Total +2 manifester level vs power resistance for Telepathy.
- **Linked Menace** — When you land a Telepathy debuff on a target, they take -2 penalty tp ac for 2 rounds

### Tier 3
- **Dominion** — +3 Telepathy DCs total; charm/dominate effects gain +2 rounds duration on failed save (non-boss only).
- **Psychic Sundering** — Telepathy damage powers treat targets as vulnerable, and they take 10% extra damage from all sources for 3 rounds.
- **Mental Backlash** — When a target saves vs your Telepathy, it still takes level-based psychic chip damage (5 + 1/2 level, no save).
- **Piercing Will** — Ignore 5 points of power resistance with Telepathy powers (stacks with Disruptor bonuses).

### Tier 4 (Capstones)
- **Absolute Geas** — When you use a telepathy power against a target, there's a 10% chance that it will apply the following debuffs to the target for 3 rounds: shaken, fatigued, deafened, will save to negate
- **Hive Commander** — Any hostile telepathy power you use on a target, if successful, gives you a +3 dc bonus to further powers used against it, and allies get +2 to attack rolls against it.

## Tree B: Psychokinetic Arsenal (Energy / Forced Movement / Defense)

### Tier 1
- **Kinetic Edge I** — +1 damage die on Psychokinesis blasts (energy ray, crystal shard, energy push, concussion blast) if augmented by ≥1 PSP.
- **Force Screen Adept** — Inertial armor / force screen grant +1 AC and +10% duration.
- **Vector Shove** — Energy push/telekinetic shoves get +2 to the movement check; on success deal +1 die force.
- **Energy Specialization** — Set an energy type; your energy powers of that type gain +1 DC.

### Tier 2
- **Kinetic Edge II** — Total +2 dice on Psychokinesis blasts when augmented by ≥3 PSP; energy burst/concussion blast splash +1 die.
- **Deflective Screen** — While force screen or inertial armor is active, gain +2 AC vs ranged and +2 Reflex; first hit each round is reduced by 5 damage
- **Accelerated Manifestation** — Once per combat, reduce PSP cost of a Psychokinesis power by 2 (min 1) and make it a faster action.
- **Energy Retort** — When struck in melee while you have an active Psychokinesis affect (force screen, energy retort, inertial armor), return level-based energy damage (scales with augment if present).

### Tier 3
- **Kinetic Edge III** — Total +3 dice on Psychokinesis blasts; energy ray/energy push gain +2 DC.
- **Gravity Well** — once per combat, aoe effect, halves speed, prevents fleeing (Reflex negates each round); lasts 3 rounds.
- **Force Aegis** — +3 AC vs ranged/spells while force screen/inertial armor; gain temp HP = manifester level on cast.
- **Kinetic Crush** — Forced-movement powers add prone on failed Reflex; if target collides, take extra force = manifester level.

### Tier 4 (Capstones)
- **Singular Impact** — 1/day Psychokinesis strike: heavy force damage, auto-bull rush, and stun 1 round (Fort partial: half damage, no stun).
- **Perfect Deflection** — 1/day reaction: negate one ranged/spell/psionic attack against you and reflect it using your casting stat vs the original attacker.

## Tree C: Metacreative Genius (Creation / Buffs / Summons)

### Tier 1
- **Ectoplasmic Artisan I** — Metacreativity powers cost 1 less PSP (min 1) once per encounter; +10% duration on metacreative buffs (concealing amorpha, inertial armor, wall of ectoplasm).
- **Shard Volley** — Crystal shard gains +1 projectile (additional attack roll) when augmented by ≥2 PSP.
- **Hardened Constructs I** — Summons/creations gain temp HP = manifester level and +1 AC.
- **Fabricate Focus** — Manual/creation powers (wall of ectoplasm, shambler, planar travel constructs) manifest 10% faster.

### Tier 2
- **Ectoplasmic Artisan II** — Total –2 PSP (min 1) once per encounter; +20% duration on metacreative buffs.
- **Shardstorm** — Crystal shard/swarm of crystals: convert to cone/line option; on hit inflict 1-round bleed (or –2 attack) (save negates rider).
- **Hardened Constructs II** — Summons/creations gain +2 AC and DR 2/—; attacks count as magic for DR.
- **Rapid Manifester** — Once per encounter reduce action time of a metacreative power by one step.

### Tier 3
- **Ectoplasmic Artisan III** — Total –3 PSP (min 1) once per encounter; +30% duration on metacreative buffs and walls.
- **Empowered Creation** — Metacreativity powers that deal damage (shrapnel burst, razor storm) add +2 dice if augmented by ≥4 PSP.
- **Construct Commander** — Summons gain +1 attack and +10% movement; shambler gains taunt pulse (1/round, small radius).
- **Self-Forged** — When you manifest a metacreative power, gain temp HP = 1/2 manifester level (stacks up to manifester level); converting temp HP to a shield reduces next hit by that amount (1/short rest).

### Tier 4 (Capstones)
- **Astral Juggernaut** — 1/day summon a Large construct with reach, taunt, and force slam that scales on manifester level; lasts several rounds or until destroyed.
- **Perfect Fabricator** — 1/day metacreative power is free (0 PSP), manifests as a swift action, and its conjured gear/construct counts as masterwork/magical for 1 hour.

---
Tuning guidance:
- Map “faster action” to your system’s casting/manifest time reduction rules.
- “Dice” means the base damage dice of that power before stat/augment bonuses; adjust to your dice scheme.
- “Once per encounter”/daily can map to your cooldown system if different.
- Ensure DC bonuses apply only to psionic power saves; PR penetration bonuses stack additively with manifester-level checks in code.
