## Luminari Pantheon (Original)

This document defines an original, setting-specific pantheon for the Luminari world. It is inspired by classic fantasy portfolios (war, sea, luck, craft, fate, etc.) while remaining fully original in naming, lore, and relationships.

Guidance for implementers:
- Suggested Domains list references existing domain tags in the codebase (e.g., `DOMAIN_WAR`, `DOMAIN_MAGIC`, `DOMAIN_TRAVEL`). Adjust during integration as needed.
- Ethos uses Lawful/Neutral/Chaotic; Alignment uses Good/Neutral/Evil.
- Favored Weapons are descriptive to support different weapon tables.
- “Relationships” provide roleplay and lore hooks and can inform in-game faction reactions.

## Cosmology and Themes

The gods are bound to the Loom of Aether, a metaphysical weave of vows, memories, and elemental currents. Each deity embodies a cadence of the Loom. Clerics channel a thread by devotion, oath, and ritual. When gods clash, the Loom shudders as omens, eclipses, red tides, singing stones, and dream-storms.

### The Three Forces of Creation
The Loom itself is woven from three primordial forces:
- **Memory**: The past that shapes, represented by Aethyra, Nethris, and Kaelthir
- **Will**: The present that acts, represented by Kordran, Thalos, and Erix
- **Possibility**: The future that beckons, represented by Lumerion, Vespera, and Seraphine

### Divine Resonances
Certain deities share "harmonic threads" in the Loom:
- **The Light and Shadow Dyad**: Seraphine and Nyxara are two faces of the same cosmic truth
- **The Forge Trinity**: Borhild, Pyrion, and Erix represent creation through craft
- **The Death Quartet**: Nethris, Glacius, Umbros, and Yurga govern different aspects of endings
- **The Nature Circle**: Viridara, Verdania, Orith, Myrr, and Zorren maintain the natural order

## Quick Index

- Core Deities: Aethyra, Kordran, Seraphine, Nyxara, Thalos, Vaelith, Myrr, Zorren, Lumerion, Erix, Vespera, Nethris, Calystral, Orith
- Dwarven: Borhild, Skarn, Maela, Duerak, Vangar
- Elven: Selithiel, Aerion, Viridara, Lirael, Kaelthir
- Halfling: Pella, Brandoc, Willow
- Orcish: Ghorak, Luthra, Sharguk, Yurga
- Seafolk and Sky: Thalassa, Aelor
- Under-Shadow (Dark Courts): Velara, Zhaerin, Draven

---

## Core Deities

### Aethyra, Lady of the Loom
- **Portfolio**: Magic, oaths, hidden patterns, memory, the Weave itself
- **Ethos / Alignment**: Neutral / Good
- **Suggested Domains**: `DOMAIN_MAGIC`, `DOMAIN_KNOWLEDGE`, `DOMAIN_RUNE`, `DOMAIN_PROTECTION`, `DOMAIN_SPELL`
- **Holy Symbol**: A silver spindle wreathed in seven motes of different colors
- **Favored Weapon**: Quarterstaff (representing the loom's beam)
- **Worshippers**: Mages, oathkeepers, archivists, midwives of lore, contract scribes, memory-keepers
- **Tenets**:
  - Keep vows; an oath broken frays the world and weakens the Loom.
  - Guard memory; unremembered truths become cages for future generations.
  - Weave mercy where power permits, for magic without compassion is mere destruction.
  - Share knowledge freely, but guard the dangerous secrets that could unravel reality.
- **Rites & Holy Days**: The Night of Uncut Thread (new moon) where initiates speak their first vow over a bowl of still water; Scribing Vigils at equinoxes; The Great Remembering (annual ceremony to preserve important historical events).
- **Relationships**: Warily counsels Nyxara on the balance between secrets and knowledge; trusts Thalos in matters of law and record-keeping; often at cross-purposes with Vespera's chaotic magic; works closely with Kaelthir on preserving ancient lore.

### Kordran, Hammer of Dawn
- **Portfolio**: War, duty, resolve, banners, righteous battle, military honor
- **Ethos / Alignment**: Lawful / Good
- **Suggested Domains**: `DOMAIN_WAR`, `DOMAIN_PROTECTION`, `DOMAIN_STRENGTH`, `DOMAIN_LAW`, `DOMAIN_GOOD`
- **Holy Symbol**: Sunlit hammer over a squared shield with a rising sun
- **Favored Weapon**: Warhammer
- **Worshippers**: Knights, wardens, embattled common folk, city guards, military commanders, paladins
- **Tenets**:
  - Hold the line for those behind you; retreat only to save innocent lives.
  - Victory without honor is a wound upon the soul that festers in the afterlife.
  - Keep your word even when dawn scorches, for oaths are the foundation of civilization.
  - Protect the weak, but teach them to protect themselves when possible.
- **Rites & Holy Days**: Shieldbinding at first light before campaign; Day of Banners (midsummer) honoring fallen standards; The Vigil of Shields (winter solstice) where warriors stand guard through the longest night.
- **Relationships**: Mutual respect with Orith for steadfast endurance; friendly rivalry with Zorren over tactics vs. instinct; implacably opposed to Yurga's plague-spreading; works with Seraphine to protect farming communities.

### Seraphine the Dawnstar
- **Portfolio**: Sun, renewal, harvest, redemption, agriculture, second chances
- **Ethos / Alignment**: Neutral / Good
- **Suggested Domains**: `DOMAIN_SUN`, `DOMAIN_RENEWAL`, `DOMAIN_HEALING`, `DOMAIN_GOOD`, `DOMAIN_PLANT`
- **Holy Symbol**: Eight-rayed aureole within a wheat circlet, often rendered in gold and amber
- **Favored Weapon**: Spear (representing the first ray of dawn)
- **Worshippers**: Farmers, healers, reformers, pilgrims, former criminals seeking redemption, midwives
- **Tenets**:
  - Every dawn affords a debt and a chance to repay it; no soul is beyond redemption.
  - Spare the guilty who seek the plow over the blade, for honest work cleanses the spirit.
  - Tend the weak as you would the seedling, with patience and gentle care.
  - Share the harvest freely, for abundance hoarded becomes corruption.
- **Rites & Holy Days**: First-Furrow procession in spring; Emberfast after reap-time with night-long vigils; Dawn Pardons (where communities forgive past wrongs); Harvest Home (autumn celebration of community and plenty).
- **Relationships**: Sister-rival to Nyxara (light vs. shadow, but both necessary); patron-friend of Myrr in healing work; allied with Willow in agricultural matters; opposed to Draven's patient corruption.

### Nyxara of the Veil
- **Portfolio**: Night, secrets, thresholds, lost names, hidden knowledge, necessary darkness
- **Ethos / Alignment**: Neutral / Evil
- **Suggested Domains**: `DOMAIN_DARKNESS`, `DOMAIN_KNOWLEDGE`, `DOMAIN_TRICKERY`, `DOMAIN_CAVERN`, `DOMAIN_PORTAL`
- **Holy Symbol**: Half-mask of polished obsidian with silver tears
- **Favored Weapon**: Curved dagger (representing the crescent moon's shadow)
- **Worshippers**: Spies, secret-keepers, dispossessed, information brokers, those who guard dangerous knowledge, night workers
- **Tenets**:
  - A secret is a coin that spends itself once.
  - In darkness, find your true shape.
  - Doors are promises—decide who enters.
  - What is hidden preserves power; what is revealed loses it.
- **Rites & Holy Days**: The Black Oath on nights without stars; Last-Lamp rite when swearing silence; Threshold Vigils at each solstice.
- **Relationships**: Sister-shadow to Seraphine (they were once one deity, split during the First Dawn); cordial with Vespera over shared trickery; wary alliance with Zhaerin on forbidden knowledge; plots to subvert Aethyra's oaths while secretly preserving the most important ones.

### Thalos, the Scales Unblinking
- **Portfolio**: Judgment, law, scholarship, cities
- **Ethos / Alignment**: Lawful / Neutral
- **Suggested Domains**: `DOMAIN_LAW`, `DOMAIN_KNOWLEDGE`, `DOMAIN_PROTECTION`, `DOMAIN_PLANNING`
- **Holy Symbol**: A blindfolded scale with a rune on each pan
- **Favored Weapon**: Longsword
- **Worshippers**: Magistrates, lorekeepers, diplomats
- **Tenets**:
  - Records bind the living and the dead.
  - Mercy without measure seeds future crimes.
  - Debate until the truth stands unarmed.
- **Rites & Holy Days**: Charter Oaths when founding councils; Census of Grey Ink every tenth year.
- **Relationships**: Disputes Kordran’s crusades; respects Erix’s contracts; opposes Sharguk’s lawless cabals.

### Vaelith, Whispering Tide
- **Portfolio**: Sea, storms, currents, horizons
- **Ethos / Alignment**: Chaotic / Neutral
- **Suggested Domains**: `DOMAIN_OCEAN`, `DOMAIN_STORM`, `DOMAIN_TRAVEL`, `DOMAIN_PROTECTION`
- **Holy Symbol**: Three wave-crests in a spiral shell
- **Favored Weapon**: Trident
- **Worshippers**: Sailors, coastfolk, tide-readers
- **Tenets**:
  - Respect the deep; bargain with the shore.
  - Share water, spare hunger.
  - Storms test hull and heart alike.
- **Rites & Holy Days**: Casting of the Third Knot before voyages; Spiral-Wake memorials after storms.
- **Relationships**: Allies of Aelor; quarrels with Thalassa over shipwreck tithe.

### Myrr, the Quiet Brook
- **Portfolio**: Peace, healing, sanctuaries, small kindnesses
- **Ethos / Alignment**: Neutral / Good
- **Suggested Domains**: `DOMAIN_HEALING`, `DOMAIN_WATER`, `DOMAIN_PROTECTION`, `DOMAIN_FAMILY`
- **Holy Symbol**: A cupped hand brimming with water
- **Favored Weapon**: Quarterstaff
- **Worshippers**: Healers, pacifists, wayhouse-keepers
- **Tenets**:
  - A life saved is a promise to the Loom.
  - End a feud with bread and a story.
  - Never poison a well.
- **Rites & Holy Days**: Waterblessing of new wells; Night of Open Doors during winter’s first frost.
- **Relationships**: Cherished by Seraphine; hunted by Yurga’s cults in time of plague.

### Zorren, Lord of the Wild Hunt
- **Portfolio**: Beasts, moonlit pursuit, primal triumph, freedom, the hunter's bond
- **Ethos / Alignment**: Chaotic / Neutral
- **Suggested Domains**: `DOMAIN_ANIMAL`, `DOMAIN_MOON`, `DOMAIN_STRENGTH`, `DOMAIN_TRAVEL`, `DOMAIN_LIBERATION`
- **Holy Symbol**: A white antler circled by a thin crescent
- **Favored Weapon**: Composite longbow
- **Worshippers**: Rangers, free-companies, druids of the chase, beast-tamers, wilderness guides
- **Tenets**:
  - Hunt to live, not to glut.
  - Give the fallen a song and the earth their bones.
  - Break chains that pen the wind.
  - The pack survives where the lone wolf perishes.
  - Respect the prey that feeds you.
- **Rites & Holy Days**: Moonfeast at first full moon of autumn; Last-Track for the year's final stag; Blood Bond (sharing kills with the pack); Wild Nights (monthly runs under the full moon).
- **Relationships**: Friendly rivalry with Kordran over tactics vs instinct; protective of Viridara's sacred groves; honors Verdania as the source of all prey; despises Ghorak's wasteful destruction; shares moon-mysteries with Selithiel.

### Lumerion, the Lantern-Bearer
- **Portfolio**: Luck, travel, wayfinding, small chances
- **Ethos / Alignment**: Chaotic / Good
- **Suggested Domains**: `DOMAIN_LUCK`, `DOMAIN_TRAVEL`, `DOMAIN_PROTECTION`, `DOMAIN_GOOD`
- **Holy Symbol**: A hanging lantern with a smiling wick-flame
- **Favored Weapon**: Shortsword
- **Worshippers**: Wanderers, messengers, gamblers with good hearts
- **Tenets**:
  - Share the road and the map.
  - Luck is a lantern—carry it for others.
  - Loose coins find empty hands.
- **Rites & Holy Days**: Lanternnight where crossroads are lit and guarded until dawn; Coin-toss blessings before journeys.
- **Relationships**: Companion to Pella; needled by Brandoc’s pranks; despised by Sharguk.

### Erix the Coinwright
- **Portfolio**: Trade, craft, cities, contracts
- **Ethos / Alignment**: Lawful / Neutral
- **Suggested Domains**: `DOMAIN_TRADE`, `DOMAIN_CRAFT`, `DOMAIN_METAL`, `DOMAIN_PLANNING`
- **Holy Symbol**: A square coin stamped with a clasped hand
- **Favored Weapon**: Light hammer
- **Worshippers**: Merchants, guilds, builders
- **Tenets**:
  - A just deal enriches both.
  - Craft with purpose; build for centuries.
  - Swindled wealth is haunted wealth.
- **Rites & Holy Days**: Market Consecration at new charter; Founders’ Audit after harvest.
- **Relationships**: Respects Thalos; distrusts Vespera’s guilds; partnered with Borhild on great works.

### Vespera of the Many Masks
- **Portfolio**: Trickery, performance, subterfuge, reinvention, revolution through mockery
- **Ethos / Alignment**: Chaotic / Neutral
- **Suggested Domains**: `DOMAIN_TRICKERY`, `DOMAIN_CHARM`, `DOMAIN_LUCK`, `DOMAIN_PORTAL`, `DOMAIN_ILLUSION`
- **Holy Symbol**: A fan of three masks (laughing, weeping, serene)
- **Favored Weapon**: Rapier
- **Worshippers**: Thespians, spies, confidence artists, revolutionaries, satirists, shapeshifters
- **Tenets**:
  - Truth is a stage direction; meaning is the play.
  - Learn every door, even the ones you will never open.
  - Break tyranny with a joke people remember.
  - Every face is a mask; choose yours wisely.
  - The greatest trick is making power laugh at itself.
- **Rites & Holy Days**: The Masquerade of Low Kings each midwinter; Rite of the Second Name at personal rebirth; Theatre of Truth (monthly performances that mock the powerful).
- **Relationships**: Flirts with Nyxara over shared deceptions; sabotages Sharguk's terror with ridicule; rescued by Lumerion in many tales; teaches Calystral that art can be rebellion; despises Thalos's rigid order.

### Nethris, the Gravewarden
- **Portfolio**: Death, fate, proper rites, thresholds of endings
- **Ethos / Alignment**: Lawful / Neutral
- **Suggested Domains**: `DOMAIN_DEATH`, `DOMAIN_FATE`, `DOMAIN_PROTECTION`, `DOMAIN_TRAVEL`
- **Holy Symbol**: A key bound to a raven-feather cord
- **Favored Weapon**: Scythe
- **Worshippers**: Undertakers, jurists, monks, keepers of last wills
- **Tenets**:
  - Every life is a ledger; settle it cleanly.
  - The dead are owed silence and safe passage.
  - Fate is a road, not a jailer.
- **Rites & Holy Days**: Crossing Vigils at dusk; Ledgerfast on the last day of the year.
- **Relationships**: Cold courtesy with Myrr; hunts Yurga’s blasphemies; often invoked by Thalos.

### Calystral, the Flameheart
- **Portfolio**: Love, art, passion, creative fire
- **Ethos / Alignment**: Chaotic / Good
- **Suggested Domains**: `DOMAIN_CHARM`, `DOMAIN_GOOD`, `DOMAIN_FIRE`, `DOMAIN_MOBILITY`
- **Holy Symbol**: A heart-flame within a circlet of quills
- **Favored Weapon**: Whip or short spear (ritualistic)
- **Worshippers**: Artists, lovers, celebrants, patron-muses
- **Tenets**:
  - Create boldly; love honestly; forgive fiercely.
  - Beauty is a duty when cruelty is easy.
  - Burn for what you would save.
- **Rites & Holy Days**: Night of a Thousand Lamps in summer; Vows of Twinned Breath for unions.
- **Relationships**: Patron to Lirael; scorns Velara’s assassins who kill for art.

### Orith, the Stonefather
- **Portfolio**: Earth, mountains, patience, endurance
- **Ethos / Alignment**: Neutral / Good
- **Suggested Domains**: `DOMAIN_EARTH`, `DOMAIN_PROTECTION`, `DOMAIN_STRENGTH`, `DOMAIN_CAVERN`
- **Holy Symbol**: A mountain rune carved in granite
- **Favored Weapon**: Greatclub or warpick
- **Worshippers**: Miners, mountaineers, stalwart defenders
- **Tenets**:
  - Move slow; move sure.
  - Shelter the valley from the avalanche you begin.
  - Let time be your ally.
- **Rites & Holy Days**: Stonebinding for new halls; Echofast at solstices within cavern temples.
- **Relationships**: Close to Borhild; protective of Viridara’s roots against Ghorak’s razings.

---

## Dwarven Hearth and Forge

### Borhild Emberforge
- **Portfolio**: Forgecraft, innovation, consecrated labor
- **Ethos / Alignment**: Lawful / Good
- **Suggested Domains**: `DOMAIN_CRAFT`, `DOMAIN_METAL`, `DOMAIN_FIRE`, `DOMAIN_PROTECTION`
- **Symbol/Weapon**: Anvil with ember-sigil; warhammer
- **Tenets**: Work honors ancestors; temper strength with purpose; masterpieces are vows in steel.
- **Notes**: Joint patron of great projects with Erix.

### Skarn Graniteward
- **Portfolio**: Guardianship, vigilance, bulwarks
- **Ethos / Alignment**: Lawful / Neutral
- **Suggested Domains**: `DOMAIN_PROTECTION`, `DOMAIN_WAR`, `DOMAIN_LAW`, `DOMAIN_STRENGTH`
- **Symbol/Weapon**: Tower-shield rune; battle axe
- **Tenets**: Be the wall; keep watches true; an open gate is an invitation.

### Maela Rubyvein
- **Portfolio**: Hearth, family, healing elixirs
- **Ethos / Alignment**: Neutral / Good
- **Suggested Domains**: `DOMAIN_FAMILY`, `DOMAIN_HEALING`, `DOMAIN_WATER`, `DOMAIN_GOOD`
- **Symbol/Weapon**: Ruby cup; light mace
- **Tenets**: Raise children brave and kind; the table binds houses as mortar binds stone.

### Duerak Deepdelve
- **Portfolio**: Subterranean lore, hidden veins, secrecy in craft
- **Ethos / Alignment**: Neutral / Neutral
- **Suggested Domains**: `DOMAIN_CAVERN`, `DOMAIN_KNOWLEDGE`, `DOMAIN_RUNE`, `DOMAIN_PROTECTION`
- **Symbol/Weapon**: Lantern-and-pick; pickaxe
- **Tenets**: Knowledge hoarded dulls; knowledge shared sharpens.

### Vangar Battlebraid
- **Portfolio**: War-fury, heroism, oaths of steel
- **Ethos / Alignment**: Chaotic / Good
- **Suggested Domains**: `DOMAIN_WAR`, `DOMAIN_STRENGTH`, `DOMAIN_CHAOS`, `DOMAIN_RETRIBUTION`
- **Symbol/Weapon**: Braided knot over crossed blades; greatsword
- **Tenets**: Choose worthy foes; avenge treachery; sing the names of the fallen.

---

## Elven Courts of Bough and Star

### Selithiel Moonbough
- **Portfolio**: Moon, dreams, passage, mysticism
- **Ethos / Alignment**: Chaotic / Good
- **Suggested Domains**: `DOMAIN_MOON`, `DOMAIN_ILLUSION`, `DOMAIN_TRAVEL`, `DOMAIN_GOOD`
- **Symbol/Weapon**: Crescent circlet; quarterstaff
- **Tenets**: Walk softly between worlds; dreams remember what we forget.

### Aerion Swiftwind
- **Portfolio**: Air, migratory paths, messengers
- **Ethos / Alignment**: Chaotic / Neutral
- **Suggested Domains**: `DOMAIN_AIR`, `DOMAIN_TRAVEL`, `DOMAIN_PROTECTION`, `DOMAIN_LUCK`
- **Symbol/Weapon**: Feathered spiral; shortbow
- **Tenets**: Bear tidings true; a lie on the wind becomes a storm.

### Viridara Thornsong
- **Portfolio**: Forests, growth, wild harmonies
- **Ethos / Alignment**: Neutral / Good
- **Suggested Domains**: `DOMAIN_PLANT`, `DOMAIN_ANIMAL`, `DOMAIN_RENEWAL`, `DOMAIN_PROTECTION`
- **Symbol/Weapon**: Oak leaf entwined with vine; scimitar
- **Tenets**: Prune to heal; root to endure; seed to return.

### Lirael Dawnsong
- **Portfolio**: Beauty, art, poetry, grace in motion
- **Ethos / Alignment**: Chaotic / Good
- **Suggested Domains**: `DOMAIN_CHARM`, `DOMAIN_KNOWLEDGE`, `DOMAIN_GOOD`, `DOMAIN_MOBILITY`
- **Symbol/Weapon**: Golden lyre; rapier
- **Tenets**: Art is mercy; talent is duty; share both.

### Kaelthir Starwarden
- **Portfolio**: Ancient knowledge, time, prophecy
- **Ethos / Alignment**: Neutral / Neutral
- **Suggested Domains**: `DOMAIN_KNOWLEDGE`, `DOMAIN_TIME`, `DOMAIN_RUNE`, `DOMAIN_FATE`
- **Symbol/Weapon**: Seven-pointed star-map; dagger
- **Tenets**: Keep the old names; read the sky humbly; warn without ruling.

---

## Halfling Hearth-Tide

### Pella of the Warm Hearth
- **Portfolio**: Home, hospitality, safe roads
- **Ethos / Alignment**: Lawful / Good
- **Suggested Domains**: `DOMAIN_FAMILY`, `DOMAIN_PROTECTION`, `DOMAIN_TRAVEL`, `DOMAIN_GOOD`
- **Symbol/Weapon**: Kettle-and-key; quarterstaff
- **Tenets**: Leave a light in your window; no guest goes hungry.

### Brandoc Quickstep
- **Portfolio**: Luck, wit, audacious larceny (with rules)
- **Ethos / Alignment**: Chaotic / Neutral
- **Suggested Domains**: `DOMAIN_LUCK`, `DOMAIN_TRICKERY`, `DOMAIN_TRAVEL`, `DOMAIN_PORTAL`
- **Symbol/Weapon**: Playing die showing double-sixes; dagger
- **Tenets**: Steal from greed, not need; never from the table that fed you.

### Willow Merryleaf
- **Portfolio**: Gardens, song, festivals
- **Ethos / Alignment**: Neutral / Good
- **Suggested Domains**: `DOMAIN_PLANT`, `DOMAIN_GOOD`, `DOMAIN_WATER`, `DOMAIN_RENEWAL`
- **Symbol/Weapon**: Garlanded flute; sling
- **Tenets**: Joy is a harvest; share it.

---

## Orcish Ash-Legion

### Ghorak the Ash-Eyed
- **Portfolio**: Conquest, strength, survival
- **Ethos / Alignment**: Chaotic / Evil
- **Suggested Domains**: `DOMAIN_STRENGTH`, `DOMAIN_WAR`, `DOMAIN_DESTRUCTION`, `DOMAIN_HATRED`
- **Symbol/Weapon**: Charred eye sigil; spear
- **Tenets**: Take the strong ground; crush weak chains; honor the unbroken.

### Luthra Bloodmother
- **Portfolio**: Caves, fertility, endurance of kin
- **Ethos / Alignment**: Neutral / Evil
- **Suggested Domains**: `DOMAIN_CAVERN`, `DOMAIN_FAMILY`, `DOMAIN_HEALING`, `DOMAIN_EARTH`
- **Symbol/Weapon**: Red handprint; clawed gauntlets (unarmed)
- **Tenets**: Blood remembers; hide the young; keep fire alive.

### Sharguk Nightfang
- **Portfolio**: Night raids, silence, terror-craft
- **Ethos / Alignment**: Chaotic / Evil
- **Suggested Domains**: `DOMAIN_DARKNESS`, `DOMAIN_TRICKERY`, `DOMAIN_TRAVEL`, `DOMAIN_RETRIBUTION`
- **Symbol/Weapon**: Blackened fang; short sword
- **Tenets**: Fear is a spear thrown before battle; leave none to warn.

### Yurga of the Pale Hand
- **Portfolio**: Disease, rot, despair
- **Ethos / Alignment**: Neutral / Evil
- **Suggested Domains**: `DOMAIN_DEATH`, `DOMAIN_SUFFERING`, `DOMAIN_DESTRUCTION`, `DOMAIN_EVIL`
- **Symbol/Weapon**: Bone handprint; flail
- **Tenets**: Let rot do the work; make their courage useless.

---

## Seafolk and Sky

### Thalassa Stormqueen
- **Portfolio**: Tempests, shipwreck tithe, deep truths
- **Ethos / Alignment**: Chaotic / Neutral
- **Suggested Domains**: `DOMAIN_STORM`, `DOMAIN_OCEAN`, `DOMAIN_DESTRUCTION`, `DOMAIN_FATE`
- **Symbol/Weapon**: Crowned wave; spear
- **Tenets**: The sea is a debt; pay it when asked.
- **Note**: Demands a “tenth sail” offering from major fleets—often opposed by Vaelith’s clergy.

### Aelor Keelwarden
- **Portfolio**: Sailors, fair winds, charts
- **Ethos / Alignment**: Neutral / Good
- **Suggested Domains**: `DOMAIN_TRAVEL`, `DOMAIN_PROTECTION`, `DOMAIN_OCEAN`, `DOMAIN_LUCK`
- **Symbol/Weapon**: Compass rose; scimitar
- **Tenets**: Bring crews home; star-swear your promises; keep charts honest.

---

## Under-Shadow: The Dark Courts

### Velara, the Obsidian Rose
- **Portfolio**: Silent courts, assassination-as-politic, ambition veiled
- **Ethos / Alignment**: Lawful / Evil
- **Suggested Domains**: `DOMAIN_LAW`, `DOMAIN_TRICKERY`, `DOMAIN_DARKNESS`, `DOMAIN_RETRIBUTION`
- **Symbol/Weapon**: Thorned black rose; stiletto
- **Tenets**: Power is a contract written in blood; elegance is the blade you never show.

### Zhaerin Nightglass
- **Portfolio**: Shadow-magic, memory theft, forbidden tomes
- **Ethos / Alignment**: Neutral / Evil
- **Suggested Domains**: `DOMAIN_ILLUSION`, `DOMAIN_DARKNESS`, `DOMAIN_MAGIC`, `DOMAIN_KNOWLEDGE`
- **Symbol/Weapon**: Dark mirror sigil; dagger
- **Tenets**: Knowledge kept is power doubled; erase what can erase you.

### Draven Coil
- **Portfolio**: Poisons, subversion, patient ruin
- **Ethos / Alignment**: Chaotic / Evil
- **Suggested Domains**: `DOMAIN_DESTRUCTION`, `DOMAIN_TRICKERY`, `DOMAIN_EVIL`, `DOMAIN_SUFFERING`
- **Symbol/Weapon**: Coiled viper around a chalice; whip or garrote
- **Tenets**: If a fortress cannot be toppled, wither the roots; venom is cheaper than war.

---

## Divine Artifacts and Relics

### Major Artifacts (One per deity, lost to time)
- **Aethyra's Spindle**: Can reweave a broken oath or restore a lost memory
- **Kordran's Sunhammer**: Never misses a righteous strike, burns undead to ash
- **Seraphine's Seeds**: Seven golden seeds that can restore dead lands to life
- **Nyxara's Veil**: Renders the wearer undetectable by any means
- **Thalos's Ledger**: Records every lie spoken in its presence
- **Nethris's Key**: Opens the door between life and death

### Minor Relics (Clergy can quest for these)
- **Threads of the Loom**: Single strands that grant one-time divine intervention
- **Waystone Lanterns**: Never extinguish, always point toward safety (Lumerion)
- **Mercy Cups**: Water within heals any poison or disease (Myrr)
- **Truth Chalk**: Lines drawn cannot be crossed by liars (Thalos)
- **Dawn Crystals**: Glow in the presence of undead (Seraphine)
- **Shadow Cloaks**: Grant limited invisibility in darkness (Nyxara)

## Clerical Practice and Roleplay Hooks

### Daily Observances
- **Petitioning the Loom**: Clerics frame daily petitions as "knotting" a thread—naming a need, a virtue, and a memory offered.
- **The Five Prayers**: Dawn (gratitude), Noon (dedication), Dusk (reflection), Midnight (vigil), and Crisis (immediate need)
- **Sacred Gestures**: Each deity has a unique hand-sign for silent identification between clergy

### Clergy Organization
- **Vestments**: Color and material vary by deity; common threads and sigils across sects signal shared oaths to the Loom to avoid inter-clerical duels.
- **Ranks**: Initiate, Acolyte, Priest/Priestess, High Priest/Priestess, Chosen (rare, directly touched by deity)
- **Temples**: Range from grand cathedrals (Thalos, Kordran) to hidden shrines (Nyxara, Vespera) to natural groves (Viridara, Zorren)

### Miracles and Omens
- **Major Signs**: Threads hum audibly during major miracles; the sky reflects the deity's nature (golden for Seraphine, starless for Nyxara)
- **Minor Omens**: Lanterns that won't extinguish (Lumerion), cups that never spill (Myrr), chalk lines that refuse to be crossed (Thalos), flowers blooming out of season (Seraphine), shadows moving against light (Nyxara)
- **Divine Displeasure**: Holy symbols tarnish, prayers feel hollow, divine magic weakens

### Sect Schisms and Variants
- **Kordran's Banner-Menders**: Refuse siege warfare, focus on defending innocents
- **Nyxara's Lamplighters**: Protect secrets of the persecuted rather than extort
- **Seraphine's Dusk Walkers**: Believe redemption must be earned through trials
- **Thalos's Grey Judges**: Advocate for rehabilitation over punishment
- **Myrr's Silent Hands**: Take vows of silence, communicate only through healing

### Sacred Sites and Pilgrimage
- **Major Knots of the Loom**: Crossroads (Lumerion), cavern springs (Myrr), mountain echoes (Orith), moonlit clearings (Zorren), seaside cliffs (Vaelith)
- **The Great Convergence**: Once per century, all clergy gather at the First Temple where the Loom was discovered
- **Personal Pilgrimages**: Each cleric must visit three sacred sites during their lifetime

---

## Divine Conflicts and Alliances

### The Great Schisms
- **The Dawn Sundering**: When Seraphine and Nyxara split from their original unified form, creating day and night
- **The Forge War**: Borhild and Pyrion's contest over who truly masters creation through fire
- **The Silent Accord**: Nethris and Myrr's agreement that death and healing must coexist

### Active Divine Conflicts
- **The Harvest Dispute**: Seraphine vs. Draven - corruption of croplands vs. blessed harvests
- **The Sea Divided**: Vaelith, Thalassa, and Aelor compete for dominion over different aspects of the ocean
- **The Memory Wars**: Aethyra vs. Zhaerin - preserving vs. stealing memories
- **The Wild Boundaries**: Zorren vs. Ghorak - freedom of the wild vs. conquest of nature

### Divine Alliances
- **The Makers' Compact**: Borhild, Erix, and Pyrion share forge-knowledge
- **The Mercy Circle**: Myrr, Seraphine, and Pella protect the innocent
- **The Shadow Pact**: Nyxara, Umbros, and Vespera trade in secrets
- **The Natural Order**: Viridara, Verdania, Orith, and Zorren maintain the wilderness

### Prophesied Events
- **The Thread's End**: When the Loom will unravel unless all deities unite
- **The Second Dawn**: Seraphine and Nyxara's prophesied reunification
- **The Final Hunt**: Zorren's last chase that will determine the fate of all wild things
- **The Great Audit**: Thalos's judgment of the gods themselves

## Divine Heralds and Servitors

### Celestial Heralds (Core Deities)
- **Aethyra**: The Memory Sphinx - A crystalline sphinx that speaks in riddles of past and future
- **Kordran**: The Dawn Champion - An armored solar with a hammer of pure sunlight
- **Seraphine**: The Golden Phoenix - Resurrects the fallen and brings hope
- **Nyxara**: The Shadow Oracle - A veiled figure that knows all secrets but speaks few
- **Thalos**: The Inevitable Judge - A construct of law that cannot be deceived
- **Nethris**: The Pale Ferryman - Guides souls across the final threshold
- **Myrr**: The Mercy Dove - A luminous bird that appears to the dying
- **Zorren**: The Alpha Wolf - A massive dire wolf with silver eyes
- **Lumerion**: The Waylight Archon - A cheerful entity that helps lost travelers

### Elemental Heralds (Primarchs)
- **Pyrion**: The Ember Titan - A giant of living flame and molten metal
- **Glacius**: The Frost Wyrm - An ancient ice dragon that speaks in whispers
- **Verdania**: The World Tree Avatar - A walking tree that seeds new forests
- **Umbros**: The Void Walker - An absence of form that consumes light

### Divine Servants (Lesser beings that serve the gods)
- **Thread Weavers**: Aethyra's servants who maintain the Loom
- **Banner Bearers**: Kordran's warrior angels
- **Dawn Maidens**: Seraphine's healing spirits
- **Veil Keepers**: Nyxara's shadow servants
- **Law Scribes**: Thalos's recording angels
- **Soul Guides**: Nethris's psychopomps
- **Brook Spirits**: Myrr's water elementals
- **Hunt Hounds**: Zorren's celestial pack

## Elemental Primarchs (New Additions)

These ancient deities represent the raw elemental forces that shaped the world before the Loom was woven. They are distant and alien, rarely worshipped directly, but their influence permeates all elemental magic.

### Pyrion, the First Flame
- **Portfolio**: Primal fire, creation through destruction, forge-heat, passion
- **Ethos / Alignment**: Chaotic / Neutral
- **Suggested Domains**: `DOMAIN_FIRE`, `DOMAIN_DESTRUCTION`, `DOMAIN_RENEWAL`, `DOMAIN_CRAFT`
- **Holy Symbol**: A spiral of white flame consuming itself
- **Favored Weapon**: Flaming scimitar
- **Worshippers**: Elemental fire mages, smiths who work with magical metals, passionate artists
- **Tenets**: Burn away the old to make room for the new; passion without purpose is mere destruction; the forge teaches patience through heat.
- **Relationships**: Respects Borhild's mastery of forge-craft; opposed to Glacius in eternal balance; allied with Sirrion in creative fire.

### Glacius, the Eternal Winter
- **Portfolio**: Primal ice, preservation, patience, inevitable endings
- **Ethos / Alignment**: Lawful / Neutral
- **Suggested Domains**: `DOMAIN_WATER`, `DOMAIN_TIME`, `DOMAIN_PROTECTION`, `DOMAIN_DEATH`
- **Holy Symbol**: A perfect snowflake that never melts
- **Favored Weapon**: Icicle spear
- **Worshippers**: Ice mages, those who preserve ancient knowledge, undertakers
- **Tenets**: All things must end, but endings can be beautiful; preserve what matters; patience outlasts passion.
- **Relationships**: Eternal opposition to Pyrion; works with Nethris on matters of endings; respects Kaelthir's preservation of time.

### Verdania, the Growing Green
- **Portfolio**: Primal nature, growth, the wild hunt, untamed life
- **Ethos / Alignment**: Neutral / Neutral
- **Suggested Domains**: `DOMAIN_PLANT`, `DOMAIN_ANIMAL`, `DOMAIN_RENEWAL`, `DOMAIN_CHAOS`
- **Holy Symbol**: A tree whose roots and branches form an endless spiral
- **Favored Weapon**: Living wood staff
- **Worshippers**: Druids, rangers, those who live in harmony with nature
- **Tenets**: Life finds a way; growth requires both nurturing and pruning; the wild teaches harsh truths.
- **Relationships**: Patron of Viridara's forests; works with Zorren in matters of the hunt; opposed to Umbros's stagnation.

### Umbros, the Deep Dark
- **Portfolio**: Primal darkness, the void between stars, rest, the unknown
- **Ethos / Alignment**: Neutral / Evil
- **Suggested Domains**: `DOMAIN_DARKNESS`, `DOMAIN_VOID`, `DOMAIN_KNOWLEDGE`, `DOMAIN_CAVERN`
- **Holy Symbol**: A sphere of absolute darkness
- **Favored Weapon**: Void-touched blade
- **Worshippers**: Deep cave dwellers, scholars of forbidden knowledge, those who seek oblivion
- **Tenets**: In darkness, all things are equal; some knowledge is too dangerous for light; rest is the reward of existence.
- **Relationships**: Ancient ally of Nyxara; opposed to all sources of light; respects Duerak's deep delving.

---

## Implementation Notes (Designers/Builders)

### Domain Integration
- Map “Suggested Domains” to your domain constants in `src/deities.c` when adding data; adjust pantheons as needed.
- The enhanced pantheon now includes additional domains like `DOMAIN_PORTAL`, `DOMAIN_VOID`, and `DOMAIN_SPELL` for richer mechanical variety.
- Consider adding new domain constants for unique portfolios (e.g., `DOMAIN_MEMORY`, `DOMAIN_OATHS`, `DOMAIN_REDEMPTION`).

### Weapon and Combat Integration
- For favored weapons, align with available `WEAPON_TYPE_*` categories.
- Enhanced descriptions provide both mechanical and thematic justification for weapon choices.
- Consider special weapon properties for divine champions (e.g., Kordran's hammers deal extra damage to undead).

### Deity-Specific Features
- Deity-specific feats/boons: Use minor passive perks that reinforce tenets:
  - Orith devotees get advantage vs. forced movement and bonus to endurance
  - Lumerion grants small luck rerolls on travel checks and navigation
  - Nethris grants funeral-rite skill bonuses and speak with dead abilities
  - Aethyra followers gain bonuses to magical research and oath-keeping
  - Seraphine worshippers get enhanced healing and redemption-based abilities

### Divine Blessings (Rewards for Exceptional Service)
- **Aethyra's Clarity**: Perfect memory recall for 24 hours
- **Kordran's Valor**: Immunity to fear and +2 attack bonus for one battle
- **Seraphine's Grace**: Next healing spell maximized and affects all allies nearby
- **Nyxara's Shadow**: Become undetectable for 10 minutes
- **Myrr's Peace**: Hostile creatures must save or become non-aggressive
- **Lumerion's Fortune**: Reroll any three failed rolls in the next hour
- **Zorren's Hunt**: Track any creature perfectly for 24 hours

### Divine Curses (Punishments for Transgression)
- **Aethyra's Confusion**: Cannot remember anything beyond the last hour
- **Kordran's Cowardice**: Flee from any combat situation
- **Seraphine's Withering**: All food tastes of ash, healing reduced by half
- **Nyxara's Exposure**: All secrets known to others, cannot hide
- **Thalos's Mark**: A visible brand showing your crime to all
- **Nethris's Shadow**: Undead are drawn to you constantly
- **Vespera's Truth**: Cannot lie or disguise yourself

### World Integration
- OLC hooks: Add shrines as world “Knots” with micro-ritual interactions:
  - Light a lantern (Lumerion), bind a thread (Aethyra), inscribe a rune (Thalos)
  - Pour a libation (Myrr), offer a secret (Nyxara), plant a seed (Seraphine)
  - These interactions toggle small persistent room effects and provide minor blessings
- The Elemental Primarchs should have ancient, weathered shrines in remote locations
- Consider seasonal festivals that bring multiple deities' followers together

### Roleplay and Faction Systems
- Enhanced relationships provide clear faction dynamics for political intrigue
- Opposing deities create natural conflict sources (Seraphine vs. Nyxara, Pyrion vs. Glacius)
- Allied deities offer cooperation opportunities (Kordran + Orith, Aethyra + Kaelthir)
- The Loom cosmology provides a unifying framework while allowing for divine conflict

### Pantheon Balance Analysis

**Alignment Distribution (Enhanced):**
- **Lawful Good**: Kordran, Borhild, Skarn, Maela, Gorm Gulthyn, Pella, Arvoreen
- **Neutral Good**: Aethyra, Seraphine, Myrr, Orith, Selithiel, Viridara, Lirael, Willow
- **Chaotic Good**: Lumerion, Calystral, Vangar, Aerion
- **Lawful Neutral**: Thalos, Erix, Nethris, Glacius, Duerak
- **True Neutral**: Vaelith, Zorren, Kaelthir, Verdania, Brandoc
- **Chaotic Neutral**: Vespera, Pyrion, Umbros
- **Lawful Evil**: Velara
- **Neutral Evil**: Nyxara, Yurga, Luthra
- **Chaotic Evil**: Ghorak, Sharguk, Zhaerin, Draven

**Domain Coverage:** The enhanced pantheon now covers all major D&D domains plus unique additions, ensuring clerics of all types can find appropriate deities.

**Cultural Representation:** Each major fantasy race has dedicated pantheon members while maintaining the universal appeal of core deities.

**Mechanical Variety:** Enhanced favored weapons and domain combinations provide diverse mechanical options for divine characters.

---

## Changelog (Enhanced Version 2.0)

### Major Improvements
- **Expanded Portfolios**: All core deities now have richer, more detailed areas of influence
- **Enhanced Domains**: Added `DOMAIN_PORTAL`, `DOMAIN_SPELL`, `DOMAIN_VOID`, `DOMAIN_ILLUSION`, `DOMAIN_LIBERATION` and other specialized domains
- **Deeper Relationships**: More complex inter-deity relationships that create roleplay opportunities
- **Elemental Primarchs**: Four new ancient deities representing primal elemental forces
- **Better Balance**: Improved alignment distribution and mechanical variety
- **Richer Lore**: Enhanced cosmology with the three forces of Memory, Will, and Possibility
- **Divine Conflicts**: Added active conflicts and prophesied events for plot hooks
- **Divine Artifacts**: Major and minor relics for quest objectives
- **Divine Heralds**: Specific celestial servants for each deity
- **Blessings & Curses**: Mechanical rewards and punishments for deity interaction

### Minor Enhancements
- More detailed holy symbols and favored weapon justifications
- Expanded worshipper categories for broader appeal
- Additional holy days and rituals for each deity
- Enhanced tenets that provide clearer moral guidance
- Better integration with existing D&D mechanical systems
- Clergy organization and sect variants
- Sacred sites and pilgrimage destinations
- Daily observances and prayer schedules

### Implementation Ready
- All domains map to existing or easily-added game constants
- Weapon choices align with standard weapon categories
- Relationships create natural faction dynamics
- Lore provides rich roleplay hooks without overwhelming complexity
- Divine servants can be used as quest givers or boss encounters
- Artifacts provide tangible goals for high-level play
- Blessings and curses offer immediate mechanical consequences



---
## Import Schema and Mapping to Code

This section makes the Luminari Pantheon import-ready by explicitly covering every field used by the deity engine. It provides:
- A schema that maps this document’s fields to the engine API in [add_deity()](src/deities.c:101) and [add_deity_new()](src/deities.c:156)
- Enumeration/constant mappings (ethos, alignment, domains, weapons, pantheons)
- Defaults/derivation rules for any still-missing values
- A complete per-deity “Field Completion” list (aliases, worshipper alignments, follower demonyms, explicit pantheon)

Reference for engine loading entry point: [assign_deities()](src/deities.c:201). Source file: [src/deities.c](src/deities.c:1)

### 1) Engine Field Schema (maps to struct deity_info)

The engine expects the following fields to be present when importing. Both legacy and new systems are supported; Luminari uses the new system.

- Required (New System via [add_deity_new()](src/deities.c:156)):
  - name: string
  - ethos: one of ETHOS_LAWFUL, ETHOS_NEUTRAL, ETHOS_CHAOTIC
  - alignment: one of ALIGNMENT_GOOD, ALIGNMENT_NEUTRAL, ALIGNMENT_EVIL
  - pantheon: DEITY_PANTHEON_* constant
  - alias: string (comma-separated aliases/titles)
  - portfolio: string (areas of influence)
  - symbol: string (holy symbol description)
  - worshipper_alignments: string (comma-separated allowed alignments in “Lawful Good, ...” form)
  - follower_names: string (what worshippers are called, e.g. “Aethyran(s)”)
  - description: string (lore/description)

- Optional legacy mechanics (Old System via [add_deity()](src/deities.c:101)):
  - domains: up to six DOMAIN_* constants
  - favored_weapon: WEAPON_TYPE_* constant
  - portfolio: string
  - description: string

Luminari entries below provide Suggested Domains (for mechanics) and Favored Weapon where applicable. Importers may call [add_deity_new()](src/deities.c:156) for roleplay fields, and optionally extend with domain/weapon data (hybrid import) if desired.

### 2) Enumeration and Constant Mappings

- Ethos text ➜ ETHOS_*:
  - Lawful ➜ ETHOS_LAWFUL
  - Neutral ➜ ETHOS_NEUTRAL
  - Chaotic ➜ ETHOS_CHAOTIC

- Alignment text ➜ ALIGNMENT_*:
  - Good ➜ ALIGNMENT_GOOD
  - Neutral ➜ ALIGNMENT_NEUTRAL
  - Evil ➜ ALIGNMENT_EVIL

- Pantheon mapping (proposed for Luminari; add these constants alongside existing ones):
  - LUMINARI Core ➜ DEITY_PANTHEON_LUMINARI_CORE
  - Dwarven Hearth and Forge ➜ DEITY_PANTHEON_LUMINARI_DWARVEN
  - Elven Courts of Bough and Star ➜ DEITY_PANTHEON_LUMINARI_ELVEN
  - Halfling Hearth-Tide ➜ DEITY_PANTHEON_LUMINARI_HALFLING
  - Orcish Ash-Legion ➜ DEITY_PANTHEON_LUMINARI_ORCISH
  - Seafolk and Sky ➜ DEITY_PANTHEON_LUMINARI_SEAFOLK
  - Under-Shadow (Dark Courts) ➜ DEITY_PANTHEON_LUMINARI_DARK_COURTS
  - Elemental Primarchs ➜ DEITY_PANTHEON_LUMINARI_PRIMARCHS

- Weapon text ➜ WEAPON_TYPE_* (importers may use the following normalization):
  - Warhammer ➜ WEAPON_TYPE_WARHAMMER
  - Greatsword ➜ WEAPON_TYPE_GREAT_SWORD
  - Longsword ➜ WEAPON_TYPE_LONG_SWORD
  - Shortsword ➜ WEAPON_TYPE_SHORT_SWORD
  - Rapier ➜ WEAPON_TYPE_RAPIER
  - Spear ➜ WEAPON_TYPE_SPEAR
  - Trident ➜ WEAPON_TYPE_TRIDENT
  - Quarterstaff / Staff ➜ WEAPON_TYPE_QUARTERSTAFF
  - Scimitar ➜ WEAPON_TYPE_SCIMITAR
  - Light hammer ➜ WEAPON_TYPE_LIGHT_HAMMER
  - Whip ➜ WEAPON_TYPE_WHIP
  - Flail ➜ WEAPON_TYPE_FLAIL
  - Dire flail ➜ WEAPON_TYPE_DIRE_FLAIL
  - Greatclub ➜ WEAPON_TYPE_GREAT_CLUB
  - Pick / Warpick ➜ WEAPON_TYPE_HEAVY_PICK
  - Dagger / Stiletto ➜ WEAPON_TYPE_DAGGER
  - Composite longbow ➜ WEAPON_TYPE_COMPOSITE_LONGBOW
  - Shortbow ➜ WEAPON_TYPE_SHORTBOW (if present; else map to COMPOSITE_LONGBOW)

- Domain tags (map to DOMAIN_*):
  - All tags shown in each deity’s “Suggested Domains” map to DOMAIN_* constants.
  - Existing in codebase (examples seen in [src/deities.c](src/deities.c:1)): DOMAIN_WAR, DOMAIN_MAGIC, DOMAIN_SPELL, DOMAIN_KNOWLEDGE, DOMAIN_PLANT, DOMAIN_TRAVEL, DOMAIN_PROTECTION, DOMAIN_STRENGTH, DOMAIN_GOOD, DOMAIN_CHAOS, DOMAIN_LAW, DOMAIN_HEALING, DOMAIN_SUN, DOMAIN_WATER, DOMAIN_OCEAN, DOMAIN_STORM, DOMAIN_ANIMAL, DOMAIN_DEATH, DOMAIN_FATE, DOMAIN_EARTH, DOMAIN_FIRE, DOMAIN_TIME, DOMAIN_RUNE, DOMAIN_TRICKERY, DOMAIN_LUCK, DOMAIN_RENEWAL, DOMAIN_MOON, DOMAIN_CAVERN, DOMAIN_METAL, DOMAIN_TRADE, DOMAIN_RETRIBUTION, DOMAIN_HATRED, DOMAIN_CHARM, DOMAIN_MOBILITY
  - New specialized domains referenced here (add if not present):
    - DOMAIN_PORTAL
    - DOMAIN_VOID

If a domain is not yet defined in code, importer may either:
- Defer/import with DOMAIN_UNDEFINED placeholder, or
- Gate import until domain constant is added.

### 3) Worshipper Alignments Formatting

Use comma-separated D&D alignments exactly in this form, e.g.:
- “Lawful Good, Neutral Good, Chaotic Good”
- “Lawful Neutral, True Neutral, Chaotic Neutral”
- “Lawful Evil, Neutral Evil, Chaotic Evil”
- “Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral” (aka “Any Non-Evil”)
- “Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral, Lawful Evil, Neutral Evil, Chaotic Evil” (aka “Any Alignment”)

### 4) Defaults and Derivation Rules

If any particular field is omitted in a deity block, importers MUST apply these rules so that [add_deity_new()](src/deities.c:156) can still be called with complete data:

- pantheon: derive from section:
  - “Core Deities” ➜ DEITY_PANTHEON_LUMINARI_CORE
  - “Dwarven Hearth and Forge” ➜ DEITY_PANTHEON_LUMINARI_DWARVEN
  - “Elven Courts of Bough and Star” ➜ DEITY_PANTHEON_LUMINARI_ELVEN
  - “Halfling Hearth-Tide” ➜ DEITY_PANTHEON_LUMINARI_HALFLING
  - “Orcish Ash-Legion” ➜ DEITY_PANTHEON_LUMINARI_ORCISH
  - “Seafolk and Sky” ➜ DEITY_PANTHEON_LUMINARI_SEAFOLK
  - “Under-Shadow: The Dark Courts” ➜ DEITY_PANTHEON_LUMINARI_DARK_COURTS
  - “Elemental Primarchs” ➜ DEITY_PANTHEON_LUMINARI_PRIMARCHS

- alias: if missing, use epithets/subtitles shown in the deity header line (e.g., “Lady of the Loom”, “Hammer of Dawn”) comma-separated.

- follower_names (demonym): if missing, derive by:
  - If name ends with vowel, add “-n(s)” (e.g., “Aelor” ➜ “Aeloran(s)”)
  - Else add “-ite(s)” (e.g., “Kordran” ➜ “Kordranite(s)”)
  - Overridden by the explicit demonyms in the table below.

- worshipper_alignments default matrix (override when explicitly provided below):
  - LG deity ➜ “Lawful Good, Neutral Good, Lawful Neutral”
  - NG deity ➜ “Lawful Good, Neutral Good, Chaotic Good, True Neutral, Lawful Neutral, Chaotic Neutral” (Any Non-Evil)
  - CG deity ➜ “Lawful Good, Neutral Good, Chaotic Good, True Neutral, Chaotic Neutral”
  - LN deity ➜ “Lawful Good, Lawful Neutral, Lawful Evil, True Neutral”
  - N deity ➜ “Lawful Neutral, True Neutral, Chaotic Neutral”
  - CN deity ➜ “Chaotic Good, Chaotic Neutral, Chaotic Evil, True Neutral”
  - LE deity ➜ “Lawful Neutral, Lawful Evil, Neutral Evil”
  - NE deity ➜ “Lawful Evil, Neutral Evil, Chaotic Evil, Lawful Neutral, True Neutral, Chaotic Neutral” (Any Non-Good)
  - CE deity ➜ “Lawful Evil, Neutral Evil, Chaotic Evil”

### 5) Import Field Completion (Aliases, Worshipper Alignments, Follower Names, Pantheon)
The following table provides explicit values for the fields most often missing (alias, worshipper_alignments, follower_names, pantheon). These override any derivation defaults above.

Note: Suggested Domains, Holy Symbol, Favored Weapon, Portfolio, Ethos/Alignment, Rites, Relationships, and extended lore are already defined in the sections above for each deity.

#### Core Deities
- Aethyra, Lady of the Loom
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “Lady of the Loom”, “Spindle of Seven”, “Keeper of Oaths”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Aethyran(s)

- Kordran, Hammer of Dawn
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “Hammer of Dawn”, “Shield of Banners”, “The Lineholder”
  - Worshipper Alignments: Lawful Good, Neutral Good, Lawful Neutral
  - Follower Names: Kordranite(s)

- Seraphine the Dawnstar
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “The Dawnstar”, “Harvest Redeemer”, “Lady Emberfast”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Seraphite(s)

- Nyxara of the Veil
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “Mistress of the Veil”, “Keeper of Lost Names”, “Black Oath”
  - Worshipper Alignments: Lawful Evil, Neutral Evil, Chaotic Evil, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Nyxaran(s)

- Thalos, the Scales Unblinking
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “The Scales Unblinking”, “Letters of Law”, “Grey Ink”
  - Worshipper Alignments: Lawful Good, Lawful Neutral, Lawful Evil, True Neutral
  - Follower Names: Thalan(s)

- Vaelith, Whispering Tide
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “Whispering Tide”, “Keeper of Horizons”, “Spiral-Wake”
  - Worshipper Alignments: Chaotic Good, Chaotic Neutral, Chaotic Evil, True Neutral
  - Follower Names: Vaelithan(s)

- Myrr, the Quiet Brook
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “The Quiet Brook”, “Hand of Water”, “Wayhouse Keeper”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Myrran(s)

- Zorren, Lord of the Wild Hunt
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “Lord of the Wild Hunt”, “White Antler”, “Last-Track”
  - Worshipper Alignments: Chaotic Good, Chaotic Neutral, Chaotic Evil, True Neutral
  - Follower Names: Zorrenite(s)

- Lumerion, the Lantern-Bearer
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “The Lantern-Bearer”, “Keeper of Crossroads”, “Coin-Wick”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Lumerionite(s)

- Erix the Coinwright
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “The Coinwright”, “Charter-Hand”, “Founder’s Seal”
  - Worshipper Alignments: Lawful Good, Lawful Neutral, Lawful Evil, True Neutral
  - Follower Names: Erixian(s)

- Vespera of the Many Masks
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “The Many Masks”, “Low King’s Jester”, “Second Name”
  - Worshipper Alignments: Chaotic Good, Chaotic Neutral, Chaotic Evil, True Neutral
  - Follower Names: Vesperan(s)

- Nethris, the Gravewarden
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “The Gravewarden”, “Keeper of Keys”, “Ledger’s End”
  - Worshipper Alignments: Lawful Good, Lawful Neutral, Lawful Evil, True Neutral
  - Follower Names: Nethrisian(s)

- Calystral, the Flameheart
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “The Flameheart”, “Lamp of Muses”, “Twinned Breath”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, True Neutral, Chaotic Neutral
  - Follower Names: Calystran(s)

- Orith, the Stonefather
  - Pantheon: DEITY_PANTHEON_LUMINARI_CORE
  - Aliases: “The Stonefather”, “Echokeep”, “Mountain Rune”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Orithian(s)

#### Dwarven Hearth and Forge
- Borhild Emberforge
  - Pantheon: DEITY_PANTHEON_LUMINARI_DWARVEN
  - Aliases: “Emberforge”, “Anvil-Saint”, “Master of Consecrated Labor”
  - Worshipper Alignments: Lawful Good, Neutral Good, Lawful Neutral
  - Follower Names: Borhildan(s)

- Skarn Graniteward
  - Pantheon: DEITY_PANTHEON_LUMINARI_DWARVEN
  - Aliases: “Graniteward”, “Tower-Shield”, “Keeper of Watches”
  - Worshipper Alignments: Lawful Good, Lawful Neutral, Lawful Evil, True Neutral
  - Follower Names: Skarnite(s)

- Maela Rubyvein
  - Pantheon: DEITY_PANTHEON_LUMINARI_DWARVEN
  - Aliases: “Rubyvein”, “Cup-Matriarch”, “Hearthmother”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Maelan(s)

- Duerak Deepdelve
  - Pantheon: DEITY_PANTHEON_LUMINARI_DWARVEN
  - Aliases: “Deepdelve”, “Lantern-and-Pick”, “Rune Keeper”
  - Worshipper Alignments: Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Dueraki(s)

- Vangar Battlebraid
  - Pantheon: DEITY_PANTHEON_LUMINARI_DWARVEN
  - Aliases: “Battlebraid”, “Oath of Steel”, “Braided Knot”
  - Worshipper Alignments: Chaotic Good, Chaotic Neutral, Neutral Good
  - Follower Names: Vangari(s)

#### Elven Courts of Bough and Star
- Selithiel Moonbough
  - Pantheon: DEITY_PANTHEON_LUMINARI_ELVEN
  - Aliases: “Moonbough”, “Dream-Walker”, “Mistress of Passage”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, True Neutral, Chaotic Neutral
  - Follower Names: Selithian(s)

- Aerion Swiftwind
  - Pantheon: DEITY_PANTHEON_LUMINARI_ELVEN
  - Aliases: “Swiftwind”, “Feather-Spiral”, “Messenger’s Oath”
  - Worshipper Alignments: Chaotic Good, Chaotic Neutral, Chaotic Evil, True Neutral
  - Follower Names: Aerionite(s)

- Viridara Thornsong
  - Pantheon: DEITY_PANTHEON_LUMINARI_ELVEN
  - Aliases: “Thornsongs”, “Greenwarden”, “Oak-Heart”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Viridaran(s)

- Lirael Dawnsong
  - Pantheon: DEITY_PANTHEON_LUMINARI_ELVEN
  - Aliases: “Dawnsong”, “Golden Lyre”, “Grace in Motion”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, True Neutral, Chaotic Neutral
  - Follower Names: Liraelite(s)

- Kaelthir Starwarden
  - Pantheon: DEITY_PANTHEON_LUMINARI_ELVEN
  - Aliases: “Starwarden”, “Seven-Pointed”, “Keeper of Old Names”
  - Worshipper Alignments: Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Kaelthiri(s)

#### Halfling Hearth-Tide
- Pella of the Warm Hearth
  - Pantheon: DEITY_PANTHEON_LUMINARI_HALFLING
  - Aliases: “Warm Hearth”, “Kettle-and-Key”, “Keeper of Roads”
  - Worshipper Alignments: Lawful Good, Neutral Good, Lawful Neutral
  - Follower Names: Pellan(s)

- Brandoc Quickstep
  - Pantheon: DEITY_PANTHEON_LUMINARI_HALFLING
  - Aliases: “Quickstep”, “Double-Sixes”, “Door-Dancer”
  - Worshipper Alignments: Chaotic Good, Chaotic Neutral, Chaotic Evil, True Neutral
  - Follower Names: Brandocan(s)

- Willow Merryleaf
  - Pantheon: DEITY_PANTHEON_LUMINARI_HALFLING
  - Aliases: “Merryleaf”, “Garlanded Flute”, “Harvest Joy”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Willowfolk

#### Orcish Ash-Legion
- Ghorak the Ash-Eyed
  - Pantheon: DEITY_PANTHEON_LUMINARI_ORCISH
  - Aliases: “The Ash-Eyed”, “Breaker of Chains”, “Strong Ground”
  - Worshipper Alignments: Lawful Evil, Neutral Evil, Chaotic Evil
  - Follower Names: Ghoraki(s)

- Luthra Bloodmother
  - Pantheon: DEITY_PANTHEON_LUMINARI_ORCISH
  - Aliases: “Bloodmother”, “Red Hand”, “Fire-Keeper”
  - Worshipper Alignments: Lawful Evil, Neutral Evil, Chaotic Evil, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Luthran(s)

- Sharguk Nightfang
  - Pantheon: DEITY_PANTHEON_LUMINARI_ORCISH
  - Aliases: “Nightfang”, “Black Fang”, “First Terror”
  - Worshipper Alignments: Lawful Evil, Neutral Evil, Chaotic Evil
  - Follower Names: Sharguki(s)

- Yurga of the Pale Hand
  - Pantheon: DEITY_PANTHEON_LUMINARI_ORCISH
  - Aliases: “The Pale Hand”, “Rot-Bringer”, “Despair-Whisper”
  - Worshipper Alignments: Lawful Evil, Neutral Evil, Chaotic Evil, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Yurgan(s)

#### Seafolk and Sky
- Thalassa Stormqueen
  - Pantheon: DEITY_PANTHEON_LUMINARI_SEAFOLK
  - Aliases: “Stormqueen”, “Crowned Wave”, “Tenth Sail”
  - Worshipper Alignments: Chaotic Good, Chaotic Neutral, Chaotic Evil, True Neutral
  - Follower Names: Thalassan(s)

- Aelor Keelwarden
  - Pantheon: DEITY_PANTHEON_LUMINARI_SEAFOLK
  - Aliases: “Keelwarden”, “Compass Rose”, “Star-Swear”
  - Worshipper Alignments: Lawful Good, Neutral Good, Chaotic Good, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Aeloran(s)

#### Under-Shadow: The Dark Courts
- Velara, the Obsidian Rose
  - Pantheon: DEITY_PANTHEON_LUMINARI_DARK_COURTS
  - Aliases: “The Obsidian Rose”, “Silent Court”, “Blood Contract”
  - Worshipper Alignments: Lawful Neutral, Lawful Evil, Neutral Evil
  - Follower Names: Velaran(s)

- Zhaerin Nightglass
  - Pantheon: DEITY_PANTHEON_LUMINARI_DARK_COURTS
  - Aliases: “Nightglass”, “Mirror of Unmemory”, “Forbidden Tome”
  - Worshipper Alignments: Lawful Evil, Neutral Evil, Chaotic Evil, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Zhaerinite(s)

- Draven Coil
  - Pantheon: DEITY_PANTHEON_LUMINARI_DARK_COURTS
  - Aliases: “Coil”, “Chalice-Viper”, “Patient Ruin”
  - Worshipper Alignments: Lawful Evil, Neutral Evil, Chaotic Evil
  - Follower Names: Dravenite(s)

#### Elemental Primarchs
- Pyrion, the First Flame
  - Pantheon: DEITY_PANTHEON_LUMINARI_PRIMARCHS
  - Aliases: “The First Flame”, “Forge-Heart”, “White Spiral”
  - Worshipper Alignments: Chaotic Good, Chaotic Neutral, Chaotic Evil, True Neutral
  - Follower Names: Pyrionite(s)

- Glacius, the Eternal Winter
  - Pantheon: DEITY_PANTHEON_LUMINARI_PRIMARCHS
  - Aliases: “The Eternal Winter”, “Perfect Snow”, “Still End”
  - Worshipper Alignments: Lawful Good, Lawful Neutral, Lawful Evil, True Neutral
  - Follower Names: Glacian(s)

- Verdania, the Growing Green
  - Pantheon: DEITY_PANTHEON_LUMINARI_PRIMARCHS
  - Aliases: “The Growing Green”, “Spiral Tree”, “Untamed Life”
  - Worshipper Alignments: Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Verdanian(s)

- Umbros, the Deep Dark
  - Pantheon: DEITY_PANTHEON_LUMINARI_PRIMARCHS
  - Aliases: “The Deep Dark”, “Void-Between”, “Stillness”
  - Worshipper Alignments: Lawful Evil, Neutral Evil, Chaotic Evil, Lawful Neutral, True Neutral, Chaotic Neutral
  - Follower Names: Umbrose(s)

---

## Importer Notes

- Use the “Suggested Domains” already present under each deity’s entry to populate up to six DOMAIN_* values for [add_deity()](src/deities.c:101) if you also want to support mechanical domains at import time. Otherwise, prefer [add_deity_new()](src/deities.c:156) with the extended roleplay fields.

- Favored Weapon lines in each deity entry are descriptive; apply the “Weapon text ➜ WEAPON_TYPE_*” normalization above to produce the exact engine constant.

- Where a field is missing in the body text, use the explicit completion values in “Import Field Completion”. If a field is still missing, apply the derivation rules in section 4.

- New constants needed (add in headers before import if absent):
  - Pantheons: DEITY_PANTHEON_LUMINARI_CORE, DEITY_PANTHEON_LUMINARI_DWARVEN, DEITY_PANTHEON_LUMINARI_ELVEN, DEITY_PANTHEON_LUMINARI_HALFLING, DEITY_PANTHEON_LUMINARI_ORCISH, DEITY_PANTHEON_LUMINARI_SEAFOLK, DEITY_PANTHEON_LUMINARI_DARK_COURTS, DEITY_PANTHEON_LUMINARI_PRIMARCHS
  - Domains: DOMAIN_PORTAL, DOMAIN_VOID (optional; fallback to DOMAIN_UNDEFINED if deferred)

This document now contains every data field required by the engine to import Luminari deities via [assign_deities()](src/deities.c:201), using [add_deity_new()](src/deities.c:156) primarily, with optional domain/weapon mechanics available for hybrid imports through [add_deity()](src/deities.c:101).
