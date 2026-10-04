-- Vessel System Phase 22: DurisMUD study step S6, NPC raiders
-- (docs/adr/0003-durismud-naval-model.md, 3.3.8). Mirrors the runtime DDL in
-- vessel_raider_ensure_schema() (src/vessels/vessels_raiders.c).

-- Each row lets one ship prototype sail as a raider of one tier (0-3). A
-- prototype may serve several tiers; the tier sets the raider's fit-out,
-- crew, and brain. Content rows come from vessels_raider_content.sql.
CREATE TABLE IF NOT EXISTS vessel_raider_tiers (
  tier TINYINT NOT NULL,
  prototype_id INT NOT NULL,
  PRIMARY KEY (tier, prototype_id)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;
