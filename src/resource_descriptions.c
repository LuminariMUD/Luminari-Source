/* ******#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "wilderness.h"
#include "weather.h"
#include "resource_depletion.h"
#include "resource_descriptions.h"**********************************************************
 *   File: resource_descriptions.c             Part of LuminariMUD        *
 *  Usage: Dynamic resource-based descriptions implementation              *
 * Author: Dynamic Descriptions Implementation                             *
 ***************************************************************************
 * Resource-aware description generation for creating beautiful,           *
 * immersive wilderness descriptions that change based on ecological state *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "wilderness.h"
#include "resource_depletion.h"
#include "resource_system.h"
#include "resource_descriptions.h"

/* Only compile if dynamic descriptions are enabled for this campaign */
#ifdef ENABLE_DYNAMIC_RESOURCE_DESCRIPTIONS

/* ===== DESCRIPTION TEMPLATES ===== */

/* Base terrain descriptions by vegetation level */
static const char *forest_abundant[] = {
    "Ancient %s trees tower overhead, their massive trunks rising from rich, dark earth",
    "Towering %s trees form a majestic canopy that filters golden sunlight",
    "Magnificent %s trees stretch skyward, their gnarled branches intertwining overhead",
    "Colossal %s trees create a cathedral of living wood, their canopy lost in shadow",
    "Massive %s trees rise like pillars, their trunks scarred by centuries of growth",
    "Primeval %s trees form a dense woodland where ancient magic seems to linger",
    "Enormous %s trees dominate the landscape, their roots visible above the fertile soil",
    "Giant %s trees create a verdant maze of trunks and shadows",
    "Majestic %s trees form natural archways with their intertwining branches",
    "Towering %s trees rise from a carpet of thick moss and fallen leaves",
    "Ancient %s trees create a living fortress of bark and branch",
    "Immense %s trees dwarf all else, their canopy home to countless creatures",
    "Stately %s trees rise like columns in nature's grand cathedral",
    "Graceful %s trees weave a tapestry of branches against the sky",
    "Noble %s trees stand as silent sentinels of the woodland realm",
    "Venerable %s trees whisper secrets in their rustling leaves",
    "Regal %s trees form a crown of emerald splendor",
    "Epic %s trees tell stories written in rings of countless seasons",
    "Sublime %s trees create a sanctuary of tranquil beauty",
    "Resplendent %s trees shimmer with dewdrops like nature's jewelry",
    "Immortal %s trees bridge earth and heaven with their soaring presence",
    "Breathtaking %s trees dance in harmonious symphony with the wind"
};

static const char *forest_moderate[] = {
    "Mature %s trees create a pleasant woodland grove",
    "Sturdy %s trees rise from soil enriched by countless seasons of fallen leaves",
    "Well-established %s trees form a comfortable forest setting",
    "Healthy %s trees create dappled patterns of light and shadow",
    "Robust %s trees grow in natural clusters across the woodland floor",
    "Solid %s trees form a peaceful grove where wildlife feels safe",
    "Strong %s trees create natural clearings between their sturdy trunks",
    "Mature %s trees offer shelter beneath their spreading branches",
    "Established %s trees create a balanced ecosystem of forest life",
    "Thriving %s trees show the resilience of this woodland community",
    "Stable %s trees form natural windbreaks across the gentle terrain",
    "Dependable %s trees create reliable landmarks in the forest paths",
    "Handsome %s trees frame picturesque clearings with natural elegance",
    "Charming %s trees create intimate glades perfect for quiet reflection",
    "Welcoming %s trees form natural shelters beneath their protective boughs",
    "Gracious %s trees offer their shade to weary travelers",
    "Stalwart %s trees stand as faithful guardians of the woodland paths",
    "Serene %s trees create pockets of peace within their gentle embrace",
    "Harmonious %s trees blend seamlessly with the surrounding landscape",
    "Inviting %s trees beckon visitors to explore their woodland secrets",
    "Companionable %s trees seem to nod in greeting to passing travelers",
    "Hospitable %s trees create natural resting places along the forest ways"
};

static const char *forest_sparse[] = {
    "Scattered %s trees dot the rolling landscape",
    "Weathered %s stumps and young saplings mark this quiet hillside",
    "Hardy %s trees grow in isolated groves across the terrain",
    "Lone %s trees stand sentinel over the recovering landscape",
    "Remnant %s trees bear witness to the forest's earlier glory",
    "Stubborn %s trees persist despite the challenging conditions",
    "Surviving %s trees create islands of green in the sparse terrain",
    "Resilient %s trees mark where the forest once thrived",
    "Determined %s saplings struggle to reclaim their ancestral home",
    "Sparse %s groves offer little shelter from the elements",
    "Isolated %s trees stand like lonely guardians of the past",
    "Struggling %s trees fight to establish new growth patterns",
    "Solitary %s trees cast long shadows across the open ground",
    "Brave %s saplings reach toward the unfiltered sky",
    "Pioneer %s trees mark the beginning of nature's slow return",
    "Tenacious %s trees cling to life in this exposed landscape",
    "Stoic %s trees endure the harsh winds with quiet dignity",
    "Hopeful %s seedlings emerge from between patches of bare earth",
    "Courageous %s trees face the elements with unwavering resolve",
    "Patient %s trees await the forest's eventual renaissance",
    "Defiant %s trees refuse to surrender to the austere conditions",
    "Visionary %s trees imagine the woodland's future splendor"
};

/* Hills terrain descriptions */
static const char *hills_abundant[] = {
    "Rolling hills stretch into the distance, their slopes carpeted with lush vegetation",
    "Verdant hillsides rise and fall across the landscape, rich with plant life",
    "Gently undulating hills create a picturesque countryside scene",
    "Emerald hills flow like waves across the fertile landscape",
    "Lush hillsides cascade into gentle valleys filled with wildflowers",
    "Terraced hills create natural amphitheaters of abundant growth",
    "Rounded hills form a patchwork quilt of diverse plant communities",
    "Sweeping hills offer panoramic views of thriving ecosystems",
    "Graceful hills create natural gardens where every slope blooms",
    "Fertile hills rise and fall like a green ocean beneath the sky",
    "Magnificent hills showcase nature's artistry in living color",
    "Bountiful hills create perfect harmony between earth and sky",
    "Opulent hills drape themselves in cloaks of emerald splendor",
    "Luxuriant hillsides unfold like pages in nature's poetry book",
    "Sumptuous hills create an earthly paradise of rolling beauty",
    "Velvet hills stretch like royal carpets across the countryside",
    "Jeweled hills sparkle with dewdrops and morning mist",
    "Enchanted hills seem touched by fairy magic and ancient dreams",
    "Silk-soft hills caress the eye with their gentle curves",
    "Golden hills glow with inner light beneath the dancing sky",
    "Painterly hills create masterpieces of shadow and illumination",
    "Symphonic hills rise and fall in perfect natural rhythm"
};

static const char *hills_moderate[] = {
    "Modest hills rise from the surrounding terrain, dotted with hardy shrubs",
    "The rolling landscape shows patches of vegetation across weathered slopes",
    "Gentle hillsides create natural contours in the surrounding countryside",
    "Weathered hills display a mosaic of grass and exposed earth",
    "Rounded hills offer comfortable resting spots among scattered plants",
    "Modest slopes create natural pathways through the varied terrain",
    "Balanced hills show both challenge and opportunity for growth",
    "Steady hills provide reliable landmarks in the changing landscape",
    "Practical hills offer shelter in their valleys and exposure on their peaks",
    "Honest hills reveal the true character of this working landscape",
    "Straightforward hills create clear boundaries between different areas",
    "Reliable hills form the backbone of this pastoral countryside",
    "Contemplative hills invite quiet reflection in their peaceful folds",
    "Humble hills showcase understated beauty in simple forms",
    "Approachable hills welcome wanderers with their gentle gradients",
    "Thoughtful hills create natural viewing platforms across the terrain",
    "Restful hills offer sanctuary from the demands of level ground",
    "Neighborly hills form a community of modest earthen dwellings",
    "Comfortable hills provide natural windbreaks and sunny exposures",
    "Familiar hills create recognizable landmarks for frequent travelers",
    "Practical hills offer both challenge and rest in perfect measure",
    "Sensible hills demonstrate the wisdom of moderate proportions"
};

static const char *hills_sparse[] = {
    "Barren hills stretch across the landscape, their slopes scoured by wind and weather",
    "Rocky hillsides rise starkly from the surrounding terrain",
    "Windswept hills create a dramatic but desolate landscape",
    "Harsh hills bear the scars of persistent erosion and neglect",
    "Stark hills rise like sleeping giants from the austere terrain",
    "Rugged hills challenge any attempt at easy passage",
    "Unforgiving hills reveal layers of ancient stone and thin soil",
    "Severe hills create natural barriers across the challenging landscape",
    "Austere hills offer little comfort to travelers or wildlife",
    "Demanding hills test the endurance of any who cross them",
    "Relentless hills stretch endlessly under the vast sky",
    "Stoic hills stand as monuments to nature's harder lessons",
    "Dramatic hills thrust bold silhouettes against the horizon",
    "Sculptural hills reveal the raw artistry of geological forces",
    "Primal hills speak of earth's ancient and tumultuous history",
    "Magnificent hills demonstrate the savage beauty of exposed stone",
    "Haunting hills create an otherworldly landscape of mystery",
    "Brooding hills cast long shadows across the desolate terrain",
    "Elemental hills showcase the fundamental power of earth and sky",
    "Moonlike hills create an alien beauty beneath the wandering clouds",
    "Timeless hills stand as eternal witnesses to the passage of ages",
    "Sublime hills inspire both wonder and respect for nature's raw power"
};

/* Plains/Field descriptions */
static const char *plains_abundant[] = {
    "Vast expanses of rich grassland stretch to the horizon",
    "Rolling plains extend in all directions, carpeted with thick grass",
    "Fertile meadows create a sea of green beneath the open sky",
    "Endless grasslands wave like an ocean under the infinite sky",
    "Luxuriant plains create a carpet of emerald that extends beyond sight",
    "Magnificent prairies showcase the grandeur of open spaces",
    "Abundant grasslands form a living tapestry of countless plant species",
    "Thriving meadows create natural amphitheaters for wildlife gatherings",
    "Rich plains offer sustenance to countless creatures great and small",
    "Verdant steppes create highways for migrating herds and wanderers",
    "Bountiful grasslands form the foundation of entire ecosystems",
    "Prolific plains demonstrate nature's ability to create abundance from simplicity",
    "Majestic prairies unfold like nature's grandest ballroom floor",
    "Silken grasslands ripple with ethereal beauty beneath the vast dome of sky",
    "Celestial meadows seem to merge earth with heaven in endless harmony",
    "Dreamlike plains dance with millions of grass blades in synchronized ballet",
    "Paradisiacal steppes create a living canvas painted with infinite shades of green",
    "Euphoric grasslands sing with the music of wind and growing things",
    "Transcendent plains offer glimpses of earth's original perfection",
    "Mystical meadows shimmer with an almost supernatural vitality",
    "Rapturous prairies celebrate the pure joy of unrestrained growth",
    "Halcyon grasslands embody peace and plenty in their swaying abundance"
};

static const char *plains_moderate[] = {
    "Open grasslands stretch across the landscape",
    "The plains show patches of grass and wildflowers",
    "Rolling fields create gentle waves across the terrain",
    "Modest grasslands provide reliable grazing for local wildlife",
    "Steady plains create natural corridors through the countryside",
    "Balanced meadows offer both shelter and exposure as needed",
    "Working grasslands show the practical beauty of functional landscapes",
    "Reliable plains form the dependable heart of this region",
    "Honest grasslands reveal the true character of the open country",
    "Straightforward plains offer clear views and easy travel",
    "Practical meadows serve the needs of both wildlife and travelers",
    "Stable grasslands create predictable patterns across the terrain",
    "Pastoral plains paint scenes of rustic charm across the countryside",
    "Bucolic meadows create perfect settings for peaceful contemplation",
    "Serene grasslands offer respite from the complexities of forest and mountain",
    "Tranquil plains provide mental clarity through their simple beauty",
    "Harmonious steppes blend earth and sky in pleasing proportions",
    "Gentle prairies cradle the spirit with their understated grace",
    "Wholesome grasslands embody the virtue of unpretentious beauty",
    "Welcoming meadows invite travelers to pause and appreciate simple pleasures",
    "Companionable plains offer friendship to those who journey across their expanse",
    "Reassuring grasslands provide comfort through their consistent, honest character"
};

static const char *plains_sparse[] = {
    "Sparse grassland stretches across the barren landscape",
    "Patches of hardy grass struggle to survive in the dry soil",
    "The plains show signs of drought and neglect",
    "Withered grasslands bear witness to harsh conditions",
    "Struggling plains reveal the challenges of survival in open country",
    "Austere grasslands offer little comfort to passing travelers",
    "Harsh plains test the endurance of all who venture across them",
    "Demanding steppes challenge both wildlife and wanderers alike",
    "Severe grasslands create natural barriers through their very emptiness",
    "Unforgiving plains stretch endlessly under the relentless sky",
    "Stark meadows offer stark lessons about nature's harder truths",
    "Barren grasslands stand as monuments to environmental extremes",
    "Minimalist plains reveal the raw elegance of empty space",
    "Zen-like grasslands embrace the profound beauty of simplicity",
    "Monastic steppes offer contemplation through their sacred emptiness",
    "Meditative meadows invite deep reflection beneath the infinite sky",
    "Philosophical plains speak eloquently through their silent vastness",
    "Spiritual grasslands touch the soul with their austere magnificence",
    "Ethereal steppes blur the boundary between earth and emptiness",
    "Ghostly plains haunt the imagination with their spectral beauty",
    "Apocalyptic meadows reveal landscapes from the edge of dreams",
    "Otherworldly grasslands transport visitors to realms beyond the ordinary"
};

/* Water terrain descriptions */
static const char *water_abundant[] = {
    "Crystal-clear waters sparkle with vibrant aquatic life",
    "The pristine waters reveal a thriving underwater ecosystem",
    "Deep, clear waters teem with fish and aquatic vegetation",
    "Brilliant waters dance with schools of colorful fish beneath the surface",
    "Radiant waters reflect light in countless shimmering patterns",
    "Luminous waters create an underwater cathedral of liquid crystal",
    "Gleaming waters reveal the intricate dance of aquatic life below",
    "Sparkling waters form natural windows into a vibrant underwater world",
    "Shimmering waters pulse with the rhythm of countless living creatures",
    "Glittering waters showcase nature's artistry in motion and light",
    "Iridescent waters create ever-changing patterns of beauty and life",
    "Translucent waters offer glimpses into the mysteries of the depths",
    "Sapphire waters mirror the heavens while harboring earthly treasures below",
    "Liquid diamonds cascade in endlessly shifting patterns of pure beauty",
    "Celestial waters seem to flow with starlight captured from the sky above",
    "Enchanted waters weave spells of wonder with their hypnotic movements",
    "Magical waters shimmer with an inner light that defies explanation",
    "Divine waters blessed by the gods themselves flow here in sacred streams",
    "Crystalline waters sing ancient songs in their melodious flow",
    "Pearlescent waters glow with opalescent beauty in the shifting light",
    "Ethereal waters dance between the physical and spiritual realms",
    "Paradisiacal waters create scenes of such beauty that angels might weep"
};

static const char *water_moderate[] = {
    "The calm waters reflect the sky with gentle ripples",
    "Clear waters flow peacefully with a moderate current",
    "Tranquil waters stretch across the aquatic landscape",
    "Steady waters provide reliable passage for travelers and wildlife",
    "Balanced waters create natural boundaries between different regions",
    "Peaceful waters offer respite from the challenges of travel",
    "Reliable waters form dependable routes through the countryside",
    "Honest waters reveal their depths and currents without deception",
    "Practical waters serve the needs of all who depend upon them",
    "Stable waters create predictable patterns of flow and tide",
    "Modest waters offer simple pleasures to those who appreciate them",
    "Comfortable waters welcome both wildlife and weary travelers",
    "Contemplative waters invite quiet reflection in their mirror surfaces",
    "Gentle waters whisper soft secrets to the listening shores",
    "Serene waters embody the perfect balance between motion and rest",
    "Graceful waters flow with the unhurried dignity of natural rivers",
    "Noble waters carry themselves with quiet authority through the landscape",
    "Dignified waters maintain their composure through all seasons",
    "Thoughtful waters pause in peaceful pools between their journeys",
    "Companionable waters provide faithful service to all who approach",
    "Trustworthy waters flow with consistent reliability year after year",
    "Harmonious waters sing gentle songs of contentment and flow"
};

static const char *water_sparse[] = {
    "The murky waters show little sign of life",
    "Dark, stagnant waters reflect the gloomy atmosphere",
    "Shallow, clouded waters reveal a struggling ecosystem",
    "Troubled waters hint at deeper environmental challenges",
    "Stagnant waters create pools of uncertainty and concern",
    "Clouded waters obscure whatever lies beneath their murky surface",
    "Sluggish waters move reluctantly through their channels",
    "Turbid waters reflect the stress placed upon this ecosystem",
    "Brackish waters taste of salt and disappointment",
    "Murky waters hide their secrets beneath layers of sediment",
    "Tainted waters serve as warnings of environmental distress",
    "Polluted waters struggle to support even the hardiest life forms",
    "Mysterious waters conceal ancient secrets in their shadowy depths",
    "Gothic waters create haunting reflections of the brooding sky",
    "Dramatic waters flow with the intensity of liquid shadows",
    "Tempestuous waters roil with dark energy and hidden power",
    "Brooding waters mirror the storm clouds gathering overhead",
    "Melancholic waters sing mournful songs of solitude and depth",
    "Romantic waters evoke tales of lost lovers and forgotten dreams",
    "Phantom waters seem to flow between reality and imagination",
    "Spectral waters glow with an eerie, otherworldly luminescence",
    "Twilight waters capture the essence of approaching night in their flow"
};

/* Underwater terrain descriptions */
static const char *underwater_abundant[] = {
    "Dense kelp forests and coral formations create an underwater jungle",
    "Vibrant coral reefs teem with colorful marine life",
    "Lush underwater gardens flourish in the crystal-clear depths",
    "Magnificent coral cities rise from the ocean floor like living architecture",
    "Towering kelp forests create underwater cathedrals of swaying green",
    "Brilliant coral gardens showcase every color of the rainbow",
    "Thriving underwater meadows carpet the seabed with diverse plant life",
    "Spectacular coral formations create natural aquariums of incredible beauty",
    "Abundant sea grass beds wave gracefully in the gentle currents",
    "Luxuriant underwater forests provide shelter for countless marine species",
    "Prolific coral ecosystems demonstrate the ocean's incredible creativity",
    "Verdant underwater landscapes rival any terrestrial garden in their beauty"
};

static const char *underwater_moderate[] = {
    "Scattered sea plants and coral provide habitat for marine life",
    "Modest coral formations dot the sandy ocean floor",
    "Gentle underwater currents carry nutrients through the depths",
    "Steady coral outcroppings create reliable landmarks in the depths",
    "Balanced underwater vegetation provides both shelter and open spaces",
    "Practical sea plants form natural boundaries between different zones",
    "Reliable coral formations offer consistent shelter for marine wildlife",
    "Honest underwater terrain reveals its character without pretense",
    "Modest kelp beds sway gently in the predictable currents",
    "Stable underwater gardens maintain their beauty through all seasons",
    "Dependable coral clusters create waypoints for underwater travelers",
    "Comfortable sea grass beds welcome tired swimmers to rest"
};

static const char *underwater_sparse[] = {
    "The barren seafloor stretches into the murky depths",
    "Rocky underwater terrain shows little sign of marine life",
    "The dim underwater landscape appears mostly lifeless",
    "Desolate underwater plains stretch endlessly across the ocean floor",
    "Stark rocky formations jut from the barren seabed like ancient monuments",
    "Empty underwater valleys echo with the absence of marine activity",
    "Harsh underwater terrain challenges even the hardiest sea creatures",
    "Austere rocky outcroppings create underwater badlands of stone and sand",
    "Severe underwater conditions test the limits of marine survival",
    "Unforgiving depths offer little sustenance to passing sea life",
    "Demanding underwater terrain requires special adaptation to survive",
    "Relentless currents scour the seafloor clean of most vegetation"
};

/* Desert terrain descriptions */
static const char *desert_abundant[] = {
    "Magnificent sand dunes ripple like golden waves across the landscape",
    "Towering red sandstone formations create natural sculptures beneath the sky",
    "An oasis of palm trees and clear springs provides sanctuary in the vast desert",
    "Colorful desert wildflowers bloom across the sandy terrain in surprising abundance",
    "Lush desert vegetation showcases nature's adaptation to harsh conditions",
    "Verdant cacti and succulents create gardens of extraordinary resilience",
    "Rich desert soil supports unexpected diversity of specialized plant life",
    "Thriving desert ecosystems demonstrate life's triumph over adversity",
    "Abundant wildlife tracks crisscross the pristine sand between flourishing plants",
    "Desert springs create emerald oases that support remarkable biodiversity",
    "Spectacular rock formations frame valleys filled with hardy desert flora",
    "Magnificent sand patterns tell stories of wind, time, and endurance",
    "Paradisiacal oases shimmer like jewels scattered across golden seas",
    "Symphonic dunes sing ancient melodies in the shifting desert winds",
    "Kaleidoscopic desert blooms paint the landscape in impossible colors",
    "Architectural cacti rise like living sculptures against painted skies",
    "Crystalline springs create liquid mirrors that reflect desert dreams",
    "Tapestried sand tells epic stories written by wind and time",
    "Gilded dunes flow like molten gold beneath the desert sun",
    "Paradoxical gardens flourish in defiance of the arid vastness",
    "Mirage-like beauty makes the impossible seem merely improbable",
    "Alchemical desert transforms sand and sun into living art"
};

static const char *desert_moderate[] = {
    "Rolling sand dunes stretch across the arid landscape",
    "Rocky outcroppings provide shade and shelter in the desert terrain",
    "Scattered cacti and hardy shrubs dot the sandy expanse",
    "Modest desert vegetation shows life's persistence in challenging conditions",
    "Steady rock formations create reliable landmarks in the shifting sands",
    "Balanced desert terrain offers both open spaces and sheltered areas",
    "Working desert landscapes show the practical beauty of survival",
    "Reliable stone outcroppings provide waypoints for desert travelers",
    "Honest desert terrain reveals its character without pretense",
    "Straightforward sandy paths wind between clusters of desert plants",
    "Practical desert formations offer shelter when the winds grow strong",
    "Stable rock foundations anchor this corner of the shifting desert",
    "Contemplative desert spaces invite philosophical reflection",
    "Zen-like sand gardens create patterns of natural meditation",
    "Honest desert beauty speaks truth through simplicity",
    "Monastic desert solitude offers respite from worldly concerns",
    "Dignified desert terrain maintains its composure through all weather",
    "Noble sand formations stand as testimonies to quiet endurance",
    "Meditative desert pathways lead to inner peace and clarity",
    "Serene desert valleys cradle travelers in protective embrace",
    "Philosophical desert landscapes inspire deep contemplation",
    "Spiritual desert sanctuaries connect earth-bound souls to the infinite"
};

static const char *desert_sparse[] = {
    "Endless expanses of barren sand stretch to the horizon",
    "Harsh desert winds sculpt the dunes into stark geometric patterns",
    "The unforgiving desert offers little shelter from sun or storm",
    "Bleached bones and weathered rocks mark this desolate wasteland",
    "Scorching sands reflect heat in shimmering waves of distortion",
    "Merciless desert sun beats down on the empty, windswept terrain",
    "Severe sandstorms have scoured this landscape clean of vegetation",
    "Austere desert vistas stretch endlessly under the punishing sky",
    "Demanding desert conditions test the limits of survival",
    "Unforgiving sands shift constantly, erasing all traces of passage",
    "Relentless desert heat creates mirages that mock weary travelers",
    "Stark desert beauty lies in its very emptiness and challenge",
    "Apocalyptic desert vistas stretch beyond the edge of imagination",
    "Gothic sand castles rise like monuments to primordial forces",
    "Surreal desert mirages blur the boundaries between dream and reality",
    "Alien desert landscapes seem plucked from distant worlds",
    "Mystical desert emptiness resonates with cosmic significance",
    "Transcendent desert silence speaks louder than any earthly sound",
    "Infinite desert horizons merge earth and sky in seamless unity",
    "Ethereal desert light transforms barren sand into liquid gold",
    "Otherworldly desert beauty haunts the soul with its austere magnificence",
    "Primal desert forces speak in languages older than civilization"
};

/* Mountain terrain descriptions */
static const char *mountain_abundant[] = {
    "Majestic peaks rise toward the clouds, their slopes covered with alpine forests",
    "Towering mountain faces create dramatic backdrops for lush valley ecosystems",
    "Alpine meadows burst with wildflowers between towering granite summits",
    "Mountain streams cascade down rock faces, nourishing rich mountain vegetation",
    "Pristine mountain lakes reflect snow-capped peaks surrounded by verdant slopes",
    "Dense mountain forests climb impossible gradients toward the sky",
    "Magnificent waterfalls thunder down from heights lost in the clouds",
    "Mountain valleys create hidden worlds of extraordinary natural beauty",
    "Soaring peaks frame ecosystems that change dramatically with elevation",
    "Crystal mountain air carries the scent of pine and distant snow",
    "Mountain ranges create natural amphitheaters of stone and growing things",
    "Spectacular alpine vistas showcase nature's grandest architectural achievements",
    "Himalayan-scale peaks pierce the very fabric of heaven itself",
    "Titanic mountain walls rise like the vertebrae of sleeping gods",
    "Cathedral peaks create natural sanctuaries where earth touches sky",
    "Olympian summits reign over kingdoms of stone and snow",
    "Epic mountain ranges tell geological stories spanning millions of years",
    "Colossal peaks stand as eternal monuments to the earth's creative power",
    "Divine mountain vistas inspire religious awe in mortal hearts",
    "Celestial peaks seem to bridge the gap between earth and paradise",
    "Immortal mountain faces have witnessed the rise and fall of civilizations",
    "Sublime alpine beauty transcends human understanding and description"
};

static const char *mountain_moderate[] = {
    "Rocky peaks rise steadily from the surrounding landscape",
    "Mountain slopes show patches of hardy vegetation between stone outcroppings",
    "Modest alpine terrain offers both challenge and beauty to travelers",
    "Steady mountain paths wind between reliable stone landmarks",
    "Balanced mountain terrain provides both shelter and exposure",
    "Working mountain slopes show evidence of careful adaptation to harsh conditions",
    "Reliable mountain formations create dependable navigation points",
    "Honest mountain terrain reveals its character in every weathered stone",
    "Straightforward mountain paths offer clear routes through challenging territory",
    "Practical mountain features provide shelter and vantage points as needed",
    "Stable mountain foundations anchor this dramatic corner of the landscape",
    "Mountain steadiness provides comfort in an otherwise changing world"
};

static const char *mountain_sparse[] = {
    "Stark mountain peaks thrust like spears into the empty sky",
    "Barren rock faces show the scars of ancient geological violence",
    "Harsh mountain terrain challenges even the most experienced climbers",
    "Windswept peaks offer no mercy to those caught in mountain storms",
    "Severe mountain conditions test the limits of endurance and preparation",
    "Austere mountain landscapes speak of forces beyond human comprehension",
    "Demanding mountain terrain requires respect and careful navigation",
    "Unforgiving mountain weather can change from calm to deadly in moments",
    "Relentless mountain winds carve strange patterns in the ancient stone",
    "Stark mountain beauty lies in its indifference to human concerns",
    "Forbidding mountain faces guard their secrets behind walls of stone",
    "Merciless mountain conditions humble all who dare to challenge them"
};

/* Marshland/Swamp terrain descriptions */
static const char *marsh_abundant[] = {
    "Lush wetlands teem with diverse plant and animal life",
    "Towering cypress trees rise from rich, dark waters beneath trailing moss",
    "Vibrant marshland creates a complex ecosystem of water, earth, and vegetation",
    "Magnificent wetland habitats support an incredible diversity of species",
    "Abundant marsh grasses create natural highways for wildlife movement",
    "Thriving swampland demonstrates nature's ability to create beauty from wetness",
    "Rich wetland soil supports towering trees and delicate aquatic flowers",
    "Verdant marsh vegetation creates natural filters for the flowing waters",
    "Prolific wetland ecosystems showcase the importance of water in all life",
    "Spectacular marsh vistas change with every shift of light and shadow",
    "Luxuriant swamp vegetation creates natural cathedrals of green and brown",
    "Bountiful marshlands provide crucial habitat for countless specialized species"
};

static const char *marsh_moderate[] = {
    "Wetland areas create natural boundaries between water and dry land",
    "Marsh grasses and reeds provide habitat for a variety of wildlife",
    "Modest wetland terrain offers both challenges and opportunities",
    "Steady marsh vegetation creates reliable cover for nesting waterfowl",
    "Balanced wetland ecosystems demonstrate the harmony of water and earth",
    "Working marshlands show the practical value of these transitional spaces",
    "Reliable wetland features provide consistent resources for local wildlife",
    "Honest marsh terrain reveals the truth about water's patient, shaping power",
    "Straightforward wetland paths wind between islands of solid ground",
    "Practical marsh features offer both concealment and clear passage",
    "Stable wetland foundations support this unique ecosystem between worlds",
    "Dependable marsh resources sustain both permanent and visiting wildlife"
};

static const char *marsh_sparse[] = {
    "Struggling wetlands show signs of drought and environmental stress",
    "Sparse marsh vegetation barely holds the eroding soil together",
    "Harsh wetland conditions challenge even specially adapted species",
    "Stagnant marsh waters reflect a gloomy and uncertain ecosystem",
    "Severe marshland degradation threatens the stability of local wildlife",
    "Austere wetland terrain offers little comfort to passing travelers",
    "Demanding marsh conditions require special knowledge to navigate safely",
    "Unforgiving wetland terrain can trap the unwary in hidden mud or water",
    "Relentless marsh decay creates an atmosphere of decline and uncertainty",
    "Stark wetland beauty lies in its reminder of ecological fragility",
    "Forbidding marsh terrain guards its secrets behind walls of mist and mud",
    "Troubled marshlands serve as warnings about environmental vulnerability"
};

/* Road terrain descriptions */
static const char *road_abundant[] = {
    "Well-maintained stone roads wind gracefully through flourishing countryside",
    "Magnificent paved highways create corridors through abundant landscapes",
    "Tree-lined avenues provide shaded pathways between thriving communities",
    "Royal roads showcase the prosperity and care of well-governed lands",
    "Cobblestone streets reflect the wealth and attention of prosperous regions",
    "Grand thoroughfares demonstrate the engineering skills of advanced civilizations",
    "Beautiful roads enhance rather than detract from the natural landscape",
    "Carefully planned roadways work in harmony with the surrounding environment",
    "Prosperous trade routes show evidence of regular use and careful maintenance",
    "Excellent roads provide safe and efficient travel through rich territories",
    "Impressive roadworks create lasting monuments to skilled engineering",
    "Magnificent highways serve as arteries connecting thriving communities",
    "Gilded pathways seem paved with liquid sunlight and golden dreams",
    "Enchanted roads beckon travelers toward distant adventures and romance",
    "Silk-smooth highways flow like ribbons through pastoral paradise",
    "Jeweled thoroughfares sparkle with embedded gems and precious stones",
    "Musical roads sing beneath the wheels of passing carriages",
    "Poetic pathways inspire verses with every footstep and hoofbeat",
    "Celestial avenues seem designed by angels for mortal convenience",
    "Dreamlike roads blur the boundary between journey and destination",
    "Mystical highways carry whispers of ancient magic and wonder",
    "Paradisiacal paths transform mere travel into spiritual pilgrimage"
};

static const char *road_moderate[] = {
    "Serviceable roads provide reliable passage through the countryside",
    "Well-traveled paths show evidence of regular use and basic maintenance",
    "Practical roadways offer straightforward routes between destinations",
    "Steady roads create dependable connections across varied terrain",
    "Balanced road construction works adequately with the surrounding landscape",
    "Working roads serve the practical needs of travelers and merchants",
    "Reliable pathways provide consistent routes through changing seasons",
    "Honest roads reveal their purpose without unnecessary ornamentation",
    "Straightforward routes offer clear directions to intended destinations",
    "Functional roads demonstrate practical engineering adapted to local conditions",
    "Stable roadways provide predictable travel times and conditions",
    "Dependable paths create the backbone of regional transportation networks",
    "Companionable roads offer friendship to the frequent traveler",
    "Welcoming pathways embrace visitors with familiar comfort",
    "Trustworthy highways earn loyalty through consistent performance",
    "Humble roads serve faithfully without seeking recognition",
    "Generous pathways accommodate all manner of travelers and vehicles",
    "Patient roads endure through seasons of heavy use and neglect",
    "Sensible highways demonstrate the wisdom of practical design",
    "Neighborly roads connect communities with bonds of mutual benefit",
    "Hospitable paths offer respite at regular intervals",
    "Steadfast roadways remain faithful through fair weather and storm"
};

static const char *road_sparse[] = {
    "Deteriorating roads show signs of neglect and abandonment",
    "Crumbling pathways tell stories of better times and lost prosperity",
    "Harsh road conditions challenge vehicles and travelers alike",
    "Poorly maintained routes require careful navigation to avoid hazards",
    "Severe road decay reflects broader economic and social decline",
    "Austere travel conditions offer little comfort to weary wayfarers",
    "Demanding road terrain tests both equipment and determination",
    "Unforgiving pathway conditions can strand the unprepared",
    "Relentless weathering gradually reclaims abandoned roadworks",
    "Stark road remnants serve as monuments to vanished civilizations",
    "Forbidding travel conditions warn of dangers ahead",
    "Troubled roadways reflect the instability of the lands they traverse",
    "Haunting roads whisper tales of travelers who never returned",
    "Gothic pathways wind through landscapes touched by shadow",
    "Romantic ruins of ancient highways speak of love and loss",
    "Melancholic routes carry the weight of forgotten journeys",
    "Dramatic roadways challenge the spirit as much as the body",
    "Mysterious paths lead to destinations unknown and unknowable",
    "Phantom highways seem to exist between reality and dream",
    "Spectral roads shimmer with memories of vanished civilizations",
    "Otherworldly pathways transport travelers to realms beyond",
    "Twilight roads blur the boundary between past and present"
};

/* ===== ADDITIONAL DETAIL TEMPLATES ===== */

/* Geological detail templates for mineral-rich areas */
static const char *mineral_details_abundant[] = {
    ", where veins of precious metals glint between layers of rich stone",
    ", their foundations shot through with seams of valuable ore",
    ", where exposed mineral deposits create natural works of art",
    ", with crystalline formations that catch and scatter light",
    ", where golden threads of metal weave through dark rock",
    ", their surfaces decorated with natural mineral patterns",
    ", where gemstone deposits sparkle in hidden crevices",
    ", with copper and silver veins creating metallic tapestries",
    ", where rare earth elements paint the stone in subtle hues",
    ", their rocky hearts revealing treasures formed over millennia"
};

static const char *mineral_details_moderate[] = {
    ", with occasional metallic glints visible in the stone",
    ", where small mineral deposits hint at greater treasures",
    ", their foundations showing traces of valuable ore",
    ", with modest gem formations scattered throughout",
    ", where careful examination reveals useful metals",
    ", their surfaces marked by natural mineral patterns",
    ", with practical stone suitable for building and tools",
    ", where mineral content provides useful raw materials",
    ", their rocky structure enriched by modest ore deposits",
    ", where industrious folk might find valuable minerals"
};

static const char *mineral_details_sparse[] = {
    ", their stone worn smooth by time and weather",
    ", where only the hardest rocks remain after long erosion",
    ", their foundations stripped clean by natural forces",
    ", with little of value visible in the weathered stone",
    ", where mineral wealth has long since been claimed or lost",
    ", their surfaces scoured by wind and rain",
    ", with only the most common stones remaining",
    ", where erosion has carried away softer deposits",
    ", their rocky bones laid bare by persistent weathering",
    ", where poverty of stone reflects larger challenges"
};

/* Water feature detail templates */
static const char *water_details_abundant[] = {
    " while crystal streams dance between moss-covered stones",
    " as clear springs bubble up from the rich earth below",
    " where mountain streams cascade in miniature waterfalls",
    " while hidden brooks sing soft songs through the vegetation",
    " as pristine pools reflect the beauty of their surroundings",
    " where natural springs create oases of refreshment",
    " while flowing waters carve graceful patterns through the landscape",
    " as clear rivulets trace silver pathways across the terrain",
    " where abundant water sources support all manner of life",
    " while pure streams provide the foundation for ecosystem health"
};

static const char *water_details_moderate[] = {
    " while a modest stream provides necessary water for local wildlife",
    " as small pools gather in natural depressions",
    " where seasonal streams flow during wetter periods",
    " while reliable water sources meet the basic needs of the area",
    " as gentle brooks wind their way through the landscape",
    " where natural drainage creates practical water features",
    " while steady flows maintain adequate moisture levels",
    " as dependable springs support local plant communities",
    " where water resources prove sufficient for current needs",
    " while modest flows create important habitat for wildlife"
};

static const char *water_details_sparse[] = {
    " while dry creek beds remember more abundant times",
    " as occasional pools collect precious rainwater",
    " where water scarcity creates challenges for all life",
    " while dried springs hint at more prosperous past seasons",
    " as rare water sources become precious beyond measure",
    " where drought conditions test the resilience of local species",
    " while absent streams leave only memories in carved channels",
    " as water becomes the limiting factor for all growth",
    " where every drop of moisture gains critical importance",
    " while arid conditions challenge traditional ways of life"
};

/* Atmospheric detail templates for time and weather */
static const char *atmosphere_dawn[] = {
    ". The first light of dawn paints everything in soft pastels",
    ". Morning mist creates an atmosphere of quiet mystery",
    ". Dawn's gentle light reveals the peaceful details of this place",
    ". The early morning air carries the promise of a new day",
    ". Sunrise transforms ordinary features into things of beauty",
    ". The quiet of dawn makes every sound seem significant",
    ". Morning's fresh beginning energizes the entire landscape",
    ". Dawn's soft illumination creates a sense of hope and renewal"
};

static const char *atmosphere_day[] = {
    ". Bright daylight reveals every detail with crystal clarity",
    ". The midday sun creates sharp patterns of light and shadow",
    ". Full daylight showcases the true character of this terrain",
    ". Under the open sky, the landscape displays its honest nature",
    ". Daylight hours bring the bustle of active wildlife",
    ". The sun's warmth encourages growth and movement",
    ". Clear daylight makes navigation and exploration straightforward",
    ". The active day showcases this area at its most vibrant"
};

static const char *atmosphere_dusk[] = {
    ". Evening light softens harsh edges with golden warmth",
    ". Dusk creates an atmosphere of peaceful contemplation",
    ". The approaching night brings a sense of mystery and calm",
    ". Evening shadows add depth and character to familiar features",
    ". Twilight transforms the ordinary into something magical",
    ". The day's end brings a reflective quality to the landscape",
    ". Dusk's gentle transition prepares the world for night's rest",
    ". Evening's quiet beauty offers perfect moments for reflection"
};

static const char *atmosphere_night[] = {
    ". The darkness creates an entirely different world of sensation",
    ". Night sounds replace daytime activity with their own symphony",
    ". Moonlight and starlight provide mysterious illumination",
    ". The night air carries scents and sounds more clearly",
    ". Darkness brings out the nocturnal aspects of this place",
    ". Night's quiet allows subtle details to become prominent",
    ". The peaceful darkness offers rest from daytime concerns",
    ". Nighttime creates an atmosphere of mystery and possibility"
};

/* Elevation-based description templates */
static const char *elevation_sea_level[] = {
    " at sea level where the horizon stretches endlessly",
    " where the land meets the vast expanse of water",
    " in the low-lying coastal realm",
    " where salt air mingles with terrestrial scents",
    " at the edge between land and sea",
    " where tidal influences shape the landscape",
    " in the maritime lowlands",
    " where the water table lies close to the surface",
    " near the level of the eternal waters",
    " where land and ocean exist in delicate balance",
    " in the coastal plains that embrace the sea",
    " where the horizon blurs between earth and water",
    " at the threshold of oceanic vastness",
    " where waves have shaped the nearby shores for eons",
    " in the lowlands touched by maritime influence",
    " where the land slopes gently toward distant waters",
    " at the foundation level of the continent",
    " where coastal breezes carry stories of the sea",
    " in the realm where terrestrial meets aquatic",
    " where the ancient shoreline once carved its mark"
};

static const char *elevation_lowlands[] = {
    " in the gentle lowlands where waters naturally gather",
    " where the terrain slopes gradually toward distant valleys",
    " in the rolling countryside of modest elevation",
    " where streams meander through fertile depressions",
    " in the comfortable lowlands below the hills",
    " where the land spreads in welcoming undulations",
    " in the peaceful lowlands sheltered from highland winds",
    " where rivers have carved their patient paths",
    " in the gentle terrain that cradles life",
    " where the earth lies in natural contentment",
    " in the lowlands blessed with collected waters",
    " where the landscape flows in easy curves",
    " in the fertile depression between higher grounds",
    " where the land gathers nature's bounty",
    " in the lowlands where agriculture flourishes",
    " where the terrain offers gentle walking paths",
    " in the pastoral lowlands of quiet beauty",
    " where the earth forms natural basins of plenty",
    " in the comfortable valleys between the hills",
    " where lowland breezes carry the scents of rich earth"
};

static const char *elevation_hills[] = {
    " among the rolling hills that define the horizon",
    " where gentle slopes create a landscape of curves",
    " in the undulating terrain of modest peaks",
    " where hilltops offer commanding views of the surroundings",
    " among the hills that rise like sleeping giants",
    " where ridgelines trace patterns against the sky",
    " in the hilly country of varied elevations",
    " where slopes provide natural vantage points",
    " among the rounded summits that dot the landscape",
    " where the terrain rises and falls in gentle waves",
    " in the hill country where every turn reveals new vistas",
    " where ancient upheavals created these modest heights",
    " among the hills that shelter hidden valleys",
    " where the land reaches toward the heavens",
    " in the undulating highlands of pastoral beauty",
    " where hillsides provide natural amphitheaters",
    " among the elevated terrain that commands respect",
    " where the rolling topography creates endless variety",
    " in the hilly realm between plains and mountains",
    " where every summit tells a story of geological time"
};

static const char *elevation_mountains[] = {
    " high in the mountains where the air grows thin",
    " among the towering peaks that scrape the sky",
    " in the alpine realm where eagles soar",
    " where mountain winds carry the voice of the heights",
    " among the ancient peaks that have stood for millennia",
    " in the rarified atmosphere of the high country",
    " where the mountains rise in majestic grandeur",
    " among the snow-capped summits that pierce the clouds",
    " in the elevated realm where storms are born",
    " where the earth reaches its loftiest expressions",
    " among the mountain fastnesses far above the valleys",
    " in the alpine wilderness where silence reigns",
    " where the peaks form a natural cathedral of stone",
    " among the mountains that dwarf all other features",
    " in the high country where weather is made",
    " where the mountains stand as monuments to time",
    " among the peaks that have witnessed the ages",
    " in the elevated realm of rock and sky",
    " where the mountains create their own weather patterns",
    " among the heights where only the hardy survive"
};

static const char *elevation_peaks[] = {
    " at the summit where earth meets heaven",
    " on the highest peaks above the world below",
    " at the pinnacle where the air is crystal clear",
    " on the towering heights that command all directions",
    " at the apex where storms gather and break",
    " on the summit where the horizon curves away",
    " at the peak where the world spreads out below",
    " on the highest point where eagles fear to venture",
    " at the crown of the world where winds are born",
    " on the summit where clouds form and dissipate",
    " at the ultimate height where few dare to tread",
    " on the peak where the atmosphere grows ethereal",
    " at the summit where the curvature of earth is visible",
    " on the highest point where weather originates",
    " at the pinnacle where the terrestrial meets the celestial",
    " on the peak where the view encompasses vast distances",
    " at the summit where the rarified air whispers secrets",
    " on the highest ground where solitude is absolute",
    " at the apex where the world's roof touches the sky",
    " on the ultimate peak where mortals glimpse eternity"
};

/* ===== TERRAIN TEMPLATE ARRAYS ===== */

/* Base terrain descriptions by vegetation level */
/* ===== MAIN DESCRIPTION GENERATION ===== */

/* Forward declaration */
void add_elevation_details(char *desc, struct environmental_context *context);

char *generate_resource_aware_description(struct char_data *ch, room_rnum room)
{
    static char description[MAX_STRING_LENGTH];
    struct resource_state state;
    struct environmental_context context;
    char *base_desc;
    
    log("DEBUG: generate_resource_aware_description called for room %d", GET_ROOM_VNUM(room));
    
    if (!ch || room == NOWHERE) {
        log("DEBUG: Invalid parameters - ch=%p, room=%d", ch, room);
        return NULL;
    }
    
    /* Get current resource state and environmental context */
    get_resource_state(room, &state);
    get_environmental_context(room, &context);
    
    log("DEBUG: Got resource state and environmental context");
    
    /* Generate base terrain description */
    base_desc = get_terrain_base_description(room, &state, &context);
    if (!base_desc) {
        log("DEBUG: get_terrain_base_description returned NULL");
        return NULL;
    }
    
    log("DEBUG: Base description generated: %.100s...", base_desc);
    
    /* Initialize description buffer safely */
    description[0] = '\0';
    strncat(description, base_desc, MAX_STRING_LENGTH - 1);
    
    /* Add layered details - avoid redundant water descriptions for water terrains */
    if (context.terrain_type != SECT_WATER_SWIM && 
        context.terrain_type != SECT_WATER_NOSWIM && 
        context.terrain_type != SECT_OCEAN && 
        context.terrain_type != SECT_UNDERWATER) {
        
        add_vegetation_details(description, &state, &context);
        add_geological_details(description, &state, &context);
        add_water_features(description, &state, &context);
        add_elevation_details(description, &context);
        add_temporal_atmosphere(description, &context);
        add_wildlife_presence(description, &state, &context);
    } else {
        /* Water terrains: add aquatic vegetation and geological features, skip redundant water descriptions */
        add_vegetation_details(description, &state, &context);  /* Aquatic plants, seaweed, coral */
        add_geological_details(description, &state, &context); /* Underwater rocks, seafloor */
        /* Skip add_water_features() to avoid "waters move with currents" redundancy */
        add_elevation_details(description, &context);  /* Elevation context still relevant for depth/underwater features */
        add_temporal_atmosphere(description, &context);
        add_wildlife_presence(description, &state, &context);  /* Aquatic wildlife */
    }
    
    /* Ensure proper ending */
    int len = strlen(description);
    if (len > 0 && description[len - 1] != '.') {
        if (len < MAX_STRING_LENGTH - 2) {
            strncat(description, ".", MAX_STRING_LENGTH - len - 1);
        }
    }
    len = strlen(description);
    if (len < MAX_STRING_LENGTH - 3) {
        strncat(description, "\r\n", MAX_STRING_LENGTH - len - 1);
    }
    
    return strdup(description);
}

/* ===== RESOURCE STATE FUNCTIONS ===== */

void get_resource_state(room_rnum room, struct resource_state *state)
{
    int x, y;
    
    if (!state || room == NOWHERE) return;
    
    /* Get coordinates for resource calculation */
    x = world[room].coords[0];
    y = world[room].coords[1];
    
    /* Get base resource levels and apply depletion from harvesting */
    float base_vegetation = calculate_current_resource_level(RESOURCE_VEGETATION, x, y);
    float base_minerals = calculate_current_resource_level(RESOURCE_MINERALS, x, y);
    float base_water = calculate_current_resource_level(RESOURCE_WATER, x, y);
    float base_herbs = calculate_current_resource_level(RESOURCE_HERBS, x, y);
    float base_game = calculate_current_resource_level(RESOURCE_GAME, x, y);
    float base_wood = calculate_current_resource_level(RESOURCE_WOOD, x, y);
    float base_stone = calculate_current_resource_level(RESOURCE_STONE, x, y);
    float base_clay = calculate_current_resource_level(RESOURCE_CLAY, x, y);
    float base_salt = calculate_current_resource_level(RESOURCE_SALT, x, y);
    
    /* Apply depletion levels from harvesting to get actual current state */
    float depletion_vegetation = get_resource_depletion_level(room, RESOURCE_VEGETATION);
    float depletion_minerals = get_resource_depletion_level(room, RESOURCE_MINERALS);
    float depletion_water = get_resource_depletion_level(room, RESOURCE_WATER);
    float depletion_herbs = get_resource_depletion_level(room, RESOURCE_HERBS);
    float depletion_game = get_resource_depletion_level(room, RESOURCE_GAME);
    float depletion_wood = get_resource_depletion_level(room, RESOURCE_WOOD);
    float depletion_stone = get_resource_depletion_level(room, RESOURCE_STONE);
    float depletion_clay = get_resource_depletion_level(room, RESOURCE_CLAY);
    float depletion_salt = get_resource_depletion_level(room, RESOURCE_SALT);
    
    /* Calculate final effective resource levels (base * depletion) */
    state->vegetation_level = base_vegetation * depletion_vegetation;
    state->mineral_level = base_minerals * depletion_minerals;
    state->water_level = base_water * depletion_water;
    state->herb_level = base_herbs * depletion_herbs;
    state->game_level = base_game * depletion_game;
    state->wood_level = base_wood * depletion_wood;
    state->stone_level = base_stone * depletion_stone;
    state->clay_level = base_clay * depletion_clay;
    state->salt_level = base_salt * depletion_salt;
}

/* Calculate total light level including room lights, player/mob equipment */
int calculate_total_light_level(room_rnum room) {
    int total_light = 0;
    struct char_data *ch;
    struct obj_data *obj;
    int wear_pos;
    
    if (room == NOWHERE) return 0;
    
    /* Base room light level */
    total_light = world[room].light;
    
    /* Add natural light based on time of day and outdoor conditions */
    if (ROOM_OUTDOORS(room)) {
        switch(weather_info.sunlight) {
            case SUN_LIGHT:
                total_light += 100; /* Full daylight for outdoor rooms */
                break;
            case SUN_RISE:
            case SUN_SET:
                total_light += 50; /* Dawn/dusk lighting for outdoor rooms */
                break;
            case SUN_DARK:
                /* Night time - no natural light */
                break;
        }
    }
    
    /* Check all characters in room for light sources */
    for (ch = world[room].people; ch; ch = ch->next_in_room) {
        /* Check worn equipment for light sources */
        for (wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
            obj = GET_EQ(ch, wear_pos);
            if (obj && GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
                total_light += GET_OBJ_VAL(obj, 2); /* Add light value */
            }
        }
        
        /* Check inventory for active light sources */
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
                total_light += GET_OBJ_VAL(obj, 2);
            }
        }
    }
    
    /* Check room contents for light sources */
    for (obj = world[room].contents; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
            total_light += GET_OBJ_VAL(obj, 2);
        }
    }
    
    return total_light;
}

/* Calculate artificial light level from non-natural sources */
int calculate_artificial_light_level(room_rnum room) {
    int artificial_light = 0;
    struct char_data *ch;
    struct obj_data *obj;
    int wear_pos;
    
    if (room == NOWHERE) return 0;
    
    /* Base room light level (includes magical room lighting) */
    artificial_light = world[room].light;
    
    /* Check all characters in room for light sources */
    for (ch = world[room].people; ch; ch = ch->next_in_room) {
        /* Check worn equipment for light sources */
        for (wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
            obj = GET_EQ(ch, wear_pos);
            if (obj && GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
                artificial_light += GET_OBJ_VAL(obj, 2);
            }
        }
        
        /* Check inventory for active light sources */
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
                artificial_light += GET_OBJ_VAL(obj, 2);
            }
        }
    }
    
    /* Check room contents for light sources */
    for (obj = world[room].contents; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
            artificial_light += GET_OBJ_VAL(obj, 2);
        }
    }
    
    return artificial_light;
}

/* Calculate natural light level from sun/moon only */
int calculate_natural_light_level(room_rnum room) {
    int natural_light = 0;
    
    if (room == NOWHERE) return 0;
    
    /* Add natural light based on time of day and outdoor conditions */
    if (ROOM_OUTDOORS(room)) {
        switch(weather_info.sunlight) {
            case SUN_LIGHT:
                natural_light = 100; /* Full daylight */
                break;
            case SUN_RISE:
            case SUN_SET:
                natural_light = 50; /* Dawn/dusk lighting */
                break;
            case SUN_DARK:
                natural_light = 5; /* Moonlight/starlight */
                break;
        }
    }
    
    return natural_light;
}

/* Helper function to categorize wilderness weather values into weather types */
static int categorize_weather(int weather_value)
{
    if (weather_value >= WEATHER_LIGHTNING_MIN) return WEATHER_LIGHTNING;
    if (weather_value >= WEATHER_STORM_MIN) return WEATHER_STORMY;
    if (weather_value >= WEATHER_RAIN_MIN) return WEATHER_RAINY;
    if (weather_value > 127) return WEATHER_CLOUDY;  /* Mid-range values = clouds */
    return WEATHER_CLEAR;  /* Low values = clear weather */
}

void get_environmental_context(room_rnum room, struct environmental_context *context)
{
    int x, y;
    int raw_weather;
    
    if (!context || room == NOWHERE) return;
    
    /* Get wilderness coordinates and use wilderness weather if applicable */
    if (IS_WILDERNESS_VNUM(GET_ROOM_VNUM(room))) {
        x = world[room].coords[0];
        y = world[room].coords[1];
        
        /* Use wilderness-specific weather system */
        raw_weather = get_weather(x, y);
        context->weather = categorize_weather(raw_weather);
    } else {
        /* Non-wilderness room, use global weather (convert to our weather types) */
        switch (weather_info.sky) {
            case SKY_CLOUDLESS:  context->weather = WEATHER_CLEAR; break;
            case SKY_CLOUDY:     context->weather = WEATHER_CLOUDY; break;
            case SKY_RAINING:    context->weather = WEATHER_RAINY; break;
            case SKY_LIGHTNING:  context->weather = WEATHER_LIGHTNING; break;
            default:             context->weather = WEATHER_CLEAR; break;
        }
    }
    
    context->season = get_current_season();
    context->time_of_day = weather_info.sunlight; /* SUN_DARK, SUN_RISE, SUN_LIGHT, SUN_SET */
    
    /* Calculate different types of light levels */
    context->light_level = calculate_total_light_level(room);
    context->artificial_light = calculate_artificial_light_level(room);
    context->natural_light = calculate_natural_light_level(room);
    context->has_light_sources = (context->artificial_light > 0);
    
    context->terrain_type = get_terrain_type(room);
    
    /* Get elevation - check if we're in wilderness first */
    if (IS_WILDERNESS_VNUM(GET_ROOM_VNUM(room))) {
        /* For wilderness rooms, use the comprehensive elevation function */
        int x = world[room].coords[0];
        int y = world[room].coords[1];
        zone_rnum zone = world[room].zone;
        
        if (ZONE_FLAGGED(zone, ZONE_WILDERNESS)) {
            /* Use elevation relative to sea level for realistic descriptions */
            float elevation_meters = get_elevation_relative_sea_level(x, y);
            
            /* Convert to 0.0-1.0 scale for consistency with existing description logic */
            /* Using a reasonable max elevation for scaling (1000m) */
            context->elevation = elevation_meters / 1000.0f;
            
            /* Clamp to ensure we stay within expected range */
            if (context->elevation > 1.0f) context->elevation = 1.0f;
            if (context->elevation < 0.0f) context->elevation = 0.0f;
        } else {
            context->elevation = 0.5f; /* Default for non-wilderness */
        }
    } else {
        /* For indoor/non-wilderness rooms, try to infer elevation from sector type */
        switch (world[room].sector_type) {
            case SECT_MOUNTAIN:
            case SECT_HIGH_MOUNTAIN:
                context->elevation = 0.8f + (float)(rand() % 20) / 100.0f; /* 0.8-1.0 */
                break;
            case SECT_HILLS:
                context->elevation = 0.6f + (float)(rand() % 20) / 100.0f; /* 0.6-0.8 */
                break;
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
            case SECT_OCEAN:
            case SECT_UNDERWATER:
                context->elevation = 0.0f + (float)(rand() % 30) / 100.0f; /* 0.0-0.3 */
                break;
            case SECT_BEACH:
            case SECT_MARSHLAND:
                context->elevation = 0.15f + (float)(rand() % 20) / 100.0f; /* 0.15-0.35 */
                break;
            default:
                context->elevation = 0.4f + (float)(rand() % 20) / 100.0f; /* 0.4-0.6 */
                break;
        }
    }
    
    /* Check for nearby features */
    context->near_water = (world[room].sector_type == SECT_WATER_SWIM ||
                          world[room].sector_type == SECT_WATER_NOSWIM ||
                          world[room].sector_type == SECT_UNDERWATER);
    
    context->in_forest = (world[room].sector_type == SECT_FOREST);
    context->in_mountains = (world[room].sector_type == SECT_MOUNTAIN);
}

/* ===== DESCRIPTION COMPONENT GENERATORS ===== */

/* Safe string concatenation helper */
void safe_strcat(char *dest, const char *src) {
    int dest_len = strlen(dest);
    int remaining = MAX_STRING_LENGTH - dest_len - 1;
    if (remaining > 0) {
        strncat(dest, src, remaining);
    }
}

char *get_terrain_base_description(room_rnum room, struct resource_state *state, 
                                  struct environmental_context *context)
{
    static char base_desc[MAX_STRING_LENGTH];
    const char *tree_type = "oak"; /* Default tree type */
    const char **templates;
    int template_count;
    int terrain_type;
    
    if (room == NOWHERE) return NULL;
    
    terrain_type = world[room].sector_type;
    
    /* Select appropriate templates based on terrain type and vegetation level */
    switch (terrain_type) {
        case SECT_FOREST:
            /* Determine tree type based on season and environment */
            switch (context->season) {
                case SEASON_SPRING:
                case SEASON_SUMMER:
                    tree_type = (context->near_water) ? "willow" : "oak";
                    break;
                case SEASON_AUTUMN:
                    tree_type = "maple";
                    break;
                case SEASON_WINTER:
                    tree_type = "pine";
                    break;
                default:
                    tree_type = "oak";
                    break;
            }
            
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = forest_abundant;
                template_count = sizeof(forest_abundant) / sizeof(forest_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = forest_moderate;
                template_count = sizeof(forest_moderate) / sizeof(forest_moderate[0]);
            } else {
                templates = forest_sparse;
                template_count = sizeof(forest_sparse) / sizeof(forest_sparse[0]);
            }
            break;
            
        case SECT_HILLS:
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = hills_abundant;
                template_count = sizeof(hills_abundant) / sizeof(hills_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = hills_moderate;
                template_count = sizeof(hills_moderate) / sizeof(hills_moderate[0]);
            } else {
                templates = hills_sparse;
                template_count = sizeof(hills_sparse) / sizeof(hills_sparse[0]);
            }
            break;
            
        case SECT_FIELD:
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = plains_abundant;
                template_count = sizeof(plains_abundant) / sizeof(plains_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = plains_moderate;
                template_count = sizeof(plains_moderate) / sizeof(plains_moderate[0]);
            } else {
                templates = plains_sparse;
                template_count = sizeof(plains_sparse) / sizeof(plains_sparse[0]);
            }
            break;
            
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_OCEAN:
            /* Water terrain - use template-based descriptions */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = water_abundant;
                template_count = sizeof(water_abundant) / sizeof(water_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = water_moderate;
                template_count = sizeof(water_moderate) / sizeof(water_moderate[0]);
            } else {
                templates = water_sparse;
                template_count = sizeof(water_sparse) / sizeof(water_sparse[0]);
            }
            break;
            
        case SECT_UNDERWATER:
            /* Underwater terrain - use template-based descriptions */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = underwater_abundant;
                template_count = sizeof(underwater_abundant) / sizeof(underwater_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = underwater_moderate;
                template_count = sizeof(underwater_moderate) / sizeof(underwater_moderate[0]);
            } else {
                templates = underwater_sparse;
                template_count = sizeof(underwater_sparse) / sizeof(underwater_sparse[0]);
            }
            break;
            
        case SECT_BEACH:
            /* Beach terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "The sandy shore is lined with dune grasses and coastal vegetation");
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "The beach shows patches of hardy coastal plants among the sand");
            } else {
                snprintf(base_desc, sizeof(base_desc), "The bare sandy shore meets the water with minimal vegetation");
            }
            return strdup(base_desc);
            
        case SECT_DESERT:
            /* Desert terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = desert_abundant;
                template_count = sizeof(desert_abundant) / sizeof(desert_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = desert_moderate;
                template_count = sizeof(desert_moderate) / sizeof(desert_moderate[0]);
            } else {
                templates = desert_sparse;
                template_count = sizeof(desert_sparse) / sizeof(desert_sparse[0]);
            }
            break;
            
        case SECT_MARSHLAND:
            /* Marshland terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = marsh_abundant;
                template_count = sizeof(marsh_abundant) / sizeof(marsh_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = marsh_moderate;
                template_count = sizeof(marsh_moderate) / sizeof(marsh_moderate[0]);
            } else {
                templates = marsh_sparse;
                template_count = sizeof(marsh_sparse) / sizeof(marsh_sparse[0]);
            }
            break;
            
        case SECT_MOUNTAIN:
        case SECT_HIGH_MOUNTAIN:
            /* Mountain terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = mountain_abundant;
                template_count = sizeof(mountain_abundant) / sizeof(mountain_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = mountain_moderate;
                template_count = sizeof(mountain_moderate) / sizeof(mountain_moderate[0]);
            } else {
                templates = mountain_sparse;
                template_count = sizeof(mountain_sparse) / sizeof(mountain_sparse[0]);
            }
            break;

        case SECT_ROAD_NS:
        case SECT_ROAD_EW:
        case SECT_ROAD_INT:
            /* Road terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = road_abundant;
                template_count = sizeof(road_abundant) / sizeof(road_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = road_moderate;
                template_count = sizeof(road_moderate) / sizeof(road_moderate[0]);
            } else {
                templates = road_sparse;
                template_count = sizeof(road_sparse) / sizeof(road_sparse[0]);
            }
            break;
            
        default:
            /* Fallback to hills descriptions for unknown terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = hills_abundant;
                template_count = sizeof(hills_abundant) / sizeof(hills_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = hills_moderate;
                template_count = sizeof(hills_moderate) / sizeof(hills_moderate[0]);
            } else {
                templates = hills_sparse;
                template_count = sizeof(hills_sparse) / sizeof(hills_sparse[0]);
            }
            break;
    }
    
    /* Choose a random template for variety - avoid light references at night */
    int choice;
    const char *selected_template;
    
    do {
        choice = rand() % template_count;
        selected_template = templates[choice];
        
        /* Skip templates with light references during nighttime */
        if (context->time_of_day == SUN_DARK && 
            (strstr(selected_template, "sunlight") || 
             strstr(selected_template, "golden") ||
             strstr(selected_template, "bright"))) {
            continue; /* Try another template */
        }
        break; /* Template is acceptable */
    } while (1);
    
    /* Format the description based on terrain type */
    if (terrain_type == SECT_FOREST) {
        snprintf(base_desc, sizeof(base_desc), selected_template, tree_type);
    } else {
        /* Non-forest terrain doesn't need tree type formatting */
        snprintf(base_desc, sizeof(base_desc), "%s", selected_template);
    }
    
    return strdup(base_desc);
}

void add_vegetation_details(char *desc, struct resource_state *state, 
                           struct environmental_context *context)
{
    if (!desc || !state || !context) return;
    
    /* Handle aquatic vegetation separately */
    if (context->terrain_type == SECT_WATER_SWIM || 
        context->terrain_type == SECT_WATER_NOSWIM ||
        context->terrain_type == SECT_UNDERWATER ||
        context->terrain_type == SECT_OCEAN) {
        
        /* Add aquatic vegetation based on vegetation levels */
        if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
            if (context->terrain_type == SECT_UNDERWATER) {
                safe_strcat(desc, ". Swaying kelp forests and colorful coral formations create an underwater garden");
            } else if (context->terrain_type == SECT_OCEAN) {
                safe_strcat(desc, ". Patches of floating seaweed drift across the surface");
            } else {
                safe_strcat(desc, ". Aquatic plants and reeds line the water's edge");
            }
        } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
            if (context->terrain_type == SECT_UNDERWATER) {
                safe_strcat(desc, ". Scattered sea plants and small coral formations dot the seafloor");
            } else {
                safe_strcat(desc, ". Sparse aquatic vegetation breaks the water's surface");
            }
        }
        return; /* Done with aquatic vegetation */
    }
    
    /* Add seasonal vegetation details - terrain-aware for appropriate plant types */
    if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
        switch (context->season) {
            case SEASON_SPRING:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", their branches alive with new growth and emerging buds");
                } else {
                    safe_strcat(desc, ", where lush grasses and vibrant wildflowers bloom in abundance");
                }
                break;
            case SEASON_SUMMER:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", their emerald canopy dense with lush foliage");
                } else {
                    safe_strcat(desc, ", where thick carpets of grass wave gently in the breeze");
                }
                break;
            case SEASON_AUTUMN:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", their leaves a brilliant tapestry of gold and crimson");
                } else {
                    safe_strcat(desc, ", where golden grasses and late-season flowers create a warm mosaic");
                }
                break;
            case SEASON_WINTER:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", their bare branches creating intricate patterns against the sky");
                } else {
                    safe_strcat(desc, ", where hardy winter grasses persist despite the cold");
                }
                break;
        }
    } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
        switch (context->season) {
            case SEASON_SPRING:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", showing the first signs of spring's awakening");
                } else {
                    safe_strcat(desc, ", where patches of new grass emerge among scattered wildflowers");
                }
                break;
            case SEASON_SUMMER:
                /* Time and light-aware descriptions */
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    if (context->time_of_day == SUN_DARK) {
                        if (context->has_light_sources) {
                            if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                                safe_strcat(desc, ", their healthy canopy heavy with rain, droplets glistening in the flickering light");
                            } else {
                                safe_strcat(desc, ", their healthy canopy creating dancing shadows in the flickering light");
                            }
                        } else {
                            if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                                safe_strcat(desc, ", their rain-soaked canopy rustling softly in the darkness");
                            } else {
                                safe_strcat(desc, ", their healthy canopy rustling softly in the night breeze");
                            }
                        }
                    } else {
                        if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                            safe_strcat(desc, ", their healthy canopy dripping steadily from the ongoing rain");
                        } else {
                            safe_strcat(desc, ", their healthy canopy providing pleasant shade");
                        }
                    }
                } else {
                    /* Plains/field descriptions */
                    if (context->time_of_day == SUN_DARK) {
                        if (context->has_light_sources) {
                            if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                                safe_strcat(desc, ", the rain-laden grasses and flowers bending under the weight of water in the artificial light");
                            } else {
                                safe_strcat(desc, ", the grasses and flowers swaying gently in the artificial light");
                            }
                        } else {
                            if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                                safe_strcat(desc, ", the vegetation heavy with rainwater rustling quietly in the darkness");
                            } else {
                                safe_strcat(desc, ", the vegetation rustling quietly in the darkness");
                            }
                        }
                    } else {
                        if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                            safe_strcat(desc, ", where grasses and scattered flowers glisten with fresh raindrops");
                        } else {
                            safe_strcat(desc, ", creating a pleasant meadow dotted with colorful blooms");
                        }
                    }
                }
                break;
            case SEASON_AUTUMN:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", touched with the colors of the changing season");
                } else {
                    safe_strcat(desc, ", where autumn grasses turn golden and seed heads catch the wind");
                }
                break;
            case SEASON_WINTER:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", standing quiet and still in winter's embrace");
                } else {
                    safe_strcat(desc, ", where frost-touched grasses create a sparse but resilient ground cover");
                }
                break;
        }
    } else {
        switch (context->season) {
            case SEASON_SPRING:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", where tender new shoots push through the soil");
                } else {
                    safe_strcat(desc, ", where scattered shoots of grass emerge from the earth");
                }
                break;
            case SEASON_SUMMER:
                /* Time and light-aware descriptions */
                if (context->time_of_day == SUN_DARK) {
                    if (context->has_light_sources) {
                        if (context->terrain_type == SECT_FOREST || context->in_forest) {
                            safe_strcat(desc, ", their sparse forms casting twisted shadows in the artificial light");
                        } else {
                            safe_strcat(desc, ", where sparse patches of vegetation are highlighted by the flickering light");
                        }
                    } else {
                        if (context->terrain_type == SECT_FOREST || context->in_forest) {
                            safe_strcat(desc, ", their sparse forms barely visible in the darkness");
                        } else {
                            safe_strcat(desc, ", where scattered vegetation fades into the night");
                        }
                    }
                } else {
                    if (context->terrain_type == SECT_FOREST || context->in_forest) {
                        safe_strcat(desc, ", creating patches of shade in the open landscape");
                    } else {
                        safe_strcat(desc, ", where scattered wildflowers add splashes of color to the grassland");
                    }
                }
                break;
            case SEASON_AUTUMN:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", their sparse foliage rustling in the breeze");
                } else {
                    safe_strcat(desc, ", where dry grasses and fading wildflowers bend in the autumn wind");
                }
                break;
            case SEASON_WINTER:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    safe_strcat(desc, ", stark and beautiful against the winter landscape");
                } else {
                    safe_strcat(desc, ", where only the hardiest grasses survive the winter cold");
                }
                break;
        }
    }
}

void add_geological_details(char *desc, struct resource_state *state, 
                           struct environmental_context *context)
{
    if (!desc || !state || !context) return;
    
    /* Skip geological descriptions for terrain types where minerals don't make sense */
    if (context->terrain_type == SECT_WATER_SWIM || 
        context->terrain_type == SECT_WATER_NOSWIM ||
        context->terrain_type == SECT_UNDERWATER ||
        context->terrain_type == SECT_OCEAN ||
        context->terrain_type == SECT_FLYING) {
        return; /* No geological descriptions for water/flying areas */
    }
    
    /* Use template arrays for variety */
    const char **templates;
    int template_count;
    
    if (state->mineral_level >= RESOURCE_ABUNDANT_THRESHOLD) {
        templates = mineral_details_abundant;
        template_count = sizeof(mineral_details_abundant) / sizeof(mineral_details_abundant[0]);
    } else if (state->mineral_level >= RESOURCE_MODERATE_THRESHOLD) {
        templates = mineral_details_moderate;
        template_count = sizeof(mineral_details_moderate) / sizeof(mineral_details_moderate[0]);
    } else if (state->mineral_level >= RESOURCE_SPARSE_THRESHOLD) {
        templates = mineral_details_sparse;
        template_count = sizeof(mineral_details_sparse) / sizeof(mineral_details_sparse[0]);
    } else {
        return; /* No geological details for extremely low mineral areas */
    }
    
    /* Choose a random template for variety */
    int choice = rand() % template_count;
    safe_strcat(desc, templates[choice]);
}

void add_water_features(char *desc, struct resource_state *state, 
                       struct environmental_context *context)
{
    if (!desc || !state || !context) return;
    
    /* Only add water features if there's significant water present */
    if (state->water_level >= RESOURCE_SPARSE_THRESHOLD) {
        
        /* Random choice for variety - 50% chance to use template arrays */
        int use_template = (rand() % 2);
        
        /* Terrain-appropriate water descriptions */
        if (state->water_level >= RESOURCE_ABUNDANT_THRESHOLD) {
            /* Abundant water - terrain-specific descriptions */
            if (use_template) {
                safe_strcat(desc, water_details_abundant[rand() % 20]);
            } else {
                switch (context->terrain_type) {
                    case SECT_WATER_SWIM:
                    case SECT_WATER_NOSWIM:
                    case SECT_OCEAN:
                        safe_strcat(desc, ". The clear waters reflect the sky above");
                        break;
                    case SECT_UNDERWATER:
                        safe_strcat(desc, ". The underwater currents flow gently through the aquatic environment");
                        break;
                    case SECT_BEACH:
                        safe_strcat(desc, ". Waves wash rhythmically against the sandy shore");
                        break;
                    case SECT_MARSHLAND:
                        safe_strcat(desc, ". Meandering waterways wind through the marshy terrain");
                        break;
                    case SECT_MOUNTAIN:
                        safe_strcat(desc, ". A mountain spring bubbles forth from rocky clefts");
                        break;
                    case SECT_FOREST:
                        safe_strcat(desc, ". A crystal-clear brook winds between moss-covered boulders");
                        break;
                    case SECT_FIELD:
                        safe_strcat(desc, ". A lively creek dances through the meadow over smooth stones");
                        break;
                    case SECT_HILLS:
                        safe_strcat(desc, ". Sparkling streams cascade down the hillsides");
                        break;
                    case SECT_DESERT:
                        safe_strcat(desc, ". A rare oasis provides life-giving water in the arid landscape");
                        break;
                    default:
                        safe_strcat(desc, ". Abundant fresh water flows through the area");
                        break;
                }
            }
        } else if (state->water_level >= RESOURCE_MODERATE_THRESHOLD) {
            /* Moderate water - terrain-specific descriptions */
            if (use_template) {
                safe_strcat(desc, water_details_moderate[rand() % 20]);
            } else {
                switch (context->terrain_type) {
                    case SECT_WATER_SWIM:
                    case SECT_WATER_NOSWIM:
                    case SECT_OCEAN:
                        safe_strcat(desc, ". The waters move with steady currents");
                        break;
                    case SECT_BEACH:
                        safe_strcat(desc, ". Gentle waves lap against the shoreline");
                        break;
                    case SECT_MARSHLAND:
                        safe_strcat(desc, ". Shallow channels weave through the wetland");
                        break;
                    case SECT_MOUNTAIN:
                        safe_strcat(desc, ". A steady mountain stream flows over rocky terrain");
                        break;
                    case SECT_FOREST:
                    case SECT_FIELD:
                    case SECT_HILLS:
                        safe_strcat(desc, ". A steady stream flows over smooth stones");
                        break;
                    case SECT_DESERT:
                        safe_strcat(desc, ". A small spring provides precious water in the dry landscape");
                        break;
                    default:
                        safe_strcat(desc, ". Water flows quietly through the terrain");
                        break;
                }
            }
        } else {
            /* Sparse water - terrain-specific descriptions */
            if (use_template) {
                safe_strcat(desc, water_details_sparse[rand() % 20]);
            } else {
                switch (context->terrain_type) {
                    case SECT_BEACH:
                        safe_strcat(desc, ". Occasional tide pools collect seawater among the rocks");
                        break;
                    case SECT_MARSHLAND:
                        safe_strcat(desc, ". Shallow puddles dot the boggy ground");
                        break;
                    case SECT_MOUNTAIN:
                        safe_strcat(desc, ". A thin trickle of water seeps from cracks in the stone");
                        break;
                    case SECT_DESERT:
                        safe_strcat(desc, ". Rare pools reflect the sky where water briefly collects");
                        break;
                    case SECT_FOREST:
                    case SECT_FIELD:
                    case SECT_HILLS:
                        safe_strcat(desc, ". Small pools reflect the sky where water once flowed freely");
                        break;
                    default:
                        safe_strcat(desc, ". Traces of water hint at hidden springs");
                        break;
                }
            }
        }
    }
}

void add_temporal_atmosphere(char *desc, struct environmental_context *context)
{
    if (!desc || !context) return;
    
    /* Random selection for atmospheric variety */
    int atmosphere_choice = rand() % 3;
    
    /* Add weather atmospheric details first */
    switch (context->weather) {
        case WEATHER_CLEAR:
            /* Clear weather - let time-of-day descriptions dominate, but add occasional atmospheric touches */
            if (atmosphere_choice == 0) {
                /* Use time-appropriate atmosphere arrays for clear weather */
                switch (context->time_of_day) {
                    case SUN_RISE:
                        safe_strcat(desc, atmosphere_dawn[rand() % 20]);
                        break;
                    case SUN_LIGHT:
                        safe_strcat(desc, atmosphere_day[rand() % 20]);
                        break;
                    case SUN_SET:
                        safe_strcat(desc, atmosphere_dusk[rand() % 20]);
                        break;
                    case SUN_DARK:
                        safe_strcat(desc, atmosphere_night[rand() % 20]);
                        break;
                }
            }
            break;
        case WEATHER_CLOUDY:
            if (atmosphere_choice == 0) {
                safe_strcat(desc, " under a canopy of gray clouds");
            } else {
                /* Use atmospheric variety for cloudy weather */
                switch (context->time_of_day) {
                    case SUN_RISE:
                        safe_strcat(desc, atmosphere_dawn[rand() % 20]);
                        break;
                    case SUN_LIGHT:
                        safe_strcat(desc, atmosphere_day[rand() % 20]);
                        break;
                    case SUN_SET:
                        safe_strcat(desc, atmosphere_dusk[rand() % 20]);
                        break;
                    case SUN_DARK:
                        safe_strcat(desc, atmosphere_night[rand() % 20]);
                        break;
                }
            }
            break;
        case WEATHER_RAINY:
            switch (context->time_of_day) {
                case SUN_DARK:
                    if (atmosphere_choice == 0) {
                        safe_strcat(desc, " as gentle rain patters softly in the darkness");
                    } else {
                        safe_strcat(desc, atmosphere_night[rand() % 20]);
                    }
                    break;
                case SUN_RISE:
                case SUN_SET:
                    if (atmosphere_choice == 0) {
                        safe_strcat(desc, " where light rain creates a misty veil over the landscape");
                    } else if (context->time_of_day == SUN_RISE) {
                        safe_strcat(desc, atmosphere_dawn[rand() % 20]);
                    } else {
                        safe_strcat(desc, atmosphere_dusk[rand() % 20]);
                    }
                    break;
                default:
                    if (atmosphere_choice == 0) {
                        safe_strcat(desc, " as steady rain drums against the earth");
                    } else {
                        safe_strcat(desc, atmosphere_day[rand() % 20]);
                    }
                    break;
            }
            break;
        case WEATHER_STORMY:
            if (context->time_of_day == SUN_DARK) {
                if (atmosphere_choice == 0) {
                    safe_strcat(desc, " while heavy rain pounds the ground through the night");
                } else {
                    safe_strcat(desc, atmosphere_night[rand() % 20]);
                }
            } else {
                if (atmosphere_choice == 0) {
                    safe_strcat(desc, " as sheets of rain sweep across the terrain");
                } else {
                    /* Use time-appropriate atmosphere for stormy weather */
                    switch (context->time_of_day) {
                        case SUN_RISE:
                            safe_strcat(desc, atmosphere_dawn[rand() % 20]);
                            break;
                        case SUN_SET:
                            safe_strcat(desc, atmosphere_dusk[rand() % 20]);
                            break;
                        default:
                            safe_strcat(desc, atmosphere_day[rand() % 20]);
                            break;
                    }
                }
            }
            break;
        case WEATHER_LIGHTNING:
            if (context->time_of_day == SUN_DARK) {
                if (atmosphere_choice == 0) {
                    safe_strcat(desc, " as lightning tears through the storm-darkened sky, briefly illuminating the rain-soaked landscape");
                } else {
                    safe_strcat(desc, atmosphere_night[rand() % 20]);
                }
            } else {
                if (atmosphere_choice == 0) {
                    safe_strcat(desc, " where lightning splits the turbulent sky above the storm-lashed terrain");
                } else {
                    /* Use time-appropriate atmosphere for lightning weather */
                    switch (context->time_of_day) {
                        case SUN_RISE:
                            safe_strcat(desc, atmosphere_dawn[rand() % 20]);
                            break;
                        case SUN_SET:
                            safe_strcat(desc, atmosphere_dusk[rand() % 20]);
                            break;
                        default:
                            safe_strcat(desc, atmosphere_day[rand() % 20]);
                            break;
                    }
                }
            }
            break;
    }
    
    /* Add time-of-day atmospheric details using game's actual sunlight values */
    /* Only add time details if weather didn't already provide atmospheric ending */
    if (context->weather == WEATHER_CLEAR || context->weather == WEATHER_CLOUDY) {
        switch (context->time_of_day) {
            case SUN_RISE: /* Hours 5-6 */
                if (context->has_light_sources) {
                    safe_strcat(desc, " as the first light of dawn mingles with the warm glow of torchlight");
                } else {
                    safe_strcat(desc, " as the first light of dawn filters through the landscape");
                }
                break;
            case SUN_LIGHT: /* Hours 6-21 */
                /* More specific descriptions based on likely time periods */
                if (time_info.hours >= 6 && time_info.hours < 10) {
                    safe_strcat(desc, " in the gentle light of morning");
                } else if (time_info.hours >= 10 && time_info.hours < 14) {
                    safe_strcat(desc, " under the bright midday sun");
                } else if (time_info.hours >= 14 && time_info.hours < 18) {
                    safe_strcat(desc, " in the warm afternoon light");
                } else {
                    safe_strcat(desc, " in the fading daylight of evening");
                }
                break;
            case SUN_SET: /* Hours 21-22 */
                if (context->has_light_sources) {
                    safe_strcat(desc, " as twilight deepens and artificial light begins to push back the gathering darkness");
                } else {
                    safe_strcat(desc, " as twilight casts long shadows across the terrain");
                }
                break;
            case SUN_DARK: /* Hours 22-5 */
                if (context->has_light_sources) {
                    if (context->artificial_light >= 50) {
                        safe_strcat(desc, " illuminated by the warm glow of torchlight dancing across the landscape");
                    } else if (context->artificial_light >= 20) {
                        safe_strcat(desc, " where flickering light creates shifting patterns of illumination and shadow");
                    } else {
                        safe_strcat(desc, " where a faint light source barely pierces the encompassing darkness");
                    }
                } else {
                    safe_strcat(desc, " under the pale light of moon and stars");
                }
                break;
        }
    }
}

void add_wildlife_presence(char *desc, struct resource_state *state, 
                          struct environmental_context *context)
{
    if (!desc || !state || !context) return;
    
    /* Check if this is an aquatic environment */
    bool is_aquatic = (context->terrain_type == SECT_WATER_SWIM || 
                       context->terrain_type == SECT_WATER_NOSWIM || 
                       context->terrain_type == SECT_OCEAN || 
                       context->terrain_type == SECT_UNDERWATER);
    
    /* Add wildlife based on vegetation and game levels */
    if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD && 
        state->game_level >= RESOURCE_MODERATE_THRESHOLD) {
        
        if (is_aquatic) {
            /* Aquatic wildlife descriptions */
            switch (context->time_of_day) {
                case SUN_RISE:
                case SUN_SET:
                    if (context->terrain_type == SECT_UNDERWATER) {
                        safe_strcat(desc, ". Shadowy fish move through the changing light");
                    } else {
                        safe_strcat(desc, ". Fish leap from the water's surface in the changing light");
                    }
                    break;
                case SUN_LIGHT:
                    if (time_info.hours >= 6 && time_info.hours < 12) {
                        safe_strcat(desc, ". Schools of fish glimmer beneath the surface");
                    } else if (time_info.hours >= 12 && time_info.hours < 18) {
                        safe_strcat(desc, ". Aquatic life moves gracefully through the depths");
                    } else {
                        safe_strcat(desc, ". Evening feeders stir in the deeper waters");
                    }
                    break;
                case SUN_DARK:
                    safe_strcat(desc, ". Nocturnal marine life becomes active in the darkness");
                    break;
            }
        } else {
            /* Terrestrial wildlife descriptions */
            switch (context->time_of_day) {
                case SUN_RISE:
                case SUN_SET:
                    safe_strcat(desc, ". Small creatures can be heard moving through the underbrush");
                    break;
                case SUN_LIGHT:
                    /* Different wildlife sounds based on time within daylight hours */
                    if (time_info.hours >= 6 && time_info.hours < 12) {
                        safe_strcat(desc, ". Birdsong echoes from the canopy above");
                    } else if (time_info.hours >= 12 && time_info.hours < 18) {
                        safe_strcat(desc, ". The quiet rustle of leaves hints at hidden wildlife");
                    } else {
                        safe_strcat(desc, ". Evening wildlife begins to stir in the shadows");
                    }
                    break;
                case SUN_DARK:
                    safe_strcat(desc, ". Night sounds drift through the darkness");
                    break;
            }
        }
    } else if (state->vegetation_level >= RESOURCE_SPARSE_THRESHOLD) {
        if (is_aquatic) {
            safe_strcat(desc, ". The waters rest in serene tranquility");
        } else {
            safe_strcat(desc, ". The area rests in peaceful solitude");
        }
    }
}

void add_elevation_details(char *desc, struct environmental_context *context)
{
    if (!desc || !context) return;
    
    /* Random chance to add elevation-based description (40% chance) */
    if (rand() % 100 < 40) {
        /* Get actual elevation in meters by converting back from 0.0-1.0 scale */
        float elevation_meters = context->elevation * 1000.0f;
        
        /* Choose elevation template based on actual elevation in meters */
        if (elevation_meters <= 5.0f) {
            /* Sea level and very low elevations (0-5m) */
            safe_strcat(desc, elevation_sea_level[rand() % 20]);
        } else if (elevation_meters <= 50.0f) {
            /* Lowlands (5-50m) */
            safe_strcat(desc, elevation_lowlands[rand() % 20]);
        } else if (elevation_meters <= 200.0f) {
            /* Hills (50-200m) */
            safe_strcat(desc, elevation_hills[rand() % 20]);
        } else if (elevation_meters <= 500.0f) {
            /* Mountains (200-500m) */
            safe_strcat(desc, elevation_mountains[rand() % 20]);
        } else {
            /* High peaks (500m+) */
            safe_strcat(desc, elevation_peaks[rand() % 20]);
        }
    }
}

/* ===== UTILITY FUNCTIONS ===== */

int get_current_season(void)
{
    /* Simple season calculation based on month */
    int month = time_info.month;
    
    if (month >= 2 && month <= 4) return SEASON_SPRING;
    if (month >= 5 && month <= 7) return SEASON_SUMMER;
    if (month >= 8 && month <= 10) return SEASON_AUTUMN;
    return SEASON_WINTER;
}

int get_terrain_type(room_rnum room)
{
    if (room == NOWHERE) return SECT_FIELD;
    return world[room].sector_type;
}

const char *get_resource_abundance_category(float level)
{
    if (level >= RESOURCE_ABUNDANT_THRESHOLD) return "abundant";
    if (level >= RESOURCE_MODERATE_THRESHOLD) return "moderate";
    if (level >= RESOURCE_SPARSE_THRESHOLD) return "sparse";
    return "depleted";
}

#endif /* ENABLE_DYNAMIC_RESOURCE_DESCRIPTIONS */
