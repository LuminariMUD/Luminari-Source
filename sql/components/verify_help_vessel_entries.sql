-- Read-only verification for help_vessel_entries.sql.
--
-- The command-keyword list mirrors the vessel, vehicle, unified transport,
-- autopilot, and staff recovery registrations in src/core/interpreter.c.

SELECT
  'entry_count' AS check_name,
  COUNT(*) AS actual,
  34 AS expected,
  IF(COUNT(*) = 34, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN (
  'VESSELS', 'VEDIT', 'SHIPFIRE', 'SHIPRENOWN', 'SHIPBROWSE', 'SHIPHIRE',
  'MARKET', 'CONTRACTS', 'PLUNDER', 'SEASTATE', 'SHIPLIST',
  'VMERCHANT', 'VESSELDEBUG', 'AUTOPILOT', 'SETWAYPOINT', 'LISTWAYPOINTS',
  'DELWAYPOINT', 'CREATEROUTE', 'ADDTOROUTE', 'DELROUTE',
  'LISTROUTES', 'SETROUTE', 'SETSCHEDULE', 'CLEARSCHEDULE',
  'SHOWSCHEDULE', 'VMOUNT', 'VDISMOUNT', 'DRIVE', 'VSTATUS',
  'VEHICLE-TRANSPORT', 'VEHICLE-ADMIN', 'ASSIGNPILOT',
  'UNASSIGNPILOT', 'VEVENT'
);

SELECT
  'command_keywords' AS check_name,
  COUNT(*) AS actual,
  91 AS expected,
  IF(COUNT(*) = 91, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('VESSELS', 'BOARD'),
  ('VESSELS', 'DISEMBARK'),
  ('VESSELS', 'TACTICAL'),
  ('VESSELS', 'SHIPSTATUS'),
  ('VESSELS', 'SHIPTALK'),
  ('VESSELS', 'SPEED'),
  ('VESSELS', 'HEADING'),
  ('VESSELS', 'SETSAIL'),
  ('VESSELS', 'CONTACTS'),
  ('VESSELS', 'DOCK'),
  ('VESSELS', 'DOCKFEES'),
  ('VESSELS', 'UNDOCK'),
  ('VESSELS', 'ANCHOR'),
  ('VESSELS', 'LOOK_OUTSIDE'),
  ('VESSELS', 'LOOKOUT'),
  ('VESSELS', 'SHIP_ROOMS'),
  ('VESSELS', 'BOARD_HOSTILE'),
  ('VEDIT', 'VEDIT'),
  ('SHIPFIRE', 'SHIPFIRE'),
  ('SHIPFIRE', 'SHIPREPAIR'),
  ('SHIPFIRE', 'CLAIMSHIP'),
  ('SHIPFIRE', 'SHIPSALVAGE'),
  ('SHIPFIRE', 'STRIKECOLORS'),
  ('SHIPFIRE', 'SHIPLOCK'),
  ('SHIPFIRE', 'SHIPSIGHT'),
  ('SHIPFIRE', 'SHIPSCAN'),
  ('SHIPFIRE', 'SHIPRAM'),
  ('SHIPRENOWN', 'SHIPRENOWN'),
  ('SHIPBROWSE', 'SHIPBROWSE'),
  ('SHIPBROWSE', 'SHIPBUY'),
  ('SHIPBROWSE', 'SHIPCHRISTEN'),
  ('SHIPBROWSE', 'SHIPCUSTOMIZE'),
  ('SHIPBROWSE', 'SHIPDEED'),
  ('SHIPBROWSE', 'SHIPPERMIT'),
  ('SHIPBROWSE', 'SHIPREVOKE'),
  ('SHIPBROWSE', 'SHIPCREW'),
  ('SHIPBROWSE', 'SHIPSUMMON'),
  ('SHIPHIRE', 'SHIPHIRE'),
  ('SHIPHIRE', 'SHIPDISMISS'),
  ('SHIPHIRE', 'SHIPUPGRADE'),
  ('SHIPHIRE', 'SHIPWEAPON'),
  ('SHIPHIRE', 'SHIPEQUIP'),
  ('SHIPHIRE', 'SHIPREARM'),
  ('MARKET', 'MARKET'),
  ('MARKET', 'CARGOBUY'),
  ('MARKET', 'CARGOSELL'),
  ('MARKET', 'CARGOMANIFEST'),
  ('CONTRACTS', 'CONTRACTS'),
  ('CONTRACTS', 'CONTRACTACCEPT'),
  ('CONTRACTS', 'CONTRACTDELIVER'),
  ('CONTRACTS', 'CONTRACTABANDON'),
  ('PLUNDER', 'PLUNDER'),
  ('PLUNDER', 'BOUNTY'),
  ('PLUNDER', 'MARQUE'),
  ('SEASTATE', 'SEASTATE'),
  ('SHIPLIST', 'SHIPLIST'),
  ('SHIPLIST', 'SHIPGOTO'),
  ('SHIPLIST', 'SHIPFIX'),
  ('SHIPLIST', 'SHIPPURGE'),
  ('SHIPLIST', 'SHIPLOAD'),
  ('VMERCHANT', 'VMERCHANT'),
  ('VESSELDEBUG', 'VDEBUG'),
  ('VESSELDEBUG', 'VESSELDEBUG'),
  ('VESSELDEBUG', 'VTRADECHECK'),
  ('AUTOPILOT', 'AUTOPILOT'),
  ('SETWAYPOINT', 'SETWAYPOINT'),
  ('LISTWAYPOINTS', 'LISTWAYPOINTS'),
  ('DELWAYPOINT', 'DELWAYPOINT'),
  ('CREATEROUTE', 'CREATEROUTE'),
  ('ADDTOROUTE', 'ADDTOROUTE'),
  ('DELROUTE', 'DELROUTE'),
  ('LISTROUTES', 'LISTROUTES'),
  ('SETROUTE', 'SETROUTE'),
  ('SETSCHEDULE', 'SETSCHEDULE'),
  ('CLEARSCHEDULE', 'CLEARSCHEDULE'),
  ('SHOWSCHEDULE', 'SHOWSCHEDULE'),
  ('VMOUNT', 'VMOUNT'),
  ('VDISMOUNT', 'VDISMOUNT'),
  ('DRIVE', 'DRIVE'),
  ('VSTATUS', 'VSTATUS'),
  ('VEHICLE-TRANSPORT', 'LOADVEHICLE'),
  ('VEHICLE-TRANSPORT', 'UNLOADVEHICLE'),
  ('VEHICLE-TRANSPORT', 'TENTER'),
  ('VEHICLE-TRANSPORT', 'TEXIT'),
  ('VEHICLE-TRANSPORT', 'TGO'),
  ('VEHICLE-TRANSPORT', 'TSTATUS'),
  ('VEHICLE-ADMIN', 'VEHICLECREATE'),
  ('VEHICLE-ADMIN', 'VEHICLEPURGE'),
  ('ASSIGNPILOT', 'ASSIGNPILOT'),
  ('UNASSIGNPILOT', 'UNASSIGNPILOT'),
  ('VEVENT', 'VEVENT')
);

SELECT
  'access_levels' AS check_name,
  COUNT(*) AS actual,
  34 AS expected,
  IF(COUNT(*) = 34, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  (
    tag IN ('VEDIT', 'SHIPLIST', 'VMERCHANT', 'VESSELDEBUG', 'VEHICLE-ADMIN')
    AND min_level = 31
  )
  OR
  (
    tag IN (
      'VESSELS', 'SHIPFIRE', 'SHIPRENOWN', 'SHIPBROWSE', 'SHIPHIRE', 'MARKET',
      'CONTRACTS', 'PLUNDER', 'SEASTATE', 'AUTOPILOT', 'SETWAYPOINT',
      'LISTWAYPOINTS', 'DELWAYPOINT', 'CREATEROUTE', 'ADDTOROUTE',
      'DELROUTE', 'LISTROUTES', 'SETROUTE', 'SETSCHEDULE',
      'CLEARSCHEDULE', 'SHOWSCHEDULE', 'VMOUNT', 'VDISMOUNT', 'DRIVE',
      'VSTATUS', 'VEHICLE-TRANSPORT', 'ASSIGNPILOT', 'UNASSIGNPILOT',
      'VEVENT'
    )
    AND min_level = 0
  );

SELECT
  'nonempty_entries' AS check_name,
  COUNT(*) AS actual,
  34 AS expected,
  IF(COUNT(*) = 34, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN (
  'VESSELS', 'VEDIT', 'SHIPFIRE', 'SHIPRENOWN', 'SHIPBROWSE', 'SHIPHIRE',
  'MARKET', 'CONTRACTS', 'PLUNDER', 'SEASTATE', 'SHIPLIST',
  'VMERCHANT', 'VESSELDEBUG', 'AUTOPILOT', 'SETWAYPOINT', 'LISTWAYPOINTS',
  'DELWAYPOINT', 'CREATEROUTE', 'ADDTOROUTE', 'DELROUTE',
  'LISTROUTES', 'SETROUTE', 'SETSCHEDULE', 'CLEARSCHEDULE',
  'SHOWSCHEDULE', 'VMOUNT', 'VDISMOUNT', 'DRIVE', 'VSTATUS',
  'VEHICLE-TRANSPORT', 'VEHICLE-ADMIN', 'ASSIGNPILOT',
  'UNASSIGNPILOT', 'VEVENT'
)
AND entry IS NOT NULL
AND CHAR_LENGTH(TRIM(entry)) > 0;

/* Guard details that previously drifted away from the command handlers. */
SELECT
  'content_contracts' AS check_name,
  COUNT(*) AS actual,
  44 AS expected,
  IF(COUNT(*) = 44, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'VESSELS' AS tag, 'moving no faster than speed 2' AS required_pattern
  UNION ALL SELECT 'VESSELS', 'elevation or depth'
  UNION ALL SELECT 'VESSELS', 'two-letter ID that SHIPFIRE accepts'
  UNION ALL SELECT 'VESSELS', 'covers 10[[:space:]]+rooms every 45 seconds'
  UNION ALL SELECT 'VESSELS', 'needs speed 6 or less'
  UNION ALL SELECT 'VESSELS', 'berth in 30 seconds or weigh anchor in 13'
  UNION ALL SELECT 'VESSELS', 'contact IDs [(]as CONTACTS shows them[)]'
  UNION ALL SELECT 'VESSELS', 'answers only to her pilot: her passengers ride'
  UNION ALL SELECT 'SHIPFIRE', 'five real[[:space:]]+minutes'
  UNION ALL SELECT 'SHIPFIRE', 'Harbors are neutral ground'
  UNION ALL SELECT 'SHIPFIRE', 'at most [+]7'
  UNION ALL SELECT 'SHIPFIRE', 'within 20 rooms [(]22 with a posted lookout[)]'
  UNION ALL SELECT 'SHIPFIRE', 'fights only with its owner.s consent'
  UNION ALL SELECT 'SHIPBROWSE', 'christen the[[:space:]]+ship again later'
  UNION ALL SELECT 'SHIPBROWSE', 'Aboard a ship you own, rename her'
  UNION ALL SELECT 'SHIPBROWSE', 'same room as you'
  UNION ALL SELECT 'SHIPBROWSE', 'hired crew positions'
  UNION ALL SELECT 'SHIPBROWSE', 'at most three[[:space:]]+hulls'
  UNION ALL SELECT 'SHIPBROWSE', '22 for warships'
  UNION ALL SELECT 'VEDIT', 'new[[:space:]]+prototypes are not for sale'
  UNION ALL SELECT 'SHIPBROWSE', 'need not be[[:space:]]+present'
  UNION ALL SELECT 'SHIPHIRE', 'draws no wages'
  UNION ALL SELECT 'SHIPHIRE', 'one per hull, mounted only on a hull of the[[:space:]]+renown the list shows'
  UNION ALL SELECT 'SHIPHIRE', '540 to 700 renown'
  UNION ALL SELECT 'SHIPRENOWN', '2.5 gold for each point'
  UNION ALL SELECT 'SHIPRENOWN', 'cut by 8 percent'
  UNION ALL SELECT 'MARKET', 'contraband it does not stock'
  UNION ALL SELECT 'MARKET', 'warship four tenths less'
  UNION ALL SELECT 'PLUNDER', 'pays the whole bounty to those who sank her'
  UNION ALL SELECT 'SHIPHIRE', 'green hands earn promotion at sea'
  UNION ALL SELECT 'SHIPFIRE', 'one Craft [(]woodworking[)] check, DC 15'
  UNION ALL SELECT 'SHIPFIRE', 'she needs speed 6 or more'
  UNION ALL SELECT 'SHIPFIRE', 'about once in 17 minutes of sailing'
  UNION ALL SELECT 'VESSELDEBUG', 'vesseldebug raider <0-3> [[]hunter[]]'
  UNION ALL SELECT 'SHIPBROWSE', 'waits in the[[:space:]]+wreck registry'
  UNION ALL SELECT 'SHIPBROWSE', 'never more than 75 minutes'
  UNION ALL SELECT 'SHIPBROWSE', 'whose insurance has paid for her, earns nothing'
  UNION ALL SELECT 'PLUNDER', 'clear your whole bounty for 125%'
  UNION ALL SELECT 'PLUNDER', 'after 21 quiet days'
  UNION ALL SELECT 'SHIPLIST', 'evacuates occupants and loose objects'
  UNION ALL SELECT 'SHIPLIST', 'releases loaded[[:space:]]+vehicles'
  UNION ALL SELECT 'SHIPLIST', 'slots 0 and 1'
  UNION ALL SELECT 'VEHICLE-ADMIN', 'does[[:space:]]+not print its ID'
  UNION ALL SELECT 'boats', 'BOARD her by a word of her name'
) AS expected_content ON BINARY h.tag = expected_content.tag
WHERE h.entry REGEXP expected_content.required_pattern;

SELECT
  'obsolete_duplicates' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE
  (BINARY help_tag = 'board_hostile' AND UPPER(keyword) = 'BOARD_HOSTILE')
  OR (BINARY help_tag = 'disembark' AND UPPER(keyword) = 'DISEMBARK')
  OR (BINARY help_tag = 'dock' AND UPPER(keyword) = 'DOCK')
  OR (BINARY help_tag = 'look_outside' AND UPPER(keyword) = 'LOOK_OUTSIDE')
  OR (BINARY help_tag = 'ship_rooms' AND UPPER(keyword) = 'SHIP_ROOMS')
  OR (BINARY help_tag = 'speed' AND UPPER(keyword) = 'SPEED')
  OR (BINARY help_tag = 'SHIPHIRE' AND UPPER(keyword) = 'SHIPWAGES')
  OR (BINARY help_tag = 'SHIPHIRE' AND UPPER(keyword) = 'SHIPINSURE')
  OR (BINARY help_tag = 'undock' AND UPPER(keyword) = 'UNDOCK')
  OR (BINARY help_tag = 'boats' AND UPPER(keyword) IN ('SHIPS', 'TRANSPORTSS'));

SELECT
  'retired_aliases' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE BINARY tag = 'boats' AND COALESCE(alternate_keywords, '') <> '';
