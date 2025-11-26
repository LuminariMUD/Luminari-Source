# Berserker (Barbarian) Perk System Design

## Overview
The Berserker perk system is divided into three thematic trees, each with four tiers of increasing power and cost. Each tree focuses on a different aspect of barbarian combat and survival.

---

## Tree 1: RAVAGER (Offensive Fury)
*Focus: Raw damage output, critical hits, and devastating attacks*

### Tier 1 Perks (1 point each)

#### Power Attack Mastery I
- **Description**: Increase power attack damage bonus by +1 per rank
- **Max Ranks**: 5
- **Prerequisites**: None
- **Effect**: When using power attack, gain additional +1 damage per rank (+5 total at max), or +2 per rank if wielding a two-handed melee weapon.

#### Rage Damage I
- **Description**: Deal additional damage while raging
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +2 damage per rank while raging (+6 total at max)

#### Improved Critical I
- **Description**: Increase critical threat range with melee weapons
- **Max Ranks**: 1
- **Prerequisites**: None
- **Effect**: +1 critical threat range (stacks with Improved Critical feat)

#### Cleaving Strikes
- **Description**: When fighting multiple opponents you have a 10% chance per round, per extra opponent, to make an attack on that opponent.
- **Max Ranks**: 1
- **Prerequisites**: Cleave feat
- **Effect**: When fighting multiple opponents you have a 10% chance per round, per extra opponent, to make an attack on that opponent.

---

### Tier 2 Perks (2 points each)

#### Power Attack Mastery II
- **Description**: Further increase power attack damage
- **Max Ranks**: 3
- **Prerequisites**: Power Attack Mastery I (5 ranks)
- **Effect**: Additional +1 damage per rank when using power attack (+3 total at max), +2 per rank if wielding a two-handed melee weapon.

#### Rage Damage II
- **Description**: Further increase rage damage bonus
- **Max Ranks**: 2
- **Prerequisites**: Rage Damage I (3 ranks)
- **Effect**: Additional +3 damage per rank while raging (+6 total at max)

#### Blood Frenzy
- **Description**: Gain extra speed when you deal critical hits
- **Max Ranks**: 1
- **Prerequisites**: Improved Critical I (2 ranks)
- **Effect**: Critical hits have a 20% chance to give you haste for 3 rounds.

#### Devastating Critical
- **Description**: Critical hits deal additional damage
- **Max Ranks**: 3
- **Prerequisites**: Improved Critical I (2 ranks)
- **Effect**: +1d6 bonus damage per rank on critical hits

---

### Tier 3 Perks (3 points each)

#### Power Attack Mastery III
- **Description**: Maximum power attack enhancement
- **Max Ranks**: 2
- **Prerequisites**: Power Attack Mastery II (3 ranks)
- **Effect**: Additional +1 damage per rank when using power attack (+3 total at max), +2 per rank for two handed weapons.

#### Overwhelming Force
- **Description**: Powerful attacks can stagger enemies
- **Max Ranks**: 1
- **Prerequisites**: Power Attack Mastery II (2 ranks)
- **Effect**: Power attacks have 15% chance to stagger opponents for 2 rounds.

#### Crimson Rage
- **Description**: Rage damage scales with missing health
- **Max Ranks**: 1
- **Prerequisites**: Rage Damage II (2 ranks)
- **Effect**: Gain +1 damage for every 10% HP missing while raging (max +9 damage at 10% HP)

#### Carnage
- **Description**: Critical hits have splash damage
- **Max Ranks**: 1
- **Prerequisites**: Devastating Critical (2 ranks)
- **Effect**: Critical hits deal 25% weapon damage to other opponents when fighting multiple enemies.

---

### Tier 4 Perks (4 points each)

#### Frenzied Berserker
- **Description**: Enter a state of ultimate fury
- **Max Ranks**: 1
- **Prerequisites**: Power Attack Mastery III (2 ranks)
- **Effect**: Once per rage, activate to gain +4 to hit, +6 damage, and +2 critical multiplier for 5 rounds, 5 minute cooldown.

#### Relentless Assault
- **Description**: Your attacks become unstoppable
- **Max Ranks**: 1
- **Prerequisites**: Blood Frenzy
- **Effect**: When under the effect of blood frenzy's haste effect, each power attack you make reduces the target's AC by one for 3 rounds to a amximum of +5.

#### Death from Above
- **Description**: Leap attack devastates enemies
- **Max Ranks**: 1
- **Prerequisites**: Overwhelming Force
- **Effect**: Gain "Leap Attack" ability - start a fight by leaping at your opponent and dealing x2 weapon damage on a hit.

---

## Tree 2: OCCULT SLAYER (Supernatural Resilience)
*Focus: Defense, damage reduction, and resistances*

### Tier 1 Perks (1 point each)

#### Thick Skin I
- **Description**: Increase natural armor bonus
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +1 natural armor bonus per rank (+3 total at max)

#### Damage Reduction I
- **Description**: Reduce incoming physical damage
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: Gain 1/- damage reduction per rank (3/- total at max)

#### Elemental Resistance I
- **Description**: Resist elemental damage
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +5% resistance to fire, cold, electric, and acid per rank (+15% total at max)

#### Hardy
- **Description**: Improved constitution and fortitude
- **Max Ranks**: 1
- **Prerequisites**: None
- **Effect**: Can use the hardy command to give +2 Constitution score, +1 Fortitude saves for 10 rounds, with no cooldown.

---

### Tier 2 Perks (2 points each)

#### Thick Skin II
- **Description**: Further increase natural armor
- **Max Ranks**: 3
- **Prerequisites**: Thick Skin I (3 ranks)
- **Effect**: Additional +1 natural armor per rank (+3 total at max)

#### Damage Reduction II
- **Description**: Further increase damage reduction
- **Max Ranks**: 2
- **Prerequisites**: Damage Reduction I (3 ranks)
- **Effect**: Additional 1/- damage reduction per rank (2/- total at max)

#### Elemental Resistance II
- **Description**: Increase elemental resistance
- **Max Ranks**: 2
- **Prerequisites**: Elemental Resistance I (3 ranks)
- **Effect**: Additional +5% resistance to fire, cold, electric, and acid per rank (+10% total at max)

#### Savage Defiance
- **Description**: Take less damage while raging
- **Max Ranks**: 3
- **Prerequisites**: Damage Reduction I (2 ranks)
- **Effect**: +2 DR per rank while raging (+6 DR total at max)

---

### Tier 3 Perks (3 points each)

#### Damage Reduction III
- **Description**: Maximum damage reduction
- **Max Ranks**: 2
- **Prerequisites**: Damage Reduction II (2 ranks)
- **Effect**: Additional 1/- damage reduction per rank (2/- total at max)

#### Deathless Frenzy
- **Description**: Rage beyond death
- **Max Ranks**: 1
- **Prerequisites**: Savage Defiance (2 ranks)
- **Effect**: When reduced to 0 HP while raging, 50% chance to be healed to 1/4 max hp with a 5 minute cooldown.

#### Spell Resistance
- **Description**: Resist hostile magic
- **Max Ranks**: 3
- **Prerequisites**: Elemental Resistance II (2 ranks)
- **Effect**: Gain spell resistance equal to 10 + (5 per rank)

#### Pain Tolerance
- **Description**: Shrug off debilitating effects
- **Max Ranks**: 1
- **Prerequisites**: Hardy
- **Effect**: +4 bonus to saves vs. poison, disease, and mind-affecting effects

---

### Tier 4 Perks (4 points each)

#### Unstoppable
- **Description**: Become nearly invulnerable
- **Max Ranks**: 1
- **Prerequisites**: Damage Reduction III (2 ranks)
- **Effect**: While raging, DR increases by 5/-

#### Indomitable Will
- **Description**: Mental fortitude breaks enchantments
- **Max Ranks**: 1
- **Prerequisites**: Pain Tolerance
- **Effect**: Automatically succeed on one save vs. mind-affecting per rage, +4 bonus to all other mental saves.

#### Raging Defender
- **Description**: Damage reduction becomes more effective
- **Max Ranks**: 1
- **Prerequisites**: Deathless Frenzy
- **Effect**: Your DR is doubled when hit by a critical hit or sneak attack.

---

## Tree 3: RAVAGER (Primal Warrior)
*Focus: Mobility, crowd control, and tactical combat*

### Tier 1 Perks (1 point each)

#### Fleet of Foot I
- **Description**: Increase movement speed
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +10% movement speed per rank while not wearing heavy armor (+30% total at max)

#### Intimidating Presence I
- **Description**: Improve intimidate checks
- **Max Ranks**: 5
- **Prerequisites**: None
- **Effect**: +2 intimidate per rank, enemies have -1 morale penalty per 2 ranks (+5 total)

#### Mighty Leap
- **Description**: Jump further and avoid falling damage
- **Max Ranks**: 1
- **Prerequisites**: None
- **Effect**: +10 to climbing checks, reduce falling damage by 50%

#### Thick Headed
- **Description**: Resist stunning and knockdown
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +2 bonus to saves vs. stun and knockdown per rank (+6 total at max)

---

### Tier 2 Perks (2 points each)

#### Fleet of Foot II
- **Description**: Further increase movement speed
- **Max Ranks**: 2
- **Prerequisites**: Fleet of Foot I (3 ranks)
- **Effect**: Additional +10% movement speed per rank while not wearing heavy armor (+20% total at max)

#### Intimidating Presence II
- **Description**: Terrify your enemies
- **Max Ranks**: 2
- **Prerequisites**: Intimidating Presence I (5 ranks)
- **Effect**: Additional +1 intimidate per rank, Enemies attacking you must make a discipline check against your intimidate skill or be shaken for 5 rounds. An enemy only needs to make this check once per 10 rounds.

#### Sprint
- **Description**: Burst of speed
- **Max Ranks**: 1
- **Prerequisites**: Fleet of Foot I (2 ranks)
- **Effect**: Gain "Sprint" ability - double movement speed for 5 rounds, 2 minute cooldown.

#### Crippling Blow
- **Description**: Attacks can slow enemies
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: 5% chance per rank to apply a slow effect on enemies for 3 rounds when you critically hit them.

---

### Tier 3 Perks (3 points each)

#### Reckless Abandon
- **Description**: Trade defense for overwhelming offense
- **Max Ranks**: 1
- **Prerequisites**: Fleet of Foot II (2 ranks)
- **Effect**: Activate to gain +4 to hit and +8 damage but -4 AC for 5 rounds, usable while raging. Cooldown of 5 minutes.

#### Blidning Rage
- **Description**: Your rage causes blindness in enemies
- **Max Ranks**: 1
- **Prerequisites**: Intimidating Presence II (2 ranks)
- **Effect**: When entering rage, all enemies currently fighting you must save vs. will or be blinded for 1d4 rounds.

#### Stunning Blow
- **Description**: Powerful strikes can stun
- **Max Ranks**: 1
- **Prerequisites**: Crippling Blow (2 ranks)
- **Effect**: When starting a rage, your next attack forces a Fortitude save or target is stunned for 2 rounds.

#### Uncanny Dodge Mastery
- **Description**: Enhanced defensive awareness
- **Max Ranks**: 1
- **Prerequisites**: Thick Headed (2 ranks)
- **Effect**: Gain +5 to perception skill and +3 to dodge ac.

---

### Tier 4 Perks (4 points each)

#### Savage Charge
- **Description**: Devastating charge attacks
- **Max Ranks**: 1
- **Prerequisites**: Reckless Abandon
- **Effect**: Charge attacks deal triple damage and knock down targets (no save), usable once per rage

#### War Cry
- **Description**: Rally allies and demoralize foes
- **Max Ranks**: 1
- **Prerequisites**: Terrifying Rage
- **Effect**: Gain "War Cry" ability - all allies within 30 feet gain +2 attack/damage, all enemies take -2 for 5 rounds, 5 minute cooldown

#### Earthshaker
- **Description**: Ground pound affects area
- **Max Marks**: 1
- **Prerequisites**: Stunning Blow
- **Effect**: Gain "Earthshaker" ability - slam ground dealing damage equal to your STR mod and knocking prone all enemies currently fighting you or a party member of yours, 30 second cooldown, uses and requires a swift action.

---

## Perk Point Costs Summary

### By Tier:
- **Tier 1**: 1 point per perk
- **Tier 2**: 2 points per perk  
- **Tier 3**: 3 points per perk
- **Tier 4**: 4 points per perk

### Maximum Investment Per Tree:
Assuming all ranks and all perks taken in a single tree:
- **Tier 1**: ~15-20 points
- **Tier 2**: ~10-16 points
- **Tier 3**: ~9-12 points
- **Tier 4**: ~8-12 points
- **Total per tree**: ~45-60 points for complete mastery

---

## Design Philosophy

### Ravager (Offensive Fury)
- Emphasizes dealing massive damage through critical hits, rage bonuses, and power attacks
- Encourages aggressive, high-risk playstyle
- Synergizes with two-handed weapons and strength builds

### Occult Slayer (Supernatural Resilience)  
- Focuses on survivability through damage reduction, resistances, and defensive abilities
- Allows barbarians to stand toe-to-toe with the toughest enemies
- Synergizes with tanking roles and front-line combat

### Ravager (Primal Warrior)
- Provides tactical flexibility through mobility, crowd control, and battlefield control
- Enables hit-and-run tactics and strategic positioning
- Synergizes with skirmisher builds and group tactics

---

## Synergies with Existing Systems

### Rage Feature
- Many perks specifically enhance or require rage to be active
- Rage becomes more versatile and powerful with perk investment
- Creates meaningful choices between offensive and defensive rage usage

### Feats
- Perks complement and enhance existing feats (Power Attack, Cleave, Improved Critical)
- Some perks require specific feats as prerequisites
- Encourages feat/perk synergy planning

### Equipment
- Mobility perks work best with light/medium armor
- Damage reduction perks stack with armor
- Some perks have armor restrictions for balance

---

## Implementation Notes

### Balance Considerations
- Tier 4 perks should feel game-changing but not overpowered
- Multiple paths to effectiveness (offense, defense, or mobility)
- No single "best" build - encourage diversity
- Synergies should reward focused builds but not make hybrid builds unviable

### Future Expansion
- Could add "capstone" perks at tier 5 requiring heavy tree investment
- Potential for rage-type variations (berserker rage, focused rage, etc.)
- Cross-tree synergies for truly hybrid builds
- Racial variant perks for different barbarian cultures

### Testing Priorities
1. Damage output scaling at each tier
2. Survivability improvements balance
3. Mobility and crowd control effectiveness
4. Perk point economy and progression rate
5. Multi-classing interactions

---

## Notes on DDO Inspiration

This system draws inspiration from DDO's barbarian trees:
- **Ravager** = Similar to DDO's Frenzied Berserker (offense)
- **Occult Slayer** = Similar to DDO's Occult Slayer (defense/resistance)
- **Primal Warrior** = Similar to DDO's Ravager (versatility/tactics)

However, adapted for Luminari's combat system with considerations for:
- Turn-based/tick-based combat vs. real-time
- Different action economy
- Existing feat and class feature systems
- MUD-appropriate cooldowns and resource management
