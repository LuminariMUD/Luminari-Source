# Artifact Object Stats from the Source MUDs

## 1. Purpose and status

This is a source-data inventory, not a balance proposal and not a description
of the current LuminariMUD artifact roster. It records the actual artifact
object prototypes found in the two source MUD snapshots under `EXAMPLE/`.

The current LuminariMUD implementation is documented in
[`docs/systems/ARTIFACT_SYSTEM.md`](../../systems/ARTIFACT_SYSTEM.md). The
long-form HomelandMUD behavior study is in [`artifacts.md`](artifacts.md).

Snapshot provenance:

| Source MUD | Commit | Commit date |
| --- | --- | --- |
| HomelandMUD | `0dfd8fc0053b2c5573463e43066ec3de669bd46f` | 2020-03-17 |
| RealmsOfLuminari | `3f57e70c45327335187fd123c991388e8bab2661` | 2025-08-11 |

## 2. Scope and method

"Artifact" is determined from each source MUD's artifact registry, not from
an object description containing the word "artifact":

- RealmsOfLuminari reads every VNUM in `areas/world.artifact` and marks the
  corresponding prototype with `IDX_ARTIFACT`. The bundled file contains 12
  rows, and all 12 prototypes are present.
- HomelandMUD hard-codes nine VNUMs in `is_artifact()`. Six prototypes are
  present; VNUMs 501, 513, and 599 are absent from all object files.

This produces 21 registry entries across the snapshots: 18 recoverable object
prototypes and three entries for which no honest object stats can be given.

The following layers are kept separate throughout this document:

1. **Stored prototype:** type, values, weight, cost, wear flags, extra flags,
   object applies, and permanent affect bits read from the object file.
2. **Assigned special procedure:** a function attached by VNUM at boot. This
   is behavior, not a numeric object field.
3. **Realms artifact overlay:** optional level-scaled bonuses initialized in
   `initializeArtifacts()`. These stack with the stored prototype while the
   artifact is equipped.

Damage averages below are mathematical dice averages. Weight and cost are
reported as raw game fields because neither snapshot establishes a portable
real-world unit or economy conversion.

## 3. Source format notes

### 3.1 RealmsOfLuminari

Relevant sources:

- artifact membership: `EXAMPLE/RealmsOfLuminari/areas/world.artifact:7-18`;
- object parser: `EXAMPLE/RealmsOfLuminari/src/db.c:3187-3378`;
- object, wear, extra, anti, affect, and apply constants:
  `EXAMPLE/RealmsOfLuminari/src/structs.h:283-425,1162-1353`;
- artifact overlay initialization:
  `EXAMPLE/RealmsOfLuminari/src/db.c:5293-5571`;
- overlay application and level scaling:
  `EXAMPLE/RealmsOfLuminari/src/handler.c:4778-4913`; and
- special-procedure assignments:
  `EXAMPLE/RealmsOfLuminari/src/specs.assign.c:1717-1728,1858-1864`.

Realms object records use these numeric groups:

```text
type extra_flags wear_flags [anti_flags]
value[0] value[1] value[2] value[3] [...]
weight cost durability [affect_word_1] [affect_word_2]
```

For `ITEM_WEAPON`, `value[0]` is the stored proc value, `value[1]dvalue[2]`
is weapon damage, and `value[3]` selects the attack message/type.

The parser transforms stored primary-stat applies before the object reaches
the game. Applies from `APPLY_STR` through `APPLY_CON`, and from `APPLY_AGI`
through `APPLY_LUCK`, become `(stored_value * 45) / 10` using integer
arithmetic. Consequently, stored `APPLY_CON +5` loads as internal modifier
`+22`, and stored `APPLY_CON +4` loads as `+18`. Both values are shown below.

The artifact overlay is a separate affect source. Its numeric bonuses are
multiplied by artifact level, from 1 through 5. Its resistance percentages
and proc chances are not multiplied by level.

### 3.2 HomelandMUD

Relevant sources:

- artifact membership:
  `EXAMPLE/HomelandMUD/src/act.informative.c:889-905`;
- object parser: `EXAMPLE/HomelandMUD/src/db.c:1352-1502`;
- object, wear, extra, affect, and apply constants:
  `EXAMPLE/HomelandMUD/src/structs.h:438-530,670-806`;
- display-name tables:
  `EXAMPLE/HomelandMUD/src/constants.c:294-399,617-699`; and
- special-procedure assignments:
  `EXAMPLE/HomelandMUD/src/spec_assign.c:766-835`.

Homeland records use these numeric groups:

```text
type extra_flags wear_flags
value[0] value[1] value[2] value[3]
weight cost cost_per_day race_restriction_mask
```

`A` records are numeric object applies. `C`, `D`, and `F` records grant
permanent `AFF_*`, `AFF2_*`, and `AFF3_*` states while worn. Homeland has no
artifact-level stat overlay, so its decoded prototype is its complete static
numeric package. Special procedures still add behavior.

## 4. RealmsOfLuminari: stored prototype stats

Names below are the stored short descriptions with color codes removed.

| VNUM | Stored object | Type and wear | Values or damage | Weight | Cost | Durability | Stored object applies |
| --- | --- | --- | --- | ---: | ---: | ---: | --- |
| 1007 | Mystical warhammer of the barbarian kings (Kelrarin) | Weapon; take, wield; two-handed flag | proc 0; 8d4 crush, avg. 20 | 10 | 525872 | 1 | hitroll +20, damroll +8 |
| 1008 | Tiamat's poison stinger | Weapon; take, wield | proc 0; 5d4 pierce, avg. 12.5 | 1 | 20000 | 1 | hitroll +5, damroll +5 |
| 1009 | Mystical warhammer of the barbarian kings (Kelrarin) | Weapon; take, wield; two-handed flag | proc 0; 8d4 crush, avg. 20 | 1 | 525872 | 1 | hitroll +30, HP +50 |
| 1042 | Fade, bringer of Revenge | Weapon; take, wield | proc 0; 5d4 pierce, avg. 12.5 | 1 | 100000 | 1 | hitroll +15, HP +75 |
| 1043 | Trorxek, living staff of the ancient oaks | **Treasure**; take, hold | raw values `0 0 0 0` | 1 | 100000 | 1 | CON +5 stored / +22 loaded, HP +75 |
| 1044 | Amaukekel, holy Rod of Resurrection | Weapon; take, **hold** | proc 0; 5d4 pierce, avg. 12.5 | 1 | 100000 | 1 | CON +5 stored / +22 loaded, HP +75 |
| 1045 | Raven earring with fiery eyes of vigilance | Armor; take, earring | AC 0, warmth 0, prestige 0, proc 0 | 1 | 100000000 | 1 | CON +4 stored / +18 loaded, hitroll +2 |
| 1046 | War Horn of Henekar | Instrument; take, hold | raw values `299 25 0 45` | 1 | 1 | 1 | HP +100, save vs. spell -15 |
| 1048 | Ancient war axe of Kelrom | Weapon; take, wield; two-handed flag | proc 0; 8d4 slash, avg. 20 | 1 | 525872 | 1 | hitroll +8, damroll +8 |
| 1050 | Doombringer | Weapon; take, wield; two-handed flag | proc 0; 8d5 slash, avg. 24 | 1 | 525872 | 1 | hitroll +8, damroll +8 |
| 5343 | Mighty battleaxe of the barbarian warlords (Gesen assignment) | Weapon; take, wield; two-handed flag | proc 0; 8d4 slash, avg. 20 | 20 | 525872 | 1 | hitroll +5, damroll +5 |
| 19730 | Avernus, life stealer of Arex the Great | Weapon; take, wield; two-handed flag | proc 0; 8d4 slash, avg. 20 | 30 | 5000000 | 2 | hitroll +8, damroll +8 |

Prototype locations:

| VNUM | Source record |
| --- | --- |
| 1007 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:132` |
| 1008 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:154` |
| 1009 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:172` |
| 1042 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:539` |
| 1043 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:560` |
| 1044 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:584` |
| 1045 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:609` |
| 1046 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:631` |
| 1048 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:675` |
| 1050 | `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj:714` |
| 5343 | `EXAMPLE/RealmsOfLuminari/areas/obj/waterdeep_harbor.obj:450` |
| 19730 | `EXAMPLE/RealmsOfLuminari/areas/obj/astral_main.obj:129` |

### 4.1 Extra flags, restrictions, permanent states, and procedures

The following names are the exact source constants represented by the stored
bitvectors.

#### VNUM 1007 - Kelrarin warhammer variant A

- Extra flags: `ITEM_MAGIC`, `ITEM_NODROP`, `ITEM_BLESS`, `ITEM_FLOAT`,
  `ITEM_NOBURN`, `ITEM_NOLOCATE`, `ITEM_NOSUMMON`, `ITEM_TWOHANDS`.
- Separate anti flags: `ITEM_ANTI_HUMAN`, `ITEM_ANTI_GREYELF`,
  `ITEM_ANTI_HALFELF`, `ITEM_ANTI_DWARF`, `ITEM_ANTI_HALFLING`,
  `ITEM_ANTI_GNOME`, `ITEM_ANTI_DUERGAR`, `ITEM_ANTI_DROWELF`,
  `ITEM_ANTI_TROLL`, `ITEM_ANTI_OGRE`, `ITEM_ANTI_ILLITHID`,
  `ITEM_ANTI_YUANTI`, and `ITEM_ANTI_LICH`. `ITEM_ANTI_BARBARIAN` is
  conspicuously absent.
- Permanent states: `AFF_FARSEE`, `AFF_DETECT_INVISIBLE`, `AFF_HASTE`,
  `AFF_SENSE_LIFE`, `AFF_AWARE`, `AFF_PROT_FIRE`, `AFF_DETECT_EVIL`,
  `AFF_DETECT_GOOD`, `AFF_DETECT_MAGIC`, `AFF_PROT_COLD`,
  `AFF_PROT_LIGHTNING`, `AFF_PROT_GAS`, and `AFF_PROT_ACID`.
- Assigned procedure: `Kelrarin`.

#### VNUM 1008 - Tiamat's poison stinger

- Extra flags: `ITEM_NOBURN`, `ITEM_ANTI_CL`, `ITEM_ANTI_MU`.
- Separate anti flags: none.
- Permanent states: `AFF_FARSEE`, `AFF_DETECT_INVISIBLE`, `AFF_HASTE`,
  `AFF_SENSE_LIFE`, and `AFF_SNEAK`.
- Assigned procedure: `tiamat_stinger`.

#### VNUM 1009 - Kelrarin warhammer variant B

- Extra flags: `ITEM_NOSELL`, `ITEM_NODROP`, `ITEM_ANTI_EVIL`,
  `ITEM_ANTI_NEUTRAL`, `ITEM_SECRET`, `ITEM_FLOAT`, `ITEM_TWOHANDS`,
  `ITEM_ANTI_TH`, and `ITEM_ANTI_MU`. It does not have `ITEM_MAGIC`.
- Separate anti flags: the same race mask as VNUM 1007.
- Permanent states: `AFF_FARSEE`, `AFF_DETECT_INVISIBLE`, `AFF_HASTE`,
  `AFF_SENSE_LIFE`, `AFF_INFRAVISION`, and `AFF_PROT_FIRE`.
- Assigned procedure: `Kelrarin`.

#### VNUM 1042 - Fade

- Extra flags: `ITEM_NOSHOW`, `ITEM_MAGIC`, `ITEM_NODROP`, `ITEM_FLOAT`,
  `ITEM_NOBURN`, and `ITEM_ANTI_EVILRACE`.
- Separate anti flags: none.
- Permanent states: `AFF_DETECT_INVISIBLE`, `AFF_HASTE`, `AFF_SNEAK`,
  `AFF_PROT_FIRE`, `AFF_DETECT_EVIL`, and `AFF_DETECT_GOOD`.
- Assigned procedure: `Fade2`.

#### VNUM 1043 - Trorxek

- Extra flags: `ITEM_GLOW`, `ITEM_MAGIC`, `ITEM_NODROP`, `ITEM_BLESS`,
  `ITEM_FLOAT`, `ITEM_NOBURN`, `ITEM_NOLOCATE`, and `ITEM_NOSUMMON`.
- Separate anti flags: none.
- Permanent states: `AFF_DETECT_INVISIBLE`, `AFF_BARKSKIN`,
  `AFF_PROT_FIRE`, `AFF_DETECT_EVIL`, `AFF_DETECT_GOOD`, `AFF_PROT_COLD`,
  `AFF_PROT_LIGHTNING`, `AFF_PROT_GAS`, and `AFF_PROT_ACID`.
- Assigned procedure: `OakenDefender`.

#### VNUM 1044 - Amaukekel

- Extra flags: `ITEM_GLOW`, `ITEM_MAGIC`, `ITEM_NODROP`, `ITEM_BLESS`,
  `ITEM_FLOAT`, `ITEM_NOBURN`, `ITEM_NOLOCATE`, `ITEM_NOSUMMON`, and
  `ITEM_LIT`.
- Separate anti flags: none.
- Permanent states: `AFF_FARSEE`, `AFF_DETECT_INVISIBLE`,
  `AFF_INFRAVISION`, `AFF_PROT_FIRE`, `AFF_DETECT_EVIL`, `AFF_DETECT_GOOD`,
  `AFF_PROT_COLD`, `AFF_PROT_LIGHTNING`, `AFF_PROT_GAS`, and
  `AFF_PROT_ACID`.
- Assigned procedure: `Amaukekel`.

#### VNUM 1045 - raven earring

- Extra flags: `ITEM_NODROP`, `ITEM_NOBURN`, and `ITEM_NOLOCATE`.
- Separate anti flags: none.
- Permanent states: none.
- Assigned procedure: `NeverLooseItem`.

#### VNUM 1046 - Horn of Henekar

- Extra flags: `ITEM_BLESS`, `ITEM_FLOAT`, `ITEM_NOBURN`, and
  `ITEM_NOSUMMON`. It does not have `ITEM_MAGIC`.
- Separate anti flags: none.
- Permanent states: `AFF_FARSEE`, `AFF_DETECT_INVISIBLE`, `AFF_HASTE`,
  `AFF_SENSE_LIFE`, `AFF_FLY`, and `AFF_PROT_FIRE`.
- Assigned procedure: `HornOfHenekar`.

#### VNUM 1048 - Kelrom

- Extra flags: `ITEM_NOSELL`, `ITEM_MAGIC`, `ITEM_NODROP`, `ITEM_ANTI_EVIL`,
  `ITEM_ANTI_NEUTRAL`, `ITEM_SECRET`, `ITEM_FLOAT`, `ITEM_TWOHANDS`,
  `ITEM_ANTI_TH`, and `ITEM_ANTI_MU`.
- Separate anti flags: none.
- Permanent states: `AFF_FARSEE`, `AFF_DETECT_INVISIBLE`, `AFF_HASTE`,
  `AFF_SENSE_LIFE`, `AFF_INFRAVISION`, and `AFF_PROT_FIRE`.
- Assigned procedure: `Kelrom`.

#### VNUM 1050 - Doombringer

- Extra flags: `ITEM_NOSELL`, `ITEM_MAGIC`, `ITEM_NODROP`, `ITEM_ANTI_EVIL`,
  `ITEM_ANTI_NEUTRAL`, `ITEM_FLOAT`, `ITEM_TWOHANDS`, `ITEM_ANTI_TH`, and
  `ITEM_ANTI_MU`.
- Separate anti flags: none.
- Permanent states: `AFF_FARSEE`, `AFF_DETECT_INVISIBLE`, `AFF_HASTE`,
  `AFF_SENSE_LIFE`, `AFF_INFRAVISION`, and `AFF_PROT_FIRE`.
- Assigned procedure: `Doombringer`.

#### VNUM 5343 - Gesen assignment

- Extra flags: `ITEM_NOSELL`, `ITEM_NODROP`, `ITEM_ANTI_EVIL`,
  `ITEM_SECRET`, `ITEM_FLOAT`, `ITEM_TWOHANDS`, `ITEM_ANTI_TH`, and
  `ITEM_ANTI_MU`. It does not have `ITEM_MAGIC`.
- Separate anti flags: the same race mask as VNUM 1007.
- Permanent states: none.
- Assigned procedure: `Gesen`.
- Data mismatch: the stored extra description says the name carved into the
  axe is `Kelrarin`, while the assignment table and procedure call it Gesen.

#### VNUM 19730 - Avernus

- Extra flags: `ITEM_MAGIC`, `ITEM_FLOAT`, and `ITEM_TWOHANDS`.
- Separate anti flags: none.
- Permanent states: `AFF_FARSEE`, `AFF_DETECT_INVISIBLE`, `AFF_HASTE`,
  `AFF_SENSE_LIFE`, and `AFF_PROT_FIRE`.
- Assigned procedure: `New_Avernus`.

### 4.2 Realms artifact overlay and effective level-1 totals

Only six of the 12 registry rows receive nonzero artifact-template data in
`initializeArtifacts()`. All bundled registry rows are level 1 and binding
type 0. The table below combines the level-1 overlay with the loaded static
object applies; permanent states from section 4.1 also remain active.

| VNUM | Overlay base per artifact level | Fixed overlay | Ability | Effective numeric package at bundled level 1 |
| --- | --- | --- | --- | --- |
| 1007 | STR +2, CON +1, hitroll +3, damroll +3 | 15% generic proc | `soulstrike`, 300 sec; code charges 50 mana | STR +2, CON +1, hitroll +23, damroll +11 |
| 1008 | STR +1, DEX +3, hitroll +5, move +50 | 10% physical resistance; 18% generic proc | none | STR +1, DEX +3, hitroll +10, damroll +5, move +50, 10% physical resistance |
| 1009 | STR +2, CON +1, hitroll +3, damroll +3 | 15% generic proc | `soulstrike`, 300 sec; code charges 50 mana | STR +2, CON +1, hitroll +33, damroll +3, HP +50 |
| 1042 | none | none | none | hitroll +15, HP +75 |
| 1043 | none | none | none | internal CON +22, HP +75 |
| 1044 | INT +2, WIS +2, AC -2, mana +50 | none | `divineward`, 600 sec; code charges 100 mana | INT +2, WIS +2, internal CON +22, AC -2, HP +75, mana +50 |
| 1045 | none | none | none | internal CON +18, hitroll +2 |
| 1046 | INT +2, WIS +1, CHA +1, mana +75 | 15% magical resistance | none | INT +2, WIS +1, CHA +1, HP +100, mana +75, save vs. spell -15, 15% magical resistance |
| 1048 | none | none | none | hitroll +8, damroll +8 |
| 1050 | STR +3, DEX +1, hitroll +4, damroll +5, HP +30 | 20% generic proc | `doomblast`, 180 sec; code charges 75 mana | STR +3, DEX +1, hitroll +12, damroll +13, HP +30 |
| 5343 | none | none | none | hitroll +5, damroll +5 |
| 19730 | none | none | none | hitroll +8, damroll +8 |

The overlay's `ability_cost` field remains zero for every artifact. The three
real resource costs shown above are hard-coded in the ability functions, so
the displayed metadata and executed costs do not share one source of truth.

### 4.3 Raw Realms tuples

These are the exact numeric object records, normalized only by removing
trailing whitespace. `aff1` and `aff2` are optional permanent-affect words.

```text
VNUM  header(type extra wear [anti])          values             physical(weight cost durability [aff1] [aff2])       A records
1007  5 4383168 8193 1069481984              0 8 4 6            10 525872 1 805306428 12508                           18 20; 19 8
1008  5 335560704 8193                        0 5 4 11           1 20000 1 524348                                    18 5; 19 5
1009  5 406862984 8193 1069481984             0 8 4 6            1 525872 1 570425404                                 18 30; 13 50
1042  5 16801986 8193                         0 5 4 11           1 100000 1 537395224 12                              18 15; 13 75
1043  8 188865 16385                          0 0 0 0            1 100000 1 553648136 12492                           5 5; 13 75
1044  5 451009 16385                          0 5 4 11           1 100000 1 570425356 12492                           5 5; 13 75
1045  9 49280 524289                          0 0 0 0            1 100000000 1                                       5 4; 18 2
1046  32 155904 16385                         299 25 0 45        1 1 1 671088700                                     13 100; 24 -15
1048  5 406863048 8193                        0 8 4 3            1 525872 1 570425404                                 18 8; 19 8
1050  5 406858952 8193                        0 8 5 3            1 525872 1 570425404                                 18 8; 19 8
5343  5 406860936 8193 1069481984             0 8 4 3            20 525872 1                                          18 5; 19 5
19730 5 4202560 8193                          0 8 4 3            30 5000000 2 536870972                               18 8; 19 8
```

## 5. HomelandMUD: stored prototype stats

Homeland has no level-scaled artifact overlay. These are therefore the full
static numeric object packages, before behavior from the assigned procedure.

| VNUM | Stored object | Type and wear | Values or damage | Weight | Cost | Cost/day | Race mask | Object applies |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- |
| 1199 | Vengeance | Weapon; take, wield | value[0] 0; 2d8 slash, avg. 9 | 6 | 10000000 | 0 | 0 | hitroll +4, damroll +4 |
| 1850 | Earthcrier | Weapon; take, two-handed wield | value[0] 0; 8d4 bludgeon, avg. 20 | 15 | 1 | 0 | 0 | hitroll +8, damroll +6 |
| 17022 | Wyrmfang, the Spear of Dragons | Weapon; take, two-handed wield | value[0] 8; 8d4 pierce, avg. 20 | 16 | 12000 | 0 | 0 | hitroll +8, damroll +8 |
| 39250 | Courage | Weapon; take, wield | value[0] 0; 3d6 crush, avg. 10.5 | 1 | 0 | 0 | 0 | HP +35, hitroll +8 |
| 43600 | Icedge, the Dagger of Cold | Weapon; take, wield | value[0] 0; 4d7 pierce, avg. 16 | 2 | 10000 | 0 | 0 | hitroll +6, damroll +6, magic resistance +2 |
| 96081 | Twilight, the Sword of Destruction | Weapon; take, two-handed wield | value[0] 0; 8d4 slash, avg. 20 | 15 | 1000000 | 0 | 0 | hitroll +8, damroll +8 |

Prototype locations:

| VNUM | Source record |
| --- | --- |
| 1199 | `EXAMPLE/HomelandMUD/lib/world/obj/10.obj:715` |
| 1850 | `EXAMPLE/HomelandMUD/lib/world/obj/18.obj:36` |
| 17022 | `EXAMPLE/HomelandMUD/lib/world/obj/170.obj:400` |
| 39250 | `EXAMPLE/HomelandMUD/lib/world/obj/392.obj:25` |
| 43600 | `EXAMPLE/HomelandMUD/lib/world/obj/436.obj:1` |
| 96081 | `EXAMPLE/HomelandMUD/lib/world/obj/960.obj:1245` |

### 5.1 Extra flags, permanent states, and procedures

#### VNUM 1199 - Vengeance

- Extra flags: `ITEM_HUM`, `ITEM_FLOAT`, `ITEM_MAGIC`, `ITEM_ANTI_EVIL`,
  `ITEM_ANTI_NEUTRAL`, `ITEM_ANTI_MAGE`, `ITEM_ANTI_CLERIC`,
  `ITEM_ANTI_ROGUE`, `ITEM_ANTI_WARRIOR`, `ITEM_ANTI_MONK`,
  `ITEM_ANTI_RANGER`, `ITEM_ANTI_BARD`, `ITEM_ANTI_DRUID`,
  `ITEM_NOLOCATE`, and `ITEM_NOBURN`.
- Permanent states: `AFF_PROTECT_EVIL` and `AFF2_MINOR_GLOBE`.
- Assigned procedure: `vengeance`.
- Data note: its prose calls it two-handed, but the wear mask is ordinary
  `ITEM_WEAR_WIELD`, not `ITEM_WEAR_WIELD_2H`.

#### VNUM 1850 - Earthcrier

- Extra flags: `ITEM_FLOAT`, `ITEM_MAGIC`, `ITEM_NODROP`, `ITEM_ANTI_GOOD`,
  `ITEM_ANTI_MAGE`, `ITEM_ANTI_ROGUE`, `ITEM_NOSELL`, `ITEM_ANTI_MONK`,
  `ITEM_ANTI_RANGER`, `ITEM_ANTI_BARD`, `ITEM_ANTI_DRUID`,
  `ITEM_NOLOCATE`, `ITEM_NOBURN`, and `ITEM_AUTOPROC`.
- Permanent states: none.
- Assigned procedure: `skullsmasher`.

#### VNUM 17022 - Wyrmfang

- Extra flags: `ITEM_HUM`, `ITEM_FLOAT`, `ITEM_MAGIC`, `ITEM_NODROP`,
  `ITEM_ANTI_EVIL`, `ITEM_ANTI_MAGE`, `ITEM_ANTI_CLERIC`,
  `ITEM_ANTI_ROGUE`, `ITEM_ANTI_MONK`, `ITEM_ANTI_BARD`,
  `ITEM_ANTI_DRUID`, and `ITEM_NOBURN`.
- Permanent states: `AFF_DETECT_INVIS`, `AFF_SENSE_LIFE`,
  `AFF_INFRAVISION`, `AFF_FARSEE`, `AFF_HASTE`, and `AFF3_DANGERSENSE`.
- Assigned procedure: none.

#### VNUM 39250 - Courage

- Extra flags: `ITEM_FLOAT`, `ITEM_MAGIC`, `ITEM_ANTI_MAGE`,
  `ITEM_ANTI_ROGUE`, `ITEM_ANTI_WARRIOR`, `ITEM_ANTI_MONK`,
  `ITEM_ANTI_RANGER`, `ITEM_ANTI_BARD`, `ITEM_ANTI_PALADIN`,
  `ITEM_MAGLIGHT`, and `ITEM_NOBURN`.
- Permanent states: `AFF_HASTE`, `AFF_PROTECT_LIGHTNING`, `AFF3_NOSLEEP`,
  and `AFF3_BRAVE`.
- Assigned procedure: `courage`.

#### VNUM 43600 - Icedge

- Extra flags: `ITEM_FLOAT`, `ITEM_MAGIC`, `ITEM_NODROP`, `ITEM_NOSELL`,
  `ITEM_MAGLIGHT`, `ITEM_NOLOCATE`, and `ITEM_NOBURN`.
- Permanent state: `AFF_PROTECT_COLD`.
- Assigned procedure: none.

#### VNUM 96081 - Twilight

- Extra flags: `ITEM_HUM`, `ITEM_MAGIC`, `ITEM_ANTI_MAGE`,
  `ITEM_ANTI_CLERIC`, `ITEM_ANTI_ROGUE`, and `ITEM_HIDDEN`.
- Permanent states: `AFF_SENSE_LIFE`, `AFF_INFRAVISION`, `AFF_FARSEE`, and
  `AFF_HASTE`.
- Assigned procedure: `twilight`.

### 5.2 Raw Homeland tuples

```text
VNUM  header(type extra wear)       values             physical(weight cost cost/day race-mask)   A records                 C/D/F words
1199  5 52362314 8193               0 2 8 3            6 10000000 0 0                              18 4; 19 4                C 4096; D 8
1850  5 186602184 524289            0 8 4 5            15 1 0 0                                    18 8; 19 6                none
17022 5 35288266 524289             8 8 4 11           16 12000 0 0                                19 8; 18 8                C 197672; F 4
39250 5 45011016 8193               0 3 6 6            1 0 0 0                                     13 35; 18 8               C 1073872896; F 1048578
43600 5 58785992 8193               0 4 7 11           2 10000 0 0                                 18 6; 19 6; 37 2          C 536870912
96081 5 4223042 524289              0 8 4 3            15 1000000 0 0                              18 8; 19 8                C 197664
```

## 6. Homeland registry entries with no object prototype

The following VNUMs are in `is_artifact()` and have procedure assignments,
but no object record exists anywhere under
`EXAMPLE/HomelandMUD/lib/world/obj/`:

| VNUM | Recoverable evidence | Object stats |
| --- | --- | --- |
| 501 | Procedure `xvim_artifact`; owner-file row; messages imply an Xvim avenger | Unknown: no type, dice, flags, weight, cost, applies, affects, or canonical short description survive |
| 513 | Procedure `halberd`; owner-file row | Unknown: no prototype fields survive |
| 599 | Procedure `tormblade`; owner-file row | Unknown: no prototype fields survive |

The procedure implementations preserve behavior shapes, but procedure code
cannot establish the missing prototypes' dice, static bonuses, flags, or
canonical names. Those fields must not be reconstructed by guesswork.

## 7. Material source-data findings

1. RealmsOfLuminari has 12 artifact prototypes, not merely the ten later
   adapted into LuminariMUD. Its source registry also includes a second
   Kelrarin hammer and the raven earring.
2. Realms static prototype applies and the artifact overlay stack. The raw
   object record alone is not the final equipped stat package for VNUMs 1007,
   1008, 1009, 1044, 1046, and 1050.
3. Six Realms registry rows receive no overlay bonuses at all, even though
   most still have substantial static stats and assigned special procedures.
4. Realms primary-stat `A` records use a parser conversion. Reporting the
   stored `+5 CON` as the in-memory value would be inaccurate; it loads as
   internal `+22`.
5. Several records contradict their presentation: Trorxek is type
   `ITEM_TREASURE`, Amaukekel is a weapon worn in `HOLD`, Vengeance describes
   itself as two-handed but has the ordinary wield mask, and Gesen's extra
   description names Kelrarin.
6. Homeland contributes six complete prototypes and three mechanics-only
   registry entries. There is no source basis for publishing object stats for
   the three missing prototypes.
7. The two engines use different object formats, flag spaces, class/race
   restrictions, stat scales, and runtime layers. Their raw numbers are
   historical evidence, not directly comparable balance values.
