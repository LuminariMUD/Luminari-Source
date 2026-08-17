-- Builder help for registry-backed special procedures.
--
-- The help system is database-first. This idempotent migration is the
-- reviewable source for the builder-facing SpecProc topic.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('spec-proc', 'SPECIAL PROCEDURES (SPECPROCS)

Special procedures provide coded behavior for rooms, mobiles, and objects.
In medit, oedit, or redit, choose Z) SpecProc to view procedures that are safe
for that prototype type. The menu explains each procedure\'s events and any
required flags or placement. Enter a menu number to select it or 0 to clear it,
then save normally. The selected name is stored in the world file and restored
at boot.

The menu lists canonical names only. Explicit aliases remain load-compatible,
but selecting an entry writes its canonical name. Selecting a procedure does
not add runtime prerequisites such as MOB_SPEC, ITEM_AUTOPROC, equipped,
carried, or combat state; review the entry description and configure those
requirements separately.

World loading preserves the exact authored request even when a name is unknown
or incompatible and installs no callback. Merely opening and saving OLC keeps
that request. Select a menu entry to replace it with a canonical name, or enter
0 to clear the authored procedure and omit it on the next zone save.

At boot, SPEC_BIND lines show ordered world, parser-hook, legacy-assignment,
shop, and quest contributions. SPEC_BIND_FINAL shows the authored request,
chosen callback, source, and collision count. In -s mode only sources that
actually run are reported; -s is not a global callback-disable switch.

Immortal staff can inspect the same recorded post-boot chain on a live server
with SPECBIND <mob|obj|room> <vnum>. The command reports the effective callback,
every ordered contribution and outcome, source locations, collision count,
saved shop or quest secondary, and the chosen source. SPECBIND is read-only and
does not change the prototype or rebuild history after a later OLC edit.
This history is diagnostic, not a persisted multiple-procedure dispatch chain.
Each prototype still stores zero or one authored procedure name; shop and quest
secondaries are reconstructed compatibility state.

A moving room cannot also have a named room SpecProc. Both features own the
same callback slot, so redit refuses that selection and zone saving or boot
rejects a room containing both forms of data.

Converted RoL-Demon, RoL-Devil, and RoL-Umberhulk mobile flags are not
additional authored SpecProcs. Independent compatibility hooks run their
source race behavior beside the one persistent mobile SpecProc slot. These
flags are converter-owned and should not be added to unrelated new mobiles.

Converted RoL death flags likewise run independently of the named SpecProc
slot. Familiar, mount, summoned-monster, shaman-spirit, and Bloodstone black-vapor
messages retain their target no-corpse policy without consuming that slot. The
breath_attack and breath_weapon entries are named mobile
combat SpecProcs; attack variants affect the current opponent, while weapon
variants affect eligible targets across the room every fourth combat turn.

Forty-five converted mobiles use converter-owned death profiles. In addition to
the tentacle, mephit, elemental, treant, phantom-steed, and dark-shade no-corpse
profiles, they preserve source death bursts, darkness, poison gas, returned
possessions, stone-pile inventory, replacement forms, dropped objects, loose
possessions, splitting skeletons, and ordinary-corpse messages. Three replacement
families also preserve source cleric-retargeting activity. The profiles run beside
the one named SpecProc slot without requiring a builder-assigned flag.

RoL Bloodstone Critter is mobile-owned and requires MOB_SPEC. While awake and
idle, the four converted critters use the current snarl and growl socials with
the source two-in-81 activity-pulse cadence. This procedure is converter-owned.

RoL Designated Follower is mobile-owned and requires MOB_SPEC. Five converted
Icecrag guards find their fixed NPC leaders when awake and colocated, then use
the target follower system for movement and combat assistance. This procedure
and its follower-to-leader mapping are converter-owned.

RoL Fixed Bodyguard is mobile-owned and requires MOB_SPEC. Converted Icecrag
bodyguards 2097040-2097042 rescue assigned mobiles 2097023, 2097029, and 2097008
respectively when awake, colocated, and the assigned mobile has an attacker.
The fixed mapping and procedure are converter-owned.

RoL Floating Pool is object-owned and requires ITEM_AUTOPROC. Four converted
Ethereal pools left in rooms have a 12 percent chance per object pulse to move
through one random open cardinal exit. Closed, hidden, blocked, invalid, and
ROOM_NOMOB destinations are excluded. This procedure is converter-owned.

RoL Bloodstone Portal is object-owned. Four converted portals remap object
value 0 to the target destination room. Awake characters can enter the exact
portal object when the destination passes target teleport admission. Mortals
lose 1-20 hit points and 1-30 movement points after arrival and die only when
the hit-point loss would leave them below -10. Staff are immune to this stress.

RoL Portal Door is object-owned. Four converted miscellaneous portals remap
object value 0 to the destination room and use value 3 to reject source-good or
source-evil races. LOOK IN previews the loaded destination. ENTER preserves the
source level-20 and arena gates, applies target teleport-admission safety, and
moves only a character selecting that exact portal object.

RoL Travel Portal is object-owned and identity-keyed to nine converted objects.
It preserves a dimensional fold with LOOK IN preview, two Waterdeep portals,
a Wizard-only illusionist fountain, two level-20 Elf gates with four randomized
destination slots, two carried Cleric spores with a non-Cleric stun path, and a
Blip portal that gives converted badge 2041900. Do not assign it to unrelated
objects because their identities have no travel profile.

RoL Item Blocker is object-owned and reads its blocked cardinal direction from
object value 0. While an aggressive NPC occupies the room, it blocks mortal
players and player pets from moving or unlocking a door in that direction.
The procedure and its six converted ATD objects are converter-owned.

RoL Magic Pool is object-owned. Converted pools keep their fixed entry damage
and remapped destination in object values; builders should not assign it to an
ordinary object without configuring both values deliberately.

RoL Banana is object-owned and converter-owned. Converted object 2001235 is
eaten with EAT BANANA, restores its food value, and leaves temporary peel
2001234 in the room. Awake mortals moving over the peel may avoid it through
Intelligence, fall or pass out based on Dexterity, or recover and continue.
Flying, levitating, mounted, and staff characters ignore the peel. The peel
decays after ten real minutes. Do not assign this identity-keyed procedure to
unrelated objects.

RoL Auto Distributor is room-owned. Any command by a non-staff character is
intercepted and moves that character to a random loaded room in the same zone.
It is intended only for converted RoL boundary rooms.

RoL Command Sentinel is mobile- or room-owned. Four converter-owned mobile
profiles enforce their exact room, direction, race, level, and chance gates;
mobile owners require MOB_SPEC. Two room profiles preserve the cage command
ward and the Necromancer passage glyph. The glyph deliberately preserves the
source behavior of dealing 25 damage through Minor Globe or Globe of
Invulnerability, versus one damage otherwise, and never lowers hit points below
one. Do not assign this identity-keyed procedure to unrelated prototypes.

RoL Toll Keeper is mobile-owned and requires MOB_SPEC. Nine converter-owned
profiles preserve three fixed-fee passages and two bridge throws, plus four
ticketed ship entries. Source platinum and copper fees map to target gold: use GIVE
<amount> GOLD <keeper>. Ticket takers accept the configured ticket from the
player or keeper, destroy one, and then allow ENTER to continue. Do not assign
this identity-keyed procedure to unrelated mobiles.

RoL Shadow Giant is mobile-owned and requires MOB_SPEC. While fighting, each
mobile-activity pulse has the source one-in-21 chance to spook every player and
charmed pet in the room for mental damage and a possible short stun. It is
intended for converted RoL shadow giants.

RoL Guild Guard is mobile-owned and requires MOB_SPEC. It enforces converted
room-specific class and race gates only while the guard remains in its original
load room. Protected guards punish and relocate mortal attackers. Use the
ordinary Guild Guard procedure for new Luminari guild entrances. Six converted
Bloodstone gates map Antipaladin to Blackguard, Shaman to Cleric, and Lich to
Necromancer. Seven Waterdeep guild guards compose their class gates and guardian
retaliation with generated, source-hashed idle and fighting flavor tables.

RoL Waterdeep Guild Room is room-owned and converter-owned. Twelve converted
Waterdeep guild rooms retain their room-specific exact or class-family gate and
delegate accepted PRACTICE, TRAIN, and BOOSTS commands to the current target
guild service. Source Mercenary maps to target Warrior, and any matching class
in a multiclass build is sufficient.

RoL Major Beholder is mobile-owned and requires MOB_SPEC. Each of its ten eye
rays has an independent three-combat-turn cooldown and a one-in-three chance to
fire while ready. Target-native effects cover fire, acid, slow,
enfeeblement/feeblemind, wither, room-wide dispel, prismatic spray, hold
monster, harm, and finger of death. Pet targets redirect to an eligible master
in the room. The source-only all-eyes weapon-critical burst has no target
combat-turn event equivalent. This procedure is converter-owned.

RoL Monster Combat is mobile-owned and requires MOB_SPEC. Forty-five converted
mobiles across thirty-five source families use identity-keyed combat, activity,
and command profiles. They cover poison, lycans, shockwaves, celestial and
prismatic effects, four Elemental Tower bosses, kobolds, piercers, purple worms,
phalanxes, splitting skeletons, transformations, tree spirits, Dranum, swallow
attacks, Canthus, Jotuns, and pit fiends. The four Elemental Tower bosses compose
their alerts, and pit-fiend tails compose through this single persisted procedure.
The critical prismatic identity maps its unavailable NPC-critical callback to a
documented one-in-20 combat-turn burst. This procedure is converter-owned and
must not be assigned to unrelated mobiles.

RoL Lich Energy Drain is mobile-owned and requires MOB_SPEC. On activity
pulses and combat turns, each eligible current opponent or party member has a
one-in-five chance to lose all current hit points plus five. Death Ward maps
the source protection-from-undead case and leaves the victim at zero instead.
The lich receives the victim\'s former current hit points unless Blackmantled,
and each drain adds two combat rounds of stun. Casting suppresses the drain.
This procedure is converter-owned.

RoL Undead Drain is mobile-owned and requires MOB_SPEC. Converted ghoul,
shadow, wight, ghast, wraith, spectre, and ghost mobiles 2001256 through
2001262 use one identity-profiled combat adapter. Their one-in-16 or one-in-21
combat-turn attacks apply the source-specific armor, Dexterity, Strength, save,
and slow penalties for two to three affect ticks after a failed Will save.
Undead victims and characters protected by Death Ward are immune. Melee-profile
and spell-profile drains each have a shared cooldown marker, preserving the two
source exclusion groups. This procedure is converter-owned and should not be
assigned to unrelated mobiles.

RoL Trade Bandit is mobile-owned and requires MOB_SPEC. It intercepts movement,
FLEE, and GET when a merchant carries converted resources or owns a loaded wagon.
Seven converter-owned VNUMs select fixed, cargo-relative, all-gold-and-wagon,
alignment-sensitive, or immediate-attack demands. Source platinum maps to ten
target gold. Pay with GIVE <amount> GOLD <bandit>. Underpayment starts combat;
sufficient payment lets the bandit disappear. Do not assign this procedure to
unrelated mobiles.

RoL Alert Caller is mobile-owned and requires MOB_SPEC. Eleven converted callers
shout their source warning once per fight and send only configured awake, idle,
same-zone helpers within 100 reachable rooms to pursue the attacker. Soundproof
rooms, silence, paralysis, casting, and sleep suppress the call. The Imix and
Yancbin callers compose this alert beside their existing breath procedure.
Elemental Tower callers 2062401, 2062402, 2062405, and 2062406 use the same
adapter with their source-specific helper groups.

RoL Yggdrasil Branch is mobile-owned and requires MOB_SPEC. Converted mobiles
2062800-2062804 make a 50 percent entangle attempt against the current opponent
or a vulnerable group target. A failed Reflex save at the source -10 modifier
entangles for four to twelve combat rounds; release halves current movement.

RoL Waterdeep Ambient is mobile-owned and requires MOB_SPEC. Fifty-six
converted citizens across 44 source families emit their authored speech and
room actions on the original two-die periodic distributions while standing.
Multi-line outcomes retain their order, including the casino-player fall-through.
Merchant 2005310 emits its harbor dialog only in room 2005400. Do not assign this
converter-owned, identity-keyed procedure to unrelated mobiles. Converted guards
2003035, 2003059, and 2003070 remain quiet while fighting.

RoL Waterdeep Peacekeeper is mobile-owned and requires MOB_SPEC. Converted
tavern bouncers 2005523 and 2005541-2005543 return to their assigned posts and
drag the lowest-alignment aggressor through their source-authored tavern route
to room 2003258. Casino bouncer 2003207 returns to its load room and throws the
same kind of aggressor into room 2003254. Both variants stop the offender and
everyone attacking the offender before moving them. Off-duty guard 2003229
retains its drunken ambient table and joins eligible fights against the
aggressor. This procedure is converter-owned and should not be assigned to
unrelated mobiles.

RoL Weapon Proc is object-owned and must be equipped. Fifty converted weapon
objects use identity-keyed profiles for their source critical, sneak-attack,
random-hit, periodic, wielder-restriction, charge, extra-swing, damage, spell,
and summon behavior. The hit gateway supplies exact damage, attack type, and critical
state. Object 2095878 accepts SAY LABELAS for weekly group barkskin;
the Halruaan staves accept their documented summon and corpse-preservation SAY
phrases, and Kor\'s lethal damage leaves converted severed-head object 2001058.
This procedure is converter-owned and should not be assigned to unrelated
objects.

RoL Source Periodic is mobile-owned and requires MOB_SPEC. One hundred converted
Bloodstone, Icecrag, Menden, Fun, Mobile, Realm, Lavatubes, Tower of Sorcery, and
Waterdeep mobiles across 94 source families use 367 source random outcomes
containing 601 ordered speech or room-visible actions. The generated profiles
preserve each source random range or dice expression, fall-through order, room
text, visibility setting, awake or sleeping gate, and combat gate. Fun mobile
2001230, jester 2003069, and cricket 2014048 do not require an awake mobile.
Waterdeep guard 2003212 runs only while sleeping. Fun mobile 2001230, jester
2003069, and Menden magus 2088806 continue during combat. Do not assign this
converter-owned, identity-keyed procedure to unrelated mobiles.

RoL Stateful Periodic is mobile-owned and requires MOB_SPEC. Thirty-four converted
Waterdeep mobiles have generated idle and fighting profiles containing 266 source
outcomes and 274 ordered speech or room-visible actions. Idle tables require an
awake, standing mobile. Fighting tables run whenever a mobile has a current opponent,
making the source-authored combat tables usable even where the source tested standing
position before fighting state. Casino owner 2003206 independently rolls both its
fighting and standing tables during combat, as in the source. Guildmaster 2003020 has no authored
fighting table and remains quiet in combat. Twenty-seven profiles use
this procedure directly; seven Waterdeep guild guards compose the same generated
profiles through RoL Guild Guard. Do not assign either converter-owned, identity-keyed procedure
to unrelated mobiles.

RoL Sister Knight is mobile-owned and requires MOB_SPEC. When one of the five
converted Sister Knight prototypes enters combat, it shouts once across the zone
and sends every awake, idle, reachable converted sister within 100 rooms to pursue
the attacker. Soundproof rooms, silence, paralysis, casting, and repeat alerts in
the same fight suppress the call. This procedure is converter-owned.

RoL Shaman Totem is object-owned and must be held or wielded. It permanently
bonds one of 21 converted totem identities to a Cleric and its original object.
Summoning unlocks at Cleric level 21, uses a Cleric-level and Wisdom success
curve, permits three attempts per seven MUD days, and allows one active spirit.
Converted source-race gating and corpse-free spirit deaths are preserved. The
procedure and RoL-Totem-Spirit flag are converter-owned.

The five RoL Ship procedures preserve seven converted fixed-interior ships. RoL
Ship boards the hull with ENTER, RoL Ship Control handles panel instruments and
ship orders, RoL Ship Exit and RoL Ship Lookout expose the exterior, and RoL Ship
Navigator protects orders and supports scheduled routes. The navigator requires
MOB_SPEC for its combat-turn crew response. These procedures use converter-owned
hull, interior, route, and navigator associations and do not configure arbitrary
new ships.

Guild is the mobile-owned training procedure. RoL Guild Room provides the
same current training service for unrestricted converted room-owned guild
bindings. RoL Mage Guild Room, RoL Thief Guild Room, RoL Warrior Guild Room,
RoL Cleric Guild Room, and RoL Bard Guild Room preserve source class-family
admission while using the target multiclass model; any matching class level is
sufficient. All seven procedures are available only in redit, including RoL
Waterdeep Guild Room.
Pet Shop is room-owned,
Postmaster is mobile-owned, and Bank is available for compatible mobile and
object prototypes.

Use trigedit when a script is sufficient. Ask a coder when the needed behavior
is not present in the SpecProc menu. Shops, quests, pet shops, and boards have
additional setup requirements beyond choosing a callback.

See also: SPECBIND, OLC, MEDIT, OEDIT, REDIT, TRIGEDIT, PETSHOP, BOARDS', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

-- Retire the two stale file-imported entries from keyword search. The help
-- query displays only its first database match, so duplicate mappings would
-- make the maintained result nondeterministic.
DELETE FROM help_keywords
WHERE UPPER(keyword) IN (
  '<SPEC>', 'SPEC', 'SPEC-PROC', 'SPECIAL-PROCEDURE', 'SPECIALS', 'SPECBIND', 'SPECPROC'
);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPEC');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPEC-PROC');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPECIAL-PROCEDURE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPECIALS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPECBIND');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPECPROC');

COMMIT;
