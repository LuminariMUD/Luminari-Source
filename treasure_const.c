/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\                                                             
/  Luminari Treasure System, Inspired by D20mud's Treasure System                                                           
/  Created By: Zusuk, original d20 code written by Gicker                                                           
\                                                             
/  using treasure.h as the header file currently                                                           
\         todo: CP system by Ornir                                               
/                                                                                                                                                                                       
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "treasure.h"

/* for ease of use, added defines with number of items
   in each array
 * SO if you add something here, make sure to modify the 
 * defines in treasure.h */

const char *gemstones[NUM_A_GEMSTONES + 1] = {
    "onyx", //0
    "obsidian",
    "amber",
    "amethyst",
    "opal",
    "fire opal", //5
    "ruby",
    "emerald",
    "sapphire",
    "diamond",
    "agate", //10
    "citrine",
    "coral",
    "quartz",
    "peridot",
    "pearl", //15
    "malachite",
    "lapis lazuli",
    "jade",
    "jasper",
    "fire agate", //20
    "sphene",
    "spinel",
    "sunstone",
    "tigers eye",
    "topaz", //25
    "turquoise",
    "\n"};
//27

const char *ring_descs[NUM_A_RING_DESCS + 1] = {
    "ring", //0
    "ring",
    "band",
    "\n"};
//3

const char *wrist_descs[NUM_A_WRIST_DESCS + 1] = {
    "bracer", //0
    "bracer",
    "bracer",
    "bracelet",
    "bracelet",
    "bracelet", //5
    "armband",
    "bangle",
    "armlet",
    "charm",
    "\n"};
//10

const char *neck_descs[NUM_A_NECK_DESCS + 1] = {
    "necklace", //0
    "necklace",
    "necklace",
    "necklace",
    "pendant",
    "pendant", //5
    "amulet",
    "amulet",
    "chain",
    "chain",
    "choker", //10
    "gorget",
    "collar",
    "locket",
    "\n"};
//14

const char *head_descs[NUM_A_HEAD_DESCS + 1] = {
    "helmet", //0
    "helmet",
    "helm",
    "helm",
    "greathelm",
    "greathelm", //5
    "tiara",
    "crown",
    "cap",
    "cap",
    "hat", //10
    "hat",
    "hood",
    "hood",
    "hood",
    "cowl", //15
    "headband",
    "\n"};
//17

const char *hands_descs[NUM_A_HAND_DESCS + 1] = {
    "gauntlets", //0
    "gloves",
    "gauntlets",
    "gloves",
    "\n"};
//4

const char *monk_glove_descs[NUM_A_MONK_GLOVE_DESCS + 1] = {
    "fingerless gloves", //0
    "monk gloves",
    "sparring gloves",
    "martial artist handwraps",
    "\n"};
//4

const char *cloak_descs[NUM_A_CLOAK_DESCS + 1] = {
    "cloak", //0
    "cloak",
    "cloak",
    "shroud",
    "cape",
    "\n"};
//5

const char *waist_descs[NUM_A_WAIST_DESCS + 1] = {
    "belt", //0
    "belt",
    "belt",
    "girdle",
    "girdle",
    "sash", //5
    "\n"};
//6

const char *boot_descs[NUM_A_BOOT_DESCS + 1] = {
    "boots", //0
    "boots",
    "boots",
    "boots",
    "boots",
    "sandals", //5
    "moccasins",
    "shoes",
    "knee-high boots",
    "riding boots",
    "slippers", //10
    "\n"};
//11

const char *blade_descs[NUM_A_BLADE_DESCS + 1] = {
    "serrated", //0
    "barbed",
    "sharp",
    "razor-sharp",
    "gem-encrusted",
    "jewel-encrusted", //5
    "fine-edged",
    "finely-forged",
    "grooved",
    "ancient",
    "elven-crafted", //10
    "dwarven-crafted",
    "magnificent",
    "fancy",
    "ceremonial",
    "shining", //15
    "glowing",
    "gleaming",
    "scorched",
    "exquisite",
    "anointed", //20
    "thick-bladed",
    "crescent-bladed",
    "rune-etched",
    "blackened",
    "slim", //25
    "curved",
    "glittering",
    "wavy-bladed",
    "dual-edged",
    "ornate", //30
    "brutal",
    "\n"};
//32

const char *piercing_descs[NUM_A_PIERCING_DESCS + 1] = {
    "barbed", //0
    "sharp",
    "needle-sharp",
    "gem-encrusted",
    "jewel-encrusted",
    "fine-pointed", //5
    "finely-forged",
    "grooved",
    "ancient",
    "elven-crafted",
    "dwarven-crafted", //10
    "magnificent",
    "fancy",
    "ceremonial",
    "shining",
    "glowing", //15
    "gleaming",
    "scorched",
    "exquisite",
    "thick-pointed",
    "tri-pointed", //20
    "rune-etched",
    "blackened",
    "slim",
    "glittering",
    "anointed", //25
    "dual-pointed",
    "ornate",
    "brutal",
    "\n"};
//29

const char *blunt_descs[NUM_A_BLUNT_DESCS + 1] = {
    "gem-encrusted", //0
    "jewel-encrusted",
    "finely-forged",
    "grooved",
    "ancient",
    "elven-crafted", //5
    "dwarven-crafted",
    "magnificent",
    "fancy",
    "ceremonial",
    "shining", //10
    "glowing",
    "gleaming",
    "scorched",
    "exquisite",
    "thick-headed", //15
    "crescent-headed",
    "rune-etched",
    "blackened",
    "massive",
    "glittering", //20
    "dual-headed",
    "brutal",
    "sturdy",
    "ornate",
    "notched", //25
    "spiked",
    "wickedly-spiked",
    "cruel-looking",
    "anointed",
    "\n"};
//30

const char *colors[NUM_A_COLORS + 1] = {
    "amber", //0
    "amethyst",
    "azure",
    "black",
    "blue",
    "brown", //5
    "cerulean",
    "cobalt",
    "copper",
    "crimson",
    "cyan", //10
    "emerald",
    "forest-green",
    "gold",
    "grey",
    "green", //15
    "indigo",
    "ivory",
    "jade",
    "lavender",
    "magenta", //20
    "malachite",
    "maroon",
    "midnight-blue",
    "navy-blue",
    "ochre", //25
    "olive",
    "orange",
    "pink",
    "powder-blue",
    "purple", //30
    "red",
    "royal-blue",
    "sapphire",
    "scarlet",
    "sepia", //35
    "silver",
    "slate-grey",
    "steel-blue",
    "tan",
    "turquoise", //40
    "ultramarine",
    "violet",
    "white",
    "yellow",
    "\n"};
//45

const char *crystal_descs[NUM_A_CRYSTAL_DESCS + 1] = {
    "sparkling", //0
    "shimmering",
    "iridescent",
    "mottled",
    "cloudy",
    "grainy", //5
    "thick",
    "thin",
    "glowing",
    "radiant",
    "glittering", //10
    "incandescent",
    "effulgent",
    "scintillating",
    "murky",
    "opaque", //15
    "shadowy",
    "tenebrous",
    "\n"};
//18

const char *potion_descs[NUM_A_POTION_DESCS + 1] = {
    "sparkling", //0
    "shimmering",
    "iridescent",
    "mottled",
    "cloudy",
    "milky", //5
    "grainy",
    "clumpy",
    "thick",
    "viscous",
    "bubbly", //10
    "thin",
    "watery",
    "oily",
    "coagulated",
    "gelatinous", //15
    "diluted",
    "syrupy",
    "gooey",
    "fizzy",
    "glowing", //20
    "radiant",
    "glittering",
    "incandescent",
    "effulgent",
    "scintillating", //25
    "murky",
    "opaque",
    "shadowy",
    "tenebrous",
    "\n"};
//30

const char *armor_special_descs[NUM_A_ARMOR_SPECIAL_DESCS + 1] = {
    "spiked", //0
    "engraved",
    "ridged",
    "charred",
    "jeweled",
    "elaborate", //5
    "ceremonial",
    "expensive",
    "battered",
    "shadowy",
    "gleaming", //10
    "iridescent",
    "shining",
    "ancient",
    "glowing",
    "glittering", //15
    "exquisite",
    "magnificent",
    "dwarven-made",
    "elven-made",
    "gnomish-made", //20
    "finely-made",
    "gem-encrusted",
    "gold-laced",
    "silver-laced",
    "platinum-laced", //25
    "\n"};
//26

const char *ammo_descs[NUM_A_AMMO_DESCS + 1] = {
    "spiked", //0
    "engraved",
    "ridged",
    "charred",
    "jeweled",
    "elaborate", //5
    "ceremonial",
    "expensive",
    "battered",
    "shadowy",
    "gleaming", //10
    "iridescent",
    "shining",
    "ancient",
    "glowing",
    "glittering", //15
    "exquisite",
    "magnificent",
    "dwarven-made",
    "elven-made",
    "gnomish-made", //20
    "finely-made",
    "gem-encrusted",
    "gold-laced",
    "silver-laced",
    "platinum-laced", //25
    "\n"};
//26

const char *ammo_head_descs[NUM_A_AMMO_HEAD_DESCS + 1] = {
    "spiked", //0
    "sharp",
    "vicious",
    "extremely sharp",
    "jagged",
    "serrated", //5
    "barbed",
    "jewel-encrusted",
    "grooved",
    "thick-pointed",
    "tri-pointed", //10
    "\n"};
//11

const char *armor_crests[NUM_A_ARMOR_CRESTS + 1] = {
    "falcon", //0
    "dragon",
    "rose",
    "sword",
    "crown",
    "lily", //5
    "thorn",
    "skull",
    "shield",
    "mantis",
    "infinity-loop", //10
    "broken merchant scale",
    "bison",
    "phoenix",
    "white circle",
    "feather", //15
    "open book",
    "forging hammer",
    "griffon wing",
    "multicolored flame",
    "green and gold tree", //20
    "red-eyed hood",
    "red circle",
    "black circle",
    "five-headed dragon",
    "condor", //25
    "turtle shell",
    "cross",
    "open eye",
    "lightning bolt",
    "mounted horse", //30
    "dripping dagger",
    "jester's mask",
    "heart",
    "diamond",
    "pegasus", //35
    "unicorn",
    "battle axe",
    "bow and arrow",
    "\n"};
//39

const char *handle_types[NUM_A_HANDLE_TYPES + 1] = {
    "handle", //0
    "shaft",
    "hilt",
    "strap",
    "grip",
    "handle", //5
    "\n"};
//6

const char *head_types[NUM_A_HEAD_TYPES + 1] = {
    "headed", //0
    "bladed",
    "headed",
    "pointed",
    "bowed",
    "strapped", //5
    "lengthened",
    "meshed",
    "chained",
    "fisted",
    "\n"};
//10
