# Paladin Perk System Design

## Overview
The Paladin perk system is divided into three thematic trees, each with four tiers of increasing power and cost. Each tree focuses on a different aspect of the holy warrior's abilities, inspired by D&D 5e oaths and DDO enhancement trees.

---

## Tree 1: KNIGHT OF THE CHALICE (Divine Warrior)
*Focus: Melee combat, smiting evil, and weapon mastery*
*Inspired by D&D 5e Oath of Devotion and DDO Knight of the Chalice*

### Tier 1 Perks (1 point each)

#### Extra Smite I
- **Description**: Gain additional uses of Smite Evil per day
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +1 use of Smite Evil per day per rank (+3 total at max)

#### Holy Weapon I
- **Description**: Your melee attacks deal additional holy damage
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +2 divine damage per rank on melee attacks against evil creatures (+6 at max)

#### Sacred Defender
- **Description**: Divine energy protects you in combat
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +1 AC per rank when wielding a weapon and shield (+3 at max)

#### Faithful Strike
- **Description**: Channel divine power to strike true
- **Max Ranks**: 1
- **Prerequisites**: None
- **Effect**: Gain "Faithful Strike" ability - swift action to add WIS modifier to next attack roll, 1 minute cooldown

---

### Tier 2 Perks (2 points each)

#### Extra Smite II
- **Description**: Further increase smite uses
- **Max Ranks**: 2
- **Prerequisites**: Extra Smite I (3 ranks)
- **Effect**: +1 uses of Smite Evil per day per rank (+2 total at max)

#### Holy Weapon II
- **Description**: Your weapons burn with righteous fire
- **Max Ranks**: 2
- **Prerequisites**: Holy Weapon I (3 ranks)
- **Effect**: Additional +2 divine damage per rank on melee attacks against evil creatures (+4 at max)

#### Improved Smite
- **Description**: Your smites are more devastating
- **Max Ranks**: 3
- **Prerequisites**: Extra Smite I (2 ranks)
- **Effect**: Smite Evil deals +1d6 bonus damage per rank (+3d6 at max)

#### Holy Blade
- **Description**: Enchant your weapon with divine power
- **Max Ranks**: 1
- **Prerequisites**: Holy Weapon I (2 ranks)
- **Effect**: Gain "Holy Blade" ability - Your weapon gains +2 enhancement bonus and good alignment for 5 minutes, 10 minute cooldown

---

### Tier 3 Perks (3 points each)

#### Divine Might
- **Description**: Channel divinity into devastating strikes
- **Max Ranks**: 1
- **Prerequisites**: Improved Smite (2 ranks)
- **Effect**: Gain "Divine Might" ability - Swift action for 1 minute: add CHA modifier to melee damage, 5 minute cooldown

#### Exorcism of the Slain
- **Description**: Your smites cause extra damage to evil outsiders
- **Max Ranks**: 1
- **Prerequisites**: Improved Smite (3 ranks)
- **Effect**: When you hit an evil outsider, demon, or devil with Smite Evil, deal an extra +4d6 damage.

#### Holy Sword
- **Description**: Ultimate weapon enchantment
- **Max Ranks**: 1
- **Prerequisites**: Holy Blade
- **Effect**: Holy Blade now grants additional +2 enhancement bonus, deals +2d6 holy damage vs evil

#### Zealous Smite
- **Description**: Smites restore your vigor
- **Max Ranks**: 1
- **Prerequisites**: Extra Smite II (2 ranks)
- **Effect**: When you kill an enemy with Smite Evil, regain 1 use of Smite Evil (once per 5 minutes)

---

### Tier 4 Perks (4 points each)

#### Blinding Smite
- **Description**: Your smites blind evil foes
- **Max Ranks**: 1
- **Prerequisites**: Divine Might
- **Effect**: Smite Evil also blinds target for 2 rounds on hit (Will save negates, DC 10 + Paladin level + CHA mod)

#### Overwhelming Smite
- **Description**: Smites knock down foes
- **Max Ranks**: 1
- **Prerequisites**: Exorcism of the Slain
- **Effect**: Critical hits with Smite Evil active knock target prone (Fortitude save negates, DC 10 + Paladin level + STR mod)

#### Sacred Vengeance
- **Description**: When allies fall, you grow stronger
- **Max Ranks**: 1
- **Prerequisites**: Zealous Smite
- **Effect**: When an ally drops below 25% HP or dies in the same room as you, gain +4 to hit and +8 damage for 3 rounds (5 minute cooldown)

---

## Tree 2: SACRED DEFENDER (Protection & Healing)
*Focus: Defense, healing, and protecting allies*
*Inspired by D&D 5e Oath of Devotion/Redemption and DDO Sacred Defender*

### Tier 1 Perks (1 point each)

#### Extra Lay on Hands I
- **Description**: Increase healing power reserves
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +1 uses of Lay on Hands per day per rank (+3 total at max)

#### Shield of Faith I
- **Description**: Divine protection shields you
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +1 Deflection AC per rank (+3 at max)

#### Bulwark of Defense
- **Description**: Improved defensive stance with shield
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +1 bonus to saves per rank when wielding a shield (+3 at max)

#### Defensive Strike
- **Description**: Make an attack, which, if it hits, raises your AC
- **Max Ranks**: 1
- **Prerequisites**: None
- **Effect**: Use the defensivestrike command to make an attack, which if it hits, raises AC by 2 for 5 rounds. 2 minute cooldown.

---

### Tier 2 Perks (2 points each)

#### Extra Lay on Hands II
- **Description**: Further increase healing reserves
- **Max Ranks**: 2
- **Prerequisites**: Extra Lay on Hands I (3 ranks)
- **Effect**: +1 uses of Lay on Hands per day per rank (+3 total at max)

#### Shield of Faith II
- **Description**: Enhanced divine protection
- **Max Ranks**: 2
- **Prerequisites**: Shield of Faith I (3 ranks)
- **Effect**: Additional +1 Deflection AC per rank (+2 at max)

#### Healing Hands
- **Description**: More potent healing
- **Max Ranks**: 3
- **Prerequisites**: Extra Lay on Hands I (2 ranks)
- **Effect**: Lay on Hands heals +10% more per rank (+30% at max)

#### Shield Guardian
- **Description**: Protect adjacent allies with your shield
- **Max Ranks**: 1
- **Prerequisites**: Bulwark of Defense (2 ranks)
- **Effect**: Grouped allies in your room gain +2 AC bonus from your shield proficiency

---

### Tier 3 Perks (3 points each)

#### Aura of Protection
- **Description**: Strengthen your protective aura
- **Max Ranks**: 1
- **Prerequisites**: Shield of Faith II (2 ranks)
- **Effect**: Your Aura of Courage radius grants +2 to all saves

#### Sanctuary
- **Description**: Become a beacon of divine safety
- **Max Ranks**: 1
- **Prerequisites**: Shield Guardian
- **Effect**: Gain a damage shield that reduces all incoming damage by 10%

#### Merciful Touch
- **Description**: Lay on Hands can cure conditions
- **Max Ranks**: 1
- **Prerequisites**: Healing Hands (3 ranks)
- **Effect**: Lay on Hands also gives +20 to current and max hit points for 5 rounds. Doesn't stack.

#### Bastion of Defense
- **Description**: Become an immovable defender
- **Max Ranks**: 1
- **Prerequisites**: Bulwark of Defense (3 ranks), Shield Guardian
- **Effect**: Gain "Bastion" ability - Swift action: gain 20 temporary HP, +4 AC, and immunity to knockdown for 5 rounds, 5 minute cooldown

---

### Tier 4 Perks (4 points each)

#### Aura of Life
- **Description**: Your presence sustains allies
- **Max Ranks**: 1
- **Prerequisites**: Aura of Protection
- **Effect**: Allies within your aura regenerate 2 HP per round in combat, 5 HP per round out of combat

#### Cleansing Touch
- **Description**: Touch banishes evil magic
- **Max Ranks**: 1
- **Prerequisites**: Merciful Touch
- **Effect**: Lay on Hands can remove one negative affect and can be used as a swift action

#### Divine Sacrifice
- **Description**: Take damage meant for allies
- **Max Ranks**: 1
- **Prerequisites**: Bastion of Defense
- **Effect**: When an ally within 30 feet would be reduced below 0 HP, you may take the damage instead (once per 10 minutes)

---

## Tree 3: DIVINE CHAMPION (Spellcasting & Auras)
*Focus: Divine magic, turning undead, and channeling energy*
*Inspired by D&D 5e Oath of Glory and DDO Radiant Servant*

### Tier 1 Perks (1 point each)

#### Spell Focus I
- **Description**: More potent divine spells
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +1 to spell save DCs per rank (+3 at max)

#### Turn Undead Mastery I
- **Description**: Enhanced turning
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: Turn Undead affects +2 HD worth of undead per rank (+6 at max)

#### Divine Grace
- **Description**: Divine favor protects you
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: +1 to all saving throws per rank (+3 at max)

#### Radiant Aura
- **Description**: Emit holy light
- **Max Ranks**: 1
- **Prerequisites**: None
- **Effect**: Gain "Radiant Aura" ability - Toggle: emit light, undead within 10 feet take 1d4 divine damage per round, costs 1 move per round active

---

### Tier 2 Perks (2 points each)

#### Spell Focus II
- **Description**: Master of divine magic
- **Max Ranks**: 2
- **Prerequisites**: Spell Focus I (3 ranks)
- **Effect**: Additional +1 to spell save DCs per rank (+2 at max)

#### Turn Undead Mastery II
- **Description**: Devastating turning
- **Max Ranks**: 2
- **Prerequisites**: Turn Undead Mastery I (3 ranks)
- **Effect**: Turn Undead affects additional +3 HD worth per rank and deals +2d6 damage per rank

#### Quickened Blessing
- **Description**: Swift divine blessings
- **Max Ranks**: 1
- **Prerequisites**: Spell Focus I (2 ranks)
- **Effect**: Once per day, you may cast Bless, Shield of Faith, or Protection from Evil as a swift action

#### Channel Energy I
- **Description**: Channel positive energy
- **Max Ranks**: 3
- **Prerequisites**: None
- **Effect**: Gain "Channel Energy" ability - Standard action: heal allies or damage undead in 30 foot radius for 1d6 per rank, 2 uses per day

---

### Tier 3 Perks (3 points each)

#### Spell Penetration
- **Description**: Pierce spell resistance
- **Max Ranks**: 1
- **Prerequisites**: Spell Focus II (2 ranks)
- **Effect**: +4 bonus to overcome spell resistance with paladin spells

#### Destroy Undead
- **Description**: Turn Undead can destroy
- **Max Ranks**: 1
- **Prerequisites**: Turn Undead Mastery II (2 ranks)
- **Effect**: When Turn Undead affects undead with less than half your paladin level in HD, they are destroyed instead of turned

#### Channel Energy II
- **Description**: Powerful channeling
- **Max Ranks**: 2
- **Prerequisites**: Channel Energy I (3 ranks)
- **Effect**: Channel Energy heals/damages +2d6 per rank and gains +2 uses per day per rank

#### Aura of Courage Mastery
- **Description**: Enhanced fear immunity
- **Max Ranks**: 1
- **Prerequisites**: Divine Grace (3 ranks)
- **Effect**: Your Aura of Courage grants immunity to fear and charm effects, and grants +4 bonus vs mind-affecting

---

### Tier 4 Perks (4 points each)

#### Mass Cure Wounds
- **Description**: Healing burst
- **Max Ranks**: 1
- **Prerequisites**: Channel Energy II (2 ranks)
- **Effect**: Gain "Mass Cure Wounds" ability - Standard action: heal all allies within 30 feet for 3d8 + CHA modifier, twice per day

#### Holy Avenger
- **Description**: Ultimate spell and turning synergy
- **Max Ranks**: 1
- **Prerequisites**: Spell Penetration, Destroy Undead
- **Effect**: When you destroy undead with Turn Undead, your next spell cast within 1 round is cast at +4 caster level and has +2 DC

#### Beacon of Hope
- **Description**: Inspire allies with divine presence
- **Max Ranks**: 1
- **Prerequisites**: Aura of Courage Mastery
- **Effect**: Gain "Beacon of Hope" ability - Standard action: all allies within 30 feet gain advantage on saves, immunity to fear, and maximize healing received for 5 rounds, once per day

---

## Perk Point Costs Summary

### By Tier:
- **Tier 1**: 1 point per perk
- **Tier 2**: 2 points per perk  
- **Tier 3**: 3 points per perk
- **Tier 4**: 4 points per perk

### Maximum Investment Per Tree:
Assuming all ranks and all perks taken in a single tree:
- **Tier 1**: ~8-10 points
- **Tier 2**: ~12-16 points
- **Tier 3**: ~9-12 points
- **Tier 4**: ~8-12 points
- **Total per tree**: ~40-50 points for complete mastery

---

## Design Philosophy

### Knight of the Chalice
- Primary focus: Offensive paladin, melee damage dealer
- Smite Evil is central mechanic with multiple enhancements
- Weapon enchantments and holy damage
- Best for paladins who want to deal damage and destroy evil

### Sacred Defender  
- Primary focus: Tank/support paladin, protecting party
- Lay on Hands is central mechanic with multiple enhancements
- AC bonuses, damage mitigation, and ally protection
- Best for paladins who want to tank and support allies

### Divine Champion
- Primary focus: Caster/support paladin, divine magic specialist
- Turn Undead and Channel Energy are central mechanics
- Spell enhancement and aura improvements
- Best for paladins who emphasize their divine spellcasting and anti-undead role

---

## Integration Notes

### Spell Preparation Adaptation
- No spell point costs in any perks
- Perks that affect spells work with prepared spells
- Daily use abilities use standard cooldown system
- Channel Energy uses daily charges (not spell slots)
- Quickened Blessing allows swift casting without affecting preparation

### Synergies Between Trees
- Knight of the Chalice + Sacred Defender: Tanky damage dealer with self-healing
- Knight of the Chalice + Divine Champion: Spell-enhanced smiter with turning
- Sacred Defender + Divine Champion: Ultimate support/healer build with channeling

### Balance Considerations
- Each tree offers unique playstyle
- No tree is strictly superior
- Can mix trees for hybrid builds
- Tier 4 perks are powerful but have meaningful costs
- Prerequisites guide players through logical progression

---

## Implementation Priority

### Phase 1 (Core mechanics):
1. Extra Smite / Extra Lay on Hands / Turn Undead Mastery perks
2. Basic damage/AC/save bonuses
3. Simple ability perks (Faithful Strike, Defensive Strike)

### Phase 2 (Active abilities):
4. Holy Blade, Divine Might, Bastion of Defense
5. Channel Energy system
6. Radiant Aura toggle

### Phase 3 (Advanced features):
7. Condition removal (Merciful Touch, Cleansing Touch)
8. Aura enhancements
9. Exorcism and destruction mechanics

### Phase 4 (Ultimate abilities):
10. Tier 4 capstone abilities
11. Complex synergies
12. Mass healing and group buffs

---

## Testing Recommendations

1. **Smite scaling**: Verify damage bonuses don't become overpowered at high levels
2. **Healing balance**: Ensure extra Lay on Hands uses don't trivialize damage
3. **AC stacking**: Monitor Sacred Defender AC bonuses for excessive tankiness
4. **Channel Energy**: Balance healing/damage output vs other class abilities
5. **Cooldowns**: Adjust ability cooldowns based on actual gameplay
6. **Prerequisite chains**: Ensure prerequisites flow logically and aren't too restrictive

---

## Future Expansion Ideas

### Potential Tree 4: Oath Breaker (Anti-Paladin)
- Focus on dark powers and fallen paladin abilities
- Requires alignment shift to unlock
- Smite Good instead of Smite Evil
- Touch of Corruption instead of Lay on Hands

### Potential Additional Perks:
- Mount-specific combat perks
- Oath-specific abilities (Devotion, Ancients, Vengeance)
- Divine weapon summons
- Consecrated ground mechanics
- Redemption/Atonement themed abilities
