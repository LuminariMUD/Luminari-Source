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
  'required_keywords' AS check_name,
  COUNT(*) AS actual,
  5 AS expected,
  IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('spec-proc', 'SPEC'),
  ('spec-proc', 'SPEC-PROC'),
  ('spec-proc', 'SPECIAL-PROCEDURE'),
  ('spec-proc', 'SPECIALS'),
  ('spec-proc', 'SPECPROC')
);

SELECT
  'conflicting_keywords' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN (
  '<SPEC>', 'SPEC', 'SPEC-PROC', 'SPECIAL-PROCEDURE', 'SPECIALS', 'SPECPROC'
)
AND BINARY help_tag <> 'spec-proc';
