#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"
#include "spells.h"
#include "backgrounds.h"
#include "character_creation_content.h"

#include <string.h>

#if !defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)

static const char content_provenance[] =
    "Approved Luminari canon snapshot; see The Thirteen Homelands, The Heart Tongues of the "
    "Homelands, and The Character's Compass.";

static const struct character_creation_homeland homelands[NUM_REGIONS] = {
    [REGION_ASHENPORT] =
        {REGION_ASHENPORT, "homeland/ashenport", "Ashenport", "Charter city", "Onduis",
         "Free Cities of Kohn", LANG_ASHEN_CANT,
         "The Great Port where every road becomes a bargain and every bargain becomes a story.",
         "Ashenport stands where the River Veyr loosens its fist and gives itself to the Shallow "
         "Sea. "
         "Its roofs are red tile, green copper, patched sailcloth, and the occasional inverted "
         "hull of "
         "a ship that found a second career as a tavern. Seven old quays divide the harbor, though "
         "any "
         "dockhand will swear there are nine and accuse the tax office of losing the other "
         "two.\r\n\r\nThe city calls itself the Phoenix Crown because its founding marks the "
         "modern "
         "human calendar: Year One of the New Calendar began when refugees kindled a signal fire "
         "in "
         "the ash of the Last War and ships answered from three horizons. That fire has never been "
         "allowed to die. It burns today in the Harbor House, watched by a noble, a commoner, and "
         "a "
         "child chosen by lot.\r\n\r\nAshenport is governed twice. A mayor and appointed judges "
         "keep "
         "the streets, while the Republic of Nine bargains for the waters and lands beyond the "
         "walls. "
         "Four seats belong to old houses, five to elected wards, and the Field Marshal breaks a "
         "tie "
         "only after publicly naming the cost of doing so. It is a system built to irritate "
         "everyone "
         "equally, which Ashenporters consider the nearest politics comes to justice.\r\n\r\nHere "
         "the "
         "Swiftpath hums beneath gull cries. Here the Jade Jug keeps lamps for travelers who have "
         "nowhere else to be. Here one may buy a crystal memory, a forged genealogy, an honest "
         "meal, "
         "or a dishonest map--sometimes from the same stall.\r\n\r\nAshenport teaches that "
         "strangers "
         "are unfinished opportunities, that a promise needs witnesses, and that civilization is "
         "not "
         "clean. It is the daily labor of keeping ten thousand hungers from becoming one "
         "riot.\r\n\r\nChoose Ashenport for a character shaped by crowds, commerce, rumor, civic "
         "pride, quick friendship, and quicker suspicion.",
         content_provenance},
    [REGION_SANCTUS] =
        {REGION_SANCTUS, "homeland/sanctus", "Sanctus", "Charter city and diaspora", "Axtros",
         "Free Cities of Kohn", LANG_SANCTINE,
         "The Silent City whose bells no longer ring--and whose scattered people refuse to forget "
         "their sound.",
         "Sanctus was built in concentric circles around a bell that had no clapper. According to "
         "its "
         "oldest charter, a city devoted to free thought should never be summoned by force. The "
         "bell "
         "would ring only when every citizen wished it. It never rang, but generations learned to "
         "hear "
         "the promise inside its silence.\r\n\r\nBefore the Incident, Sanctus served as High "
         "Mediator "
         "among the Free Cities of Kohn. Its courts were famous for the Hour of Listening, during "
         "which advocates had to restate an opponent's case so faithfully that the opponent "
         "accepted "
         "the summary. Its crafters worked in open arcades. Its player-owned stalls, shrine "
         "services, "
         "and eastern goods made the city a destination rather than merely a capital.\r\n\r\nThree "
         "months ago, Sanctus proper went quiet. No lawful road enters the inner rings. Birds turn "
         "aside. Divinations return the face of the questioner. Voices occasionally speak from "
         "behind "
         "the pale boundary with perfect courtesy and no breath between words.\r\n\r\nThe "
         "contradiction seen on common maps is real but not impossible: the Pilgrim's Ring lies "
         "outside the sealed city. This outer harbor and service ward remains open under Axtrosi "
         "quarantine. Ships dock, caravaners trade, craftspeople work, and former Sanctine "
         "officials "
         "stamp documents beneath the shadow of gates they cannot cross. Travelers who say they "
         "\"went "
         "to Sanctus\" usually mean the Ring. Those born inside the Silence, those raised in the "
         "Ring, "
         "and the refugees now scattered across Lumia all remain Sanctine.\r\n\r\nSanctus teaches "
         "the "
         "difference between silence and consent. Its people listen closely, choose words "
         "carefully, "
         "and distrust any harmony that requires every voice to become the same "
         "voice.\r\n\r\nChoose "
         "Sanctus for a mediator, refugee, artisan, investigator, survivor, or anyone carrying a "
         "home "
         "that can be remembered but not reached.",
         content_provenance},
    [REGION_ONDUIS] =
        {REGION_ONDUIS, "homeland/onduis", "Onduis", "Littoral region", "", "Free Cities of Kohn",
         LANG_ONDUIC,
         "A many-roaded coast of green lowlands, old ruins, and towns that survive by helping one "
         "another.",
         "Onduis curves around the middle Shallow Sea like a hand held beneath a falling cup. "
         "Rivers, "
         "roads, and old war routes all gather there. Ashenport crowns its central estuary, but "
         "the "
         "region is larger than its famous city: Mosswood Village keeps the northern green; Graven "
         "Hollow watches a wounded valley; farms lean against old forts; and harmless-looking "
         "hills "
         "conceal places that remember the Last War too well.\r\n\r\nNo single ruler owns Onduis. "
         "The "
         "Republic of Nine claims the roads nearest Ashenport, village councils hold the interior, "
         "and "
         "the Free Cities maintain harbor law along the coast. When these authorities disagree, "
         "which "
         "is often, the people rely on the Lantern Compact: any settlement that lights three blue "
         "lamps may ask food, shelter, or defense from its neighbors until the danger passes. The "
         "Compact has no army and has outlived four armies.\r\n\r\nOnduic identity is practical "
         "rather "
         "than grand. People mend what they have, mark safe wells, and leave chalk signs where "
         "monsters have moved. They are accustomed to heroes passing through on important business "
         "and "
         "unimpressed by importance that cannot stack firewood.\r\n\r\nOnduis teaches that roads "
         "are "
         "promises between strangers. A character from here may be a village guide, caravan guard, "
         "hedge scholar, ruin scavenger, militia runner, or ordinary person who learned early that "
         "ordinary people keep the world alive.",
         content_provenance},
    [REGION_SELERISH] =
        {REGION_SELERISH, "homeland/selerish", "Selerish", "Coastal scholarly province", "",
         "Magocracy of Chulan", LANG_SELERIC,
         "A rain-bright coast where arguments are archived, storms are named, and magic must "
         "survive "
         "peer review.",
         "Selerish occupies a long southeastern shelf where warm sea air strikes black cliffs and "
         "becomes rain. Its harbors smell of wet slate, ink, and citrus peel. The province entered "
         "Chulan's sphere not by conquest but by examination: its three coastal colleges "
         "challenged "
         "the Council of Nine to prove that rule by magic was better than rule by evidence. Chulan "
         "answered with a decade of debates. Nobody agrees who won, so both sides claim the "
         "result.\r\n\r\nEvery Selerish town keeps a Book of Disagreements in its public hall. "
         "Citizens may record a claim, a counterclaim, and the evidence that would change their "
         "mind. "
         "Children learn to sign their first page before they learn formal spellcraft. The "
         "practice "
         "has made Selerish excellent at navigation, weather prediction, and finding polite ways "
         "to "
         "call an archmage wrong.\r\n\r\nCorm Orp is the best-known port, though Selerish people "
         "insist it is neither the oldest nor the most beautiful. Inland, rain terraces carry "
         "orchards "
         "and herbs used by healers across Lumia. Along the cliff road stand abandoned "
         "observatories "
         "whose brass roofs still turn toward stars no current chart contains.\r\n\r\nSelerish "
         "teaches "
         "that changing one's mind is a discipline, not a defeat. Choose it for a skeptic, "
         "weather-worker, healer, navigator, apprentice mage, archivist, or curious soul who would "
         "rather ask a dangerous question than inherit a comfortable lie.",
         content_provenance},
    [REGION_CARSTAN] =
        {REGION_CARSTAN, "homeland/carstan", "Carstan", "Glass-coast province", "",
         "Magocracy of Chulan", LANG_CARSTANI,
         "A sun-struck coast of glassworks, hard bargains, and towers that remember lightning.",
         "Carstan is where Lumia's eastern stone meets a sea so bright that sailors wrap dark "
         "cloth "
         "around their eyes at noon. Long ago, a storm of magical fire melted whole beaches into "
         "green-black glass. Carstani builders learned to cut those sheets without waking the "
         "sparks "
         "trapped inside. Their windows hold dawn for hours. Their lenses can find flaws in gems, "
         "armor, and occasionally arguments.\r\n\r\nThe Glass Tower rises above the inner coast, "
         "part "
         "observatory, part lightning archive. Every storm that strikes its crown leaves a "
         "branching "
         "white memory in the walls. Chulan's scholars study the marks as records of the Loom "
         "under "
         "stress. The fishing and smuggling town of Hardbuckler distrusts this interpretation and "
         "maintains that lightning simply has terrible handwriting.\r\n\r\nCarstan joined Chulan "
         "after "
         "bargaining for the Right of Refusal: no provincial household may be compelled to "
         "participate "
         "in an experimental spell. This right is fiercely guarded, frequently litigated, and "
         "sometimes sold for an alarming sum.\r\n\r\nCarstan teaches that beauty can be dangerous "
         "long "
         "after the fire is gone. Choose it for a glassworker, storm-reader, smuggler, advocate, "
         "artificer, fisher, or anyone who has learned to look through a thing without mistaking "
         "clarity for truth.",
         content_provenance},
    [REGION_AXTROS] =
        {REGION_AXTROS, "homeland/axtros", "Axtros", "Eastern maritime march", "",
         "Free Cities of Kohn", LANG_AXTROSI,
         "An eastern road-and-sea march that keeps trading while the Silent City watches from "
         "behind "
         "pale walls.",
         "Axtros stretches along Lumia's eastern sea-lanes, a country of dry headlands, red grass, "
         "salt vineyards, and roads built broad enough for two caravans to pass without either "
         "admitting fear. Sanctus stands within it but never ruled it. The region is a braid of "
         "independent port councils, caravan families, and the old March Wardens, who maintain "
         "wells "
         "and warning towers in exchange for hospitality rather than taxes.\r\n\r\nSince the "
         "Silence, "
         "Axtros has become the hinge on which half the eastern world turns. The Pilgrim's Ring "
         "receives Sanctine refugees and contains the trade that once crossed Sanctus proper. "
         "Quarantine lanterns burn violet along the roads. Every inn keeps a mirror by the "
         "door--not "
         "for vanity, but because people returning from the Silent boundary sometimes forget to "
         "cast "
         "reflections in the same direction as everyone else.\r\n\r\nAxtrosi culture prizes motion "
         "with memory. Caravan families carry household shrines on wagons. Vintners bury one "
         "bottle "
         "from every harvest beside a road marker, \"so the land may taste what it gave.\" "
         "Children "
         "learn the routes by song, and a missed verse can put a traveler a hundred miles "
         "wrong.\r\n\r\nAxtros teaches hospitality with boundaries. Choose it for a caravaner, "
         "scout, "
         "vintner, border guard, refugee worker, road-priest, or someone who knows that an open "
         "door "
         "and an unwatched door are not the same thing.",
         content_provenance},
    [REGION_HIR] =
        {REGION_HIR, "homeland/hir", "Hir", "Inland river basin", "", "Empire of New Anteria",
         LANG_HIRI,
         "A broad river basin where the living bargain with inherited duty and every old road "
         "seems to "
         "remember a war.",
         "Hir is a basin of long rivers and low brown hills west of Onduis. Pesh, its oldest "
         "market "
         "city, sits where three stone roads meet a ford that no flood has managed to move. This "
         "is "
         "why travelers say Hir/Pesh when they mean the region's commercial heart, much as sailors "
         "may "
         "use a harbor's name for the whole coast.\r\n\r\nThe Empire of New Anteria claims Hir "
         "through "
         "inheritance tablets recovered after the fall of Old Anteria. Hir accepts the claim with "
         "qualifications, footnotes, and several armed tollhouses. Its villages send grain and "
         "recruits to the Empire; in return, New Anteria's Registered Dead maintain ancient "
         "causeways "
         "and remember where plague pits must never be opened.\r\n\r\nHiri households keep a "
         "Second "
         "Chair at important meals. It may honor an ancestor, an absent traveler, or the person "
         "one "
         "has not forgiven yet. No one sits there. To remove it is to declare that the past has no "
         "claim on the present, a statement considered either monstrous or brave.\r\n\r\nGrunwald "
         "guards the western road, while Pesh's traders carry news between Onduis, the Ubdinas, "
         "and "
         "the Anterean interior. Old quest records place Darkling agents, half-orc camps, and the "
         "earliest resistance routes across this basin. The locals do not call that history. They "
         "call "
         "it directions.\r\n\r\nHir teaches that inheritance can be shelter, debt, or both. Choose "
         "it "
         "for a road warden, farmer, ancestor-keeper, dissident, half-orc veteran, merchant, or "
         "someone deciding which obligations deserve to survive them.",
         content_provenance},
    [REGION_QUECHIAN] =
        {REGION_QUECHIAN, "homeland/quechian", "Quechian", "Twilight-forest province", "",
         "Mosswood Federation", LANG_QUECHIAN,
         "A deep western forest where paths are agreements and no tree is assumed to be merely "
         "scenery.",
         "Quechian lies beneath a canopy so old that noon arrives green and quiet. Its settlements "
         "occupy clearings negotiated with the forest rather than cut from it. A house may stand "
         "for "
         "thirty years, then be dismantled because the roots beneath it have asked for darkness. "
         "The "
         "request is delivered by druids, wood-elves, awakened birds, or mushrooms whose legal "
         "testimony remains controversial everywhere except Mosswood.\r\n\r\nThe province joined "
         "the "
         "Mosswood Federation by the Covenant of Shade. In return for a voice in the Federation, "
         "Quechian protects the old twilight pools, the Reaching Woods, and the enormous darkwood "
         "trees whose roots touch places not entirely inside the waking world.\r\n\r\nQuechian "
         "culture "
         "distinguishes ownership from stewardship. One may own an axe, but never the tree it "
         "cuts; "
         "own a bow, but never the path of the arrow; own a memory, but not another person's "
         "telling "
         "of it. Visitors find this poetic until the first property dispute is adjudicated by a "
         "jury "
         "of squirrels.\r\n\r\nQuechian teaches attention to lives that cannot speak in familiar "
         "ways. "
         "Choose it for a ranger, druid, herbalist, dreamer, patient hunter, seasonal diplomat, or "
         "anyone who suspects every landscape has an opinion.",
         content_provenance},
    [REGION_VAILAND] =
        {REGION_VAILAND, "homeland/vailand", "Vailand", "Lake-and-heath march", "",
         "Mosswood Federation", LANG_VAILIC,
         "A western country of windy heaths and wandering lakes, where communities follow water "
         "rather "
         "than walls.",
         "Vailand begins where Quechian's trees loosen into heather and high grass. Its four great "
         "lakes do not always remain in the same basins. During certain moon phases, one will "
         "drain "
         "without mud or flood and rise days later many miles away. Villages travel after them on "
         "broad wooden runners, leaving stone hearths for whoever inherits the empty "
         "shore.\r\n\r\nThe "
         "Mosswood Federation recognizes Vailand's lakes as voting citizens. Their voices are "
         "interpreted by mere-listeners, people trained to read shoreline changes, fish "
         "migrations, "
         "and the dreams of those sleeping nearest the water. Critics say this gives enormous "
         "power to "
         "a priestly class. Vailanders reply that the critics have never heard a lake say "
         "no.\r\n\r\nThe region's south road passes old towers and shadowed valleys; its northern "
         "waters reach stranger ruins. Vailanders therefore value portable traditions. Their law "
         "is "
         "sung, their family histories are woven into tent bands, and their dead are remembered "
         "with "
         "cups of water poured onto whatever ground the household presently calls "
         "home.\r\n\r\nVailand "
         "teaches that leaving a place need not mean abandoning it. Choose it for a wanderer, "
         "fisher, "
         "horse-herder, lake mystic, scout, portable artisan, or someone who believes belonging "
         "can "
         "move without becoming shallow.",
         content_provenance},
    [REGION_OORPII] =
        {REGION_OORPII, "homeland/oorpii", "Oorpii", "Northern island league", "",
         "Free Cities of Kohn", LANG_OORPIC,
         "A cold island league of rope bridges, elected harbor-mothers, and ships built to survive "
         "the "
         "sea changing its mind.",
         "Oorpii is not one island but a chain of basalt backs rising from the northern sea. Rope "
         "bridges join the nearest cliffs; boats join everything else. In winter, spray freezes "
         "sideways and whole villages glitter like chandeliers until sunrise.\r\n\r\nEach island "
         "governs itself, but the league chooses a Harbor-Mother whenever storm season begins. The "
         "title is not hereditary and need not belong to a woman. It goes to the person trusted to "
         "decide which ships may sail, which must stay, and which stranded strangers will be fed "
         "first. The Harbor-Mother's word is absolute until the first calm week, after which the "
         "office dissolves and its holder is expected to return every borrowed "
         "privilege.\r\n\r\nOorpii joined the Free Cities through a charter written on sailcloth "
         "so no "
         "capital could lock it in a vault. Its pilots are prized wherever reefs, Swiftpath tides, "
         "or "
         "political tempers make a straight route dangerous. Its storykeepers carve records into "
         "whalebone replicas rather than the bones of whales; Oorpii law forbids claiming another "
         "creature's death as one's own memory.\r\n\r\nOorpii teaches that authority is a tool for "
         "a "
         "storm, not a chair for a lifetime. Choose it for a pilot, fisher, bridge-runner, storm "
         "priest, shipwright, temporary leader, or someone who knows when survival requires "
         "obedience "
         "and when survival requires taking the keys back.",
         content_provenance},
    [REGION_KELLUST] =
        {REGION_KELLUST, "homeland/kellust", "Kellust", "Highland reach", "", "Crystal Reaches",
         LANG_TAL,
         "A high country where stone sings beneath the snow and every bridge is tuned before it "
         "bears "
         "weight.",
         "Kellust occupies the northern heights above the Crystal Reaches. Its valleys belong to "
         "surface farmers, dwarven halls, monastery mines, and Crystal Dwarf listening posts built "
         "where Arcanite veins hum close to the air. Mithril Hall is its best-known gate, but the "
         "Reach extends through seven passes and more than seven hundred named "
         "echoes.\r\n\r\nPeople "
         "in Kellust test a structure by song. A mason strikes a bridge and listens for fear. A "
         "miner "
         "hums into a wall and waits for the mountain's refusal. A family settling an argument may "
         "ask "
         "each person to sustain a note until the discord resolves--not because agreement is "
         "always "
         "possible, but because hidden strain should be heard before it becomes "
         "fracture.\r\n\r\nSurface Kellustans and Crystal Dwarves do not pretend to be one people. "
         "They share roads, trade, and the Heart Tongue Tal, while disagreeing over mining, "
         "memory, "
         "caste, and who is entitled to call a mountain an ancestor. The Arcanite crisis has "
         "sharpened "
         "every disagreement. Still, when an avalanche falls, all voices join the same rescue "
         "chord.\r\n\r\nKellust teaches that strength is not silence; sound reveals where strength "
         "fails. Choose it for a miner, mason, resonator, mountain guide, archivist, craftsperson, "
         "or "
         "someone caught between inherited communities.",
         content_provenance},
    [REGION_EAST_UBDINA] =
        {REGION_EAST_UBDINA, "homeland/east-ubdina", "East Ubdina", "River-and-forest province",
         "Ubdina", "Empire of New Anteria", LANG_UBDINIC,
         "The greener half of a divided southern land, where forest roads and ancestor bridges "
         "carry "
         "both trade and old resentment.",
         "Ubdina was one province before the Ash Years broke its central river into two channels "
         "and "
         "its surviving councils into two certainties. East Ubdina took the forests, broad water, "
         "and "
         "the old imperial road toward New Anteria. West Ubdina took the frost basins, marsh "
         "coast, "
         "and most of the province's grief.\r\n\r\nEast Ubdina is a land of red-barked forests and "
         "deep rivers crossed by ancestor bridges. The Empire's Registered Dead tend these "
         "bridges, "
         "not as slaves but as officeholders whose terms continue until a named repair is "
         "finished. "
         "Some have served for a century because their descendants keep finding new "
         "cracks.\r\n\r\nVillages hold two-name markets. Sellers display a public price and a "
         "remembered price: what the item cost before the division. No one must honor the old "
         "price, "
         "but refusing to show it is considered a claim that history does not matter. The custom "
         "causes excellent arguments and terrible accounting.\r\n\r\nEast Ubdina teaches the uses "
         "and "
         "dangers of continuity. Choose it for a forester, river trader, bridge keeper, imperial "
         "clerk, skeptic of empire, explorer, or someone trying to repair an inheritance without "
         "becoming trapped inside it.",
         content_provenance},
    [REGION_WEST_UBDINA] =
        {REGION_WEST_UBDINA, "homeland/west-ubdina", "West Ubdina", "Frost-and-marsh province",
         "Ubdina", "Empire of New Anteria", LANG_UBDINIC,
         "The colder half of Ubdina, where marsh lights, frost keeps, and stubborn villages "
         "outlast "
         "every map drawn over them.",
         "West Ubdina lies below long winter skies. Its northwestern heights carry the Frozen "
         "Castle; "
         "its coast descends through reed marsh and black water toward old keeps, grave roads, and "
         "villages built on pilings. Winter comes early, fog comes whenever it pleases, and the "
         "lights "
         "walking over the marsh are not always lanterns.\r\n\r\nAfter Ubdina divided, the west "
         "refused to move its provincial dead to New Anteria's registries. Instead, communities "
         "maintain local Hearth Rolls: lists of the living, the dead, the missing, and those whose "
         "state is presently under dispute. New Anterian law recognizes three categories. West "
         "Ubdina "
         "recognizes that the world is rarely so tidy.\r\n\r\nThe province is famous for thaw "
         "feasts. "
         "When the first river ice breaks, neighbors carry preserved food into the road and feed "
         "whoever arrives, including rivals. A feud may resume at sunset, but no hunger is allowed "
         "to "
         "cross the thaw. West Ubdinans say civilization is proven by what it postpones for "
         "mercy.\r\n\r\nWest Ubdina teaches endurance without romance. Choose it for a marsh "
         "guide, "
         "grave tender, hunter, keep survivor, herbalist, local patriot, or anyone who knows that "
         "stubbornness can be both shield and prison.",
         content_provenance},
};

static const struct character_creation_language heart_tongues[NUM_LANGUAGES] = {
    [LANG_ASHEN_CANT] = {LANG_ASHEN_CANT, "language/ashen-cant", "Ashen Cant",
                         "The fast, consequence-first harbor tongue of Ashenport's docks, markets, "
                         "and Republic wards."},
    [LANG_SANCTINE] =
        {LANG_SANCTINE, "language/sanctine", "Sanctine",
         "The deliberate language of Sanctus, where pauses carry consent, doubt, and respect as "
         "clearly as words."},
    [LANG_ONDUIC] = {LANG_ONDUIC, "language/onduic", "Onduic",
                     "The practical road language of Onduis, shaped by distance, mutual aid, and "
                     "the changing cost "
                     "of travel."},
    [LANG_SELERIC] =
        {LANG_SELERIC, "language/seleric", "Seleric",
         "The evidence-marking language of Selerish scholars, healers, navigators, and public "
         "arguments."},
    [LANG_CARSTANI] = {LANG_CARSTANI, "language/carstani", "Carstani",
                       "Carstan's many-angled glass-coast tongue, used by artisans, advocates, "
                       "storm-readers, and "
                       "smugglers."},
    [LANG_AXTROSI] = {LANG_AXTROSI, "language/axtrosi", "Axtrosi",
                      "The sung road language of Axtros, preserving routes, hospitality, and "
                      "careful boundaries."},
    [LANG_HIRI] =
        {LANG_HIRI, "language/hiri", "Hiri",
         "The river-and-market language of Hir and Pesh, joining Anterean memory to frontier "
         "practicality."},
    [LANG_QUECHIAN] = {LANG_QUECHIAN, "language/quechian", "Quechian",
                       "The living forest tongue of Quechian, attentive to the effects of every "
                       "choice on people, "
                       "creatures, and place."},
    [LANG_VAILIC] =
        {LANG_VAILIC, "language/vailic", "Vailic",
         "Vailand's mobile lake-and-heath language, woven to preserve routes and belonging through "
         "change."},
    [LANG_OORPIC] = {LANG_OORPIC, "language/oorpic", "Oorpic",
                     "The wind-carrying tongue and tactile rope script of Oorpii's ships, bridges, "
                     "and temporary "
                     "councils."},
    [LANG_TAL] = {LANG_TAL, "language/tal", "Tal",
                  "The harmonic Heart Tongue of the Crystal Reaches and Kellust, spoken in voice, "
                  "vibration, "
                  "and stone."},
    [LANG_UBDINIC] = {LANG_UBDINIC, "language/ubdinic", "Ubdinic",
                      "The shared Heart Tongue of East and West Ubdina, rich in words for memory, "
                      "states of being, "
                      "and merciful delay."},
};

static const struct character_creation_guidance guidance[] = {
    {"profile/goals",
     "What your character is trying to achieve, why it matters, and what makes it difficult.",
     "A goal gives your character somewhere to lean. It may be immediate, such as earning passage "
     "across the Shallow Sea; personal, such as finding a missing teacher; or vast, such as "
     "preventing the next Darkling War. Strong goals include an objective, a reason, and a "
     "complication. The stakes answer what may be lost if the character fails or refuses to try.",
     "What does your character want now? Why do they want it? What stands in the way, and what "
     "will failure cost?",
     "1. Objective: A concrete direction, not a guaranteed ending. 2. Reason: The need, hope, "
     "fear, duty, or desire beneath it. 3. Complication: An obstacle that demands choices rather "
     "than mere time. 4. Stakes: Optional editor guidance describing the cost of failure or "
     "inaction. A minimal generated outline may fold this into Complication.\r\n\r\nGoals can be "
     "short-, middle-, or long-term. They can conflict. They should be revised when play changes "
     "the character."},
    {"profile/personality",
     "The habits, mannerisms, tastes, and contradictions that make your character recognizable.",
     "Personality is how a character's inner life becomes visible. Prefer specific behavior over "
     "broad labels. \"I am clever\" says little; \"I correct maps in other people's homes\" "
     "suggests pride, curiosity, and an excellent way to start an argument. A useful pair of "
     "traits often contains one strength and one complication.",
     "What does your character repeatedly do, notice, enjoy, avoid, or misunderstand? What would a "
     "companion imitate when telling a story about them?",
     "Two distinct first-person traits shaped by the selected inspiration theme."},
    {"profile/ideals", "The principles your character tries to protect when choices become costly.",
     "An ideal is not a slogan worn when convenient. It is the belief a character uses to decide "
     "between competing goods, or the justification they reach for when doing harm. Ideals may be "
     "noble, selfish, contradictory, inherited, or newly chosen. The best ones create decisions in "
     "play.",
     "What principle will your character defend at a cost? What could persuade them that they have "
     "understood it wrongly?",
     "Two distinct first-person convictions shaped by the selected inspiration theme. Alignment "
     "may color an ideal, but never replaces one."},
    {"profile/bonds",
     "The people, places, promises, events, and treasured things your character cannot treat as "
     "ordinary.",
     "A bond gives the world a handle on the character. It may inspire courage or terrible "
     "judgment. Name the person, place, promise, event, or object when possible, and explain why "
     "it matters. A bond can be gained, fulfilled, betrayed, transformed, or released through "
     "play.",
     "Who or what can call your character back, draw them onward, or make them risk more than "
     "reason allows?",
     "Two distinct first-person connections shaped by the selected inspiration theme."},
    {"profile/flaws",
     "The fear, vice, compulsion, blind spot, or weakness that can pull your character against "
     "their own interests.",
     "A flaw is an invitation to meaningful trouble, not a punishment and not permission to spoil "
     "another player's play. It should create choices, consequences, or vulnerability. \"I am "
     "evil\" is too broad. \"I mistake obedience for loyalty when I am afraid\" gives the "
     "character somewhere to struggle and grow.",
     "What can provoke, tempt, frighten, or mislead your character? How does the flaw hurt "
     "something they genuinely value?",
     "Two distinct first-person complications shaped by the selected inspiration theme."},
};

static const struct character_creation_background backgrounds[NUM_BACKGROUNDS] = {
    [BACKGROUND_ACOLYTE] =
        {BACKGROUND_ACOLYTE,
         "background/acolyte",
         "Service placed between the mortal and the sacred",
         "You learned sacred work before you understood sacred power. Perhaps you kept the dawn "
         "lamps "
         "of Seraphine, counted burial names for Nethris, tuned a roadside shrine to the Loom, or "
         "served a temple whose god never answered in a voice you could recognize. You know that "
         "faith "
         "is made from ordinary labor: floors swept, food shared, rites remembered, grief given a "
         "shape it can survive.\r\n\r\nAn acolyte need not be a divine caster. The calling may be "
         "devotion, scholarship, family duty, refuge, doubt, or an old promise still being tested.",
         {
             {"I remember the proper rite for every occasion and improvise one when none exists.",
              "I speak gently in temples and argue fiercely about what they owe the hungry."},
             {"Sacred things are proven by the care they inspire, not the fear they demand.",
              "A vow freely made can hold more firmly than iron."},
             {"I still carry the key to a shrine whose doors no longer exist.",
              "A pilgrim once trusted me with a confession I have never been able to forget."},
             {"I mistake suffering for proof that a path is righteous.",
              "When the divine remains silent, I fill the silence with my own certainty."},
         }},
    [BACKGROUND_CHARLATAN] =
        {BACKGROUND_CHARLATAN,
         "background/charlatan",
         "Desire read quickly and truth bent artfully",
         "You learned that most people do not buy an object. They buy relief, importance, hope, "
         "revenge, youth, or the brief pleasure of being told the world works as they wish. You "
         "can "
         "hear that hidden purchase in a person's voice. Perhaps you sold false relics in "
         "Ashenport, "
         "impossible weather insurance in Selerish, or maps to roads that would exist by the time "
         "the "
         "buyer arrived.\r\n\r\nThe art is not merely lying. It is building a bridge from desire "
         "and "
         "charging toll before anyone notices the far bank is painted.",
         {
             {"I give every stranger the version of me they most want to meet.",
              "I cannot resist improving a dull truth until it sparkles dangerously."},
             {"If hope keeps someone moving, its pedigree matters less than its effect.",
              "No title deserves immunity from a well-aimed embarrassment."},
             {"I owe my life to the only mark who saw through me and laughed.",
              "Somewhere, a family treasures a worthless charm I sold them for a worthy reason."},
             {"I keep performing sincerity after the truth would serve me better.",
              "The more impossible the deception, the more personally I need it to work."},
         }},
    [BACKGROUND_CRIMINAL] =
        {BACKGROUND_CRIMINAL,
         "background/criminal",
         "Survival within hidden systems and dangerous trust",
         "You know the city beneath the city: chalk signs under bridges, names omitted from "
         "ledgers, "
         "doors that open only after the wrong knock. You may have stolen, smuggled, watched, "
         "forged, "
         "carried messages, or survived among people for whom trust is both currency and "
         "weapon.\r\n\r\nThe hidden world is not one family. Ashenport's street crews, Sanctine "
         "refugee networks, Anterean dissidents, Free City spies, and Darkling agents may use the "
         "same "
         "tunnel for very different ends. You learned to ask who benefits before calling any "
         "shadow "
         "kin.",
         {
             {"I notice exits before faces and hands before smiles.",
              "I answer direct questions with useful truths that are not quite answers."},
             {"A law that protects only the powerful has already declared itself my enemy.",
              "Trust should be difficult to earn and terrible to betray."},
             {"My old crew knows the name I wore before I became this person.",
              "I keep one route open for people escaping the life I once served."},
             {"I test loyal people until they finally behave like traitors.",
              "I feel safest when I possess a secret someone else cannot afford to lose."},
         }},
    [BACKGROUND_ENTERTAINER] =
        {BACKGROUND_ENTERTAINER,
         "background/entertainer",
         "Art made public enough to change a room",
         "You have felt a crowd become one listening creature. Song, dance, story, comedy, "
         "acrobatics, "
         "masks, and small impossibilities are tools; attention is the true instrument. In Lumia, "
         "where memory strengthens the Loom, a performance can keep a village's name alive or give "
         "a "
         "frightened company the courage to take one more step.\r\n\r\nYou know the labor behind "
         "wonder: rehearsal on bleeding feet, strings replaced in rain, jokes rebuilt for a "
         "grieving "
         "room, and the lonely walk after applause has spent itself.",
         {
             {"I narrate tense moments as if better pacing might save us.",
              "I collect the laugh of every place and borrow it when my own courage fails."},
             {"Beauty is not an escape from suffering; it is evidence suffering did not take "
              "everything.",
              "A story belongs partly to every listener who carries it onward."},
             {"My first audience was a village that no longer appears on any map.",
              "I am searching for the lost final verse of my teacher's greatest song."},
             {"Silence feels so much like rejection that I fill it before listening.",
              "I would rather fail spectacularly in public than succeed unnoticed."},
         }},
    [BACKGROUND_FOLK_HERO] =
        {BACKGROUND_FOLK_HERO,
         "background/folk-hero",
         "An ordinary community's extraordinary expectation",
         "You were ordinary where ordinary people knew your name. Then the flood came, the beast "
         "crossed the fence, the tax collector took too much, or the local strongman learned that "
         "fear "
         "had limits. You acted. The story grew in the telling. Now people from home look at you "
         "as if "
         "their hope were a cloak you chose to wear.\r\n\r\nPerhaps the story is true. Perhaps it "
         "leaves out help, luck, or harm. Either way, a community has placed part of its future in "
         "your hands.",
         {
             {"I speak to rulers with the same plain courtesy I give a neighbor.",
              "Praise makes me uncomfortable, so I turn every compliment into a task."},
             {"Great powers exist to serve the people who carry their cost.",
              "Courage begins when someone decides a familiar wrong is no longer normal."},
             {"My home keeps a chair for me, and I fear the day I no longer fit it.",
              "The person who truly saved everyone receives none of the songs sung about me."},
             {"I accept dangers alone because asking help would complicate the legend.",
              "I assume humble origins make my judgment morally cleaner than it is."},
         }},
    [BACKGROUND_GLADIATOR] =
        {BACKGROUND_GLADIATOR,
         "background/gladiator",
         "Survival performed beneath the judgment of crowds",
         "You learned violence beneath watching eyes. Whether the arena was an Ashenport pit, a "
         "noble "
         "court, a military exhibition, or a traveling ring, you were trained to make danger "
         "legible "
         "to a crowd. A clean victory could be forgotten; a memorable one bought another week of "
         "life.\r\n\r\nThe audience saw confidence, rivalry, and spectacle. You remember sand in "
         "the "
         "mouth, coded glances between opponents, healers waiting just beyond the gate, and the "
         "strange intimacy of trusting another fighter to make a near miss look fatal.",
         {
             {"I enter every room as if someone has already announced my name.",
              "I can read a crowd's mood faster than I can read a private conversation."},
             {"Skill deserves witness, but no audience owns the person performing it.",
              "Mercy shown from strength is the highest form of victory."},
             {"I owe a rival the honest rematch neither of us was allowed.",
              "I still hear the arena medic who taught me that survival is also a craft."},
             {"I turn real danger into spectacle and miss when others are truly afraid.",
              "Being ignored wounds me more deeply than losing."},
         }},
    [BACKGROUND_TRADER] =
        {BACKGROUND_TRADER,
         "background/trader",
         "Craft, value, reputation, and exchange",
         "You learned value at a workbench, market stall, guild table, caravan fire, or night-tide "
         "bazaar. You know that price is never the whole cost. Reputation, scarcity, time, danger, "
         "beauty, and the buyer's need all sit invisibly on the scale.\r\n\r\nPerhaps you craft "
         "what "
         "you sell. Perhaps you connect distant makers to people who need their work. Either way, "
         "your "
         "true inventory is relationship: who pays fairly, who delivers in winter, who cheats only "
         "strangers, and who keeps a promise after profit disappears.",
         {
             {"I appraise unfamiliar objects when nervous, including furniture and people.",
              "I remember every favor as carefully as other traders remember coin."},
             {"Exchange is honorable only when both people can afford to walk away.",
              "A well-made thing is a promise from the maker to a future stranger."},
             {"My guild mark opens doors, and I intend to learn what was done to earn that trust.",
              "A ruined caravan partner's family receives a share of every profit I make."},
             {"I reduce choices to transactions when no fair price exists.",
              "I cannot leave a bargain unfinished, even when winning it costs more than losing."},
         }},
    [BACKGROUND_HERMIT] =
        {BACKGROUND_HERMIT,
         "background/hermit",
         "Solitude that revealed or concealed a truth",
         "You stepped away from the noise. Perhaps you sought a god, escaped a war, guarded a "
         "place, "
         "studied one impossible question, recovered from a wound, or simply discovered that "
         "solitude "
         "asked less of you than people did.\r\n\r\nIn the silence you found something: a truth, a "
         "delusion, a discipline, a friendship with the weather, or the uncomfortable knowledge "
         "that "
         "isolation had become another appetite. Returning does not mean the hermitage failed. "
         "Sometimes understanding needs friction before it becomes wisdom.",
         {
             {"I answer after long pauses because thoughts deserve room to arrive.",
              "I treat weather, animals, and old buildings as participants in conversation."},
             {"A truth that cannot survive solitude was probably applause.",
              "Wisdom must eventually return to the world or become a beautifully guarded waste."},
             {"I left someone maintaining the place that remade me.",
              "My seclusion began with a question whose answer now frightens me."},
             {"I call avoidance peace when other people become difficult.",
              "I assume clarity earned alone grants authority over lives lived together."},
         }},
    [BACKGROUND_SQUIRE] =
        {BACKGROUND_SQUIRE,
         "background/squire",
         "Service beside an ideal not yet fully earned",
         "You served beside a knight, champion, officer, or sworn wanderer. You polished steel, "
         "kept "
         "schedules, calmed mounts, learned heraldry, carried messages, and saw which glorious "
         "stories "
         "omitted wet socks and frightened horses. You stood close enough to an ideal to notice "
         "the "
         "person failing beneath it.\r\n\r\nPerhaps you still seek knighthood. Perhaps you "
         "rejected "
         "the Orders, lost your mentor, surpassed them, or discovered that service taught you a "
         "form "
         "of leadership ceremony never could.",
         {
             {"I prepare for other people's needs before asking what I need.",
              "I judge impressive armor by the state of its least visible strap."},
             {"Honor is what remains of a vow when nobody important is watching.",
              "Service should train a person to stand, not teach them to remain kneeling."},
             {"I carry my mentor's unfinished oath and do not know whether to fulfill it.",
              "Another squire took blame meant for me; every success since has carried their "
              "name."},
             {"I wait for permission from authorities who are no longer present.",
              "I confuse proximity to greatness with possession of its virtues."},
         }},
    [BACKGROUND_NOBLE] =
        {BACKGROUND_NOBLE,
         "background/noble",
         "Privilege carrying power, expectation, and debt",
         "You were raised inside consequence. A family name, inherited office, estate, court "
         "appointment, merchant elevation, or ancestral claim taught others to listen before you "
         "had "
         "earned their attention. Privilege can provide education, safety, and reach. It can also "
         "make "
         "comfort look like merit and obedience look like love.\r\n\r\nLumia's nobility is not one "
         "species. Anterean houses inherit duties from the dead. Free City patrons buy public "
         "works "
         "and private influence. Chulani magisters turn exam scores into dynasties. Your title may "
         "be "
         "secure, disputed, newly granted, disowned, or carried like an unpaid bill.",
         {
             {"I was trained to make every entrance look intentional, including escapes.",
              "I know the correct form of address and use the wrong one when respect requires it."},
             {"Privilege is a debt payable only through public service.",
              "My name should open doors because of what I do with access, not who first owned the "
              "key."},
             {"My house protects people history treats as entries beneath its crest.",
              "A rival relative knows the true cost of my inheritance."},
             {"I mistake being heard quickly for being right.",
              "When ashamed, I retreat into rank and make everyone else pay for the distance."},
         }},
    [BACKGROUND_OUTLANDER] =
        {BACKGROUND_OUTLANDER,
         "background/outlander",
         "A life measured by land rather than walls",
         "You grew where roads were occasional suggestions. Herd routes, mountain weather, desert "
         "stars, forest permission, and the moods of rivers mattered more than walls. This does "
         "not "
         "make you ignorant of civilization. It means you learned another kind "
         "first.\r\n\r\nPerhaps "
         "you were a nomad, hunter, scout, raider, pilgrim, exile, caravan child, or keeper of an "
         "isolated station. You know landscapes are not empty between settlements. They are full "
         "of "
         "appetite, memory, warning, and lives that do not need a city to become real.",
         {
             {"I note wind, tracks, and exits aloud without realizing others cannot read them.",
              "I sleep more easily beneath an uncertain sky than a locked roof."},
             {"Land is relationship, not the blank space between owners.",
              "Preparation is respect paid to dangers before meeting them."},
             {"A migration route taught me more faithfully than any living mentor.",
              "I promised to return with news to people who may already have moved on."},
             {"I dismiss city customs as softness when I simply do not understand them.",
              "I keep moving so no place can ask me to become accountable."},
         }},
    [BACKGROUND_PIRATE] =
        {BACKGROUND_PIRATE,
         "background/pirate",
         "Freedom and predation beyond settled law",
         "You lived by taking freedom from waters claimed by others--and perhaps by taking cargo "
         "from "
         "people who claimed it first. Pirate crews range from brutal predators to outlaw navies, "
         "escaped laborers, political rebels, sanctioned privateers, and overlooked sailors who "
         "made a "
         "country from a deck.\r\n\r\nThe sea does not make anyone noble. It merely removes many "
         "witnesses. Whatever code your crew kept mattered because no distant court could enforce "
         "it. "
         "You know exactly what kind of person you became when law was over the horizon.",
         {
             {"I treat every formal plan as weather: worth reading, foolish to trust.",
              "I remember people by what they did during storms."},
             {"No crown owns the horizon.", "A crew survives only when shares, danger, and voice "
                                            "are distributed by a code everyone can "
                                            "name."},
             {"My old vessel is still sailing under someone who should never have inherited "
              "command.",
              "I buried a share of treasure for a crewmate who did not live to spend it."},
             {"I call predation freedom whenever admitting harm would cost me pride.",
              "Authority provokes me even when cooperation would protect my crew."},
         }},
    [BACKGROUND_SAGE] =
        {BACKGROUND_SAGE,
         "background/sage",
         "Knowledge pursued until it begins to pursue back",
         "You pursued knowledge long enough for it to alter your posture, your sleep, and the "
         "number "
         "of friends willing to ask a simple question. Your school may have been a Chulani "
         "college, a "
         "Crystal memory vault, a temple archive, a halfling map library, an apprenticeship, or "
         "ruins "
         "that killed every previous researcher.\r\n\r\nIn Lumia, scholarship is dangerous because "
         "some records remember the reader. The wise learn methods, provenance, and humility. The "
         "merely learned acquire more confident ways to be wrong.",
         {
             {"I cite sources during arguments and apologize only for the footnote order.",
              "I become delighted, not embarrassed, when evidence defeats my favorite theory."},
             {"Knowledge deserves stewardship because secrecy and disclosure can both wound.",
              "No authority is old enough to stand above a well-formed question."},
             {"I seek the missing volume of an archive whose surviving pages refer to me by name.",
              "My greatest discovery belongs partly to an assistant history forgot."},
             {"I would open a sealed door for the chance to learn why it was sealed.",
              "I treat people as evidence when their pain complicates my conclusion."},
         }},
    [BACKGROUND_SAILOR] =
        {BACKGROUND_SAILOR,
         "background/sailor",
         "Duty and belonging aboard a working vessel",
         "You served aboard a vessel where every hand's mistake became shared weather. Merchant "
         "hull, "
         "fishing boat, naval ship, ferry, explorer, or Swiftpath tender: the work taught knots, "
         "watches, repairs, currents, and the intimacy of trusting sleep to people you did not "
         "choose.\r\n\r\nUnlike a pirate, your identity need not reject shore law. Unlike a "
         "passenger, "
         "you know a ship is not freedom floating on water. It is obligation made of wood, cloth, "
         "labor, and constant small repairs.",
         {
             {"I turn household chores into watch rotations before anyone can object.",
              "I judge a leader by whether they take the worst watch in bad weather."},
             {"Competence is a form of care when other lives depend on one's hands.",
              "No voyage justifies treating the crew as cargo."},
             {"I can find my old ship by the sound of one loose board.",
              "The sea kept someone I loved, and I still bargain with every horizon for their "
              "return."},
             {"I obey a confident order before asking whether it is wise.",
              "On land, I create emergencies because calm without a task feels like drift."},
         }},
    [BACKGROUND_SOLDIER] =
        {BACKGROUND_SOLDIER,
         "background/soldier",
         "Training, comradeship, and the memory of violence",
         "You were trained to make fear arrive on schedule. A national army, town militia, "
         "mercenary "
         "company, Knight auxiliary, caravan defense, or desperate uprising taught you weapons, "
         "formations, supply, and the thousand uncelebrated skills that keep fighters "
         "alive.\r\n\r\nWar gave you comrades and may have taken them. It may have taught "
         "discipline, "
         "obedience, courage, numbness, cruelty, or the difference between each. Lumia prepares "
         "for "
         "another conflict while still misunderstanding the last. You carry one of its smaller, "
         "truer "
         "histories.",
         {
             {"I sit where I can see the door and make it look like chance.",
              "I use dry humor when everyone else needs permission to admit fear."},
             {"Discipline exists to protect people from panic, including the panic of commanders.",
              "The purpose of force is to end the need for force."},
             {"I keep the names of my unit where no official history can revise them.",
              "A former enemy spared me for a reason I still need to understand."},
             {"I obey structure when conscience would require disobedience.",
              "I treat peaceful disagreement as a threat to cohesion."},
         }},
    [BACKGROUND_URCHIN] =
        {BACKGROUND_URCHIN,
         "background/urchin",
         "A childhood survived in the overlooked city",
         "You grew in the city spaces maps leave blank: roofs, culverts, market awnings, abandoned "
         "shrines, kitchens after closing, and alleys with six names depending on who is asking. "
         "Survival required speed, observation, alliances, and the ability to become unimportant "
         "when "
         "danger looked your way.\r\n\r\nPoverty was not a picturesque teacher. It was hunger, "
         "exposure, sickness, and adults deciding not to see. What you learned belongs to you. "
         "What "
         "happened to you was not proof that suffering was necessary.",
         {
             {"I pocket food before remembering I no longer need to.",
              "I know which grand buildings have warm vents and badly watched windows."},
             {"Nobody is disposable merely because powerful people learned not to notice them.",
              "Survival creates responsibility when others are still trapped where I escaped."},
             {"A loose family of street children still uses the signs I taught them.",
              "I owe everything to a shopkeeper who pretended not to notice one theft too many."},
             {"I hoard resources past need because safety still feels temporary.",
              "Kindness makes me search for the trap until I injure the person offering it."},
         }},
};

const struct character_creation_homeland *character_creation_homeland_for_region(int region)
{
  if (region <= REGION_NONE || region >= NUM_REGIONS || homelands[region].content_id == NULL)
    return NULL;

  return &homelands[region];
}

const struct character_creation_language *character_creation_language_for_index(int language)
{
  if (language < 0 || language >= NUM_LANGUAGES || heart_tongues[language].content_id == NULL)
    return NULL;

  return &heart_tongues[language];
}

const struct character_creation_guidance *
character_creation_guidance_for_profile(const char *profile_id)
{
  size_t index = 0;

  if (profile_id == NULL)
    return NULL;

  for (index = 0; index < sizeof(guidance) / sizeof(guidance[0]); index++)
    if (!strcmp(guidance[index].profile_id, profile_id))
      return &guidance[index];

  return NULL;
}

const struct character_creation_background *character_creation_background_for_value(int background)
{
  if (background <= BACKGROUND_NONE || background >= NUM_BACKGROUNDS ||
      backgrounds[background].content_id == NULL)
    return NULL;

  return &backgrounds[background];
}

const char *character_creation_inspiration_seed(int background,
                                                enum character_creation_inspiration_kind kind,
                                                int index)
{
  const struct character_creation_background *record =
      character_creation_background_for_value(background);

  if (record == NULL || kind < 0 || kind >= NUM_CHARACTER_CREATION_INSPIRATION_KINDS || index < 0 ||
      index >= 2)
    return NULL;

  return record->seeds[kind][index];
}

const char *character_creation_hometown_summary(int hometown)
{
  if (hometown == 1)
    return "Lumia's Great Port and principal adventuring hub.";

  return NULL;
}

const char *character_creation_hometown_description(int hometown)
{
  if (hometown == 1)
  {
    return "Ashenport is a diverse harbor city at the mouth of the River Veyr, governed by a "
           "mayor and the Republic of Nine. It is Lumia's principal trade and adventuring hub, "
           "with a major Swiftpath, broad services, and roads into the quest regions of Onduis. "
           "Choosing it as Hometown makes the city your practical point of return; choosing "
           "Ashenport as Homeland separately means its civic culture formed your character.";
  }

  return NULL;
}

const char *character_creation_content_provenance(void)
{
  return content_provenance;
}

#else

const struct character_creation_homeland *
character_creation_homeland_for_region(int region __attribute__((unused)))
{
  return NULL;
}

const struct character_creation_language *
character_creation_language_for_index(int language __attribute__((unused)))
{
  return NULL;
}

const struct character_creation_guidance *
character_creation_guidance_for_profile(const char *profile_id __attribute__((unused)))
{
  return NULL;
}

const struct character_creation_background *
character_creation_background_for_value(int background __attribute__((unused)))
{
  return NULL;
}

const char *character_creation_inspiration_seed(int background __attribute__((unused)),
                                                enum character_creation_inspiration_kind kind
                                                __attribute__((unused)),
                                                int index __attribute__((unused)))
{
  return NULL;
}

const char *character_creation_hometown_summary(int hometown __attribute__((unused)))
{
  return NULL;
}

const char *character_creation_hometown_description(int hometown __attribute__((unused)))
{
  return NULL;
}

const char *character_creation_content_provenance(void)
{
  return NULL;
}

#endif
