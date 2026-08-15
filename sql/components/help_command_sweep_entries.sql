-- Player help corrections from the 2026-08-15 full command sweep.
--
-- The database help system is authoritative. This migration is safe to run
-- repeatedly and replaces the affected entries with the current behavior.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('action-queue', 'ACTION QUEUE

Usage:
  queue
  queue clear

Commands that consume an unavailable standard, move, swift, or full-round
action can enter the action queue only after a non-mutating availability check
validates their current arguments. Commands without a safe advance check are
not queued; retry those commands after the required action becomes available.

QUEUE displays pending actions. QUEUE CLEAR removes all pending actions.

See also: ACTIONS, ATTACK-QUEUE, COMBAT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('ACTION-QUEUE', 'QUEUE')
  AND help_tag <> 'action-queue';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('action-queue', 'ACTION-QUEUE');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('action-queue', 'QUEUE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('autoblast', 'AUTOBLAST

Usage:
  autoblast

AUTOBLAST toggles automatic eldritch blasts in place of normal attacks. The
command reports whether automatic blasting is now enabled or disabled.

See also: ELDRITCH-BLAST, WARLOCK', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'AUTOBLAST' AND help_tag <> 'autoblast';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('autoblast', 'AUTOBLAST');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('consumables', 'STORED CONSUMABLES

Usage:
  usestoredconsumables
  autostore
  store <item>
  store list <potions|scrolls|wands|staves>
  unstore <potion|scroll|wand|staff> <spell name>

USESTOREDCONSUMABLES enables or disables the stored-consumables system and
reports the new setting. AUTOSTORE controls whether eligible consumable items
are stored automatically. Potions, scrolls, wands, and magical staves can be
stored and then used by spell name instead of by inventory object name.

See also: AUTOSTORE, STORE, UNSTORE, USE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('consumables', 'CONSUMABLES');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('consumables', 'STORED-CONSUMABLES');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('consumables', 'USESTOREDCONSUMABLES');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('spellrecall', 'SPELL RECALL

Usage:
  spellrecall

Spell Recall is a wizard perk that can be used once per real-world day. It
instantly completes one randomly selected spell currently being prepared. For
spontaneous caster levels, it instead immediately recovers one randomly
selected spell slot. Spell Recall is not consumed when there is nothing to
recover.

See also: PREPARATION, PERKS, SPELLS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('SPELLRECALL', 'SPELL-RECALL')
  AND help_tag <> 'spellrecall';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('spellrecall', 'SPELLRECALL');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('spellrecall', 'SPELL-RECALL');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('forum', 'LUMINARI COMMUNITY

Visit the Luminari website at https://luminarimud.com/ for current community
links and project information.

For help in game, send game mail to Ornir. For email contact, write to
ornir@luminarimud.com.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('forum', 'FORUM');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('forum', 'LUMINARI-WEBSITE');

COMMIT;
