# Combat Restoration Live Evidence, 2026-09-16

Live transcripts from the `revert-combat` branch at `77db64f44` (mechanics
identical to `ae4ebc752`), run on port 4100 in a private network namespace
against the development database. The player is an ordinary level 1 human
sorcerer created through the normal creation flow, with `combatroll` on. A
staff character loads a fremlin (mob 145291) into the room and leaves.
Timestamps are seconds since the client connected. The account name is
redacted as `<acct>`.

## Workflow: melee, cast after ordinary attacks, queued kicks

The sorcerer won initiative (roll 12 vs 3), so its phases fall two seconds
after joining and the fremlin's four seconds after: the two sides alternate
every two seconds, as before the refactor. The cast is admitted after six
seconds of ordinary attacks (it fizzled because room 145383 blocks magic, a
setup mistake fixed in the next run). Each queued kick replaces the next
ordinary hit and the prompt keeps `[smw]` throughout; no recovery notices
are printed.

```
[  79.91] >>> kill fremlin
[  79.93] 27/27H 830/830V [s-w] (news) (motd) >
[  79.93] [R: 6]You try to hit a fremlin who easily avoids the blow.
[  79.93] 27/27H 830/830V [s-w] (news) (motd) >
[  81.92] [R:18]  [3] You hit a fremlin hard.
[  81.92] 27/27H 830/830V [smw] (news) (motd) >
[  83.91] [R: 4]A fremlin tries to hit you but you easily avoid the blow.
[  83.91] 27/27H 830/830V [smw] (news) (motd) >
[  87.92] [R:16]  [4] You hit a fremlin very hard.
[  87.92] 27/27H 830/830V [smw] (news) (motd) >
[  87.92] >>> cast 'mage armor' me
[  87.95] 27/27H 830/830V [smw] (news) (motd) >
[  87.99] Your magic fizzles out and dies.
[  87.99] 27/27H 830/830V [smw] (news) (motd) >
[  89.92] [R:11]  [6] A fremlin injures you harshly with his hit.
[  89.92] 21/27H 830/830V [smw] (news) (motd) >
[  93.23] 21/27H 830/830V [smw] (news) (motd) >
[  93.24] 22/27H 830/830V [smw] (news) (motd) >
[  93.91] [R:15]  [3] You hit a fremlin very hard.
[  93.91] 22/27H 830/830V [smw] (news) (motd) >
[  95.91] [R:15]  [3] A fremlin hits you extremely hard.
[  95.91] 19/27H 830/830V [smw] (news) (motd) >
[  99.91] [R:13]  [4] You injure a fremlin with your hit.
[  99.91] 19/27H 830/830V [smw] (news) (motd) >
[ 100.11] >>> kick
[ 100.12] 19/27H 830/830V [smw] (news) (motd) >
[ 100.12] Attack queued.
[ 100.12] 19/27H 830/830V [smw] (news) (motd) >
[ 101.91] [R: 7]A fremlin tries to hit you but you easily avoid the blow.
[ 101.91] 19/27H 830/830V [smw] (news) (motd) >
[ 105.94]   [2] Your kick hits a fremlin in the solar plexus!
[ 105.94] *(Challenge:13<Refl:19) Opponent Saved!*
[ 105.94] 19/27H 830/830V [smw] (news) (motd) >
[ 107.91] [R: 7]You duck under a fremlin's fist as he takes a swing at you.
[ 107.91] 19/27H 830/830V [smw] (news) (motd) >
[ 110.12] >>> kick
[ 110.12] 20/27H 830/830V [smw] (news) (motd) >
[ 110.12] Attack queued.
[ 110.12] 20/27H 830/830V [smw] (news) (motd) >
[ 111.94] You miss your kick at a fremlin's groin, much to his relief...
[ 111.94] 20/27H 830/830V [smw] (news) (motd) >
[ 115.95] [R: 6]You barely avoid a fremlin's fist as he takes a swing at you!
[ 115.95] 21/27H 830/830V [smw] (news) (motd) >
[ 118.17] >>> initiative
[ 118.17] 21/27H 830/830V [smw] (news) (motd) >
[ 118.17] Initiative - Upcoming combat phases
[ 118.17]  #  Roll  Phase  In (sec)  Combatant
[ 118.17] --  ----  -----  --------  ---------
[ 118.17]  1.    3      3       1.8  a fremlin
[ 118.17]  2.   12      1       1.8  Revsorc (you)
[ 118.17] 21/27H 830/830V [smw] (news) (motd) >
[ 119.95] [R: 3]You try to hit a fremlin who easily avoids the blow.
[ 119.95] 21/27H 830/830V [smw] (news) (motd) >
[ 120.35] >>> queue
```

## Workflow: timed casting mid-fight

Room 145203. The first cast starts one second after admission, prints four
progress lines, completes, and spends the move action (`[s-w]`), which is
back exactly six seconds later (`[smw]` at 44.91). Automatic attacks are
suppressed while casting (only the fremlin swings at 38.91) and resume
afterward without a catch-up burst. A second cast is interrupted by damage.
The queued kick again replaces the next hit.

```
[  30.98] >>> kill fremlin
[  31.00] 27/27H 830/830V [smw] (news) (motd) >
[  31.00] [R:13]  [4] You hit a fremlin very hard.
[  31.00] 27/27H 830/830V [smw] (news) (motd) >
[  32.92] [R: 2]A fremlin misses a wild punch at you.
[  32.92] [R: 5]You swing your fist at a fremlin, but miss him!
[  32.92] 27/27H 830/830V [smw] (news) (motd) >
[  37.14] >>> cast 'mage armor' me
[  37.16] 27/27H 830/830V [s-w] (news) (motd) >
[  37.16] You begin casting your spell...
[  37.16] 27/27H 830/830V [s-w] (news) (motd) >
[  38.11] Casting: mage armor ....
[  38.11] A faint rift begins to form in reality...
[  38.11] 27/27H 830/830V [s-w] (news) (motd) >
[  38.91] [R: 9]You barely avoid a fremlin's fist as he takes a swing at you!
[  38.91] 27/27H 830/830V [s-w] (news) (motd) >
[  39.11] Casting: mage armor ...
[  39.11] The tear in space widens, revealing glimpses beyond...
[  39.11] 27/27H 830/830V [s-w] (news) (motd) >
[  40.11] Casting: mage armor ..
[  40.11] Extraplanar energy pours through the opening...
[  40.11] 27/27H 830/830V [s-w] (news) (motd) >
[  41.12] Casting: mage armor .
[  41.12] Reality bends dangerously around you!
[  41.12] You complete your spell...You feel someone protecting you.
[  41.12] 27/27H 830/830V [s-w] (news) (motd) >
[  44.91] [R: 5]You barely avoid a fremlin's fist as he takes a swing at you!
[  44.91] [R: 9]A fremlin ducks under your fist as you try to hit him.
[  44.91] 27/27H 830/830V [smw] (news) (motd) >
[  47.32] >>> cast 'mage armor' me
[  47.35] 27/27H 830/830V [s-w] (news) (motd) >
[  47.35] You begin casting your spell...
[  47.35] 27/27H 830/830V [s-w] (news) (motd) >
[  48.36] Casting: mage armor ....
[  48.36] A faint rift begins to form in reality...
[  48.36] 27/27H 830/830V [s-w] (news) (motd) >
[  49.31] Casting: mage armor ...
[  49.31] The tear in space widens, revealing glimpses beyond...
[  49.31] 27/27H 830/830V [s-w] (news) (motd) >
[  50.33] Casting: mage armor ..
[  50.33] Extraplanar energy pours through the opening...
[  50.33] 27/27H 830/830V [s-w] (news) (motd) >
[  50.92] [R:19]The damage breaks your concentration!
[  50.92] You stop casting (damage interrupted it).
[  50.92]   [4] A fremlin hits you extremely hard.
[  50.92] 23/27H 830/830V [s-w] (news) (motd) >
[  57.51] >>> kick
[  57.51] 23/27H 830/830V [smw] (news) (motd) >
[  57.51] Attack queued.
[  57.51] 23/27H 830/830V [smw] (news) (motd) >
[  58.95] [R: 7]A fremlin tries to hit you but you easily avoid the blow.
[  58.95] You miss your kick at a fremlin's groin, much to his relief...
[  58.95] 23/27H 830/830V [smw] (news) (motd) >
[  63.59] >>> cast 'mage armor' me
[  63.61] 24/27H 830/830V [s-w] (news) (motd) >
[  63.61] You begin casting your spell...
[  63.61] 24/27H 830/830V [s-w] (news) (motd) >
[  64.59] Casting: mage armor ....
[  64.59] A faint rift begins to form in reality...
[  64.59] 24/27H 830/830V [s-w] (news) (motd) >
[  64.88] [R: 7]You barely avoid a fremlin's fist as he takes a swing at you!
[  64.88] 24/27H 830/830V [s-w] (news) (motd) >
[  65.58] Casting: mage armor ...
[  65.58] The tear in space widens, revealing glimpses beyond...
[  65.58] 24/27H 830/830V [s-w] (news) (motd) >
[  66.58] Casting: mage armor ..
[  66.58] Extraplanar energy pours through the opening...
[  66.58] 25/27H 830/830V [s-w] (news) (motd) >
[  67.58] Casting: mage armor .
[  67.58] Reality bends dangerously around you!
[  67.58] You complete your spell...You feel someone protecting you.
[  67.58] 25/27H 830/830V [s-w] (news) (motd) >
[  70.88] [R: 5]You duck under a fremlin's fist as he takes a swing at you.
[  70.88] [R:17]  [4] You hit a fremlin very hard.
[  70.88] 26/27H 830/830V [smw] (news) (motd) >
[  71.68] >>> initiative
```

## NPC caster: the Red Magi (mob 128, level 10 wizard)

This run first exposed a regression: a level 3 NPC wizard (the smelly bum,
mob 168) only punched, and tracing showed the baseline's in-combat NPC
race/class/spell behavior had no caller at all since the refactor. With that
dispatch restored (`npc_combat_behave()`, once per rotation at phase 1), the
Red Magi chants with the historical two-second NPC casting steps, loses a
spell to damage, calls a familiar, and casts again on later rotations. The
sorcerer is restored by staff every few seconds to survive.

```
[  22.42] >>> kill magi
[  22.43] 27/27H 830/830V [smw] (news) (motd) >
[  22.43] [R:16]  [8] You hit the Red Magi.
[  22.43] 27/27H 830/830V [smw] (news) (motd) >
[  24.40] [R:20][CRIT!] [16] You hit the Red Magi extremely hard.
[  24.40] 27/27H 830/830V [smw] (news) (motd) >
[  26.40] [AOO][R: 5]The Red Magi ducks under your fist as you try to hit him.
[  26.40] The Red Magi scrambles to his feet!
[  26.40] [stum!][R: 1]The Red Magi misses a wild punch at you.
[  26.40] 27/27H 830/830V [smw] (news) (motd) >
[  27.42] 27/27H 830/830V [smw] (news) (motd) >
[  32.57] [R: 3]You wildly punch at the air, missing the Red Magi.
[  32.57] 27/27H 830/830V [smw] (news) (motd) >
[  34.57] [R:10]You duck under the Red Magi's fist as he takes a swing at you.
[  34.57] 27/27H 830/830V [smw] (news) (motd) >
[  35.61] 27/27H 830/830V [smw] (news) (motd) >
[  38.57] [R: 7]You try to hit the Red Magi who easily avoids the blow.
[  38.57] 27/27H 830/830V [smw] (news) (motd) >
[  40.57] [R: 5]The Red Magi tries to hit you but you easily avoid the blow.
[  40.57] The Red Magi weaves his hands in an intricate pattern and begins to chant the words, 'qahif gsgrul'
[  40.57] 27/27H 830/830V [smw] (news) (motd) >
[  42.57] The air around the Red Magi shimmers oddly...
[  42.57] 27/27H 830/830V [smw] (news) (motd) >
[  43.58] Reality seems to blur near the Red Magi...
[  43.58] The Red Magi stares at you and utters the words, 'qahif gsgrul'.
[  43.58] *Save Roll Twenty! You absorb all the damage! (0)
[  43.58] *(Will:9<Challenge:20) Failed Save!* You are stunned by the colors!
[  43.58] 27/27H 830/830V [smw] (news) (motd) >
[  43.62] 27/27H 830/830V [smw] (news) (motd) >
[  44.57] [R:16]You try to hit the Red Magi who easily avoids the blow.
[  44.57] 27/27H 830/830V [smw] (news) (motd) >
[  46.57] [R:18]  [4] The Red Magi hits you extremely hard.
[  46.57] The Red Magi's concentration is lost, and spell is aborted!
[  46.57] 23/27H 830/830V [smw] (news) (motd) >
[  50.57] [R: 8]You try to hit the Red Magi who easily avoids the blow.
[  50.57] 24/27H 830/830V [smw] (news) (motd) >
[  51.62] 27/27H 830/830V [smw] (news) (motd) >
[  52.42] >>> initiative
[  52.43] 27/27H 830/830V [smw] (news) (motd) >
[  52.43] You try, but you are unable to move due to being stunned!
[  52.43] 27/27H 830/830V [smw] (news) (motd) >
[  52.58] [R:19]  [5] The Red Magi injures you with his hit.
[  52.58] 22/27H 830/830V [smw] (news) (motd) >
[  56.58] [R: 9]You wildly punch at the air, missing the Red Magi.
[  56.58] 22/27H 830/830V [smw] (news) (motd) >
[  58.57] [R: 5]The Red Magi misses a wild punch at you.
[  58.57] The Red Magi calls an eagle!
[  58.57] 22/27H 830/830V [smw] (news) (motd) >
[  59.63] 27/27H 830/830V [smw] (news) (motd) >
[  64.72] [R:10]The Red Magi ducks under your fist as you try to hit him.
[  64.72] 27/27H 830/830V [smw] (news) (motd) >
[  65.39] Your 'color spray' effect has expired
[  65.39] 27/27H 830/830V [smw] (news) (motd) >
[  66.72] [R:20]  [5] The Red Magi injures you with his hit.
[  66.72] The Red Magi weaves his hands in an intricate pattern and begins to chant the words, 'mosailla pibmg
```

## Group assist

The wizard character follows the sorcerer, enables autoassist, leaves its
solo group, and joins the sorcerer's. When the sorcerer attacks, the wizard
joins the fight on its own initiative offset; the three combatants alternate
on their own two-second phases.

```
[  26.42] >>> group join Revsorc
[  26.42] 28/28H 830/830V [smw] (news) (motd)
[  26.42] [Group] Revwiz joins the group.
[  26.42] 28/28H 830/830V [smw] (news) (motd)
[  30.83] A fremlin barely avoids a blow from Revsorc!
[  30.83] 28/28H 830/830V [smw] (news) (motd)
[  32.74] You join the fight!
[  32.74] Revsorc hits a fremlin hard.
[  32.74] 28/28H 830/830V [smw] (news) (motd) >
[  34.74] [R: 8]You swing your fist at a fremlin, but miss him!
[  34.74] A fremlin practices shadow-boxing while Revsorc takes a break.
[  34.74] 28/28H 830/830V [smw] (news) (motd) >
[  38.74] A fremlin ducks under Revsorc's fist as he takes a swing at him.
```
