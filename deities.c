#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "db.h"
#include "deities.h"


struct deity_info deity_list[NUM_DEITIES];

#define Y TRUE
#define N FALSE

void init_deities(void)
{

  int i = 0, j = 0;

  for (i = 0; i < NUM_DEITIES; i++) {

    deity_list[i].name = "None";
    deity_list[i].ethos = ETHOS_NEUTRAL;
    deity_list[i].alignment = ALIGNMENT_NEUTRAL;
    for (j = 0; j < 6; j++)
      deity_list[i].domains[j] = DOMAIN_UNDEFINED;
    deity_list[i].favored_weapon = WEAPON_TYPE_UNARMED;
    deity_list[i].pantheon = DEITY_PANTHEON_NONE;
    deity_list[i].portfolio = "Nothing";
    deity_list[i].description = "You do not worship a deity at all for reasons of your own.";
  }

}

void add_deity(int deity, const char *name, int ethos, int alignment, int d1, int d2, int d3, int d4, int d5, int d6, int weapon, int pantheon,
               const char *portfolio, const char *description)
{
  deity_list[deity].name = name;
  deity_list[deity].ethos = ethos;
  deity_list[deity].alignment = alignment;
  deity_list[deity].domains[0] = d1;
  deity_list[deity].domains[1] = d2;
  deity_list[deity].domains[2] = d3;
  deity_list[deity].domains[3] = d4;
  deity_list[deity].domains[4] = d5;
  deity_list[deity].domains[5] = d6;
  deity_list[deity].favored_weapon = weapon;
  deity_list[deity].pantheon = pantheon;
  deity_list[deity].portfolio = portfolio;
  deity_list[deity].description = description;

}

void assign_deities(void) {

  init_deities();

// This is the guide line of 80 characters for writing descriptions of proper
// length.  Cut and paste it where needed then return it here.
// -----------------------------------------------------------------------------


  add_deity(DEITY_NONE, "None", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_ALL, "The Faithless",
	"Those who choose to worship no deity at all are known as the faithless.  It is their\r\n"
	"destiny to become part of the living wall in Kelemvor's domain when they die, to\r\n"
	"ultimately have their very soul devoured and destroyed forever.\r\n");

  add_deity(DEITY_ILMATER, "Ilmater", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_HEALING, DOMAIN_LAW, DOMAIN_STRENGTH, DOMAIN_SUFFERING,
            DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN, "Endurance, Suffering, Martyrdom, Perseverance",
	"Ilmater is a generous and self-sacrificing deity.  He is willing to shoulder any burden for another\r\n"
	"person, whether it ius a heavy load or a terrible pain.  A gentle god, Ilmater is quiet, kind and\r\n"
	"good-spirited.  He appreciates a humorous story and is always slow to anger.  While most consider\r\n"
	"him non-violent, in the face of extreme cruelty or atrocities, his anger rises and his wrath is\r\n"
	"terrible to behold.  He takes great care to reassure and protect children and young creatures, and\r\n"
	"he takes exceptional offense at those that would harm them.\r\n"
	"\r\n"
	"Unlike most other faiths, the church of Ilmater has many saints.  Perceived by most as a puzzling\r\n"
	"crowd of martyrs, the church of Ilmater spends most of its efforts on providing healing to those\r\n"
	"that have been hurt.  It sends its clerics to impoverished areas, places struck by plague, and war-\r\n"
	"torn lands in order to allieviate the suffering of others.\r\n"
	"\r\n"
	"Ilmater's clerics are the most sensative and caring beings in the world.  While some grow cynical\r\n"
	"at all the suffering they see, they still are compelled to help those in need whenever they\r\n"
	"encounter them.  His clerics share whatever they have with the needy and act on behalf of those\r\n"
	"who cannot act for or defend themselves.  Many learn to brew potions so they can help those beyond\r\n"
	"their immediate reach.\r\n"
	"\r\n"
	"Clerics of Ilmater pray for spells in the morning, although they still have to pray to Ilmater at\r\n"
	"least six times per day altogether.  They have no annual holy days, but occasionally a cleric asks\r\n"
	"for a Plea of Rest.  This allows him a tenday of respite from Ilmater's dictates, which prevents the\r\n"
	"cleric from suffering emotiuonal exhaustion, or allows him to perform some act that Ilmater normally\r\n"
	"frowns upon.  One group of monks of Ilmater act as the defenders of the faithful and the church\r\n"
	"temples.  These monks often multiclass as clerics.\r\n");

  add_deity(DEITY_TEMPUS, "Tempus", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS, DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_WAR,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FAERUNIAN, "War, Battle, Warriors",
	"Tempus is random in his support, but his chaotic nature ends up favoring all \r\n"
	"equally in time.  The god of war is liable to back one army one day and another\r\n"
	"one the next.  Soliders of all alignments pray to him for help in coming \r\n"
	"battles.  Mighty and honorable in battle, he answers to his own warrior's code \r\n"
	"and pursues no long-lasting alliances.  He has never been known to speak.  He \r\n"
	"uses the spirits of fallen warriors as intermediares.\r\n"
	"\r\n"
	"The church of Tempus welcomes worshippers of all alignments (though its clerics\r\n"
	"abide by the normal rules), and its temples are more like walled military \r\n"
	"compounds.  Tempus`s clerics are charged to keep warfare a thing of rules and \r\n"
	"respected reputation, minimizing uncontrolled bloodshed and working to end \r\n"
	"pointless extended fueding.  They train themselves and others in battle \r\n"
	"readiness in order to protect civilization from monsters, and they punish those\r\n"
	"who fight dishonorably or with cowardice.  Collecting and venerating the weapons\r\n"
	"of famous and respected warriors is a common practice in Tempus`s church.  \r\n"
	"Clerics are expected to spill a few drops of blood (preferably their own a of a\r\n"
	"worthy foe`s) every tenday.\r\n"
	"\r\n"
	"Tempus`s clerics pray for spells just before highsun.  Most of his clerics tend\r\n"
	"to be battle-minded male humans, although others are welco. Eves and \r\n"
	"anniversaries of great battles important to a local temple are holidays.  The \r\n"
	"Feast of the Moon is the annual day to honor the dead.  Each temple holds a \r\n"
	"Feast of Heroes at highsun and a Song of the Fallen at sunset, and most also \r\n"
	"have a Song of the Sword ceremony for layfolk after dark.\r\n"
	"Tempus's clerics usually multiclass as fighters.\r\n");

  add_deity(DEITY_UTHGAR, "Uthgar", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_ANIMAL, DOMAIN_CHAOS, DOMAIN_RETRIBUTION, DOMAIN_STRENGTH,
            DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FAERUNIAN, "Uthgardt Barabrian Tribes, Physical Strength",
            "\r\n"
            "Uthgar (pronounced UHTH-gar), the Battle Father, is the god of the Uthgardt and\r\n"
            "physical strength as well as, by 1479 DR, an exarch of Tempus.\r\n"
            "\r\n"
            "Worshipers\r\n"
            "\r\n"
            "Uthgar's followers consist of many human tribes collectively termed as the \r\n"
            "Uthgardt barbarians. His clerics often multi-class as barbarians, druids \r\n"
            "or rangers.\r\n"
            "\r\n"
            "Tribes and alignments\r\n"
            "\r\n"
            "The dogma of the Uthgardt religion varies slightly from tribe to tribe as each \r\n"
            "beast cult emphasizes different barbaric virtues. The clerics of these tribes, \r\n"
            "along with those who take Uthgar as a patron deity, must abide by the somewhat \r\n"
            "broader alignment guidelines of the beast totems who mediate between Uthgar and \r\n"
            "his people. Any alignment that fits the guideline for a beast totem is suitable \r\n"
            "for a cleric of Uthgar of that totem. The names of these tribes, along with \r\n"
            "their corresponding totems and alignments, are as follows:\r\n"
            "\r\n"
            "    * Black Lion . chaotic good\r\n"
            "    * Black Raven . chaotic evil\r\n"
            "    * Blue Bear . chaotic evil\r\n"
            "    * Elk . chaotic neutral\r\n"
            "    * Gray Wolf . chaotic neutral\r\n"
            "    * Great Worm . chaotic good\r\n"
            "    * Griffon . neutral\r\n"
            "    * Red Tiger . chaotic neutral\r\n"
            "    * Sky Pony . chaotic neutral\r\n"
            "    * Tree Ghost . neutral good\r\n"
            "    * Thunderbeast . chaotic neutral \r\n"
            "\r\n"
            "Relationships\r\n"
            "\r\n"
            "Uthgar's superior is the Lord of Battles, Tempus.\r\n"
            "\r\n"
            "History\r\n"
            "\r\n"
            "Uthgar became a Demigod when he was elevated by Tempus. Once a mortal \r\n"
            "Northlander he founded a dynasty, and is worshiped by the Uthgardt tribes. \r\n"
            "\r\n");

  add_deity(DEITY_UBTAO, "Ubtao", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_PLANNING, DOMAIN_PLANT, DOMAIN_PROTECTION, DOMAIN_SCALYKIND,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_PICK, DEITY_PANTHEON_FAERUNIAN, "Creation, Jungles, Cult, the Chultans, Dinosaurs",
            "\r\n"
            "Ubtao (pronounced oob-TAY-oh ) is the patron deity of Chult. He is also known as\r\n"
            "The Father of the Dinosaurs, Creator of Chult, Founder of Mezro, and The \r\n"
            "Deceiver. He stays distant from both mortals and other deities, and he seems to \r\n"
            "be above the daily doings of the world and his followers. This may be partly due \r\n"
            "to his origin as a primordial , and in fact it's not fully known if he is a deity\r\n"
            "in the traditional sense. Only since the Time of Troubles has he begun to show \r\n"
            "interest in his followers again. The many jungle spirits worshiped in Chult are \r\n"
            "all aspects of Ubtao.\r\n"
            "\r\n"
            "Contents\r\n"
            "\r\n"
            "    * 1 Worshipers\r\n"
            "    * 2 Rituals\r\n"
            "    * 3 Relationships\r\n"
            "    * 4 History\r\n"
            "    * 5 Notes\r\n"
            "    * 6 References\r\n"
            "\r\n"
            "Worshipers\r\n"
            "\r\n"
            "The Church of Ubtao is split among three wholly independent sects, all based in \r\n"
            "the jungles of Chult among the various clans of the humans. Many of the church \r\n"
            "multiclass as rangers.\r\n"
            "\r\n"
            "Mazewalkers \r\n"
            "\r\n"
            "Mazewalkers are found only in the city of Mezro. They take care of the general \r\n"
            "spiritual welfare of the clan, and try to prepare the faithful for their trek \r\n"
            "through the maze of life. They teach history and lore of Chult to both \r\n"
            "children and adults. Mazewalkers also provide council about important life \r\n"
            "decisions, such as marriages. They are the mediators when inter- and \r\n"
            "intraclan disputes break out, and they are the caretakers of Mezro's laws. \r\n"
            "Mazewalkers are usually clerics.\r\n"
            "\r\n"
            "Spiritlords \r\n"
            "\r\n"
            "Spiritlords live outside the city and seek to ensure a safe passage for their \r\n"
            "clan through the spirit-infested world. Their main goal is to ensure the clan \r\n"
            "does not offend ancestor spirits or elemental deities by missing sacrifices \r\n"
            "and rituals. They also aim for favors from the spirits. Spiritlords are \r\n"
            "usually adepts.\r\n"
            "\r\n"
            "Jungle druids \r\n"
            "\r\n"
            "Jungle druids teach the clans to to fit as best as possible into the web of \r\n"
            "life in the jungle. Most of the time they function as clan healer. Outside \r\n"
            "this they also collect and teach knowledge about plants, animals (including \r\n"
            "dinosaurs) and their behavior. Jungle druids also train the very few \r\n"
            "domesticated animals kept in Chult. Jungle druids are druids.\r\n"
            "Rituals\r\n"
            "\r\n"
            "Both clerics and druids pray for their spells at noon, when Ubtao's \r\n"
            "majesty hangs over all of Chult. There are scores of ceremonies and holy \r\n"
            "days, most particular to dead ancestors, the season or locations that are \r\n"
            "to be visited. Many of these rituals are necessary before performing \r\n"
            "certain activities, such as hunting special animals or burials. The \r\n"
            "small, moveable stone altars which are used for these ceremonies are \r\n"
            "treated just like any other rock in the jungle, at the times they are \r\n"
            "not in use.\r\n"
            "Relationships\r\n"
            "\r\n"
            "Because of Ubtao's distance to the other deities he has only one ally: \r\n"
            "Thard Harr. His biggest foe is Eshowdow. He has only one other enemy: \r\n"
            "Sseth, a deity of yuan-ti and an aspect of Set.\r\n"
            "History\r\n"
            "\r\n"
            "During the Blue Age, when Toril was still a new world, the gods and \r\n"
            "the primordials fought for dominance. This Age came to a sudden stop \r\n"
            "when the primordial known as the Dendar the Night Serpent took the sun \r\n"
            "out of the sky. Eventually, the Elder Gods won the fight when the \r\n"
            "primordials were betrayed by one of their own: Ubtao the Deceiver.\r\n"
            "\r\n"
            "Ubtao made an agreement with the rest of the Faer�nian Pantheon to o\r\n"
            "guard the Peaks of Flame for the day when Dendar the Night Serpent \r\n"
            "enters Toril and ends the world. As reward for this service, the other \r\n"
            "deities granted him sole control over Chult and agreed to leave his \r\n"
            "lands alone and never to spread their own religions there. Over time \r\n"
            "Ubtao's essence began to split into many nature spirits which now are \r\n"
            "spread across the jungle, one of these was the shadow entity Eshowdow. \r\n"
            "Shar recently absorbed Eshowdow, and it is believed this activity might \r\n"
            "be the beginning of the end for the agreement between Ubtao and the \r\n"
            "rest of the deities. \r\n"
            "\r\n");

  add_deity(DEITY_TYMORA, "Tymora", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_GOOD, DOMAIN_LUCK, DOMAIN_PROTECTION,
            DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHURIKEN, DEITY_PANTHEON_FAERUNIAN, "Good Fortune, Skill, Victory, Adventurers",

            "\r\n"
            "Tymora (pronounced tie-MORE-ah ), or more commonly Lady Luck, is the goddess of \r\n"
            "good fortune. She shines upon those who take risks and blesses those who deal \r\n"
            "harshly with the followers of Beshaba. Should someone flee from her sisters' \r\n"
            "mischievous followers or defile the dead, their fate would be decided with a \r\n"
            "roll of Tymora's dice.\r\n"
            "\r\n"
            "Worshipers\r\n"
            "\r\n"
            "Commonly consisting of adventurers and others who rely on a mixture of luck \r\n"
            "and skill to achieve their goals, the Tymoran clergy encourages folk to pursue \r\n"
            "their dreams. They are also dutybound to aid the daring by providing healing \r\n"
            "and even some minor magic items.\r\n"
            "\r\n"
            "Shrines and temples of Tymora are widespread as the need of adventurers to be \r\n"
            "healed making  the temples wealthy. These places of worship often differ \r\n"
            "significantly from each other in powers, manners and titles though, with little \r\n"
            "overall authority or hierarchy. The temples provide potions, scrolls or other \r\n"
            "little things like glowstones, often as rewards to those who serve Tymora and \r\n"
            "her tenets well\r\n"
            "\r\n"
            "Orders\r\n"
            "\r\n"
            "Fellows of Free Fate \r\n"
            "    This is a special fellowship of clergy within the church of Tymora who \r\n"
            "	have dedicated themselves to countering the efforts of Beshaba, and \r\n"
            "	especially of the Black Fingers, her assassins. Any clergy member may \r\n"
            "	join who shows experience, dedication to the cause and is vouched for \r\n"
            "	by a senior Fellow. \r\n"
            "\r\n"
            "Relationships\r\n"
            "\r\n"
            "Tymora is also a known ally of the demipower Finder Wyvernspur. She also has \r\n"
            "a relationship with Brandobaris of the halfling pantheon.\r\n"
            "\r\n"
            "History\r\n"
            "\r\n"
            "Tymora is a sister to Beshaba, the goddess of misfortune, having been created \r\n"
            "when Tyche, the former deity of luck, was infected by Moander's evil essence \r\n"
            "and split apart by Selune.\r\n\r\n");

  add_deity(DEITY_MASK, "Mask", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DARKNESS, DOMAIN_EVIL, DOMAIN_LUCK, DOMAIN_TRICKERY,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN, "Thieves, Thievery, Shadows",
            "\r\n"
            "Mask (pronounced MASK ), the Lord of Shadows, is a loner god, most often\r\n"
            "associated with thieves or those of otherwise ill-repute. He is a neutral evil \r\n"
            "Intermediate deity, from the Shadow Keep in the Plane of Shadow, whose portfolio \r\n"
            "includes shadows, thievery and thieves; although it previously also included \r\n"
            "intrigue. Mask's symbol is a black velvet mask tinged with red.\r\n"
            "\r\n"
            "Known for his constant scheming, cool head, and oft reserved biting comment he \r\n"
            "has recently lost a significant portion of his power, the intrigue portfolio, \r\n"
            "to Cyric. This has of course lead to two things, an enduring hatred of Cyric \r\n"
            "and the Lord of Shadows leading himself to be more direct than his prior \r\n"
            "elaborate plots.\r\n"
            "\r\n"
            "Worshipers\r\n"
            "\r\n"
            "The church of Mask states that wealth rightfully belongs to those who can \r\n"
            "acquire it. Honesty is for fools but apparent honesty is a very valuable \r\n"
            "thing while subtlety is everything. Priests of Mask are known as the Circle \r\n"
            "of the Gray Ribbon.\r\n"
            "\r\n"
            "It is rumored that the Cult of Mask maintains a large network of informants \r\n"
            "throughout the cities of the realm. It is also rumored that this network \r\n"
            "provides employment for all sorts of thieves, beggars and thugs.\r\n"
            "\r\n"
            "Chosen\r\n"
            "\r\n"
            "    * Avner of Hartsvale (briefly a Chosen of Kelemvor, during the Time \r\n"
            "	of Troubles)\r\n"
            "    * Erevis Cale\r\n"
            "    * Drasek Riven\r\n"
            "    * Kesson Rel (Former Chosen) \r\n"
            "\r\n"
            "Relationships\r\n"
            "\r\n"
            "Simply put, Mask is a loner. However, in the past he has had frequent, \r\n"
            "presently infrequent, alliances with Bane. If nothing else, their sizable \r\n"
            "hatred of Cyric gives them a common ground in addition to their history \r\n"
            "of working together. Mask is also at direct odds with Waukeen, the \r\n"
            "goddess of merchants and honest trade. All guardians of light, knowledge, \r\n"
            "and duty are opposed to him. This includes Sel�ne, whose light tends to o\r\n"
            "reveal his own faithful whilst they work.\r\n"
            "\r\n"
            "History\r\n"
            "\r\n"
            "Godsbane\r\n"
            "\r\n"
            "During the Time of Troubles Mask took shape of a powerful blade called \r\n"
            "Godsbane. He would eventually come to be wielded by the then-mortal \r\n"
            "Cyric; he acquired the sword by murdering a halfling named Sneakabout, \r\n"
            "who in turn killed the former wielder of the sword. In the years following \r\n"
            "the Time of Troubles, Mask released the powerful hound Kezef to try and \r\n"
            "kill Cyric, but the hound turned instead on the Lord of Shadows, not \r\n"
            "stopping the chase until it had bitten off one of the god's limbs. Mask, \r\n"
            "seemingly too weak to heal, acquired the powerful weapon Houndsbane to \r\n"
            "defend himself, the weapon, a gift from the deity of magic, Mystra.\r\n\r\n");


  add_deity(DEITY_LLIIRA, "Lliira", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_FAMILY, DOMAIN_GOOD,
            DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHURIKEN, DEITY_PANTHEON_FAERUNIAN,
            "Joy, Happiness, Dance, Festivals, Freedom, Liberty",

            "\r\n"
            "Lliira (pronounced LEER-ah ), also known as Our Lady of Joy, is a chaotic good \r\n"
            "lesser deity who is responsible for the domains of chaos, charm, family, good \r\n"
            "and travel.\r\n"
            "\r\n"
            "Worshipers\r\n"
            "\r\n"
            "Priests and priestesses of Lliira, called \"joy bringers\", are the apple of her \r\n"
            "eye. They are entrusted with the leadership and tasked as plan-makers of festivals.\r\n"
            "\r\n"
            "Followers of Lliira are called Lliirans and wear brightly colored outfits, and \r\n"
            "adorn themselves with rubies and sapphires. They are known for always having a \r\n"
            "smile on their lips; it is unheard of to see one with a frown.\r\n"
            "Orders\r\n"
            "\r\n"
            "Scarlet Mummers\r\n"
            "\r\n"
            "    The Mummers were formed to avenge the murder of Lliira's High Revelmistress \r\n"
            "	in Selgaunt. Their duties include protecting Lliiran temples and battling \r\n"
            "	with Loviatans. Their battle prowess causes their fellows in the church to \r\n"
            "	both fear and respect them. \r\n\r\n");

  add_deity(DEITY_HELM, "Helm", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_LAW, DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_FAERUNIAN,
            "Guardians, Protectors, Protection",

            "\r\n"
            "Helm (pronounced HELM ), also known as the Vigilant One and The Watcher, is the \r\n"
            "god of guardians, protection and protectors. He is worshiped by guards and \r\n"
            "paladins both, long being seen as a cold and focused deity who impartially took \r\n"
            "the role of defender and sometimes also enforcer. His activities in the Time of \r\n"
            "Troubles caused the folk of Faerun to look differently on the Watcher.rs\r\n"
            "\r\n"
            "Festivals\r\n"
            "\r\n"
            "Helmites celebrate the festival known as the Ceremony of Honor to Helm on \r\n"
            "Shieldmeet; the faith being, or was, especially popular in Cormyr, the Dragon \r\n"
            "Coast, Tethyr, the Vilhon Reach, and the Western Heartlands.\r\n"
            "Worshipers\r\n"
            "\r\n"
            "Helmites have long been respected and revered for their dedication and purpose,\r\n"
            "and their pledge to come to the defense of those who call for it. They wear \r\n"
            "polished full suits of armor often with plumed helmets. Their hierarchy is \r\n"
            "strict and militaristic, with specific groups such as the order of paladins \r\n"
            "called the Vigilant Eyes of the Deity, and originally also a single pontiff, \r\n"
            "head of the church.the Supreme Watcher. However, there has not been someone \r\n"
            "in this post since 992 DR.\r\n"
            "\r\n"
            "When preparing for battle, a Patriarch of Helm might use a ceremonial mace \r\n"
            "to cover troops in holy water, known as \"Tears of Helm\".\r\n"
            "\r\n"
            "Orders\r\n"
            "\r\n"
            "Watchers over the Fallen \r\n"
            "    The Watchers over the Fallen form a small fellowship of battlefield\r\n" 
            "	healers who worship Helm. Only clerics in high favor may join. \r\n"
            "\r\n"
            "Everwatch Knights \r\n"
            "    The Everwatch Knights are a group of dedicated bodyguards whom Helmite \r\n"
            "	temples hire out to others to generate revenue. \r\n"
            "\r\n"
            "Vigilant Eyes of the Deity \r\n"
            "    This is the order of the paladins who worship Helm. All paladins may \r\n"
            "	join this guild after squirehood. \r\n"
            "\r\n"
            "He Who Watches Over Travelers \r\n"
            "    This is a relatively obscure order of clerics who see to the blessings \r\n"
            "	of those about to partake on long journeys, such as traders and merchants. \r\n"
            "\r\n"
            "Relationships\r\n"
            "\r\n"
            "A very old deity, Helm is the eternal sentry and is always represented and \r\n"
            "seen wearing  a full suit of armor that represents the weight of his heavy \r\n"
            "responsibility. Yet Helm gets, and has always gotten, the job at hand done \r\n"
            "without complaint. The people of the Realms widely admired these qualities \r\n"
            "in what they saw as a humble and reassuring god.\r\n"
            "History\r\n"
            "\r\n"
            "Far back in time, the deity Lathander caused a divine purge known as the \r\n"
            "Dawn Cataclysm in which Helm's lover, a lesser deity of pragmatism called \r\n"
            "Murdane, was victim. Helm has begrudged the Morninglord this ever since. \r\n"
            "However Helm reserves his real opposition for deities whose plots threaten \r\n"
            "the people and stability of Faer�n, especially Bane and Cyric, as well as \r\n"
            "Mask and Shar. He is also especially at odds with the uncontrolled violence \r\n"
            "and careless destruction of the deities Garagos, Malar, and Talos.\r\n"
            "\r\n"
            "Time of Troubles\r\n"
            "\r\n"
            "    During the Time of Troubles, when the gods walked Toril, it was in \r\n"
            "	reliable Helm that Lord Ao trusted the task of keeping the other \r\n"
            "	deities from returning to their divine realms in the planes without \r\n"
            "	returning the stolen Tablets of Fate. For this task Ao left Helm with \r\n"
            "	all his divine abilities, guarding the Celestial Stairway to the planes. \r\n"
            "	When the goddess Mystra.who had spirited away a portion of her divine \r\n"
            "	power in the realms, which she then recovered once the gods were cast \r\n"
            "	from the heavens.attempted to pass him without the Tablets she was \r\n"
            "	turned back by the Watcher, and when she forcibly tried to pass the \r\n"
            "	Watcher, he destroyed her, on Midsummer, in the skies north of Arabel.\r\n"
            "	This action had enormous repercussions for Helm, causing the other \r\n"
            "	deities and mortals alike to hold Helm in great contempt. \r\n"
            "\r\n"
            "Maztica\r\n"
            "\r\n"
            "After the Time of Troubles ended and other gods were restored to their former \r\n"
            "existences, Helm himself was no longer bound to stand guard against them and \r\n"
            "much of his worship had faltered. Things amongst his clergy were made worse \r\n"
            "when the natives of recently-discovered Maztica, whom the priests of Helm were \r\n"
            "subjugating in their conquest of the region, highlighted their cause. It is \r\n"
            "only in recent times that Helm has regained some of his popularity and respect, \r\n"
            "as people acknowledge that in the Time of Troubles he was doing what he had to. \r\n"
            "The only god who could be considered a full ally of the Watcher is Torm, the \r\n"
            "god of paladins. Strongly-held ideological differences have caused a great \r\n"
            "rivalry verging on hatred between the clergy of the two gods, but the deities \r\n"
            "themselves remain close. \r\n"
            "\r\n");

  add_deity(DEITY_SHAR, "Shar", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_CAVERN, DOMAIN_DARKNESS, DOMAIN_EVIL,
          DOMAIN_KNOWLEDGE, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_KUKRI, DEITY_PANTHEON_FAERUNIAN,
            "Dark, Night, Loss, Secrets, Underdark",

            "Shar (pronounced SHAHR ), the Mistress of the Night, is the goddess of shadows\r\n"
            "as well as a neutral evil greater deity. Counterpart to her twin Selune, she \r\n"
            "presides over caverns, darkness, dungeons, forgetfulness, loss, night, secrets, \r\n"
            "and the Underdark. Among her array of twisted powers is the ability to see \r\n"
            "everything that lies or happens in the dark. Shar's symbol is a black disk with \r\n"
            "a deep purple border. Her divine realm is the Palace of Loss in the Plane of \r\n"
            "Shadow, and her domains are Cavern, Darkness, Evil, and Knowledge. She is also \r\n"
            "the creator of the Shadow Weave, meant as a counterpart and to foil the Weave, \r\n"
            "controlled by Mystra, the goddess of magic.\r\n"
            "\r\n"
            "Church of Shar\r\n"
            "\r\n"
            "The clergy of Shar is a secretive organization that pursues subversive tactics \r\n"
            "rather than direct confrontation with its rivals. In addition to her clerics, \r\n"
            "Shar maintains an elite order of sorcerer monks who can tap Shar's Shadow Weave.\r\n"
            "Among her worshipers are the Shadovar (the citizens of Thultanthar, a floating \r\n"
            "city which is home to the survivors of ancient Netheril who fled into the shadow \r\n"
            "plane before Karsus's Folly). Shar holds power over all who use the Shadow Weave.\r\n"
            "\r\n"
            "Orders\r\n"
            "\r\n"
            "Dark Justicars \r\n"
            "In order to gain admittance to the order of the Dark Justicars, a priest of Shar \r\n"
            "has to have killed a priest of Selune.\r\n"
            "\r\n"
            "Order of the Dark Moon \r\n"
            "Shar's secretive monastic order is referred to as the Order of the Dark Moon. They \r\n"
            "tap into the Shadow Weave through their powers of sorcery.\r\n"
            "\r\n"
            "Nightbringers \r\n"
            "The Nightbringers, or the Avatars of Shar, are an elite Sharran force. They are \r\n"
            "spirits that infest hosting bodies, possessing them and using the bodies as puppets. \r\n"
            "Once one is infected with a Nightbringer that person fuses to being as one with the \r\n"
            "Nightbringer gaining the strength and beauty of Shar. Only females are selected as \r\n"
            "hosts for the Nightbringers. Though Nightbringer numbers were large within the \r\n"
            "Avatar Wars, their numbers fell to the hundreds in the modern day.\r\n"
            "\r\n"
            "Relationships\r\n"
            "\r\n"
            "The creation of the Shadow Weave has made Shar the eternal enemy of the goddess of \r\n"
            "magic, Mystra. This has resulted in the brewing of a terrible war between these two \r\n"
            "powerful deities. By her very nature, however, she is opposed to powers of light, \r\n"
            "the unsecretive Shaundakul, and her own sister. Her only frequent ally is Talona, who \r\n"
            "may eventually serve Shar to stave off the predations of Loviatar.\r\n"
            "\r\n"
            "Those who believe in the Dark Moon heresy believe that Shar and Selune are two faces \r\n"
            "of the same goddess.\r\n"
            "\r\n"
            "History\r\n"
            "\r\n"
            "Shar is the dark twin of Selune. She has battled her sister since shortly after \r\n"
            "their creation. Their primordial feud has resulted in the creation of many other \r\n"
            "deities. Rather than overtly confronting other deities, Shar seeks to gain power \r\n"
            "by subverting mortal worshipers to her faith.\r\n"
            "\r\n"
            "Dogma\r\n"
            "\r\n"
            "Reveal secrets only to fellow members of the faithful. Never follow hope or turn \r\n"
            "to promises of success. Quench the light of the moon (agents and items of Selune) \r\n"
            "whenever you find it, and hide from it when you cannot prevail. The dark is a time \r\n"
            "to act, not wait. It is forbidden to strive to better your lot in life or to plan \r\n"
            "ahead save when directly overseen by the faithful of the Dark Deity. Consorting \r\n"
            "with the faithful of good deities is a sin except in business dealings or to corrupt \r\n"
            "them from their beliefs. Obey and never speak out against ranking clergy unless it \r\n"
            "would result in your own death.\r\n"
            "\r\n"
            "\"Love is a lie. Only hate endures.\"\r\n");


  add_deity(DEITY_MYSTRA, "Mystra", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_ILLUSION, DOMAIN_RUNE, 
          DOMAIN_KNOWLEDGE, DOMAIN_MAGIC, DOMAIN_SPELL, WEAPON_TYPE_SHURIKEN, DEITY_PANTHEON_FAERUNIAN,
            "Magic, Spells, The Weave",

            "Mystra (pronounced MISS-trah), the Mother of all Magic, formerly known as Midnight,\r\n"
            "was the most recent greater goddess who guided the magic that enveloped Toril and \r\n"
            "its surrounding space. Mystra tended to the Weave constantly, making possible all \r\n"
            "the miracles and mysteries wrought by magic and users of magic. She was believed \r\n"
            "to be the embodiment of the Weave and of magic itself, though this is now known to \r\n"
            "be false in the wake of the Spellplague. Mystra's symbol was a ring of seven stars \r\n"
            "surrounding a rising red mist, spiraling to the heavens. Mystra ruled over the \r\n"
            "divine dominion of Dweomerheart.\r\n"
            "\r\n"
            "Worshipers\r\n"
            "\r\n"
            "The church of Mystra preserved magical lore so that magic would continue and \r\n"
            "flourish in the future even if the dominant races of Faerun were to fall. Its \r\n"
            "members also searched out those skilled in magic or who had the potential to use it, \r\n"
            "keeping a close eye on those who were likely to become skilled. Her clerics were \r\n"
            "encouraged to explore magical theory and create new spells and magic items. Sites \r\n"
            "dedicated to the goddess were enhanced by the Weave to allow any spell cast by her \r\n"
            "clerics while in them to be affected by metamagic. Mystra honored the commitments \r\n"
            "that members of her predecessor's clergy who joined the church before the Time of \r\n"
            "Troubles, preventing them from being forced to leave the clergy due to alignment \r\n"
            "differences.\r\n"
            "\r\n"
            "Mystra's Chosen\r\n"
            "Mystra also has powerful mortal servants among her ranks of followers, including \r\n"
            "Elminster, Khelben Arunsun and the Seven Sisters.\r\n"
            "\r\n"
            "Religious Orders\r\n"
            "\r\n"
            "Order of the Starry Quill\r\n"
            "The Starry Quill was an order of Mystran bards who often worked as information \r\n"
            "gatherers and rumor-mongers for the church or spent part of their time in designated\r\n" 
            "libraries unearthing magical knowledge and then preserving it for posterity.\r\n"
            "\r\n"
            "Order of the Shooting Star\r\n"
            "The Church of Mystra sponsored an order of rangers, known as the Order of the \r\n"
            "Shooting Star. These rangers received spells from Mystra and served as long-range \r\n"
            "scouts and spies for the church, also dealing with magical threats that imposed upon \r\n"
            "the natural order of things, such as unloosed tanar'ri and baatezu as well as \r\n"
            "creatures born of irresponsible wizardly experimentation.\r\n"
            "\r\n"
            "Knights of Mystic Fire\r\n"
            "The Church of Mystra also sponsored a knightly order of paladins called the \r\n"
            "Knights of the Mystic Fire, who were granted their spells by Mystra. They often \r\n"
            "accompanied members of the clergy on quests to locate lost hoards of ancient magic \r\n"
            "and also formed the cadre from which the leadership for the small groups of armed \r\n"
            "forces who guarded Mystra's larger temples and workshops were drawn.\r\n"
            "\r\n"
            "Relationships\r\n"
            "\r\n"
            "Mystra's greatest enemies were Shar, who created the Shadow Weave to oppose \r\n"
            "Mystra's Weave, and Cyric, who was a mortal along with Mystra and Kelemvor. \r\n"
            "Mystra's customary adviser was Azuth and she was also served indirectly by Savras\r\n" 
            "and Velsharoon. Other allies of hers included Selune and Kelemvor, whom she knew \r\n"
            "as a man when she was a mortal.\r\n"
            "\r\n"
            "History\r\n"
            "\r\n"
            "Mystra as she existed between the Time of Troubles and the Spellplague was a \r\n"
            "different entity than the goddess who bore the name previously and was, in fact, the \r\n"
            "third such incarnation. All shared the same role and responsibilities, but they were \r\n"
            "different in alignment and temperament, as well as in origin.\r\n"
            "\r\n"
            "Early Life\r\n"
            "\r\n"
            "Midnight, born Ariel Manx, was the second child of Theus Manx a merchant and his wife \r\n"
            "Paiyse. Midnight had an elder sibling named Rysanna who assumed the role of the family's \r\n"
            "demure \"princess\" whenever wealthy suitors called. As a teenager Midnight became familiar \r\n"
            "with the night's populace of bards, thieves, sorcerers, and fighters and was eventually \r\n"
            "nicknamed \"Midnight\" by these friends, one she immediately preferred to Ariel.\r\n"
            "\r\n"
            "Midnight's first taste of magic began with her tryst with the conjurer Tad, who set her \r\n"
            "on her path. She began to exhibit less interest in her hedonistic pursuits and more in \r\n"
            "the quest for magical knowledge and training, gradually becoming more obsessed with her \r\n"
            "magical quest. Eventually, Midnight moved out of the family home to seek her own path.\r\n"
            "\r\n"
            "It was during this time that she fell into the worship of Mystra, whose attention \r\n"
            "Midnight attracted during her time of service in one of Mystra's temple. From her 21st \r\n"
            "year on Midnight began to feel a presence from time to time. She would feel her skin \r\n"
            "tingle coolly and began to feel that she was somehow being followed or observed. After \r\n"
            "such attentions, she always found that spells, which she had labored over for weeks, \r\n"
            "would suddenly work without any problem. Midnight soon suspected that she had been \r\n"
            "granted special attention of Mystra herself and believed that she was being groomed for \r\n"
            "the position of magister.\r\n"
            "\r\n"
            "Sunlar, high priest of the Deepingdale temple of Mystra, took Midnight under his \r\n"
            "supervision. It was during this time that Midnight's knowledge of self-defense and magic \r\n"
            "improved leaps and bounds and Midnight spent a year in the temple at Deepingdale before \r\n"
            "she left. For the next three years Midnight devoted herself to Mystra's worship and \r\n"
            "pursued every scrap of magic she could.\r\n"
            "\r\n"
            "Ascension\r\n"
            "\r\n"
            "During the Time of Troubles, when the gods were cast down by Ao, Midnight joined \r\n"
            "with Kelemvor Lyonsbane, Cyric, and Adon in the search for the stolen Tablets of \r\n"
            "Fate. During this time, the previous Mystra was killed by the deity Helm for \r\n"
            "defying Ao's command and trying to climb the Celestial Stairway back to the \r\n"
            "heavens. Her death caused great damage to the Weave, but eventually Ao selected \r\n"
            "Midnight to replace the destroyed Mystra, restoring the magic of Toril. \r\n"
            "Immediately prior to her ascension, Midnight killed Myrkul, the god of death,\r\n" 
            "in a duel in the skies over the city of Waterdeep.\r\n"
            "\r\n"
            "Mystra has a secret revealed stating she is more powerful than any god, save for \r\n"
            "Lord Ao, but therein lies the secret. Roughly half of her power lies in her Chosen \r\n"
            "and in the lesser power Azuth, thus planned by Ao so Mystra does not rule all \r\n"
            "Realmspace.\r\n");

  add_deity(DEITY_AKADI, "Akadi", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_AIR, DOMAIN_ILLUSION, DOMAIN_TRAVEL,
          DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_FLAIL, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Air, Movement, Speed, Flying Creatures",
          "Akadi (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

  add_deity(DEITY_AURIL, "Auril", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_AIR, DOMAIN_EVIL, DOMAIN_STORM,
          DOMAIN_WATER, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FAERUNIAN,
          "Cold, Winter",
          "Auril (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

  add_deity(DEITY_AZUTH, "Azuth", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_ILLUSION, DOMAIN_MAGIC, DOMAIN_KNOWLEDGE,
          DOMAIN_LAW, DOMAIN_SPELL, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_FLAIL, DEITY_PANTHEON_FAERUNIAN,
          "Wizards, Mages, Spellcasters in General",
          "Azuth (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_BANE, "Bane", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_EVIL, DOMAIN_HATRED,
          DOMAIN_LAW, DOMAIN_TYRANNY, DOMAIN_UNDEFINED, WEAPON_TYPE_MORNINGSTAR, DEITY_PANTHEON_FAERUNIAN,
          "Hatred, Tyranny, Fear",
          "Bane (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_BESHABA, "Beshaba", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CHAOS, DOMAIN_EVIL, DOMAIN_FATE,
          DOMAIN_LUCK, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_FAERUNIAN,
          "Random Mischief, Misfortune, Bad Luck, Accidents",
          "Beshaba (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_CHAUNTEA, "Chauntea", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_EARTH, DOMAIN_GOOD,
          DOMAIN_PLANT, DOMAIN_PROTECTION, DOMAIN_RENEWAL, WEAPON_TYPE_SCYTHE, DEITY_PANTHEON_FAERUNIAN,
          "Agriculture, Gardeners, Farmers, Summer",
          "Chauntea (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_CYRIC, "Cyric", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CHAOS, DOMAIN_EVIL, DOMAIN_DESTRUCTION,
          DOMAIN_ILLUSION, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Murder, Lies, Intrigue, Strife, Deception, Illusion",
          "Cyric (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_DENEIR, "Deneir", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_KNOWLEDGE, DOMAIN_RUNE,
          DOMAIN_PROTECTION, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FAERUNIAN,
          "Glyphs, Images, Literature, Scribes, Cartography",
          "Deneir (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_ELDATH, "Eldath", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_FAMILY, DOMAIN_GOOD, DOMAIN_PLANT,
          DOMAIN_PROTECTION, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_NET, DEITY_PANTHEON_FAERUNIAN,
          "Quiet Places, Springs, Pools, Peace, Waterfalls",
          "Eldath (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_FINDER_WYVERNSPUR, "Finder Wyvernspur", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS,
          DOMAIN_CHARM, DOMAIN_RENEWAL,
          DOMAIN_SCALYKIND, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Cycle of Life, Transformation of Art, Saurials",
          "Finder Wyvernspur (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_GARAGOS, "Garagos", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS, DOMAIN_DESTRUCTION,
          DOMAIN_STRENGTH,
          DOMAIN_WAR, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "War, Skill-at-Arms, Destruction, Plunder",
          "Garagos (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_GARGAUTH, "Gargauth", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_CHARM, DOMAIN_EVIL, DOMAIN_LAW,
          DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FAERUNIAN,
          "Betrayal, Cruelty, Politcal Corruption, Powerbrokers",
          "Gargauth (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_GOND, "Gond", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CRAFT, DOMAIN_EARTH, DOMAIN_FIRE,
          DOMAIN_KNOWLEDGE, DOMAIN_METAL, DOMAIN_PLANNING, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FAERUNIAN,
          "Artifice, Craft, COnstruction, Smithwork",
          "Gond (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_GRUMBAR, "Grumbar", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CAVERN, DOMAIN_EARTH, DOMAIN_METAL,
          DOMAIN_TIME, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Earth, Solidity, Changelessness, Oaths",
          "Grumbar (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_GWAERON_WINDSTROM, "Dwaeron Windstrom", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_GOOD,
          DOMAIN_KNOWLEDGE,
          DOMAIN_PLANT, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Tracking, Rangers of the North",
          "Gwaeron Windstrom (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_HOAR, "Hoar", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_FATE, DOMAIN_LAW, DOMAIN_RETRIBUTION,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_JAVELIN, DEITY_PANTHEON_FAERUNIAN,
          "Revenge, Retribution, Poetic Justice",
          "Hoar (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_ISTISHIA, "Istishia", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DESTRUCTION, DOMAIN_OCEAN, DOMAIN_STORM,
          DOMAIN_TRAVEL, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Water, Purification, Wetness",
          "Istishia (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_JERGAL, "Jergal", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_DEATH, DOMAIN_FATE, DOMAIN_LAW,
          DOMAIN_RUNE, DOMAIN_SUFFERING, DOMAIN_UNDEFINED, WEAPON_TYPE_SCYTHE, DEITY_PANTHEON_FAERUNIAN,
          "Fatalism, Proper Burial, Guardianship of Tombs",
          "Jergal (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_KELEMVOR, "Kelemvor", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_DEATH, DOMAIN_FATE, DOMAIN_LAW,
          DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Death, the Dead",
          "Kelemvor (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_KOSSUTH, "Kossuth", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DESTRUCTION, DOMAIN_FIRE, DOMAIN_RENEWAL,
          DOMAIN_SUFFERING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_FAERUNIAN,
          "Elemental Fire, Purification through Fire",
          "Kossuth (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_LATHANDER, "Lathander", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_NOBILITY, DOMAIN_PROTECTION,
          DOMAIN_RENEWAL, DOMAIN_STRENGTH, DOMAIN_SUN, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FAERUNIAN,
          "Spring, Dawn, Youth, Birth, Vitality, Athletics",
          "Lathander (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_LOVIATAR, "Loviatar", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_LAW, DOMAIN_RETRIBUTION,
          DOMAIN_SUFFERING, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_FAERUNIAN,
          "Pain, Hurt, Agony, Torment, Suffering Torture",
          "Loviatar (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_LURUE, "Lurue", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_CHAOS, DOMAIN_GOOD,
          DOMAIN_HEALING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORTSPEAR, DEITY_PANTHEON_FAERUNIAN,
          "Talking Beasts, Intelligent Non-Humanoid Creatures",
          "Lurue (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_MALAR, "Malar", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_ANIMAL, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_MOON, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN,
          "Hunters, Stalking, Bloodlust, Evil Lycanthropes",
          "Malar (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_MIELIKKI, "Meilikki", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_GOOD, DOMAIN_PLANT,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SCIMITAR, DEITY_PANTHEON_FAERUNIAN,
          "Forests, Forest Creatures, Rangers, Dryads, Autumn",
          "Meilikki (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_MILIL, "Milil", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_CHARM, DOMAIN_GOOD, DOMAIN_KNOWLEDGE,
          DOMAIN_MOBILITY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_RAPIER, DEITY_PANTHEON_FAERUNIAN,
          "Poetry, Song, Eloquence",
          "Milil (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_NOBANION, "Nobanion", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_GOOD, DOMAIN_LAW,
          DOMAIN_NOBILITY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_PICK, DEITY_PANTHEON_FAERUNIAN,
          "Royalty, Lions and Feline Beasts, Good Beasts",
          "Nobanion (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_OGHMA, "Oghma", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CHARM, DOMAIN_LUCK, DOMAIN_KNOWLEDGE,
          DOMAIN_TRAVEL, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Knowledge, Invention, Inspiration, Bards",
          "Oghma (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_RED_KNIGHT, "Red Knight", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_LAW, DOMAIN_NOBILITY, DOMAIN_PLANNING,
          DOMAIN_WAR, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Strategy, Planning, Tactics",
          "Red Knight (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_SAVRAS, "Savras", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_FATE, DOMAIN_KNOWLEDGE, DOMAIN_LAW,
          DOMAIN_MAGIC, DOMAIN_SPELL, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FAERUNIAN,
          "Divination, Fate, Truth",
          "Savras (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_SELUNE, "Selune", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_GOOD, DOMAIN_MOON,
          DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FAERUNIAN,
          "Moon, Stars, Navigation, Prophecy, Questers, Good && Neutral Lycanthropes",
          "Selune (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_SHARESS, "Sharess", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_TRAVEL, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN,
          "Hedonism, Sensual Fulfilment, Festhalls, Cats",
          "Sharess (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_SHAUNDAKUL, "Shaundakul", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_AIR, DOMAIN_CHAOS, DOMAIN_PORTAL,
          DOMAIN_PROTECTION, DOMAIN_TRADE, DOMAIN_TRAVEL, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Travel, Exploration, Caravans, Portals",
          "Shaundakul (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_SHIALLIA, "Shiallia", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_PLANT, DOMAIN_GOOD,
          DOMAIN_RENEWAL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FAERUNIAN,
          "Woodland GLades && Fertility, The High Forest, Neverwinter Wood",
          "Shiallia (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_SIAMORPHE, "Siamorphe", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_KNOWLEDGE, DOMAIN_LAW, DOMAIN_NOBILITY,
          DOMAIN_PLANNING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LIGHT_MACE, DEITY_PANTHEON_FAERUNIAN,
          "Nobles, Rightful Rule of Nobility, Human Royalty",
          "Siamorphe (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_SILVANUS, "Silvanus", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_ANIMAL, DOMAIN_PLANT, DOMAIN_RENEWAL,
          DOMAIN_PROTECTION, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_CLUB, DEITY_PANTHEON_FAERUNIAN,
          "Wild Nature, Druids",
          "Silvanus (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_SUNE, "Sune", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WHIP, DEITY_PANTHEON_FAERUNIAN,
          "Beauty, Love, Passion",
          "Sune (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_TALONA, "Talona", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_SUFFERING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FAERUNIAN,
          "Disease, Poison",
          "Talona (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_TALOS, "Talos", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_FIRE, DOMAIN_CHAOS,
          DOMAIN_EVIL, DOMAIN_STORM, DOMAIN_UNDEFINED, WEAPON_TYPE_SPEAR, DEITY_PANTHEON_FAERUNIAN,
          "Storms, Destruction, Rebellion, Conflagrations, Earthquakes, Vortices",
          "Talos (Description not done yet.  If you wish to write it, let Gicker know.)\r\n"
          );

    add_deity(DEITY_TIAMAT, "Tiamat", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_LAW, DOMAIN_SCALYKIND,
          DOMAIN_TYRANNY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_PICK, DEITY_PANTHEON_FAERUNIAN,
          "Evil Dragons && Reptiles, Greed, Chessenta",
          "Tiamat (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_TORM, "Torm", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_HEALING, DOMAIN_LAW,
          DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Duty, Loyalty, Obedience, Paladins",
          "Torm (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_TYR, "Tyr", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_KNOWLEDGE, DOMAIN_LAW,
          DOMAIN_RETRIBUTION, DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FAERUNIAN,
          "Justice",
          "Tyr (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_UMBERLEE, "Umberlee", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_OCEAN, DOMAIN_STORM, DOMAIN_WATER, WEAPON_TYPE_TRIDENT, DEITY_PANTHEON_FAERUNIAN,
          "Oceans, Currents, Waves, Sea Winds",
          "Umberlee (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_VALKUR, "Valkur", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_AIR, DOMAIN_CHAOS, DOMAIN_GOOD,
          DOMAIN_OCEAN, DOMAIN_PROTECTION, DOMAIN_UNDEFINED, WEAPON_TYPE_SCIMITAR, DEITY_PANTHEON_FAERUNIAN,
          "Sailors, Ships, Favorable Winds, Naval Combat",
          "Valkur (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_VELSHAROON, "Velsharoon", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DEATH, DOMAIN_EVIL, DOMAIN_MAGIC,
          DOMAIN_UNDEATH, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FAERUNIAN,
          "Necromancy, Necormancers, Evil Liches, Undeath, Undead",
          "Velsharoon (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_WAUKEEN, "Waukeen", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_KNOWLEDGE, DOMAIN_PROTECTION, DOMAIN_TRADE,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_NUNCHAKU, DEITY_PANTHEON_FAERUNIAN,
          "Trade, Money, Wealth",
          "Waukeen (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    // End FR Pantheon Deities

  // Dwarven Pantheon

    add_deity(DEITY_BERRONAR_TRUESILVER, "Berronar Truesilver", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_FAMILY, DOMAIN_GOOD,
          DOMAIN_HEALING, DOMAIN_LAW, DOMAIN_PROTECTION, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FR_DWARVEN,
          "Safety, Honesty, Home, Healing, The Dwarven Family, Records, Marriage, Faithfulness, Loyalty, Oaths",
          "Berronar Truesilver (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_ABBATHOR, "Abbathor", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DWARF, DOMAIN_LUCK, DOMAIN_TRADE,
          DOMAIN_TRICKERY, DOMAIN_EVIL, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_DWARVEN,
          "Greed",
          "Abbathor (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_CLANGEDDIN_SILVERBEARD, "Clangeddin Silverbeard", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_STRENGTH, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_DWARVEN,
          "Battle, War, Bravery, Honor in Battle",
          "Clangeddin Silverbeard (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );
    
    add_deity(DEITY_DEEP_DUERRA, "Deep Duerra", ETHOS_LAWFUL, ALIGNMENT_EVIL,
          DOMAIN_DWARF, DOMAIN_EVIL, DOMAIN_MENTALISM,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_DWARVEN,
          "Psionics, COnquest, Expansion",
          "Deep Duerra (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_DUGMAREN_BRIGHTMANTLE, "Dugmaren Brightmantle", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_CRAFT, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_KNOWLEDGE, DOMAIN_RUNE, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_DWARVEN,
          "Scholarship, Invention, Discovery",
          "Dugmaren Brightmantle (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_DUMATHOIN, "Dumathoin", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_DWARF, DOMAIN_CAVERN, DOMAIN_CRAFT,
          DOMAIN_EARTH, DOMAIN_KNOWLEDGE, DOMAIN_PROTECTION, WEAPON_TYPE_GREAT_CLUB, DEITY_PANTHEON_FR_DWARVEN,
          "Buried Wealth, Ores, Gems, Mining, Exploration, Shield Dwarves, Guardian of the Dead",
          "Dumathoin (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_GORM_GULTHYN, "Gorm Gulthyn", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_PROTECTION, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_DWARVEN,
          "Guardian of all Dwarves, Defense, Watchfulness",
          "Gorm Gulthyn (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_HAELA_BRIGHTAXE, "Haela Brightaxe", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_LUCK, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_FR_DWARVEN,
          "Luck in Battle, Joy of Battle, Dwarven Fighters",
          "Haela Brightaxe (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_LADUGUER, "Laduguer", ETHOS_LAWFUL, ALIGNMENT_EVIL,
          DOMAIN_DWARF, DOMAIN_CRAFT, DOMAIN_EVIL,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_PROTECTION, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_DWARVEN,
          "Magic Weapon Creation, Artisans, Duergar, Magic",
          "Laduguer (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_MARTHAMMOR_DUIN, "Marthammor Duin", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_PROTECTION, DOMAIN_GOOD,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FR_DWARVEN,
          "Guides, Explorers, Expatriates, Travelers, Lightning",
          "Marthammor Duin (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_SHARINDLAR, "Sharindlar", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_HEALING, DOMAIN_CHAOS, DOMAIN_MOON, WEAPON_TYPE_WHIP, DEITY_PANTHEON_FR_DWARVEN,
          "Healing, Mercy, Romantic Love, Fertility, Dancing, Courtship, the Moon",
          "Sharindlar (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_THARD_HARR, "Thard Harr", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_DWARF, DOMAIN_ANIMAL, DOMAIN_GOOD,
          DOMAIN_PLANT, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_DWARVEN,
          "Wild Dwarves, Jungle Survival, Hunting",
          "Thard Harr (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

    add_deity(DEITY_VERGADAIN, "Vergadain", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_DWARF, DOMAIN_LUCK, DOMAIN_TRADE,
          DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FR_DWARVEN,
          "Wealth, Luck, Chance, Non-Evil Thieves, Suspicion, Trickery, Negotiations, Sly Cleverness",
          "Vergadain (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_MORADIN, "Moradin", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_CRAFT, DOMAIN_DWARF, DOMAIN_EARTH, DOMAIN_GOOD,
            DOMAIN_LAW, DOMAIN_PROTECTION, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_DWARVEN,
            "Dwarves, Creation, Smithing, Metalcraft, Stonework",
            "\r\n"
            "Moradin, or the Dwarffather, is the lawful good god of the dwarves and the chief\r\n"
            "deity in their pantheon. A harsh but fair judge, he is strength and force of will\r\n"
            "embodied. Moradin inspires dwarven inventions and constantly seeks to improve that\r\n"
            "race, encouraging their good nature, intelligence, and harmonious existence with \r\n"
            "other good races while battling their pride and isolationist tendencies. Moradin's\r\n"
            "holy day is on the crescent moon and he is worshipped at forges and hearths.\r\n"
            "\r\n"
            "Moradin's clerics, known as Sonnlinor, are usually drawn from family lines, like \r\n"
            "most dwarven occupations; they wear earthy colors, with chain mail and silvered \r\n"
            "helms. Moradin charges his followers with the task of removing the kingdoms of \r\n"
            "orcs and wiping out the followers of Gruumsh. The church of Moradin has an active\r\n"
            "role in guiding the morals of dwarven communities; they emphasize his hand in \r\n"
            "everyday dwarven activities such as mining, smithing, and engineering, and invoke\r\n"
            "his blessing when these tasks are begun. They lead the push to found new dwarven\r\n"
            "kingdoms and increase their status among surface communities. Many of these \r\n"
            "communities celebrate Hammer 1st, believing that day in 1306 DR to be a \r\n"
            "blessing by the Dwarffather.\r\n"
            "Orders\r\n"
            "\r\n"
            "    * Hammers of Moradin: The Hammers of Moradin are an elite military order \r\n"
            "	dominated by crusaders and fighting clerics with chapters in nearly every\r\n"
            "	dwarven stronghold and members drawn from every dwarven clan. The Hammers\r\n"
            "	serve both as commanders of dwarven armies and as an elite strike force \r\n"
            "	skilled in dealing with anything from large groups of orcs to great wyrms\r\n"
            "	to malevolent fiends from the Fiendish Planes. The order is dedicated to \r\n"
            "	the defense of existing dwarven holdings and the carving out of new \r\n"
            "	dwarven territories. Individual chapters have a great deal of local a\r\n"
            "	utonomy but, in times of great crisis, a Grand Council will assemble to \r\n"
            "	plot strategy and divine Moradin's will. \r\n"
            "\r\n"
            "Relationships\r\n"
            "\r\n"
            "Moradin has a strategic but cool alliance with Gond, Kossuth, Helm, Torm, \r\n"
            "Tyr, and the heads of the elven, gnome, and halfling pantheons. He opposes \r\n"
            "the gods of the goblinoids, orcs, evil giants, and banished dwarves.\r\n"
            "\r\n"
            "History\r\n"
            "\r\n"
            "Moradin is held in dwarven myths to have been incarnated from rock, stone, \r\n"
            "and metal, and that his soul an ember of fire. It is said he forged the first \r\n"
            "dwarves from metals and gems and breathed souls into them when he blew on his \r\n"
            "creations to cool them. Moradin was responsible for banishing the evil gods \r\n"
            "of the derro and duergar from the surface. \r\n\r\n");


  // Elven Pantheon

  add_deity(DEITY_AERDRIE_FAENYA, "Aerdrie Faenya", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_ANIMAL, DOMAIN_GOOD,
          DOMAIN_AIR, DOMAIN_CHAOS, DOMAIN_STORM, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_ELVEN,
          "Air, Weather, Avians, Rain, Fertility, Avariels",
          "Aerdrie Faenya (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_ANGHARRADH, "Angharradh", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_KNOWLEDGE, DOMAIN_GOOD,
          DOMAIN_PLANT, DOMAIN_CHAOS, DOMAIN_RENEWAL, WEAPON_TYPE_SPEAR, DEITY_PANTHEON_FR_ELVEN,
          "Spring, Fertility, Planting, Birth, Defense, Wisdom",
          "Angharradh (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_CORELLON_LARETHIAN, "Corellon Larethian", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_MAGIC, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_CHAOS, DOMAIN_WAR, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FR_ELVEN,
          "Magic, Music, Arts, Crafts, War, The Elven Race, Sun Elves, Poetry, Bards, Warriors",
          "Corellon Larethian (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_DEEP_SASHELAS, "Deep Sashelas", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_KNOWLEDGE, DOMAIN_GOOD,
          DOMAIN_OCEAN, DOMAIN_CHAOS, DOMAIN_WATER, WEAPON_TYPE_TRIDENT, DEITY_PANTHEON_FR_ELVEN,
          "Oceans, Sea Elves, Creation, Knowledge",
          "Deep Sashelas (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_EREVAN_ILESERE, "Erevan Ilesere", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL,
          DOMAIN_ELF, DOMAIN_LUCK, DOMAIN_TRICKERY,
          DOMAIN_CHAOS, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_ELVEN,
          "Mischief, Change, Rogues",
          "Erevan Ilesere (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_FENMAREL_MESTARINE, "Fenmarel Mestarine", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL,
          DOMAIN_ELF, DOMAIN_ANIMAL, DOMAIN_PLANT,
          DOMAIN_TRAVEL, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_ELVEN,
          "Feral Elves, Outcasts, Scapegoats, Isolation",
          "Fenmarel Mestarine (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_HANALI_CELANIL, "Hanali Celanil", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_MAGIC, DOMAIN_CHAOS, DOMAIN_PROTECTION, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_ELVEN,
          "Love, Romance, Beauty, Enchantment, Magic Item Artistry, Fine Art, Artists",
          "Hanali Celanil (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_LABELAS_ENORETH, "Labelas Enoreth", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_KNOWLEDGE, DOMAIN_GOOD,
          DOMAIN_TIME, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_ELVEN,
          "Time, Longevity, The Moment of Choice, History",
          "Labelas Enoreth (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_RILLIFANE_RALLATHIL, "Rillifane Rallathil", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_PLANT, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_ELVEN,
          "Woodlands, Nature, Wild Elves, Druids",
          "Rillifane Rallathil (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SEHANINE_MOONBOW, "Sehanine Moonbow", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_KNOWLEDGE, DOMAIN_GOOD,
          DOMAIN_ILLUSION, DOMAIN_CHAOS, DOMAIN_MOON, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_ELVEN,
          "Mysticism, Dreams, Death, Journeys, Transcendence, the moon, stars && heavens, Moon Elves",
          "Sehanine Moonbow (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SHEVARASH, "Shevarash", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL,
          DOMAIN_ELF, DOMAIN_RETRIBUTION, DOMAIN_WAR,
          DOMAIN_CHAOS, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_COMPOSITE_LONGBOW, DEITY_PANTHEON_FR_ELVEN,
          "Hatred of the Drow, Vengeance, Crusades, Loss",
          "Shevarash (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SOLONOR_THELANDIRA, "Solonor Thelandira", ETHOS_CHAOTIC, ALIGNMENT_GOOD,
          DOMAIN_ELF, DOMAIN_PLANT, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_CHAOS, DOMAIN_UNDEFINED, WEAPON_TYPE_COMPOSITE_LONGBOW, DEITY_PANTHEON_FR_ELVEN,
          "Archery, Hunting, Wilderness Survival",
          "Solonor Thelandira (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  // Gnome Pantheon

  add_deity(DEITY_BAERVAN_WILDWANDERER, "Baervan Wildwanderer", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_PLANT, DOMAIN_GOOD,
          DOMAIN_ANIMAL, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORTSPEAR, DEITY_PANTHEON_FR_GNOME,
          "Forests, Travel, Nature",
          "Baervan Wildwanderer (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_BARAVAR_CLOAKSHADOW, "Baravar Cloakshadow", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_PROTECTION, DOMAIN_GOOD,
          DOMAIN_TRICKERY, DOMAIN_ILLUSION, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_GNOME,
          "Illusions, Deception, Traps, Wards",
          "Daravar Cloakshadow (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_CALLARDURAN_SMOOTHHANDS, "Callarduran Smoothhands", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_GNOME, DOMAIN_CAVERN, DOMAIN_CRAFT,
          DOMAIN_EARTH, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_GNOME,
          "Stone, The Underdark, Mining, Svirfneblin",
          "Callarduran Smoothhands (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_FLANDAL_STEELSKIN, "Flandal Steelskin", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_CRAFT, DOMAIN_GOOD,
          DOMAIN_METAL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_GNOME,
          "Mining, Physical Fitness, Smithing, Metalworking",
          "Flandal Steelskin (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_GAERDAL_IRONHAND, "Gaerdal Ironhand", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_PROTECTION, DOMAIN_GOOD,
          DOMAIN_WAR, DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_GNOME,
          "Vigilance, Combat, Martial Defense",
          "Gaerdal Ironhand (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_GARL_GLITTERGOLD, "Garl Glittergold", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_CRAFT, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_LAW, DOMAIN_TRICKERY, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_FR_GNOME,
          "Protection, Humor, Gem-Cutting, Trickery, Gnomes",
          "Garl Glittergold (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SEGOJAN_EARTHCALLER, "Segojan Earthcaller", ETHOS_NEUTRAL, ALIGNMENT_GOOD,
          DOMAIN_GNOME, DOMAIN_CAVERN, DOMAIN_GOOD,
          DOMAIN_EARTH, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FR_GNOME,
          "Earth, Nature, the Dead",
          "Segojan Earthcaller (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_URDLEN, "Urdlen", ETHOS_CHAOTIC, ALIGNMENT_EVIL,
          DOMAIN_GNOME, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_EARTH, DOMAIN_HATRED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_GNOME,
          "Greed, Bloodlust, Evil, Hatred, Spriggans, Uncontrolled Impulse",
          "Urdlen (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );


  // Halfing Pantheon

  add_deity(DEITY_ARVOREEN, "Arvoreen", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_HALFLING, DOMAIN_LAW, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_HALFLING,
          "Defense, War, Vigilence, Halfling Warriors, Duty",
          "Arvoreen (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_BRANDOBARIS, "Brandobaris", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_HALFLING, DOMAIN_LUCK, DOMAIN_TRAVEL,
          DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_HALFLING,
          "Stealth, Thievery, Adventuring, Halfling Rogues",
          "Brandobaris (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_CYRROLLALEE, "Cyrrollalee", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_HALFLING, DOMAIN_LAW, DOMAIN_GOOD,
          DOMAIN_FAMILY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_FR_HALFLING,
          "Friendship, Trust, the Hearth, Hospitality, Crafts",
          "Cyrrollalee (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SHEELA_PERYROYL, "Sheela Peryroyl", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL,
          DOMAIN_HALFLING, DOMAIN_AIR, DOMAIN_CHARM,
          DOMAIN_PLANT, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SICKLE, DEITY_PANTHEON_FR_HALFLING,
          "Nature, Agriculture, Weather, Song, Dance, Beauty, Romantic Love",
          "Sheela Peryroyl (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_UROGALAN, "Urogalan", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL,
          DOMAIN_HALFLING, DOMAIN_LAW, DOMAIN_DEATH,
          DOMAIN_PROTECTION, DOMAIN_EARTH, DOMAIN_UNDEFINED, WEAPON_TYPE_DIRE_FLAIL, DEITY_PANTHEON_FR_HALFLING,
          "Earth, Death, Protection of the Dead",
          "Urogalan (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_YONDALLA, "Yondalla", ETHOS_LAWFUL, ALIGNMENT_GOOD,
          DOMAIN_HALFLING, DOMAIN_LAW, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_FAMILY, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_HALFLING,
          "Protection, Bounty, Halflings, Children, Security, Leadership, Wisdom, Creation, Family, Tradition",
          "Yondalla (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  // Orc Pantheon

  add_deity(DEITY_BAHGTRU, "Bahgtru", ETHOS_CHAOTIC, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_STRENGTH, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_ORC,
          "Loyalty, Stupidity, Brute Strength",
          "Bahgtru (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_GRUUMSH, "Gruumsh", ETHOS_CHAOTIC, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_STRENGTH, DOMAIN_HATRED, DOMAIN_WAR, WEAPON_TYPE_SPEAR, DEITY_PANTHEON_FR_ORC,
          "Orcs, COnquest, Survival, Strength, Territory",
          "Gruumsh (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_ILNEVAL, "Ilneval", ETHOS_NEUTRAL, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_WAR, DOMAIN_EVIL,
          DOMAIN_DESTRUCTION, DOMAIN_PLANNING, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_FR_ORC,
          "War, Combat, Overwhelming Numbers, Strategy",
          "Ilneval (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_LUTHIC, "Luthic", ETHOS_NEUTRAL, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_CAVERN, DOMAIN_EVIL,
          DOMAIN_EARTH, DOMAIN_FAMILY, DOMAIN_HEALING, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_ORC,
          "Caves, Orc Females, Home, Wisdom, Fertility, Healing, Servitude",
          "Luthic (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SHARGAAS, "Shargaas", ETHOS_CHAOTIC, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_DARKNESS, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_ORC,
          "Night, Thieves, Stealth, Darkness, The Underdark",
          "Shargaas (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_YURTRUS, "Yurtrus", ETHOS_NEUTRAL, ALIGNMENT_EVIL,
          DOMAIN_ORC, DOMAIN_DEATH, DOMAIN_EVIL,
          DOMAIN_DESTRUCTION, DOMAIN_SUFFERING, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_FR_ORC,
          "Death, Disease",
          "Yurtrus (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  // Drow Pantheon

  add_deity(DEITY_LOLTH, "Lolth", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CHAOS, DOMAIN_DARKNESS, DOMAIN_DESTRUCTION,
          DOMAIN_DROW, DOMAIN_EVIL, DOMAIN_SPIDER, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_DROW,
          "Spiders, Evil, Darkness, Chaos, Assassins, Drow",
          "Lolth (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_GHAUNADAUR, "Ghaunadaur", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CAVERN, DOMAIN_CHAOS, DOMAIN_DROW,
          DOMAIN_HATRED, DOMAIN_EVIL, DOMAIN_SLIME, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_FR_DROW,
          "Oozes, Slimes, Jellies, Outcasts, Ropers, Rebels",
          "Ghaunadaur (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_KIARANSALEE, "Kiaransalee", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_UNDEATH, DOMAIN_CHAOS, DOMAIN_DROW,
          DOMAIN_RETRIBUTION, DOMAIN_EVIL, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER, DEITY_PANTHEON_FR_DROW,
          "Undead, Vengeance",
          "Kiaransalee (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_SELVETARM, "Selvetarm", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_WAR, DOMAIN_CHAOS, DOMAIN_DROW,
          DOMAIN_SPIDER, DOMAIN_EVIL, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_FR_DROW,
          "Drow Warriors",
          "Selvetarm (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_VHAERAUN, "Vhaeraun", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_TRAVEL, DOMAIN_CHAOS, DOMAIN_DROW,
          DOMAIN_TRICKERY, DOMAIN_EVIL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_FR_DROW,
          "Thievery, Drow Males, Evil Activity on the Surface",
          "Vhaeraun (Description not done yet.  If you wish to write it, email to ilmater@@gmail.com)\r\n"
          );

  add_deity(DEITY_EILISTRAEE, "Eilistraee", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_DROW, DOMAIN_ELF,
            DOMAIN_GOOD, DOMAIN_MOON, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_FR_DROW,
            "Song, Beauty, Dance, Swordwork, Hunting, Moonlight",

            "\r\n"
            "Eilistraee (pronounced eel-iss-TRAY-yee ), also referred to as \"The Dark Maiden\",\r\n"
            "is the chaotic good drow goddess of song, swordwork, hunting, the moon and \r\n"
            "beauty. She is greatly angered by the evil of most drow but glad that some have\r\n"
            "worked their way free of the Spider Queen's web. Eilistraee appears as an \r\n"
            "unclad, glossy-skinned drow woman of great height with ankle-length, sweeping\r\n"
            "hair of glowing silver.\r\n"
            "\r\n"
            "She is worshiped by song and dance, if at all possible, in the surface world\r\n"
            "under the moonlit night among the woods. She takes great pleasure in bards \r\n"
            "learning new songs, craftsmen at work, and the doing of kindhearted deeds.\r\n"
            "\r\n"
            "Worshippers\r\n"
            "\r\n"
            "The church of Eilistraee is little known and poorly understood by inhabitants\r\n"
            "of the surface world. Her worshipers are good-aligned drow hoping to escape \r\n"
            "the Underdark's evil, Lolth-worshiping matriarchal society, and regain a \r\n"
            "place in the surface world. Among her followers are drow, humans, gnomes, \r\n"
            "elves, shapeshifters and half-elves.\r\n"
            "\r\n"
            "The church of Eilistraee headquarters at The Promenade is located near\r\n"
            "Skullport was led by the High Priestess Qilu� Veladorn, youngest of the \r\n"
            "Seven Sisters and Chosen of Mystra. Second to her was The Promenade's \r\n"
            "Battlemistress Rylla. All priestesses were free to do as they liked unless \r\n"
            "given a mission by either Qilu� or Rylla. Although the Darksong knights, \r\n"
            "priestess/warriors, had more independence they too were subject being given \r\n"
            "missions. Generally its clerics wear their hair long and dress practically \r\n"
            "for whatever they are currently doing. For rituals, they wear as little as \r\n"
            "possible. Otherwise, they tend to wear soft leathers for hunting, aprons \r\n"
            "while cooking, and�rarely�armor when battle is expected. When relaxing, \r\n"
            "they favor silvery, diaphanous gowns. The Holy symbol is a small sword and \r\n"
            "clerics prefer holy symbols of silver, typically worn as pins or hung around \r\n"
            "the neck on slender silver or mithral chains. They pray for spells at night, \r\n"
            "after moonrise, singing them whenever possible. Their rituals revolve around \r\n"
            "a hunt followed by a feast, dancing, and a Circle of Song. This last is held \r\n"
            "preferably in a wooded glade on a moonlit night, in which the worshipers sit \r\n"
            "and dance by turns in a circle, each one leading a song.\r\n"
            "\r\n"
            "Relationships\r\n"
            "\r\n"
            "Eilistraee's allies are the Seldarine, Mystra, Sel�ne, and the good deities\r\n"
            "of the Underdark races; her enemies are the evil deities of the Underdark, \r\n"
            "especially the rest of the drow pantheon.\r\n"
			"\r\n"
            "History\r\n"
            "\r\n"
            "Eilistraee is the daughter of Corellon Larethian and Araushnee, who later\r\n"
            "became Lolth, and the sister of Vhaeraun. She was banished along with the \r\n"
            "other drow deities for her role, albeit inadvertently, in the war against \r\n"
            "the Seldarine. Eilistraee insisted upon this punishment from her reluctant \r\n"
            "father, because she foresaw that the dark elves would need a beacon of good \r\n"
            "within their reach.\r\n"
            "\r\n"
            "The Promenade\r\n"
            "\r\n"
            "Located near Skullport it is the main headquarters for the faithful of\r\n"
            "Eilistraee with many portals to other parts of the world but also access \r\n"
            "to tunnels into the Underdark for redemtion missions. These missions \r\n"
            "would involve sneaking into cities of the Underdark and seeking out \r\n"
            "amongst the drow those who may wish to join Eilistraee's faith and could be redeemed.\r\n"
            "\r\n"
			"The Promenade was led by High Priestess Qilu� Veladorn, youngest of the Seven\r\n"
            "Sisters and Chosen of Mystra. She led the priestesses while also giving out \r\n"
            "missions when something needed her attention. Second to her was The \r\n"
            "Promenade's Battlemistress Rylla. She led both the fierce warrior/priestesses\r\n"
            "of the Darksong Knights (who had training in demon-hunting), and the \r\n"
            "Protectors, the group of elite warriors, often armed with one of the twenty\r\n"
            "magical singing swords, who's first duty was to guard the Promenade. It had \r\n"
            "once played host to a cult of Ghaunadaur and where Qilu� defeated and \r\n"
            "imprisoned Ghaunadaur himself. \r\n"
            "\r\n"
            "One of the highest honors bestowed on a warrior of the Promenade was to be\r\n"
            "granted use of one of the 20 sacred singing swords. These swords were magically\r\n"
            "powerful weapons, with magically durability and sharp edges along with the \r\n"
            "humming providing a defense against Psychic attacks or spells. The \"singing\" of\r\n"
            "the sword would cut through and clear the mind of the attack. They also often\r\n"
            "humm when danger is near also with their semi-sentience they can be mentally \r\n"
            "ordered to be quiet.\r\n"
            "\r\n");


};
