# Lycanthrope source

Install the werewolf and weretiger prototypes, mooncall wand, and its producing
stock entry in the existing Training Halls Shop from a development checkout:

```sh
python3 scripts/world/install_pet_constructs.py
```

The installer preserves other shop stock and rejects world-range collisions.
The new object and shop stock become available on the next world boot.

The mooncall wand (object 19500) has one charge of Call Lycanthrope (607),
uses the native wand activation path, and has a base cost of 5000 gold. Shop
14100 in room 14125 supplies it. No class or domain spell assignment changes.
DC 20 Use Magic Device supplies player access. The caster-level field is 20;
the summon handler deliberately derives creature level from the user instead.

Missing prototypes and occupied lycanthrope capacity preserve the charge.
The ordinary wand check also preserves charges on failure. Once admitted, the
native spell owns acquisition, control, 30-second expiry/control checks, and
potential hostility. See CALL-LYCANTHROPE/MOONCALL help for player rules.
