TODO:
*
*
*

Weapon Resize Charts:
F      D      T       S     M      L       H      G      C      ?      ??
1d2 -> 1d3 -> 1d4 -> 1d6 -> 1d8 -> 2d6  -> 3d6 -> 4d6 -> 6d6 -> 8d6 -> 12d6
1d1 -> 2d1 -> 2d3 -> 1d7 -> 2d4 -> 1d12 -> 4d4 -> 6d4 -> 5d8 -> 6d8 -> 8d10
       3d1 -> 2d2 -> 3d2 -> 1d9 -> 1d10 -> 2d8 -> 3d8 -> 4d8 -> 8d7 -> 9d8

Connection between materials and skills
 * Hard metal -> Mining
 * Leather -> Hunting
 * Wood -> Foresting
 * Cloth -> Knitting
 * Crystals / Essences -> Chemistry

crafting related skills:
#define SKILL_MINING                    471  //implemented
  used for acquiring hard metals
#define SKILL_HUNTING                   472  //implemented
  used for acquiring leather, dragonhide
#define SKILL_FORESTING                 473  //implemented
  used for acquiring wood, darkwood
#define SKILL_KNITTING                  474  //implemented
  used for acquiring cloth, creating cloth armor
#define SKILL_CHEMISTRY                 475  //implemented
  used for processing crystal, essences
#define SKILL_ARMOR_SMITHING            476  //implemented
  used for creating armor metal armor
#define SKILL_WEAPON_SMITHING           477  //implemented
  used for creating weapons
#define SKILL_JEWELRY_MAKING            478  //implemented
  used for creating 'miscellaneous' worn pieces
#define SKILL_LEATHER_WORKING           479  //implemented
  used for creating non-metal armor
#define SKILL_FAST_CRAFTER              480  //implemented
  increases speed of all crafting related events
#define SKILL_BONE_ARMOR                481
  can create metal armor using bone
#define SKILL_ELVEN_CRAFTING            482
  can make lighter armor
#define SKILL_MASTERWORK_CRAFTING       483
  higher chance of "crit"ting (creating rare, legendary, mythic)
#define SKILL_DRACONIC_CRAFTING         484
  ?  - higher bonus without affecting level?
#define SKILL_DWARVEN_CRAFTING          485
  can craft using rare heavy metals

/*
 * Our current list of materials distributed in this manner:
 METALS (hard)
 * bronze
 * iron
 * steel
 * cold iron
 * alchemal silver
 * mithril
 * adamantine
 METALS (precious)
 * copper
 * brass
 * silver
 * gold
 * platinum
 LEATHERS
 * leather
 * dragonhide
 WOODS
 * wood
 * darkwood
 CLOTH
 * burlap
 * hemp
 * cotton
 * wool
 * velvet
 * satin
 * silk
 */

Crafting Commands:
augment - combine essence to make them stronger
convert - convert 10 of one mat to make something else ?
restring - rename an object
autocraft - crafting quest, supply order
resize - resize object
disenchant - create essence
create - create/craft an object
checkcraft - check the result of create