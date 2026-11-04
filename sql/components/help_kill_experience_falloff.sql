-- Match the Experience help entry in lib/text/help/help.hlp.
-- Keep any existing text and apply the addition at most once.
UPDATE help_entries
SET
  entry = INSERT(
    entry,
    LOCATE('ACCOUNT EXPERIENCE (ACCEXP):', entry),
    0,
    'KILL EXPERIENCE:
At the default setting, a monster exactly 5 levels below you pays 25% of its
normal kill XP after the per-kill caps. Each additional level below you halves
that award again, rounded down to a minimum of 1 XP for an eligible kill.
For groups, compare the monster to the average level of group members in the
room, rounded down and excluding pets.

'
  )
WHERE
  tag = 'experience'
  AND LOCATE('ACCOUNT EXPERIENCE (ACCEXP):', entry) > 0
  AND LOCATE('KILL EXPERIENCE:', entry) = 0;
