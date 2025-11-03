# Perks System Design Document

## Overview
A character progression system inspired by DDO's enhancement trees, providing players with additional customization between levels through a staged advancement system.

---

## Core Mechanics

### Stage-Based Progression
- **4 stages per level** (Stages 1, 2, 3, 4)
- **Stage 1-3**: Player gains 1 perk point per stage
- **Stage 4**: Player advances to the next level
- **Experience per stage**: 25% of total XP required for next level

### Perk Points
- Players earn **3 perk points per level** (one at each of stages 1, 2, 3)
- Perk points are spent to purchase perks from class-specific lists
- Points are specific to the class that earned them (see Class Tracking below)

### Class Tracking
- Each base class has its own perk list
- Each prestige class has its own perk list
- When gaining a level in a class, perk points earned are tagged to that class
- Players can only spend perk points from a class on that class's perk list
- Example: A Fighter 5/Wizard 3 has separate perk points for Fighter perks and Wizard perks

### Perk Stacking
- **Same perk from different classes = separate instances**
- Example: "Toughness I" (+5 HP) from Fighter and "Toughness I" (+5 HP) from Ranger stack to give +10 HP total
- This encourages multiclassing and diverse character builds
- Each class's version of a perk is tracked independently

---

## Base Class Perks

### Fighter Perks

#### Combat Prowess Tree
1. **Weapon Specialization I-III** (1/2/3 points)
   - +1/+2/+3 damage with all melee weapons

2. **Armor Mastery I-III** (1/2/3 points)
   - Reduce armor check penalty by 1/2/3

3. **Shield Expertise I-II** (2/3 points)
   - +1/+2 AC when using a shield

4. **Toughness I-V** (1 point each)
   - +5 HP per rank

5. **Physical Resistance I-III** (2/3/4 points)
   - +1/+2/+3 to Fortitude saves

6. **Combat Reflexes Enhancement** (3 points)
   - +2 to attacks of opportunity per round

7. **Intimidating Presence I-II** (1/2 points)
   - +2/+4 to Intimidate skill

8. **Second Wind I-II** (2/3 points)
   - Reduce action cooldown by 10%/20%

9. **Battle Awareness** (2 points)
   - +1 dodge AC

10. **Crushing Blow I-II** (2/3 points)
    - +5%/+10% chance to critical hit

#### Defender Tree
11. **Stalwart Defense I-III** (1/2/3 points)
    - +2/+4/+6 damage reduction while defensive fighting

12. **Defensive Stance I-II** (2/3 points)
    - +5%/+10% to defensive fighting AC bonus

13. **Rapid Recovery I-II** (2/3 points)
    - +1/+2 HP regeneration per round

14. **Shield Block** (3 points)
    - 5% chance to block all damage from one attack per round

15. **Heavy Armor Focus** (2 points)
    - Move at full speed in heavy armor

#### Weapon Master Tree
16. **Critical Mastery I-II** (2/3 points)
    - Increase critical multiplier by +1/+2

17. **Improved Cleave** (3 points)
    - Cleave affects +1 additional target

18. **Power Attack Enhancement I-II** (1/2 points)
    - Power attack penalty reduced by 1/2

19. **Weapon Focus Enhancement I-II** (1/2 points)
    - +1/+2 to hit with chosen weapon type (stackable per weapon)

20. **Precision Strike** (3 points)
    - +2d6 damage on critical hits

---

### Wizard Perks

#### Arcane Studies Tree
1. **Spell Focus Enhancement I-III** (1/2/3 points)
   - +1/+2/+3 DC to chosen spell school

2. **Arcane Augmentation I-V** (1 point each)
   - +5 spell points per rank

3. **Extended Spell Enhancement I-II** (2/3 points)
   - +10%/+20% spell duration

4. **Potent Magic I-III** (1/2/3 points)
   - +1/+2/+3 to spell penetration checks

5. **Mental Toughness I-III** (1/2/3 points)
   - +1/+2/+3 to Will saves

6. **Spell Power: Evocation I-III** (2/3/4 points)
   - +5/+10/+15 to evocation spell damage

7. **Efficient Metamagic I-II** (2/3 points)
   - Reduce metamagic spell point cost by 10%/20%

8. **Arcane Mastery** (3 points)
   - +1 caster level for spell effects

9. **Quick Casting I-II** (2/3 points)
   - -5%/-10% spell casting time

10. **Spell Critical I-II** (2/3 points)
    - +5%/+10% chance for spells to critical

#### Battle Mage Tree
11. **Combat Casting Enhancement I-II** (1/2 points)
    - +2/+4 to concentration checks

12. **Arcane Shield I-III** (2/3/4 points)
    - +1/+2/+3 AC bonus

13. **Spellsword Training** (3 points)
    - Cast in light armor without spell failure

14. **Eldritch Strike** (3 points)
    - Weapon attacks reduce enemy spell resistance by 10 for 6 seconds

15. **Magical Defenses I-II** (2/3 points)
    - +2/+4 to saves vs. spells

#### Elementalist Tree
16. **Fire Specialization I-III** (2/3/4 points)
    - +10/+20/+30% fire spell damage

17. **Cold Specialization I-III** (2/3/4 points)
    - +10/+20/+30% cold spell damage

18. **Electric Specialization I-III** (2/3/4 points)
    - +10/+20/+30% electric spell damage

19. **Energy Substitution** (3 points)
    - Can change energy type of spells

20. **Elemental Mastery** (4 points)
    - Spells ignore 25% elemental resistance

---

### Cleric Perks

#### Divine Healing Tree
1. **Healing Amplification I-V** (1 point each)
   - +10% healing spell effectiveness per rank

2. **Empowered Healing I-II** (2/3 points)
   - Heal an additional 1d6/2d6 HP with healing spells

3. **Radiant Servant I-II** (2/3 points)
   - Channel energy heals +1d6/+2d6

4. **Healing Touch** (3 points)
   - Reduce cooldown on healing spells by 20%

5. **Divine Vitality I-III** (1/2/3 points)
   - +1/+2/+3 to Heal skill

6. **Mass Cure Enhancement** (3 points)
   - Mass healing spells affect +2 targets

7. **Martyr's Blessing I-II** (2/3 points)
   - When you heal others, you heal 10%/20% of that amount

8. **Resilient Healing** (2 points)
   - Your healing spells cannot be interrupted

9. **Beacon of Hope I-II** (2/3 points)
   - Allies within 20' gain +1/+2 to saves

10. **Toughness I-V** (1 point each)
    - +5 HP per rank

#### Wrath of the Gods Tree
11. **Spell Power: Inflict I-III** (2/3/4 points)
    - +5/+10/+15 to inflict wound spell damage

12. **Holy Smite Enhancement I-II** (2/3 points)
    - +1d6/+2d6 damage to evil creatures

13. **Divine Might I-II** (2/3 points)
    - Turn undead deals +2d6/+4d6 damage

14. **Weapon of Faith I-II** (2/3 points)
    - +1/+2 to hit and damage with deity's favored weapon

15. **Spiritual Weapon Enhancement** (3 points)
    - Spiritual weapon lasts 50% longer and attacks +1 additional time

#### Divine Protection Tree
16. **Faith Shield I-III** (2/3/4 points)
    - +5/+10/+15 spell resistance

17. **Divine Grace I-II** (1/2 points)
    - +1/+2 to all saves

18. **Damage Reduction: Good I-II** (2/3 points)
    - 2/4 damage reduction vs. evil creatures

19. **Warding Light** (3 points)
    - Aura grants 5/magic damage reduction to nearby allies

20. **Martyr's Shield** (4 points)
    - Can sacrifice HP to grant temporary HP to allies

---

### Rogue Perks

#### Assassin Tree
1. **Sneak Attack Enhancement I-V** (1 point each)
   - +1d6 sneak attack damage per rank

2. **Deadly Precision I-II** (2/3 points)
   - Critical hits with sneak attack deal +1d6/+2d6 damage

3. **Improved Flanking I-II** (1/2 points)
   - +1/+2 to hit when flanking

4. **Quick Strike** (3 points)
   - Attack speed increased by 10% for 10 seconds after sneak attack

5. **Assassinate I-II** (3/4 points)
   - 5%/10% chance for sneak attack to instantly kill target below 50% HP

6. **Shadow Blend I-II** (2/3 points)
   - +5%/+10% to hide skill

7. **Poisoner I-III** (1/2/3 points)
   - Poison save DC +1/+2/+3

8. **Silent Killer** (2 points)
   - Sneak attacks don't break stealth on kill

9. **Ambush Training I-II** (2/3 points)
   - First attack from stealth deals +10%/+20% damage

10. **Knife Specialist I-II** (1/2 points)
    - +1/+2 to hit and damage with daggers and short swords

#### Thief Tree
11. **Nimble Fingers I-III** (1/2/3 points)
    - +2/+4/+6 to Pick Locks and Disable Trap

12. **Trap Savant I-II** (2/3 points)
    - Reduced trap disarm time by 25%/50%

13. **Evasion Enhancement** (3 points)
    - Take no damage on successful Reflex saves (requires Evasion feat)

14. **Acrobatic Training I-III** (1/2/3 points)
    - +2/+4/+6 to Tumble skill

15. **Light Armor Mastery** (2 points)
    - +1 AC in light armor, no armor check penalty

#### Shadowdancer Tree
16. **Shadow Step I-II** (2/3 points)
    - Reduce stealth movement penalty by 25%/50%

17. **Shadow Veil I-II** (2/3 points)
    - +10%/+20% concealment when in shadows

18. **Uncanny Dodge Enhancement** (3 points)
    - Cannot be caught flat-footed

19. **Slippery Mind I-II** (2/3 points)
    - +2/+4 to saves vs. mind-affecting spells

20. **Shadow Training I-III** (1/2/3 points)
    - +2/+4/+6 to Hide and Move Silently

---

### Ranger Perks

#### Hunter Tree
1. **Favored Enemy Enhancement I-III** (1/2/3 points)
   - +1/+2/+3 damage vs. favored enemy

2. **Improved Tracking I-II** (1/2 points)
   - +2/+4 to Survival skill

3. **Toughness I-V** (1 point each)
   - +5 HP per rank

4. **Bow Mastery I-III** (2/3/4 points)
   - +1/+2/+3 to hit with bows

5. **Deadly Aim I-II** (2/3 points)
   - +5%/+10% critical hit chance with ranged weapons

6. **Rapid Shot Enhancement** (3 points)
   - Rapid shot penalty reduced by 2

7. **Hunter's Focus I-II** (2/3 points)
   - +1d6/+2d6 damage to first attack each round

8. **Wilderness Stride** (2 points)
   - Move at full speed through difficult terrain

9. **Master Hunter** (3 points)
   - +2 to hit and damage against all favored enemies

10. **Arrow Deflection I-II** (2/3 points)
    - 5%/10% chance to deflect ranged attacks

#### Beast Master Tree
11. **Companion Bond I-III** (1/2/3 points)
    - Animal companion gains +5/+10/+15 HP

12. **Pack Leader I-II** (2/3 points)
    - Animal companion deals +1d6/+2d6 damage

13. **Defensive Companion** (3 points)
    - Animal companion gains +2 AC

14. **Coordinated Attack** (3 points)
    - You and companion get +2 to hit when both threaten same target

15. **Pack Tactics I-II** (2/3 points)
    - Share 10%/20% of your HP with companion

#### Two-Weapon Fighting Tree
16. **Dual Wielding Mastery I-III** (2/3/4 points)
    - Reduce two-weapon fighting penalty by 1/2/3

17. **Whirling Steel I-II** (2/3 points)
    - +1/+2 attacks per round when dual wielding

18. **Twin Blades** (3 points)
    - Off-hand attacks deal full damage

19. **Bladestorm I-II** (2/3 points)
    - 10%/20% chance to attack all adjacent enemies

20. **Perfect Balance** (4 points)
    - No penalties for two-weapon fighting

---

### Barbarian Perks

#### Berserker Tree
1. **Rage Enhancement I-V** (1 point each)
   - +1 to Strength and Constitution while raging per rank

2. **Extended Rage I-III** (1/2/3 points)
   - Rage lasts +2/+4/+6 rounds longer

3. **Toughness I-V** (1 point each)
   - +5 HP per rank

4. **Savage Blow I-II** (2/3 points)
   - +1d6/+2d6 damage while raging

5. **Furious Defense** (2 points)
   - Gain +2 AC while raging (reduces normal AC penalty)

6. **Relentless Rage I-II** (2/3 points)
   - Can rage +1/+2 additional times per day

7. **Intimidating Rage I-II** (1/2 points)
   - Enemies within 10' must save or be shaken while you rage

8. **Greater Rage Enhancement** (3 points)
   - Rage bonuses increased by +2

9. **Tireless Rage** (3 points)
   - No fatigue after rage ends

10. **Devastating Critical I-II** (3/4 points)
    - Critical hits while raging deal +2d6/+4d6 damage

#### Primal Warrior Tree
11. **Natural Armor I-III** (2/3/4 points)
    - +1/+2/+3 natural armor AC

12. **Primal Instinct I-II** (2/3 points)
    - +2/+4 to Survival and Intimidate

13. **Fast Movement Enhancement** (2 points)
    - +10 feet movement speed

14. **Physical Resistance I-III** (2/3/4 points)
    - +1/+2/+3 to Fortitude saves

15. **Thick Skinned I-II** (2/3 points)
    - 2/4 damage reduction

#### Frenzied Berserker Tree
16. **Battle Frenzy I-II** (2/3 points)
    - +10%/+20% attack speed

17. **Blood Frenzy** (3 points)
    - Gain +1 to hit for each 10% HP lost

18. **Reckless Attack I-II** (2/3 points)
    - Trade 2/4 AC for +2/+4 damage

19. **Mighty Rage** (4 points)
    - Rage grants +8 Str, +8 Con, +4 Will saves

20. **Death or Glory** (4 points)
    - When below 25% HP, deal double damage but take 50% more damage

---

### Paladin Perks

#### Holy Warrior Tree
1. **Smite Evil Enhancement I-V** (1 point each)
   - Smite Evil deals +1d6 per rank

2. **Divine Health Enhancement** (2 points)
   - Immunity to disease and +2 to saves vs. poison

3. **Lay on Hands Enhancement I-III** (1/2/3 points)
   - Heal +1d6/+2d6/+3d6 with Lay on Hands

4. **Righteous Might I-II** (2/3 points)
   - +1/+2 to hit and damage vs. evil creatures

5. **Holy Sword I-II** (2/3 points)
   - Weapon gains +1d6/+2d6 damage vs. undead

6. **Toughness I-V** (1 point each)
   - +5 HP per rank

7. **Extra Smite** (3 points)
   - Can use Smite Evil +1 additional time per day

8. **Divine Grace Enhancement I-II** (2/3 points)
   - +1/+2 to all saves from Charisma

9. **Aura of Courage Enhancement** (2 points)
   - Aura grants +4 to saves vs. fear (doubled)

10. **Weapon of Good** (3 points)
    - Weapon counts as good-aligned for DR purposes

#### Defender of Faith Tree
11. **Shield of Faith I-III** (2/3/4 points)
    - +1/+2/+3 deflection AC

12. **Defensive Aura I-II** (2/3 points)
    - Allies within 10' gain +1/+2 AC

13. **Healing Aura** (3 points)
    - Allies within 10' heal 1d6 HP per round

14. **Turn Undead Enhancement I-II** (2/3 points)
    - Turn undead affects +2/+4 HD worth of undead

15. **Divine Resolve I-II** (2/3 points)
    - +2/+4 to saves vs. spells

#### Divine Champion Tree
16. **Crusader's Strike I-II** (2/3 points)
    - Melee attacks heal you for 1d6/2d6 HP when you hit evil creatures

17. **Holy Vengeance** (3 points)
    - When hit by evil creature, deal 1d6 holy damage back

18. **Divine Weapon I-II** (2/3 points)
    - +1/+2 enhancement bonus to weapon

19. **Zealous Fervor I-II** (2/3 points)
    - +5%/+10% attack speed

20. **Avatar of Good** (4 points)
    - Gain DR 5/evil for 1 minute, 1/day

---

### Monk Perks

#### Master of Forms Tree
1. **Stunning Fist Enhancement I-III** (1/2/3 points)
   - Stunning Fist DC +1/+2/+3

2. **Flurry of Blows Enhancement I-II** (2/3 points)
   - +1/+2 attacks with Flurry of Blows

3. **Unarmed Damage I-V** (1 point each)
   - +1 to unarmed damage per rank

4. **Ki Strike Enhancement** (2 points)
   - Unarmed strikes count as magic and good for DR

5. **Toughness I-V** (1 point each)
   - +5 HP per rank

6. **Martial Training I-III** (1/2/3 points)
   - +1/+2/+3 to hit with unarmed strikes

7. **Rapid Strikes I-II** (2/3 points)
   - +10%/+20% attack speed

8. **Defensive Stance I-III** (2/3/4 points)
   - +1/+2/+3 AC while in defensive stance

9. **Ki Power I-III** (1/2/3 points)
   - +5/+10/+15 Ki points

10. **Perfect Strike** (3 points)
    - Unarmed critical multiplier increased by 1

#### Shintao Monk Tree
11. **Healing Ki I-II** (2/3 points)
    - Spend 10 Ki to heal 2d6/4d6 HP

12. **Diamond Soul Enhancement I-II** (2/3 points)
    - +5/+10 spell resistance

13. **Empty Body** (3 points)
    - Can become incorporeal for 1 round by spending 20 Ki

14. **Quivering Palm Enhancement** (3 points)
    - Quivering Palm DC +2

15. **Timeless Body Enhancement** (2 points)
    - Immunity to aging effects and +2 to saves vs. death

#### Wind Stance Tree
16. **Evasion Enhancement** (2 points)
    - Improved evasion - take half damage even on failed Reflex saves

17. **Dodge Training I-III** (2/3/4 points)
    - +1/+2/+3 dodge AC

18. **Spring Attack Enhancement** (2 points)
    - Can move after each attack

19. **Wind Through the Trees I-II** (2/3 points)
    - +10/+20 feet movement speed

20. **Lightning Reflexes I-III** (1/2/3 points)
    - +1/+2/+3 to Reflex saves

---

### Bard Perks

#### Spellsinger Tree
1. **Spell Focus: Enchantment I-III** (1/2/3 points)
   - +1/+2/+3 DC to enchantment spells

2. **Bardic Music Enhancement I-V** (1 point each)
   - Bardic music effects +1 per rank

3. **Lingering Song I-II** (2/3 points)
   - Bardic music lasts +2/+4 rounds after you stop performing

4. **Mental Toughness I-III** (1/2/3 points)
   - +1/+2/+3 to Will saves

5. **Spell Power: Enchantment I-III** (2/3/4 points)
   - +5/+10/+15 to enchantment spell power

6. **Arcane Augmentation I-V** (1 point each)
   - +5 spell points per rank

7. **Enthralling Performance** (3 points)
   - Fascinate affects +2 creatures

8. **Inspire Courage Enhancement I-II** (2/3 points)
   - Inspire Courage grants +1/+2 additional bonus

9. **Sonic Blast I-II** (2/3 points)
   - Sonic spells deal +1d6/+2d6 damage

10. **Versatile Performer I-II** (1/2 points)
    - Can use Perform for +1/+2 additional skills

#### Warchanter Tree
11. **Battle Song I-II** (2/3 points)
    - Allies gain +1/+2 to hit while you perform

12. **Song of Heroism** (3 points)
    - Inspire Courage also grants +10 temporary HP

13. **Defensive Ballad I-II** (2/3 points)
    - Song grants +1/+2 AC to allies

14. **Martial Training I-II** (1/2 points)
    - +1/+2 to hit with weapons

15. **Toughness I-V** (1 point each)
    - +5 HP per rank

#### Virtuoso Tree
16. **Jack of All Trades Enhancement I-II** (2/3 points)
    - +2/+4 to all skills

17. **Skill Mastery I-III** (1/2/3 points)
    - +2/+4/+6 to Perform and chosen skill

18. **Bardic Knowledge Enhancement I-II** (2/3 points)
    - +2/+4 to all Knowledge skills

19. **Cunning Wit** (3 points)
    - Can use Charisma for Intelligence-based skill checks

20. **Master Performer** (4 points)
    - Can maintain two bardic songs simultaneously

---

### Druid Perks

#### Nature's Warrior Tree
1. **Wild Shape Enhancement I-V** (1 point each)
   - Wild shape forms gain +2 Str/Con per rank

2. **Natural Armor Enhancement I-III** (2/3/4 points)
   - +1/+2/+3 natural armor AC in wild shape

3. **Predator's Grace I-II** (2/3 points)
   - +10/+20 feet movement speed in wild shape

4. **Savage Bite I-II** (2/3 points)
   - Natural attacks deal +1d6/+2d6 damage

5. **Pack Leader I-III** (1/2/3 points)
   - Animal companion gains +5/+10/+15 HP

6. **Primal Instinct I-II** (2/3 points)
   - +2/+4 to Survival and Handle Animal

7. **Toughness I-V** (1 point each)
   - +5 HP per rank

8. **Feral Charge** (3 points)
   - Can charge in wild shape form

9. **Aspect of the Beast I-II** (2/3 points)
   - Gain animal senses: +2/+4 to Spot and Listen

10. **Primordial Fury** (3 points)
    - Critical hits in wild shape deal +2d6 damage

#### Nature's Protector Tree
11. **Spell Power: Healing I-III** (2/3/4 points)
    - +5/+10/+15 to healing spell power

12. **Barkskin Enhancement I-II** (2/3 points)
    - Barkskin grants +2/+4 additional natural armor

13. **Entangle Enhancement** (2 points)
    - Entangle affects larger area and higher DC

14. **Thornshield I-II** (2/3 points)
    - Enemies hitting you take 1d6/2d6 damage

15. **Nature's Blessing I-II** (2/3 points)
    - +1/+2 to all saves

#### Elemental Mastery Tree
16. **Spell Power: Cold I-III** (2/3/4 points)
    - +5/+10/+15 to cold spell damage

17. **Spell Power: Electric I-III** (2/3/4 points)
    - +5/+10/+15 to electric spell damage

18. **Spell Power: Fire I-III** (2/3/4 points)
    - +5/+10/+15 to fire spell damage

19. **Summon Nature's Ally Enhancement** (3 points)
    - Summoned creatures have +25% HP and damage

20. **Elemental Form** (4 points)
    - Can wild shape into elemental forms

---

### Sorcerer Perks

#### Draconic Bloodline Tree
1. **Dragon Heritage I-V** (1 point each)
   - +5 HP per rank (draconic vitality)

2. **Dragon Scales I-III** (2/3/4 points)
   - +1/+2/+3 natural armor AC

3. **Draconic Resistance I-II** (2/3 points)
   - Resist 10/20 to chosen energy type

4. **Dragon Breath Enhancement I-II** (2/3 points)
   - Breath weapon DC +1/+2, damage +1d6/+2d6

5. **Wings of the Dragon** (4 points)
   - Gain ability to fly

6. **Draconic Presence I-II** (2/3 points)
   - +2/+4 to Intimidate and Charisma-based checks

7. **Energy Affinity** (2 points)
   - Choose energy type: spells of that type deal +10% damage

8. **Draconic Spell I-II** (2/3 points)
   - Spells gain +1/+2 caster level for overcoming SR

9. **Bloodline Power I-III** (1/2/3 points)
   - +1/+2/+3 to spell DCs

10. **Ancient Power** (3 points)
    - Spells ignore 25% energy resistance

#### Wild Magic Tree
11. **Surge of Power I-III** (2/3/4 points)
    - 5%/10%/15% chance spells cost no spell points

12. **Chaotic Casting I-II** (2/3 points)
    - Spells have 10%/20% chance to deal +50% damage

13. **Arcane Augmentation I-V** (1 point each)
    - +5 spell points per rank

14. **Spell Power: Universal I-III** (2/3/4 points)
    - +5/+10/+15 to all spell damage

15. **Wild Surge** (3 points)
    - Can choose to add +2 to spell DC with 10% chance of wild magic effect

#### Elemental Savant Tree
16. **Elemental Focus: Fire I-III** (2/3/4 points)
    - +10/+20/+30% fire damage, but -10% to other elements

17. **Elemental Focus: Cold I-III** (2/3/4 points)
    - +10/+20/+30% cold damage, but -10% to other elements

18. **Elemental Focus: Electric I-III** (2/3/4 points)
    - +10/+20/+30% electric damage, but -10% to other elements

19. **Spell Penetration Enhancement I-II** (2/3 points)
    - +2/+4 to spell penetration checks

20. **Energy Mastery** (4 points)
    - Your chosen element ignores 50% resistance

---

## Prestige Class Perks

### Arcane Archer Perks

1. **Enchanted Arrow I-III** (1/2/3 points)
   - Arrows gain +1/+2/+3 enhancement bonus

2. **Imbue Arrow Enhancement I-II** (2/3 points)
   - Area spells can be cast on arrow with +1/+2 caster level

3. **Seeker Arrow Enhancement** (2 points)
   - Seeker arrow ignores miss chance

4. **Phase Arrow Enhancement** (3 points)
   - Phase arrow can pass through multiple targets

5. **Deadly Precision I-III** (2/3/4 points)
   - +1d6/+2d6/+3d6 damage with arrows

6. **Rapid Fire I-II** (2/3 points)
   - +10%/+20% attack speed with bows

7. **Arrow of Death Enhancement** (4 points)
   - Arrow of Death DC +2

8. **Elemental Arrows I-II** (2/3 points)
   - Add 1d6/2d6 elemental damage to arrows

9. **Perfect Aim I-II** (2/3 points)
   - +1/+2 to hit with bows

10. **Hail of Arrows** (3 points)
    - Can shoot at all enemies in cone

11. **Distance Shot I-II** (1/2 points)
    - +25%/+50% range increment

12. **Critical Strike I-II** (2/3 points)
    - +5%/+10% critical chance with bows

13. **Piercing Shot** (3 points)
    - Arrows ignore 50% of target's AC

14. **Toughness I-V** (1 point each)
    - +5 HP per rank

15. **Arcane Strike** (3 points)
    - Spend spell points to add 1d6 damage per 2 spell points

16. **Bow Mastery** (4 points)
    - +2 to hit and damage with bows

17. **Magical Fletching I-II** (2/3 points)
    - Create magical arrows that return after use

18. **True Strike Enhancement** (2 points)
    - True Strike also applies to next 2 attacks

19. **Arcane Quiver** (3 points)
    - Arrows never run out

20. **Rain of Arrows** (4 points)
    - Once per day, make 10 shots in one round

---

### Assassin Perks

1. **Death Attack Enhancement I-III** (2/3/4 points)
   - Death attack DC +1/+2/+3

2. **Poison Use Enhancement I-III** (1/2/3 points)
   - Poison DC +1/+2/+3

3. **Deadly Sneak I-V** (1 point each)
   - +1d6 sneak attack damage per rank

4. **Hide in Plain Sight Enhancement** (3 points)
   - Can hide even while observed

5. **Silent Death I-II** (2/3 points)
   - Death attacks are completely silent, +2/+4 to Move Silently

6. **Crippling Strike Enhancement** (2 points)
   - Crippling strike also reduces target's AC by 2

7. **Quick Death I-II** (2/3 points)
   - Death attack study time reduced by 1/2 rounds

8. **Assassin's Resolve I-II** (2/3 points)
   - +2/+4 to saves vs. mind-affecting

9. **Shadow Blend I-III** (2/3/4 points)
   - +5%/+10%/+15% concealment in shadows

10. **Master of Disguise I-II** (2/3 points)
    - +5/+10 to Disguise skill

11. **Lethal Precision** (3 points)
    - Critical hits with sneak attack have +2 multiplier

12. **Toxic Mastery I-II** (2/3 points)
    - Poisons last +2/+4 rounds longer

13. **Improved Death Attack** (4 points)
    - Death attack can kill or paralyze targets up to 3 HD higher

14. **Shadow Strike I-II** (2/3 points)
    - First attack from stealth deals +25%/+50% damage

15. **Evasion Enhancement** (2 points)
    - Improved evasion in light armor

16. **Killer's Instinct I-II** (2/3 points)
    - +2/+4 to Spot and Sense Motive

17. **Toughness I-V** (1 point each)
    - +5 HP per rank

18. **Silent Kill** (3 points)
    - Kills don't alert nearby enemies

19. **Master Assassin** (4 points)
    - Can use death attack as standard action

20. **Angel of Death** (4 points)
    - Once per day, automatically succeed on death attack

---

### Blackguard Perks

1. **Smite Good Enhancement I-V** (1 point each)
   - Smite Good deals +1d6 per rank

2. **Dark Blessing I-II** (2/3 points)
   - +1/+2 to all saves from Charisma

3. **Aura of Despair Enhancement I-II** (2/3 points)
   - Enemies within 10' take -1/-2 to saves

4. **Unholy Weapon I-III** (2/3/4 points)
   - Weapon deals +1d6/+2d6/+3d6 unholy damage

5. **Dark Knight I-II** (2/3 points)
   - +1/+2 to hit and AC vs. good creatures

6. **Command Undead Enhancement I-II** (2/3 points)
   - Command undead affects +2/+4 HD worth of undead

7. **Fiendish Mount Enhancement** (3 points)
   - Mount gains +25% HP and damage

8. **Toughness I-V** (1 point each)
   - +5 HP per rank

9. **Corrupt Touch I-II** (2/3 points)
   - Touch attack deals 1d6/2d6 negative energy damage

10. **Extra Smite** (3 points)
    - Can use Smite Good +1 additional time per day

11. **Dark Purpose I-II** (2/3 points)
    - +2/+4 to Intimidate

12. **Dread Aura I-II** (2/3 points)
    - Aura causes fear (save DC 10+level/12+level)

13. **Unholy Resilience I-II** (2/3 points)
    - +10/+20 temporary HP when smiting

14. **Fell Weapon** (3 points)
    - Critical hits drain 1 level from good creatures

15. **Dark Might** (3 points)
    - Gain DR 5/good

16. **Sadistic Strike I-II** (2/3 points)
    - Healing spells on you deal 50%/100% of healing as damage to nearby good creatures

17. **Terrifying Presence I-II** (2/3 points)
    - Enemies within 20' must save or be frightened

18. **Unholy Champion** (4 points)
    - +2 to all ability scores vs. good creatures

19. **Plague Bringer** (3 points)
    - Touch can inflict disease

20. **Avatar of Evil** (4 points)
    - Once per day, gain +4 Str/Con and immune to good spells for 1 minute

---

### Eldritch Knight Perks

1. **Spellsword I-III** (2/3/4 points)
   - +1/+2/+3 to concentration checks in combat

2. **Battle Casting I-II** (2/3 points)
   - Cast spells in medium/heavy armor with 10%/20% reduced failure

3. **Arcane Strike I-III** (2/3/4 points)
   - Spend spell points to add 1d6 damage per 2/1.5/1 points

4. **Spell Critical I-II** (2/3 points)
   - Weapon critical hits restore 2/4 spell points

5. **Toughness I-V** (1 point each)
   - +5 HP per rank

6. **Weapon Focus Enhancement I-II** (1/2 points)
   - +1/+2 to hit with chosen weapon

7. **Arcane Defense I-III** (2/3/4 points)
   - +1/+2/+3 AC bonus

8. **Spell Power: Universal I-II** (2/3 points)
   - +5/+10 to all spell damage

9. **Martial Prowess I-II** (2/3 points)
   - +1/+2 to all melee attack rolls

10. **Arcane Augmentation I-V** (1 point each)
    - +5 spell points per rank

11. **Blade Shield** (3 points)
    - When casting, gain +4 AC for 1 round

12. **Spell Fury I-II** (2/3 points)
    - +10%/+20% spell critical chance

13. **Warrior Mage** (3 points)
    - Can cast one spell per round as a swift action

14. **Enhanced Mobility** (2 points)
    - Can move and cast in same round without penalty

15. **Arcane Channeling I-II** (2/3 points)
    - Weapon attacks have 10%/20% chance to cast spell on hit

16. **Spellstrike** (3 points)
    - Can deliver touch spells through weapon

17. **Improved Spell Combat** (3 points)
    - Cast two spells per round

18. **Battle Meditation** (2 points)
    - Regain 1 spell point per round in combat

19. **Arcane Supremacy I-II** (3/4 points)
    - +1/+2 to both melee hit and spell DCs

20. **Spell Blade Master** (4 points)
    - Attacks and spells have +25% critical chance

---

### Weapon Master Perks

1. **Weapon of Choice I-V** (1 point each)
   - +1 to hit and damage per rank with chosen weapon

2. **Superior Weapon Focus I-III** (2/3/4 points)
   - Additional +1/+2/+3 to hit with chosen weapon

3. **Devastating Critical Enhancement I-II** (3/4 points)
   - Critical hits with chosen weapon deal +2d6/+4d6 damage

4. **Increased Multiplier I-II** (3/4 points)
   - Critical multiplier with chosen weapon +1/+2

5. **Ki Critical** (3 points)
   - Can spend 10 Ki to automatically confirm critical

6. **Weapon Mastery** (4 points)
   - Chosen weapon never breaks and counts as adamantine

7. **Toughness I-V** (1 point each)
   - +5 HP per rank

8. **Lightning Reflexes I-III** (2/3/4 points)
   - +1/+2/+3 to Reflex saves

9. **Combat Prowess I-II** (2/3 points)
   - +1/+2 to all attack rolls

10. **Defensive Mastery I-II** (2/3 points)
    - +1/+2 dodge AC

11. **Precise Strike I-III** (2/3/4 points)
    - +1d6/+2d6/+3d6 precision damage with chosen weapon

12. **Master's Strike** (3 points)
    - Critical threat range with chosen weapon expanded by 2

13. **Improved Critical Enhancement** (3 points)
    - Critical threat range doubled (stacks with Improved Critical feat)

14. **Blinding Speed I-II** (3/4 points)
    - +1/+2 attacks per round with chosen weapon

15. **Perfect Strike I-II** (2/3 points)
    - Ignore 25%/50% of target's AC with chosen weapon

16. **Sudden Strike** (3 points)
    - Once per encounter, make 3 attacks as one action

17. **Weapon Supremacy** (4 points)
    - All attacks with chosen weapon automatically threaten critical

18. **Unstoppable Strike I-II** (3/4 points)
    - Ignore 5/10 points of damage reduction

19. **Legendary Weapon** (4 points)
    - Chosen weapon gains +2 enhancement bonus

20. **Master of Arms** (4 points)
    - Can apply weapon mastery to second chosen weapon

---

## Implementation Notes

### Experience Tracking
- Current level XP requirement split into 4 stages
- Stage 1: 0-25% of level XP
- Stage 2: 25-50% of level XP
- Stage 3: 50-75% of level XP
- Stage 4: 75-100% of level XP (level up occurs)

### Perk Point Allocation
- Points earned at Stage 1, 2, and 3 are tagged to the class being leveled
- Points can only be spent on perks from that class's list
- Multiclass characters maintain separate perk point pools per class

### Prerequisites
- Some higher-tier perks may require previous ranks
- Example: "Toughness III" requires "Toughness I" and "Toughness II"
- This creates natural progression trees

### Retraining
- Consider allowing perk retraining at trainer NPCs for a gold cost
- Full retrain or individual perk changes
- Prevents players from feeling "locked in" to bad choices

### Display
- UI should show:
  - Current stage progress (1/4, 2/4, 3/4)
  - Available perk points per class
  - Perk trees in browsable format
  - Which perks are purchased and at what rank

### Balance Considerations
- Perks should feel meaningful but not mandatory
- Avoid "must-have" perks that overshadow others
- Ranged values can be adjusted during playtesting
- Some perks intentionally weaker to allow player choice without wrong answers

---

## Future Expansion Ideas

1. **Racial Perks**: Consider adding racial perk trees that anyone can access
2. **Legendary Perks**: High-level perks (20+) that are extremely powerful
3. **Multiclass Synergy Perks**: Special perks that unlock when you have levels in multiple classes
4. **Deity Perks**: Divine classes could have deity-specific perks
5. **Crafting Perks**: Tie into existing crafting system
6. **Epic Perks**: Post-level 20 epic progression perks

---

## Questions for Further Design

1. Should there be a maximum number of ranks in a single perk?
2. Should some perks have level requirements beyond just previous ranks?
3. How do we handle prestige class perks for characters with multiple prestige classes?
4. Should perk effects scale with character level or remain static?
5. Do we want "capstone" perks at the end of each tree that are particularly powerful?
6. Should racial perks be implemented in phase 1 or phase 2?
7. How should the respec system work economically and narratively?

---

*Document Version: 1.0*
*Last Updated: October 19, 2025*
*Status: Initial Design - Ready for Review*
