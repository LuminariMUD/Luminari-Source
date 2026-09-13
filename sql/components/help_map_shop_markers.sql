-- ASCII map shop markers and command usage. Matches lib/text/help/help.hlp.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES
('automapping', 'Usage: map [distance] [normal|world]

The map command prints an ASCII map of your surroundings. Distance can be
1 to 12; omitting it uses the configured default. Normal mode shows rooms
and exits. World mode shows one symbol per room. For example, use map,
map world, map 6 normal, or map 6 world.

Shop rooms appear as [$] in normal mode and $ in world mode. Your current
room always shows &, including when you are standing in a shop. A room with
several shops shows one $; the marker remains when a shop is closed or its
keeper is absent. It marks a shop location, not current vendor availability.
Other rooms keep their terrain symbols. The same shop markers appear on
the automatic minimap. Wilderness uses its separate terrain map.

Usage: toggle automap on

Automap displays a minimap with room descriptions. Use toggle automap off
to hide it. Screen-reader mode also hides automatic maps; the map command
remains available.

See also: MAP, GUI-MAP, WILDERNESS, SCREEN-READER
', 0, 0)
ON DUPLICATE KEY UPDATE entry=VALUES(entry), min_level=VALUES(min_level),
                        auto_generated=VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('automapping', 'AUTOMAPPING'),
('automapping', 'MAPS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES
('gui-map', 'Use map or map world for an ASCII map of nearby rooms. Shop locations show
$ (even when closed), and & marks your current room, including inside a shop.
You can set the distance with map 6 normal or map 6 world. See AUTOMAPPING
for the ASCII map and automatic minimap options.

If you are using Luminari''s GUI, a mapper will appear on the top right.

To use the mapper:
start mapping - to begin actively mapping, move slower, check your map accuracy
stop mapping - to pause active mapping, this will help protect your map during
     fast movement

The mapper does try to sync actively with the server, BUT due to network latency,
etc...  it is very possible to manually mess up your map while you are actively
mapping (start mapping).  You can manually modify your map by clicking, right-
clicking and dragging elements.  You can also add labels and do various other
functions using the mapper.

See also:  AUTOMAPPING
', 0, 0)
ON DUPLICATE KEY UPDATE entry=VALUES(entry), min_level=VALUES(min_level),
                        auto_generated=VALUES(auto_generated);

-- Reclaim aliases from earlier imports or the previous migration's map tag.
DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('GUI-MAP', 'MAP', 'MAPPER', 'MAPPING')
  AND help_tag <> 'gui-map';

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('gui-map', 'GUI-MAP'),
('gui-map', 'MAP'),
('gui-map', 'MAPPER'),
('gui-map', 'MAPPING');

COMMIT;
