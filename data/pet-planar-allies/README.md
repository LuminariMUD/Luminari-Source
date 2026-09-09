# Planar allies

These records provide the celestial guardian (19700), healer (19701), and the
Xvim artifact's dedicated nightmare steed (19702).
The prototype-only zone has no resets. Install from a development checkout:

```sh
python3 scripts/world/install_pet_constructs.py
python3 scripts/world/wtool.py validate --paths data/pet-planar-allies/197.mob data/pet-planar-allies/197.zon --strict
```

The shared installer preserves existing area records and refuses collisions.
The records are loaded on the next normal development world restart.

Planar Ally is assigned to Cleric 11 and Summoner 16. Both options are level-15
flying outsiders with angel subrace, zero gold, and the native planar-ally flag.
The guardian emphasizes melee, armor, and DR 5. The healer has native known
spells for blessing, cure critic, and paralysis removal, bounded by the ordinary
two-slot-per-spell recovery system.

Both use the same one-ally allowance, normal spell preparation/actions, permanent
pet lifetime, equipment, orders, and dismissal. The selected prototype and
allowance are checked before committing the prepared spell. See PLANAR-ALLY help
for player-facing rules. Live class and combat acceptance remains required.

The nightmare steed is not a Planar Ally choice. Whispering `nightmare` while
wielding Tyranny calls it through that artifact's existing General-follower path.
The reset-free record prevents unrelated area resets from occupying the artifact's
single-instance summon.
