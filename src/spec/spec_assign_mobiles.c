/**************************************************************************
 *  File: spec/spec_assign_mobiles.c                  Part of LuminariMUD *
 *  Usage: Compiled mobile special-procedure assignment inventory.         *
 *                                                                         *
 *  All rights reserved. See license for complete information.             *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"

#include "act.h"
#include "character/backgrounds.h"
#include "character/guild_services.h"
#include "character/vampire_cloak.h"
#include "comms/mail.h"
#include "craft/craft.h"
#include "craft/crafting_molds.h"
#include "craft/crafting_new.h"
#include "magic/spellbook_scroll.h"
#include "obj/objsave.h"
#include "obj/player_shop.h"
#include "obj/treasure.h"
#include "obj/vendor.h"
#include "quest/hunts.h"
#include "quest/missions.h"
#include "quest/quest_services.h"
#include "spec_assign.h"
#include "spec_assign_internal.h"
#include "spec_mobile_archetypes.h"
#include "spec_mobiles.h"
#include "spec_rooms.h"
#include "spec_zone_abyss.h"
#include "spec_zone_abyssal_vortex.h"
#include "spec_zone_agrach_dyrr.h"
#include "spec_zone_air_plane.h"
#include "spec_zone_bandit_castle.h"
#include "spec_zone_banshee.h"
#include "spec_zone_battlemaze.h"
#include "spec_zone_celestial_leviathan.h"
#include "spec_zone_crimson_flame.h"
#include "spec_zone_earth_plane.h"
#include "spec_zone_feybranche.h"
#include "spec_zone_fire_giant.h"
#include "spec_zone_fire_plane.h"
#include "spec_zone_flaming_tower.h"
#include "spec_zone_hive_of_passion.h"
#include "spec_zone_illithid_enclave.h"
#include "spec_zone_jot.h"
#include "spec_zone_kenjin_tower.h"
#include "spec_zone_kings_castle.h"
#include "spec_zone_kobold_caverns.h"
#include "spec_zone_longsaddle.h"
#include "spec_zone_mad_drow.h"
#include "spec_zone_menzoberranzan.h"
#include "spec_zone_mere_of_dead_men.h"
#include "spec_zone_orc_ruins.h"
#include "spec_zone_prisoner.h"
#include "spec_zone_secomber.h"
#include "spec_zone_shadow_dragon.h"
#include "spec_zone_shobalar.h"
#include "spec_zone_snake_pit.h"
#include "spec_zone_ttf.h"
#include "spec_zone_water_plane.h"
#include "spec_zone_zusuk.h"

#define SPEC_ASSIGN_STRINGIFY_INNER(value) #value
#define SPEC_ASSIGN_STRINGIFY(value) SPEC_ASSIGN_STRINGIFY_INNER(value)
#define SPEC_ASSIGN_LOCATION "src/spec/spec_assign_mobiles.c:" SPEC_ASSIGN_STRINGIFY(__LINE__)

#define ASSIGNMOB(mob, handler) spec_assign_mobile((mob), (handler), #handler, SPEC_ASSIGN_LOCATION)
#define ASSIGNOBJ(obj, handler) spec_assign_object((obj), (handler), #handler, SPEC_ASSIGN_LOCATION)

/* Assignments */

/* assign special procedures to mobiles. Guildguards, snake, thief, wizard,
 * puff, fido, janitor, and cityguards are now implemented via triggers. */
void assign_mobiles(void)
{
#ifdef CAMPAIGN_FR

  // Luskan Market
  ASSIGNMOB(3276, buyweapons);
  ASSIGNMOB(3275, buyarmor);

  // Luskan Host Tower
  ASSIGNMOB(3278, buyweapons);
  ASSIGNMOB(3277, buyarmor);
  ASSIGNMOB(3076, eqstats);

  // Luskan Misc.
  ASSIGNMOB(3077, huntsmaster);
  ASSIGNMOB(3044, crafting_quest);

  // Silverymoon
  ASSIGNMOB(6000, buyweapons);
  ASSIGNMOB(6002, buyweapons);
  ASSIGNMOB(6006, buyweapons);
  ASSIGNMOB(6003, buyarmor);

  // Mirabar
  ASSIGNMOB(4825, buyweapons);
  ASSIGNMOB(4824, buyarmor);

  // Longsaddle
  ASSIGNMOB(305, buyweapons);
  ASSIGNMOB(304, buyarmor);
  ASSIGNMOB(303, pet_shops);

  // Triboar
  ASSIGNMOB(7021, buymolds);
  ASSIGNMOB(7022, faction_mission);
  ASSIGNMOB(7023, huntsmaster);
#elif defined(CAMPAIGN_DL)
  // palanthas
  ASSIGNMOB(2427, buyweapons);
  ASSIGNMOB(2428, buyarmor);
  ASSIGNMOB(2430, buyweapons);
  ASSIGNMOB(2429, buyarmor);
  ASSIGNMOB(15325, faction_mission);
  // ASSIGNMOB(15321, krynn_supply_orders);
  ASSIGNMOB(15322, buymolds);
  ASSIGNMOB(7021, identify_mob);
  ASSIGNMOB(15326, huntsmaster);
  ASSIGNMOB(15327, replace_quest_item);
  ASSIGNMOB(15378, temple);

  // sanction
  ASSIGNMOB(13800, buyweapons);
  ASSIGNMOB(13801, buyarmor);
  ASSIGNMOB(13802, buyweapons);
  ASSIGNMOB(13803, buyarmor);
  ASSIGNMOB(13810, faction_mission);
  ASSIGNMOB(13808, crafting_quest);
  ASSIGNMOB(13809, buymolds);
  ASSIGNMOB(13822, identify_mob);
  ASSIGNMOB(13811, huntsmaster);
  ASSIGNMOB(13821, replace_quest_item);
  ASSIGNMOB(6505, temple);

#else

  /* vampire mobs (spec to do vampire-like abilities) */
  ASSIGNMOB(29906, vampire_mob);  /* erich - master vampire */
  ASSIGNMOB(29241, vampire_mob);  /* tiersten */
  ASSIGNMOB(26115, vampire_mob);  /* vampiress */
  ASSIGNMOB(157709, vampire_mob); /* young vampires */
  ASSIGNMOB(120004, vampire_mob); /* a vampire */
  ASSIGNMOB(121763, vampire_mob); /* a feeding vampire */
  ASSIGNMOB(101047, vampire_mob); /* a vicious vampire */
  ASSIGNMOB(125503, vampire_mob); /* a vampirical ixzan */
  ASSIGNMOB(110608, vampire_mob); /* a dwarven vampire */
  ASSIGNMOB(196052, vampire_mob); /* hel's emissary */
  ASSIGNMOB(117032, vampire_mob); /* a vampire */
  ASSIGNMOB(117026, vampire_mob); /* a vampire */
  ASSIGNMOB(117014, vampire_mob); /* a vampire */
  ASSIGNMOB(117012, vampire_mob); /* zarkathan */

  // mosswood
  ASSIGNMOB(145391, buyweapons);
  ASSIGNMOB(145392, buyarmor);
  ASSIGNMOB(145394, eqstats);

  // ashenport
  ASSIGNMOB(103499, buyarmor);   // +1 armor
  ASSIGNMOB(103498, buyweapons); // +1 weapons
  ASSIGNMOB(103801, huntsmaster);
  // ASSIGNMOB(103802, buyarmor); // +2 armor - mob #103802 doesn't exist
  ASSIGNMOB(103803, buyweapons); // +2 weapons

  /* faction mission system */
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
  ASSIGNMOB(196200, jot_invasion_loader);
  ASSIGNMOB(196027, thrym);
  ASSIGNMOB(196077, planetar);
  ASSIGNMOB(196070, ymir);
  ASSIGNMOB(196033, gatehouse_guard);
  ASSIGNMOB(196032, gatehouse_guard);

  /* fire giant zones */
  ASSIGNMOB(34699, fg_invasion_loader);

  /* more assigns unsorted */
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
  /* Aurgloroasa (110600) is a shadow dragon that became a dracolich; the
   * dracolich_mob assignment below is the effective one.  The shadowdragon
   * callback was silently overwritten here, so it is no longer assigned. */

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

  /*Labyrinth*/
  ASSIGNMOB(115008, phantom);

  /*Mithril Hall Palace*/
  ASSIGNMOB(126332, lich_mob); /* tharger */

  /*Lizard Marsh*/
  ASSIGNMOB(121210, lich_mob); /* redeye */

  /*Deep Caverns*/
  ASSIGNMOB(136903, lich_mob); /* Azcre */

  /* more lich! */
  ASSIGNMOB(180300, lich_mob); /* an undead creature */
  ASSIGNMOB(157502, lich_mob); /* an ancient lich */
  ASSIGNMOB(135517, lich_mob); /* the lichdrow Dyrr */
  ASSIGNMOB(114933, lich_mob); /* Thak Neleth */
  ASSIGNMOB(109505, lich_mob); /* Mortanen */
  ASSIGNMOB(107850, lich_mob); /* Aumvor, the Lichlord */
  ASSIGNMOB(105783, lich_mob); /* Rekalogh, the Archwizard of Gracklstugh */
  ASSIGNMOB(29239, lich_mob);  /* the Liche */
  ASSIGNMOB(24712, lich_mob);  /* the demilich */
  ASSIGNMOB(114124, lich_mob); /* the Banelich */

  /*Longsaddle*/
  ASSIGNMOB(106807, receptionist);
  ASSIGNMOB(106827, receptionist);

  /* assign harpell with loop */
  int j;
  for (j = 106830; j <= 106863; j++)
  {
    if (j == 106838 || (j > 106846 && j < 106856))
      continue;
    else
      ASSIGNMOB(j, harpell);
  }
  /* end harpell assign loop */

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

  /* totem animals */
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

  /* fire plane invoker */
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

  /* big baddie - the prisoner */
  ASSIGNMOB(113751, prisoner_dracolich);
  ASSIGNMOB(113750, the_prisoner);
  ASSIGNMOB(13700, celestial_leviathan);

  /* dracolich mobs */
  ASSIGNMOB(138703, dracolich_mob); /* Neremeezder */
  ASSIGNMOB(110600, dracolich_mob); /* Aurgloroasa */
  ASSIGNMOB(31102, dracolich_mob);  /* a dracolich */
  ASSIGNMOB(29905, dracolich_mob);  /* Wyrenthoth */
  ASSIGNMOB(5010, dracolich_mob);   /* the dracolich */
  ASSIGNMOB(138419, dracolich_mob); /* Daurgothoth */

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
#endif
}

/* eof */
