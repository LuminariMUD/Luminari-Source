/**************************************************************************
 *  File: spec_assign.c                                Part of LuminariMUD *
 *  Usage: Functions to assign function pointers to objs/mobs/rooms        *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "spec_procs.h"
#include "ban.h" /* for SPECIAL(gen_board) */
#include "boards.h"
#include "mail.h"
#include "treasure.h"
#include "missions.h"
#include "hunts.h"

SPECIAL_DECL(questmaster);
SPECIAL_DECL(shop_keeper);
SPECIAL_DECL(buyweapons);
SPECIAL_DECL(buyarmor);
SPECIAL_DECL(faction_mission);
SPECIAL_DECL(eqstats);
SPECIAL_DECL(vampire_cloak);

/* local (file scope only) functions */
static void ASSIGNROOM(room_vnum room, SPECIAL_DECL(fname));
static void ASSIGNMOB(mob_vnum mob, SPECIAL_DECL(fname));
static void ASSIGNOBJ(obj_vnum obj, SPECIAL_DECL(fname));

/* functions to perform assignments */
static void ASSIGNMOB(mob_vnum mob, SPECIAL_DECL(fname))
{
  mob_rnum rnum;

  if ((rnum = real_mobile(mob)) != NOBODY)
    mob_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

static void ASSIGNOBJ(obj_vnum obj, SPECIAL_DECL(fname))
{
  obj_rnum rnum;

  if ((rnum = real_object(obj)) != NOTHING)
    obj_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

static void ASSIGNROOM(room_vnum room, SPECIAL_DECL(fname))
{
  room_rnum rnum;

  if ((rnum = real_room(room)) != NOWHERE)
    world[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}

/* Assignments */

/* assign special procedures to mobiles. Guildguards, snake, thief, wizard,
 * puff, fido, janitor, and cityguards are now implemented via triggers. */
void assign_mobiles(void)
{

  // mosswood
  ASSIGNMOB(145391, buyweapons);
  ASSIGNMOB(145392, buyarmor);
  ASSIGNMOB(145394, eqstats);

  // ashenport
  ASSIGNMOB(103499, buyarmor);
  ASSIGNMOB(103498, buyweapons);
  ASSIGNMOB(103801, huntsmaster);

  ASSIGNMOB(103698, faction_mission);

  assign_kings_castle();

  /* cryogenicist */
  ASSIGNMOB(3095, cryogenicist);

  /* guildmasters */
  ASSIGNMOB(120, guild);
  ASSIGNMOB(121, guild);
  ASSIGNMOB(122, guild);
  ASSIGNMOB(123, guild);
  /* female newbie trainer sanctus */
  ASSIGNMOB(196, guild);
  ASSIGNMOB(2556, guild);
  ASSIGNMOB(2559, guild);
  ASSIGNMOB(2562, guild);
  ASSIGNMOB(2564, guild);
  ASSIGNMOB(2800, guild);
  ASSIGNMOB(3013, guild);
  ASSIGNMOB(3020, guild);
  ASSIGNMOB(3021, guild);
  ASSIGNMOB(3022, guild);
  ASSIGNMOB(3023, guild);
  ASSIGNMOB(5400, guild);
  ASSIGNMOB(5401, guild);
  ASSIGNMOB(5402, guild);
  ASSIGNMOB(5403, guild);
  ASSIGNMOB(11518, guild);
  ASSIGNMOB(14105, guild);
  /* female newbie trainer newbie school */
  ASSIGNMOB(23411, guild);
  ASSIGNMOB(25720, guild);
  ASSIGNMOB(25721, guild);
  ASSIGNMOB(25722, guild);
  ASSIGNMOB(25723, guild);
  ASSIGNMOB(25726, guild);
  ASSIGNMOB(25732, guild);
  ASSIGNMOB(27572, guild);
  ASSIGNMOB(27573, guild);
  ASSIGNMOB(27574, guild);
  ASSIGNMOB(27575, guild);
  ASSIGNMOB(27721, guild);
  ASSIGNMOB(29204, guild);
  ASSIGNMOB(29227, guild);
  ASSIGNMOB(31601, guild);
  ASSIGNMOB(31603, guild);
  ASSIGNMOB(31605, guild);
  ASSIGNMOB(31607, guild);
  ASSIGNMOB(31609, guild);
  ASSIGNMOB(31611, guild);
  ASSIGNMOB(31639, guild);
  ASSIGNMOB(31641, guild);
  /* female newbie trainer mosswood village */
  ASSIGNMOB(145333, guild);

  /* player owned shop mobiles */
  ASSIGNMOB(899, player_owned_shops); /* example shop */
  ASSIGNMOB(822, player_owned_shops); /* zusuk created shop for melaw */
  ASSIGNMOB(825, player_owned_shops); /* thazull created shop for Ellyanor */
  ASSIGNMOB(830, player_owned_shops); /* Ickthak the Kobold - for Thimblethorp */
  ASSIGNMOB(836, player_owned_shops); /* Towering Woman - for Brondo */

  /* mayors */
  ASSIGNMOB(3105, mayor);

  /* postmasters */
  ASSIGNMOB(110, postmaster);
  ASSIGNMOB(1201, postmaster);
  ASSIGNMOB(3010, postmaster);
  ASSIGNMOB(10412, postmaster);
  ASSIGNMOB(10719, postmaster);
  ASSIGNMOB(23496, postmaster);
  ASSIGNMOB(25710, postmaster);
  ASSIGNMOB(27164, postmaster);
  ASSIGNMOB(30128, postmaster);
  ASSIGNMOB(31510, postmaster);
  ASSIGNMOB(103010, postmaster);
  ASSIGNMOB(145293, postmaster); /*wolves assigned this*/

  /* receptionists */
  ASSIGNMOB(1200, receptionist);
  ASSIGNMOB(3005, receptionist);
  ASSIGNMOB(5404, receptionist);
  ASSIGNMOB(27713, receptionist);
  ASSIGNMOB(27730, receptionist);

  /* walls */
  ASSIGNMOB(47, wall);
  ASSIGNMOB(90, wall);

  /* hounds */
  ASSIGNMOB(49, hound);

  /* abyss randomizer */
  ASSIGNMOB(142300, abyss_randomizer);

  /* crimson flame zone mob specs */
  ASSIGNMOB(106040, cf_trainingmaster); // training master
  ASSIGNMOB(106000, cf_alathar);        // lord alathar

  /* Jotunheim */
  ASSIGNMOB(196027, thrym);
  ASSIGNMOB(196077, planetar);
  ASSIGNMOB(196070, ymir);
  ASSIGNMOB(196033, gatehouse_guard);
  ASSIGNMOB(196032, gatehouse_guard);
  ASSIGNMOB(196200, jot_invasion_loader); // this will load invasion

  /* more homeland assigns, unsorted */
  ASSIGNMOB(200002, postmaster);
  ASSIGNMOB(200001, receptionist);
  /* not yet defined
  ASSIGNMOB(200000, guild_golem);
   */
  ASSIGNMOB(155699, cube_slider);

  // ASSIGNMOB(126907, receptionist);

  /* Immortal Zone */
  ASSIGNMOB(101200, receptionist);
  ASSIGNMOB(101201, postmaster);
  ASSIGNMOB(101202, janitor);

  /* Nice dogs. */
  // ASSIGNMOB(100028, dog);
  ASSIGNMOB(100103, dog);
  ASSIGNMOB(102102, dog);
  ASSIGNMOB(103030, dog);
  ASSIGNMOB(103506, dog);
  ASSIGNMOB(103517, dog);
  ASSIGNMOB(103532, dog);
  ASSIGNMOB(104958, dog);
  ASSIGNMOB(106852, dog);
  ASSIGNMOB(108145, dog);
  ASSIGNMOB(111345, dog);
  ASSIGNMOB(117930, dog);
  ASSIGNMOB(118506, dog);
  ASSIGNMOB(119197, dog);
  ASSIGNMOB(126621, dog);

  /* Trade master mobs */
  /* not yet defined
  ASSIGNMOB(122000, trade_master);
  ASSIGNMOB(122001, trade_master);
  ASSIGNMOB(122002, trade_master);
  ASSIGNMOB(122003, trade_master);
  ASSIGNMOB(122004, trade_master);
  ASSIGNMOB(122005, trade_master);
  ASSIGNMOB(122006, trade_master);
  ASSIGNMOB(122007, trade_master);
  ASSIGNMOB(122008, trade_master);
  ASSIGNMOB(122009, trade_master);
  ASSIGNMOB(122010, trade_master);
   */
  /* trade object mobs */
  /* not yet defined
  ASSIGNOBJ(122000, trade_object);
  ASSIGNOBJ(122001, trade_object);
  ASSIGNOBJ(122002, trade_object);
  ASSIGNOBJ(122003, trade_object);
  ASSIGNOBJ(122004, trade_object);
  ASSIGNOBJ(122005, trade_object);
  ASSIGNOBJ(122006, trade_object);
  ASSIGNOBJ(122007, trade_object);
  ASSIGNOBJ(122008, trade_object);
  ASSIGNOBJ(122009, trade_object);
  ASSIGNOBJ(122010, trade_object);
  ASSIGNOBJ(122011, trade_object);
  ASSIGNOBJ(122012, trade_object);
  ASSIGNOBJ(122013, trade_object);
  ASSIGNOBJ(122014, trade_object);
  ASSIGNOBJ(122015, trade_object);
  ASSIGNOBJ(122016, trade_object);
  ASSIGNOBJ(122017, trade_object);
  ASSIGNOBJ(122018, trade_object);
  ASSIGNOBJ(122019, trade_object);
  ASSIGNOBJ(122020, trade_object);
  ASSIGNOBJ(122021, trade_object);
  ASSIGNOBJ(122022, trade_object);
  ASSIGNOBJ(122023, trade_object);
  ASSIGNOBJ(122024, trade_object);
  ASSIGNOBJ(122025, trade_object);
  ASSIGNOBJ(122026, trade_object);
  ASSIGNOBJ(122027, trade_object);
  ASSIGNOBJ(122028, trade_object);
  ASSIGNOBJ(122029, trade_object);
  ASSIGNOBJ(122030, trade_object);
  ASSIGNOBJ(122031, trade_object);
  ASSIGNOBJ(122032, trade_object);
   */

  /* Trade Bandit mobs */
  /* not yet defined
  ASSIGNMOB(122030, trade_bandit);
  ASSIGNMOB(122031, trade_bandit);
  ASSIGNMOB(122032, trade_bandit);
  ASSIGNMOB(122033, trade_bandit);
  ASSIGNMOB(122034, trade_bandit);
  ASSIGNMOB(122035, trade_bandit);
  ASSIGNMOB(122036, trade_bandit);
  ASSIGNMOB(122037, trade_bandit);
  ASSIGNMOB(122038, trade_bandit);
  ASSIGNMOB(122039, trade_bandit);
  ASSIGNMOB(122040, trade_bandit);
  ASSIGNMOB(122041, trade_bandit);
  ASSIGNMOB(122042, trade_bandit);
  ASSIGNMOB(122043, trade_bandit);
  ASSIGNMOB(122044, trade_bandit);
  ASSIGNMOB(122045, trade_bandit);
  ASSIGNMOB(122046, trade_bandit);
  ASSIGNMOB(122047, trade_bandit);
  ASSIGNMOB(122048, trade_bandit);
   */

  /* bandit guard */
  ASSIGNMOB(143304, bandit_guard);

  /* Waterdeep*/
  ASSIGNMOB(103001, receptionist);
  ASSIGNMOB(103010, postmaster);
  ASSIGNMOB(103200, guild_guard); // Anti-Paladin
  ASSIGNMOB(103201, guild);       // Anti-Paladin
  ASSIGNMOB(103202, guild_guard); // Assassin
  ASSIGNMOB(103203, guild);       // Assassin
  ASSIGNMOB(103204, guild_guard); // Bard
  ASSIGNMOB(103205, guild);       // Bard
  ASSIGNMOB(103027, guild_guard); // Berzerker/Warrior
  ASSIGNMOB(103023, guild);       // Berzerker/Warrior
  ASSIGNMOB(103025, guild_guard); // Cleric/Shaman
  ASSIGNMOB(103021, guild);       // Cleric/Shaman
  ASSIGNMOB(103024, guild_guard); // Conjurer/Necromancer/Sorcerer
  ASSIGNMOB(103020, guild);       // Conjurer/Necromancer/Sorcerer
  ASSIGNMOB(103206, guild_guard); // Druid
  ASSIGNMOB(103207, guild);       // Druid
  ASSIGNMOB(103208, guild_guard); // Monk
  ASSIGNMOB(103209, guild);       // Monk
  ASSIGNMOB(103210, guild_guard); // Paladin
  ASSIGNMOB(103211, guild);       // Paladin
  ASSIGNMOB(103900, guild_guard); // Ranger
  ASSIGNMOB(103901, guild);       // Ranger
  ASSIGNMOB(103026, guild_guard); // Thief
  ASSIGNMOB(103022, guild);       // Thief

  /*Evereska*/
  ASSIGNMOB(127564, guild); // warrior
  ASSIGNMOB(127563, guild); // mage
  ASSIGNMOB(127566, guild); // thief
  ASSIGNMOB(127567, guild); // bard
  ASSIGNMOB(127568, guild); // cleric
  ASSIGNMOB(127565, guild); // ranger
  ASSIGNMOB(127575, guild); // druid
  ASSIGNMOB(127545, receptionist);
  ASSIGNMOB(127618, bank);
  ASSIGNMOB(122696, postmaster);

  /* orc Ruins */
  ASSIGNMOB(106231, shar_statue);
  ASSIGNOBJ(106229, shar_heart);

  /*Zhentil Keep*/
  ASSIGNMOB(119101, receptionist);
  ASSIGNMOB(119103, guild);
  ASSIGNMOB(119182, guild);
  ASSIGNMOB(119168, guild);
  ASSIGNMOB(119104, guild);
  ASSIGNMOB(119175, guild);
  ASSIGNMOB(119189, guild);
  ASSIGNMOB(119185, guild);
  ASSIGNMOB(119224, guild);
  ASSIGNMOB(119183, guild_guard);
  ASSIGNMOB(119181, guild_guard);
  ASSIGNMOB(119169, guild_guard);
  ASSIGNMOB(119170, guild_guard);
  ASSIGNMOB(119172, guild_guard);
  ASSIGNMOB(119188, guild_guard);
  ASSIGNMOB(119184, guild_guard);
  ASSIGNMOB(119102, bank);

  /*Moradins newbie zone*/
  ASSIGNMOB(114721, duergar_guard);

  /*Illithid Enclave*/
  ASSIGNMOB(126928, illithid_gguard);
  ASSIGNMOB(126904, guild);
  // ASSIGNMOB(126906, bank);
  // ASSIGNMOB(129607, receptionist);

  /*Secomber*/
  ASSIGNMOB(125064, secomber_guard);
  ASSIGNMOB(125088, bank);

  /* Grunwald*/
  ASSIGNMOB(117462, guild);
  ASSIGNMOB(117460, guild);
  ASSIGNMOB(117458, guild);
  ASSIGNMOB(117461, guild_guard);
  ASSIGNMOB(117459, guild_guard);
  ASSIGNMOB(117457, guild_guard);
  ASSIGNMOB(117463, receptionist);

  /*Bloodfist cavern*/
  ASSIGNMOB(102505, receptionist);
  ASSIGNMOB(102536, bank);
  ASSIGNMOB(102547, postmaster);
  // ASSIGNMOB(102643, guild);
  ASSIGNMOB(102546, guild);
  ASSIGNMOB(102522, guild);
  ASSIGNMOB(102521, guild);
  ASSIGNMOB(102518, guild_guard);
  ASSIGNMOB(102506, guild_guard);
  ASSIGNMOB(102507, guild_guard);

  /* Wild-Elves */
  ASSIGNMOB(105395, guild);
  ASSIGNMOB(105396, guild);
  ASSIGNMOB(105397, guild);

  /* Oak Valley */
  ASSIGNMOB(105391, guild);
  ASSIGNMOB(105390, guild);
  ASSIGNMOB(105392, guild);
  ASSIGNMOB(105393, guild);
  ASSIGNMOB(105394, guild);
  ASSIGNMOB(107100, receptionist);

  /* ZZ */
  ASSIGNMOB(138816, receptionist);
  ASSIGNMOB(138809, bank);
  ASSIGNMOB(138833, guild);
  ASSIGNMOB(138834, guild);
  ASSIGNMOB(138835, guild);
  ASSIGNMOB(138800, guild);
  ASSIGNMOB(138801, guild);
  ASSIGNMOB(138802, guild);
  ASSIGNMOB(138803, guild);
  ASSIGNMOB(138804, guild);
  ASSIGNMOB(138805, guild);
  ASSIGNMOB(138826, guild);

  /* Gracklstugh */
  ASSIGNMOB(105691, guild); // Sorcerer
  ASSIGNMOB(105662, guild); // Fighters
  ASSIGNMOB(105776, guild); // Rogues
  ASSIGNMOB(105772, guild); // Clerics
  ASSIGNMOB(105782, receptionist);
  ASSIGNMOB(105807, bank);
  ASSIGNMOB(105812, postmaster);

  /* Broken Tusk Village */
  ASSIGNMOB(125957, guild);       // Sorcerer
  ASSIGNMOB(125952, guild);       // Fighters
  ASSIGNMOB(125953, guild);       // Rogues
  ASSIGNMOB(125958, guild);       // Clerics
  ASSIGNMOB(125955, guild_guard); // sorc
  ASSIGNMOB(125950, guild_guard); // warrior
  ASSIGNMOB(125987, receptionist);
  ASSIGNMOB(125984, bank);

  /*Thunderholme*/
  ASSIGNMOB(110600, shadowdragon);

  /* Mercenaries */
  ASSIGNMOB(104300, mercenary);
  ASSIGNMOB(104301, mercenary);
  ASSIGNMOB(104302, mercenary);
  ASSIGNMOB(104303, mercenary);
  ASSIGNMOB(104304, mercenary);
  ASSIGNMOB(104305, mercenary);
  ASSIGNMOB(104306, mercenary);
  ASSIGNMOB(104307, mercenary);

  /*Hardbuckler*/
  ASSIGNMOB(118551, bank);
  ASSIGNMOB(118519, receptionist);
  ASSIGNMOB(119900, guild);
  ASSIGNMOB(118513, guild);
  ASSIGNMOB(118514, guild);
  ASSIGNMOB(118515, guild);
  ASSIGNMOB(118517, guild);
  ASSIGNMOB(118522, guild_guard);
  ASSIGNMOB(118523, guild_guard);
  ASSIGNMOB(118524, guild_guard);
  ASSIGNMOB(118525, guild_guard);
  ASSIGNMOB(119902, guild_guard);
  ASSIGNMOB(118552, guild_guard);

  /*Mithril Hall*/
  ASSIGNMOB(108177, guild);
  ASSIGNMOB(108198, bank);
  ASSIGNMOB(108181, bank);
  ASSIGNMOB(108182, receptionist);
  ASSIGNMOB(108190, guild);
  ASSIGNMOB(108184, guild);
  ASSIGNMOB(108208, guild);

  /*Ethereal plane*/
  ASSIGNMOB(129602, planewalker);

  /*Mere*/
  ASSIGNMOB(126717, mereshaman);
  ASSIGNMOB(126707, willowisp);
  ASSIGNMOB(126715, willowisp);

  /*Snake pit*/
  ASSIGNMOB(132712, naga_golem);
  ASSIGNMOB(132700, naga);
  ASSIGNMOB(132701, naga);
  ASSIGNMOB(132702, naga);
  ASSIGNMOB(132716, naga);

  /*Serpent Hills*/
  ASSIGNMOB(132630, naga);
  ASSIGNMOB(132631, naga);
  ASSIGNMOB(132632, naga);
  ASSIGNMOB(132633, naga);

  /*Arabel*/
  ASSIGNMOB(121475, bank);
  ASSIGNMOB(121503, receptionist);

  /*Elven settlement*/
  ASSIGNMOB(106405, receptionist);

  /*Neverwinter*/
  ASSIGNMOB(122632, receptionist);
  ASSIGNMOB(122685, bank);

  /*Corm Orp*/
  ASSIGNMOB(105030, receptionist);
  ASSIGNMOB(105001, guild);
  ASSIGNMOB(105007, guild);
  ASSIGNMOB(105012, guild);
  ASSIGNMOB(105017, guild);
  ASSIGNMOB(105003, guild_guard);
  ASSIGNMOB(105011, guild_guard);
  ASSIGNMOB(105045, postmaster);

  /*Tughrak Gol*/
  ASSIGNMOB(110404, guild);
  ASSIGNMOB(110411, guild);
  ASSIGNMOB(110413, guild);
  ASSIGNMOB(110412, receptionist);
  ASSIGNMOB(110421, bank);

  /* BANKS */
  ASSIGNMOB(121825, bank);
  ASSIGNMOB(103007, bank);
  ASSIGNMOB(110421, bank);
  ASSIGNMOB(113010, bank);
  ASSIGNMOB(105039, bank);

  /*Mithril Hall Palace*/
  ASSIGNMOB(126332, lichdrain);

  /*Labyrinth*/
  ASSIGNMOB(115008, phantom);

  /*Lizard Marsh*/
  ASSIGNMOB(121210, lichdrain);

  /*Deep Caverns*/
  ASSIGNMOB(136903, lichdrain);

  /*Longsaddle*/
  ASSIGNMOB(106807, receptionist);
  ASSIGNMOB(106827, receptionist);

  int j;
  for (j = 106830; j <= 106863; j++)
  {
    if (j == 106838 || (j > 106846 && j < 106856))
      continue;
    else
      ASSIGNMOB(j, harpell);
  }

  /*Ashabenford*/
  ASSIGNMOB(113701, receptionist);

  /*Beregost*/
  ASSIGNMOB(121822, receptionist);
  ASSIGNMOB(121825, bank);

  /*Tilverton*/
  ASSIGNMOB(111373, receptionist);
  ASSIGNMOB(111374, bank);

  /* OTHER */
  ASSIGNMOB(101264, ethereal_pet);
  ASSIGNMOB(101260, shades);
  ASSIGNMOB(101225, wraith_elemental);
  ASSIGNMOB(101207, wraith_elemental);
  ASSIGNMOB(101206, solid_elemental);
  ASSIGNMOB(101208, solid_elemental);
  ASSIGNMOB(101209, wraith_elemental);
  ASSIGNMOB(100502, wraith);
  ASSIGNMOB(100503, vampire);
  ASSIGNMOB(100504, vampire);
  ASSIGNMOB(100506, vampire);
  ASSIGNMOB(100507, bonedancer);
  ASSIGNMOB(100501, skeleton_zombie);
  // ASSIGNMOB(100011, skeleton_zombie);

  ASSIGNMOB(101400, totemanimal);
  ASSIGNMOB(101401, totemanimal);
  ASSIGNMOB(101402, totemanimal);
  ASSIGNMOB(101404, totemanimal);
  ASSIGNMOB(101405, totemanimal);
  ASSIGNMOB(101406, totemanimal);
  ASSIGNMOB(101408, totemanimal);
  ASSIGNMOB(101409, totemanimal);
  ASSIGNMOB(101410, totemanimal);
  ASSIGNMOB(101412, totemanimal);
  ASSIGNMOB(101413, totemanimal);
  ASSIGNMOB(101414, totemanimal);

  ASSIGNMOB(112501, fp_invoker);

  /*Menzo*/
  ASSIGNMOB(135200, gromph);
  ASSIGNMOB(135301, guild_guard);
  ASSIGNMOB(135302, guild);
  ASSIGNMOB(135304, guild);
  ASSIGNMOB(135306, guild);
  ASSIGNMOB(135309, guild);
  ASSIGNMOB(135034, receptionist);
  ASSIGNMOB(135048, receptionist);
  ASSIGNMOB(135051, bank);
  ASSIGNMOB(135500, shobalar);
  ASSIGNMOB(135504, shobalar);
  ASSIGNMOB(135518, agrachdyrr);
  ASSIGNMOB(135530, feybranche);
  ASSIGNMOB(135603, battlemaze_guard);

  ASSIGNMOB(136702, ogremoch);

  ASSIGNMOB(106000, cf_alathar);
  ASSIGNMOB(106040, cf_trainingmaster);

  ASSIGNMOB(145146, ttf_monstrosity);
  ASSIGNMOB(145116, ttf_abomination);
  ASSIGNMOB(145182, ttf_rotbringer);
  ASSIGNMOB(145189, ttf_patrol);

  ASSIGNMOB(113751, dracolich);
  ASSIGNMOB(113750, the_prisoner);

  /* Tower of Kenjin */
  ASSIGNMOB(132910, kt_kenjin);

  ASSIGNMOB(112600, wallach);
  ASSIGNMOB(112607, beltush);
  ASSIGNMOB(100580, imix);
  ASSIGNMOB(100508, practice_dummy);
  ASSIGNMOB(100509, practice_dummy);
  ASSIGNMOB(142300, abyss_randomizer);
  ASSIGNMOB(109718, banshee);

  ASSIGNMOB(106230, banshee);
  ASSIGNMOB(136300, olhydra);
  // ASSIGNMOB(136100, yan);
  ASSIGNMOB(136105, chan);

  ASSIGNMOB(100581, fzoul);
}

/* assign special procedures to objects */
void assign_objects(void)
{
  ASSIGNOBJ(1226, gen_board);   /* builder's board */
  ASSIGNOBJ(1227, gen_board);   /* staff board */
  ASSIGNOBJ(1228, gen_board);   /* advertising board */
  ASSIGNOBJ(3096, gen_board);   /* social board */
  ASSIGNOBJ(3097, gen_board);   /* freeze board */
  ASSIGNOBJ(3098, gen_board);   /* immortal board */
  ASSIGNOBJ(3099, gen_board);   /* mortal board */
  ASSIGNOBJ(100400, gen_board); /* quest board */
  ASSIGNOBJ(103093, gen_board); /* ashenport market board */
  ASSIGNOBJ(103094, gen_board); /* forger board */
  ASSIGNOBJ(103095, gen_board); /* areas board */
  ASSIGNOBJ(103096, gen_board); /* social board */
  ASSIGNOBJ(103097, gen_board); /* freeze board */
  ASSIGNOBJ(103098, gen_board); /* immortal board */
  ASSIGNOBJ(103099, gen_board); /* mortal board */

  ASSIGNOBJ(115, bank);
  ASSIGNOBJ(334, bank);  /* atm */
  ASSIGNOBJ(336, bank);  /* cashcard */
  ASSIGNOBJ(3034, bank); /* atm */
  ASSIGNOBJ(3036, bank); /* cashcard */
  ASSIGNOBJ(3907, bank);
  ASSIGNOBJ(10640, bank);
  ASSIGNOBJ(10751, bank);
  ASSIGNOBJ(25758, bank);
  ASSIGNOBJ(102541, bank);
  ASSIGNOBJ(103122, bank);
  /* homeland - need to be converted to objects */
  /*
    ASSIGNOBJ(105039, bank);
    ASSIGNOBJ(105807, bank);
    ASSIGNOBJ(108181, bank);
    ASSIGNOBJ(108198, bank);
    ASSIGNOBJ(110421, bank);
    ASSIGNOBJ(111374, bank);
    ASSIGNOBJ(113010, bank);
    ASSIGNOBJ(119102, bank);
    ASSIGNOBJ(121475, bank);
    ASSIGNOBJ(121825, bank);
    ASSIGNOBJ(122685, bank);
    ASSIGNOBJ(125088, bank);
    ASSIGNOBJ(125984, bank);
    ASSIGNOBJ(126906, bank);
    ASSIGNOBJ(127618, bank);
    ASSIGNOBJ(135051, bank);
    ASSIGNOBJ(138809, bank);
   */

  ASSIGNOBJ(3118, crafting_kit);

  ASSIGNOBJ(128106, ches); // weapon

  ASSIGNOBJ(128150, spikeshield); // shield

  ASSIGNOBJ(224, monk_glove); /*electric damage*/
  ASSIGNOBJ(9215, monk_glove_cold);

  /* the prisoner */
  ASSIGNOBJ(132125, tia_rapier);
  // ASSIGNOBJ(132109, magi_staff);
  ASSIGNOBJ(132104, star_circlet);
  ASSIGNOBJ(132101, malevolence);
  ASSIGNOBJ(132128, speed_gaunts);
  ASSIGNOBJ(132126, rune_scimitar);
  ASSIGNOBJ(132300, celestial_sword);
  ASSIGNOBJ(132133, stability_boots);
  ASSIGNOBJ(132118, ancient_moonblade);

  ASSIGNOBJ(132115, warbow);

  /* not yet defined? */
  // ASSIGNOBJ(133103, mithril_rapier);
  // ASSIGNOBJ(141800, treantshield);

  ASSIGNOBJ(136100, air_sphere); // weapon (lightning)

  /* JOTUNHEIM EQ */
  ASSIGNOBJ(196012, mistweave);
  ASSIGNOBJ(196000, frostbite);
  ASSIGNOBJ(196059, ymir_cloak);
  ASSIGNOBJ(196062, vaprak_claws);
  ASSIGNOBJ(196056, valkyrie_sword);
  ASSIGNOBJ(196081, twilight);
  ASSIGNOBJ(196090, fake_twilight);
  ASSIGNOBJ(196066, giantslayer);
  ASSIGNOBJ(196073, planetar_sword);

  /* more homeland, unsorted */
  ASSIGNOBJ(100400, gen_board); /* quest board */

  ASSIGNOBJ(123419, neverwinter_button_control);
  ASSIGNOBJ(123418, neverwinter_valve_control);

  ASSIGNOBJ(113803, nutty_bracer);

  /* Moving Portals */
  ASSIGNOBJ(106019, floating_teleport);

  ASSIGNOBJ(110015, floating_teleport);

  ASSIGNOBJ(112500, floating_teleport);

  ASSIGNOBJ(126703, floating_teleport);
  ASSIGNOBJ(126712, floating_teleport);
  ASSIGNOBJ(126713, floating_teleport);
  ASSIGNOBJ(126714, floating_teleport);
  ASSIGNOBJ(126715, floating_teleport);

  ASSIGNOBJ(129015, floating_teleport);

  ASSIGNOBJ(129500, floating_teleport);
  ASSIGNOBJ(129501, floating_teleport);
  ASSIGNOBJ(129502, floating_teleport);

  ASSIGNOBJ(136400, floating_teleport);

  ASSIGNOBJ(139200, floating_teleport);
  ASSIGNOBJ(139201, floating_teleport);
  ASSIGNOBJ(139202, floating_teleport);
  ASSIGNOBJ(139203, floating_teleport);

  /* ferry, ferry-like */
  ASSIGNOBJ(104072, chionthar_ferry);
  ASSIGNOBJ(126429, alandor_ferry);
  ASSIGNOBJ(120010, md_carpet);

  /* purchased pet objects */
  ASSIGNOBJ(118190, bought_pet);
  ASSIGNOBJ(103670, bought_pet);
  ASSIGNOBJ(103671, bought_pet);
  ASSIGNOBJ(103672, bought_pet);
  ASSIGNOBJ(103673, bought_pet);
  ASSIGNOBJ(103674, bought_pet);

  ASSIGNOBJ(VAMPIRE_CLOAK_OBJ_VNUM, vampire_cloak);

  /* not yet defined
  ASSIGNOBJ(101290, storage_chest);
  ASSIGNOBJ(101291, storage_chest);
   */

  /* not yet defined? */
  // ASSIGNOBJ(100600, forest_idol);
  // ASSIGNOBJ(100601, forest_idol);
  // ASSIGNOBJ(100602, forest_idol);
  // ASSIGNOBJ(100603, forest_idol);
  // ASSIGNOBJ(100604, forest_idol);
  // ASSIGNOBJ(100605, forest_idol);

  /* Weapon Procs */
  ASSIGNOBJ(141914, witherdirk);
  ASSIGNOBJ(135511, snakewhip);
  ASSIGNOBJ(135500, snakewhip);
  ASSIGNOBJ(135534, snakewhip);
  ASSIGNOBJ(135199, acidsword);
  ASSIGNOBJ(100510, halberd);
  ASSIGNOBJ(100513, halberd);

  ASSIGNOBJ(114838, rughnark);
  ASSIGNOBJ(139900, magma);
  ASSIGNOBJ(110601, bolthammer);
  ASSIGNOBJ(111507, prismorb);
  ASSIGNOBJ(129602, flamingwhip);
  ASSIGNOBJ(126315, dorfaxe);
  ASSIGNOBJ(121207, helmblade);
  ASSIGNOBJ(117014, bloodaxe);
  ASSIGNOBJ(100501, xvim_artifact);
  ASSIGNOBJ(100502, xvim_normal);

  ASSIGNOBJ(109802, whisperwind);
  ASSIGNOBJ(127224, sparksword);
  ASSIGNOBJ(100581, tyrantseye);
  ASSIGNOBJ(113898, flaming_scimitar);
  ASSIGNOBJ(113897, frosty_scimitar);
  ASSIGNOBJ(129011, purity);
  ASSIGNOBJ(117024, etherealness);
  ASSIGNOBJ(129001, greatsword);
  ASSIGNOBJ(125519, sarn);
  ASSIGNOBJ(115003, fog_dagger);
  ASSIGNOBJ(115007, dragonbone_hammer);
  ASSIGNOBJ(126704, viperdagger);
  ASSIGNOBJ(126717, acidstaff);
  ASSIGNOBJ(132102, hellfire);
  ASSIGNOBJ(110017, vengeance);
  ASSIGNOBJ(101199, vengeance);
  ASSIGNOBJ(101849, skullsmasher);
  ASSIGNOBJ(101850, skullsmasher);
  ASSIGNOBJ(139250, courage);
  ASSIGNOBJ(139251, courage);
  ASSIGNOBJ(121456, clang_bracer);
  ASSIGNOBJ(128150, spikeshield);
  ASSIGNOBJ(128106, ches);
  ASSIGNOBJ(100596, tormblade);
  ASSIGNOBJ(100599, tormblade);

  ASSIGNOBJ(138447, disruption_mace);
  ASSIGNOBJ(138415, haste_bracers);
  ASSIGNOBJ(135626, menzo_chokers);
  ASSIGNOBJ(135627, menzo_chokers);
  ASSIGNOBJ(106021, angel_leggings);
  ASSIGNOBJ(135535, spiderdagger);

  /* clouds realm */
  ASSIGNOBJ(144669, dragon_robes);
}

/* assign special procedures to rooms */
void assign_rooms(void)
{
  room_rnum i;

  /* bazaar - spend quest points on magic gear */
  ASSIGNROOM(103006, bazaar);

  /* crafting quest (autocraft) */
  ASSIGNROOM(370, crafting_quest);

  /* wizard library - research wizard spells for spellbook */
  ASSIGNROOM(5905, wizard_library);   /* wizard training mansion */
  ASSIGNROOM(103047, wizard_library); /* Ashenport Mage's Guild */

  /* buy pets */
  ASSIGNROOM(3031, pet_shops);
  ASSIGNROOM(10738, pet_shops);
  ASSIGNROOM(23281, pet_shops);
  ASSIGNROOM(25722, pet_shops);
  ASSIGNROOM(27155, pet_shops);
  ASSIGNROOM(27616, pet_shops);
  ASSIGNROOM(31523, pet_shops);
  /* this doesn't seem to be at all valid */
  // ASSIGNROOM(103031, pet_shops);
  ASSIGNROOM(145287, pet_shops); /* mosswood petshop */

  /* abyssal vortex */
  ASSIGNROOM(139200, abyssal_vortex);
  ASSIGNROOM(139201, abyssal_vortex);
  ASSIGNROOM(139202, abyssal_vortex);
  ASSIGNROOM(139203, abyssal_vortex);
  ASSIGNROOM(139204, abyssal_vortex);
  ASSIGNROOM(139205, abyssal_vortex);
  ASSIGNROOM(139206, abyssal_vortex);
  ASSIGNROOM(139207, abyssal_vortex);
  ASSIGNROOM(139208, abyssal_vortex);
  ASSIGNROOM(139209, abyssal_vortex);
  ASSIGNROOM(139210, abyssal_vortex);
  ASSIGNROOM(139211, abyssal_vortex);
  ASSIGNROOM(139212, abyssal_vortex);
  ASSIGNROOM(139213, abyssal_vortex);
  ASSIGNROOM(139214, abyssal_vortex);
  ASSIGNROOM(139215, abyssal_vortex);
  ASSIGNROOM(139216, abyssal_vortex);
  ASSIGNROOM(139217, abyssal_vortex);
  ASSIGNROOM(139218, abyssal_vortex);
  ASSIGNROOM(139219, abyssal_vortex);
  ASSIGNROOM(139210, abyssal_vortex);
  ASSIGNROOM(139221, abyssal_vortex);
  ASSIGNROOM(139222, abyssal_vortex);
  ASSIGNROOM(139223, abyssal_vortex);
  ASSIGNROOM(139224, abyssal_vortex);
  ASSIGNROOM(139225, abyssal_vortex);
  ASSIGNROOM(139226, abyssal_vortex);
  ASSIGNROOM(139227, abyssal_vortex);
  ASSIGNROOM(139228, abyssal_vortex);
  ASSIGNROOM(139229, abyssal_vortex);
  ASSIGNROOM(139230, abyssal_vortex);
  ASSIGNROOM(139231, abyssal_vortex);
  ASSIGNROOM(139232, abyssal_vortex);
  ASSIGNROOM(139233, abyssal_vortex);
  ASSIGNROOM(139234, abyssal_vortex);
  ASSIGNROOM(139235, abyssal_vortex);
  ASSIGNROOM(139236, abyssal_vortex);
  ASSIGNROOM(139237, abyssal_vortex);
  ASSIGNROOM(139238, abyssal_vortex);
  ASSIGNROOM(139239, abyssal_vortex);
  ASSIGNROOM(139240, abyssal_vortex);
  ASSIGNROOM(139241, abyssal_vortex);
  ASSIGNROOM(139242, abyssal_vortex);
  ASSIGNROOM(139243, abyssal_vortex);
  ASSIGNROOM(139244, abyssal_vortex);
  ASSIGNROOM(139245, abyssal_vortex);
  ASSIGNROOM(139246, abyssal_vortex);
  ASSIGNROOM(139247, abyssal_vortex);
  ASSIGNROOM(139248, abyssal_vortex);
  ASSIGNROOM(139249, abyssal_vortex);
  ASSIGNROOM(139250, abyssal_vortex);

  /* hive death */
  ASSIGNROOM(139300, hive_death);

  /* kt twister */
  ASSIGNROOM(132902, kt_twister);
  ASSIGNROOM(132903, kt_twister);
  ASSIGNROOM(132904, kt_twister);
  ASSIGNROOM(132905, kt_twister);

  /* kt shadowmaker */
  // ASSIGNROOM( 32921, kt_shadowmaker);

  /* quicksand */
  ASSIGNROOM(126771, quicksand);
  ASSIGNROOM(126776, quicksand);
  ASSIGNROOM(126752, quicksand);
  ASSIGNROOM(126710, quicksand);
  ASSIGNROOM(126716, quicksand);
  ASSIGNROOM(126731, quicksand);
  ASSIGNROOM(126870, quicksand);
  ASSIGNROOM(126871, quicksand);
  ASSIGNROOM(126887, quicksand);
  ASSIGNROOM(126831, quicksand);
  ASSIGNROOM(126840, quicksand);
  ASSIGNROOM(126848, quicksand);
  ASSIGNROOM(126788, quicksand);
  ASSIGNROOM(126793, quicksand);
  ASSIGNROOM(126800, quicksand);

  /* death traps are dumps, i.e. will destroy all gear that hits the ground */
  if (CONFIG_DTS_ARE_DUMPS)
    for (i = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
        world[i].func = dump;
}

struct spec_func_data
{
  const char *name;
  SPECIAL_DECL(*func);
  const char *description;
};

/* spec proc list */
/** !!MAKE SURE TO ADD TO: spec_procs.h!!!  **/
static const struct spec_func_data spec_func_list[] = {

    /* a-c */
    {"Abyss Randomizer", abyss_randomizer, ""},
    {"Abyssal Vortex", abyssal_vortex, ""},
    {"Acid Staff", acidstaff, ""},
    {"Acid Sword", acidsword, ""},
    {"Agrachdyrr", agrachdyrr, ""},
    {"Air Sphere", air_sphere, ""},
    {"Alandor Ferry", alandor_ferry, ""},
    {"Angel Leggings", angel_leggings, ""},
    {"Bandit Guard", bandit_guard, ""},
    {"Bank", bank, ""},
    {"Banshee", banshee, ""},
    {"Battlemaze Guard", battlemaze_guard, ""},
    {"Beltush", beltush, ""},
    {"BloodAxe", bloodaxe, ""},
    {"Pet Object", bought_pet, ""},
    {"BoltHammer", bolthammer, ""},
    {"Bone Dancer", bonedancer, ""},
    {"Boots of Stability", stability_boots, ""},
    {"Alathar", cf_alathar, ""},
    {"Chan", chan, ""},
    {"Ches", ches, ""},
    {"Chionthar Ferry", chionthar_ferry, ""},
    {"Circlet of the Stars", star_circlet, ""},
    {"Cityguard", cityguard, ""},
    {"Clang Bracer", clang_bracer, ""},
    {"Courage", courage, ""},
    {"Crafting Kit", crafting_kit, ""},
    {"Crafting Quest", crafting_quest, ""},
    {"Cryogenicist", cryogenicist, ""},
    {"Cube Slider", cube_slider, ""},

    /* d-f */
    {"Disruption Mace", disruption_mace, ""},
    {"Dog", dog, ""},
    {"Dorf Axe", dorfaxe, ""},
    {"Dracolich", dracolich, ""},
    {"Dragon Robes", dragon_robes, ""},
    {"Dragonbone Hammer", dragonbone_hammer, ""},
    //{"Drow Scimitar", drow_scimitar, ""},
    {"Duergar Guard", duergar_guard, ""},
    {"Dump", dump, ""},
    {"Ethereal Pet", ethereal_pet, ""},
    {"Etherealness", etherealness, ""},
    {"FeyBranche", feybranche, ""},
    {"Fake Twilight", fake_twilight, ""},
    {"Fido", fido, ""},
    {"Flaming Scimitar", flaming_scimitar, ""},
    {"Flaming Whip", flamingwhip, ""},
    {"Floating Teleport", floating_teleport, ""},
    {"Fog Dagger", fog_dagger, ""},
    //{"Forest Idol", forest_idol, ""},
    {"Invoker", fp_invoker, ""},
    {"Frostbite", frostbite, ""},
    {"Frosty Scimitar", frosty_scimitar, ""},
    {"Fzoul", fzoul, ""},

    /* g-i */
    {"Gatehouse Guard", gatehouse_guard, ""},
    {"Gauntlets of Speed", speed_gaunts, ""},
    {"Bulletin Board", gen_board, ""},
    {"Giantslayer", giantslayer, ""},
    {"Greatsword", greatsword, ""},
    {"Gromph", gromph, ""},
    {"Guild", guild, ""},
    //{"Guild Golem", guild_golem, ""},
    {"Guild Guard", guild_guard, ""},
    {"Guildmaster", guild, ""},
    {"Halberd", halberd, ""},
    {"Harpell", harpell, ""},
    {"Haste Bracers", haste_bracers, ""},
    {"Hellfire", hellfire, ""},
    {"HelmBlade", helmblade, ""},
    {"Hive Death", hive_death, ""},
    {"Faithful Hound", hound, ""},
    {"Illithid Guard", illithid_gguard, ""},
    {"Imix", imix, ""},

    /* j-l */
    {"Janitor", janitor, ""},
    {"Invasion", jot_invasion_loader, ""},
    {"Kenjin", kt_kenjin, ""},
    //{"ShadowMaker", kt_shadowmaker, ""},
    {"Twister", kt_twister, ""},
    {"LichDrain", lichdrain, ""},

    /* m-o */
    {"Magma", magma, ""},
    {"Malevolence", malevolence, ""},
    {"Mayor", mayor, ""},
    //{"MD Carpet", md_carpet, ""},
    {"Menzo Choker", menzo_chokers, ""},
    {"Mercenary", mercenary, ""},
    {"Mere Shaman", mereshaman, ""},
    {"Mistweave", mistweave, ""},
    //{"Mithril Rapier", mithril_rapier, ""},
    {"Monk Shock Gloves", monk_glove, ""},
    {"Monk Frost Gloves", monk_glove_cold, ""},
    {"Ancient Moonblade", ancient_moonblade, ""},
    {"Naga", naga, ""},
    {"Naga Golem", naga_golem, ""},
    {"NW Button Control", neverwinter_button_control, ""},
    {"NW Valve Control", neverwinter_valve_control, ""},
    {"Nutty Bracer", nutty_bracer, ""},
    {"Ogremoch", ogremoch, ""},
    {"Olhydra", olhydra, ""},

    /* p-r */
    {"Pet Shop", pet_shops, ""},
    {"Planetar", planetar, ""},
    {"Planetar Sword", planetar_sword, ""},
    {"PlaneWalker", planewalker, ""},
    {"Player Shop", player_owned_shops, ""},
    {"Postmaster", postmaster, ""},
    {"Practice Dummy", practice_dummy, ""},
    {"PrismOrb", prismorb, ""},
    {"Puff", puff, ""},
    {"Purity", purity, ""},
    {"Questmaster", questmaster, ""},
    {"Quicksand", quicksand, ""},
    {"Receptionist", receptionist, ""},
    {"Rughnark", rughnark, ""},

    /* s-u */
    {"Sarn", sarn, ""},
    {"Shades", shades, ""},
    {"ShadowDragon", shadowdragon, ""},
    {"Shar Heart", shar_heart, ""},
    {"Shar Statue", shar_statue, ""},
    {"Runed Scimitar", rune_scimitar, ""},
    {"Shobalar", shobalar, ""},
    {"Shopkeeper", shop_keeper, ""},
    {"Secomber Guard", secomber_guard, ""},
    {"Skeleton Zombie", skeleton_zombie, ""},
    {"SkullSmasher", skullsmasher, ""},
    {"Snake", snake, ""},
    {"Snake Whip", snakewhip, ""},
    {"Solid Elemental", solid_elemental, ""},
    {"SparkSword", sparksword, ""},
    {"SpiderDagger", spiderdagger, ""},
    {"SpikeShield", spikeshield, ""},
    {"Celestial Sword", celestial_sword, ""},
    //{"Staff of the Magi", magi_staff, ""},
    //{"Storage Chest", storage_chest, ""},
    {"Thief", thief, ""},
    {"Thrym", thrym, ""},
    {"Crystal Rapier", tia_rapier, ""},
    {"The Prisoner", the_prisoner, ""},
    {"TormBlade", tormblade, ""},
    {"Totem Animal", totemanimal, ""},
    //{"Trade Bandit", trade_bandit, ""},
    //{"Trade Master", trade_master, ""},
    //{"Trade Object", trade_object, ""},
    {"Trainingmaster", cf_trainingmaster, ""},
    //{"Treant Shield", treantshield, ""},
    {"TTF Abomination", ttf_abomination, ""},
    {"TTF Monstrosity", ttf_monstrosity, ""},
    {"TTF Patrol", ttf_patrol, ""},
    {"TTF RotBringer", ttf_rotbringer, ""},
    {"Twilight", twilight, ""},
    {"Tyrant's Eye", tyrantseye, ""},

    /* v-z */
    {"Valkyrie Sword", valkyrie_sword, ""},
    {"Vampire", vampire, ""},
    {"Vaprak Claws", vaprak_claws, ""},
    {"Vengeance", vengeance, ""},
    {"ViperDagger", viperdagger, ""},
    {"Magical Wall", wall, ""},
    {"Wallach", wallach, ""},
    {"Warbow", warbow, ""},
    {"WhisperWind", whisperwind, ""},
    {"Will O' Wisp", willowisp, ""},
    {"Wither Dirk", witherdirk, ""},
    {"Wizard", wizard, ""},
    {"Wizard Library", wizard_library, ""},
    {"Wraith", wraith, ""},
    {"Wraith Elemental", wraith_elemental, ""},
    {"Xvim Artifact", xvim_artifact, ""},
    {"Xvim", xvim_normal, ""},
    {"Yan", yan, ""},
    {"Ymir", ymir, ""},
    {"Ymir Cloak", ymir_cloak, ""},

    /* this has to be last */
    {
        "\n", NULL, ""}};
/** !!MAKE SURE TO ADD TO: spec_procs.h!!!  **/

/* return the spec's name */
const char *get_spec_func_name(SPECIAL_DECL(*func))
{
  int i;

  for (i = 0; *(spec_func_list[i].name) != '\n'; i++)
  {
    if (func == spec_func_list[i].func)
      return (spec_func_list[i].name);
  }

  return NULL;
}

/*eof*/
