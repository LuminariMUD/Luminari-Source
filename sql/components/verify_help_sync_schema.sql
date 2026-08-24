-- Read-only verification for the help synchronization schema contract.

SELECT t.TABLE_NAME, t.ENGINE
FROM information_schema.TABLES t
WHERE t.TABLE_SCHEMA = DATABASE()
  AND t.TABLE_NAME IN ('help_entries', 'help_keywords', 'help_related_topics')
ORDER BY t.TABLE_NAME;

SELECT c.TABLE_NAME, c.COLUMN_NAME, c.COLUMN_TYPE, c.IS_NULLABLE
FROM information_schema.COLUMNS c
WHERE c.TABLE_SCHEMA = DATABASE()
  AND (
    (c.TABLE_NAME = 'help_entries' AND c.COLUMN_NAME IN
      ('tag', 'alternate_keywords', 'entry', 'min_level', 'max_level', 'category',
       'auto_generated', 'last_updated'))
    OR
    (c.TABLE_NAME = 'help_keywords' AND c.COLUMN_NAME IN ('help_tag', 'keyword'))
    OR
    (c.TABLE_NAME = 'help_related_topics' AND c.COLUMN_NAME IN
      ('source_tag', 'related_tag', 'relevance_score'))
    OR
    (c.TABLE_NAME = 'help_versions' AND c.COLUMN_NAME IN
      ('tag', 'alternate_keywords', 'entry', 'min_level', 'max_level', 'category',
       'auto_generated', 'changed_by', 'change_date', 'change_type', 'sync_plan_id'))
  )
ORDER BY c.TABLE_NAME, c.ORDINAL_POSITION;

SELECT version, description, applied_at
FROM schema_migrations
WHERE version BETWEEN 2026082401 AND 2026082408
ORDER BY version;
