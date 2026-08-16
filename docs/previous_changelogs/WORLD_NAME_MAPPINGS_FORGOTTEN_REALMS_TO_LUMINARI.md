# World Name Mappings: Forgotten Realms to Luminari

## Overview

This document records the comprehensive naming and reference replacement mappings applied across the LuminariMUD world files, source code, and documentation to transition legacy Forgotten Realms (FR) intellectual property and geographic designations into native Luminari lore.

Historical reference commits:
- `b8fac1538a4824651adc13232bc28e70cdb8ae94`: Initial document setting name replacements & pantheon ledger
- `c2a49734831e9d292579f92480b40a9b75840f08`: Replacement tracking updates
- `51de1ee2d8fc99c4b7ec626c808d3b64918978a1`: Consolidated 102 world reference replacements
- `ce4537c15f8a986343037ee387b93d60ace4c3b3`: Merged world reference replacement cleanup

---

## 1. Core Setting and Major Settlements

| Forgotten Realms Original | Luminari Replacement | Region / Notes |
| :--- | :--- | :--- |
| **Faerun** | **Lumia** | Main continent / core world setting |
| **Waterdeep** | **Ashenport** | Major metropolis and trade hub |
| **Neverwinter** | **Koorvik** | Whitehook coastal settlement |
| **Menzoberranzan** | **Nharavel** | Primary Underdark drow metropolis |
| **Gracklstugh** | **Ordrum** | Underdark duergar city |
| **Thugrak Gol** | **Rootfast** | Hir's Sunkwater March settlement |
| **Arabel** | **Kohn Prime** | Major eastern settlement |
| **Beregost** | **Three-Road Market** | Crossroads trading hub |
| **Candlekeep** | **The Floating Library of Chulan** | Fortress/library of knowledge |
| **Corm Orp** | **Selerish Slateharbor** | Canonical region: Selerish |
| **Dagger Falls** | **The Bone Gardens** | Northern frontier |
| **Daggerford** | **Cerax** | Fortified river/coastal crossing |
| **Evereska** | **The Root-Cities of Quechian** | Canonical region: Quechian |
| **Hardbuckler** | **Carstan Glassmarket** | Canonical region: Carstan |
| **Hulburg** | **The Green Maw** | Moonsea / Lenadrian ruin |
| **Longsaddle** | **Stormglass Academy** | Magocratic scholarly haven |
| **Mithril Hall** | **Nagburim Prime** | Canonical region: Kellust / Dwarven stronghold |
| **Red Larch** | **Briarwatch** | Overland settlement |
| **Secomber** | **East Ubdina Bridge Market** | River/marsh trading post |
| **Shadowdale** | **Northroad Cellars** | Heartland dale settlement |
| **Soubar** | **Violet Lantern Market** | Trade outpost |
| **Tilverton** | **Memoria** | Shadowed / ruined city |
| **Zhentil Keep** | **Llawryn Keep** | Warlord fortress |

---

## 2. Pantheon and Deity Name Replacements

The divine pantheon was systematically mapped from Forgotten Realms deities to native Luminari deities with equivalent portfolios, clerical domains, and divine mechanics. See also `docs/lore_luminari/DEITIES.md` and `src/character/deities.c`.

| FR Deity | Luminari Deity | Divine Portfolio & Domains |
| :--- | :--- | :--- |
| **Auril** | **Glacius** | Goddess of winter, cold, and ice |
| **Azuth** | **Aethyra** | God of wizards and spellcraft |
| **Bane** | **Ghorak** | God of tyranny, conquest, and oppression |
| **Beshaba** | **Goddess of Misfortune** | Goddess of bad luck, misfortune, and accidents |
| **Bhaal** | **Velara** | God of murder, assassination, and violence |
| **Chauntea** | **Seraphine** | Goddess of agriculture, harvest, and cultivation |
| **Cyric** | **Vespera** | God of lies, deception, and strife |
| **Deneir** | **Aethyra** | God of writing, literacy, and literature |
| **Eldath** | **Myrr** | Goddess of peace, tranquility, and calm waters |
| **Gond** | **Borhild** | God of craft, smithing, and engineering |
| **Helm** | **Skarn** | God of protection, vigilance, and duty |
| **Ilmater** | **Orith** | God of endurance, suffering, and martyrs |
| **Kelemvor** | **Nethris** | God of the dead, judgment, and solemn transit |
| **Lathander** | **Seraphine** | God of birth, dawn, vitality, and renewal |
| **Leira** | **Vespera** | Goddess of illusion, mist, and false appearances |
| **Lliira** | **Willow** | Goddess of joy, celebration, and happiness |
| **Lloth** | **Spider Queen** | Goddess of spiders, darkness, chaos, and drow |
| **Loviatar** | **Yurga** | Goddess of pain, agony, and torment |
| **Malar** | **Zorren** | God of the hunt, savage beasts, and bloodlust |
| **Mask** | **Brandoc** | God of thieves, shadows, and stealth |
| **Mielikki** | **Viridara** | Goddess of forests, woodland creatures, and flora |
| **Milil** | **Lirael** | God of poetry, lyricism, and song |
| **Myrkul** | **Iluai** | God of death, decay, and the underworld |
| **Mystra** | **Aethyra** | Goddess of magic, the Weave, and arcana |
| **Oghma** | **Kaelthir** | God of knowledge, wisdom, and thought |
| **Savras** | **Kaelthir** | God of divination, truth, and fate |
| **Selune** | **Selithiel** | Goddess of the moon, navigation, and stars |
| **Shar** | **Nyxara** | Goddess of darkness, secrets, night, and loss |
| **Silvanus** | **Verdania** | God of wild nature, balance, and the wilds |
| **Sune** | **Calystral** | Goddess of love, beauty, and passion |
| **Talona** | **Yurga** | Goddess of disease, poison, and blight |
| **Talos** | **Thalassa** | God of storms, wrath, and devastation |
| **Tempus** | **Kordran** | God of war, warriors, and battle |
| **Torm** | **Kordran** | God of courage, honor, and self-sacrifice |
| **Tymora** | **Lumerion** | Goddess of good fortune, luck, and victory |
| **Tyr** | **Thalos** | God of justice, truth, and righteousness |
| **Umberlee** | **Thalassa** | Goddess of the sea, ocean depths, and currents |
| **Waukeen** | **Erix** | Goddess of trade, wealth, and commerce |

---

## 3. World Zones, Roads, Dungeons, and Landmarks (102 Mappings)

| # | Forgotten Realms Original | Luminari Replacement | Regional / Topological Context |
| :---: | :--- | :--- | :--- |
| 1 | **Amiskal's Keep** | **Void Crown Keep** | Void Crown strongholds |
| 2 | **Amphail** | **Vailand Horsefair** | Vailand equestrian plains |
| 3 | **Ardeep Forest** | **The Blueleaf Weald** | Ancient woodland weald |
| 4 | **Ashabenford** | **Three-Lamp Crossroads** | Regional trade intersection |
| 5 | **Aumvor's Castle** | **The Ossuary Eternal** | Necromantic stronghold |
| 6 | **Bargewright Inn** | **Violet Lantern Stockade** | Violet Lantern border waystation |
| 7 | **Battle of Bones** | **The Darkling Wastes** | Desolate battleground |
| 8 | **Bleak Palace** | **The Paradox Palace** | Eldritch planar palace |
| 9 | **Candlekeep Proper** | **The Floating Library of Chulan** | Citadel of arcanists & scholars |
| 10 | **Ch'Chitl** | **The Echo Chambers** | Subterranean illithid / aberration cavern |
| 11 | **City of Mirabar** | **Resonance Forgecity** | Kellust smithing and mining bastion |
| 12 | **Corm Orp Caverns** | **Selerish Undercliff Caverns** | Selerish coastal caves |
| 13 | **Dark Dominion** | **The Wound Beneath Old Anteria** | Underdark ruin |
| 14 | **Darklake** | **Lockwater Deep** | Subterranean lake |
| 15 | **Daurgothoth's Domain** | **Blackroot Sanctuary** | Draconic underways lair |
| 16 | **Dawn Pass & Lonely Moor** | **Hir's Western Road** | Hir overland transit |
| 17 | **Deep Eveningstar Halls** | **The Arcanite Workings** | Subterranean arcane delvings |
| 18 | **Delimbiyr Vale** | **Hir River Vale** | Hir river basin |
| 19 | **Delimiyr Route** | **Lanternwood Road** | Lanternwood highway |
| 20 | **Dragon Cult Fortress** | **Void Crown Fortress** | Dragon-aligned citadel |
| 21 | **Dragonspear Castle** | **Five-Scar Keep** | Ruined demon-scarred fortress |
| 22 | **Eveningstar** | **Blue Lantern Village** | Lantern Compact rural hub |
| 23 | **Eveningstar Haunted Halls** | **Echoing Cliffhold** | Cliffside ruins and catacombs |
| 24 | **Evereska Way** | **Quechian Covenant Way** | Quechian access highway |
| 25 | **Evermeet Ancient Forest** | **Quechian Ancient Darkwood** | Quechian ancestral darkwood |
| 26 | **Evermeet E Coast Rd N** | **Quechian East Canopy Road** | Quechian coastal thoroughfare |
| 27 | **Evermeet Main Rd** | **Quechian Root-City Road** | Quechian core highway |
| 28 | **Evermeet Misc Rooms/Mobs** | **Quechian Outlands** | Quechian periphery |
| 29 | **Evermeet Rd to Elven Settl** | **Quechian Rootward Road** | Quechian elven settlement access |
| 30 | **Evermeet W Coast Rd N** | **Quechian West Canopy Road** | Quechian western canopy path |
| 31 | **Flaming Tower** | **The Everforge Citadel** | Volcanic forge citadel |
| 32 | **Forest of Wyrms** | **East Ubdina Wyrmwood** | East Ubdina wilderness |
| 33 | **Greycloak Hills Side Path** | **Hir Low-Hill Side Road** | Hir hill trails |
| 34 | **Grunwald** | **Lantern Watch** | Lantern Compact watchfort |
| 35 | **Hardbuckler Caverns** | **Carstan Smugglers' Underways** | Carstan underworld passages |
| 36 | **Hellgate Keep** | **Ashfall Gate** | Fiendish ruins |
| 37 | **High Horn** | **The Harmonic Gates** | Mountain fortress pass |
| 38 | **Hulburg Trail** | **Lenadrian Ruined Trail** | Lenadrian coastal pass |
| 39 | **Iron Road** | **Kellust Stone Road** | Kellust dwarf highway |
| 40 | **Lost City of Thunderholme** | **Shatterdeep Necropolis** | Ancient dwarven ruins |
| 41 | **Luskan Outpost** | **Alfarth Watch-Fort** | Northern coastal bastion |
| 42 | **Luskan Southbank** | **Phoenix Crown Southbank** | Phoenix Crown riverside district |
| 43 | **Malaugrym Castle** | **The Inverse Citadel** | Shadow realm fortress |
| 44 | **Mantol-Derith** | **Nightmarket Trench** | Underdark neutral market |
| 45 | **Mantol-Derith Tunnels** | **Blackstone Transit Tunnels** | Subterranean merchant tunnels |
| 46 | **MarblePyramid HighForest E** | **Stilltear Grounds** | High Forest exterior pyramid grounds |
| 47 | **MarblePyramid HighForest I** | **Stilltear Sepulcher** | High Forest interior pyramid sepulcher |
| 48 | **Mere of Dead Men** | **West Ubdina Grave Marsh** | Coastal saltwater marshland |
| 49 | **Misty Forest** | **Quechian Twilight Forest** | Misty woodland weald |
| 50 | **Mithril Hall Palace** | **Nagburim Prime Palace** | Throne room of the dwarf lords |
| 51 | **Mount Hotenow** | **Kellust Silent Peak** | Volcanic mountain peak |
| 52 | **Pelleor's Prairie** | **Hir's Old War Prairie** | Grasslands battleground |
| 53 | **Road through Orlbar** | **Hir River Road** | River valley road |
| 54 | **Road West of Secomber** | **East Ubdina West Road** | East Ubdina highway |
| 55 | **Roads of Amn** | **Axtros Caravan Roads** | Southern trade highways |
| 56 | **Settlestone** | **Kellust Gatehold** | Mountain approaches fortress |
| 57 | **Sewers of CandleKeep** | **The Inkdrain** | Subterranean drainage of Chulan |
| 58 | **Skull Crag** | **Kellust Silent Pass** | Mountain gorge |
| 59 | **Skull Gorge** | **The Crimson Spindle** | Narrow canyon pass |
| 60 | **Skullport Heart** | **Inkwater** | Subterranean port hub |
| 61 | **Skullport Port & Island** | **Forgotten Reach** | Subterranean docks and islands |
| 62 | **Skullport Trade Lanes** | **Wakebound Trade Lanes** | Subterranean water channels |
| 63 | **Sloopdilmonpolop** | **Deepcurrent Haven** | Kuo-toa / aquatic cavern settlement |
| 64 | **Soubar Underhalls** | **The First Temple** | Subterranean shrine complex |
| 65 | **Temple of Ghaundaur** | **Draven's Chasm** | Slime deity chasm |
| 66 | **Tesh Trail** | **Vailand South Road** | Vailand southern highway |
| 67 | **Tethyamar Trail** | **Quechian Covenant Trail** | Mountain / forest boundary trail |
| 68 | **The Coast Way** | **Onduis Coast Way** | Western coastal highway |
| 69 | **The Dusk Road** | **Phoenix Crown Road** | Highway of the Phoenix Crown |
| 70 | **The Evermoor Way** | **Three-Ford Market Road** | Moorland overland route |
| 71 | **The Friendly Arm** | **The Open-Hand Roadhouse** | Fortified roadside inn |
| 72 | **The Halfway Inn** | **The Lantern Cup Inn** | Lanternway rest station |
| 73 | **The Labyrinth** | **Bonecoil Labyrinth** | Underdark labyrinthine maze |
| 74 | **The Lost Vale** | **Singing Orchard Vale** | Hidden idyllic valley |
| 75 | **The Lurkwood** | **East Ubdina Timber Road** | Northwood lumber trail |
| 76 | **The Moonsea Ride** | **Axtros March Road** | Eastern marsh highway |
| 77 | **The North Ride** | **Quechian Northroad** | Northern thoroughfare |
| 78 | **The Rat Hills** | **Cinderheap Warren** | Waste dunes & refuse hills |
| 79 | **The Rauvin Ride** | **Kellust Resonance Road** | Northern mountain valley road |
| 80 | **The Reaching Woods** | **Quechian Covenant Woods** | Protected ancient forest |
| 81 | **The Serpent Hills** | **Hir's Brown Hills** | Reptilian rolling hillcountry |
| 82 | **The Stonelands** | **The Crystal Frontier** | Rugged rocky wasteland |
| 83 | **The Tower of Twilight** | **The Threshold Citadel** | Arcane boundary spire |
| 84 | **The Trade Way II** | **Lantern Compact Road** | Primary north-south trade route |
| 85 | **The Vault of Ages** | **The Memory Vaults** | Ancient archival crypt |
| 86 | **The Way Inn** | **The Orchard Gate Inn** | Forest edge coaching inn |
| 87 | **Thethyr Bandit Castle** | **Hearthbreak** | Renegade castle stronghold |
| 88 | **Triel** | **Lantern Compact Granary** | Farming & storage crossroads |
| 89 | **Trollbark Forest** | **Thornwake Forest** | Monster-infested dense forest |
| 90 | **Trollclaws** | **Broken Span Road** | Jagged rock terrain & bridge ruins |
| 91 | **Ulcaster's College** | **The Archive That Defends Itself** | Ruined magical academy |
| 92 | **Undermountain [Level I]** | **The First Descent** | Primary mega-dungeon upper level |
| 93 | **Wormwrithings** | **Wormroad Deeps** | Purple worm tunnel network |
| 94 | **Yellow Snake Pass** | **Axtros Dry Pass** | Mountain gorge passage |
| 95 | **Zhent Graveyard** | **Bone Cathedral Necropolis** | Ancient dark mausoleums |
