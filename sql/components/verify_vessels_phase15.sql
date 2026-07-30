-- Vessel System Phase 15 verification.

SELECT COUNT(*) AS vessel_phase15_tables_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME IN (
     'vessel_hunter_encounters',
     'vessel_bounty_hunts'
   );

SELECT COUNT(*) AS vessel_hunter_definition_columns_present
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'vessel_hunter_encounters'
   AND COLUMN_NAME IN (
     'encounter_id',
     'prototype_id',
     'pilot_mob_vnum',
     'min_bounty',
     'pursuit_speed',
     'hunt_duration_seconds',
     'target_grace_seconds',
     'cooldown_seconds',
     'enabled'
   );

SELECT hunter.encounter_id, encounter.name, hunter.prototype_id,
       hunter.pilot_mob_vnum, hunter.min_bounty, hunter.pursuit_speed,
       hunter.hunt_duration_seconds, hunter.target_grace_seconds,
       hunter.cooldown_seconds
  FROM vessel_hunter_encounters AS hunter
  LEFT JOIN vessel_encounters AS encounter
    ON encounter.encounter_id = hunter.encounter_id
  LEFT JOIN ship_prototypes AS prototype
    ON prototype.prototype_id = hunter.prototype_id
 WHERE encounter.encounter_id IS NULL
    OR prototype.prototype_id IS NULL
    OR prototype.vessel_class <> 3
    OR hunter.prototype_id <= 0
    OR hunter.pilot_mob_vnum <= 0
    OR hunter.min_bounty < 2000
    OR hunter.pursuit_speed NOT BETWEEN 1 AND 100
    OR hunter.hunt_duration_seconds NOT BETWEEN 10 AND 86400
    OR hunter.target_grace_seconds NOT BETWEEN 0 AND 600
    OR hunter.cooldown_seconds NOT BETWEEN 1 AND 604800;

SELECT hunt.target_player, hunt.encounter_id, hunt.target_ship_id,
       hunt.hunter_ship_id, hunt.generation, hunt.status,
       hunt.expires_at, hunt.next_eligible_at, hunt.end_reason
  FROM vessel_bounty_hunts AS hunt
  LEFT JOIN vessel_hunter_encounters AS hunter
    ON hunter.encounter_id = hunt.encounter_id
  LEFT JOIN ship_runtime_state AS runtime
    ON runtime.ship_id = hunt.hunter_ship_id
 WHERE hunter.encounter_id IS NULL
    OR hunt.status NOT IN ('active', 'spawning', 'cooldown')
    OR (hunt.status = 'active' AND runtime.ship_id IS NULL)
    OR (hunt.status = 'active' AND hunt.hunter_ship_id IS NULL)
    OR (hunt.status <> 'active' AND hunt.hunter_ship_id IS NOT NULL)
    OR (hunt.status = 'cooldown' AND hunt.next_eligible_at <= 0);
