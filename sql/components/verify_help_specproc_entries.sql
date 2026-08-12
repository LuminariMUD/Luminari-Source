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
  AND INSTR(entry, 'RoL conjured-death flags') > 0
  AND INSTR(entry, 'breath_attack and breath_weapon') > 0
  AND INSTR(entry, 'RoL Magic Pool is object-owned') > 0
  AND INSTR(entry, 'fixed entry damage') > 0
  AND INSTR(entry, 'RoL Auto Distributor is room-owned') > 0
  AND INSTR(entry, 'random loaded room in the same zone') > 0
  AND INSTR(entry, 'RoL Shadow Giant is mobile-owned') > 0
  AND INSTR(entry, 'one-in-21 chance') > 0
  AND INSTR(entry, 'RoL Guild Room') > 0
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
