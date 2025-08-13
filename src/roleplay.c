// These are miscellanous functions and features that
// contribute to the various role playing systems in
// the game. Not all 'role play' features are found in
// this file, however.

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "backgrounds.h"
#include "treasure.h"
#include "handler.h"
#include "fight.h"
#include "spec_procs.h"
#include "clan.h"
#include "dg_scripts.h"
#include "feats.h"
#include "constants.h"
#include "improved-edit.h"
#include "roleplay.h"
#include "deities.h"
#include "char_descs.h"
#include "modify.h"

extern const char * personality_traits[NUM_BACKGROUNDS][10];
extern const char * character_ideals[NUM_BACKGROUNDS][8];
extern const char * character_bonds[NUM_BACKGROUNDS][8];
extern const char * character_flaws[NUM_BACKGROUNDS][8];

#define GOAL_DESCRIPTION ("Character goals are an important part of role playing. A character usually has " \
      "multiple goals, some short term, some mid term and some long term. Sometimes " \
      "these goals are interconnected and sometimes they aren't.  A goal should have " \
      "an objective, a reason for pursuing it and one or more obstacles to achieving " \
      "the goal. A goal should be something achievable, and it should have a consequence " \
      "to failure. Goals can also change as a character grows and progresses. As a character " \
      "achieves or fails at completing their goals, their goals should be adjusted, with some " \
      "potentially removed while other new ones are added. Recording these here in-game helps " \
      "you establish a solid base for role playing your character, and it assists role-play " \
      "staff in helping you in your person role play stories as well as weave you into more " \
      "wide and global narratives. If you're serious about role play, this is well worth your time.")

#define PERSONALITY_DESCRIPTION ("Personality traits are small, simple ways to help you set your character apart " \
      "from every other character. Your personality traits should tell you something " \
      "interesting and fun about your character. They should be self-descriptions that " \
      "are specific about what makes your character stand out. \"I'm smart\" is not a " \
      "good trait, because it describes a lot of characters. \"I've read every book in " \
      "Palanthas\" tells you something specific about your character's interests and " \
      "disposition. " \
      "Personality traits might describe the things your character likes, his or her " \
      "past accomplishments, things your character dislikes or fears, your character's " \
      "self-attitude or mannerisms, or the influence of his or her ability scores. " \
      "A useful place to start thinking about personality traits is to look at your " \
      "highest and lowest ability scores and define one trait related to each. Either " \
      "one could be positive or negative: you might work hard to overcome a low score, " \
      "for example, or be cocky about your high score. ")

#define BONDS_DESCRIPTION ("Bonds represent a character's connections to people, places, and events in the " \
      "world. They tie you to things from your background. They might inspire you to " \
      "heights of heroism, or lead you to act against your own best interests if they " \
      "are threatened. They can work very much like ideals, driving a character's " \
      "motivations and goals. " \
      "Bonds might answer any of these questions: Whom do you care most about? To what " \
      "place do you feel a special connection? What is your most treasured possession? " \
      "Your bonds might be tied to your class, your background, your race, or some " \
      "other aspect of your character's history or personality. You might also gain new " \
      "bonds over the course of your adventures. ")

#define IDEALS_DESCRIPTION ("Your ideals are the things that you believe in most strongly, the fundamental " \
      "moral and ethical principles that compel you to act as you do. Ideals encompass " \
      "everything from your life goals to your core belief system. " \
      "Ideals might answer any of these questions: What are the principles that you " \
      "will never betray? What would prompt you to make sacrifices? What drives you to " \
      "act and guides your goals and ambitions? What is the single most important thing " \
      "you strive for? " \
      "You can choose any ideals you like, but your character's alignment is a good " \
      "place to start defining them. Each background in this chapter includes six " \
      "suggested ideals. Five of them are linked to aspects of alignment: law, chaos, " \
      "good, evil, and neutrality. The last one has more to do with the particular " \
      "background than with moral or ethical perspectives. ")

#define FLAWS_DESCRIPTION ("Your character's flaw represents some vice, compulsion, fear, or weakness—in " \
      "particular, anything that someone else could exploit to bring you to ruin or " \
      "cause you to act against your best interests. More significant than negative " \
      "personality traits, a flaw might answer any of these questions: What enrages " \
      "you? What's the one person, concept, or event that you are terrified of? What " \
      "are your vices? ")

const char * personality_traits[NUM_BACKGROUNDS][10] =
{
    { // none - 0
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        ""
    },
    { // acolyte
        "",
        "I idolize a particular hero of my faith, and constantly refer to that person's deeds and example.",
        "I can find common ground between the fiercest enemies, empathizing with them and always working toward peace.",
        "I see omens in every event and action. The gods try to speak to us, we just need to listen.",
        "Nothing can shake my optimistic attitude.",
        "I quote (or misquote) sacred texts and proverbs in almost every situation.",
        "I am tolerant (or intolerant) of other faiths and respect (or condemn) the worship of other gods.",
        "I've enjoyed fine food, drink, and high society among my temple's elite. Rough living grates on me.",
        "I've spent so long in the temple that I have little practical experience dealing with people in the outside world.",
        ""
    },
    { // charlatan
        "",
        "I fall in and out of love easily, and am always pursuing someone.",
        "I have a joke for every occasion, especially occasions where humor is inappropriate.",
        "Flattery is my preferred trick for getting what I want.",
        "I'm a born gambler who can't resist taking a risk for a potential payoff.",
        "I lie about almost everything, even when there's no good reason to.",
        "Sarcasm and insults are my weapons of choice.",
        "I keep multiple holy symbols on me and invoke whatever deity might come in useful at any given moment.",
        "I pocket anything I see that might have some value.",
        ""
    },
    { // criminal
        "",
        "I always have a plan for what to do when things go wrong.",
        "I am always calm, no matter what the situation. I never raise my voice or let my emotions control me.",
        "The first thing I do in a new place is note the locations of everything valuable-or where such things could be hidden.",
        "I would rather make a new friend than a new enemy.",
        "I am incredibly slow to trust. Those who seem the fairest often have the most to hide.",
        "I don't pay attention to the risks in a situation. Never tell me the odds.",
        "The best way to get me to do something is to tell me I can't do it.",
        "I blow up at the slightest insult.",
        ""
    },
    { // entertainer
        "",
        "I know a story relevant to almost every situation.",
        "Whenever I come to a new place, I collect local rumors and spread gossip.",
        "I'm a hopeless romantic, always searching for that 'special someone.'",
        "Nobody stays angry at me or around me for long, since I can defuse any amount of tension.",
        "I love a good insult, even one directed at me.",
        "I get bitter if I'm not the center of attention.",
        "I'll settle for nothing less than perfection.",
        "I change my mood or my mind as quickly as I change key in a song.",
        ""
    },
    { // folk hero
        "",
        "I judge people by their actions, not their words.",
        "If someone is in trouble, I'm always ready to lend help.",
        "When I set my mind to something, I follow through no matter what gets in my way.",
        "I have a strong sense of fair play and always try to find the most equitable solution to arguments.",
        "I'm confident in my own abilities and do what I can to instill confidence in others.",
        "Thinking is for other people. I prefer action.",
        "I misuse long words in an attempt to sound smarter.",
        "I get bored easily. When am I going to get on with my destiny?",
        ""
    },
    { //gladiator
        "",
        "I know a story relevant to almost every situation.",
        "Whenever I come to a new place, I collect local rumors and spread gossip.",
        "I'm a hopeless romantic, always searching for that 'special someone.'",
        "Nobody stays angry at me or around me for long, since I can defuse any amount of tension.",
        "I love a good insult, even one directed at me.",
        "I get bitter if I'm not the center of attention.",
        "I'll settle for nothing less than perfection.",
        "I change my mood or my mind as quickly as I change key in a song.",
        ""
    },
    { // trader
        "",
        "I believe that anything worth doing is worth doing right. I can't help it-I'm a perfectionist.",
        "I'm a snob who looks down on those who can't appreciate fine art.",
        "I always want to know how things work and what makes people tick.",
        "I'm full of witty aphorisms and have a proverb for every occasion.",
        "I'm rude to people who lack my commitment to hard work and fair play.",
        "I like to talk at length about my profession.",
        "I don't part with my money easily and will haggle tirelessly to get the best deal possible.",
        "I'm well known for my work, and I want to make sure everyone appreciates it. I'm always taken aback when people haven't heard of me.",
        ""
    },
    { // hermit
        "",
        "I've been isolated for so long that I rarely speak, preferring gestures and the occasional grunt.",
        "I am utterly serene, even in the face of disaster.",
        "The leader of my community had something wise to say on every topic, and I am eager to share that wisdom.",
        "I feel tremendous empathy for all who suffer.",
        "I'm oblivious to etiquette and social expectations.",
        "I connect everything that happens to me to a grand, cosmic plan.",
        "I often get lost in my own thoughts and contemplation, becoming oblivious to my surroundings.",
        "I am working on a grand philosophical theory and love sharing my ideas.",
        ""
    },
    { // squire
        "",
        "My eloquent flattery makes everyone I talk to feel like the most wonderful and important person in the world.",
        "The common folk love me for my kindness and generosity.",
        "No one could doubt by looking at my regal bearing that I am a cut above the unwashed masses.",
        "I take great pains to always look my best and follow the latest fashions.",
        "I don't like to get my hands dirty, and I won't be caught dead in unsuitable accommodations.",
        "Despite my noble birth, I do not place myself above other folk. We all have the same blood.",
        "My favor, once lost, is lost forever.",
        "If you do me an injury, I will crush you, ruin your name, and salt your fields.",
        ""
    },
    { // noble
        "",
        "My eloquent flattery makes everyone I talk to feel like the most wonderful and important person in the world.",
        "The common folk love me for my kindness and generosity.",
        "No one could doubt by looking at my regal bearing that I am a cut above the unwashed masses.",
        "I take great pains to always look my best and follow the latest fashions.",
        "I don't like to get my hands dirty, and I won't be caught dead in unsuitable accommodations.",
        "Despite my noble birth, I do not place myself above other folk. We all have the same blood.",
        "My favor, once lost, is lost forever.",
        "If you do me an injury, I will crush you, ruin your name, and salt your fields.",
        ""
    },
    { // outlander
        "",
        "I'm driven by a wanderlust that led me away from home.",
        "I watch over my friends as if they were a litter of newborn pups.",
        "I once ran twenty-five miles without stopping to warn to my clan of an approaching orc horde. I'd do it again if I had to.",
        "I have a lesson for every situation, drawn from observing nature.",
        "I place no stock in wealthy or well-mannered folk. Money and manners won't save you from a hungry owlbear.",
        "I'm always picking things up, absently fiddling with them, and sometimes accidentally breaking them.",
        "I feel far more comfortable around animals than people.",
        "I was, in fact, raised by wolves.",
        ""
    },
    { // pirate
        "",
        "My friends know they can rely on me, no matter what.",
        "I work hard so that I can play hard when the work is done.",
        "I enjoy sailing into new ports and making new friends over a flagon of ale.",
        "I stretch the truth for the sake of a good story.",
        "To me, a tavern brawl is a nice way to get to know a new city.",
        "I never pass up a friendly wager.",
        "My language is as foul as an otyugh nest.",
        "I like a job well done, especially if I can convince someone else to do it.",
        ""
    },
    { // sage
        "",
        "I use polysyllabic words that convey the impression of great erudition.",
        "I've read every book in the world's greatest libraries-or I like to boast that I have.",
        "I'm used to helping out those who aren't as smart as I am, and I patiently explain anything and everything to others.",
        "There's nothing I like more than a good mystery.",
        "I'm willing to listen to every side of an argument before I make my own judgment.",
        "I ... speak ... slowly ... when talking ... to idiots, ... which ... almost ... everyone ... is ... compared ... to me.",
        "I am horribly, horribly awkward in social situations.",
        "I'm convinced that people are always trying to steal my secrets.",
        ""
    },
    { // sailor
        "",
        "My friends know they can rely on me, no matter what.",
        "I work hard so that I can play hard when the work is done.",
        "I enjoy sailing into new ports and making new friends over a flagon of ale.",
        "I stretch the truth for the sake of a good story.",
        "To me, a tavern brawl is a nice way to get to know a new city.",
        "I never pass up a friendly wager.",
        "My language is as foul as an otyugh nest.",
        "I like a job well done, especially if I can convince someone else to do it.",
        ""
    },
    { // soldier
        "",
        "I'm always polite and respectful.",
        "I'm haunted by memories of war. I can't get the images of violence out of my mind.",
        "I've lost too many friends, and I'm slow to make new ones.",
        "I'm full of inspiring and cautionary tales from my military experience relevant to almost every combat situation.",
        "I can stare down a hell hound without flinching.",
        "I enjoy being strong and like breaking things.",
        "I have a crude sense of humor.",
        "I face problems head-on. A simple, direct solution is the best path to success.",
        ""
    },
    { // urchin
        "",
        "I hide scraps of food and trinkets away in my pockets.",
        "I ask a lot of questions.",
        "I like to squeeze into small places where no one else can get to me.",
        "I sleep with my back to a wall or tree, with everything I own wrapped in a bundle in my arms.",
        "I eat like a pig and have bad manners.",
        "I think anyone who's nice to me is hiding evil intent.",
        "I don't like to bathe.",
        "I bluntly say what other people are hinting at or hiding.",
        ""
    }
};

const char * character_ideals[NUM_BACKGROUNDS][8] =
{
 {
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  ""
 },
 { // acolyte
  "",
  "Tradition. The ancient traditions of worship and sacrifice must be preserved and upheld. (Lawful)",
  "Charity. I always try to help those in need, no matter what the personal cost. (Good)",
  "Change. We must help bring about the changes the gods are constantly working in the world. (Chaotic)",
  "Power. I hope to one day rise to the top of my faith's religious hierarchy. (Lawful)",
  "Faith. I trust that my deity will guide my actions. I have faith that if I work hard, things will go well. (Lawful)",
  "Aspiration. I seek to prove myself worthy of my god's favor by matching my actions against his or her teachings. (Any)",
  ""
 },
 { // charlatan
  "",
  "Independence. I am a free spirit-no one tells me what to do. (Chaotic)",
  "Fairness. I never target people who can't afford to lose a few coins. (Lawful)",
  "Charity. I distribute the money I acquire to the people who really need it. (Good)",
  "Creativity. I never run the same con twice. (Chaotic)",
  "Friendship. Material goods come and go. Bonds of friendship last forever. (Good)",
  "Aspiration. I'm determined to make something of myself. (Any)",
  ""
 },
 { // criminal
  "",
  "Honor. I don't steal from others in the trade. (Lawful)",
  "Freedom. Chains are meant to be broken, as are those who would forge them. (Chaotic)",
  "Charity. I steal from the wealthy so that I can help people in need. (Good)",
  "Greed. I will do whatever it takes to become wealthy. (Evil)",
  "People. I'm loyal to my friends, not to any ideals, and everyone else can take a trip down the Styx for all I care. (Neutral)",
  "Redemption. There's a spark of good in everyone. (Good)",
  ""
 },
 { // entertainer
  "",
  "Beauty. When I perform, I make the world better than it was. (Good)",
  "Tradition. The stories, legends, and songs of the past must never be forgotten, for they teach us who we are. (Lawful)",
  "Creativity. The world is in need of new ideas and bold action. (Chaotic)",
  "Greed. I'm only in it for the money and fame. (Evil)",
  "People. I like seeing the smiles on people's faces when I perform. That's all that matters. (Neutral)",
  "Honesty. Art should reflect the soul; it should come from within and reveal who we really are. (Any)",
  ""
 },
 { // folk hero
  "",
  "Respect. People deserve to be treated with dignity and respect. (Good)",
  "Fairness. No one should get preferential treatment before the law, and no one is above the law. (Lawful)",
  "Freedom. Tyrants must not be allowed to oppress the people. (Chaotic)",
  "Might. If I become strong, I can take what I want-what I deserve. (Evil)",
  "Sincerity. There's no good in pretending to be something I'm not. (Neutral)",
  "Destiny. Nothing and no one can steer me away from my higher calling. (Any)",
  ""
 },
 { // gladiator
  "",
  "Beauty. When I perform, I make the world better than it was. (Good)",
  "Tradition. The stories, legends, and songs of the past must never be forgotten, for they teach us who we are. (Lawful)",
  "Creativity. The world is in need of new ideas and bold action. (Chaotic)",
  "Greed. I'm only in it for the money and fame. (Evil)",
  "People. I like seeing the smiles on people's faces when I perform. That's all that matters. (Neutral)",
  "Honesty. Art should reflect the soul; it should come from within and reveal who we really are. (Any)",
  ""
 },
 { // trader
  "",
  "Community. It is the duty of all people to strengthen the bonds and security of communities. (Lawful)",
  "Generosity. My talents were given to me so that I could use them to benefit the world. (Good)",
  "Freedom. Everyone should be free to pursue his or her own livelihood. (Chaotic)",
  "Greed. I'm only in it for the money. (Evil)",
  "People. I'm committed to the people I care about, not to ideals. (Neutral)",
  "Aspiration. I work hard to be the best there is at my craft. (Any)",
  ""
 },
 { // hermit
  "",
  "Greater Good. My gifts are meant to be shared with all, not used for my own benefit. (Good)",
  "Logic. Emotions must not cloud our sense of what is right and true, or our logical thinking. (Lawful)",
  "Free Thinking. Inquiry and curiosity are the pillars of progress. (Chaotic)",
  "Power. Solitude and contemplation are paths toward mystical or magical power. (Evil)",
  "Live and Let Live. Meddling in the affairs of others only causes trouble. (Neutral)",
  "Self-Knowledge. If you know yourself, there's nothing left to know. (Any)",
  ""
 },
 { // squire
  "",
  "Respect. Respect is due to me because of my position, but all people regardless of station deserve to be treated with dignity. (Good)",
  "Responsibility. It is my duty to respect the authority of those above me, just as those below me must respect mine. (Lawful)",
  "Independence. I must prove that I can handle myself without the coddling of my family. (Chaotic)",
  "Power. If I can attain more power, no one will tell me what to do. (Evil)",
  "Family. Blood runs thicker than water. (Any)",
  "Noble Obligation. It is my duty to protect and care for the people beneath me. (Good)",
  ""
 },
 { // noble
  "",
  "Respect. Respect is due to me because of my position, but all people regardless of station deserve to be treated with dignity. (Good)",
  "Responsibility. It is my duty to respect the authority of those above me, just as those below me must respect mine. (Lawful)",
  "Independence. I must prove that I can handle myself without the coddling of my family. (Chaotic)",
  "Power. If I can attain more power, no one will tell me what to do. (Evil)",
  "Family. Blood runs thicker than water. (Any)",
  "Noble Obligation. It is my duty to protect and care for the people beneath me. (Good)",
  ""
 },
 { // outlander
  "",
  "Change. Life is like the seasons, in constant change, and we must change with it. (Chaotic)",
  "Greater Good. It is each person's responsibility to make the most happiness for the whole tribe. (Good)",
  "Honor. If I dishonor myself, I dishonor my whole clan. (Lawful)",
  "Might. The strongest are meant to rule. (Evil)",
  "Nature. The natural world is more important than all the constructs of civilization. (Neutral)",
  "Glory. I must earn glory in battle, for myself and my clan. (Any)",
  ""
 },
 { // pirate
  "",
  "Respect. The thing that keeps a ship together is mutual respect between captain and crew. (Good)",
  "Fairness. We all do the work, so we all share in the rewards. (Lawful)",
  "Freedom. The sea is freedom-the freedom to go anywhere and do anything. (Chaotic)",
  "Mastery. I'm a predator, and the other ships on the sea are my prey. (Evil)",
  "People. I'm committed to my crewmates, not to ideals. (Neutral)",
  "Aspiration. Someday I'll own my own ship and chart my own destiny. (Any",
  ""
 },
 { // sage
  "",
  "Knowledge. The path to power and self-improvement is through knowledge. (Neutral)",
  "Beauty. What is beautiful points us beyond itself toward what is true. (Good)",
  "Logic. Emotions must not cloud our logical thinking. (Lawful)",
  "No Limits. Nothing should fetter the infinite possibility inherent in all existence. (Chaotic)",
  "Power. Knowledge is the path to power and domination. (Evil)",
  "Self-Improvement. The goal of a life of study is the betterment of oneself. (Any)",
  ""
 },
 { // sailor
  "",
  "Respect. The thing that keeps a ship together is mutual respect between captain and crew. (Good)",
  "Fairness. We all do the work, so we all share in the rewards. (Lawful)",
  "Freedom. The sea is freedom-the freedom to go anywhere and do anything. (Chaotic)",
  "Mastery. I'm a predator, and the other ships on the sea are my prey. (Evil)",
  "People. I'm committed to my crewmates, not to ideals. (Neutral)",
  "Aspiration. Someday I'll own my own ship and chart my own destiny. (Any)",
  ""
 },
 { // soldier
  "",
  "Greater Good. Our lot is to lay down our lives in defense of others. (Good)",
  "Responsibility. I do what I must and obey just authority. (Lawful)",
  "Independence. When people follow orders blindly, they embrace a kind of tyranny. (Chaotic)",
  "Might. In life as in war, the stronger force wins. (Evil)",
  "Live and Let Live. Ideals aren't worth killing over or going to war for. (Neutral)",
  "Nation. My city, nation, or people are all that matter. (Any)",
  ""
 },
 { // urchin
  "",
  "Respect. All people, rich or poor, deserve respect. (Good)",
  "Community. We have to take care of each other, because no one else is going to do it. (Lawful)",
  "Change. The low are lifted up, and the high and mighty are brought down. Change is the nature of things. (Chaotic)",
  "Retribution. The rich need to be shown what life and death are like in the gutters. (Evil)",
  "People. I help the people who help me - that's what keeps us alive. (Neutral)",
  "Aspiration. I'm going to prove that I'm worthy of a better life.",
  ""
 }
};

const char * character_bonds[NUM_BACKGROUNDS][8] =
{
 {
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  ""
 },
 { // acolyte
  "",
  "I would die to recover an ancient relic of my faith that was lost long ago.",
  "I will someday get revenge on the corrupt temple hierarchy who branded me a heretic.",
  "I owe my life to the priest who took me in when my parents died.",
  "Everything I do is for the common people.",
  "I will do anything to protect the temple where I served.",
  "I seek to preserve a sacred text that my enemies consider heretical and seek to destroy.",
  ""
 },
 { // charlatan
  "",
  "I fleeced the wrong person and must work to ensure that this individual never crosses paths with me or those I care about.",
  "I owe everything to my mentor-a horrible person who's probably rotting in jail somewhere.",
  "Somewhere out there, I have a child who doesn't know me. I'm making the world better for him or her.",
  "I come from a noble family, and one day I'll reclaim my lands and title from those who stole them from me.",
  "A powerful person killed someone I love. Some day soon, I'll have my revenge.",
  "I swindled and ruined a person who didn't deserve it. I seek to atone for my misdeeds but might never be able to forgive myself.",
  ""
 },
 { // criminal
  "",
  "I'm trying to pay off an old debt I owe to a generous benefactor.",
  "My ill-gotten gains go to support my family.",
  "Something important was taken from me, and I aim to steal it back.",
  "I will become the greatest thief that ever lived.",
  "I'm guilty of a terrible crime. I hope I can redeem myself for it.",
  "Someone I loved died because of a mistake I made. That will never happen again.",
  ""
 },
 { // entertainer
  "",
  "My instrument is my most treasured possession, and it reminds me of someone I love.",
  "Someone stole my precious instrument, and someday I'll get it back.",
  "I want to be famous, whatever it takes.",
  "I idolize a hero of the old tales and measure my deeds against that person's.",
  "I will do anything to prove myself superior to my hated rival.",
  "I would do anything for the other members of my old troupe.",
  ""
 },
 { // folk hero
  "",
  "I have a family, but I have no idea where they are. One day, I hope to see them again.",
  "I worked the land, I love the land, and I will protect the land.",
  "A proud noble once gave me a horrible beating, and I will take my revenge on any bully I encounter.",
  "My tools are symbols of my past life, and I carry them so that I will never forget my roots.",
  "I protect those who cannot protect themselves.",
  "I wish my childhood sweetheart had come with me to pursue my destiny.",
  ""
 },
 { // gladiator
  "",
  "My instrument is my most treasured possession, and it reminds me of someone I love.",
  "Someone stole my precious instrument, and someday I'll get it back.",
  "I want to be famous, whatever it takes.",
  "I idolize a hero of the old tales and measure my deeds against that person's.",
  "I will do anything to prove myself superior to my hated rival.",
  "I would do anything for the other members of my old troupe.",
  ""
 },
 { // trader
  "",
  "The workshop where I learned my trade is the most important place in the world to me.",
  "I created a great work for someone, and then found them unworthy to receive it. I'm still looking for someone worthy.",
  "I owe my guild a great debt for forging me into the person I am today.",
  "I pursue wealth to secure someone's love.",
  "One day I will return to my guild and prove that I am the greatest artisan of them all.",
  "I will get revenge on the evil forces that destroyed my place of business and ruined my livelihood.",
  ""
 },
 { // hermit
  "",
  "Nothing is more important than the other members of my hermitage, order, or association.",
  "I entered seclusion to hide from the ones who might still be hunting me. I must someday confront them.",
  "I'm still seeking the enlightenment I pursued in my seclusion, and it still eludes me.",
  "I entered seclusion because I loved someone I could not have.",
  "Should my discovery come to light, it could bring ruin to the world.",
  "My isolation gave me great insight into a great evil that only I can destroy.",
  ""
 },
 { // squire
  "",
  "I will face any challenge to win the approval of my family.",
  "My house's alliance with another noble family must be sustained at all costs.",
  "Nothing is more important than the other members of my family.",
  "I am in love with the heir of a family that my family despises.",
  "My loyalty to my sovereign is unwavering.",
  "The common folk must see me as a hero of the people.",
  ""
 },
 { // noble
  "",
  "I will face any challenge to win the approval of my family.",
  "My house's alliance with another noble family must be sustained at all costs.",
  "Nothing is more important than the other members of my family.",
  "I am in love with the heir of a family that my family despises.",
  "My loyalty to my sovereign is unwavering.",
  "The common folk must see me as a hero of the people.",
  ""
 },
 { // outlander
  "",
  "My family, clan, or tribe is the most important thing in my life, even when they are far from me.",
  "An injury to the unspoiled wilderness of my home is an injury to me.",
  "I will bring terrible wrath down on the evildoers who destroyed my homeland.",
  "I am the last of my tribe, and it is up to me to ensure their names enter legend.",
  "I suffer awful visions of a coming disaster and will do anything to prevent it.",
  "It is my duty to provide children to sustain my tribe.",
  ""
 },
 { // pirate
  "",
  "I'm loyal to my captain first, everything else second.",
  "The ship is most important-crewmates and captains come and go.",
  "I'll always remember my first ship.",
  "In a harbor town, I have a paramour whose eyes nearly stole me from the sea.",
  "I was cheated out of my fair share of the profits, and I want to get my due.",
  "Ruthless pirates murdered my captain and crewmates, plundered our ship, and left me to die. Vengeance will be mine.",
  ""
 },
 { // sage
  "",
  "It is my duty to protect my students.",
  "I have an ancient text that holds terrible secrets that must not fall into the wrong hands.",
  "I work to preserve a library, university, scriptorium, or monastery.",
  "My life's work is a series of tomes related to a specific field of lore.",
  "I've been searching my whole life for the answer to a certain question.",
  "I sold my soul for knowledge. I hope to do great deeds and win it back.",
  ""
 },
 { // sailor
  "",
  "I'm loyal to my captain first, everything else second.",
  "The ship is most important-crewmates and captains come and go.",
  "I'll always remember my first ship.",
  "In a harbor town, I have a paramour whose eyes nearly stole me from the sea.",
  "I was cheated out of my fair share of the profits, and I want to get my due.",
  "Ruthless pirates murdered my captain and crewmates, plundered our ship, and left me to die. Vengeance will be mine.",
  ""
 },
 { // soldier
  "",
  "I would still lay down my life for the people I served with.",
  "Someone saved my life on the battlefield. To this day, I will never leave a friend behind.",
  "My honor is my life.",
  "I'll never forget the crushing defeat my company suffered or the enemies who dealt it.",
  "Those who fight beside me are those worth dying for.",
  "I fight for those who cannot fight for themselves.",
  ""
 },
 { // urchin
  "",
  "My town or city is my home, and I'll fight to defend it.",
  "I sponsor an orphanage to keep others from enduring what I was forced to endure.",
  "I owe my survival to another urchin who taught me to live on the streets.",
  "I owe a debt I can never repay to the person who took pity on me.",
  "I escaped my life of poverty by robbing an important person, and I'm wanted for it.",
  "No one else should have to endure the hardships I've been through.",
  ""
 }
};

const char * character_flaws[NUM_BACKGROUNDS][8] =
{
 {
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  ""
 },
 { // acolyte
  "",
  "I judge others harshly, and myself even more severely.",
  "I put too much trust in those who wield power within my temple's hierarchy.",
  "My piety sometimes leads me to blindly trust those that profess faith in my god.",
  "I am inflexible in my thinking.",
  "I am suspicious of strangers and expect the worst of them.",
  "Once I pick a goal, I become obsessed with it to the detriment of everything else in my life.",
  ""
 },
 { // charlatan
  "",
  "I can't resist a pretty face.",
  "I'm always in debt. I spend my ill-gotten gains on decadent luxuries faster than I bring them in...",
  "I'm convinced that no one could ever fool me the way I fool others.",
  "I'm too greedy for my own good. I can't resist taking a risk if there's money involved.",
  "I can't resist swindling people who are more powerful than me.",
  "I hate to admit it and will hate myself for it, but I'll run and preserve my own hide if the going gets tough.",
  ""
 },
 { // criminal
  "",
  "When I see something valuable, I can't think about anything but how to steal it.",
  "When faced with a choice between money and my friends, I usually choose the money.",
  "If there's a plan, I'll forget it. If I don't forget it, I'll ignore it.",
  "I have a “tell” that reveals when I'm lying.",
  "I turn tail and run when things look bad.",
  "An innocent person is in prison for a crime that I committed. I'm okay with that.",
  ""
 },
 { // entertainer
  "",
  "I'll do anything to win fame and renown.",
  "I'm a sucker for a pretty face.",
  "A scandal prevents me from ever going home again. That kind of trouble seems to follow me around.",
  "I once satirized a noble who still wants my head. It was a mistake that I will likely repeat.",
  "I have trouble keeping my true feelings hidden. My sharp tongue lands me in trouble.",
  "Despite my best efforts, I am unreliable to my friends.",
  ""
 },
 { // folk hero
  "",
  "The tyrant who rules my land will stop at nothing to see me killed.",
  "I'm convinced of the significance of my destiny, and blind to my shortcomings and the risk of failure.",
  "The people who knew me when I was young know my shameful secret, so I can never go home again.",
  "I have a weakness for the vices of the city, especially hard drink.",
  "Secretly, I believe that things would be better if I were a tyrant lording over the land.",
  "I have trouble trusting in my allies.",
  ""
 },
 { // gladiator
  "",
  "I'll do anything to win fame and renown.",
  "I'm a sucker for a pretty face.",
  "A scandal prevents me from ever going home again. That kind of trouble seems to follow me around.",
  "I once satirized a noble who still wants my head. It was a mistake that I will likely repeat.",
  "I have trouble keeping my true feelings hidden. My sharp tongue lands me in trouble.",
  "Despite my best efforts, I am unreliable to my friends.",
  ""
 },
 { // trader
  "",
  "I'll do anything to get my hands on something rare or priceless.",
  "I'm quick to assume that someone is trying to cheat me.",
  "No one must ever learn that I once stole money from guild coffers.",
  "I'm never satisfied with what I have-I always want more.",
  "I would kill to acquire a noble title.",
  "I'm horribly jealous of anyone who can outshine my handiwork. Everywhere I go, I'm surrounded by rivals.",
  ""
 },
 { // hermit
  "",
  "Now that I've returned to the world, I enjoy its delights a little too much.",
  "I harbor bloodthirsty thoughts that my isolation and meditation failed to quell.",
  "I am dogmatic in my thoughts and philosophy.",
  "I let my need to win arguments overshadow friendships and harmony.",
  "I'd risk too much to uncover a lost bit of knowledge.",
  "I like keeping secrets and won't share them with anyone.",
  ""
 },
 { // squire
  "",
  "I secretly believe that everyone is beneath me.",
  "I hide a truly scandalous secret that could ruin my family forever.",
  "I too often hear veiled insults and threats in every word addressed to me, and I'm quick to anger.",
  "I have an insatiable desire for decadent pleasures.",
  "In fact, the world does revolve around me.",
  "By my words and actions, I often bring shame to my family.",
  ""
 },
 { // noble
  "",
  "I secretly believe that everyone is beneath me.",
  "I hide a truly scandalous secret that could ruin my family forever.",
  "I too often hear veiled insults and threats in every word addressed to me, and I'm quick to anger.",
  "I have an insatiable desire for decadent pleasures.",
  "In fact, the world does revolve around me.",
  "By my words and actions, I often bring shame to my family.",
  ""
 },
 { // outlander
  "",
  "I am too enamored of ale, wine, and other intoxicants.",
  "There's no room for caution in a life lived to the fullest.",
  "I remember every insult I've received and nurse a silent resentment toward anyone who's ever wronged me.",
  "I am slow to trust members of other races, tribes, and societies.",
  "Violence is my answer to almost any challenge.",
  "Don't expect me to save those who can't save themselves. It is nature's way that the strong thrive and the weak perish.",
  ""
 },
 { // pirate
  "",
  "I follow orders, even if I think they're wrong.",
  "I'll say anything to avoid having to do extra work.",
  "Once someone questions my courage, I never back down no matter how dangerous the situation.",
  "Once I start drinking, it's hard for me to stop.",
  "I can't help but pocket loose coins and other trinkets I come across.",
  "My pride will probably lead to my destruction.",
  ""
 },
 { // sage
  "",
  "I am easily distracted by the promise of information.",
  "Most people scream and run when they see a demon. I stop and take notes on its anatomy.",
  "Unlocking an ancient mystery is worth the price of a civilization.",
  "I overlook obvious solutions in favor of complicated ones.",
  "I speak without really thinking through my words, invariably insulting others.",
  "I can't keep a secret to save my life, or anyone else's.",
  ""
 },
 { // sailor
  "",
  "I follow orders, even if I think they're wrong.",
  "I'll say anything to avoid having to do extra work.",
  "Once someone questions my courage, I never back down no matter how dangerous the situation.",
  "Once I start drinking, it's hard for me to stop.",
  "I can't help but pocket loose coins and other trinkets I come across.",
  "My pride will probably lead to my destruction.",
  ""
 },
 { // soldier
  "",
  "The monstrous enemy we faced in battle still leaves me quivering with fear.",
  "I have little respect for anyone who is not a proven warrior.",
  "I made a terrible mistake in battle that cost many lives-and I would do anything to keep that mistake secret.",
  "My hatred of my enemies is blind and unreasoning.",
  "I obey the law, even if the law causes misery.",
  "I'd rather eat my armor than admit when I'm wrong.",
  ""
 },
 { // urchin
  "",
  "If I'm outnumbered, I will run away from a fight.",
  "Gold seems like a lot of money to me, and I'll do just about anything for more of it.",
  "I will never fully trust anyone other than myself.",
  "I'd rather kill someone in their sleep than fight fair.",
  "It's not stealing if I need it more than someone else.",
  "People who can't take care of themselves get what they deserve.",
  ""
 }
};

void choose_random_roleplay_goal(struct char_data *ch)
{

  int objective = dice(1, 20),
      reason = dice(1, 20),
      complication = dice(1, 20);

  if (!ch) return;

  draw_line(ch, 80, '-', '-');
  text_line(ch, "EXAMPLE GOAL OUTLINE", 80, '-', '-');
  draw_line(ch, 80, '-', '-');

  send_to_char(ch, "%-15s: %s.\r\n", "Objective", character_rp_goal_objectives[objective]);
  send_to_char(ch, "%-15s: %s.\r\n", "Reason", character_rp_goal_reasons[reason]);
  send_to_char(ch, "%-15s: %s.\r\n", "Complication", character_rp_goal_complications[complication]);

  send_to_char(ch, "\r\n");

}

void choose_random_roleplay_personality(struct char_data *ch, int background)
{

  if (background < 1 || background >= NUM_BACKGROUNDS)
  {
    send_to_char(ch, "That is an invalid background.\r\n");
    return;
  }

  int choiceOne = 0, choiceTwo = 0;
  char buf[200];

  if (!ch) return;

  draw_line(ch, 80, '-', '-');
  text_line(ch, "EXAMPLE PERSONALITY QUALITIES", 80, '-', '-');
  draw_line(ch, 80, '-', '-');

  choiceOne = dice(1, 8);
  choiceTwo = dice(1, 8);
  while (choiceTwo == choiceOne)
    choiceTwo = dice(1, 8);
  
  snprintf(buf, sizeof(buf), "%s", personality_traits[background][choiceOne]);
  send_to_char(ch, " %s", strfrmt(buf, 78, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  snprintf(buf, sizeof(buf), "%s", personality_traits[background][choiceTwo]);
  send_to_char(ch, " %s", strfrmt(buf, 78, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "\r\n");

}

void choose_random_roleplay_ideals(struct char_data *ch, int background)
{

  if (background < 1 || background >= NUM_BACKGROUNDS)
  {
    send_to_char(ch, "That is an invalid background.\r\n");
    return;
  }

  int choiceOne = 0, choiceTwo = 0;
  char buf[200];

  if (!ch) return;

  draw_line(ch, 80, '-', '-');
  text_line(ch, "EXAMPLE CHARACTER IDEALS", 80, '-', '-');
  draw_line(ch, 80, '-', '-');

  choiceOne = dice(1, 6);
  choiceTwo = dice(1, 6);
  while (choiceTwo == choiceOne)
    choiceTwo = dice(1, 6);
  
  snprintf(buf, sizeof(buf), "%s", character_ideals[background][choiceOne]);
  send_to_char(ch, " %s", strfrmt(buf, 78, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  snprintf(buf, sizeof(buf), "%s", character_ideals[background][choiceTwo]);
  send_to_char(ch, " %s", strfrmt(buf, 78, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "\r\n");

}

void choose_random_roleplay_bonds(struct char_data *ch, int background)
{

  if (background < 1 || background >= NUM_BACKGROUNDS)
  {
    send_to_char(ch, "That is an invalid background.\r\n");
    return;
  }

  int choiceOne = 0, choiceTwo = 0;
  char buf[200];

  if (!ch) return;

  draw_line(ch, 80, '-', '-');
  text_line(ch, "EXAMPLE CHARACTER BONDS", 80, '-', '-');
  draw_line(ch, 80, '-', '-');

  choiceOne = dice(1, 6);
  choiceTwo = dice(1, 6);
  while (choiceTwo == choiceOne)
    choiceTwo = dice(1, 6);
  
  snprintf(buf, sizeof(buf), "%s", character_bonds[background][choiceOne]);
  send_to_char(ch, " %s", strfrmt(buf, 78, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  snprintf(buf, sizeof(buf), "%s", character_bonds[background][choiceTwo]);
  send_to_char(ch, " %s", strfrmt(buf, 78, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "\r\n");

}

void choose_random_roleplay_flaws(struct char_data *ch, int background)
{

  if (background < 1 || background >= NUM_BACKGROUNDS)
  {
    send_to_char(ch, "That is an invalid background.\r\n");
    return;
  }

  int choiceOne = 0, choiceTwo = 0;
  char buf[200];

  if (!ch) return;

  draw_line(ch, 80, '-', '-');
  text_line(ch, "EXAMPLE CHARACTER FLAWS", 80, '-', '-');
  draw_line(ch, 80, '-', '-');

  choiceOne = dice(1, 6);
  choiceTwo = dice(1, 6);
  while (choiceTwo == choiceOne)
    choiceTwo = dice(1, 6);
  
  snprintf(buf, sizeof(buf), "%s", character_flaws[background][choiceOne]);
  send_to_char(ch, " %s", strfrmt(buf, 78, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  snprintf(buf, sizeof(buf), "%s", character_flaws[background][choiceTwo]);
  send_to_char(ch, " %s", strfrmt(buf, 78, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "\r\n");

}

void show_character_goal_idea_menu(struct char_data *ch)
{
  char buf[MAX_STRING_LENGTH];

  snprintf(buf, sizeof(buf), "%s", GOAL_DESCRIPTION);
  send_to_char(ch, "%s\r\n", strfrmt(buf, 80, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "First, we've created a system to offer random goal ideas.\r\n");
  send_to_char(ch, "1) Generate a random goal idea.\r\n");
  send_to_char(ch, "Q) Proceed to enter in your character goals.\r\n");
  send_to_char(ch, "Enter Your Choice (1|Q): ");
}

void show_character_personality_idea_menu(struct char_data *ch)
{
  char buf[MAX_STRING_LENGTH];
  int i = 0;

  snprintf(buf, sizeof(buf), "%s", PERSONALITY_DESCRIPTION);
  send_to_char(ch, "%s\r\n", strfrmt(buf, 80, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "First, we've created a system to offer random personality ideas. For some random ideas, select a background type.\r\n");
  send_to_char(ch, "You do not need to have chosen the background to select it for personality ideas.\r\n");
  for (i = 1; i < NUM_BACKGROUNDS; i++)
  {
    send_to_char(ch, "%2d) %s\r\n", i, background_list[backgrounds_listed_alphabetically[i]].name);
  }
  send_to_char(ch, "Q) Proceed to enter in your character personality.\r\n");
  send_to_char(ch, "Enter Your Choice (1|Q): ");
}

void show_character_ideals_idea_menu(struct char_data *ch)
{
  char buf[MAX_STRING_LENGTH];
  int i = 0;

  snprintf(buf, sizeof(buf), "%s", IDEALS_DESCRIPTION);
  send_to_char(ch, "%s\r\n", strfrmt(buf, 80, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "First, we've created a system to offer random ideals ideas. For some random ideas, select a background type.\r\n");
  send_to_char(ch, "You do not need to have chosen the background to select it for ideals ideas.\r\n");
  for (i = 1; i < NUM_BACKGROUNDS; i++)
  {
    send_to_char(ch, "%2d) %s\r\n", i, background_list[backgrounds_listed_alphabetically[i]].name);
  }
  send_to_char(ch, "Q) Proceed to enter in your character ideals.\r\n");
  send_to_char(ch, "Enter Your Choice (1|Q): ");
}

void show_character_bonds_idea_menu(struct char_data *ch)
{
  char buf[MAX_STRING_LENGTH];
  int i = 0;

  snprintf(buf, sizeof(buf), "%s", BONDS_DESCRIPTION);
  send_to_char(ch, "%s\r\n", strfrmt(buf, 80, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "First, we've created a system to offer random bonds ideas. For some random ideas, select a background type.\r\n");
  send_to_char(ch, "You do not need to have chosen the background to select it for bonds ideas.\r\n");
  for (i = 1; i < NUM_BACKGROUNDS; i++)
  {
    send_to_char(ch, "%2d) %s\r\n", i, background_list[backgrounds_listed_alphabetically[i]].name);
  }
  send_to_char(ch, "Q) Proceed to enter in your character bonds.\r\n");
  send_to_char(ch, "Enter Your Choice (1|Q): ");
}

void show_character_flaws_idea_menu(struct char_data *ch)
{
  char buf[MAX_STRING_LENGTH];
  int i = 0;

  snprintf(buf, sizeof(buf), "%s", FLAWS_DESCRIPTION);
  send_to_char(ch, "%s\r\n", strfrmt(buf, 80, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "First, we've created a system to offer random flaws ideas. For some random ideas, select a background type.\r\n");
  send_to_char(ch, "You do not need to have chosen the background to select it for flaws ideas.\r\n");
  for (i = 1; i < NUM_BACKGROUNDS; i++)
  {
    send_to_char(ch, "%2d) %s\r\n", i, background_list[backgrounds_listed_alphabetically[i]].name);
  }
  send_to_char(ch, "Q) Proceed to enter in your character flaws.\r\n");
  send_to_char(ch, "Enter Your Choice (1|Q): ");
}

void show_character_goal_edit(struct descriptor_data *d)
{
  if (d->character->player.goals)
  {
    write_to_output(d, "Current Character Goals:\r\n%s", d->character->player.goals);
    d->backstr = strdup(d->character->player.goals);
  }

  write_to_output(d, "Enter your character goals.\r\n");
  send_editor_help(d);
  d->str = &d->character->player.goals;
  d->max_str = PLR_GOALS_LENGTH;
  STATE(d) = CON_CHARACTER_GOALS_ENTER;
}

void show_character_personality_edit(struct descriptor_data *d)
{
  if (d->character->player.personality)
  {
    write_to_output(d, "Current Character Personality:\r\n%s", d->character->player.personality);
    d->backstr = strdup(d->character->player.personality);
  }

  write_to_output(d, "Enter your character personality.\r\n");
  send_editor_help(d);
  d->str = &d->character->player.personality;
  d->max_str = PLR_PERSONALITY_LENGTH;
  STATE(d) = CON_CHARACTER_PERSONALITY_ENTER;
}

void show_character_ideals_edit(struct descriptor_data *d)
{
  if (d->character->player.ideals)
  {
    write_to_output(d, "Current Character Ideals:\r\n%s", d->character->player.ideals);
    d->backstr = strdup(d->character->player.ideals);
  }

  write_to_output(d, "Enter your character ideals.\r\n");
  send_editor_help(d);
  d->str = &d->character->player.ideals;
  d->max_str = PLR_IDEALS_LENGTH;
  STATE(d) = CON_CHARACTER_IDEALS_ENTER;
}

void show_character_bonds_edit(struct descriptor_data *d)
{
  if (d->character->player.bonds)
  {
    write_to_output(d, "Current Character Bonds:\r\n%s", d->character->player.bonds);
    d->backstr = strdup(d->character->player.bonds);
  }

  write_to_output(d, "Enter your character bonds.\r\n");
  send_editor_help(d);
  d->str = &d->character->player.bonds;
  d->max_str = PLR_BONDS_LENGTH;
  STATE(d) = CON_CHARACTER_BONDS_ENTER;
}

void show_character_flaws_edit(struct descriptor_data *d)
{
  if (d->character->player.flaws)
  {
    write_to_output(d, "Current Character Flaws:\r\n%s", d->character->player.flaws);
    d->backstr = strdup(d->character->player.flaws);
  }

  write_to_output(d, "Enter your character flaws.\r\n");
  send_editor_help(d);
  d->str = &d->character->player.flaws;
  d->max_str = PLR_FLAWS_LENGTH;
  STATE(d) = CON_CHARACTER_FLAWS_ENTER;
}

ACMD(do_goals)
{
  char arg[200];

  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs do not have recorded goals on their character file.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Your current goals are:\r\n");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "%s\r\n", ch->player.goals ? ch->player.goals : "Not set yet.");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "To edit your goals, type 'goals edit'. To generate a random goal idea outline, type 'goal example'.\r\n");
    return;
  }
  else if (is_abbrev(arg, "example"))
  {
    send_to_char(ch, "You generate a random goal idea outline: \r\n");
    choose_random_roleplay_goal(ch);
  }
  else if (is_abbrev(arg, "edit"))
  {
    send_to_char(ch, "To edit your character goals, type quit, and at the game menu select option 2, then 4.\r\n");
    return;
  }
  else
  {
    send_to_char(ch, "That is not a valid option\r\n");
    send_to_char(ch, "To edit your goals, type 'goals edit'. To generate a random goal idea outline, type 'goal example'.\r\n");
  }
}

void display_age_menu(struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  int i, j;

  if (!ch) return;

  send_to_char(ch, "Please select the age of your character from this list. Actual numerical ages differ based on race type.\r\n");

  for (i = 0; i < NUM_CHARACTER_AGES; i++)
  {
    send_to_char(ch, "%d) %-15s ", i+1, character_ages[i]);
    for (j = 0; j < 6; j++)
    {
      if (character_age_attributes[i][j] > 0)
      {
        send_to_char(ch, "+%d %s ", character_age_attributes[i][j], ability_score_names[j]);
      }
      else if (character_age_attributes[i][j] < 0)
      {
        send_to_char(ch, "%d %s ", character_age_attributes[i][j], ability_score_names[j]);
      }
    }
    send_to_char(ch, "\r\n");
  }

  send_to_char(ch, "\r\n");
  send_to_char(ch, "Age-based ability score modifiers are applied after setting ability scores in study as part\r\n");
  send_to_char(ch, "of the 1st level study process. If you select your age after level 1, you'll have to respec\r\n");
  send_to_char(ch, "in order to receive the ability score modifiers.\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "Please select your characters age: ");
}

void HandleStateCharacterAgeParseMenuChoice(struct descriptor_data *d, char *arg)
{
    int changeStateTo = STATE(d);
    struct char_data *ch = d->character;
    int age = atoi(arg) - 1;

    switch (age)
    {
      case 0: 
      case 1:
      case 2:
      case 3:
      case 4:
        ch->player_specials->saved.character_age = age; 
        ch->player_specials->saved.character_age_saved = true;
        send_to_char(ch, "You've decided your character will be: %s.\r\n", character_ages[age]);
        changeStateTo = CON_CHAR_RP_MENU;
        show_character_rp_menu(d);
        break;
      default:
        send_to_char(ch, "That is not a valid age. Please select again: ");
        break;
    }
    STATE(d) = changeStateTo;
}

void display_faction_menu(struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  int i;
  clan_rnum c_n;
  char alphabet[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
                     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

  if (!ch) return;

  send_to_char(ch, "\r\n");
  send_to_char(ch, "You character faction, or clan, determines who your allies and enemies are in-character.\r\n"
                    "Faction membership can also provide other benefits, both in terms of game mechanics as\r\n"
                    "with role-play opportunities.\r\n"
                    "\r\n");

  send_to_char(ch, "0) %-25s A) Help on %s\r\n", "Adventurer/No Faction", "Adventurer/No Faction");

  for (i = 1; i <= num_of_clans; i++)
  {
    c_n = real_clan(i);
    send_to_char(ch, "%d) %-25s %c) Help on %s\r\n", i, CLAN_NAME(c_n), alphabet[i], CLAN_NAME(c_n));
  }

  send_to_char(ch, "Enter your faction choice: ");
}

void HandleStateCharacterFactionParseMenuChoice(struct descriptor_data *d, char *arg)
{
    int changeStateTo = STATE(d);
    struct char_data *ch = d->character;
    bool is_clan = isdigit(*arg);
    int clan = atoi(arg);
    clan_rnum c_n;
    int i = 0;
    long v_id;
    char letter;
    char alphabet[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
                     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

    if (is_clan)
    {

      if (clan < 0 || clan > num_of_clans)
      {
        send_to_char(ch, "That is an invalid selection.\r\n");
        return;
      }

      GET_CLAN(ch) = clan;
      if (clan == 0)
      {
        if ((v_id = get_ptable_by_name(GET_NAME(ch))) < 0)
        {
          send_to_char(ch, "There was an error setting your faction as adventurer. Please inform staff ERRCHMNFC001.\r\n");
          return;
        }
        player_table[v_id].clan = 0;
        GET_CLANRANK(ch) = 0;
        send_to_char(ch, "You have elected to be of no specific faction and are known as an adventurer.\r\n");
      }
      else
      {
        GET_CLANRANK(ch) = 6;
        c_n = real_clan(clan);
        set_clan(ch, clan_list[c_n].vnum);
        send_to_char(ch, "You have selected the clan %s and your new rank in that clan is %s.\r\n",
                        CLAN_NAME(c_n), clan_list[c_n].rank_name[5]);
      }
      changeStateTo = CON_CHAR_RP_MENU;
      show_character_rp_menu(d);
    }
    else
    {
      send_to_char(ch, "\tC\r\n");
      switch (*arg)
      {
        case 'a': case 'A': send_to_char(ch, "Adventurers are beholden to no specific faction. They choose their allies and\r\n"
                                             "enemies themselves, and may shift their alliegances accoridng to their personal\r\n"
                                             "agendas and goals.\r\n");
          break;
        default:
          letter = UPPER(*arg);
          for (i = 1; i < 24; i++)
          {
            if (letter == alphabet[i]) break;
          }
          if (i >= 24)
          {
            send_to_char(ch, "That is an invalid selection.\r\n");
            return;
          }
          if (i < 0 || i > num_of_clans)
          {
            send_to_char(ch, "That is an invalid selection.\r\n");
            return;
          }
          c_n = real_clan(i);
          send_to_char(ch, "Clan Description for %s:\r\n\r\n%s\r\n\r\n", CLAN_NAME(c_n), clan_list[c_n].description);
          break;
      }
      send_to_char(ch, "\tn\r\n");
      display_faction_menu(d);
    }

    STATE(d) = changeStateTo;
}

void display_hometown_menu(struct descriptor_data *d)
{

  struct char_data *ch = d->character;

  if (!ch) return;

  send_to_char(ch, "Your character's hometown determines a few things, in addition to the role play aspect.\r\n");
  send_to_char(ch, "It will determine your recall location, donation pit location, as well as tie into various\r\n");
  send_to_char(ch, "other mechanics, such as some background archtype abilities.\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "1) Palanthas : Home of the Knights of Solamnia & Forces of Whitestone\r\n");
  send_to_char(ch, "2) Sanction  : Home of the Dragonarmies\r\n");
  send_to_char(ch, "3) Solace    : Free City with No Direct Alleigances\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "Choose Your Hometown: ");

}

void HandleStateCharacterHometownParseMenuChoice(struct descriptor_data *d, char *arg)
{
    int changeStateTo = STATE(d);
    struct char_data *ch = d->character;
    int hometown = atoi(arg);

    switch (hometown)
    { 
      case 1:
      case 2:
      case 3:
        GET_HOMETOWN(ch) = hometown;
        send_to_char(ch, "You've decided your character's hometown will be: %s.\r\n", cities[hometown]);
        changeStateTo = CON_CHAR_RP_MENU;
        show_character_rp_menu(d);
        break;
      default:
        send_to_char(ch, "That is not a valid hometown. Please select again: ");
        break;
    }
    STATE(d) = changeStateTo;
}

void display_deity_menu(struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  int i;
  char dname[50];

  if (!ch) return;

  send_to_char(ch, "   %-25s - %-15s %s\r\n", "Deities of Krynn", "Alignment", "Portfolio");
  for (i = 0; i < 80; i++)
    send_to_char(ch, "-");
  send_to_char(ch, "\r\n");

  for (i = 0; i < NUM_DEITIES; i++)
  {
    if (deity_list[i].pantheon == DEITY_PANTHEON_NONE)
      continue;
    
    snprintf(dname, sizeof(dname), "%s", deity_list[i].name);
    send_to_char(ch, "%2d) %-25s - %-15s - %s\r\n", i, CAP(dname), GET_ALIGN_STRING(deity_list[i].ethos, deity_list[i].alignment), deity_list[i].portfolio);
  }

  send_to_char(ch, "\r\n");
  send_to_char(ch, "Select Your Deity: ");

}

void display_deity_info(struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  char dname[50];

  if (!ch) return;

  snprintf(dname, sizeof(dname), "%s", deity_list[GET_DEITY(ch)].name);
  send_to_char(ch, "\tA%s\r\n\tn", CAP(dname));
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tAPantheon:\tn %s\r\n", pantheons[deity_list[GET_DEITY(ch)].pantheon]);
  send_to_char(ch, "\tAAlignment:\tn %s\r\n", GET_ALIGN_STRING(deity_list[GET_DEITY(ch)].ethos, deity_list[GET_DEITY(ch)].alignment));
  send_to_char(ch, "\tAPortfolio:\tn %s\r\n", deity_list[GET_DEITY(ch)].portfolio);
  send_to_char(ch, "\tADescription:\tn\r\n%s\r\n", deity_list[GET_DEITY(ch)].description);

  send_to_char(ch, "\r\n");
  send_to_char(ch, "Do you wish to select this deity? (Y|N) ");
}

void HandleStateCharacterDeityParseMenuChoice(struct descriptor_data *d, char *arg)
{
  int changeStateTo = STATE(d);
  struct char_data *ch = d->character;
  int deity = atoi(arg);


  if (deity >= 0 && deity < NUM_DEITIES)
  {
    GET_DEITY(ch) = deity;
    display_deity_info(d);
    changeStateTo = CON_CHARACTER_DEITY_CONFIRM;
  }
  else
  {
    send_to_char(ch, "That is not a valid deity. Please select again: ");
  }
  
  STATE(d) = changeStateTo;
}
void HandleStateCharacterDeityConfirmParseMenuChoice(struct descriptor_data *d, char *arg)
{
  int changeStateTo = STATE(d);
  struct char_data *ch = d->character;

  switch (*arg)
  { 
    case 'Y':
    case 'y':
      send_to_char(ch, "You've decided your character's deity will be: %s.\r\n", deity_list[GET_DEITY(ch)].name);
      changeStateTo = CON_CHAR_RP_MENU;
      show_character_rp_menu(d);
      break;
    case 'N':
    case 'n':
      GET_DEITY(ch) = 0;
      display_deity_menu(d);
      changeStateTo = CON_CHARACTER_DEITY_SELECT;
      break;
    default:
      send_to_char(ch, "That is not a valid selection. Please select again: ");
      break;
  }
  
  STATE(d) = changeStateTo;
}

// shows:
// name, race, gender, age, title, deity, alignment, short description, description, background story, background archtype, 
// goals, personality, ideals, bonds, flaws, homeland, faction/clan, home city, deity
ACMD(do_rpsheet)
{

  char arg[100], buf[100], buf2[100];
  struct char_data *t;
  int i;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg || GET_LEVEL(ch) < LVL_IMMORT)
  {
    t = ch;
  }
  else if (!(t = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "No one by that name currently logged in.\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), " RP SHEET FOR %s ", GET_NAME(t));

  for (i = 0; i < strlen(buf); i++)
    buf[i] = toupper(buf[i]);

  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tC");
  text_line(ch, buf, 100, '-', '-');
  send_to_char(ch, "\tC");

  // Name, Race and Gender
  snprintf(buf, sizeof(buf), "%s", race_list[GET_RACE(t)].name);
  snprintf(buf2, sizeof(buf2), "%s", genders[GET_SEX(t)]);
  send_to_char(ch, "\tcName      :\tn %-20s \tcRace       :\tn %-20s \tcGender :\tn %s\r\n", GET_NAME(t), CAP(buf), CAP(buf2));
  
  // Title
  send_to_char(ch, "\tcTitle     :\tn %s\r\n", GET_TITLE(t));

  // Short Description
  send_to_char(ch, "\tcShort Desc:\tn %s\r\n", current_short_desc(t));

  // Age, Alignment, Deity
  snprintf(buf, sizeof(buf), "%s", get_align_by_num(GET_ALIGNMENT(t)));
  strip_colors(buf);
  snprintf(buf2, sizeof(buf2), "%s", character_ages[GET_CH_AGE(t)]);
  send_to_char(ch, "\tcAge       :\tn %-20s \tcAlignment  :\tn %-20s \tcDeity  :\tn %-15s\r\n", 
               CAP(buf2), buf, deity_list[GET_DEITY(t)].name);

  snprintf(buf, sizeof(buf), "%s", background_list[GET_BACKGROUND(t)].name);
  send_to_char(ch, "\tcBackground:\tn %-20s \tcFaction    :\tn %s\r\n", 
               CAP(buf), GET_CLAN(t) ? clan_list[GET_CLAN(t)-1].clan_name : "None");

  // Homeland, Home City
  send_to_char(ch, "\tcHomeland  :\tn %-20s \tcHometown   :\tn %-20s\r\n", 
               regions[GET_REGION(t)], cities[GET_HOMETOWN(t)]);

  // Description
  send_to_char(ch, "\tcDesc      :\tn %-3s\r\n", t->player.description ? ((strlen(t->player.description) > 10) ? t->player.description : "Not Set") : "Not Set");

  // Background Story, Personality, Goals
  send_to_char(ch, "\tcBG Story  :\tn %-20s \tcPersonality:\tn %-20s \tcGoals  :\tn %-3s\r\n", 
               t->player.background ? ((strlen(t->player.background) > 10) ? "Set" : "Not Set") : "Not Set",
               t->player.personality ? ((strlen(t->player.personality) > 10) ? "Set" : "Not Set") : "Not Set",
               t->player.goals ? ((strlen(t->player.goals) > 10) ? "Set" : "Not Set") : "Not Set");

  // Ideals, Bonds, Flaws
  send_to_char(ch, "\tcIdeals    :\tn %-20s \tcBonds      :\tn %-20s \tcFlaws  :\tn %-3s\r\n",
               t->player.ideals ? ((strlen(t->player.ideals) > 10) ? "Set" : "Not Set") : "Not Set",
               t->player.bonds ? ((strlen(t->player.bonds) > 10) ? "Set" : "Not Set") : "Not Set",
               t->player.flaws ? ((strlen(t->player.flaws) > 10) ? "Set" : "Not Set") : "Not Set");
  
  send_to_char(ch, "\tC");
  draw_line(ch, 100, '-', '-');
  send_to_char(ch, "\tn");
  send_to_char(ch, "To see extra info, type: (rpdescription|rpbackground|rppersonality|rpgoals|rpideals|rpbonds|rpflaws)\r\n");
  send_to_char(ch, "To toggle your role-play flag on and off type: rpset\r\n");
  send_to_char(ch, "\tC");
  draw_line(ch, 100, '-', '-');
  send_to_char(ch, "\tn");
  send_to_char(ch, "\r\n");
}

ACMD(do_showrpinfo)
{

  char arg[100], buf[100], buf2[200];
  char *text_out = NULL;
  struct char_data *t;
  int i;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg || GET_LEVEL(ch) < LVL_IMMORT)
  {
    t = ch;
  }
  else if (!(t = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "No one by that name currently logged in.\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "%s", GET_NAME(t));

  for (i = 0; i < strlen(buf); i++)
    buf[i] = toupper(buf[i]);

  switch (subcmd)
  {
    case SCMD_RP_DESC: snprintf(buf2, sizeof(buf2), " DESCRIPTION FOR %s ", buf); text_out = t->player.description; break;
    case SCMD_RP_PERSONALITY: snprintf(buf2, sizeof(buf2), " PERSONALITY FOR %s ", buf); text_out = t->player.personality; break;
    case SCMD_RP_GOALS: snprintf(buf2, sizeof(buf2), " GOALS FOR %s ", buf); text_out = t->player.goals; break;
    case SCMD_RP_IDEALS: snprintf(buf2, sizeof(buf2), " IDEALS FOR %s ", buf); text_out = t->player.ideals; break;
    case SCMD_RP_BONDS: snprintf(buf2, sizeof(buf2), " BONDS FOR %s ", buf); text_out = t->player.bonds; break;
    case SCMD_RP_FLAWS: snprintf(buf2, sizeof(buf2), " FLAWS FOR %s ", buf); text_out = t->player.flaws; break;
    case SCMD_RP_BG_STORY: snprintf(buf2, sizeof(buf2), " BACKGROUND FOR %s ", buf); text_out = t->player.background; break;
    default:
      send_to_char(ch, "That rp info doesn't exist. Please inform a staff member ERRRPSHOWINFO001.\r\n");
      return;
  }

  send_to_char(ch, "\tC\r\n");
  text_line(ch, buf2, 100, '-', '-');
  send_to_char(ch, "\tn");
  send_to_char(ch, "%s", text_out == NULL ? "Not Set" : text_out);
  send_to_char(ch, "\tC\r\n");
  draw_line(ch, 100, '-', '-');
  send_to_char(ch, "\tn\r\n");
  
}

void display_rp_decide_menu(struct descriptor_data *d)
{
  struct char_data *ch = d->character;

  if (!ch) return;

  send_to_char(ch, "\r\n"

                   "\tCChronicles of Krynn is a Role Play Focused MUD. \tnWhat that means is that the staff's focus is\r\n"
                   "on creating overarching role-play themes and stories, and assisting players with their individual\r\n"
                   "character stories. \tcHowever role-play is not mandatory\tn, and we only ask that non-role-players respect\r\n"
                   "the role play of others in their vicinity and not disrupt it.\r\n"
                   "\r\n"
                   "At this moment you have three choices:\r\n"
                   "\r\n"
                   "\tc1)\tn Flag yourself a non-role-player and enter the game.\r\n"
                   "\tc2)\tn Flag yourself a role-player and enter in additonal character info.\r\n"
                   "\tc3)\tn Delay this decision and enter the game.\r\n"
                   "\r\n"
                   "This extra character info can be entered at any time, and you can enter as little or as much as\r\n"
                   "you like. You can also flag yourself a role-player or non-role-player at any time in-game.\r\n"
                   "The extra character info includes: short and long descriptions, background archtype, background story, deity, \r\n"
                   "homeland, hometown, faction, character age, personality, goals, ideals, bonds and flaws.\r\n"
                   "\r\n"
                   "\tcPlease Enter Your Choice: \tn"
                   );

}

void HandleStateCharacterRPDecideParseMenuChoice(struct descriptor_data *d, char *arg)
{
  int changeStateTo = STATE(d);
  struct char_data *ch = d->character;

  switch (*arg)
  { 
    case '1':
      send_to_char(ch, "You've elected to be a non-role-player and can now enter the game.\r\n");
      SET_BIT_AR(PRF_FLAGS(ch), PRF_NON_ROLEPLAYER);
      write_to_output(d, "\r\n");
      write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
      changeStateTo = CON_RMOTD;
      break;
    case '2':
      send_to_char(ch, "You've elected to be a role-player and can now enter in your additional character info.\r\n");
      SET_BIT_AR(PRF_FLAGS(ch), PRF_RP);
      changeStateTo = CON_CHAR_RP_MENU;
      show_character_rp_menu(d);
      break;
    case '3':
      send_to_char(ch, "You've decided to delay your decision about role playing, and can now enter the game.\r\n");
      write_to_output(d, "\r\n");
      write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
      changeStateTo = CON_RMOTD;
      break;
    default:
      send_to_char(ch, "That is not a valid selection. Please select again: ");
      break;
  }
  
  /* Save character after roleplay decision */
  if (ch && changeStateTo != STATE(d)) {
    save_char(ch, 0);
  }
  
  STATE(d) = changeStateTo;
}