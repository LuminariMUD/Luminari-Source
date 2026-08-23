-- Mobile Damage trigger builder help.
--
-- The database help system is authoritative. This entry mirrors the shipped
-- flat-file help and is safe to apply repeatedly.

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('TRIGEDIT-MOB-DAMAGE', 'Mobile Damage Trigger

Activated before positive pending damage routed through the combat damage()
function is applied to an uncharmed mobile. Numeric Arg is the 0-100 activation
chance; Arguments is not used. The first idle attached Damage trigger whose
chance succeeds runs. Damage is option 21 when Intended for is Mobiles.

This is a synchronous, pre-mitigation hook. No explicit return preserves the
pending damage. An explicit -1 cancels the hit, 0 makes it a miss, and a
positive value replaces the pending amount. Values below -1 cancel. Later
combat defenses, reductions, redirects, and the damage cap can still change
the actual hit-point loss.

Damage does not fire for misses or zero damage, player victims, charmed mobs,
protected or rejected attempts, direct DG damage commands, direct hitp changes,
or an instance already running or waiting. A wait cannot delay the current hit:
without an earlier explicit return, the original pending damage is used. A
return after resume cannot revise completed combat.

Variables:
  %self% and %victim% - the mobile receiving pending damage
  %actor% - the source, which can equal self for self-damage
  %damage% - pending damage before later mitigation
  %attacktype% - legacy spell/skill name; physical attacks are UNDEFINED
  %attackid% / %attackname% - stable attack identifier and readable name
  %damagetype% / %damagetypename% - damage type identifier and readable name
  %attackmodeid% / %attackmode% - combat mode identifier and readable name

The minimal-world training dummy and trigger 1 provide a working example.

See also: MOB-TRIGGERS, TRIGEDIT, RETURN, WAIT', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE help_tag = 'TRIGEDIT-MOB-DAMAGE'
  AND keyword NOT IN ('TRIGEDIT-MOB-DAMAGE', 'MOB-DAMAGE-TRIGGER', 'MTRIG-DAMAGE');

DELETE FROM help_keywords
WHERE keyword IN ('TRIGEDIT-MOB-DAMAGE', 'MOB-DAMAGE-TRIGGER', 'MTRIG-DAMAGE')
  AND help_tag <> 'TRIGEDIT-MOB-DAMAGE';

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('TRIGEDIT-MOB-DAMAGE', 'TRIGEDIT-MOB-DAMAGE');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('TRIGEDIT-MOB-DAMAGE', 'MOB-DAMAGE-TRIGGER');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('TRIGEDIT-MOB-DAMAGE', 'MTRIG-DAMAGE');
