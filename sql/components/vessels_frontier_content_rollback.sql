-- Roll back wilderness frontier definitions.
-- Prototypes with persistent runtimes are deliberately retained.

DELETE FROM path_data
 WHERE vnum = 7100104
   AND name = 'Sablebranch River';

DELETE FROM region_data
 WHERE (vnum, name) IN (
   (7100101, 'Starfall Trench'),
   (7100102, 'Aetherwind Skyway'),
   (7100103, 'Shardspire Sky Island')
 );

DELETE FROM ship_prototypes
 WHERE name IN (
   'Sablebranch Raft',
   'Sablebranch Riverboat',
   'Starfall Survey Ship',
   'Starfall Bastion',
   'Aetherwind Courier',
   'Starfall Bathyscaphe',
   'Sablebranch Grand Freighter',
   'Liminal Wayfarer'
 )
   AND NOT EXISTS (
     SELECT 1
       FROM ship_runtime_state AS runtime
      WHERE runtime.prototype_id = ship_prototypes.prototype_id
   );
