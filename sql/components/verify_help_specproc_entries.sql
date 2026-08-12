-- Read-only verification for help_specproc_entries.sql.

SELECT
  'entry_contract' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE BINARY tag = 'spec-proc'
  AND min_level = 31
  AND auto_generated = FALSE
  AND INSTR(entry, 'Z) SpecProc') > 0
  AND INSTR(entry, 'moving room cannot also have a named room SpecProc') > 0;

SELECT
  'content_contract' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE BINARY tag = 'spec-proc'
  AND INSTR(entry, 'canonical names only') > 0
  AND INSTR(entry, 'Explicit aliases remain load-compatible') > 0
  AND INSTR(entry, 'MOB_SPEC') > 0
  AND INSTR(entry, 'ITEM_AUTOPROC') > 0
  AND INSTR(entry, 'preserves the exact authored request') > 0
  AND INSTR(entry, 'Select a menu entry to replace') > 0
  AND INSTR(entry, '0 to clear the authored procedure') > 0
  AND INSTR(entry, 'SPEC_BIND_FINAL') > 0
  AND INSTR(entry, 'SPECBIND <mob|obj|room> <vnum>') > 0
  AND INSTR(entry, 'SPECBIND is read-only') > 0
  AND INSTR(entry, 'not a persisted multiple-procedure dispatch chain') > 0
  AND INSTR(entry, '-s is not a global callback-disable switch') > 0
  AND INSTR(entry, 'RoL-Demon') > 0
  AND INSTR(entry, 'Independent compatibility hooks') > 0
  AND INSTR(entry, 'RoL death flags') > 0
  AND INSTR(entry, 'Bloodstone black-vapor') > 0
  AND INSTR(entry, 'Sixteen additional converted tentacle') > 0
  AND INSTR(entry, 'breath_attack and breath_weapon') > 0
  AND INSTR(entry, 'RoL Bloodstone Critter is mobile-owned') > 0
  AND INSTR(entry, 'two-in-81 activity-pulse cadence') > 0
  AND INSTR(entry, 'RoL Designated Follower is mobile-owned') > 0
  AND INSTR(entry, 'RoL Floating Pool is object-owned') > 0
  AND INSTR(entry, '12 percent chance per object pulse') > 0
  AND INSTR(entry, 'RoL Bloodstone Portal is object-owned') > 0
  AND INSTR(entry, '1-20 hit points and 1-30 movement points') > 0
  AND INSTR(entry, 'RoL Item Blocker is object-owned') > 0
  AND INSTR(entry, 'six converted ATD objects') > 0
  AND INSTR(entry, 'RoL Magic Pool is object-owned') > 0
  AND INSTR(entry, 'fixed entry damage') > 0
  AND INSTR(entry, 'RoL Auto Distributor is room-owned') > 0
  AND INSTR(entry, 'random loaded room in the same zone') > 0
  AND INSTR(entry, 'RoL Shadow Giant is mobile-owned') > 0
  AND INSTR(entry, 'one-in-21 chance') > 0
  AND INSTR(entry, 'RoL Guild Guard is mobile-owned') > 0
  AND INSTR(entry, 'room-specific class and race gates') > 0
  AND INSTR(entry, 'RoL Waterdeep Guild Room is room-owned') > 0
  AND INSTR(entry, 'Source Mercenary maps to target Warrior') > 0
  AND INSTR(entry, 'RoL Major Beholder is mobile-owned') > 0
  AND INSTR(entry, 'independent three-combat-turn cooldown') > 0
  AND INSTR(entry, 'RoL Lich Energy Drain is mobile-owned') > 0
  AND INSTR(entry, 'one-in-five chance to lose all current hit points') > 0
  AND INSTR(entry, 'unless Blackmantled') > 0
  AND INSTR(entry, 'RoL Trade Bandit is mobile-owned') > 0
  AND INSTR(entry, 'Source platinum maps to ten') > 0
  AND INSTR(entry, 'target gold. Pay with GIVE') > 0
  AND INSTR(entry, 'RoL Alert Caller is mobile-owned') > 0
  AND INSTR(entry, 'compose this alert beside their existing breath procedure') > 0
  AND INSTR(entry, 'RoL Yggdrasil Branch is mobile-owned') > 0
  AND INSTR(entry, 'four to twelve combat rounds') > 0
  AND INSTR(entry, 'RoL Waterdeep Ambient is mobile-owned') > 0
  AND INSTR(entry, 'casino-player fall-through') > 0
  AND INSTR(entry, 'RoL Source Periodic is mobile-owned') > 0
  AND INSTR(entry, '90 source') > 0
  AND INSTR(entry, 'jester 2003069, and cricket 2014048 do not require') > 0
  AND INSTR(entry, 'Menden magus 2088806 continue') > 0
  AND INSTR(entry, 'RoL Stateful Periodic is mobile-owned') > 0
  AND INSTR(entry, '206 source') > 0
  AND INSTR(entry, 'Guildmaster 2003020 has no authored') > 0
  AND INSTR(entry, 'RoL Sister Knight is mobile-owned') > 0
  AND INSTR(entry, 'reachable converted sister within 100 rooms') > 0
  AND INSTR(entry, 'RoL Shaman Totem is object-owned') > 0
  AND INSTR(entry, 'three attempts per seven MUD days') > 0
  AND INSTR(entry, 'RoL-Totem-Spirit') > 0
  AND INSTR(entry, 'five RoL Ship procedures') > 0
  AND INSTR(entry, 'RoL Ship Control handles panel instruments') > 0
  AND INSTR(entry, 'converter-owned') > 0
  AND INSTR(entry, 'RoL Guild Room') > 0
  AND INSTR(entry, 'RoL Mage Guild Room') > 0
  AND INSTR(entry, 'target multiclass model') > 0
  AND INSTR(entry, 'available only in redit') > 0;

SELECT
  'required_keywords' AS check_name,
  COUNT(*) AS actual,
  6 AS expected,
  IF(COUNT(*) = 6, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('spec-proc', 'SPEC'),
  ('spec-proc', 'SPEC-PROC'),
  ('spec-proc', 'SPECIAL-PROCEDURE'),
  ('spec-proc', 'SPECIALS'),
  ('spec-proc', 'SPECBIND'),
  ('spec-proc', 'SPECPROC')
);

SELECT
  'conflicting_keywords' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN (
  '<SPEC>', 'SPEC', 'SPEC-PROC', 'SPECIAL-PROCEDURE', 'SPECIALS', 'SPECBIND', 'SPECPROC'
)
AND BINARY help_tag <> 'spec-proc';
