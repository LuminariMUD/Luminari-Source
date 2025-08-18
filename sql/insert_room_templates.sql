INSERT INTO ship_room_templates (room_type, name_format, description_text, room_flags, sector_type, min_vessel_size) VALUES
-- Bridge/Control rooms
('bridge', 'The %s''s Bridge', 'The command center of the vessel, filled with navigation equipment and control panels. Large windows provide a panoramic view of the surroundings.', 262144, 0, 0),
('helm', 'The %s''s Helm', 'The pilot''s station, featuring the ship''s wheel and primary navigation controls.', 262144, 0, 0),

-- Crew areas
('quarters_captain', 'Captain''s Quarters', 'A spacious cabin befitting the ship''s commander, with a large bunk, desk, and personal storage.', 262144, 0, 3),
('quarters_crew', 'Crew Quarters', 'Rows of bunks line the walls of this cramped but functional sleeping area.', 262144, 0, 1),
('quarters_officer', 'Officers'' Quarters', 'Modest private cabins for the ship''s officers, each with a bunk and small desk.', 262144, 0, 5),

-- Cargo and storage
('cargo_main', 'Main Cargo Hold', 'A cavernous space filled with crates, barrels, and secured cargo. The air smells of tar and sea salt.', 262144, 0, 2),
('cargo_secure', 'Secure Cargo Hold', 'A reinforced compartment with heavy locks, used for valuable or dangerous cargo.', 262144, 0, 5),

-- Ship systems
('engineering', 'Engineering', 'The heart of the ship''s mechanical systems, filled with pipes, gauges, and machinery.', 262144, 0, 3),
('weapons', 'Weapons Deck', 'Cannons and ballistae line this reinforced deck, ready for naval combat.', 262144, 0, 5),
('armory', 'Ship''s Armory', 'Racks of weapons and armor line the walls, secured behind iron bars.', 262144, 0, 7),

-- Common areas
('mess_hall', 'Mess Hall', 'Long tables and benches fill this communal dining area. The lingering smell of the last meal hangs in the air.', 262144, 0, 3),
('galley', 'Ship''s Galley', 'The ship''s kitchen, equipped with a large stove and food preparation areas.', 262144, 0, 2),
('infirmary', 'Ship''s Infirmary', 'A small medical bay with cots and basic healing supplies.', 262144, 0, 4),

-- Connectivity
('corridor', 'Ship''s Corridor', 'A narrow passageway connecting different sections of the vessel.', 262144, 0, 0),
('deck_main', 'Main Deck', 'The open deck of the ship, exposed to the elements. Rigging and masts tower overhead.', 262144, 0, 0),
('deck_lower', 'Lower Deck', 'Below the main deck, this area provides access to the ship''s interior compartments.', 262144, 0, 1),

-- Special rooms
('airlock', 'Airlock', 'A sealed chamber used for boarding and emergency exits.', 262144, 0, 10),
('observation', 'Observation Deck', 'An elevated platform providing excellent views in all directions.', 262144, 0, 5),
('brig', 'Ship''s Brig', 'Iron-barred cells for holding prisoners or unruly crew members.', 262144, 0, 6)
ON DUPLICATE KEY UPDATE description_text = VALUES(description_text);