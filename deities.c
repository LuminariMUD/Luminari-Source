#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "db.h"
#include "comm.h"
#include "deities.h"
#include "domains_schools.h"
#include "assign_wpn_armor.h"


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

void assign_deities(void) 
{

  init_deities();

// This is the guide line of 80 characters for writing descriptions of proper
// length.  Cut and paste it where needed then return it here.
// -----------------------------------------------------------------------------


  add_deity(DEITY_NONE, "none", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_ALL, "The Faithless",
	"Those who choose to worship no deity at all are known as the faithless.  It is their\r\n"
	"destiny to become part of the living wall in Kelemvor's domain when they die, to\r\n"
	"ultimately have their very soul devoured and destroyed forever.\r\n");

  add_deity(DEITY_ILMATER, "ilmater", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_HEALING, DOMAIN_LAW, DOMAIN_STRENGTH, DOMAIN_SUFFERING,
            DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_ALL, "Endurance, Suffering, Martyrdom, Perseverance",
            "Ilmater (pronounced: ihl-MAY-ter) is an intermediate deity of the Faerunian\r\n"
            "pantheon whose portfolio included endurance, martyrdom, perseverance, and\r\n"
            "suffering. He is the god of those who suffered, the oppressed, and the\r\n"
            "persecuted, who offers  them relief and support, encouraging them to endure,\r\n"
            "and who encourages others to help them, to take their burdens or take  their\r\n"
            "places. He is called the Crying God, the Broken God, the Lord on the Rack,\r\n"
            "and the One Who Endures.\r\n"
            "\r\n"
    );

  add_deity(DEITY_TEMPUS, "tempus", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS, DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_WAR,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BATTLE_AXE, DEITY_PANTHEON_ALL, "War, Battle, Warriors",
            "Tempus (pronounced: TEM-pus), also known as the Lord of Battles, is the god of\r\n"
            "war. His dogma is primarily concerned with honorable battle, forbidding\r\n"
            "cowardice and encouraging the use of force of arms to settle disputes.\r\n"
            "Tempus is originally one of many potential war gods who emerged from the\r\n"
            "primordial clashes between Selune and Shar. These gods fought constantly with\r\n"
            "each other, the victors absorbing the essence and power of the defeated. This\r\n"
            "continued until Tempus stood as the sole god of war in the Faerunian pantheon,\r\n"
            "having defeated and absorbed all of his competitors. The barbarians of Icewind\r\n"
            "Dale claimed that Tempus' original name was 'Tempos'.\r\n"
            "\r\n"
    );

  add_deity(DEITY_TYMORA, "tymora", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_GOOD, DOMAIN_LUCK, DOMAIN_PROTECTION,
            DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_SHURIKEN, DEITY_PANTHEON_ALL, "Good Fortune, Skill, Victory, Adventurers",
            "Tymora (pronounced: ty-MOR-ah), or more commonly Lady Luck, is the goddess of\r\n"
            "good fortune. She shines upon those who take risks and blesses those who deal\r\n"
            "harshly with the followers of Beshaba. Should someone flee from her sister's\r\n"
            "mischievous followers or defile the dead, their fate would be decided with a\r\n"
            "roll of Tymora's dice. 'Fortune favors the bold.' - The battle cry of\r\n"
            "Tymora's followers.\r\n"
            "\r\n"
    );

  add_deity(DEITY_MASK, "mask", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DARKNESS, DOMAIN_EVIL, DOMAIN_LUCK, DOMAIN_TRICKERY,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_ALL, "Thieves, Thievery, Shadows",
            "Mask (pronounced: MASK), the Lord of Shadows, is the god of shadows and thieves\r\n"
            "in the Faerunian pantheon. He is a loner god most often associating with\r\n"
            "thieves or those of otherwise ill-repute. He is a neutral evil intermediate\r\n"
            "deity from the Shadow Keep in the Plane of Shadow, whose portfolio includes\r\n"
            "shadows, thievery and thieves, and previously also included intrigue. Mask's\r\n"
            "symbol is a black mask made of velvet, tainted with red.\r\n"
            "\r\n"
    );


  add_deity(DEITY_HELM, "helm", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_LAW, DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_ALL,
            "Guardians, Protectors, Protection",
            "Helm (pronounced: HELM), also known as the Vigilant One and The Watcher, is the\r\n"
            "god of guardians, protection, and protectors. He is worshiped by guards and\r\n"
            "paladins both, long being seen as a cold and focused deity who impartially took\r\n"
            "the role of defender and sometimes also enforcer. His activities in the Time of\r\n"
            "Troubles caused the folk of Faerun to look differently on the Watcher.\r\n"
            "\r\n"
    );

  add_deity(DEITY_SHAR, "shar", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_CAVERN, DOMAIN_DARKNESS, DOMAIN_EVIL,
          DOMAIN_KNOWLEDGE, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_KUKRI, DEITY_PANTHEON_ALL,
            "Dark, Night, Loss, Secrets, Underdark",
            "Shar (pronounced: SHAHR), the Mistress of the Night, is the goddess of darkness\r\n"
            "and the caverns of Faerun, as well as a neutral evil greater deity. Counterpart\r\n"
            "to her twin Selune, she presides over caverns, darkness, dungeons,\r\n"
            "forgetfulness, loss, night, secrets, and the Underdark. Among her array of\r\n"
            "twisted powers is the ability to see everything that lay or happened in the\r\n"
            "dark. Shar's symbol is a black disk with a deep purple border. Shar is also the\r\n"
            "creator of the Shadow Weave, which is a counterpart and attack upon the Weave,\r\n"
            "controlled by Mystryl and her successors, before both of the Weaves fell into\r\n"
            "ruin during the Spellplague.\r\n"
            "\r\n"
    );


  add_deity(DEITY_MYSTRA, "mystra", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_ILLUSION, DOMAIN_RUNE, 
          DOMAIN_KNOWLEDGE, DOMAIN_MAGIC, DOMAIN_SPELL, WEAPON_TYPE_SHURIKEN, DEITY_PANTHEON_ALL,
            "Magic, Spells, The Weave",
            "Mystra (pronounced: MISS-trah), the Mother of all Magic, is a greater deity and\r\n"
            "the second incarnation of the goddess of magic after her predecessor Mystryl\r\n"
            "sacrificed herself to protect the Weave from Karsus's Folly. Mystra was\r\n"
            "destroyed by Helm when she defied the will of Ao the overgod and attempted to\r\n"
            "leave the Prime Material Plane during the Time of Troubles. At the conclusion\r\n"
            "of the Godswar, Ao offered the position of Goddess of Magic to the wizard\r\n"
            "Midnight, who reluctantly accepted and took Mystra's name in order to smooth\r\n"
            "the transition after so much chaos.\r\n"
            "\r\n"
    );

  add_deity(DEITY_AKADI, "akadi", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_AIR, DOMAIN_ILLUSION, DOMAIN_TRAVEL,
          DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_FLAIL, DEITY_PANTHEON_ALL,
          "Elemental Air, Movement, Speed, Flying Creatures",
            "Akadi (pronounced: ah-KAH-dee), the Queen of Air, is the embodiment of the\r\n"
            "element of air and goddess of elemental air, speed, and flying creatures. As an\r\n"
            "immortal being of freedom and travel, she instructs her followers to move as\r\n"
            "much as possible from place to place and from activity to activity.\r\n"
            "\r\n"
    );

  add_deity(DEITY_AZUTH, "azuth", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_ILLUSION, DOMAIN_MAGIC, DOMAIN_KNOWLEDGE,
          DOMAIN_LAW, DOMAIN_SPELL, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_FLAIL, DEITY_PANTHEON_ALL,
          "Wizards, Mages, Spellcasters in General",
            "Azuth (pronounced: ah-ZOOTH) is the Faerunian Lesser deity of arcane magic whose\r\n"
            "concerns include the perpetuation of the magical arts as a craft. He is a servant\r\n"
            "of Mystra and is worshiped by all manner of arcane spellcasters, earning\r\n"
            "particular veneration from wizards.\r\n"
            "\r\n"
            "'There was once a wizard who wanted power beyond all mortal reach. Such stories\r\n"
            "always end poorly. But luckily for the wizard, the Lady of the Mysteries took a\r\n"
            "shine to him and became his queen. She granted him powers—such powers—until he\r\n"
            "was no longer a mere wizard but a god in truth. A god dedicated to his lady and\r\n"
            "all who wore her crown.' - Azuth telling his story.\r\n"
            "\r\n"
    );

    add_deity(DEITY_LOLTH, "lolth", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_EVIL, DOMAIN_HATRED,
          DOMAIN_LAW, DOMAIN_TYRANNY, DOMAIN_UNDEFINED, WEAPON_TYPE_WHIP, DEITY_PANTHEON_ALL,
          "Drow, Spiders, Chaos",
            "Lolth (pronounced: lalth loalth), or Lloth, known as the Queen of Spiders as well\r\n"
            "as the Queen of the Demonweb Pits, is the most influential goddess of the drow,\r\n"
            "within the pantheon of the Dark Seldarine. Lolth drives her worshipers into\r\n"
            "heavy infighting under the pretense of culling the weak, while her actual goals\r\n"
            "are to hold absolute control over the dark elves, prevent the rise of alternative\r\n"
            "faiths or ideas, and avoid complacency (even though she found amusement in the\r\n"
            "strife that plagued her followers' communities).\r\n"
            "\r\n"
    );

    add_deity(DEITY_BANE, "bane", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_EVIL, DOMAIN_HATRED,
          DOMAIN_LAW, DOMAIN_TYRANNY, DOMAIN_UNDEFINED, WEAPON_TYPE_MORNINGSTAR, DEITY_PANTHEON_ALL,
          "Hatred, Tyranny, Fear",
            "Bane (pronounced: BAYN) is the Faerunian god of tyrannical oppression, terror,\r\n"
            "and hate, known across Faerun as the face of pure evil through malevolent\r\n"
            "despotism. From his dread plane of Banehold, the Black Hand acts indirectly\r\n"
            "through worshipers and other agents to achieve his ultimate plan to achieve\r\n"
            "total domination of all Faerun.\r\n"
            "\r\n"
    );

    add_deity(DEITY_CHAUNTEA, "chauntea", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_EARTH, DOMAIN_GOOD,
          DOMAIN_PLANT, DOMAIN_PROTECTION, DOMAIN_RENEWAL, WEAPON_TYPE_SCYTHE, DEITY_PANTHEON_ALL,
          "Agriculture, Gardeners, Farmers, Summer",
            "Chauntea (pronounced: chawn-TEE-ah) is the Faerunian goddess of life and\r\n"
            "bounty, who views herself as the embodiment of all things agrarian. The\r\n"
            "Earthmother is seen as the tamer parallel of Silvanus, The Forest Father of\r\n"
            "druidry and wilderness, as she is the deity of agriculture and plant cultivation.\r\n"
            "\r\n"
    );

    add_deity(DEITY_CYRIC, "cyric", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_CHAOS, DOMAIN_EVIL, DOMAIN_DESTRUCTION,
          DOMAIN_ILLUSION, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_ALL,
          "Murder, Lies, Intrigue, Strife, Deception, Illusion",
            "Cyric (pronounced: SEER-ik), is the monomaniacal Faerunian god of strife and\r\n"
            "deception, and the greater god of conflict and murder, as well as lies, intrigue\r\n"
            "and illusion. It is he who murdered Mystra and caused the Spellplague, throwing\r\n"
            "the cosmos into turmoil in an act that cost him much of his following.\r\n"
            "\r\n"
    );

    add_deity(DEITY_GOND, "gond", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CRAFT, DOMAIN_EARTH, DOMAIN_FIRE,
          DOMAIN_KNOWLEDGE, DOMAIN_METAL, DOMAIN_PLANNING, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_ALL,
          "Artifice, Craft, COnstruction, Smithwork",
            "Gond (pronounced: GAHND or GOND), is the Faerunian god of craft, smithing, and\r\n"
            "inventiveness. The Lord of All Smiths pushes for innovation and imaginativeness,\r\n"
            "sometimes to a dangerous degree as a result of his short-sighted desire to create.\r\n"
            "\r\n"
    );

    add_deity(DEITY_GRUMBAR, "grumbar", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CAVERN, DOMAIN_EARTH, DOMAIN_METAL,
          DOMAIN_TIME, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_ALL,
          "Elemental Earth, Solidity, Changelessness, Oaths",
            "Grumbar (pronounced: GRUM-bar) is the elemental embodiment of earth in the\r\n"
            "Realms. He is one of the four elemental deities worshiped in Faerun, also\r\n"
            "known as Elemental Lords, those primordials that remained on Toril when it\r\n"
            "was separated from Abeir. Nonetheless, he retained worshipers and had power\r\n"
            "equivalent to that of a god.\r\n"
            "\r\n"
    );

    add_deity(DEITY_ISTISHIA, "istishia", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DESTRUCTION, DOMAIN_OCEAN, DOMAIN_STORM,
          DOMAIN_TRAVEL, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_ALL,
          "Elemental Water, Purification, Wetness",
            "Istishia (pronounced: is-TISH-ee-ah), also known as King of Water Elementals,\r\n"
            "is the neutral primordial deity of elemental water and purification. Istishia\r\n"
            "is worshiped by Sailors, Pirates, Water Elementals, Water Genasi, some Aquatic\r\n"
            "Elves, and others who feel a bond with the ocean's destructive power. \r\n"
            "\r\n"
    );

    add_deity(DEITY_KELEMVOR, "kelemvor", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_DEATH, DOMAIN_FATE, DOMAIN_LAW,
          DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_BASTARD_SWORD, DEITY_PANTHEON_ALL,
          "Death, the Dead",
            "Kelemvor (pronounced: KELL-em-vor), also known as the Lord of the Dead and Judge\r\n"
            "of the Damned, is the god of death and the dead, and master of the Crystal Spire\r\n"
            "in the Fugue Plane. Fair yet cold, Kelemvor is the god of death and the dead - \r\n"
            "the most recent deity to hold this position, following in the footsteps of Jergal,\r\n"
            "Myrkul, and Cyric. Unlike these other deities, whose rule as gods of the dead made\r\n"
            "the afterlife an uncertain and fearful thing, Kelemvor promoted that death was a\r\n"
            "natural part of life and should not be feared as long as it was understood. As a\r\n"
            "result of his deep respect for life and death, he held the undead in the uttermost\r\n"
            "contempt.\r\n"
            "\r\n"
          );

    add_deity(DEITY_KOSSUTH, "kossuth", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_DESTRUCTION, DOMAIN_FIRE, DOMAIN_RENEWAL,
          DOMAIN_SUFFERING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_ALL,
          "Elemental Fire, Purification through Fire",
            "Kossuth (pronounced: koh-SOOTH), or the Lord of Flames is the god of elemental\r\n"
            "fire. Kossuth is symbolized by the holy symbol of a twining red flame and his\r\n"
            "portfolio covers elemental fire and purification through fire. In the late 15th\r\n"
            "century DR, he was considered not to be a true god but actually an elemental\r\n"
            "primordial, a being whose power rivaled that of a true deity.\r\n"
            "\r\n"
          );

    add_deity(DEITY_LATHANDER, "lathander", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_NOBILITY, DOMAIN_PROTECTION,
          DOMAIN_RENEWAL, DOMAIN_STRENGTH, DOMAIN_SUN, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_ALL,
          "Spring, Dawn, Youth, Birth, Vitality, Athletics",
            "Lathander (pronounced: lah-THÆN-der), whose title is The Morninglord, is a deity\r\n"
            "of creativity, dawn, renewal, birth, athletics, spring, self-perfection, vitality,\r\n"
            "and youth. He favors those who dispell the undead and blesses those who plant new\r\n"
            "life. Lathander is also the god called upon to bless birth and fertility related\r\n"
            "ceremonies. Some saw him as the neutral good aspect of Amaunator but the two were\r\n"
            "considered separate deities again after the Second Sundering.\r\n"
            "\r\n"
          );

    add_deity(DEITY_LOVIATAR, "loviatar", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_LAW, DOMAIN_RETRIBUTION,
          DOMAIN_SUFFERING, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_SPIKED_CHAIN, DEITY_PANTHEON_ALL,
          "Pain, Hurt, Agony, Torment, Suffering Torture",
            "Loviatar (pronounced: loh-VEE-a-tar or loh-vee-A-tar), also known as The Maiden\r\n"
            "of Pain and The Willing Whip, is the evil goddess of agony and is both queen and\r\n"
            "servant to the greater god Bane. Bringing pain and suffering is the aim of all\r\n"
            "Loviatans, either through physical torture or sometimes more subtly and\r\n"
            "psychologically. Beauty, intelligence, and acting are useful attributes of a\r\n"
            "Loviatan, but the ability to fully understand someone is the best skill a Loviatan\r\n"
            "can acquire, as knowing someone fully could help a Loviatan inflict maximum pain,\r\n"
            "one way or another. Loviatar's followers are encouraged to wipe Ilmater's followers\r\n"
            "from the face of the Realms.\r\n"
            "\r\n"
          );

    add_deity(DEITY_MALAR, "malar", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_ANIMAL, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_MOON, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_ALL,
          "Hunters, Stalking, Bloodlust, Evil Lycanthropes",
            "Malar (pronounced: MAHL-arr or MAY-larr), The Beastlord, is the lesser deity of\r\n"
            "the hunt, evil lycanthropes, bestial savagery and bloodlust. His dogma concerns\r\n"
            "savage hunts, the spreading of the curse of lycanthropy, and general contempt\r\n"
            "for civilization. After the events of the Spellplague, he was an exarch of Silvanus.\r\n"
            "'It is ever the way of nature that the strongest should rule.' - Malar explaining\r\n"
            "his philosophy\r\n"
            "\r\n"
          );

    add_deity(DEITY_MIELIKKI, "meilikki", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_ANIMAL, DOMAIN_GOOD, DOMAIN_PLANT,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SCIMITAR, DEITY_PANTHEON_ALL,
          "Forests, Forest Creatures, Rangers, Dryads, Autumn",
            "Mielikki (pronounced: my-LEE-kee), also referred to as the Forest Queen, is the\r\n"
            "neutral good goddess of autumn, druids, dryads, forests, forest creatures, and\r\n"
            "rangers. Her symbol is a gold-horned, blue-eyed unicorn's head facing left. The\r\n"
            "clergy of Mielikki includes clerics, druids (Forestarms), and rangers (Needles).\r\n"
            "\r\n"
          );

    add_deity(DEITY_OGHMA, "oghma", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_CHARM, DOMAIN_LUCK, DOMAIN_KNOWLEDGE,
          DOMAIN_TRAVEL, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_ALL,
          "Knowledge, Invention, Inspiration, Bards",
            "Oghma (pronounced: OGG-mah), also known as The Lord of Knowledge, is the Faerunian\r\n"
            "neutral greater deity of bards, inspiration, invention, and knowledge. Oghma is the\r\n"
            "leader of the Deities of Knowledge and Invention and his home plane is the House\r\n"
            "of Knowledge. His symbol is a blank scroll. Those who worship Oghma include\r\n"
            "artists, bards, cartographers, inventors, loremasters, sages, scholars, scribes\r\n"
            "and wizards—archivists might pray to him as well. They acn be of any alignment,\r\n"
            "unlike most neutral gods. They often wear Oghma's symbol, a silver scroll on a\r\n"
            "chain, as a necklace.\r\n"
            "\r\n"
          );

    add_deity(DEITY_SELUNE, "selune", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_GOOD, DOMAIN_MOON,
          DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_MACE, DEITY_PANTHEON_ALL,
          "Moon, Stars, Navigation, Prophecy, Questers, Good & Neutral Lycanthropes",
            "Selune (pronounced: seh-LOON-eh or seh-LOON-ay), also known as Our Lady of\r\n"
            "Silver, the Moonmaiden, and the Night White Lady, is the goddess of the moon\r\n"
            "in the Faerunian pantheon. She holds the portfolios of the moon, stars,\r\n"
            "navigation, navigators, wanderers, questers, seekers, and non-evil lycanthropes.\r\n"
            "Hers is the moon's mysterious power, the heavenly force that governs the world's\r\n"
            "tides and a mother's reproductive cycles, and causes lycanthropes to shift form.\r\n"
            "Her nature, appearance, and mood all changes in turn with the phases of the\r\n"
            "moon. Her name is shared by the moon of Toril, Selune; it is unknown if the\r\n"
            "moon was named for the goddess or the goddess for the moon. Regardless, most\r\n"
            "Faerunian humans believe the moon to be the goddess herself watching over the\r\n"
            "world and the lights that trail behind it to be her tears, from both joy and sorrow.\r\n"
            "\r\n"
          );

    add_deity(DEITY_SILVANUS, "silvanus", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_ANIMAL, DOMAIN_PLANT, DOMAIN_RENEWAL,
          DOMAIN_PROTECTION, DOMAIN_WATER, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_CLUB, DEITY_PANTHEON_ALL,
          "Wild Nature, Druids",
            "Silvanus (pronounced: sihl-VANN-us) is the Faerunian god of nature, and one of\r\n"
            "the oldest and most prominent gods in the pantheon. Formerly considered only\r\n"
            "the god of wilderness and druids, the Forest Father is often seen as the wilder\r\n"
            "counterpart to Chauntea the Earthmother. The church of Silvanus has a pervasive\r\n"
            "influence, especially across the continent of Faerun. His worshipers protect\r\n"
            "places of nature from the encroachment of civilization with vigor and are\r\n"
            "implacable foes of industrious peoples. Non-worshipers often view the church\r\n"
            "unfavorably due to its tendency to disrupt expansion into woodland, sometimes\r\n"
            "with violence.\r\n"
            "\r\n"
          );

    add_deity(DEITY_SUNE, "sune", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHAOS, DOMAIN_CHARM, DOMAIN_GOOD,
          DOMAIN_PROTECTION, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WHIP, DEITY_PANTHEON_ALL,
          "Beauty, Love, Passion",
            "Sune (pronounced: SOO-nee), also known as Lady Firehair, is the deity of beauty,\r\n"
            "with governance also over love. Her dogma primarily concerns love based on\r\n"
            "outward beauty, with primary importance placed upon loving people who responded\r\n"
            "to the Sunite's appearance. Her symbol is that of a beautiful woman with red hair.\r\n"
            "Sune's clerics seek to bring beauty to the world in many forms, all of which are\r\n"
            "pleasing to the senses. They create great works of art, become patrons for\r\n"
            "promising actors, and import exotic luxuries like satin and fine wines. Her\r\n"
            "followers also enjoy looking beautiful, and hearing tales of romance. The stories\r\n"
            "range from star-crossed love, true love overcoming all else, to following one's heart.\r\n"
            "\r\n"
          );

    add_deity(DEITY_TALONA, "talona", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_SUFFERING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_ALL,
          "Disease, Poison",
            "Talona (pronounced: tah-LO-nah), called the Lady of Poison, Mistress of Disease,\r\n"
            "and Mother of All Plagues, is the goddess of poison and disease. Talona is\r\n"
            "depicted as an old crone who brings misfortune and death. On the other hand, she\r\n"
            "can also be depicted as a beautiful and innocent woman. Her priests, known\r\n"
            "collectively as Talontar, typically wear ragged gray-green robes. Though they\r\n"
            "wash these vestiments, they will never repair them. Older and high-ranking\r\n"
            "members of her priesthood tend to either ritually scar or tattoo their bodies\r\n"
            "all over. When embarking on a battle or dangerous adventure, a follower of\r\n"
            "Talona will often don armor of a black and purple hue that is adorned with a\r\n"
            "variety of spurs, horns, and spikes.\r\n"
            "\r\n"
          );

    add_deity(DEITY_TALOS, "talos", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_FIRE, DOMAIN_CHAOS,
          DOMAIN_EVIL, DOMAIN_STORM, DOMAIN_UNDEFINED, WEAPON_TYPE_SPEAR, DEITY_PANTHEON_ALL,
          "Storms, Destruction, Rebellion, Conflagrations, Earthquakes, Vortices",
            "Talos (pronounced:  TAHL-os), also known as The Storm Lord and long ago as Kozah,\r\n"
            "is the Faerunian greater deity of storms and destruction. His dogma is\r\n"
            "self-serving, demanding utter obedience from his priests and instructing them to\r\n"
            "spread destruction where they might. His followers are known as Talassans. After\r\n"
            "the Spellplague, many came to believe that Talos is, in fact, an aspect of the\r\n"
            "orcish god Gruumsh, but after the Second Sundering the two deities seemed more\r\n"
            "distinct. Clerics of Talos wear black robes and cloaks shot through with\r\n"
            "teardrops and jagged lines of gold and silver while high clergy wear blue-white\r\n"
            "ceremonial robes streaked with crimson. All of them wear eye patches. Talassan\r\n"
            "clerics generally cross-trained as barbarians, sorcerers, wizards, or as stormlords.\r\n"
            "\r\n"
          );

    add_deity(DEITY_TIAMAT, "tiamat", ETHOS_LAWFUL, ALIGNMENT_EVIL, DOMAIN_EVIL, DOMAIN_LAW, DOMAIN_SCALYKIND,
          DOMAIN_TYRANNY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_HEAVY_PICK, DEITY_PANTHEON_ALL,
          "Evil Dragons && Reptiles, Greed, Chessenta",
            "Tiamat (pronounced: TEE-a-mat or TEE-a-maht) is the lawful evil dragon goddess\r\n"
            "of greed, queen of evil dragons and, for a time, a reluctant servant of the\r\n"
            "greater gods Bane and later Asmodeus. Tiamat is also the eternal rival of her\r\n"
            "brother Bahamut, ruler of the good metallic dragons. Many evil dragons have\r\n"
            "worshiped Tiamat since their species first appeared on Toril, and kobolds\r\n"
            "believe she was their creator, and although they didn't worship her as god,\r\n"
            "they revered her as their creator. After the Spellplague, she also gained a\r\n"
            "few dragonborn followers. Tiamat accepts only evil clerics. They, like Tiamat\r\n"
            "herself, seek to place the world under the domination of evil dragons.\r\n"
            "\r\n"
          );

    add_deity(DEITY_TORM, "torm", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_HEALING, DOMAIN_LAW,
          DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_SWORD, DEITY_PANTHEON_ALL,
          "Duty, Loyalty, Obedience, Paladins",
            "Torm (pronounced: TORM) known as The True and The Loyal Fury, is the god whose\r\n"
            "portfolio consisted of duty, loyalty, righteousness, and, after the Spellplague,\r\n"
            "law. His symbol is a right-hand gauntlet held upright with palm forward. Known\r\n"
            "as Tormtar, Torm's worshipers consist mainly of human males and females who\r\n"
            "favored the causes of both good and law. Righteousness, honesty, loyalty and\r\n"
            "truth are their primary pursuits. After the Time of Troubles, the number of\r\n"
            "dwarven and elvish members has been increasing. In the post-Spellplague world,\r\n"
            "which was more dangerous and darker than ever, Torm's followers shine as\r\n"
            "beacons of hope and courage.\r\n"
            "\r\n"
          );

    add_deity(DEITY_TYR, "tyr", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_GOOD, DOMAIN_KNOWLEDGE, DOMAIN_LAW,
          DOMAIN_RETRIBUTION, DOMAIN_WAR, DOMAIN_UNDEFINED, WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_ALL,
          "Justice",
            "Tyr (pronounced: TEER) is the lawful good greater god of law and justice in the\r\n"
            "Faerunian pantheon. He is the leader of the coalition of deities known as the\r\n"
            "Triad. Tyr is particularly popular in the lands of Calimshan, Cormyr, the\r\n"
            "Dalelands, the Moonsea, Sembia, Tethyr, and the Vilhon Reach. Among Tyr's\r\n"
            "worshipers are judges, lawyers, magistrates, the oppressed, paladins, and\r\n"
            "police. The highly organized church of Tyr is strong in the more civilized lands\r\n"
            "of the Realms. They are known for never refusing service or aid to the faithful\r\n"
            "when they are in distress. Followers of Tyr are expected to show fairness,\r\n"
            "wisdom, and kindness to the innocent. Tyrrans never enforce an unjust law.\r\n"
            "\r\n"
          );

    add_deity(DEITY_UMBERLEE, "umberlee", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_DESTRUCTION, DOMAIN_CHAOS, DOMAIN_EVIL,
          DOMAIN_OCEAN, DOMAIN_STORM, DOMAIN_WATER, WEAPON_TYPE_TRIDENT, DEITY_PANTHEON_ALL,
          "Oceans, Currents, Waves, Sea Winds",
            "Umberlee (pronounced: uhm-ber-LEE) is the evil sea goddess in the Faerunian\r\n"
            "pantheon, most often worshiped by sailors or people traveling by sea, out of fear\r\n"
            "of her destructive powers. She is known as a particularly malicious, petty,\r\n"
            "greedy, and vain deity who controls the harshness of the sea while reveling in\r\n"
            "her own power and is not hesitant to drown people if she so pleased. The Grea\r\n"
            "Queen of the Sea is worshiped out of fear by most. Umberlee acts on her\r\n"
            "turbulent whims when making deals with mortals. Temples of Umberlee are few\r\n"
            "and far between, and the church is disorganized, universally hated throughout\r\n"
            "Faerun, and perpetuated out of fear of the goddess. Umberlee's church rested on\r\n"
            "her chaotic nature where disputes were ruled in favor of the strongest individual. \r\n"
            "\r\n"
          );

    add_deity(DEITY_WAUKEEN, "waukeen", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_KNOWLEDGE, DOMAIN_PROTECTION, DOMAIN_TRADE,
          DOMAIN_TRAVEL, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_NUNCHAKU, DEITY_PANTHEON_ALL,
          "Trade, Money, Wealth",
            "Waukeen (pronounced: wau-KEEN or: wah-KEEN) is a lesser deity of the Faerunian\r\n"
            "pantheon known as the Merchant's Friend, Liberty's Maiden, and the Golden Lady.\r\n"
            "Her portfolio includes everything related to commerce and the accumulation of\r\n"
            "wealth through free and fair trade, as well as the beneficial use of wealth to\r\n"
            "improve civilization. Those that venerate and appease her include merchants\r\n"
            "from lowly peddlers to the wealthy owners of trading companies, investors,\r\n"
            "accountants, entrepreneurs, caravan guides, warehouse owners, philanthropists,\r\n"
            "deal-makers, moneylenders, and so on. Waukeen is also the goddess of illicit\r\n"
            "trade and the patron of many smugglers, fences, black marketeers, and\r\n"
            "'businessmen' on the shady side of commerce. Collectively, her worshipers were\r\n"
            "known as Waukeenar.\r\n"
            "\r\n"
          ); 
};

ACMDU(do_devote)
{
      char arg1[MEDIUM_STRING], arg2[MEDIUM_STRING], dname[SMALL_STRING];

      half_chop(argument, arg1, arg2);
      int listtype = DEITY_LIST_ALL;
      int i = 0;

      if (!*arg1)
      {
            send_to_char(ch, "Please specify whether you wish to 'choose' a deity, get 'info' on one, or 'list' them.\r\n");
            return;
      }
      if (is_abbrev(arg1, "list"))
      {
            if (!*arg2)
            {
                  send_to_char(ch, "Please specify whether you wish to list 'all' deities or ones with the following keywords:\r\n");
                  //send_to_char(ch, "options: all|good|neutral|evil|lawful|chaotic|human|elf|dwarf|halfling|gnome|underdark\r\n");
                  send_to_char(ch, "options: all|good|neutral|evil|lawful|chaotic\r\n");
                  return;
            }
            if (is_abbrev(arg2, "all"))
                  listtype = DEITY_LIST_ALL;
            else if (is_abbrev(arg2, "good"))
                  listtype = DEITY_LIST_GOOD;
            else if (is_abbrev(arg2, "neutral"))
                  listtype = DEITY_LIST_NEUTRAL;
            else if (is_abbrev(arg2, "evil"))
                  listtype = DEITY_LIST_EVIL;
            else if (is_abbrev(arg2, "lawful"))
                  listtype = DEITY_LIST_LAWFUL;
            else if (is_abbrev(arg2, "chaotic"))
                  listtype = DEITY_LIST_CHAOTIC;
            else
            {
                  send_to_char(ch, "Please specify whether you wish to list 'all' deities or ones with the following keywords:\r\n");
                  //send_to_char(ch, "options: all|good|neutral|evil|lawful|chaotic|human|elf|dwarf|halfling|gnome|underdark\r\n");
                  send_to_char(ch, "options: all|good|neutral|evil|lawful|chaotic\r\n");
                  return;
            }

            send_to_char(ch, "%-15s - %-15s %s\r\n", "Deities of Faerun", "Alignment", "Portfolio");
            for (i = 0; i < 80; i++)
                  send_to_char(ch, "-");
            send_to_char(ch, "\r\n");

            for (i = 0; i < NUM_DEITIES; i++)
            {
                  if (deity_list[i].pantheon != DEITY_PANTHEON_ALL) continue;
                  if (listtype == DEITY_LIST_GOOD && deity_list[i].alignment != ALIGNMENT_GOOD) continue;
                  else if (listtype == DEITY_LIST_NEUTRAL && deity_list[i].alignment != ALIGNMENT_GOOD && deity_list[i].ethos != ETHOS_NEUTRAL) continue;
                  else if (listtype == DEITY_LIST_EVIL && deity_list[i].alignment != ALIGNMENT_EVIL) continue;
                  else if (listtype == DEITY_LIST_LAWFUL && deity_list[i].ethos != ETHOS_LAWFUL) continue;
                  else if (listtype == DEITY_LIST_CHAOTIC && deity_list[i].ethos != ETHOS_CHAOTIC) continue;
                  snprintf(dname, sizeof(dname), "%s", deity_list[i].name);
                  send_to_char(ch, "%-15s - %-15s - %s\r\n", CAP(dname), GET_ALIGN_STRING(deity_list[i].ethos, deity_list[i].alignment), deity_list[i].portfolio);
            }
      }
      else if (is_abbrev(arg1, "info"))
      {
            if (!*arg2)
            {
                  send_to_char(ch, "Please specify a deity to get info on.  Use 'devote list' to see the deity options.\r\n");
                  return;
            }
            for (i = 0; i < NUM_DEITIES; i++)
            {
                  if (deity_list[i].pantheon != DEITY_PANTHEON_ALL) continue;
                  if (is_abbrev(arg2, deity_list[i].name))
                  {
                        break;
                  }
            }
            if (i < 0 || i >= NUM_DEITIES)
            {
                  send_to_char(ch, "There is no deity by that name.\r\n");
                  return;
            }
            snprintf(dname, sizeof(dname), "%s", deity_list[i].name);
            send_to_char(ch, "\tA%s\r\n\tn", CAP(dname));
            send_to_char(ch, "\r\n");
            send_to_char(ch, "\tAAlignment:\tn %s\r\n", GET_ALIGN_STRING(deity_list[i].ethos, deity_list[i].alignment));
            send_to_char(ch, "\tAPortfolio:\tn %s\r\n", deity_list[i].portfolio);
            send_to_char(ch, "\tADescription:\tn\r\n%s\r\n", deity_list[i].description);
            return;
      }
      else if (is_abbrev(arg1, "choose"))
      {
            if (GET_DEITY(ch) != DEITY_NONE)
            {
                  send_to_char(ch, "You have already chosen your deity.  If you wish to change your deity selection, please ask a staff member.\r\n");
                  return;
            }
            if (!*arg2)
            {
                  send_to_char(ch, "Please specify the deity you want to worship.  Use 'devote list' to see the deity options.\r\n");
                  return;
            }
            for (i = 0; i < NUM_DEITIES; i++)
            {
                  if (deity_list[i].pantheon != DEITY_PANTHEON_ALL) continue;
                  if (is_abbrev(arg2, deity_list[i].name))
                  {
                        break;
                  }
            }
            if (i < 0 || i >= NUM_DEITIES)
            {
                  send_to_char(ch, "There is no deity by that name.\r\n");
                  return;
            }
            GET_DEITY(ch) = i;
            snprintf(dname, sizeof(dname), "%s", deity_list[i].name);
            send_to_char(ch, "You are now a worshipper of %s!\r\n", CAP(dname));
            save_char(ch, 0);
      }
}