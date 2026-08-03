# Production In-Game Idea Backlog

This is a read-only snapshot of the production `idea submit <header>` queue,
retrieved on 2026-08-03. The production file contained 73 submissions, all
flagged unresolved. The entries below are backlog candidates, not approved
plans or claims about current behavior.

Repeated body text and command echoes were removed. One headerless submission
containing two ideas was split, one submission containing two new spells was
split, and overlapping healer follow-ups were consolidated. No distinct idea
from the current production queue was discarded. Reporter metadata uses `L`
for character level and `#` for the record's position in this snapshot; queue
positions can change when the production list is edited.

## Interface, Accessibility, and Help

- **Categorize bug, typo, and idea records** - Add a staff-facing field for
  areas such as code, feat, mob, or content so each queue can be filtered by
  the role handling it. Reporter: Jordan (L31, 2022-02-26; #007).
- **Show active affects in Score** - Consider combining the affects display
  into the score screen. Reporter: Zorast (L7, 2022-06-26; #008).
- **Add half-orc camp coordinates to Beginning Your Journey** - Put the camp's
  coordinates in the quest description for blind players who cannot use the
  map. Reporter: Ozrim (L3, 2022-07-13; #011).
- **Standardize and correct zone help** - Make `help zones` match `zones2` and
  `zones3`, and review entries whose teleporters appear to lead to mislabeled
  or lower-level destinations. Reporter: Harlon (L3, 2022-08-12; #013).
- **Honor screen-reader negotiation from TinTin++** - Detect its screen-reader
  mode, disable ANSI or apply equivalent accessibility settings, and consider
  an account-level preference. Reporter: Harlon (L3, 2022-08-12; #014).
- **Show PSP in the GUI** - Add psionic power points to the graphical status
  display. Reporter: Zusuk (L34, 2022-08-26; #018).
- **Add a usable keyring container** - Let players store keys in a keyring and
  unlock doors with keys inside it. Reporter: Eldek (L12, 2022-08-26; #019).
  An earlier import also attributed a wearable belt-slot version to Rinne.
- **Auto-loot contents when sacrificing corpses** - Add a preference that
  retrieves corpse contents before sacrifice. Reporter: Metvagen (L20,
  2022-08-26; #021).
- **Allow help inside study menus** - Let players read help for skills, feats,
  and other choices without leaving and reopening the study interface.
  Reporter: Metvagen (L21, 2022-08-27; #022).
- **Support bulk gain after respec** - Accept a level count such as `gain 19`
  instead of requiring the command once per level. Reporter: Metvagen (L3,
  2022-08-27; #023).
- **Allow help during character creation** - Make help files available from
  chargen screens. Reporter: Zusuk (L34, 2022-08-28; #024).
- **Replace generic OK responses with richer text** - Use more descriptive or
  flavorful messages where the global OK response is currently emitted.
  Reporter: Zusuk (L34, 2022-08-28; #025).
- **Make door-unlock feedback explicit** - Replace the bare `*click*` with a
  message such as "You unlock the door." Reporter: Delax (L1, 2022-08-29;
  #026).
- **Color room names more distinctly** - Make room titles stand out from the
  surrounding description. Reporter: Delax (L1, 2022-08-29; #027).
- **Pace tutorial NPC messages** - Delay rapid NPC output so new and slower
  readers are not given several instructional messages within two seconds.
  Reporter: Delax (L1, 2022-08-29; #028).
- **List locked feats and their requirements** - Add a view of feats that can
  be unlocked and the prerequisites for each. Reporter: Dasvel (L28,
  2022-09-04; #033).
- **Allow feat changes without a full respec** - Provide a way to add or remove
  feats independently when testing a build. Reporter: Yaran (L9, 2022-09-04;
  #034).
- **Include group chat in `history all`** - Preserve group-channel messages in
  the combined history view. Reporter: Brondo (L30, 2022-09-19; #035).
- **Notify when Defensive Stance expires** - Emit a clear message when the
  Stalwart Defender stance runs out. Reporter: Brondo (L30, 2022-09-27; #036).
- **Allow news and MOTD while crafting** - Let players read those informational
  screens during a crafting action. Reporter: Kormundrad (L3, 2022-09-27;
  #037).
- **Add a combat-output visibility toggle** - Let players see or hide damage
  dealt by other players and pets. Reporter: Brondo (L30, 2022-11-07; #044).
- **Show class and circle in spell help** - Identify which classes receive a
  spell and at which circle. Reporter: Murdoch (L30, 2022-11-14; #049).
- **Hide player-house contents from Locate Object** - Prevent private stored
  items from cluttering results or exposing house contents. Reporter: Diel
  (L30, 2022-12-15; #053).
- **Teach the save command** - Add saving instructions to the tutorial or hint
  system for players unfamiliar with Diku-derived games. Reporter: Fish (L1,
  2023-01-23; #057).
- **Document feat-point types** - Explain which kinds of feat points purchase
  which categories of feats. Reporter: Neurrone (L21, 2023-03-25; #059).
- **Clarify Polymorph Self limitations** - State that polymorphed characters do
  not gain racial feats and may lose spellcasting access. Reporter: Neurrone
  (L22, 2023-03-26; #060).
- **Use words or letters instead of punctuation-only indicators** - Provide
  screen-reader-safe alternatives for symbols such as `!`, whose pronunciation
  depends on punctuation settings. Reporter: Dranulous (L2, 2024-03-19; #067).
- **Add an equipment compare command** - Compare newly found weapons or armor
  against equipped items. Reporter: Darowin (L3, 2024-08-01; #068).
- **Warn players before server reboots** - Broadcast advance notice when a
  reboot is scheduled or imminent. Reporter: Syman (L18, 2025-11-27; #070).

## Crafting, Items, and Economy

- **Stock more crafting molds in Ashenport** - Add armor and cloth molds to the
  early crafting shop instead of offering only weapon molds. Reporter: Mendev
  (L20, 2022-01-12; #001).
- **Show crafting skill progress** - Display current crafting experience and
  the amount needed for the next skill rank. Reporter: Ostvel (L27,
  2022-02-20; #006).
- **Delay the harvesting-depleted message until the final yield** - Report
  depletion after the last successful harvest rather than while the process is
  still continuing. Reporter: Variel (L2, 2022-07-03; #009).
- **Expand restring customization** - Allow optional description and material
  changes in addition to the item name. Reporter: Metvagen (L19, 2022-08-26;
  #020).
- **Add a fair group-loot mechanism** - Support dice rolls, bids, or a similar
  in-game method for dividing valuable gear. Reporter: Lamix (L30,
  2022-09-01; #032).
- **Add basic poison creation for Rogues** - Let Rogues make or buy a weak,
  long-duration poison cheaply enough to use on ordinary enemies. Reporter:
  Serul (L26, 2022-10-17; #039).
- **Add food and drink enhancements to item storage** - Extend the `store`
  feature beyond potions and scrolls to enhanced consumable food and drink.
  Reporter: Brondo (L30, 2022-11-07; #045).
- **Convert weapon types without losing enchantments** - Add a crafting or
  quest process that can reshape, for example, a flaming longsword into a
  flaming shortsword. Reporter: Murdoch (L13, 2022-11-11; #046).
- **Remove empty poison vials automatically** - Destroy a vial after its final
  poison application. Reporter: Badase (L30, 2022-11-28; #051).
- **Make Ant Hill corpse contents removable** - The dominant green ant's broken
  red-ant leg cannot be taken, which prevents sacrificing the corpse. This was
  submitted to the idea queue as a content adjustment. Reporter: Grignak (L4,
  2022-12-06; #052).
- **Reward crafters when their equipment is used** - Accumulate crafting
  experience or another benefit as other players fight with a crafter's items.
  Reporter: Gicker (L34, 2023-03-31; #061), crediting Tollymore for the idea.
- **Create Bazaar crafting commissions** - Turn player Bazaar orders into
  crafting missions posted on a board, paying the crafter a portion of the
  quest-point price. Reporter: Tollymore (L8, 2023-04-04; #062).
- **Build an adventure loop around crafting recipes** - Recipes would specify
  skills, feats, ordinary components, and a key component found at a spawned,
  guarded crafting node. Reporter: Tollymore (L8, 2023-04-04; #063).

## Combat, Classes, Feats, and Companions

- **Make Acrobatics a Berserker class skill** - Align the class with Pathfinder
  and give it another defensive option. Reporter: Mendev (L23, 2022-01-15;
  #002).
- **Allow players to name companions** - Add an optional naming command for
  followers, summons, and animal companions. Reporter: Valafar (L9,
  2022-01-16; #003).
- **Add Greater Feint** - Provide a feat that allows Feint as a move action.
  Reporter: Yure (L3, 2022-01-17; #004).
- **Add a Magus class** - Introduce Magus as an alternative to Spellsword or
  Eldritch Knight. Reporter: Sedron (L29, 2022-07-13; #010).
- **Let Mass Enhance supersede lesser ability buffs** - Strength, Grace, and
  Endurance from a pet should not block Mass Enhance; the stronger spell could
  overwrite or stack appropriately. Reporter: Serulina (L22, 2022-08-14;
  #015).
- **Give animated zombies more varied attacks** - Add a bite and occasional
  blunt attacks to Animate Dead zombies. Reporter: Metvagen (L12,
  2022-08-23; #016).
- **Create a healer specialization or class unlock** - Add healing bonuses,
  faster high-circle memorization, and an Improved Heals progression.
  Reporter: Melaw (L30, 2022-08-30; #030, #031).
- **Rebalance epic races, especially Lich** - Document undead benefits, add
  meaningful Lich drawbacks, review weak Crystal Dwarf abilities, and consider
  the Trelux equipment-slot disadvantage. Reporter: Brondo (L30, 2022-09-28;
  #038).
- **Share teamwork feats with Summon Shadow** - Grant the shadow any teamwork
  feats known by its caster. Reporter: Raiko (L15, 2022-10-20; #041).
- **Let non-Druids inspect wildshape forms** - Expose form information to
  characters evaluating Polymorph Self. Reporter: Ilzude (L23, 2022-10-22;
  #042).
- **Default Careful With Pets to enabled** - Protect friendly pets by default
  rather than requiring each player to opt in. Reporter: Malicor (L23,
  2022-11-13; #047).
- **Allow Circle or Backstab against disabled opponents while tanking** - Blind
  or paralyzed enemies should remain vulnerable, subject to Blind-Fight where
  appropriate. Reporter: Murdoch (L30, 2022-11-14; #048).
- **Give caster mobs class-specific spell lists** - Keep Cleric and Wizard
  casting distinct, and optionally let charmies buff their master from their
  available abilities. Reporter: Dudris (L31, 2022-11-23; #050).
- **Let Circle initiate combat behind another tank** - Permit the maneuver as
  an opener when somebody else already holds the target. Reporter: Badase
  (L30, 2022-12-29; #055).
- **Auto-stand for `buffself perform`** - Stand a sitting character before
  starting the performance. Reporter: Lyllian (L7, 2023-01-04; #056).
- **Increase Shifter Dragon damage** - Scale breath weapons and form-granted
  spells to be meaningful against level 30 opponents. Reporter: Ogoun (L30,
  2023-04-14; #064).
- **Track alignment on two axes** - Separate law/chaos from good/evil so changes
  move through neutral independently and reflect the alignment of defeated
  mobs. Reporter: Ogoun (L30, 2023-04-14; #065).
- **Add attacks during grappling** - Give Monks or other combatants offensive
  options while a grapple is active. Reporter: Tsol (L3, 2025-11-28; #072).

## Spells and Psionics

- **Do not spend PSP when a power cannot be applied** - Recasting a power such
  as Inertial Armor while its existing instance must first be revoked should
  not consume points. Reporter: Fyre (L3, 2022-07-28; #012).
- **Add Dispel Invisibility** - Provide a spell that removes invisibility from
  equipment. Reporter: Melaw (L30, 2022-08-30; #029).
- **Add area wall or cube spells** - Introduce expensive tactical denial spells
  such as a prismatic cube or force cube. Reporter: Melaw (L30, 2022-08-30;
  #029).
- **Add a multi-round Healing Wave** - Heal roughly 100-200 HP per round for
  three to five rounds, with a lower-level healing progression as appropriate.
  Reporters: Melaw (L30, 2022-08-30; #031) and Melow (L30, 2022-10-24; #043).
- **Add Dimension Door** - Implement the classic short-range teleport spell.
  Reporter: Gargar (L22, 2022-10-19; #040).

## Quests, World, Roleplay, and Social Systems

- **Add a smoking system** - Provide a lightweight, immersive social action
  similar to smoking NPC behavior. Reporter: Arvaunshae (L17, 2022-02-11;
  #005).
- **Add temple association** - Use the deity slot for temple affiliation and
  connect temples to optional quest chains. Reporter: Arvaunshae (L17,
  2022-02-11; #005).
- **Add Ashenport tourist escort quests** - Let lost visitors offer small,
  optional city escorts for modest coin. Reporter: Metvagen (L12, 2022-08-24;
  #017).
- **Limit mission destinations to mapped locations** - Avoid Adventurer's Guild
  missions that direct players to places absent from the website map. Reporter:
  Tanaka (L13, 2022-12-15; #054).
- **Let quest mobs surrender inventory items** - Add a command or script action
  that avoids killing the mob or losing the item in a safe room. Reporter:
  Arithon (L30, 2023-02-19; #058).
- **Acknowledge retained velocity after planar recall** - If recalling while
  falling intentionally preserves momentum, add a humorous message that makes
  the behavior explicit. Reporter: Iliri (L26, 2023-09-28; #066).
- **Have Mosswood's elder explain the route to Ashenport** - Make `ask elder
  ashenport` return directions. Reporter: Mddljeu (L2, 2025-07-05; #069).
- **Add predecessor clues to Beginning quests** - For Find Alerion and Go to
  the Wizard's Tower, identify the earlier quest or clue needed to unlock the
  chain. Reporter: Syman (L18, 2025-11-27; #071).
- **Let the Town Crier spread player-provided gossip** - Allow a whispered
  message to enter the crier's gossip rotation. Reporter: Falwel (L8,
  2025-12-02; #073).

## Earlier Imported Ideas Not in the Current Production Queue

These seven distinct ideas were already present in this document but are not
records in the 2026-08-03 production queue. They are retained to avoid losing
previously imported player feedback.

- **In-character telepathy item** - Add a restringable, craftable item in a
  unique slot for in-character communication. Reporter: Rinne (L13).
- **Charmee affects display** - Add a command to inspect spells and buffs active
  on charmees. Reporter: Plixid (L8).
- **Wilderness coordinates in the prompt** - Show location coordinates in the
  prompt while in the wilderness. Reporter: Age (L10).
- **Stop or cancel crafting** - Add a command that interrupts an active crafting
  process. Reporter: Dagmar (L1).
- **Display Wizard bonus feats** - Show the class bonus feats received every
  five levels in the class-feat display. Reporter: Ortallus (L4).
- **Rest recovery from food and water** - Grant an HP recovery bonus while
  resting when hunger and thirst needs are met. Reporter: Mendev (L17).
- **Document Intimidate duration** - Add duration information to the Intimidate
  help entry. Reporter: Mendev (L10).
