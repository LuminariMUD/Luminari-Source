/**************************************************************************
 * @file spells.h
 * Constants and function prototypes for the spell system.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 **************************************************************************/
#ifndef _SPELLS_H_
#define _SPELLS_H_


#define DEFAULT_STAFF_LVL    12
#define DEFAULT_WAND_LVL     12

#define CAST_UNDEFINED	(-1)
#define CAST_SPELL       0
#define CAST_POTION      1
#define CAST_WAND        2
#define CAST_STAFF       3
#define CAST_SCROLL      4

#define MAG_DAMAGE        (1 << 0)
#define MAG_AFFECTS       (1 << 1)
#define MAG_UNAFFECTS     (1 << 2)
#define MAG_POINTS        (1 << 3)
#define MAG_ALTER_OBJS    (1 << 4)
#define MAG_GROUPS        (1 << 5)
#define MAG_MASSES        (1 << 6)
#define MAG_AREAS         (1 << 7)
#define MAG_SUMMONS       (1 << 8)
#define MAG_CREATIONS     (1 << 9)
#define MAG_MANUAL        (1 << 10)
#define MAG_ROOM          (1 << 11)


#define TYPE_UNDEFINED               (-1)
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */
#define SPELL_ARMOR                   1  //done
#define SPELL_TELEPORT                2  //done (no longer stock)
#define SPELL_BLESS                   3  //done
#define SPELL_BLINDNESS               4  //done
#define SPELL_BURNING_HANDS           5  //done
#define SPELL_CALL_LIGHTNING          6
#define SPELL_CHARM                   7  //done
#define SPELL_CHILL_TOUCH             8  //done
#define SPELL_CLONE                   9  //done
#define SPELL_COLOR_SPRAY            10  //done
#define SPELL_CONTROL_WEATHER        11
#define SPELL_CREATE_FOOD            12  //done
#define SPELL_CREATE_WATER           13  //done
#define SPELL_CURE_BLIND             14  //done
#define SPELL_CURE_CRITIC            15  //done 
#define SPELL_CURE_LIGHT             16  //done
#define SPELL_CURSE                  17  //done
#define SPELL_DETECT_ALIGN           18  //done
#define SPELL_DETECT_INVIS           19  //done
#define SPELL_DETECT_MAGIC           20  //done
#define SPELL_DETECT_POISON          21  //done
#define SPELL_DISPEL_EVIL            22  //done
#define SPELL_EARTHQUAKE             23  //done
#define SPELL_ENCHANT_WEAPON         24  //done
#define SPELL_ENERGY_DRAIN           25  //done
#define SPELL_FIREBALL               26  //done
#define SPELL_HARM                   27  //done
#define SPELL_HEAL                   28  //done
#define SPELL_INVISIBLE              29  //done
#define SPELL_LIGHTNING_BOLT         30  //done
#define SPELL_LOCATE_OBJECT          31  //done
#define SPELL_MAGIC_MISSILE          32  //done
#define SPELL_POISON                 33  //done
#define SPELL_PROT_FROM_EVIL         34  //done
#define SPELL_REMOVE_CURSE           35  //done
#define SPELL_SANCTUARY              36  //done
#define SPELL_SHOCKING_GRASP         37  //done
#define SPELL_SLEEP                  38  //done
#define SPELL_STRENGTH               39  //done
#define SPELL_SUMMON                 40  //done
#define SPELL_VENTRILOQUATE          41
#define SPELL_WORD_OF_RECALL         42  //done
#define SPELL_REMOVE_POISON          43  //done
#define SPELL_SENSE_LIFE             44  //done
#define SPELL_ANIMATE_DEAD           45  //done
#define SPELL_DISPEL_GOOD            46  //done
#define SPELL_GROUP_ARMOR            47  //done
#define SPELL_GROUP_HEAL             48  //done
#define SPELL_GROUP_RECALL           49  //done
#define SPELL_INFRAVISION            50  //done
#define SPELL_WATERWALK              51  //done
#define SPELL_IDENTIFY               52  //done
#define SPELL_FLY                    53  //done
#define SPELL_BLUR                   54  //done
#define SPELL_MIRROR_IMAGE           55  //done
#define SPELL_STONESKIN              56  //done
#define SPELL_ENDURANCE              57  //done
#define SPELL_MUMMY_DUST             58  //done, epic
#define SPELL_DRAGON_KNIGHT          59  //done, epic
#define SPELL_GREATER_RUIN           60  //done, epic
#define SPELL_HELLBALL               61  //done, epic
#define SPELL_EPIC_MAGE_ARMOR        62  //done, epic
#define SPELL_EPIC_WARDING           63  //done, epic
#define SPELL_CAUSE_LIGHT_WOUNDS     64  //done
#define SPELL_CAUSE_MODERATE_WOUNDS  65  //done
#define SPELL_CAUSE_SERIOUS_WOUNDS   66  //done
#define SPELL_CAUSE_CRITICAL_WOUNDS  67  //done
#define SPELL_FLAME_STRIKE           68  //done
#define SPELL_DESTRUCTION            69  //done
#define SPELL_ICE_STORM              70  //done
#define SPELL_BALL_OF_LIGHTNING      71  //done
#define SPELL_MISSILE_STORM          72  //done
#define SPELL_CHAIN_LIGHTNING        73  //done
#define SPELL_METEOR_SWARM           74  //done
#define SPELL_PROT_FROM_GOOD         75  //done
#define SPELL_FIRE_BREATHE           76  //done, [not spell]
#define SPELL_POLYMORPH              77  //done
#define SPELL_ENDURE_ELEMENTS        78  //done
#define SPELL_EXPEDITIOUS_RETREAT    79  //done
#define SPELL_GREASE                 80  //done
#define SPELL_HORIZIKAULS_BOOM       81  //done
#define SPELL_ICE_DAGGER             82  //done
#define SPELL_IRON_GUTS              83  //done
#define SPELL_MAGE_ARMOR             84  //done
#define SPELL_NEGATIVE_ENERGY_RAY    85  //done
#define SPELL_RAY_OF_ENFEEBLEMENT    86  //done
#define SPELL_SCARE                  87  //done
#define SPELL_SHELGARNS_BLADE        88  //done
#define SPELL_SHIELD                 89  //done
#define SPELL_SUMMON_CREATURE_1      90  //done
#define SPELL_TRUE_STRIKE            91  //done
#define SPELL_WALL_OF_FOG            92  //done
#define SPELL_DARKNESS               93  //done
#define SPELL_SUMMON_CREATURE_2      94  //done
#define SPELL_WEB                    95  //done
#define SPELL_ACID_ARROW             96  //done
#define SPELL_DAZE_MONSTER           97  //done
#define SPELL_HIDEOUS_LAUGHTER       98  //done
#define SPELL_TOUCH_OF_IDIOCY        99  //done
#define SPELL_CONTINUAL_FLAME        100  //done
#define SPELL_SCORCHING_RAY          101  //done
#define SPELL_DEAFNESS               102  //done
#define SPELL_FALSE_LIFE             103  //done
#define SPELL_GRACE                  104  //done
#define SPELL_RESIST_ENERGY		105  //done
#define SPELL_ENERGY_SPHERE		106  //done
#define SPELL_WATER_BREATHE        107  //done
#define SPELL_PHANTOM_STEED        108  //done
#define SPELL_STINKING_CLOUD       109  //done
#define SPELL_SUMMON_CREATURE_3    110  //done
#define SPELL_HALT_UNDEAD          111  //done
#define SPELL_HEROISM              112  //done
#define SPELL_VAMPIRIC_TOUCH       113  //done
#define SPELL_HOLD_PERSON          114  //done
#define SPELL_DEEP_SLUMBER         115  //done
#define SPELL_INVISIBILITY_SPHERE  116  //done
#define SPELL_DAYLIGHT             117  //done
#define SPELL_CLAIRVOYANCE         118  //done
#define SPELL_NON_DETECTION        119  //done
#define SPELL_HASTE                120  //done
#define SPELL_SLOW                 121  //done
#define SPELL_DISPEL_MAGIC         122  //done
#define SPELL_CIRCLE_A_EVIL        123  //done
#define SPELL_CIRCLE_A_GOOD        124  //done
#define SPELL_CUNNING              125  //done
#define SPELL_WISDOM               126  //done
#define SPELL_CHARISMA             127  //done
#define SPELL_STENCH               128  //done - stinking cloud proc
#define SPELL_ACID_SPLASH          129    // cantrip
#define SPELL_RAY_OF_FROST         130    // cantrip
#define SPELL_WIZARD_EYE           131  //done
#define SPELL_FIRE_SHIELD          132  //done
#define SPELL_COLD_SHIELD          133  //done
#define SPELL_BILLOWING_CLOUD      134  //done
#define SPELL_SUMMON_CREATURE_4    135  //done
#define SPELL_GREATER_INVIS        136  //done
#define SPELL_RAINBOW_PATTERN      137  //done
#define SPELL_LOCATE_CREATURE      138  //done
#define SPELL_MINOR_GLOBE          139  //done
#define SPELL_ENLARGE_PERSON       140  //done
#define SPELL_SHRINK_PERSON        141  //done
#define SPELL_FSHIELD_DAM          142  //done, fire shield proc
#define SPELL_CSHIELD_DAM          143  //done, cold shield proc
#define SPELL_ASHIELD_DAM          144  //done, acid shield proc
#define SPELL_ACID_SHEATH          145  //done
#define SPELL_INTERPOSING_HAND     146  //done
#define SPELL_WALL_OF_FORCE        147  //done
#define SPELL_CLOUDKILL            148  //done
#define SPELL_SUMMON_CREATURE_5    149  //done
#define SPELL_WAVES_OF_FATIGUE     150  //done
#define SPELL_SYMBOL_OF_PAIN       151  //done
#define SPELL_DOMINATE_PERSON      152  //done
#define SPELL_FEEBLEMIND           153  //done
#define SPELL_NIGHTMARE            154  //done
#define SPELL_MIND_FOG             155  //done
#define SPELL_FAITHFUL_HOUND       156  //done
#define SPELL_DISMISSAL            157  //done
#define SPELL_CONE_OF_COLD         158  //done
#define SPELL_TELEKINESIS          159  //done
#define SPELL_FIREBRAND            160  //done
#define SPELL_DEATHCLOUD           161  //done - cloudkill proc
#define SPELL_FREEZING_SPHERE      162  //done
#define SPELL_ACID_FOG             163  //done
#define SPELL_SUMMON_CREATURE_6    164  //done
#define SPELL_TRANSFORMATION       165  //done
#define SPELL_EYEBITE              166  //done
#define SPELL_MASS_HASTE           167  //done
#define SPELL_GREATER_HEROISM      168  //done
#define SPELL_ANTI_MAGIC_FIELD     169  //done
#define SPELL_GREATER_MIRROR_IMAGE 170  //done
#define SPELL_TRUE_SEEING          171  //done
#define SPELL_GLOBE_OF_INVULN      172  //done
#define SPELL_GREATER_DISPELLING   173  //done
#define SPELL_GRASPING_HAND        174  //done
#define SPELL_SUMMON_CREATURE_7    175  //done
#define SPELL_POWER_WORD_BLIND     176  //done
#define SPELL_WAVES_OF_EXHAUSTION  177  //done
#define SPELL_MASS_HOLD_PERSON     178  //done
#define SPELL_MASS_FLY             179  //done
#define SPELL_DISPLACEMENT         180  //done
#define SPELL_PRISMATIC_SPRAY      181  //done
#define SPELL_POWER_WORD_STUN      182  //done
#define SPELL_PROTECT_FROM_SPELLS  183  //done
#define SPELL_THUNDERCLAP          184  //done
#define SPELL_SPELL_MANTLE         185  //done
#define SPELL_MASS_WISDOM          186  //done
#define SPELL_MASS_CHARISMA        187  //done
#define SPELL_CLENCHED_FIST        188  // done
#define SPELL_INCENDIARY_CLOUD     189  // done
#define SPELL_SUMMON_CREATURE_8    190  // done
#define SPELL_HORRID_WILTING       191  // done
#define SPELL_GREATER_ANIMATION    192  // done
#define SPELL_IRRESISTIBLE_DANCE   193  // done
#define SPELL_MASS_DOMINATION      194  // done
#define SPELL_SCINT_PATTERN        195  // done
#define SPELL_REFUGE               196  // done
#define SPELL_BANISH               197  // done
#define SPELL_SUNBURST             198  // done
#define SPELL_SPELL_TURNING        199  // done
#define SPELL_MIND_BLANK           200  // done
#define SPELL_IRONSKIN             201  // done
#define SPELL_MASS_CUNNING         202  //done
#define SPELL_BLADE_OF_DISASTER    203  //done
#define SPELL_SUMMON_CREATURE_9    204  //done
#define SPELL_GATE                 205  //done
#define SPELL_WAIL_OF_THE_BANSHEE  206  //done
#define SPELL_POWER_WORD_KILL      207  //done
#define SPELL_ENFEEBLEMENT         208  //done
#define SPELL_WEIRD                209  //done
#define SPELL_SHADOW_SHIELD        210  //done
#define SPELL_PRISMATIC_SPHERE     211  //done
#define SPELL_IMPLODE              212  //done
#define SPELL_TIMESTOP             213  //done
#define SPELL_GREATER_SPELL_MANTLE 214  //done
#define SPELL_MASS_ENHANCE         215  //done
#define SPELL_PORTAL               216  //done
#define SPELL_ACID                 217  //acid fog proc

#define SPELL_HOLY_SWORD           218  //paladin
#define SPELL_INCENDIARY           219  // incendiary cloud proc
/* some cleric spells */
#define SPELL_CURE_MODERATE        220  //done
#define SPELL_CURE_SERIOUS         221  //done
#define SPELL_REMOVE_FEAR          222  //done
#define SPELL_CURE_DEAFNESS        223  //done
#define SPELL_FAERIE_FOG           224  //done
#define SPELL_MASS_CURE_LIGHT      225  //done
#define SPELL_AID                  226  //done
#define SPELL_BRAVERY              227  //done
#define SPELL_MASS_CURE_MODERATE   228  //done
#define SPELL_REGENERATION         229  //done
#define SPELL_FREE_MOVEMENT        230  //done
#define SPELL_STRENGTHEN_BONE      231  //done
#define SPELL_MASS_CURE_SERIOUS    232  //done
#define SPELL_PRAYER               233  //done
#define SPELL_REMOVE_DISEASE       234  //done
#define SPELL_WORD_OF_FAITH        235  //done
#define SPELL_DIMENSIONAL_LOCK     236  //done
#define SPELL_SALVATION            237  //done
#define SPELL_SPRING_OF_LIFE       238  //done
#define SPELL_PLANE_SHIFT          239  //done
#define SPELL_STORM_OF_VENGEANCE   240  //done
#define SPELL_DEATH_SHIELD         241
#define SPELL_COMMAND              242
#define SPELL_AIR_WALKER           243
#define SPELL_GROUP_SUMMON         244  //done
#define SPELL_MASS_CURE_CRIT       245  //done
/* some druid spells */
#define SPELL_CHARM_ANIMAL              246
#define SPELL_FAERIE_FIRE               247
#define SPELL_GOODBERRY                 248
#define SPELL_JUMP                      249
#define SPELL_MAGIC_FANG                250
#define SPELL_MAGIC_STONE               251
#define SPELL_OBSCURING_MIST            252
#define SPELL_PRODUCE_FLAME             253
#define SPELL_SUMMON_NATURES_ALLY_1     254
#define SPELL_SUMMON_NATURES_ALLY_2     255
#define SPELL_SUMMON_NATURES_ALLY_3     256
#define SPELL_SUMMON_NATURES_ALLY_4     257
#define SPELL_SUMMON_NATURES_ALLY_5     258
#define SPELL_SUMMON_NATURES_ALLY_6     259
#define SPELL_SUMMON_NATURES_ALLY_7     260
#define SPELL_SUMMON_NATURES_ALLY_8     261
#define SPELL_SUMMON_NATURES_ALLY_9     262
#define SPELL_BARKSKIN                  263
#define SPELL_FLAME_BLADE               264
#define SPELL_FLAMING_SPHERE            265
#define SPELL_HOLD_ANIMAL               266
#define SPELL_CALL_LIGHTNING_STORM      267
#define SPELL_SUMMON_SWARM              268
#define SPELL_CONTAGION                 269
#define SPELL_GREATER_MAGIC_FANG        270
#define SPELL_UNUSED271                 271
#define SPELL_SPIKE_GROWTH              272
#define SPELL_BLIGHT                    273
#define SPELL_REINCARNATE               274
#define SPELL_UNUSED275                 275
#define SPELL_SPIKE_STONES              276
#define SPELL_BALEFUL_POLYMORPH         277
#define SPELL_DEATH_WARD                278
#define SPELL_HALLOW                    279
#define SPELL_INSECT_PLAGUE             280
#define SPELL_UNHALLOW                  281
#define SPELL_WALL_OF_FIRE              282
#define SPELL_WALL_OF_THORNS            283
#define SPELL_FIRE_SEEDS                284
#define SPELL_UNUSED285                 285 
#define SPELL_MASS_ENDURANCE            286 // *note mass enhance combines these 3
#define SPELL_MASS_STRENGTH             287 // *note mass enhance combines these 3
#define SPELL_MASS_GRACE                288 // *note mass enhance combines these 3
#define SPELL_SPELL_RESISTANCE          289
#define SPELL_SPELLSTAFF                290
#define SPELL_TRANSPORT_VIA_PLANTS      291
#define SPELL_CREEPING_DOOM             292
#define SPELL_FIRE_STORM                293
#define SPELL_GREATER_SCRYING           294
#define SPELL_SUNBEAM                   295
#define SPELL_ANIMAL_SHAPES             296
#define SPELL_CONTROL_PLANTS            297
#define SPELL_FINGER_OF_DEATH           298
#define SPELL_ELEMENTAL_SWARM           299
#define SPELL_REGENERATE                300
#define SPELL_SHAMBLER                  301
#define SPELL_SHAPECHANGE               302 // hey b, maybe use polymorph
/* some more cleric spells */
#define SPELL_BLADE_BARRIER             303  //done
#define SPELL_BLADES                    304  //blades (for blade barrier) (done)
#define SPELL_BATTLETIDE                305  //done
/* room events test spell */
#define SPELL_I_DARKNESS                306  //room event test spell
#define SPELL_DOOM                      307  // creeping doom damage proc
/** Total Number of defined spells (last spell define + 1) */
#define NUM_SPELLS                      308

/* Insert new spells here, up to MAX_SPELLS */
/* make sure this matches up with structs.h spellbook define */
#define MAX_SPELLS		               400

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB                  401  // implemented
#define SKILL_BASH                 	402  //implemented
#define SKILL_MUMMY_DUST                403  //implemented
#define SKILL_KICK                      404  //implemented
#define SKILL_WEAPON_SPECIALIST         405  //implemented
#define SKILL_WHIRLWIND                 406  //implemented
#define SKILL_RESCUE                    407  //implemented
#define SKILL_DRAGON_KNIGHT             408  //implemented
#define SKILL_LUCK_OF_HEROES            409  //implemented
#define SKILL_TRACK                     410  //implemented
#define SKILL_QUICK_CHANT               411  //implemented
#define SKILL_AMBIDEXTERITY             412  //implemented
#define SKILL_DIRTY_FIGHTING            413  //implemented
#define SKILL_DODGE                     414  //implemented
#define SKILL_IMPROVED_CRITICAL		415  //implemented
#define SKILL_MOBILITY                  416  //implemented
#define SKILL_SPRING_ATTACK             417  //implemented
#define SKILL_TOUGHNESS                 418  //implemented
#define SKILL_TWO_WEAPON_FIGHT		419  //implemented
#define SKILL_FINESSE                   420  //implemented
#define SKILL_ARMOR_SKIN                421  //implemented
#define SKILL_BLINDING_SPEED            422  //implemented
#define SKILL_DAMAGE_REDUC_1            423  //implemented
#define SKILL_DAMAGE_REDUC_2            424  //implemented
#define SKILL_DAMAGE_REDUC_3            425  //implemented
#define SKILL_EPIC_TOUGHNESS            426  //implemented
#define SKILL_OVERWHELMING_CRIT         427  //implemented
#define SKILL_SELF_CONCEAL_1            428  //implemented
#define SKILL_SELF_CONCEAL_2            429  //implemented
#define SKILL_SELF_CONCEAL_3            430  //implemented
#define SKILL_TRIP                      431  //implemented
#define SKILL_IMPROVED_WHIRL            432  //implemented
#define SKILL_CLEAVE                    433
#define SKILL_GREAT_CLEAVE              434
#define SKILL_SPELLPENETRATE            435  //implemented
#define SKILL_SPELLPENETRATE_2		436  //implemented
#define SKILL_PROWESS                   437  //implemented
#define SKILL_EPIC_PROWESS              438  //implemented
#define SKILL_EPIC_2_WEAPON             439  //implemented
#define SKILL_SPELLPENETRATE_3		440  //implemented
#define SKILL_SPELL_RESIST_1            441  //implemented
#define SKILL_SPELL_RESIST_2            442  //implemented
#define SKILL_SPELL_RESIST_3            443  //implemented
#define SKILL_SPELL_RESIST_4            444  //implemented
#define SKILL_SPELL_RESIST_5       	445  //implemented
#define SKILL_INITIATIVE                446  //implemented
#define SKILL_EPIC_CRIT                 447  //implemented
#define SKILL_IMPROVED_BASH             448  //implemented
#define SKILL_IMPROVED_TRIP             449  //implemented
#define SKILL_POWER_ATTACK              450  //implemented
#define SKILL_EXPERTISE                 451  //implemented
#define SKILL_GREATER_RUIN              452  //implemented
#define SKILL_HELLBALL                  453  //implemented
#define SKILL_EPIC_MAGE_ARMOR           454  //implemented
#define SKILL_EPIC_WARDING              455  //implemented
#define SKILL_RAGE            		456  //implemented
#define SKILL_PROF_MINIMAL         	457  //implemented
#define SKILL_PROF_BASIC                458  //implemented
#define SKILL_PROF_ADVANCED             459  //implemented
#define SKILL_PROF_MASTER               460  //implemented
#define SKILL_PROF_EXOTIC               461  //implemented
#define SKILL_PROF_LIGHT_A              462  //implemented
#define SKILL_PROF_MEDIUM_A             463  //implemented
#define SKILL_PROF_HEAVY_A              464  //implemented
#define SKILL_PROF_SHIELDS              465  //implemented
#define SKILL_PROF_T_SHIELDS            466  //implemented
#define SKILL_MURMUR                    467
#define SKILL_PROPAGANDA                468
#define SKILL_LOBBY                     469
#define SKILL_STUNNING_FIST             470  //implemented
/* initial crafting skills */
#define SKILL_MINING                    471  //implemented
#define SKILL_HUNTING                   472  //implemented
#define SKILL_FORESTING                 473  //implemented
#define SKILL_KNITTING                  474  //implemented
#define SKILL_CHEMISTRY                 475  //implemented
#define SKILL_ARMOR_SMITHING            476  //implemented
#define SKILL_WEAPON_SMITHING           477  //implemented
#define SKILL_JEWELRY_MAKING            478  //implemented
#define SKILL_LEATHER_WORKING           479  //implemented
#define SKILL_FAST_CRAFTER              480  //implemented
#define SKILL_BONE_ARMOR                481
#define SKILL_ELVEN_CRAFTING            482
#define SKILL_MASTERWORK_CRAFTING       483
#define SKILL_DRACONIC_CRAFTING         484
#define SKILL_DWARVEN_CRAFTING          485
/* finish batch crafting skills */
#define SKILL_LIGHTNING_REFLEXES        486  //implemented
#define SKILL_GREAT_FORTITUDE           487  //implemented
#define SKILL_IRON_WILL                 488  //implemented
#define SKILL_EPIC_REFLEXES             489  //implemented
#define SKILL_EPIC_FORTITUDE            490  //implemented
#define SKILL_EPIC_WILL                 491  //implemented
#define SKILL_SHIELD_SPECIALIST         492  //implemented
#define SKILL_USE_MAGIC                 493  //implemented
#define SKILL_EVASION                   494  //implemented
#define SKILL_IMP_EVASION               495  //implemented
#define SKILL_CRIP_STRIKE               496  //implemented
#define SKILL_SLIPPERY_MIND             497  //implemented
#define SKILL_DEFENSE_ROLL              498  //implemented
#define SKILL_GRACE                     499  //implemented
#define SKILL_DIVINE_HEALTH             500  //implemented
#define SKILL_LAY_ON_HANDS              501  //implemented
#define SKILL_COURAGE                   502  //implemented
#define SKILL_SMITE                     503  //implemented
#define SKILL_REMOVE_DISEASE            504  //implemented
#define SKILL_RECHARGE                  505  //implemented
#define SKILL_STEALTHY                  506  //implemented
#define SKILL_NATURE_STEP               507  //implemented
#define SKILL_FAVORED_ENEMY             508  //implemented
#define SKILL_DUAL_WEAPONS              509  //implemented
#define SKILL_ANIMAL_COMPANION          510  //implemented
#define SKILL_PALADIN_MOUNT             511  //implemented
#define SKILL_CALL_FAMILIAR             512  //implemented
#define SKILL_PERFORM                   513  //implemented
#define SKILL_SCRIBE                    514  //implemented
#define SKILL_TURN_UNDEAD               515  //implemented
#define SKILL_WILDSHAPE                 516
/* New skills may be added here up to MAX_SKILLS (600) */
#define NUM_SKILLS                      517

/* NON-PLAYER AND OBJECT SPELLS AND SKILLS: The practice levels for the spells
 * and skills below are _not_ recorded in the players file; therefore, the
 * intended use is for spells and skills associated with objects (such as
 * SPELL_IDENTIFY used with scrolls of identify) or non-players (such as NPC
 * only spells). */

/* To make an affect induced by dg_affect look correct on 'stat' we need to
 * define it with a 'spellname'. */
#define SPELL_DG_AFFECT              698

#define TOP_SPELL_DEFINE	            699
/* NEW NPC/OBJECT SPELLS can be inserted here up to 699 */

/* WEAPON ATTACK TYPES */
#define TYPE_HIT          700
#define TYPE_STING        701
#define TYPE_WHIP         702
#define TYPE_SLASH        703
#define TYPE_BITE         704
#define TYPE_BLUDGEON     705
#define TYPE_CRUSH        706
#define TYPE_POUND        707
#define TYPE_CLAW         708
#define TYPE_MAUL         709
#define TYPE_THRASH       710
#define TYPE_PIERCE       711
#define TYPE_BLAST        712
#define TYPE_PUNCH        713
#define TYPE_STAB         714
/** The total number of attack types */
#define NUM_ATTACK_TYPES  15


/* (stock)
 * hit - 0      blunt
 * sting - 1      piercing
 * whip - 2      blades (slashing)
 * slash - 3      blades (slashing)
 * bite - 4      piercing
 * bludgeon - 5      blunt
 * crush - 6      blunt
 * pound - 7      blunt
 * claw - 8      blades (slashing)
 * maul - 9      blades (slashing)
 * thrash - 10      blades (slashing)
 * pierce - 11      piercing
 * blast - 12      piercing
 * punch - 13      blunt
 * stab - 14      piercing
 */
/* weapon type macros */
#define IS_BLADE(obj)      ( GET_OBJ_VAL(obj, 3) == (TYPE_WHIP - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_SLASH - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_CLAW - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_MAUL - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_THRASH - TYPE_HIT) )
#define IS_PIERCE(obj)     ( GET_OBJ_VAL(obj, 3) == (TYPE_STING - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_BITE - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_PIERCE - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_BLAST - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_STAB - TYPE_HIT) )
#define IS_BLUNT(obj)      ( GET_OBJ_VAL(obj, 3) == (TYPE_HIT - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_BLUDGEON - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_CRUSH - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_POUND - TYPE_HIT) || \
                             GET_OBJ_VAL(obj, 3) == (TYPE_PUNCH - TYPE_HIT) )


/* other attack types */
#define TYPE_CSHIELD      796
#define TYPE_FSHIELD      797
#define TYPE_ASHIELD      798
#define TYPE_SUFFERING    799
/* new attack types can be added here - up to TYPE_SUFFERING */
#define MAX_TYPES		 800

/*---------------------------ABILITIES-------------------------------------*/

/* PLAYER ABILITIES -- Numbered from 1 to MAX_ABILITIES */
#define ABILITY_TUMBLE			1 // tumble
#define ABILITY_HIDE			2 // hide
#define ABILITY_SNEAK			3 // sneak
#define ABILITY_SPOT			4 // spot
#define ABILITY_LISTEN			5 // listen
#define ABILITY_TREAT_INJURY		6 // treat injuries
#define ABILITY_TAUNT			7 // taunt
#define ABILITY_CONCENTRATION		8 // concentration
#define ABILITY_SPELLCRAFT		9 // spellcraft
#define ABILITY_APPRAISE	          10 // appraise
#define ABILITY_DISCIPLINE		11 // discipline
#define ABILITY_PARRY			12 // parry
#define ABILITY_LORE			13 // lore
#define ABILITY_MOUNT			14 //mounts
#define ABILITY_RIDING			15 //mounts
#define ABILITY_TAME			16 //mounts
#define ABILITY_PICK_LOCK		17 //open locks
#define ABILITY_STEAL			18 //pick pockets

#define NUM_ABILITIES			19 /* Number of defined abilities */
/*	MAX_ABILITIES = 200 */
/*-------------------------------------------------------------------------*/


// ******** DAM_ *********
#define DAM_RESERVED_DBC	0	//reserve
#define DAM_FIRE		1
#define DAM_COLD		2
#define DAM_AIR		3
#define DAM_EARTH		4
#define DAM_ACID		5
#define DAM_HOLY		6
#define DAM_ELECTRIC	7
#define DAM_UNHOLY		8
#define DAM_SLICE		9
#define DAM_PUNCTURE	10
#define DAM_FORCE		11
#define DAM_SOUND		12
#define DAM_POISON		13
#define DAM_DISEASE		14
#define DAM_NEGATIVE	15
#define DAM_ILLUSION	16
#define DAM_MENTAL		17
#define DAM_LIGHT		18
#define DAM_ENERGY		19
#define DAM_WATER		20
/* ------------------------------*/
#define NUM_DAM_TYPES    21
/* =============================*/


/*********************************/
/******** Schools of Magic *******/
/*********************************/
#define NOSCHOOL       0	// non magical spells
#define ABJURATION     1
#define CONJURATION    2
#define DIVINATION     3
#define ENCHANTMENT    4
#define EVOCATION      5
#define ILLUSION       6
#define NECROMANCY     7
#define TRANSMUTATION  8

#define NUM_SCHOOLS    9
/*--------------------------------*/


/************************************/
/********** Saves *******************/
/************************************/
#define SAVING_FORT   0
#define SAVING_REFL   1
#define SAVING_WILL   2
#define SAVING_POISON 3  /* this is not really used -zusuk */
#define SAVING_DEATH  4  /* this is not really used -zusuk */
/*----------------------------------*/


/***
 **Possible Targets:
 **  TAR_IGNORE    : IGNORE TARGET.
 **  TAR_CHAR_ROOM : PC/NPC in room.
 **  TAR_CHAR_WORLD: PC/NPC in world.
 **  TAR_FIGHT_SELF: If fighting, and no argument, select tar_char as self.
 **  TAR_FIGHT_VICT: If fighting, and no argument, select tar_char as victim (fighting).
 **  TAR_SELF_ONLY : If no argument, select self, if argument check that it IS self.
 **  TAR_NOT_SELF  : Target is anyone else besides self.
 **  TAR_OBJ_INV   : Object in inventory.
 **  TAR_OBJ_ROOM  : Object in room.
 **  TAR_OBJ_WORLD : Object in world.
 **  TAR_OBJ_EQUIP : Object held.
 ***/
#define TAR_IGNORE      (1 << 0)
#define TAR_CHAR_ROOM   (1 << 1)
#define TAR_CHAR_WORLD  (1 << 2)
#define TAR_FIGHT_SELF  (1 << 3)
#define TAR_FIGHT_VICT  (1 << 4)
#define TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF    (1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     (1 << 7)
#define TAR_OBJ_ROOM    (1 << 8)
#define TAR_OBJ_WORLD   (1 << 9)
#define TAR_OBJ_EQUIP   (1 << 10)

struct spell_info_type {
  byte min_position; /* Position for caster	 */
  int mana_min; /* Min amount of mana used by a spell (highest lev) */
  int mana_max; /* Max amount of mana used by a spell (lowest lev) */
  int mana_change; /* Change in mana used by spell from lev to lev */

  int min_level[NUM_CLASSES];
  int routines;
  byte violent;
  int targets; /* See below for use with TAR_XXX  */
  const char *name; /* Input size not limited. Originates from string constants. */
  const char *wear_off_msg; /* Input size not limited. Originates from string constants. */
  int time; /* casting time */
  int memtime; /* mem time */
  int schoolOfMagic; // school of magic
};

/* Possible Targets:
   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self. */

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4


/* manual spell header info */
#define ASPELL(spellname) \
void	spellname(int level, struct char_data *ch, \
		  struct char_data *victim, struct obj_data *obj)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict);

ASPELL(spell_acid_arrow);
ASPELL(spell_banish);
ASPELL(spell_charm);
ASPELL(spell_charm_animal);
ASPELL(spell_clairvoyance);
ASPELL(spell_cloudkill);
ASPELL(spell_control_weather);
ASPELL(spell_create_water);
ASPELL(spell_creeping_doom);
ASPELL(spell_detect_poison);
ASPELL(spell_dismissal);
ASPELL(spell_dispel_magic);
ASPELL(spell_dominate_person);
ASPELL(spell_enchant_weapon);
ASPELL(spell_greater_dispelling);
ASPELL(spell_group_summon);
ASPELL(spell_identify);
ASPELL(spell_implode);
ASPELL(spell_incendiary_cloud);
ASPELL(spell_information);
ASPELL(spell_locate_creature);
ASPELL(spell_locate_object);
ASPELL(spell_mass_domination);
ASPELL(spell_plane_shift);
ASPELL(spell_polymorph);
ASPELL(spell_prismatic_sphere);
ASPELL(spell_recall);
ASPELL(spell_refuge);
ASPELL(spell_salvation);
ASPELL(spell_spellstaff);
ASPELL(spell_storm_of_vengeance);
ASPELL(spell_summon);
ASPELL(spell_teleport);
ASPELL(spell_transport_via_plants);
ASPELL(spell_wall_of_force);
ASPELL(spell_wizard_eye);

/* basic magic calling functions */
int find_skill_num(char *name);
int find_ability_num(char *name);
int mag_damage(int level, struct char_data *ch, struct char_data *victim,
        struct obj_data *obj, int spellnum, int savetype);
void mag_affects(int level, struct char_data *ch, struct char_data *victim,
        struct obj_data *obj, int spellnum, int savetype);
void mag_groups(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum, int savetype);
void mag_masses(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum, int savetype);
void mag_areas(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum, int savetype);
void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum, int savetype);
void mag_points(int level, struct char_data *ch, struct char_data *victim,
        struct obj_data *obj, int spellnum, int savetype);
void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
        struct obj_data *obj, int spellnum, int type);
void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum, int type);
void mag_creations(int level, struct char_data *ch, struct char_data *vict,
        struct obj_data *obj, int spellnum);
void mag_room(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum);
int call_magic(struct char_data *caster, struct char_data *cvict,
        struct obj_data *ovict, int spellnum, int level, int casttype);
void mag_objectmagic(struct char_data *ch, struct obj_data *obj,
        char *argument);
int cast_spell(struct char_data *ch, struct char_data *tch,
        struct obj_data *tobj, int spellnum);


/* other prototypes */
void spell_level(int spell, int chclass, int level);
void init_spell_levels(void);
const char *skill_name(int num);
int valid_mortal_tele_dest(struct char_data *ch, room_rnum dest, bool is_tele);



/* From magic.c */
int compute_mag_saves(struct char_data *vict,
        int type, int modifier);
int mag_savingthrow(struct char_data *ch, struct char_data *vict,
        int type, int modifier);
void affect_update(void);
int mag_resistance(struct char_data *ch, struct char_data *vict, int modifier);
int compute_spell_res(struct char_data *ch, struct char_data *vict, int mod);
int aoeOK(struct char_data *ch, struct char_data *tch, int spellnum);


/**************/
/* memorize.c */
/* spellbook functions */
void display_scroll(struct char_data *ch, struct obj_data *obj);
void display_spells(struct char_data *ch, struct obj_data *obj);
bool spell_in_book(struct obj_data *obj, int spellnum);
int spell_in_scroll(struct obj_data *obj, int spellnum);
bool spellbook_ok(struct char_data *ch, int spellnum, int class, bool check_scroll);
/* spellbook commands */
ACMD(do_scribe);
/* memorize related functions */
void init_spell_slots(struct char_data *ch);
int spellCircle(int class, int spellnum);
int comp_slots(struct char_data *ch, int circle, int class);
void addSpellMemming(struct char_data *ch, int spellnum, int time, int mode);
void resetMemtimes(struct char_data *ch, int class);
void addSpellMemmed(struct char_data *ch, int spellnum, int class);
void removeSpellMemming(struct char_data *ch, int spellnum, int class);
int forgetSpell(struct char_data *ch, int spellnum, int mode);
int numSpells(struct char_data *ch, int circle, int class);
bool sorcKnown(struct char_data *ch, int spellnum, int class);
bool hasSpell(struct char_data *ch, int spellnum);
int getCircle(struct char_data *ch, int class);
int sorcererKnown[LVL_IMPL + 1][10];
int bardKnown[LVL_IMPL + 1][10];
int count_sorc_known(struct char_data *ch, int circle, int class);
void sorc_extract_known(struct char_data *ch, int spellnum, int class);
int sorc_add_known(struct char_data *ch, int spellnum, int class);




/* from spell_parser.c */
ACMD(do_abort);
ACMD(do_cast);
void unused_spell(int spl);
void mag_assign_spells(void);
void resetCastingData(struct char_data *ch);
int lowest_spell_level(int spellnum);



/* Global variables exported */
#ifndef __SPELL_PARSER_C__

extern struct spell_info_type spell_info[];
extern char cast_arg2[];
extern const char *unused_spellname;

#endif /* __SPELL_PARSER_C__ */


#endif /* _SPELLS_H_ */
