/**************************************************************************
 * @file spells.h                                      LuminariMUD
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

/* summon fail message */
#define SUMMON_FAIL "You failed.\r\n"

/* object vnums for some spells */
#define PRISMATIC_SPHERE 90
#define WIZARD_EYE 45

/* Metamagic Defines*/
#define METAMAGIC_NONE 0
#define METAMAGIC_QUICKEN (1 << 0)
#define METAMAGIC_MAXIMIZE (1 << 1)
#define METAMAGIC_HEIGHTEN (1 << 2)
#define METAMAGIC_ARCANE_ADEPT (1 << 3)

/* this variable is used for the system for 'spamming' ironskin -zusuk */
#define WARD_THRESHOLD 171

/* renamed 0-values to help clarify context in code */
#define NO_DICEROLL 0
#define NO_MOD 0

#define DEFAULT_STAFF_LVL 12
#define DEFAULT_WAND_LVL 12

/* smite types */
#define SMITE_TYPE_UNDEFINED 0
#define SMITE_TYPE_EVIL 1
#define SMITE_TYPE_GOOD 2
#define SMITE_TYPE_DESTRUCTION 3

/* spell types */
#define SPELL_TYPE_SPELL 0
#define SPELL_TYPE_POTION 1
#define SPELL_TYPE_WAND 2
#define SPELL_TYPE_STAFF 3
#define SPELL_TYPE_SCROLL 4

/* cast types */
#define CAST_UNDEFINED (-1)
#define CAST_SPELL 0
#define CAST_POTION 1
#define CAST_WAND 2
#define CAST_STAFF 3
#define CAST_SCROLL 4
#define CAST_INNATE 5        /* For innate abilities */
#define CAST_WEAPON_POISON 6 /* For apply poison */
#define CAST_WEAPON_SPELL 7  /* For casting weapons */
#define CAST_TRAP 8
#define CAST_FOOD_DRINK 9
#define CAST_BOMB 10
#define CAST_CRUELTY 11
#define CAST_WALL 12

#define MAG_DAMAGE (1 << 0)
#define MAG_AFFECTS (1 << 1)
#define MAG_UNAFFECTS (1 << 2)
#define MAG_POINTS (1 << 3)
#define MAG_ALTER_OBJS (1 << 4)
#define MAG_GROUPS (1 << 5)
#define MAG_MASSES (1 << 6)
#define MAG_AREAS (1 << 7)
#define MAG_SUMMONS (1 << 8)
#define MAG_CREATIONS (1 << 9)
#define MAG_MANUAL (1 << 10)
#define MAG_ROOM (1 << 11)
#define MAG_LOOPS (1 << 12)

#define NO_SUBSCHOOL 0
#define SUBSCHOOL_CALLING 1
#define SUBSCHOOL_CREATION 2
#define SUBSCHOOL_HEALING 3
#define SUBSCHOOL_SUMMONING 4
#define SUBSCHOOL_TELEPORTATION 5
#define SUBSCHOOL_CHARM 6
#define SUBSCHOOL_COMPULSION 7
#define SUBSCHOOL_FIGMENT 8
#define SUBSCHOOL_GLAMER 9
#define SUBSCHOOL_PATTERN 10
#define SUBSCHOOL_PHANTASM 11
#define SUBSCHOOL_SHADOW 12
#define SUBSCHOOL_POLYMORPH 13
/*------*/
#define NUM_SUBSCHOOLS 14
/************************/

#define COMPONENT_VERBAL (1 << 0)
#define COMPONENT_SOMATIC (1 << 1)
#define COMPONENT_MATERIAL (1 << 2)
#define COMPONENT_FOCUS (1 << 3)
#define COMPONENT_DIVINE_FOCUS (1 << 4)
/**********************************/

#define TYPE_UNDEFINED (-1)
#define SPELL_RESERVED_DBC 0 /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */
#define SPELL_ARMOR 1 // done
#define SPELL_SHIELD_OF_FAITH SPELL_ARMOR
#define SPELL_TELEPORT 2      // done (no longer stock)
#define SPELL_BLESS 3         // done
#define SPELL_BLINDNESS 4     // done
#define SPELL_BURNING_HANDS 5 // done
#define SPELL_CALL_LIGHTNING 6
#define SPELL_CHARM 7        // done
#define SPELL_CHILL_TOUCH 8  // done
#define SPELL_CLONE 9        // done
#define SPELL_COLOR_SPRAY 10 // done
#define SPELL_CONTROL_WEATHER 11
#define SPELL_CREATE_FOOD 12    // done
#define SPELL_CREATE_WATER 13   // done
#define SPELL_CURE_BLIND 14     // done
#define SPELL_CURE_CRITIC 15    // done
#define SPELL_CURE_LIGHT 16     // done
#define SPELL_CURSE 17          // done
#define SPELL_DETECT_ALIGN 18   // done
#define SPELL_DETECT_INVIS 19   // done
#define SPELL_DETECT_MAGIC 20   // done
#define SPELL_DETECT_POISON 21  // done
#define SPELL_DISPEL_EVIL 22    // done
#define SPELL_EARTHQUAKE 23     // done
#define SPELL_ENCHANT_ITEM 24   // done
#define SPELL_ENERGY_DRAIN 25   // done
#define SPELL_FIREBALL 26       // done
#define SPELL_HARM 27           // done
#define SPELL_HEAL 28           // done
#define SPELL_INVISIBLE 29      // done
#define SPELL_LIGHTNING_BOLT 30 // done
#define SPELL_LOCATE_OBJECT 31  // done
#define SPELL_MAGIC_MISSILE 32  // done
#define SPELL_POISON 33         // done
#define SPELL_PROT_FROM_EVIL 34 // done
#define SPELL_REMOVE_CURSE 35   // done
#define SPELL_SANCTUARY 36      // done
#define SPELL_SHOCKING_GRASP 37 // done
#define SPELL_SLEEP 38          // done
#define SPELL_STRENGTH 39       // done
#define SPELL_SUMMON 40         // done
#define SPELL_VENTRILOQUATE 41
#define SPELL_WORD_OF_RECALL 42 // done
#define SPELL_REMOVE_POISON 43  // done
#define SPELL_SENSE_LIFE 44     // done
#define SPELL_ANIMATE_DEAD 45   // done
#define SPELL_DISPEL_GOOD 46    // done
#define SPELL_GROUP_ARMOR 47    // done
#define SPELL_GROUP_SHIELD_OF_FAITH SPELL_GROUP_ARMOR
#define SPELL_GROUP_HEAL 48            // done
#define SPELL_GROUP_RECALL 49          // done
#define SPELL_INFRAVISION 50           // done
#define SPELL_WATERWALK 51             // done
#define SPELL_IDENTIFY 52              // done
#define SPELL_FLY 53                   // done
#define SPELL_BLUR 54                  // done
#define SPELL_MIRROR_IMAGE 55          // done
#define SPELL_STONESKIN 56             // done
#define SPELL_ENDURANCE 57             // done
#define SPELL_MUMMY_DUST 58            // done, epic
#define SPELL_DRAGON_KNIGHT 59         // done, epic
#define SPELL_GREATER_RUIN 60          // done, epic
#define SPELL_HELLBALL 61              // done, epic
#define SPELL_EPIC_MAGE_ARMOR 62       // done, epic
#define SPELL_EPIC_WARDING 63          // done, epic
#define SPELL_CAUSE_LIGHT_WOUNDS 64    // done
#define SPELL_CAUSE_MODERATE_WOUNDS 65 // done
#define SPELL_CAUSE_SERIOUS_WOUNDS 66  // done
#define SPELL_CAUSE_CRITICAL_WOUNDS 67 // done
#define SPELL_FLAME_STRIKE 68          // done
#define SPELL_DESTRUCTION 69           // done
#define SPELL_ICE_STORM 70             // done
#define SPELL_BALL_OF_LIGHTNING 71     // done
#define SPELL_MISSILE_STORM 72         // done
#define SPELL_CHAIN_LIGHTNING 73       // done
#define SPELL_METEOR_SWARM 74          // done
#define SPELL_PROT_FROM_GOOD 75        // done
#define SPELL_FIRE_BREATHE 76          // done, [not spell]
#define SPELL_POLYMORPH 77             // done
#define SPELL_ENDURE_ELEMENTS 78       // done
#define SPELL_EXPEDITIOUS_RETREAT 79   // done
#define SPELL_GREASE 80                // done
#define SPELL_HORIZIKAULS_BOOM 81      // done
#define SPELL_ICE_DAGGER 82            // done
#define SPELL_IRON_GUTS 83             // done
#define SPELL_MAGE_ARMOR 84            // done
#define SPELL_NEGATIVE_ENERGY_RAY 85   // done
#define SPELL_RAY_OF_ENFEEBLEMENT 86   // done
#define SPELL_SCARE 87                 // done
#define SPELL_SHELGARNS_BLADE 88       // done
#define SPELL_SHIELD 89                // done
#define SPELL_MAGE_SHIELD SPELL_SHIELD
#define SPELL_SUMMON_CREATURE_1 90     // done
#define SPELL_TRUE_STRIKE 91           // done
#define SPELL_WALL_OF_FOG 92           // done
#define SPELL_DARKNESS 93              // done
#define SPELL_SUMMON_CREATURE_2 94     // done
#define SPELL_WEB 95                   // done
#define SPELL_ACID_ARROW 96            // done
#define SPELL_DAZE_MONSTER 97          // done
#define SPELL_HIDEOUS_LAUGHTER 98      // done
#define SPELL_TOUCH_OF_IDIOCY 99       // done
#define SPELL_CONTINUAL_FLAME 100      // done
#define SPELL_SCORCHING_RAY 101        // done
#define SPELL_DEAFNESS 102             // done
#define SPELL_FALSE_LIFE 103           // done
#define SPELL_GRACE 104                // done
#define SPELL_RESIST_ENERGY 105        // done
#define SPELL_ENERGY_SPHERE 106        // done
#define SPELL_WATER_BREATHE 107        // done
#define SPELL_PHANTOM_STEED 108        // done
#define SPELL_STINKING_CLOUD 109       // done
#define SPELL_SUMMON_CREATURE_3 110    // done
#define SPELL_HALT_UNDEAD 111          // done
#define SPELL_HEROISM 112              // done
#define SPELL_VAMPIRIC_TOUCH 113       // done
#define SPELL_HOLD_PERSON 114          // done
#define SPELL_DEEP_SLUMBER 115         // done
#define SPELL_INVISIBILITY_SPHERE 116  // done
#define SPELL_DAYLIGHT 117             // done
#define SPELL_CLAIRVOYANCE 118         // done
#define SPELL_NON_DETECTION 119        // done
#define SPELL_HASTE 120                // done
#define SPELL_SLOW 121                 // done
#define SPELL_DISPEL_MAGIC 122         // done
#define SPELL_CIRCLE_A_EVIL 123        // done
#define SPELL_CIRCLE_A_GOOD 124        // done
#define SPELL_CUNNING 125              // done
#define SPELL_WISDOM 126               // done
#define SPELL_CHARISMA 127             // done
#define SPELL_STENCH 128               // done - stinking cloud proc
#define SPELL_ACID_SPLASH 129          // cantrip, unfinished
#define SPELL_RAY_OF_FROST 130         // cantrip, unfinished
#define SPELL_WIZARD_EYE 131           // done
#define SPELL_FIRE_SHIELD 132          // done
#define SPELL_COLD_SHIELD 133          // done
#define SPELL_BILLOWING_CLOUD 134      // done
#define SPELL_SUMMON_CREATURE_4 135    // done
#define SPELL_GREATER_INVIS 136        // done
#define SPELL_RAINBOW_PATTERN 137      // done
#define SPELL_LOCATE_CREATURE 138      // done
#define SPELL_MINOR_GLOBE 139          // done
#define SPELL_ENLARGE_PERSON 140       // done
#define SPELL_SHRINK_PERSON 141        // done
#define SPELL_REDUCE_PERSON SPELL_SHRINK_PERSON
#define SPELL_FSHIELD_DAM 142          // done, fire shield proc
#define SPELL_CSHIELD_DAM 143          // done, cold shield proc
#define SPELL_ASHIELD_DAM 144          // done, acid shield proc
#define SPELL_ACID_SHEATH 145          // done
#define SPELL_INTERPOSING_HAND 146     // done
#define SPELL_WALL_OF_FORCE 147        // done
#define SPELL_CLOUDKILL 148            // done
#define SPELL_SUMMON_CREATURE_5 149    // done
#define SPELL_WAVES_OF_FATIGUE 150     // done
#define SPELL_SYMBOL_OF_PAIN 151       // done
#define SPELL_DOMINATE_PERSON 152      // done
#define SPELL_FEEBLEMIND 153           // done
#define SPELL_NIGHTMARE 154            // done
#define SPELL_MIND_FOG 155             // done
#define SPELL_FAITHFUL_HOUND 156       // done
#define SPELL_DISMISSAL 157            // done
#define SPELL_CONE_OF_COLD 158         // done
#define SPELL_TELEKINESIS 159          // done
#define SPELL_FIREBRAND 160            // done
#define SPELL_DEATHCLOUD 161           // done - cloudkill proc
#define SPELL_FREEZING_SPHERE 162      // done
#define SPELL_ACID_FOG 163             // done
#define SPELL_SUMMON_CREATURE_6 164    // done
#define SPELL_TRANSFORMATION 165       // done
#define SPELL_EYEBITE 166              // done
#define SPELL_MASS_HASTE 167           // done
#define SPELL_GREATER_HEROISM 168      // done
#define SPELL_ANTI_MAGIC_FIELD 169     // done
#define SPELL_GREATER_MIRROR_IMAGE 170 // done
#define SPELL_TRUE_SEEING 171          // done
#define SPELL_GLOBE_OF_INVULN 172      // done
#define SPELL_GREATER_DISPELLING 173   // done
#define SPELL_GRASPING_HAND 174        // done
#define SPELL_SUMMON_CREATURE_7 175    // done
#define SPELL_POWER_WORD_BLIND 176     // done
#define SPELL_WAVES_OF_EXHAUSTION 177  // done
#define SPELL_MASS_HOLD_PERSON 178     // done
#define SPELL_MASS_FLY 179             // done
#define SPELL_DISPLACEMENT 180         // done
#define SPELL_PRISMATIC_SPRAY 181      // done
#define SPELL_POWER_WORD_STUN 182      // done
#define SPELL_PROTECT_FROM_SPELLS 183  // done
#define SPELL_THUNDERCLAP 184          // done
#define SPELL_SPELL_MANTLE 185         // done
#define SPELL_MASS_WISDOM 186          // done
#define SPELL_MASS_CHARISMA 187        // done
#define SPELL_CLENCHED_FIST 188        // done
#define SPELL_INCENDIARY_CLOUD 189     // done
#define SPELL_SUMMON_CREATURE_8 190    // done
#define SPELL_HORRID_WILTING 191       // done
#define SPELL_GREATER_ANIMATION 192    // done
#define SPELL_IRRESISTIBLE_DANCE 193   // done
#define SPELL_MASS_DOMINATION 194      // done
#define SPELL_SCINT_PATTERN 195        // done
#define SPELL_REFUGE 196               // done
#define SPELL_BANISH 197               // done
#define SPELL_SUNBURST 198             // done
#define SPELL_SPELL_TURNING 199        // done
#define SPELL_MIND_BLANK 200           // done
#define SPELL_IRONSKIN 201             // done
#define SPELL_MASS_CUNNING 202         // done
#define SPELL_BLADE_OF_DISASTER 203    // done
#define SPELL_SUMMON_CREATURE_9 204    // done
#define SPELL_GATE 205                 // done
#define SPELL_WAIL_OF_THE_BANSHEE 206  // done
#define SPELL_POWER_WORD_KILL 207      // done
#define SPELL_ENFEEBLEMENT 208         // done
#define SPELL_WEIRD 209                // done
#define SPELL_SHADOW_SHIELD 210        // done
#define SPELL_PRISMATIC_SPHERE 211     // done
#define SPELL_IMPLODE 212              // done
#define SPELL_TIMESTOP 213             // done
#define SPELL_GREATER_SPELL_MANTLE 214 // done
#define SPELL_MASS_ENHANCE 215         // done
#define SPELL_PORTAL 216               // done
#define SPELL_ACID 217                 // acid fog proc
#define SPELL_HOLY_SWORD 218           // done (paladin)
#define SPELL_INCENDIARY 219           // incendiary cloud proc
/* some cleric spells */
#define SPELL_CURE_MODERATE 220      // done
#define SPELL_CURE_SERIOUS 221       // done
#define SPELL_REMOVE_FEAR 222        // done
#define SPELL_CURE_DEAFNESS 223      // done
#define SPELL_FAERIE_FOG 224         // done
#define SPELL_MASS_CURE_LIGHT 225    // done
#define SPELL_AID 226                // done
#define SPELL_BRAVERY 227            // done
#define SPELL_MASS_CURE_MODERATE 228 // done
#define SPELL_REGENERATION 229       // done
#define SPELL_FREE_MOVEMENT 230      // done
#define SPELL_STRENGTHEN_BONE 231    // done
#define SPELL_MASS_CURE_SERIOUS 232  // done
#define SPELL_PRAYER 233             // done
#define SPELL_REMOVE_DISEASE 234     // done
#define SPELL_WORD_OF_FAITH 235      // done
#define SPELL_DIMENSIONAL_LOCK 236   // done
#define SPELL_SALVATION 237          // done
#define SPELL_SPRING_OF_LIFE 238     // done
#define SPELL_PLANE_SHIFT 239        // done
#define SPELL_STORM_OF_VENGEANCE 240 // done
#define SPELL_DEATH_SHIELD 241       // unfinished
#define SPELL_COMMAND 242            // unfinished
#define SPELL_AIR_WALKER 243         // unfinished
#define SPELL_GROUP_SUMMON 244       // done
#define SPELL_MASS_CURE_CRIT 245     // done
/* some druid spells */
#define SPELL_CHARM_ANIMAL 246
#define SPELL_FAERIE_FIRE 247
#define SPELL_GOODBERRY 248
#define SPELL_JUMP 249
#define SPELL_MAGIC_FANG 250
#define SPELL_MAGIC_STONE 251
#define SPELL_OBSCURING_MIST 252
#define SPELL_PRODUCE_FLAME 253
#define SPELL_SUMMON_NATURES_ALLY_1 254
#define SPELL_SUMMON_NATURES_ALLY_2 255
#define SPELL_SUMMON_NATURES_ALLY_3 256
#define SPELL_SUMMON_NATURES_ALLY_4 257
#define SPELL_SUMMON_NATURES_ALLY_5 258
#define SPELL_SUMMON_NATURES_ALLY_6 259
#define SPELL_SUMMON_NATURES_ALLY_7 260
#define SPELL_SUMMON_NATURES_ALLY_8 261
#define SPELL_SUMMON_NATURES_ALLY_9 262
#define SPELL_BARKSKIN 263
#define SPELL_FLAME_BLADE 264
#define SPELL_FLAMING_SPHERE 265
#define SPELL_HOLD_ANIMAL 266
#define SPELL_CALL_LIGHTNING_STORM 267
#define SPELL_SUMMON_SWARM 268
#define SPELL_CONTAGION 269
#define SPELL_GREATER_MAGIC_FANG 270
#define SPELL_FROST_BREATHE 271 // done, [not spell]
#define SPELL_SPIKE_GROWTH 272
#define SPELL_BLIGHT 273
#define SPELL_REINCARNATE 274
#define SPELL_LIGHTNING_BREATHE 275 // done, [not spell]
#define SPELL_SPIKE_STONES 276
#define SPELL_BALEFUL_POLYMORPH 277
#define SPELL_DEATH_WARD 278
#define SPELL_HALLOW 279
#define SPELL_INSECT_PLAGUE 280
#define SPELL_UNHALLOW 281
#define SPELL_WALL_OF_FIRE 282
#define SPELL_WALL_OF_THORNS 283
#define SPELL_FIRE_SEEDS 284
#define SPELL_CONFUSION 285
#define SPELL_MASS_ENDURANCE 286 // *note mass enhance combines these 3
#define SPELL_MASS_STRENGTH 287  // *note mass enhance combines these 3
#define SPELL_MASS_GRACE 288     // *note mass enhance combines these 3
#define SPELL_SPELL_RESISTANCE 289
#define SPELL_SPELLSTAFF 290
#define SPELL_TRANSPORT_VIA_PLANTS 291
#define SPELL_CREEPING_DOOM 292
#define SPELL_FIRE_STORM 293
#define SPELL_GREATER_SCRYING 294
#define SPELL_SUNBEAM 295
#define SPELL_ANIMAL_SHAPES 296
#define SPELL_CONTROL_PLANTS 297
#define SPELL_FINGER_OF_DEATH 298
#define SPELL_ELEMENTAL_SWARM 299
#define SPELL_GENERIC_AOE 300
#define SPELL_SHAMBLER 301
#define SPELL_SHAPECHANGE 302 // hey b, maybe use polymorph
/* some more cleric spells */
#define SPELL_BLADE_BARRIER 303 // done
#define SPELL_BLADES 304        // done - blades (for blade barrier)
#define SPELL_BATTLETIDE 305    // done
/**/
#define SPELL_I_DARKNESS 306 // room event test spell
#define SPELL_DOOM 307       // creeping doom damage proc
#define SPELL_WHIRLWIND 308
#define SPELL_LEVITATE 309 /* levitation spell - very similar to waterwalk */
#define SPELL_DRACONIC_BLOODLINE_BREATHWEAPON 310
#define SPELL_VIGORIZE_LIGHT 311
#define SPELL_VIGORIZE_SERIOUS 312
#define SPELL_VIGORIZE_CRITICAL 313
#define SPELL_GROUP_VIGORIZE 314

/* unfinished spell list (homeland-port) */

#define SPELL_EMBALM 315
#define SPELL_CONTINUAL_LIGHT 316
#define SPELL_ELEMENTAL_RESISTANCE 317 // endure elements improved
#define SPELL_PRESERVE 318
#define SPELL_RESURRECT 319
#define SPELL_SILENCE 320
#define SPELL_MINOR_CREATE 321
#define SPELL_DISPEL_INVIS 322

/** OPEN SPELL NUMBERS 323 to 381
 * UPDATE THIS COMMENT AS YOU ADD SPELLS
 * INSIDE THIS BRACKET OF OPEN SPELL NUMS
 * */

#define SPELL_ESHIELD_DAM 382 // done, acid shield proc
/* end unfinished list */

/* alchemist */
#define ALC_DISC_AFFECT_PSYCHOKINETIC 383
#define SPELL_AUGURY 384
#define PSYCHOKINETIC_FEAR 385
/****/

#define SPELL_MASS_FALSE_LIFE 386
#define SPELL_ACID_BREATHE 387
#define SPELL_POISON_BREATHE 388
#define SPELL_ENTANGLE 389
#define SPELL_DRAGONFEAR 390

#define SPELL_FEAR 391
#define SPELL_SHADOW_WALK 392
#define SPELL_CIRCLE_OF_DEATH 393
#define SPELL_UNDEATH_TO_DEATH 394
#define SPELL_GRASP_OF_THE_DEAD 395
#define SPELL_GRAVE_TOUCH 396
#define SPELL_INCORPOREAL_FORM 397
#define SPELL_LESSER_MISSILE_STORM 398
#define SPELL_SHADOW_JUMP 399
#define SPELL_UNHOLY_SWORD 400
#define SPELL_DIVINE_FAVOR 401
#define SPELL_SILVERYMOON_RECALL 402
#define SPELL_HEAL_MOUNT 403
#define SPELL_RESISTANCE 404
#define SPELL_LESSER_RESTORATION 405
#define SPELL_HEDGING_WEAPONS 406
#define SPELL_HONEYED_TONGUE 407
#define SPELL_SHIELD_OF_FORTIFICATION 408
#define SPELL_STUNNING_BARRIER 409
#define SPELL_SUN_METAL 410
#define SPELL_TACTICAL_ACUMEN 411
#define SPELL_VEIL_OF_POSITIVE_ENERGY 412
#define SPELL_BESTOW_WEAPON_PROFICIENCY 413
#define SPELL_EFFORTLESS_ARMOR 414
#define SPELL_FIRE_OF_ENTANGLEMENT 415
#define SPELL_LIFE_SHIELD 416
#define SPELL_LITANY_OF_DEFENSE 417
#define SPELL_LITANY_OF_RIGHTEOUSNESS 418
#define SPELL_RIGHTEOUS_VIGOR 419
#define SPELL_SACRED_SPACE 420
#define SPELL_REMOVE_PARALYSIS 421
#define SPELL_DANCING_WEAPON 422
#define SPELL_SPIRITUAL_WEAPON 423
#define SPELL_UNDETECTABLE_ALIGNMENT 424
#define SPELL_WEAPON_OF_AWE 425
#define SPELL_BLINDING_RAY 426
#define SPELL_HOLY_JAVELIN 427
#define SPELL_INVISIBILITY_PURGE 428
#define SPELL_KEEN_EDGE 429
#define SPELL_WEAPON_OF_IMPACT 430
#define SPELL_GREATER_MAGIC_WEAPON 431
#define SPELL_MAGIC_VESTMENT 432
#define SPELL_PROTECTION_FROM_ENERGY 433
#define SPELL_COMMUNAL_PROTECTION_FROM_ENERGY 434
#define SPELL_SEARING_LIGHT 435
#define SPELL_DIVINE_POWER 436
#define SPELL_WIND_WALL 437
#define SPELL_AIR_WALK 438
#define SPELL_GASEOUS_FORM 439
#define SPELL_RESTORATION 440
#define SPELL_GAS_BREATHE 441 // done, [not spell]
#define SPELL_MINOR_ILLUSION 442
#define SPELL_MOONBEAM 443
#define SPELL_HELLISH_REBUKE 444
#define SPELL_TRIBOAR_RECALL 445
#define SPELL_LUSKAN_RECALL 446
#define SPELL_MIRABAR_RECALL 447
#define SPELL_ANT_HAUL 448
#define SPELL_MASS_ANT_HAUL 449
#define SPELL_CORROSIVE_TOUCH 450
#define SPELL_PLANAR_HEALING 451
#define SPELL_CUSHIONING_BANDS 452
#define SPELL_GHOST_WOLF 453
#define SPELL_GIRD_ALLIES 454
#define SPELL_GLITTERDUST 455
#define SPELL_PROTECTION_FROM_ARROWS 456
#define SPELL_SPIDER_CLIMB 457
#define SPELL_WARDING_WEAPON 458
#define SPELL_AQUEOUS_ORB 459
#define SPELL_MOUNT 460
#define SPELL_COMMUNAL_MOUNT 461
#define SPELL_HUMAN_POTENTIAL 462
#define SPELL_MASS_HUMAN_POTENTIAL 463
#define SPELL_BLACK_TENTACLES 464
#define SPELL_GREATER_BLACK_TENTACLES 465
#define SPELL_CONTROL_SUMMONED_CREATURE 466
#define SPELL_CHARM_MONSTER 467
#define SPELL_MASS_ENLARGE_PERSON 468
#define SPELL_MASS_REDUCE_PERSON 469
#define SPELL_COMMUNAL_PROTECTION_FROM_ARROWS 470
#define SPELL_RAGE 471
#define SPELL_COMMUNAL_RESIST_ENERGY 472
#define SPELL_SIPHON_MIGHT 473
#define SPELL_COMMUNAL_SPIDER_CLIMB 474
#define SPELL_CAUSTIC_BLOOD 475
#define SPELL_GREATER_PLANAR_HEALING 476
#define SPELL_MASS_DAZE 477
#define SPELL_HOLD_MONSTER 478
#define SPELL_OVERLAND_FLIGHT 479
#define SPELL_COMMUNAL_STONESKIN 480
#define SPELL_MASS_STONESKIN SPELL_COMMUNAL_STONESKIN
#define SPELL_HOSTILE_JUXTAPOSITION 481
#define SPELL_GREATER_HOSTILE_JUXTAPOSITION 482
#define SPELL_BANISHING_BLADE 483
#define SPELL_PLANAR_SOUL 484

/** Total Number of defined spells  */
#define NUM_SPELLS 485
#define LAST_SPELL_DEFINE NUM_SPELLS + 1

#define MAX_SPELL_AFFECTS 6 /* change if more needed */

/*******************************/

/** We're setting apart some numbers for spell affects.
 * These are not spells or powers, but they are used in
 * the code in implementing spells or power affects.
 * For example, the mind trap psionic ability can
 * cause nausea against attackers, so it needs a separate
 * 'spell name' for that nausea.
 */

#define SPELL_AFFECT_MIND_TRAP_NAUSEA 1200
#define PSIONIC_ABILITY_PSIONIC_FOCUS 1201
#define PSIONIC_ABILITY_DOUBLE_MANIFESTATION 1202
#define SPELL_DRAGONBORN_ANCESTRY_BREATH 1203
#define PALADIN_MERCY_INJURED_FAST_HEALING 1204
#define BLACKGUARD_TOUCH_OF_CORRUPTION 1205
#define BLACKGUARD_CRUELTY_AFFECTS 1206
#define ABILITY_CHANNEL_POSITIVE_ENERGY 1207
#define ABILITY_CHANNEL_NEGATIVE_ENERGY 1208
#define WEAPON_POISON_BLACK_ADDER_VENOM 1209
#define SPELL_AFFECT_STUNNING_BARRIER 1210
#define AFFECT_ENTANGLING_FLAMES 1211
#define SPELL_EFFECT_DAZZLED 1212
#define SPELL_AFFECT_DEATH_ATTACK 1213
#define RACIAL_LICH_TOUCH 1214
#define RACIAL_LICH_FEAR 1215
#define RACIAL_LICH_REJUV 1216
#define ABILITY_AFFECT_BANE_WEAPON 1217
#define ABILITY_AFFECT_TRUE_JUDGEMENT 1218
#define SPELL_AFFECT_WEAPON_OF_AWE 1219
#define UNUSED_ABILITY_1 1220
#define RACIAL_ABILITY_SKELETON_DR 1221
#define RACIAL_ABILITY_ZOMBIE_DR 1222
#define RACIAL_ABILITY_PRIMORDIAL_DR 1223
#define ABILITY_SCORE_DAMAGE 1224
#define VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT 1225
#define AFFECT_LEVEL_DRAIN 1226
#define ABILITY_CREATE_VAMPIRE_SPAWN 1227
#define ABILITY_BLOOD_DRAIN 1228
#define AFFECT_RECENTLY_DIED 1229
#define AFFECT_RECENTLY_RESPECED 1230
#define AFFECT_FOOD 1231
#define AFFECT_DRINK 1232
#define RACIAL_ABILITY_CRYSTAL_FIST 1233
#define RACIAL_ABILITY_CRYSTAL_BODY 1234
#define PSIONIC_ABILITY_MASTERMIND 1235
#define RACIAL_ABILITY_INSECTBEING 1236
#define ABILITY_VAMPIRIC_DOMINATION 1237
#define ABILITY_AFFECT_STONES_ENDURANCE 1238
#define AFFECT_CAUSTIC_BLOOD_DAMAGE 1239
#define AFFECT_IMMUNITY_BANISHING_BLADE 1240
#define STATUS_AFFECT_STAGGERED 1241
#define AFFECT_PLANAR_SOUL_SURGE 1242

/** we're going to start psionic powers at 1500.
 * most psionic stuff is either in psionics.c or spell_parser.c
 */

/***************************************/
/***************************************/
#define PSIONIC_POWER_START 1499
/***************************************/
#define PSIONIC_BROKER 1500
#define PSIONIC_CALL_TO_MIND 1501
#define PSIONIC_CATFALL 1502
#define PSIONIC_CRYSTAL_SHARD 1503
#define PSIONIC_DECELERATION 1504
#define PSIONIC_DEMORALIZE 1505
#define PSIONIC_ECTOPLASMIC_SHEEN 1506
#define PSIONIC_ENERGY_RAY 1507
#define PSIONIC_FORCE_SCREEN 1508
#define PSIONIC_FORTIFY 1509
#define PSIONIC_INERTIAL_ARMOR 1510
#define PSIONIC_INEVITABLE_STRIKE 1511
#define PSIONIC_MIND_THRUST 1512
#define PSIONIC_DEFENSIVE_PRECOGNITION 1513
#define PSIONIC_OFFENSIVE_PRECOGNITION 1514
#define PSIONIC_OFFENSIVE_PRESCIENCE 1515
#define PSIONIC_SLUMBER 1516
#define PSIONIC_VIGOR 1517
#define PSIONIC_BESTOW_POWER 1518
#define PSIONIC_BIOFEEDBACK 1519
#define PSIONIC_BODY_EQUILIBRIUM 1520
#define PSIONIC_BREACH 1521
#define PSIONIC_CONCEALING_AMORPHA 1522
#define PSIONIC_CONCUSSION_BLAST 1523
#define PSIONIC_DETECT_HOSTILE_INTENT 1524
#define PSIONIC_ELFSIGHT 1525
#define PSIONIC_ENERGY_ADAPTATION_SPECIFIED 1526
#define PSIONIC_ENERGY_PUSH 1527
#define PSIONIC_ENERGY_STUN 1528
#define PSIONIC_INFLICT_PAIN 1529
#define PSIONIC_MENTAL_DISRUPTION 1530
#define PSIONIC_MASS_MISSIVE 1531
#define PSIONIC_PSIONIC_LOCK 1532
#define PSIONIC_PSYCHIC_BODYGUARD 1533
#define PSIONIC_RECALL_AGONY 1534
#define PSIONIC_SHARE_PAIN 1535
#define PSIONIC_SWARM_OF_CRYSTALS 1536
#define PSIONIC_THOUGHT_SHIELD 1537
#define PSIONIC_BODY_ADJUSTMENT 1538
#define PSIONIC_CONCUSSIVE_ONSLAUGHT 1539
#define PSIONIC_DISPEL_PSIONICS 1540
#define PSIONIC_ENDORPHIN_SURGE 1541
#define PSIONIC_ENERGY_BURST 1542
#define PSIONIC_ENERGY_RETORT 1543
#define PSIONIC_ERADICATE_INVISIBILITY 1544
#define PSIONIC_HEIGHTENED_VISION 1545
#define PSIONIC_MENTAL_BARRIER 1546
#define PSIONIC_MIND_TRAP 1547
#define PSIONIC_PSIONIC_BLAST 1548
#define PSIONIC_FORCED_SHARED_PAIN 1549
#define PSIONIC_SHARPENED_EDGE 1550
#define PSIONIC_UBIQUITUS_VISION 1551
#define PSIONIC_DEADLY_FEAR 1552
#define PSIONIC_DEATH_URGE 1553
#define PSIONIC_EMPATHIC_FEEDBACK 1554
#define PSIONIC_ENERGY_ADAPTATION 1555
#define PSIONIC_INCITE_PASSION 1556
#define PSIONIC_INTELLECT_FORTRESS 1557
#define PSIONIC_MOMENT_OF_TERROR 1558
#define PSIONIC_POWER_LEECH 1559
#define PSIONIC_SLIP_THE_BONDS 1560
#define PSIONIC_WITHER 1561
#define PSIONIC_WALL_OF_ECTOPLASM 1562
#define PSIONIC_ADAPT_BODY 1563
#define PSIONIC_ECTOPLASMIC_SHAMBLER 1564
#define PSIONIC_PIERCE_VEIL 1565
#define PSIONIC_PLANAR_TRAVEL 1566
#define PSIONIC_POWER_RESISTANCE 1567
#define PSIONIC_PSYCHIC_CRUSH 1568
#define PSIONIC_PSYCHOPORTATION 1569
#define PSIONIC_SHATTER_MIND_BLANK 1570
#define PSIONIC_SHRAPNEL_BURST 1571
#define PSIONIC_TOWER_OF_IRON_WILL 1572
#define PSIONIC_UPHEAVAL 1573
#define PSIONIC_BREATH_OF_THE_BLACK_DRAGON 1574
#define PSIONIC_BRUTALIZE_WOUNDS 1575
#define PSIONIC_DISINTEGRATION 1576
#define PSIONIC_REMOTE_VIEW_TRAP 1577
#define PSIONIC_SUSTAINED_FLIGHT 1578
#define PSIONIC_BARRED_MIND 1579
#define PSIONIC_COSMIC_AWARENESS 1580
#define PSIONIC_ENERGY_CONVERSION 1581
#define PSIONIC_ENERGY_WAVE 1582
#define PSIONIC_EVADE_BURST 1583
#define PSIONIC_OAK_BODY 1584
#define PSIONIC_PSYCHOSIS 1585
#define PSIONIC_ULTRABLAST 1586
#define PSIONIC_BODY_OF_IRON 1587
#define PSIONIC_GREATER_PSYCHOPORTATION 1588
#define PSIONIC_RECALL_DEATH 1589
#define PSIONIC_SHADOW_BODY 1590
#define PSIONIC_APOPSI 1591
#define PSIONIC_ASSIMILATE 1592
#define PSIONIC_TIMELESS_BODY 1593
#define PSIONIC_BARRED_MIND_PERSONAL 1594
#define PSIONIC_PSYCHOPORT_GREATER 1595
#define PSIONIC_TRUE_METABOLISM 1596
// epic
#define PSIONIC_IMPALE_MIND 1597
#define PSIONIC_ECTOPLASMIC_GOLIATH 1598
#define PSIONIC_RAZOR_STORM 1599
#define PSIONIC_PSYCHOKINETIC_THRASHING 1600
#define PSIONIC_EPIC_PSIONIC_WARD 1601
// end epic

/***************************************/
#define PSIONIC_POWER_END 1602
/***************************************/

/** we're going to start warlock powers at 1649.
 */

/***************************************/
/***************************************/
#define WARLOCK_POWER_START 1648
/***************************************/
#define WARLOCK_ELDRITCH_BLAST 1649
// start least invocations
#define WARLOCK_ELDRITCH_SPEAR 1650
#define WARLOCK_HIDEOUS_BLOW 1651
#define WARLOCK_DRAINING_BLAST 1652
#define WARLOCK_FRIGHTFUL_BLAST 1653
#define WARLOCK_BEGUILING_INFLUENCE 1654
#define WARLOCK_DARK_ONES_OWN_LUCK 1655
#define WARLOCK_DARKNESS 1656
#define WARLOCK_DEVILS_SIGHT 1657
#define WARLOCK_ENTROPIC_WARDING 1658
#define WARLOCK_LEAPS_AND_BOUNDS 1659
#define WARLOCK_OTHERWORLDLY_WHISPERS 1660
#define WARLOCK_SEE_THE_UNSEEN 1661
// end least invocations
// start lesser invocations
#define WARLOCK_ELDRITCH_CHAIN 1662
#define WARLOCK_BESHADOWED_BLAST 1663
#define WARLOCK_BRIMSTONE_BLAST 1664
#define WARLOCK_HELLRIME_BLAST 1665
#define WARLOCK_CHARM 1666
#define WARLOCK_CURSE_OF_DESPAIR 1667
#define WARLOCK_DREAD_SEIZURE 1668
#define WARLOCK_FLEE_THE_SCENE 1669
#define WARLOCK_THE_DEAD_WALK 1670
#define WARLOCK_VORACIOUS_DISPELLING 1671
#define WARLOCK_WALK_UNSEEN 1672
// end lesser invocations
// start greater invocations
#define WARLOCK_ELDRITCH_CONE 1673
#define WARLOCK_BEWITCHING_BLAST 1674
#define WARLOCK_UNUSED_ABILITY 1675  // this appears to be exactly the same as DRAINING. Removed.
#define WARLOCK_NOXIOUS_BLAST 1676
#define WARLOCK_VITRIOLIC_BLAST 1677
#define WARLOCK_CHILLING_TENTACLES 1678
#define WARLOCK_DEVOUR_MAGIC 1679
#define WARLOCK_TENACIOUS_PLAGUE 1680
#define WARLOCK_WALL_OF_PERILOUS_FLAME 1681
// end greater invocations
// start dark invocations
#define WARLOCK_ELDRITCH_DOOM 1682
#define WARLOCK_BINDING_BLAST 1683
#define WARLOCK_UTTERDARK_BLAST 1684
#define WARLOCK_DARK_FORESIGHT 1685
#define WARLOCK_RETRIBUTIVE_INVISIBILITY 1686
#define WARLOCK_WORD_OF_CHANGING 1687
// end dark invocations

/***************************************/
#define WARLOCK_POWER_END 1688
/***************************************/

/***************************************/

/* Other files to be aware of for new spells:
 * 1)  if you want this spell to be avaiable as a npc spellup, mobact.c
 * 2)  if you want this spell to be available as a npc nuke, mobact.c
 */

/* Insert new spells here, up to MAX_SPELLS */
/* make sure this matches up with structs.h spellbook define */
// -----NEW NEW NEW -----
// spells are in a table separate from skills, abilities and everything else
// Gicker Feb 5, 2021
#define MAX_SPELLS 2000
#define TOP_SPELL_DEFINE 2300

#define START_SKILLS 2000

/* PLAYER SKILLS - Numbered from START_SKILLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB 2001          // implemented
#define SKILL_BASH 2002              // implemented
#define SKILL_MUMMY_DUST 2003        // implemented
#define SKILL_KICK 2004              // implemented
#define SKILL_WEAPON_SPECIALIST 2005 // implemented
#define SKILL_WHIRLWIND 2006         // implemented
#define SKILL_RESCUE 2007            // implemented
#define SKILL_DRAGON_KNIGHT 2008     // implemented
#define SKILL_LUCK_OF_HEROES 2009    // implemented
#define SKILL_TRACK 2010             // implemented
#define SKILL_QUICK_CHANT 2011       // implemented
#define SKILL_AMBIDEXTERITY 2012     // implemented
#define SKILL_DIRTY_FIGHTING 2013    // implemented
#define SKILL_DODGE 2014             // implemented
#define SKILL_IMPROVED_CRITICAL 2015 // implemented
#define SKILL_MOBILITY 2016          // implemented
#define SKILL_SPRING_ATTACK 2017     // implemented
#define SKILL_TOUGHNESS 2018         // implemented
#define SKILL_TWO_WEAPON_FIGHT 2019  // implemented
#define SKILL_FINESSE 2020           // implemented
#define SKILL_ARMOR_SKIN 2021        // implemented
#define SKILL_BLINDING_SPEED 2022    // implemented
#define SKILL_DAMAGE_REDUC_1 2023    // implemented
#define SKILL_DAMAGE_REDUC_2 2024    // implemented
#define SKILL_DAMAGE_REDUC_3 2025    // implemented
#define SKILL_EPIC_TOUGHNESS 2026    // implemented
#define SKILL_OVERWHELMING_CRIT 2027 // implemented
#define SKILL_SELF_CONCEAL_1 2028    // implemented
#define SKILL_SELF_CONCEAL_2 2029    // implemented
#define SKILL_SELF_CONCEAL_3 2030    // implemented
#define SKILL_TRIP 2031              // implemented
#define SKILL_IMPROVED_WHIRL 2032    // implemented
#define SKILL_CLEAVE 2033
#define SKILL_GREAT_CLEAVE 2034
#define SKILL_SPELLPENETRATE 2035   // implemented
#define SKILL_SPELLPENETRATE_2 2036 // implemented
#define SKILL_PROWESS 2037          // implemented
#define SKILL_EPIC_PROWESS 2038     // implemented
#define SKILL_EPIC_2_WEAPON 2039    // implemented
#define SKILL_SPELLPENETRATE_3 2040 // implemented
#define SKILL_SPELL_RESIST_1 2041   // implemented
#define SKILL_SPELL_RESIST_2 2042   // implemented
#define SKILL_SPELL_RESIST_3 2043   // implemented
#define SKILL_SPELL_RESIST_4 2044   // implemented
#define SKILL_SPELL_RESIST_5 2045   // implemented
#define SKILL_INITIATIVE 2046       // implemented
#define SKILL_EPIC_CRIT 2047        // implemented
#define SKILL_IMPROVED_BASH 2048    // implemented
#define SKILL_IMPROVED_TRIP 2049    // implemented
#define SKILL_POWER_ATTACK 2050     // implemented
#define SKILL_EXPERTISE 2051        // implemented
#define SKILL_GREATER_RUIN 2052     // implemented
#define SKILL_HELLBALL 2053         // implemented
#define SKILL_EPIC_MAGE_ARMOR 2054  // implemented
#define SKILL_EPIC_WARDING 2055     // implemented
#define SKILL_RAGE 2056             // implemented
#define SKILL_PROF_MINIMAL 2057     // implemented
#define SKILL_PROF_BASIC 2058       // implemented
#define SKILL_PROF_ADVANCED 2059    // implemented
#define SKILL_PROF_MASTER 2060      // implemented
#define SKILL_PROF_EXOTIC 2061      // implemented
#define SKILL_PROF_LIGHT_A 2062     // implemented
#define SKILL_PROF_MEDIUM_A 2063    // implemented
#define SKILL_PROF_HEAVY_A 2064     // implemented
#define SKILL_PROF_SHIELDS 2065     // implemented
#define SKILL_PROF_T_SHIELDS 2066   // implemented
#define SKILL_MURMUR 2067
#define SKILL_PROPAGANDA 2068
#define SKILL_LOBBY 2069
#define SKILL_STUNNING_FIST 2070 // implemented
/* initial crafting skills */
#define TOP_CRAFT_SKILL 2071
/**/
#define SKILL_MINING 2071          // implemented
#define SKILL_HUNTING 2072         // implemented
#define SKILL_FORESTING 2073       // implemented
#define SKILL_KNITTING 2074        // implemented
#define SKILL_CHEMISTRY 2075       // implemented
#define SKILL_ARMOR_SMITHING 2076  // implemented
#define SKILL_WEAPON_SMITHING 2077 // implemented
#define SKILL_JEWELRY_MAKING 2078  // implemented
#define SKILL_LEATHER_WORKING 2079 // implemented
#define SKILL_FAST_CRAFTER 2080    // implemented
#define SKILL_BONE_ARMOR 2081
#define SKILL_ELVEN_CRAFTING 2082
#define SKILL_MASTERWORK_CRAFTING 2083
#define SKILL_DRACONIC_CRAFTING 2084
#define SKILL_DWARVEN_CRAFTING 2085
/* */
#define BOTTOM_CRAFT_SKILL 2086
/* finish batch crafting skills */
#define SKILL_LIGHTNING_REFLEXES 2086 // implemented
#define SKILL_GREAT_FORTITUDE 2087    // implemented
#define SKILL_IRON_WILL 2088          // implemented
#define SKILL_EPIC_REFLEXES 2089      // implemented
#define SKILL_EPIC_FORTITUDE 2090     // implemented
#define SKILL_EPIC_WILL 2091          // implemented
#define SKILL_SHIELD_SPECIALIST 2092  // implemented
#define SKILL_USE_MAGIC 2093          // implemented
#define SKILL_EVASION 2094            // implemented
#define SKILL_IMP_EVASION 2095        // implemented
#define SKILL_CRIP_STRIKE 2096        // implemented
#define SKILL_SLIPPERY_MIND 2097      // implemented
#define SKILL_DEFENSE_ROLL 2098       // implemented
#define SKILL_GRACE 2099              // implemented

/* PLAYER SKILLS */
#define SKILL_DIVINE_HEALTH 2100    // implemented
#define SKILL_LAY_ON_HANDS 2101     // implemented
#define SKILL_COURAGE 2102          // implemented
#define SKILL_SMITE_EVIL 2103       // implemented
#define SKILL_REMOVE_DISEASE 2104   // implemented
#define SKILL_RECHARGE 2105         // implemented
#define SKILL_STEALTHY 2106         // implemented
#define SKILL_NATURE_STEP 2107      // implemented
#define SKILL_FAVORED_ENEMY 2108    // implemented
#define SKILL_DUAL_WEAPONS 2109     // implemented
#define SKILL_ANIMAL_COMPANION 2110 // implemented
#define SKILL_PALADIN_MOUNT 2111    // implemented
#define SKILL_CALL_FAMILIAR 2112    // implemented
#define SKILL_PERFORM 2113          // implemented
#define SKILL_SCRIBE 2114           // implemented
#define SKILL_TURN_UNDEAD 2115      // implemented
#define SKILL_WILDSHAPE 2116        // implemented
#define SKILL_SPELLBATTLE 2117      // implemented
#define SKILL_HITALL 2118
#define SKILL_CHARGE 2119
#define SKILL_BODYSLAM 2120
#define SKILL_SPRINGLEAP 2121
#define SKILL_HEADBUTT 2122
#define SKILL_SHIELD_PUNCH 2123
#define SKILL_DIRT_KICK 2124
#define SKILL_SAP 2125
#define SKILL_SHIELD_SLAM 2126
#define SKILL_SHIELD_CHARGE 2127
#define SKILL_QUIVERING_PALM 2128 // implemented
#define SKILL_SURPRISE_ACCURACY 2129
#define SKILL_POWERFUL_BLOW 2130
#define SKILL_RAGE_FATIGUE 2131 // implemented
#define SKILL_COME_AND_GET_ME 2132
#define SKILL_FEINT 2133
#define SKILL_SMITE_GOOD 2134
#define SKILL_SMITE_DESTRUCTION 2135
#define SKILL_DESTRUCTIVE_AURA 2136
#define SKILL_AURA_OF_PROTECTION 2137
#define SKILL_DEATH_ARROW 2138
#define SKILL_DEFENSIVE_STANCE 2139
#define SKILL_CRIPPLING_CRITICAL 2140
#define SKILL_DRHRT_CLAWS 2141
#define SKILL_DRHRT_WINGS 2142
#define SKILL_BOMB_TOSS 2143
#define SKILL_MUTAGEN 2144
#define SKILL_COGNATOGEN 2145
#define SKILL_INSPIRING_COGNATOGEN 2146
#define SKILL_PSYCHOKINETIC 2147
#define SKILL_INNER_FIRE 2148
#define SKILL_SACRED_FLAMES 2149
#define SKILL_EPIC_WILDSHAPE 2150 // implemented
#define SKILL_DRAGON_BITE 2151
#define SKILL_SLAM 2152

/* reserving this space for different performances 2180 - 2199*/
#define TOP_OF_PERFORMANCES 2180
/***/
#define SKILL_SONG_OF_FOCUSED_MIND 2188
#define SKILL_SONG_OF_FEAR 2189
#define SKILL_SONG_OF_ROOTING 2190
#define SKILL_SONG_OF_THE_MAGI 2191
#define SKILL_SONG_OF_HEALING 2192
#define SKILL_DANCE_OF_PROTECTION 2193
#define SKILL_SONG_OF_FLIGHT 2194
#define SKILL_SONG_OF_HEROISM 2195
#define SKILL_ORATORY_OF_REJUVENATION 2196
#define SKILL_ACT_OF_FORGETFULNESS 2197
#define SKILL_SONG_OF_REVELATION 2198
#define SKILL_SONG_OF_DRAGONS 2199
/**/
#define END_OF_PERFORMANCES 2200
/**** end songs ****/

/* New skills may be added above here, up to MAX_SKILLS (3000) */
#define NUM_SKILLS 2200

/* Special Abilities for weapons */

#define TYPE_SPECAB_FLAMING 2200
#define TYPE_SPECAB_FLAMING_BURST 2201
#define TYPE_SPECAB_FROST 2202
#define TYPE_SPECAB_ICY_BURST 2203
#define TYPE_SPECAB_CORROSIVE 2204
#define TYPE_SPECAB_HOLY 2205
#define TYPE_SPECAB_CORROSIVE_BURST 2206
#define TYPE_SPECAB_THUNDERING 2207
#define TYPE_SPECAB_BLEEDING 2208
#define TYPE_SPECAB_SHOCK 2209
#define TYPE_SPECAB_SHOCKING_BURST 2210
#define TYPE_SPECAB_ANARCHIC 2211
#define TYPE_SPECAB_UNHOLY 2212

/* alchemist bombs with effects */
#define BOMB_AFFECT_ACID 2234
#define BOMB_AFFECT_BLINDING 2235
#define BOMB_AFFECT_BONESHARD 2236
#define BOMB_AFFECT_CONCUSSIVE 2237
#define BOMB_AFFECT_CONFUSION 2238
#define BOMB_AFFECT_FIRE_BRAND 2239
#define BOMB_AFFECT_FORCE 2240
#define BOMB_AFFECT_FROST 2241
#define BOMB_AFFECT_HOLY 2242
#define BOMB_AFFECT_IMMOLATION 2243
#define BOMB_AFFECT_PROFANE 2244
#define BOMB_AFFECT_SHOCK 2245
#define BOMB_AFFECT_STICKY 2246
#define BOMB_AFFECT_SUNLIGHT 2247
#define BOMB_AFFECT_TANGLEFOOT 2248

/* Attack types */

#define TYPE_ATTACK_OF_OPPORTUNITY 2250

/* NON-PLAYER AND OBJECT SPELLS AND SKILLS: The practice levels for the spells
 * and skills below are _not_ recorded in the players file; therefore, the
 * intended use is for spells and skills associated with objects (such as
 * SPELL_IDENTIFY used with scrolls of identify) or non-players (such as NPC
 * only spells). */

/* To make an affect induced by dg_affect look correct on 'stat' we need to
 * define it with a 'spellname'. */
#define SPELL_DG_AFFECT 2298

#define TOP_SPELLS_POWERS_SKILLS_BOMBS 2299

/* NEW NPC/OBJECT SPELLS can be inserted here up to 2299 */

/**********  IMPORTANT ********************************/
/*** Due to wanting the values to be more global, I moved
     all the WEAPON ATTACK TYPES to structs.h
 Reserving values: 2300 - 2350 for this purpose
 */
/******************************************************/

/* RANGED WEAPON ATTACK TYPES */
#define TYPE_MISSILE 2351
/** total number of ranged attack types */
#define NUM_RANGED_TYPES 1

// Vampire cloak
#ifdef CAMPAIGN_FR
  #define VAMPIRE_CLOAK_OBJ_VNUM 299
#else
  #define VAMPIRE_CLOAK_OBJ_VNUM 34700
#endif

/* not hard coded, but up to 2375 */

/* weapon type macros, returns true or false */
#define IS_BLADE(obj) (GET_OBJ_VAL(obj, 3) == (TYPE_WHIP - TYPE_HIT) ||  \
                       GET_OBJ_VAL(obj, 3) == (TYPE_SLASH - TYPE_HIT) || \
                       GET_OBJ_VAL(obj, 3) == (TYPE_CLAW - TYPE_HIT) ||  \
                       GET_OBJ_VAL(obj, 3) == (TYPE_MAUL - TYPE_HIT) ||  \
                       GET_OBJ_VAL(obj, 3) == (TYPE_THRASH - TYPE_HIT))
#define IS_PIERCE(obj) (GET_OBJ_VAL(obj, 3) == (TYPE_STING - TYPE_HIT) ||  \
                        GET_OBJ_VAL(obj, 3) == (TYPE_BITE - TYPE_HIT) ||   \
                        GET_OBJ_VAL(obj, 3) == (TYPE_PIERCE - TYPE_HIT) || \
                        GET_OBJ_VAL(obj, 3) == (TYPE_BLAST - TYPE_HIT) ||  \
                        GET_OBJ_VAL(obj, 3) == (TYPE_STAB - TYPE_HIT))
#define IS_BLUNT(obj) (GET_OBJ_VAL(obj, 3) == (TYPE_HIT - TYPE_HIT) ||      \
                       GET_OBJ_VAL(obj, 3) == (TYPE_BLUDGEON - TYPE_HIT) || \
                       GET_OBJ_VAL(obj, 3) == (TYPE_CRUSH - TYPE_HIT) ||    \
                       GET_OBJ_VAL(obj, 3) == (TYPE_POUND - TYPE_HIT) ||    \
                       GET_OBJ_VAL(obj, 3) == (TYPE_PUNCH - TYPE_HIT))

/* other attack types */

#define TYPE_LAVA_DAMAGE 2391
#define TYPE_DROWNING 2392
#define TYPE_MOVING_WATER 2393
#define TYPE_SUN_DAMAGE 2394
#define TYPE_ESHIELD 2395
#define TYPE_CSHIELD 2396
#define TYPE_FSHIELD 2397
#define TYPE_ASHIELD 2398
#define TYPE_SUFFERING 2399
/* new attack types can be added here - up to TYPE_SUFFERING */
#define MAX_TYPES 2400

#define SKILL_LANG_COMMON 2401
#define SKILL_LANG_BASIC SKILL_LANG_COMMON
#define SKILL_LANG_THIEVES_CANT 2402
#define SKILL_LANG_DRUIDIC 2403
#define SKILL_LANG_ABYSSAL 2404
#define SKILL_LANG_ELVEN 2405
#define SKILL_LANG_GNOME 2406
#define SKILL_LANG_DWARVEN 2407
#define SKILL_LANG_CELESTIAL 2408
#define SKILL_LANG_DRACONIC 2409
#define SKILL_LANG_ORCISH 2410
#define SKILL_LANG_HALFLING 2411
#define SKILL_LANG_GOBLIN 2412
#define SKILL_LANG_ABERRATION 2413
#define SKILL_LANG_GIANT 2414
#define SKILL_LANG_KOBOLD 2415
#define SKILL_LANG_BARBARIAN 2416
#define SKILL_LANG_ERGOT 2417
#define SKILL_LANG_ISTARIAN 2418
#define SKILL_LANG_BALIFORIAN 2419
#define SKILL_LANG_KHAROLISIAN 2420
#define SKILL_LANG_MULHORANDI 2421
#define SKILL_LANG_RASHEMI 2422
#define SKILL_LANG_NORTHERNER 2423
#define SKILL_LANG_UNDERWORLD 2424
#define SKILL_LANG_ANCIENT 2425
#define SKILL_LANG_BINARY 2426
#define SKILL_LANG_BOCCE 2427
#define SKILL_LANG_BOTHESE 2428
#define SKILL_LANG_CEREAN 2429
#define SKILL_LANG_DOSH 2430
#define SKILL_LANG_DURESE 2431
#define SKILL_LANG_UNDERCOMMON 2432
/**/
#define SKILL_LANG_LOW 2401
#define SKILL_LANG_HIGH 2433
#define MIN_LANGUAGES SKILL_LANG_LOW
#define MAX_LANGUAGES SKILL_LANG_HIGH

/*****  !!!! MAKE SURE MAX_SKILLS (structs.h) IS BIGGER THAN THIS NUMBER!!! -zusuk ******/
#define TOP_SKILL_DEFINE 2433

/*----------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------*/

/*---------------------------ABILITIES-------------------------------------*/

/* PLAYER ABILITIES -- Numbered from 1 to MAX_ABILITIES
 * If an ability/skill here isn't part of the pfsrd, we marked the comment
 * with UNIQUE, some old deprecated skills have been phased out and are marked
 * as UNUSED in the comments -Zusuk      */
#define ABILITY_UNDEFINED 0
#define START_GENERAL_ABILITIES 1

#define ABILITY_ACROBATICS 1        /* acrobatics, matches pfsrd */
#define ABILITY_STEALTH 2           /* stealth, matches pfsrd, use to be hide + move silently */
#define ABILITY_UNUSED_1 3          /* UNUSED - use to be move silently */
#define ABILITY_PERCEPTION 4        /* perception, matches pfsrd, use to be spot + listen */
#define ABILITY_UNUSED_2 5          /* UNUSED - use to be listen */
#define ABILITY_HEAL 6              /* heal (treatinjury), matches pfsrd */
#define ABILITY_INTIMIDATE 7        /* intimidate, matches pfsrd, use to be taunt */
#define ABILITY_CONCENTRATION 8     /* UNIQUE concentration */
#define ABILITY_SPELLCRAFT 9        /* spellcraft, matches pfsrd */
#define ABILITY_APPRAISE 10         /* appraise, matches pfsrd */
#define ABILITY_DISCIPLINE 11       /* UNIQUE discipline */
#define ABILITY_TOTAL_DEFENSE 12    /* UNIQUE total defense */
#define ABILITY_LORE 13             /* UNIQUE lore */
#define ABILITY_RIDE 14             /* ride, matches pfsrd */
#define ABILITY_UNUSED_3 15         /* UNUSED - use to be balance */
#define ABILITY_CLIMB 16            /* climb, matches pfsrd */
#define ABILITY_UNUSED_4 17         /* UNUSED - use to be open locks */
#define ABILITY_SLEIGHT_OF_HAND 18  /* sleight of hand */
#define ABILITY_UNUSED_5 19         /* UNUSED - use to be search */
#define ABILITY_BLUFF 20            /* bluff, matches pfsrd */
#define ABILITY_UNUSED_6 21         /* UNUSED - use to be decipher script */
#define ABILITY_DIPLOMACY 22        /* diplomacy, matches pfsrd */
#define ABILITY_DISABLE_DEVICE 23   /* disable device, matches pfsrd */
#define ABILITY_DISGUISE 24         /* diguise, matches pfsrd */
#define ABILITY_ESCAPE_ARTIST 25    /* escape artist, matches pfsrd */
#define ABILITY_HANDLE_ANIMAL 26    /* handle animal, matches pfsrd */
#define ABILITY_UNUSED_7 27         /* UNUSED - use to be jump */
#define ABILITY_SENSE_MOTIVE 28     /* sense motive, matches pfsrd */
#define ABILITY_SURVIVAL 29         /* survival, matches pfsrd */
#define ABILITY_SWIM 30             /* swim, matches pfsrd */
#define ABILITY_USE_MAGIC_DEVICE 31 /* use magic device, matches pfsrd */
#define ABILITY_LINGUISTICS 32      // Number of languages known
#define ABILITY_PERFORM 33          /* perform, matches pfsrd */
/**/
#define END_GENERAL_ABILITIES 33

/* Start Crafting Abilities */
#define START_CRAFT_ABILITIES 34

#define ABILITY_CRAFT_WOODWORKING 34
#define ABILITY_CRAFT_TAILORING 35
#define ABILITY_CRAFT_ALCHEMY 36
#define ABILITY_CRAFT_ARMORSMITHING 37
#define ABILITY_CRAFT_WEAPONSMITHING 38
#define ABILITY_CRAFT_BOWMAKING 39
#define ABILITY_CRAFT_GEMCUTTING 40
#define ABILITY_CRAFT_LEATHERWORKING 41
#define ABILITY_CRAFT_TRAPMAKING 42
#define ABILITY_CRAFT_POISONMAKING 43
#define ABILITY_CRAFT_METALWORKING 44

#define END_CRAFT_ABILITIES 44
/* End Crafting Abilities */

/* Start Knowledge Abilities */
#define START_KNOWLEDGE_ABILITIES 45

#define ABILITY_KNOWLEDGE_ARCANA 45
#define ABILITY_KNOWLEDGE_ENGINEERING 46
#define ABILITY_KNOWLEDGE_DUNGEONEERING 47
#define ABILITY_KNOWLEDGE_GEOGRAPHY 48
#define ABILITY_KNOWLEDGE_HISTORY 49
#define ABILITY_KNOWLEDGE_LOCAL 50
#define ABILITY_KNOWLEDGE_NATURE 51
#define ABILITY_KNOWLEDGE_NOBILITY 52
#define ABILITY_KNOWLEDGE_RELIGION 53
#define ABILITY_KNOWLEDGE_PLANES 54

#define END_KNOWLEDGE_ABILITIES 54

/* The abilities below have 'subabilities', basically
 * the skill is broken down into many many sub skills
 * each of which can be chosen for a train. Not yet
 * Implemented. */
/*
#define ABILITY_CRAFT                   29
#define ABILITY_KNOWLEDGE               30
#define ABILITY_PROFESSION              32
#define ABILITY_SPEAK_LANGUAGE          33
*/

#define NUM_ABILITIES 55 /* Number of defined abilities */
/*	MAX_ABILITIES = 200 */
/*-------------------------------------------------------------------------*/

// ******** DAM_ *********
#define DAM_RESERVED_DBC 0 // reserve
#define DAM_FIRE 1
#define DAM_COLD 2
#define DAM_AIR 3
#define DAM_EARTH 4
#define DAM_ACID 5
#define DAM_HOLY 6
#define DAM_ELECTRIC 7
#define DAM_UNHOLY 8
#define DAM_SLICE 9
#define DAM_PUNCTURE 10
#define DAM_FORCE 11
#define DAM_SOUND 12
#define DAM_POISON 13
#define DAM_DISEASE 14
#define DAM_NEGATIVE 15
#define DAM_ILLUSION 16
#define DAM_MENTAL 17
#define DAM_LIGHT 18
#define DAM_ENERGY 19
#define DAM_WATER 20
#define DAM_CELESTIAL_POISON 21
#define DAM_BLEEDING 22
#define DAM_TEMPORAL 23
#define DAM_CHAOS 24
#define DAM_SUNLIGHT 25     // don't want this resistable as it's a vampire weakness
#define DAM_MOVING_WATER 26 // don't want this resistable as it's a vampire weakness
#define DAM_BLOOD_DRAIN 27  // don't want this resistable as it's vampire blood drain only
/* ------------------------------*/
#define NUM_DAM_TYPES 28
/* if you add more dam types, don't forget to assign it to a gear-slot
   so players will have some sort of method of gaining the defense against
   the new damage type */
/* =============================*/

/*********************************/
/******** Schools of Magic *******/
/*****Note Skills use this same***/
/********category for "type"******/
/*********************************/
/* this now exists in domains_schools.h */
/*--------------------------------*/

/*********************************/
/********** Skill types **********/
/*******Used for Sorting Only*****/
/*********************************/
#define UNCATEGORIZED 0
#define ACTIVE_SKILL 1
#define PASSIVE_SKILL 2
#define CRAFTING_SKILL 3
#define CASTER_SKILL 4

#define NUM_SKILL_CATEGORIES 5

/************************************/
/********** Saves *******************/
/************************************/
#define SAVING_FORT 0
#define SAVING_REFL 1
#define SAVING_WILL 2
#define SAVING_POISON 3
#define SAVING_DEATH 4
#define NUM_SAVINGS 5
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
#define TAR_IGNORE (1 << 0)
#define TAR_CHAR_ROOM (1 << 1)
#define TAR_CHAR_WORLD (1 << 2)
#define TAR_FIGHT_SELF (1 << 3)
#define TAR_FIGHT_VICT (1 << 4)
#define TAR_SELF_ONLY (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF (1 << 6)  /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV (1 << 7)
#define TAR_OBJ_ROOM (1 << 8)
#define TAR_OBJ_WORLD (1 << 9)
#define TAR_OBJ_EQUIP (1 << 10)

struct spell_info_type
{
        byte min_position; /* Position for caster	 */
        int psp_min;       /* Min amount of psp used by a spell (highest lev) */
        int psp_max;       /* Max amount of psp used by a spell (lowest lev) */
        int psp_change;    /* Change in psp used by spell from lev to lev */

        int min_level[NUM_CLASSES]; /* the level [class] gets this spell (lvl_impl + 1, if they don't get */
        int routines;
        byte violent;
        int targets;              /* See below for use with TAR_XXX  */
        const char *name;         /* Input size not limited. Originates from string constants. */
        const char *wear_off_msg; /* Input size not limited. Originates from string constants. */
        int time;                 /* casting time */
        int memtime;              /* mem time */
        int schoolOfMagic;        // school of magic, category for skills

        bool quest; // is this a quest spell?

        /* TODO: d20pfsrd expansion */

        /* school declared above as "schoolOfMagic" */

        /* sub-schools: calling, creation, healing, summoning, teleportation, charm,
      compulsion, figment, glamer, pattern, phantasm, shadow, and polymorph */
        int sub_school;
        /*The descriptors are acid, air, chaotic, cold, curse, darkness, death, disease,
   *  earth, electricity, emotion, evil, fear, fire, force, good, language-dependent,
   *  lawful, light, mind-affecting, pain, poison, shadow, sonic, and water.
    Most of these descriptors have no game effect by themselves, but they govern
   *  how the spell interacts with other spells, with special abilities, with unusual
   *  creatures, with alignment, and so on.*/
        int descriptor;
        int action_time;         /* casting time, in the form of action consumed */
        int component;           /* verbal, somatic, material, focus, or divine-focus */
        int domain[NUM_DOMAINS]; /* cleric domains! this is level, not circle like it should be */

        /* probably not ever going to use */
        int range;        /* targets covers this currently */
        int aim_type;     /* ray, spread, area,  burst, emanation, cone, cylinder, line, sphere, etc */
        int duration;     /* assigned in the code currently */
        int saving_throw; /* assigned in the code currently */
        int resistance;   /* spell resistance, assigned in code currently */
        bool ritual_spell; // If this is a ritual spell, it will have a cast time, otherwise it won't
};

/* wall struct for wall spells, like wall of fire, force, thorns, etc */
struct wall_information
{
        bool stops_movement;
        int spell_num;
        const char *longname;
        const char *shortname;
        const char *keyword;
        int duration;
};
/* wall types for wall spells, like wall of fire, wall of thorns, wall of etc */
#define WALL_TYPE_INVALID -1
#define WALL_TYPE_FORCE 0
#define WALL_TYPE_FIRE 1
#define WALL_TYPE_THORNS 2
#define WALL_TYPE_FOG 3
#define WALL_TYPE_PRISM 4
#define WALL_TYPE_ECTOPLASM 5
#define WALL_TYPE_PERILOUS_FIRE 6
/******/
#define NUM_WALL_TYPES 7
/****/
#define WALL_ITEM 101220
/* object values for walls */
#define WALL_TYPE 0  /* type, effect */
#define WALL_DIR 1   /* direction blocking */
#define WALL_LEVEL 2 /* level of wall in case creator can't be found */
#define WALL_IDNUM 3 /* creator's idnum */

/* manual spell header info */
#define ASPELL(spellname)                               \
        void spellname(int level, struct char_data *ch, \
                       struct char_data *victim, struct obj_data *obj, int casttype)

#define MANUAL_SPELL(spellname) spellname(level, caster, cvict, ovict, casttype);

/* manual spells */
ASPELL(spell_acid_arrow);
ASPELL(spell_augury);
ASPELL(spell_banish);
ASPELL(spell_charm);
ASPELL(spell_charm_monster);
ASPELL(spell_charm_animal);
ASPELL(spell_clairvoyance);
ASPELL(spell_cloudkill);
ASPELL(spell_control_plants);
ASPELL(spell_control_weather);
ASPELL(spell_create_water);
ASPELL(spell_creeping_doom);
ASPELL(spell_detect_poison);
ASPELL(spell_dismissal);
ASPELL(spell_dispel_magic);
ASPELL(spell_dominate_person);
ASPELL(spell_enchant_item);
ASPELL(spell_greater_dispelling);
ASPELL(spell_group_summon);
ASPELL(spell_identify);
ASPELL(spell_implode);
ASPELL(spell_incendiary_cloud);
ASPELL(spell_information);
ASPELL(spell_locate_creature);
ASPELL(spell_locate_object);
ASPELL(spell_resurrect);
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
ASPELL(spell_shadow_jump);
ASPELL(spell_transport_via_plants);
ASPELL(spell_resurrect);
ASPELL(spell_wall_of_fire);
ASPELL(spell_wall_of_thorns);
ASPELL(spell_wall_of_force);
ASPELL(spell_wizard_eye);
ASPELL(spell_spiritual_weapon);
ASPELL(spell_dancing_weapon);
ASPELL(spell_holy_javelin);
ASPELL(spell_moonbeam);
ASPELL(spell_luskan_recall);
ASPELL(spell_triboar_recall);
ASPELL(spell_silverymoon_recall);
ASPELL(spell_mirabar_recall);
ASPELL(spell_gird_allies);
ASPELL(spell_aqueous_orb);
ASPELL(spell_human_potential);
ASPELL(spell_mass_human_potential);
ASPELL(spell_control_summoned_creature);
ASPELL(spell_siphon_might);
ASPELL(spell_overland_flight);

// psionics
ASPELL(psionic_concussive_onslaught);
ASPELL(psionic_wall_of_ectoplasm);
ASPELL(psionic_psychoportation);

// warlocks
ASPELL(eldritch_blast);
ASPELL(warlock_charm);
ASPELL(voracious_dispelling);
ASPELL(tenacious_plague);
ASPELL(wall_of_perilous_flame);

/* basic magic calling functions */
int find_skill_num(char *name);
int find_ability_num(char *name);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
               struct obj_data *obj, int spellnum, int metamagic, int savetype, int casttype);
void mag_loops(int level, struct char_data *ch, struct char_data *victim,
               struct obj_data *obj, int spellnum, int metamagic, int savetype, int casttype);
void mag_affects(int level, struct char_data *ch, struct char_data *victim,
                 struct obj_data *obj, int spellnum, int savetype, int casttype, int metamagic);
void mag_groups(int level, struct char_data *ch, struct obj_data *obj,
                int spellnum, int savetype, int casttype);
void mag_masses(int level, struct char_data *ch, struct obj_data *obj,
                int spellnum, int savetype, int casttype, int metamagic);
void mag_areas(int level, struct char_data *ch, struct obj_data *obj,
               int spellnum, int metamagic, int savetype, int casttype);
void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
                 int spellnum, int savetype, int casttype);
void mag_points(int level, struct char_data *ch, struct char_data *victim,
                struct obj_data *obj, int spellnum, int savetype, int casttype);
void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
                   struct obj_data *obj, int spellnum, int type, int casttype);
void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
                    int spellnum, int type, int casttype);
void mag_creations(int level, struct char_data *ch, struct char_data *vict,
                   struct obj_data *obj, int spellnum, int casttype);
void mag_room(int level, struct char_data *ch, struct obj_data *obj,
              int spellnum, int casttype);

int call_magic(struct char_data *caster, struct char_data *cvict,
               struct obj_data *ovict, int spellnum, int metamagic, int level, int casttype);
void mag_objectmagic(struct char_data *ch, struct obj_data *obj,
                     char *argument);
int cast_spell(struct char_data *ch, struct char_data *tch,
               struct obj_data *tobj, int spellnum, int metamagic);

/* other prototypes */
void spell_level(int spell, int chclass, int level);
void init_spell_levels(void);
const char *skill_name(int num);
const char *spell_name(int num);
int valid_mortal_tele_dest(struct char_data *ch, room_rnum dest, bool is_tele);

/* spells.c */
bool check_wall(struct char_data *victim, int dir);
void effect_charm(struct char_data *ch, struct char_data *victim,
                  int spellnum, int casttype, int level);
bool is_wall_spell(int spellnum);

/* From magic.c */
int compute_mag_saves(struct char_data *vict,
                      int type, int modifier);
int mag_savingthrow(struct char_data *ch, struct char_data *vict,
                    int type, int modifier, int casttype, int level, int school);
int mag_savingthrow_full(struct char_data *ch, struct char_data *vict,
                         int type, int modifier, int casttype, int level, int school, int spellnum);
void affect_update(void);
int mag_resistance(struct char_data *ch, struct char_data *vict, int modifier);
int compute_spell_res(struct char_data *ch, struct char_data *vict, int mod);
int aoeOK(struct char_data *ch, struct char_data *tch, int spellnum);
bool process_healing(struct char_data *ch, struct char_data *victim, int spellnum, int healing, int move, int psp);

// Sorcerer Bloodline Types
#define SORC_BLOODLINE_NONE 0
#define SORC_BLOODLINE_DRACONIC 1
#define SORC_BLOODLINE_ARCANE 2
#define SORC_BLOODLINE_FEY 3
#define SORC_BLOODLINE_UNDEAD 4
#define NUM_SORC_BLOODLINES 5 // 1 more than the last above

/**********************/
/* spellbook_scroll.c */
/**********************/

/* spellbook functions */
void display_scroll(struct char_data *ch, struct obj_data *obj);
void display_spells(struct char_data *ch, struct obj_data *obj, int mode);
bool spell_in_book(struct obj_data *obj, int spellnum);
int spell_in_scroll(struct obj_data *obj, int spellnum);
bool spellbook_ok(struct char_data *ch, int spellnum, int class, bool check_scroll);
/* spellbook commands */
ACMD_DECL(do_scribe);

/* from spell_parser.c */
ACMD_DECL(do_gen_cast);
#define SCMD_CAST_SPELL 0 /* don't forget to add to constants.c */
#define SCMD_CAST_PSIONIC 1
#define SCMD_CAST_EXTRACT 2
#define SCMD_CAST_SHADOW 3
ACMD_DECL(do_manifest);
void display_shadowcast_spells(struct char_data *ch);

ACMD_DECL(do_abort);
void unused_spell(int spl);
void mag_assign_spells(void);
void resetCastingData(struct char_data *ch);
int lowest_spell_level(int spellnum);
bool is_spell_mind_affecting(int snum);

/* magic.c */
bool isSummonMob(int vnum);

sbyte isHighElfCantrip(struct char_data *ch, int spellnum);
sbyte canCastAtWill(struct char_data *ch, int spellnum);
sbyte isLunarMagic(struct char_data *ch, int spellnum);
sbyte isWarlockMagic(struct char_data *ch, int spellnum);
sbyte isDrowMagic(struct char_data *ch, int spellnum);
sbyte isTieflingMagic(struct char_data *ch, int spellnum);
sbyte isDuergarMagic(struct char_data *ch, int spellnum);
sbyte isForestGnomeMagic(struct char_data *ch, int spellnum);
sbyte isAasimarMagic(struct char_data *ch, int spellnum);
sbyte isNaturalIllusion(struct char_data *ch, int spellnum);
sbyte isPrimordialMagic(struct char_data *ch, int spellnum);
sbyte isFaeMagic(struct char_data *ch, int spellnum);

/**/

/* Global variables exported */
#ifndef __SPELL_PARSER_C__

extern struct spell_info_type spell_info[];
extern struct spell_info_type skill_info[];
extern struct wall_information wallinfo[];
extern char cast_arg2[];
extern char cast_arg3[];
extern const char *unused_spellname;

#endif /* __SPELL_PARSER_C__ */

#endif /* _SPELLS_H_ */
