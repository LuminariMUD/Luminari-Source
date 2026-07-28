# Web Account and Character Creation Media Manifest

**Status:** Production checklist - in delivery
**Date:** 2026-07-28
**Campaign baseline:** Default Luminari campaign
**Companion scope:**
[WEB_ACCOUNT_CHARACTER_CREATION_EXPERIENCE.md](WEB_ACCOUNT_CHARACTER_CREATION_EXPERIENCE.md)

## Delivery Status (2026-07-28)

The web client now has a working media pipeline, a versioned media manifest,
and a structured onboarding experience that consumes both. Assets are picked up
automatically as they are produced.

| Area | State |
|------|-------|
| Pipeline | `luminariweb/scripts/process-onboarding-media.mjs` converts `assets-unprocessed/` into `public/media/onboarding/v1/` and regenerates `src/features/onboarding/generated-media-manifest.json`. Idempotent; safe to re-run while art is still being produced. |
| Environment scenes | 9 of 9 composites delivered and published. Separate far/mid/fore/overlay layer masters are still outstanding; the client currently animates the composite plus shared particle layers. |
| Race art | Arriving. Delivered keys publish automatically; undelivered keys fall back to `race/fallback` and then to a labelled monogram frame. |
| Class art | `class/fallback` delivered; individual class packs outstanding. |
| Generic fallbacks | All eight delivered and wired. |
| Shared textures, motion layers, glyph sets | 6 textures, 6 motion sources, 5 glyph sets (33 SVGs) delivered with CC0/CC BY provenance. Attribution is generated to `public/media/onboarding/v1/ATTRIBUTION.md`. |
| Account, identity, build, preference, role-play, state art | Partially delivered; see the checklists below. |
| Sound effects | 61 of 61 delivered, published as Opus plus MP3. |
| Music | 8 of 8 delivered. |
| Ambience | 8 of 8 delivered. |
| Budgets | Enforced automatically. Oversized audio is re-encoded from its lossless master until it fits; a regression test asserts every published file against the budgets in this document. |
| Alt text | Authored for every delivered asset and pre-authored for the full race and class catalog, so new art is accessible the moment it lands. The pipeline reports any asset still missing alt text. |

Downstream gates that remain human responsibilities: creative listening and
viewing approval, originality and similarity review, licensing sign-off, canon
approval for the flagged ancestries, and final release approval.

## Purpose

This is the complete media-production checklist for the account login, account
lobby, core character creator, optional role-play profile, and handoff to play
described in the companion scope document.

The checklist covers:

- Still images and still-image layers used for motion-rich presentation
- Instrumental music
- Interface, transition, and environmental sound effects

The checklist deliberately excludes:

- Video
- Animated GIF, animated WebP, or other pre-rendered animation files
- Speech, voice-over, character barks, spoken whispers, or text-to-speech
- Persistent player avatars or uploaded portraits
- Art for prestige classes that cannot be selected during level-one creation

Motion is created in the web client by moving, scaling, blurring, masking, and
cross-fading still layers with CSS or the Web Animations API. The reduced-motion
experience uses the static composites from the same art packs.

## Checklist Meaning

A catalog asset checkbox is complete only when all deliverables for that asset
pack are complete. Checking off only the concept art is not sufficient.

### Scene pack

Every `scene/*` checkbox includes:

- [ ] 3840 x 2160 layered source master that passes autonomous validation
- [ ] Far/background still layer
- [ ] Midground still layer
- [ ] Foreground still layer with transparency
- [ ] Lighting or atmospheric overlay with transparency
- [ ] 16:9 desktop static composite
- [ ] 4:5 tablet static composite
- [ ] 3:4 or 9:16 mobile static composite
- [ ] Reduced-motion static composite
- [ ] Authored alt text, or empty alt text when wholly decorative
- [ ] Source, license, creator, prompt/process, and acceptance record

### Race and class art pack

Every `race/*` and `class/*` checkbox includes:

- [ ] 4:5 key-art master with safe crops for 16:9, 1:1, and 3:4
- [ ] Transparent subject or silhouette layer for restrained parallax
- [ ] Distinct one-color emblem in SVG
- [ ] Desktop detail image
- [ ] Mobile detail image
- [ ] Card thumbnail
- [ ] Authored alt text that describes the art without defining the player's
      appearance
- [ ] Canon, representation, source, license, and acceptance record

One pack serves every player sex. Do not create a male/female variant for every
race or class. Art should use ensembles, silhouettes, or a varied cast so that
the selection does not prescribe a player's exact appearance.

### Small catalog art pack

Every `background/*`, `alignment/*`, `age/*`, `faction/*`, and `deity/*`
checkbox includes:

- [ ] Square or 4:5 selection vignette
- [ ] One-color emblem in SVG
- [ ] Card thumbnail
- [ ] Authored alt text
- [ ] Source, license, and acceptance record

### Location art pack

Every `region/*` and `hometown/*` checkbox includes:

- [ ] 16:9 landscape master
- [ ] 3:4 mobile crop
- [ ] Card thumbnail
- [ ] Map marker or location emblem in SVG
- [ ] Authored alt text
- [ ] Lore-evidence, source, license, and acceptance record

### Audio asset

Every music or sound-effect checkbox includes:

- [ ] 48 kHz, 24-bit lossless WAV production master
- [ ] Web delivery encode in Ogg Opus
- [ ] Browser fallback encode that passes autonomous decode and quality checks
- [ ] Clean start and end; seamless loop where marked as a loop
- [ ] Loudness-normalized master with no clipping
- [ ] Title, composer/designer, license, source-session, and acceptance record
- [ ] Confirmation that the asset contains no speech

## Proposed Web Asset Contract

The web client should own the files and their presentation metadata. The MUD
should emit stable semantic keys such as `race/human` or `class/wizard`.

Recommended web location:

```text
public/media/onboarding/v1/
  brand/
  scene/
  shared/
  account/
  identity/
  race/
  class/
  build/
  alignment/
  roleplay/
  background/
  age/
  region/
  faction/
  hometown/
  deity/
  music/
  ambience/
  sfx/
```

The TypeScript media manifest should map each semantic key to:

- Poster and responsive image sources
- Optional transparent layers
- SVG emblem
- Width, height, and aspect ratio
- Alt text
- Dominant/accent colors
- Preload priority
- Reduced-motion behavior
- Asset version
- Provenance record identifier

Do not derive keys from display text. In particular, the current internal race
token for Tiefling is misspelled in one source assignment. The protocol should
emit an explicit stable `race/tiefling` media key.

Missing catalog media must resolve to the relevant fallback asset. It must
never produce a blank card or block character creation.

## Inventory Summary

Counts below are creative packs or audio cues, not the generated AVIF, WebP,
PNG, SVG, Opus, and fallback derivatives within each pack.

| Category | Required count | Notes |
|----------|---------------:|-------|
| Branded and shared images | 38 | Marks, fallbacks, textures, glyph sets, and motion layers |
| Environment scene packs | 9 | Layered still scenes; no video |
| Account and identity images | 17 | Lobby, connection, MOTD, name, and sex-choice presentation |
| Race art packs | 28 | Current default-campaign creation catalog |
| Class art packs | 18 | Current non-prestige, in-game base-class catalog |
| Build, alignment, and core-choice images | 19 | Includes all nine alignments |
| Role-play editor images | 9 | Shared profile and writing surfaces |
| Background art packs | 16 | Every current background archetype |
| Age art packs | 5 | Every current age band |
| Region/location packs | 14 | World map plus 13 selectable regions |
| Faction-specific packs | 2 minimum | No faction and current clan; shared fallback counted above |
| Hometown-specific packs | 1 | Ashenport; shared fallback counted above |
| Deity art packs | 22 | None plus 21 named deities; shared fallback counted above |
| Completion/recovery images | 8 | Save, fallback, disconnect, and error states |
| **Total still-image entries** | **206 minimum** | Before any additional runtime factions |
| Instrumental music cues | 8 | Opt-in and muted by default |
| Environmental ambience loops | 8 | Voice-free environmental sound |
| Interface and state sound effects | 61 | Short, nonverbal cues |
| **Total audio cues** | **77** | No speech |

## Image Sourcing Split

Use open/free assets for the generic building blocks:

- The six `shared/texture-*` assets
- `shared/motes-dust`, `shared/embers`, and `shared/stars`
- `shared/fog-far`, `shared/fog-near`, and `shared/light-rays`
- The five `shared/glyph-*` sets

Prefer CC0. CC BY is acceptable when its attribution is recorded. Do not use
assets with unclear, NonCommercial, or NoDerivatives terms.

Use image generation for the distinctive Luminari illustrations:

- All `scene/*`, `race/*`, and `class/*` art
- The raster art for generic fallbacks
- Unique account, identity, build, role-play, background, and deity
  illustration masters
- Region landscapes after autonomous canon extraction from repository evidence;
  use the region fallback when evidence remains absent

Brand marks, emblems, frames, alignment seals, and similar SVG graphics should
be drawn directly. Crops, thumbnails, state changes, and responsive versions
should reuse an accepted master rather than trigger another acquisition or
generation task.

### Downloadable Visual Source Acquisition

**Acquisition status (2026-07-27):** Complete, 17 of 17 download-eligible
visual entries sourced and accepted by autonomous validation.

The accepted raw-source package is staged at:

```text
/home/aiwithapex/projects/luminariweb/assets-unprocessed/
  web-onboarding-visual-downloads-v1/
```

| Downloadable scope | Acquired | Accepted sources | License |
|--------------------|---------:|------------------|---------|
| Six `shared/texture-*` entries | 6 / 6 | ambientCG 2K PBR materials | CC0 1.0 |
| Six reusable motion-layer entries | 6 / 6 | Kenney and OpenGameArt | CC0 1.0 |
| Five `shared/glyph-*` sets, 33 SVGs total | 5 / 5 | Game-icons.net | CC BY 3.0 |

The package includes the original source archives, selected source files,
license texts, exact creator/source mapping, archive checksums, QA results, and
downstream automated production checks. All eight ZIP archives passed integrity
checks;
the six texture color maps decode at 2048 x 2048; all 25 motion PNGs have
non-opaque alpha; and all 33 unique, one-color SVGs passed structure and render
checks.

This closes open/free asset discovery and acquisition only. The production
checkboxes below remain unchecked until the source files are converted into
the required atlases, scene-sized layers, responsive exports, accessible
interface mappings, credits, and web assets that pass the automated acceptance
suite. In particular, the downloaded light-ray image is accepted as alpha-mask
geometry; the pipeline must replace its original yellow source color.
Game-icons.net creator credits are mandatory under CC BY 3.0 and are enumerated
in the package's
`glyph-catalog.json`.

No other still-image entry in this manifest is assigned to downloaded stock.
The remaining entries require original generation, direct SVG drawing,
evidence-bound map/region work, or derivatives from a separately accepted
master. No item waits for human approval: the autonomous pipeline accepts,
regenerates, falls back, or rejects from recorded evidence and deterministic
checks.

The current faction list is runtime-authored and can change. Before final
validation, export the live emitted faction catalog and add one pack for every
additional faction. The generic faction pack remains required even after all
known factions have art.

## Image Checklist

### 1. Brand and global identity

- [ ] `brand/luminari-primary` - Full-color Luminari wordmark for arrival and
      login; wide and centered lockups.
- [ ] `brand/luminari-mark` - Standalone Luminari mark for compact headers,
      loading, and mobile.
- [ ] `brand/luminari-seal` - Ornamental world seal for confirmations and the
      account lobby.
- [ ] `brand/luminari-monochrome-light` - One-color light mark for dark art.
- [ ] `brand/luminari-monochrome-dark` - One-color dark mark for light panels.
- [ ] `brand/loading-sigil` - Still concentric sigil layers that the client can
      rotate independently.
- [ ] `brand/social-card` - Static 1200 x 630 sharing and install-preview art;
      contains no private account or character data.
- [ ] `brand/app-icon-set` - Validated favicon and install icons derived from the
      standalone mark.

### 2. Generic catalog and privacy fallbacks

- [x] `shared/character-fallback` - Anonymous adventurer silhouette for account
      cards with unavailable or private character art.
- [x] `race/fallback` - Unknown ancestry silhouette and neutral origin emblem.
- [x] `class/fallback` - Unassigned path silhouette and neutral class emblem.
- [x] `background/fallback` - Blank journal and unmarked wax seal.
- [x] `region/fallback` - Uncharted landscape and map-pin emblem.
- [x] `faction/fallback` - Unmarked banner and shield.
- [x] `hometown/fallback` - Distant neutral settlement silhouette.
- [x] `deity/fallback` - Unrecognized shrine and blank holy-symbol frame.

### 3. Shared surface textures

These must tile cleanly or have sufficiently large masters to avoid visible
repetition.

- [x] `shared/texture-parchment` - Low-contrast writing-panel texture.
- [x] `shared/texture-dark-stone` - Lobby and modal frame texture.
- [x] `shared/texture-brushed-metal` - Class and build accent texture.
- [x] `shared/texture-woven-cloth` - Banner and faction-card texture.
- [x] `shared/texture-aged-map` - Region and hometown map texture.
- [x] `shared/texture-arcane-glass` - Translucent magical panel treatment.
- [ ] `shared/frame-ornament` - Scalable corner and divider ornaments in SVG.
- [ ] `shared/card-rune-border` - Selectable, selected, locked, and error border
      states in SVG.

### 4. Reusable still layers for client-driven motion

- [x] `shared/motes-dust` - Transparent dust/mote sprite atlas.
- [x] `shared/motes-arcane` - Transparent arcane-light sprite atlas.
- [x] `shared/embers` - Transparent ember sprite atlas.
- [x] `shared/stars` - Sparse transparent star field.
- [x] `shared/fog-far` - Seamless low-frequency fog layer.
- [x] `shared/fog-near` - Seamless foreground fog layer.
- [x] `shared/light-rays` - Soft transparent light-ray overlay.
- [ ] `shared/rune-ring` - Separate concentric SVG rune rings for rotation.
- [x] `shared/ink-bloom` - Still transparent ink shapes for reveal masks.

Do not render locked, disabled, selected, or hovered duplicates of catalog art.
Those states should be created with CSS masks, borders, color treatment, and
the shared layers above.

### 5. Functional SVG glyph sets

- [x] `shared/glyph-abilities` - Strength, Dexterity, Constitution,
      Intelligence, Wisdom, and Charisma.
- [x] `shared/glyph-class-mechanics` - Hit die, base attack bonus, Fortitude,
      Reflex, Will, skills, spellcasting, armor, and primary attributes.
- [x] `shared/glyph-race-traits` - Size, ability modifier, level adjustment,
      racial feature, language, alignment restriction, and unlock cost.
- [x] `shared/glyph-session-state` - Network connected, authenticated account,
      selected character, incomplete onboarding, and playing.
- [x] `shared/glyph-audio-controls` - Muted, unmuted, music, ambience, sound
      effects, and volume.

Every glyph needs an accessible text label in the interface. The image alone
must never communicate the mechanical value or session state.

### 6. Environment scene packs

Each entry is a complete scene pack as defined above.

- [x] `scene/arrival` - Ashenport-facing arrival vista at blue hour; wonder,
      distance, and a clear dark safe zone for the login panel.
- [x] `scene/authentication` - Sheltered gatehouse or waystation derived from
      the arrival vista; private, calm, and free of visible written passwords.
- [x] `scene/account-lobby` - Warm guild hall or archive of adventurers with
      open wall space for character cards.
- [x] `scene/identity` - Scriptorium with blank ledger, quill, and lamplight;
      no readable pre-rendered names.
- [x] `scene/race-gallery` - Hall of origins with neutral architecture that
      does not privilege one race.
- [x] `scene/class-gallery` - Hall of paths combining armory, library, shrine,
      workshop, and wilderness motifs without favoring one class.
- [x] `scene/build-alignment` - Celestial loom or compass chamber with nine
      readable positions and a clear central summary area.
- [x] `scene/roleplay` - Quiet writing chamber with map table, journal, and
      symbolic keepsakes.
- [x] `scene/handoff` - Open gate or threshold leading from the creator into
      the game world; success is shown only after server-confirmed save.

### 7. Connection and account-state images

- [x] `account/connecting` - Closed gate with an unlit sigil.
- [x] `account/connected` - Lit gate sigil; means network connection only, not
      authenticated account access.
- [x] `account/reconnecting` - Partially relit gate; visually distinct from the
      first connection.
- [x] `account/disconnected` - Dormant gate and safe return path.
- [x] `account/classic-terminal` - Terminal-window emblem used for the explicit
      classic-flow action.
- [x] `account/new-account` - Blank account ledger and new seal.
- [x] `account/empty-lobby` - Empty display plinth inviting the first character.
- [x] `account/full-lobby` - Full archive shelf or completed roster; warning,
      not celebration.
- [x] `account/link-character` - Two linked seals for the advanced legacy
      character-link flow.
- [x] `account/character-card-frame` - Responsive decorative frame that can
      combine race key art, a class emblem, name, level, and status.
- [x] `account/private-input` - Veil or closed-eye emblem for password mode;
      decorative support only, never the sole indication of sensitive input.
- [x] `account/motd-frame` - Quiet responsive frame for live MOTD text; contains
      no rasterized message text.
- [x] `account/selected-character-menu` - Detailed character folio frame for
      Play and current character-menu actions.

### 8. Name and sex-choice images

- [x] `identity/name` - Blank nameplate, quill, and wax seal; no pre-rendered
      example name in the raster image.
- [x] `identity/name-confirmed` - Sealed nameplate for server-confirmed
      acceptance.
- [x] `identity/sex-male` - Respectful, abstract male identity emblem and card
      vignette; does not imply race, class, build, or body type.
- [x] `identity/sex-female` - Respectful, abstract female identity emblem and
      card vignette; does not imply race, class, build, or body type.

The current server exposes only male and female at this step. The art manifest
must follow the emitted server choices and must not promise additional values
until the MUD supports them.

### 9. Race gallery: 28 art packs

Each entry includes the complete race/class pack deliverables. Race art depicts
the ancestry as a broad possibility, not a mandatory player portrait.

When appearance canon is incomplete, the autonomous pipeline must search the
repository's code, world data, help, and lore documentation; express only
traits supported by that evidence; and otherwise use `race/fallback`. Missing
canon is an evidence condition with a deterministic fallback, never a human
approval gate.

- [x] `race/human` - Human; adaptable adventuring ensemble with mixed callings
      and an Ashenport-world connection.
- [x] `race/moon-elf` - Moon Elf; lunar, contemplative, and distinct from the
      High Elf and Wild Elf visual languages.
- [x] `race/mountain-dwarf` - Mountain Dwarf; enduring mountain craft and
      defensive strength.
- [x] `race/half-troll` - Half Troll; powerful mixed heritage presented as a
      person, not a mindless monster.
- [x] `race/crystal-dwarf` - Crystal Dwarf; epic crystalline craft and mineral
      light. Use evidence-bound mineral features or `race/fallback` when the
      repository does not define an appearance.
- [x] `race/lightfoot-halfling` - Lightfoot Halfling; agile traveler, warmth,
      and curiosity.
- [x] `race/half-elf` - Half Elf; blended heritage without visually reducing
      the character to either parent ancestry.
- [x] `race/half-orc` - Half Orc; resilient adventurer without automatic savage
      or villain framing.
- [x] `race/rock-gnome` - Rock Gnome; craft, curiosity, and practical invention.
- [x] `race/trelux` - Trelux; epic ancestry. A canon silhouette, anatomy,
      material, palette, and cultural treatment must be derived from repository
      evidence; otherwise use `race/fallback`.
- [x] `race/arcana-golem` - Arcana Golem; constructed arcane personhood, visible
      magical structure, and no generic robot shorthand. Use only
      repository-supported structure or `race/fallback`.
- [x] `race/drow` - Drow; subterranean elven identity without automatic villain
      framing.
- [x] `race/duergar` - Duergar; subterranean dwarven identity, austere craft,
      and distinct silhouette from Mountain and Gold Dwarves.
- [x] `race/high-elf` - High Elf; refined arcane or civic tradition distinct
      from Moon and Wild Elves.
- [x] `race/wild-elf` - Wild Elf; woodland movement and self-reliance without
      primitive caricature.
- [x] `race/half-drow` - Half Drow; mixed heritage with a distinct identity and
      without automatic villain framing.
- [x] `race/dragonborn` - Dragonborn; draconic ancestry and heroic personhood;
      derive scale, horn, and color range from repository evidence and fall
      back when the evidence is absent.
- [x] `race/tiefling` - Tiefling; planar heritage with varied heroic
      possibilities. Use the corrected stable media key.
- [x] `race/stout-halfling` - Stout Halfling; sturdy community and grounded
      courage, visibly distinct from Lightfoot Halfling.
- [x] `race/forest-gnome` - Forest Gnome; woodland craft, concealment, and
      animal affinity distinct from Rock Gnome.
- [x] `race/gold-dwarf` - Gold Dwarf; ceremonial metalwork and established
      tradition distinct from Mountain and Crystal Dwarves.
- [x] `race/aasimar` - Aasimar; celestial heritage through restrained light,
      not a fixed angel costume.
- [x] `race/tabaxi` - Tabaxi; agile feline personhood, curiosity, and travel;
      derive coat and anatomy range from repository evidence and fall back when
      the evidence is absent.
- [x] `race/goliath` - Goliath; mountain endurance and scale without reducing
      the ancestry to brute force.
- [x] `race/shade` - Shade; shadow-touched personhood and controlled negative
      space. Use evidence-bound origin and anatomy or `race/fallback`.
- [x] `race/fae` - Fae; tiny scale, flight, glamour, and alien woodland magic.
      Use evidence-bound wing and scale treatment or `race/fallback`.
- [x] `race/goblin` - Goblin; clever playable personhood without disposable
      enemy framing.
- [x] `race/hobgoblin` - Hobgoblin; disciplined martial culture and distinct
      stature from Goblin without automatic villain framing.

Do not produce Lich or Vampire creation cards for this scope. They are
outside the default campaign's `0..NUM_RACES-1` creation list even though race
records exist elsewhere in the source.

### 10. Class gallery: 18 art packs

Each entry includes the complete race/class pack deliverables. Art must remain
usable for any compatible race and sex.

- [x] `class/wizard` - Studied arcane mastery, spellbook or equivalent focus,
      and deliberate control.
- [x] `class/cleric` - Divine channeling and service without depicting one
      specific deity as mandatory.
- [x] `class/rogue` - Skill, mobility, tools, and opportunism without making
      criminality mandatory.
- [x] `class/warrior` - Broad weapons-and-armor mastery rather than one fixed
      weapon style.
- [ ] `class/monk` - Disciplined unarmed motion, balance, and inner focus.
- [ ] `class/druid` - Nature magic and shapechanging suggested through layered
      animal and elemental forms.
- [ ] `class/berserker` - Controlled battle rage, momentum, and resilience
      without mindless caricature.
- [ ] `class/sorcerer` - Innate arcane power emerging from within, visually
      distinct from the Wizard.
- [ ] `class/paladin` - Protective holy champion and radiant defense without
      binding the art to one race or deity.
- [ ] `class/blackguard` - Dark divine champion, fear, and command; maintain
      readability without relying on pure black.
- [ ] `class/ranger` - Tracking, wilderness mastery, and flexible ranged or
      dual-weapon combat.
- [ ] `class/bard` - Performance, inspiration, lore, and magic without choosing
      one mandatory instrument.
- [ ] `class/psionicist` - Focused mental power, geometric thought, and altered
      reality distinct from arcane spell effects.
- [ ] `class/alchemist` - Extracts, mutagens, bombs, and a controlled workshop
      silhouette.
- [ ] `class/inquisitor` - Investigation, judgment, divine pursuit, and
      determination.
- [ ] `class/summoner` - Bond between summoner and eidolon without defining one
      permanent eidolon anatomy.
- [ ] `class/warlock` - Pact-driven power and dangerous magic without depicting
      one mandatory patron.
- [ ] `class/artificer` - Magical engineering, tools, and devices distinct from
      the Alchemist.

Existing account characters may have prestige or multiclass summaries. Their
lobby cards should compose `shared/character-fallback`, race art, and available
base-class emblems. Do not block a card because prestige-class art is absent.

### 11. Build, alignment, preferences, and role-play decision

- [ ] `build/premade` - Clearly guided path with a completed map, steady
      lantern, or assembled kit.
- [ ] `build/custom` - Player-directed path with open tools, branching plan,
      or unassembled kit.
- [ ] `build/compare` - Neutral split-path illustration used when comparing the
      two choices.

- [ ] `alignment/lawful-good` - Ordered good seal; readable with text and
      without color.
- [ ] `alignment/neutral-good` - Balanced good seal; readable with text and
      without color.
- [ ] `alignment/chaotic-good` - Free good seal; readable with text and without
      color.
- [ ] `alignment/lawful-neutral` - Ordered neutral seal; readable with text and
      without color.
- [ ] `alignment/true-neutral` - Centered neutral seal; readable with text and
      without color.
- [ ] `alignment/chaotic-neutral` - Free neutral seal; readable with text and
      without color.
- [ ] `alignment/lawful-evil` - Ordered evil seal; readable with text and
      without color.
- [ ] `alignment/neutral-evil` - Balanced evil seal; readable with text and
      without color.
- [ ] `alignment/chaotic-evil` - Free evil seal; readable with text and without
      color.
- [ ] `alignment/compass` - One nine-position static compass plate on which the
      client places the individual seals.

- [ ] `preferences/recommended` - Coherent bundle or packed adventuring kit.
- [ ] `preferences/manual` - Open controls or unpacked kit; does not suggest the
      player is making a wrong choice.
- [ ] `roleplay/choice-roleplayer` - Open journal, character keepsake, and
      invitation to define identity.
- [ ] `roleplay/choice-non-roleplayer` - Direct road into play; respectful and
      equal in visual weight.
- [ ] `roleplay/choice-later` - Bookmarked journal that can be resumed later.
- [ ] `roleplay/core-summary` - Composite summary frame for name, race, class,
      build, and alignment. It must not imply unsupported Back editing.

### 12. Role-play editor shared images

- [ ] `roleplay/profile-hub` - Character-options desk with fourteen clearly
      separated symbolic slots.
- [ ] `roleplay/short-description` - Anonymous character silhouette with
      descriptor tags; not a persistent avatar builder.
- [ ] `roleplay/long-description` - Full blank character folio.
- [ ] `roleplay/background-story` - Bound journal with blank pages.
- [ ] `roleplay/goals` - Distant marker, path, and star.
- [ ] `roleplay/personality` - Faceted but coherent mask or mirrored portrait.
- [ ] `roleplay/ideals` - Guiding flame or compass-star.
- [ ] `roleplay/bonds` - Interlinked keepsakes or threads.
- [ ] `roleplay/flaws` - Repaired fracture or shadowed mirror; avoid stigmatizing
      imagery.

All free-text editor art must keep the writing area visually quiet and must not
contain rasterized example text.

### 13. Background archetypes: 16 small catalog packs

- [ ] `background/acolyte` - Temple service, ritual light, and sacred study.
- [ ] `background/charlatan` - Disguise, cards, or a convincing false remedy.
- [ ] `background/criminal-spy` - Coded message, lock tools, and hidden route.
- [ ] `background/entertainer` - Stage light and varied performance tools.
- [ ] `background/folk-hero` - Humble village token and symbol of local courage.
- [ ] `background/gladiator` - Arena sand, showmanship, and martial spectacle.
- [ ] `background/hermit` - Secluded shelter, lamp, and solitary discovery.
- [ ] `background/noble` - Signet, estate record, and civic responsibility.
- [ ] `background/outlander` - Trail markers and survival gear beyond a city.
- [ ] `background/pirate` - Weathered chart, cut rope, and open sea.
- [ ] `background/sage` - Manuscripts, diagrams, and patient research.
- [ ] `background/sailor` - Working vessel, knots, and horizon distinct from the
      Pirate.
- [ ] `background/soldier` - Campaign kit, banner remnant, and disciplined
      service.
- [ ] `background/squire` - Maintained armor, training weapon, and unfinished
      heraldry.
- [ ] `background/trader` - Scales, trade ledger, and artisan goods.
- [ ] `background/urchin` - Rooftop route, improvised kit, and survival without
      demeaning poverty.

The pipeline must pass the background names and descriptions through automated
content, similarity, and licensing checks before public web publication.
Produce original art; do not copy illustrations from tabletop books or other
games.

### 14. Character age: 5 small catalog packs

- [ ] `age/adult` - Established adult life stage; neutral silhouette.
- [ ] `age/adolescent` - Younger life stage; nonsexualized neutral silhouette.
- [ ] `age/middle-aged` - Midlife stage expressed without caricature.
- [ ] `age/old-aged` - Older life stage expressed with capability and dignity.
- [ ] `age/venerable` - Venerable life stage expressed with dignity and
      experience.

Age art must work across all playable ancestries. Use symbolic life-stage
motifs rather than a human-only aging sequence.

### 15. Homeland regions: map plus 13 location packs

- [ ] `region/luminari-world-map` - Canonical world/continent map with a quiet
      base and separate SVG markers. Do not invent borders or geography.
- [ ] `region/ashenport` - Ashenport region landscape; major port and starting
      hub.
- [ ] `region/sanctus` - Sanctus region landscape; major city identity.
- [ ] `region/onduis` - Evidence-bound landscape and marker.
- [ ] `region/selerish` - Evidence-bound landscape and marker.
- [ ] `region/carstan` - Evidence-bound landscape and marker.
- [ ] `region/axtros` - Evidence-bound landscape and marker.
- [ ] `region/hir` - Evidence-bound landscape and marker.
- [ ] `region/quechian` - Evidence-bound landscape and marker.
- [ ] `region/vailand` - Evidence-bound landscape and marker.
- [ ] `region/oorpii` - Evidence-bound landscape and marker.
- [ ] `region/kellust` - Evidence-bound landscape and marker.
- [ ] `region/east-ubdina` - Evidence-bound landscape and marker.
- [ ] `region/west-ubdina` - Evidence-bound landscape and marker.

Autonomous no-invention policy: default-campaign `get_region_info()` currently
returns the same "not yet available" placeholder for every region. The pipeline
must derive geography, climate, culture, architecture, palette, and boundaries
from other authoritative repository sources. If those facts remain absent, map
the region to `region/fallback`, record the evidence gap, and continue without
blocking the job.

### 16. Factions: current runtime catalog plus fallback

- [ ] `faction/adventurer` - No faction; independent road, blank banner, and no
      implied political allegiance.
- [ ] `faction/pyrets-pirates` - Crest and banner for the current clan entry
      "Pyret's Pirates"; preserve the runtime spelling and use only
      repository-supported heraldry, otherwise use `faction/fallback`.
- [ ] Export the runtime faction catalog immediately before final validation.
- [ ] Add a small catalog pack for every additional emitted faction ID.
- [ ] Verify that removed or renamed factions retain a safe fallback mapping.

The role-play flow reads runtime `clan_list` data. The static `factions[]`
constant is not the authoritative menu, so do not produce The Order,
Darklings, or Criminals solely from that constant. The required
`faction/fallback` pack is listed under generic catalog fallbacks.

### 17. Hometowns: current selector plus fallback

- [ ] `hometown/ashenport` - Bustling port-city establishing image, distinct
      enough to serve as the selected hometown confirmation.

Although the default `cities[]` array also names Sanctus, the current default
role-play parser accepts only Ashenport. Reuse `region/sanctus` if Sanctus later
becomes an emitted hometown; do not present it as selectable before then. The
required `hometown/fallback` pack is listed under generic catalog fallbacks.

### 18. Deities: None plus 21 Luminari small catalog packs

Each named deity pack should center the holy symbol described in the
authoritative deity text. The vignette supports the symbol and portfolio; it
must not define a single mandatory bodily depiction of the deity.

- [ ] `deity/none` - Empty shrine or unmarked stone for no deity; neutral, not
      ominous or dismissive.
- [ ] `deity/aethyra` - Lady of the Loom; magic, oaths, hidden patterns, memory,
      and the Weave.
- [ ] `deity/nethris` - Gravewarden; death, fate, proper rites, and endings.
- [ ] `deity/seraphine` - Dawnstar; sun, renewal, harvest, redemption, and
      second chances.
- [ ] `deity/kaelthir` - Starwarden; ancient knowledge, time, prophecy, and old
      names.
- [ ] `deity/pyrion` - First Flame; primal fire, creation through destruction,
      forge heat, and passion.
- [ ] `deity/vaelith` - Whispering Tide; sea, storms, currents, horizons, and
      the deep.
- [ ] `deity/orith` - Stonefather; earth, mountains, patience, endurance, and
      shelter.
- [ ] `deity/aerion` - Swiftwind; air, migratory paths, messengers, and true
      tidings.
- [ ] `deity/kordran` - Hammer of Dawn; war, duty, resolve, banners, and
      righteous defense.
- [ ] `deity/thalos` - Scales Unblinking; judgment, law, scholarship, cities,
      and records.
- [ ] `deity/lumerion` - Lantern-Bearer; luck, travel, wayfinding, small
      chances, and crossroads.
- [ ] `deity/nyxara` - Of the Veil; night, secrets, thresholds, lost names, and
      hidden knowledge.
- [ ] `deity/myrr` - Quiet Brook; peace, healing, sanctuaries, kindness, and
      wells.
- [ ] `deity/vespera` - Of the Many Masks; trickery, performance, reinvention,
      and revolution through mockery.
- [ ] `deity/calystral` - Flameheart; love, art, passion, creative fire, and
      beauty.
- [ ] `deity/borhild` - Emberforge; forgecraft, innovation, consecrated labor,
      and dwarven craft.
- [ ] `deity/selithiel` - Moonbough; moon, dreams, passage between worlds, and
      elven mysticism.
- [ ] `deity/pella` - Of the Warm Hearth; home, hospitality, safe roads, and
      halfling community.
- [ ] `deity/gearmaster` - Gnome patron; invention, humor, gems, and discovery.
- [ ] `deity/ghorak` - Ash-Eyed; conquest, strength, survival, and taking the
      strong ground.
- [ ] `deity/zorren` - Lord of the Wild Hunt; beasts, moonlit pursuit, freedom,
      and the hunter's bond.

### 19. Completion, error, and recovery images

- [ ] `state/validation-error` - Broken but repairable seal; used only with
      readable server error text.
- [ ] `state/choice-locked` - Locked seal overlay; reason and cost remain text.
- [ ] `state/save-pending` - Unfinished seal; must not look successful.
- [ ] `state/save-confirmed` - Completed seal and open path; shown only after
      authoritative server confirmation.
- [ ] `state/save-failed` - Preserved draft and blocked seal; avoid destructive
      imagery unless data loss is confirmed.
- [ ] `state/onboarding-incomplete` - Bookmarked or unfinished character folio.
- [ ] `state/protocol-fallback` - Structured frame folding safely into the
      classic terminal.
- [ ] `state/server-unavailable` - Quiet dormant gateway with reconnect and
      terminal recovery space.

## Music Checklist

All music is instrumental, has no spoken or sung words, starts only after
explicit player opt-in, and is muted by default. Do not create one track per
race or class. Shared cues keep the download and composition scope bounded.

- [x] `music/arrival-theme` - 90 to 120 second seamless loop; restrained main
      Luminari motif, mystery, distance, and welcome. No bombastic combat tone.
- [x] `music/account-lobby` - 90 to 120 second seamless loop; warm archive or
      hearth arrangement with room for repeated listening.
- [x] `music/identity` - 75 to 105 second seamless loop; sparse quill, memory,
      and first-step mood for name and sex selection.
- [x] `music/origins` - 120 to 150 second seamless loop; broad, culturally
      neutral sense of ancestry and discovery for the race gallery.
- [x] `music/paths` - 120 to 150 second seamless loop; purposeful rhythm and
      restrained magical/martial color for the class gallery.
- [x] `music/loom` - 90 to 120 second seamless loop; measured pattern and moral
      tension for build, alignment, and final core summary.
- [x] `music/roleplay` - 120 to 180 second seamless loop; intimate, low-density
      writing music for profile editors and catalog choices.
- [x] `music/handoff` - 45 to 75 second non-looping transition cue with a
      loop-safe tail; resolves the motif only after save confirmation.

### Music implementation and QA

- [ ] Music never autoplays on first visit.
- [ ] Music has an obvious mute control and independent volume slider.
- [ ] The opt-in state can persist; playback position does not need to persist.
- [ ] Music pauses when the tab is hidden and resumes without overlapping.
- [ ] Cross-fades do not delay input or server-state transitions.
- [ ] Reduced motion does not automatically enable or disable audio.
- [ ] Data-saver mode does not fetch music until the player explicitly starts it.
- [ ] Failure and password screens never use a startling musical sting.
- [ ] No music is required to understand state, validation, or progress.

## Environmental Ambience Checklist

Ambience is categorized as sound effects, not music. Every loop must be
voice-free: no intelligible crowds, tavern speech, chants, whispers, or calls.

- [x] `ambience/arrival-harbor` - Soft wind, distant water, rigging, and
      occasional wood movement; no sailors or crowd voices.
- [x] `ambience/auth-gatehouse` - Sheltered wind, low room tone, and distant
      mechanical gate resonance.
- [x] `ambience/account-lobby` - Hearth, subtle timber, and page movement; no
      tavern or guild voices.
- [x] `ambience/identity-scriptorium` - Lamp, paper, and quiet room tone.
- [x] `ambience/race-gallery` - Spacious hall tone, restrained magical motes,
      and air movement.
- [x] `ambience/class-gallery` - Distant forge, page, leather, and magical-room
      textures kept abstract and balanced.
- [x] `ambience/loom-chamber` - Low magical resonance, thread tension, and
      subtle stone movement.
- [x] `ambience/roleplay-room` - Hearth, quill, paper, map, and quiet rain or
      wind without voices.

### Ambience implementation and QA

- [ ] Ambience follows the master audio opt-in and has its own level relative to
      music.
- [ ] Loops are seamless and at least 30 seconds long to avoid obvious
      repetition.
- [ ] Only the active scene loop is decoded and playing.
- [ ] Scene changes cross-fade; loops never stack after reconnect.
- [ ] Ambience pauses in background tabs.
- [ ] No ambience cue resembles a server warning or validation sound.

## Generated Music and Ambience Delivery

**Generated:** 2026-07-27
**Source session:** `onboarding-audio-2026-07-27T20-39-14-892Z`
**Provider:** MusicAPI.ai Sonic using the Suno generator
**Model:** `sonic-v5-5`
**Delivery:** `/home/aiwithapex/projects/luminariweb/assets-unprocessed/onboarding-audio-v1`
**Provenance:** `onboarding-audio-v1/provenance.json`
**Provenance SHA-256:** `54c4d51208aed4a1d04db52aa9c14221b6bb3b69b31429c33399094bc310bc33`

The source session completed 16 generation tasks and 16 selected-source WAV
requests, consuming 256 provider credits. It produced all 8 music cues and all
8 ambience loops in this manifest. No interface or state sound-effect cue was
generated.

Each delivered asset package contains:

- [x] Selected provider WAV source
- [x] Every technically eligible retained provider MP3 candidate
- [x] Exact-duration 48 kHz stereo, 24-bit PCM production master
- [x] Ogg Opus web delivery encode
- [x] MP3 browser fallback encode
- [x] Full-decode, duration, format, loudness, headroom, loop-boundary, and
      SHA-256 records
- [x] Title, designer, license basis, source task, source clip, prompt,
      parameters, model, and terms-review record
- [ ] Human speech and unwanted-vocal listening review
- [ ] Human musical or environmental fit review
- [ ] Human similarity, originality, and final rights review
- [ ] Production approval

The asset checkboxes in the two sections above remain open because this
manifest defines them as production-complete only after approval. Their files
are generated and technically prepared; the remaining work is review, not
missing asset creation.

| Asset | Master duration | Integrated loudness | Technical status |
|-------|----------------:|--------------------:|------------------|
| `music/arrival-theme` | 105 s | -18.08 LUFS | Passed |
| `music/account-lobby` | 105 s | -18.12 LUFS | Passed |
| `music/identity` | 90 s | -18.11 LUFS | Passed |
| `music/origins` | 135 s | -17.84 LUFS | Passed |
| `music/paths` | 135 s | -18.05 LUFS | Passed |
| `music/loom` | 105 s | -18.07 LUFS | Passed |
| `music/roleplay` | 150 s | -18.31 LUFS | Passed |
| `music/handoff` | 60 s | -18.24 LUFS | Passed |
| `ambience/arrival-harbor` | 60 s | -26.24 LUFS | Passed |
| `ambience/auth-gatehouse` | 60 s | -26.19 LUFS | Passed |
| `ambience/account-lobby` | 60 s | -26.44 LUFS | Passed |
| `ambience/identity-scriptorium` | 60 s | -25.84 LUFS | Passed |
| `ambience/race-gallery` | 60 s | -26.20 LUFS | Passed |
| `ambience/class-gallery` | 60 s | -26.07 LUFS | Passed |
| `ambience/loom-chamber` | 60 s | -26.09 LUFS | Passed |
| `ambience/roleplay-room` | 60 s | -26.14 LUFS | Passed |

## Sound-Effect Checklist

All sound effects are short, nonverbal, and optional. Important state always
has a visible and screen-reader-accessible equivalent.

**Production status (2026-07-27):** All 61 `sfx/*` cues below were generated
with ElevenLabs Sound Effects v2 and delivered to
`/home/aiwithapex/projects/luminariweb/assets-unprocessed/web-onboarding-sfx-v1`.
Each cue has its original 48 kHz stereo PCM response, a 48 kHz 24-bit WAV
master, a 48 kHz Ogg Opus encode, a 48 kHz MP3 fallback, and source/technical
QA records. All 61 passed decode, duration, byte-size, signal, and clipping
checks. A whole-batch automated transcription screen detected no intelligible
speech. Originality/similarity, perceptual audio quality, in-product mix,
account eligibility, and release acceptance are autonomous downstream checks;
none requires a human listener or sign-off. Environmental ambience is
intentionally excluded from this Sound Effects job.

The reproducible cue briefs and generation parameters are recorded in
[elevenlabs-onboarding-sfx-catalog.json](../../media-gen/elevenlabs-onboarding-sfx-catalog.json).

### 1. Connection and transport

- [x] `sfx/connection-start` - Soft gateway energizing; low urgency.
- [x] `sfx/connection-ready` - Clean two-note network-ready cue; does not imply
      authentication.
- [x] `sfx/connection-lost` - Gentle descending resonance; not an alarm.
- [x] `sfx/reconnect-start` - Short re-forming pulse.
- [x] `sfx/reconnect-ready` - Variation of connection-ready.
- [x] `sfx/terminal-fallback` - Frame-fold or paper-to-glass transition.

### 2. General interface

- [x] `sfx/button-primary` - Firm, quiet activation.
- [x] `sfx/button-secondary` - Lighter activation.
- [x] `sfx/card-focus` - Very short tonal tick for keyboard/card focus; disabled
      by default if repeated navigation becomes noisy.
- [x] `sfx/card-select` - Clear selection cue.
- [x] `sfx/card-deselect` - Soft reversal cue.
- [x] `sfx/step-forward` - Page or layer advance.
- [x] `sfx/step-back` - Reversed page or layer movement, only where the server
      supports reselection.
- [x] `sfx/panel-open` - Quiet panel reveal.
- [x] `sfx/panel-close` - Quiet panel dismissal.
- [x] `sfx/modal-open` - Focused, non-startling reveal.
- [x] `sfx/modal-close` - Short release.
- [x] `sfx/toggle-on` - Small mechanical or magical latch.
- [x] `sfx/toggle-off` - Matching unlatch.

### 3. Validation, security, and state

- [x] `sfx/input-accepted` - Subtle server-confirmed acceptance.
- [x] `sfx/input-invalid` - Soft dry knock; no harsh buzzer.
- [x] `sfx/choice-locked` - Restrained sealed-lock cue.
- [x] `sfx/server-busy` - Low neutral wait cue.
- [x] `sfx/save-pending` - Brief forming-seal cue; not a success sound.
- [x] `sfx/save-confirmed` - Completed seal and warm resolve.
- [x] `sfx/save-failed` - Broken seal with a stable tail; does not imply that the
      draft was deleted.
- [x] `sfx/private-input-enter` - Optional soft veil cue when a password field
      becomes sensitive; never triggered per typed character.
- [x] `sfx/private-input-exit` - Matching veil release after the sensitive value
      is cleared.

### 4. Account lobby

- [x] `sfx/login-confirmed` - Account seal opens after server confirmation.
- [x] `sfx/new-account-confirmed` - New seal is pressed into the ledger.
- [x] `sfx/lobby-reveal` - Character archive opens.
- [x] `sfx/character-card-open` - Character folio opens.
- [x] `sfx/play-character` - Gateway begins to open.
- [x] `sfx/create-character` - Blank folio placed on the desk.
- [x] `sfx/account-capacity` - Full shelf/closed ledger warning.

### 5. Core character creation

- [x] `sfx/name-ink` - One quill or ink stroke on submission, not on every
      keystroke.
- [x] `sfx/name-sealed` - Name confirmation seal.
- [x] `sfx/identity-select` - Shared cue for either current sex choice.
- [x] `sfx/race-preview` - Soft origin-reveal swell.
- [x] `sfx/race-confirmed` - Origin emblem locks into the summary.
- [x] `sfx/class-preview` - Soft path-reveal swell, distinct from race preview.
- [x] `sfx/class-confirmed` - Class emblem locks into the summary.
- [x] `sfx/build-premade` - Guided path assembles.
- [x] `sfx/build-custom` - Branching tools unfold.
- [x] `sfx/alignment-move` - Quiet compass movement between valid positions.
- [x] `sfx/alignment-confirmed` - Alignment seal settles.
- [x] `sfx/preferences-applied` - Recommended bundle closes cleanly.
- [x] `sfx/roleplay-choice` - Journal opens, closes, or bookmarks according to
      the chosen path.
- [x] `sfx/onboarding-handoff` - Gate-opening accent synchronized with the
      server-confirmed transition.

### 6. Optional role-play suite

- [x] `sfx/editor-open` - Journal page opens.
- [x] `sfx/editor-saved` - Ink dries or page is gently sealed.
- [x] `sfx/example-generated` - Dice, cards, or shuffled notes; no spoken line.
- [x] `sfx/background-confirmed` - Background emblem stamped into the folio.
- [x] `sfx/age-confirmed` - Life-stage ring settles.
- [x] `sfx/region-preview` - Map opens or marker moves.
- [x] `sfx/region-confirmed` - Map pin is placed.
- [x] `sfx/faction-preview` - Banner unfurls without crowd noise.
- [x] `sfx/faction-confirmed` - Crest is affixed.
- [x] `sfx/hometown-confirmed` - Home marker and key settle.
- [x] `sfx/deity-preview` - Shrine-light reveal with no chant or whisper.
- [x] `sfx/deity-confirmed` - Holy symbol seal; no voice.

## Explicit No-Video Checklist

- [ ] No `.mp4`, `.webm`, `.mov`, or other video asset is produced.
- [ ] No animated GIF or animated WebP is produced as a video substitute.
- [ ] No pre-rendered race, class, background, or transition animation is
      required.
- [ ] All motion effects can be disabled without replacing a required control.
- [ ] Every scene has a complete static reduced-motion composition.
- [ ] Pausing motion does not pause or block server communication.

## Explicit No-Speech Checklist

- [ ] No voice-over explains login or character creation.
- [ ] No race or class has a spoken introduction.
- [ ] No deity has a prayer, chant, whisper, or spoken name.
- [ ] No account or validation state uses spoken instructions.
- [ ] No character bark plays on selection or confirmation.
- [ ] No text-to-speech asset is generated or bundled.
- [ ] Environmental loops contain no intelligible background conversation.
- [ ] Music contains no spoken or sung words.
- [ ] All meaning remains in visible text and accessible status announcements.

## Responsive Image Derivative Checklist

These derivatives are required for every applicable accepted master. They are
build outputs, not separate art-direction acquisitions.

- [ ] AVIF output for supported photographic or painted still art.
- [ ] WebP output for broad browser support.
- [ ] PNG fallback only where transparency or exact lossless edges require it.
- [ ] JPEG fallback only where the supported browser matrix requires it.
- [ ] Width variants at 480, 768, 1280, 1920, and 2560 pixels when the source
      composition supports those sizes.
- [ ] Dedicated mobile crop for every scene and selected catalog detail.
- [ ] Intrinsic width and height recorded in the media manifest.
- [ ] Blur or low-quality placeholder generated without exposing private data.
- [ ] Alpha edges inspected on both dark and light UI surfaces.
- [ ] No image is embedded in WebSocket or MSDP payloads.

## Proposed Performance Budgets

These are starting budgets enforced by the autonomous Phase 0 validation.

- [ ] Initial route image transfer is at most 1.0 MB on desktop and 650 KB on
      mobile before the player opens a gallery.
- [ ] One fully loaded scene pack is at most 1.2 MB on desktop and 750 KB on
      mobile.
- [ ] One race or class card thumbnail is at most 100 KB.
- [ ] One selected race or class detail pack is at most 450 KB.
- [ ] One SVG emblem is at most 25 KB compressed.
- [ ] Non-current race and class art is lazy-loaded.
- [ ] Only the current and likely next screen may be preloaded.
- [ ] Music is not preloaded before audio opt-in.
- [ ] One compressed music cue is at most 2.5 MB.
- [ ] One compressed ambience loop is at most 900 KB.
- [ ] One compressed one-shot sound effect is at most 100 KB.
- [ ] Expensive layer animation pauses in background tabs.
- [ ] Data-saver mode uses static composites and card thumbnails until the user
      requests detail.

## Accessibility and Presentation Checklist

- [ ] Every meaningful image has authored alt text verified by contextual
      snapshot, adjacency, duplication, and accessibility checks.
- [ ] Every decorative image uses empty alt text.
- [ ] Alt text does not repeat the full adjacent race, class, or deity
      description.
- [ ] A visual card always remains a normal keyboard-operable button or list
      option.
- [ ] Selected, disabled, and locked states do not depend on color or sound.
- [ ] Locked art is not blurred so heavily that its subject becomes
      inaccessible; text explains the restriction.
- [ ] `prefers-reduced-motion` uses static composites and removes parallax,
      particle drift, long pans, and spinning rings.
- [ ] Focus treatment remains visible over every image and texture.
- [ ] Text safe zones pass at desktop, 390 px, and 360 px widths.
- [ ] Audio is never required to understand validation or progress.
- [ ] Mute and volume controls are keyboard and screen-reader accessible.
- [ ] Flashing, rapid luminance changes, and high-frequency particle effects are
      not used.

## Autonomous Canon, Representation, and Licensing Checklist

- [ ] Generate one versioned art-direction guide from authoritative repository
      evidence before producing catalog packs.
- [ ] Derive and record the world palette, materials, architecture, magic
      language, and rune language from repository evidence.
- [ ] Permit generated imagery under this manifest and apply the automated
      acceptance suite to every generated asset.
- [ ] Build machine-readable canon evidence records for races, original
      ancestries, regions, factions, classes, and deities.
- [ ] Apply the evidence-or-fallback policy to Crystal Dwarf, Trelux, Arcana
      Golem, Dragonborn, Tabaxi, Shade, and Fae.
- [ ] Extract default-campaign region facts from code, world data, help, and
      lore files; use `region/fallback` wherever the facts remain absent.
- [ ] Verify every playable race is presented as a player culture/person, not
      only as an enemy archetype, using prompt lint and visual classification.
- [ ] Measure variety of sex, age, body type, skin tone, and role across the
      full asset set with batch-level representation checks.
- [ ] Confirm that race/class art does not prescribe the player's exact
      appearance.
- [ ] Confirm that evil alignment or dark-class art remains readable and avoids
      automatic real-world cultural coding.
- [ ] Complete automated content, similarity, and licensing checks for race,
      class, background, and deity names and public descriptions.
- [ ] Record creator, source files, license, modifications, and acceptance for
      every delivered asset.
- [ ] Store attribution text where a license requires it.
- [ ] Confirm that no copied tabletop-book, game, film, or fan illustration is
      used as a shipping asset.
- [ ] Define the takedown and replacement workflow for disputed media.

## Integration and Final Coverage Checklist

- [ ] The media manifest has a version and schema validation.
- [ ] Every current race media key resolves.
- [ ] Every current selectable base-class media key resolves.
- [ ] Every background, alignment, age, region, hometown, and deity key
      resolves.
- [ ] The current runtime faction catalog resolves or uses the faction fallback.
- [ ] Unknown future race, class, region, faction, hometown, and deity keys use
      the correct generic fallback.
- [ ] Account cards work for multiclass and prestige-class characters without
      dedicated prestige art.
- [ ] Locked and unavailable choices reuse normal art with accessible UI state.
- [ ] Disconnect and account switch clear private character-card state.
- [ ] Service-worker caching versions static media but never caches passwords,
      terminal transcripts, account names, or role-play text.
- [ ] The classic terminal path works with all media unavailable.
- [ ] The complete creator works with audio muted.
- [ ] The complete creator works in reduced-motion mode.
- [ ] The complete creator works on a data-saver connection.
- [ ] Broken-image, missing-audio, and partial-cache tests pass.
- [ ] Desktop, tablet, 390 px, and 360 px visual QA passes.
- [ ] Keyboard, screen-reader, contrast, focus, and audio-control QA passes.
- [ ] Final transferred-byte budgets are measured from a production build.
- [ ] Final source, license, attribution, and acceptance records are complete.

## Autonomous Completion Gate

The pipeline declares this manifest complete automatically only when:

- [ ] The MUD protocol emits the final stable media keys.
- [ ] The live race, base-class, region, faction, hometown, and deity catalogs
      have been exported and compared with this checklist.
- [ ] Every canon evidence gap has either a repository-grounded treatment or an
      explicit fallback mapping.
- [ ] Every missing catalog asset has an intentional fallback.
- [ ] No-video and no-speech audits pass.
- [ ] All responsive derivatives, alt text, provenance, and licenses are
      attached to the versioned web media manifest.
