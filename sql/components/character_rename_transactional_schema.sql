-- Character rename transaction prerequisite.
--
-- Back up and validate these tables before applying this migration. ALTER
-- TABLE performs an implicit commit in MySQL/MariaDB and can take a metadata
-- lock while the table is rebuilt.

ALTER TABLE player_mail ENGINE=InnoDB;
ALTER TABLE player_mail_deleted ENGINE=InnoDB;
ALTER TABLE player_mail_read ENGINE=InnoDB;

CREATE INDEX IF NOT EXISTS idx_player_mail_sender
  ON player_mail (sender);
CREATE INDEX IF NOT EXISTS idx_player_mail_receiver
  ON player_mail (receiver);
CREATE INDEX IF NOT EXISTS idx_player_mail_deleted_player
  ON player_mail_deleted (player_name);
CREATE INDEX IF NOT EXISTS idx_player_mail_read_player
  ON player_mail_read (player_name);

SELECT TABLE_NAME, ENGINE
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME IN (
    'player_mail',
    'player_mail_deleted',
    'player_mail_read'
  )
ORDER BY TABLE_NAME;
