-- Vessel System Phase 11 rollback.
-- Destructive: generated interiors stop receiving configured DG triggers.

DROP TABLE IF EXISTS ship_room_template_triggers;
