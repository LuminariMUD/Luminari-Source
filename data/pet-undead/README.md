# Undead spellcaster prototypes

Install in a development checkout with `python3 scripts/world/install_pet_constructs.py`.
The installer checks collisions before changing the indexed world files. Zone 198 has no resets.

`animatedead mage` unlocks at composite caster level 10 and costs one undead control point.
Its level-10 skeletal mage knows Magic Missile and Ray of Enfeeblement.
`animatedead lich` unlocks at composite caster level 25 and costs two control points.
Its level-25 lich knows Magic Missile, Vampiric Touch, Haste, and Dispel Magic.
Both are physical undead using native undead defenses and ordinary equipment rules.

Each successful call costs one daily ANIMATEDEAD use and one standard action, consumes no
corpse, and has no natural expiry. Missing prototypes, locked forms, and full capacity retain
the use. Spell slots use the native mobile limit of two per known spell and normal recovery.
Use ORDER for casting and DISMISS after retrieving equipment; dismissal does not refund a use.
Summoned corpses cannot be animated again. Persistence uses the existing pet path.
