-- Read-only verification for help_vessel_entries.sql.
--
-- The command-keyword list mirrors the vessel, vehicle, unified transport,
-- autopilot, and staff recovery registrations in src/interpreter.c.

SELECT
  'entry_count' AS check_name,
  COUNT(*) AS actual,
  33 AS expected,
  IF(COUNT(*) = 33, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN (
  'VESSELS', 'VEDIT', 'SHIPFIRE', 'SHIPBROWSE', 'SHIPHIRE',
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
  81 AS expected,
  IF(COUNT(*) = 81, 'PASS', 'FAIL') AS result
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
  ('VESSELS', 'LOOK_OUTSIDE'),
  ('VESSELS', 'LOOKOUT'),
  ('VESSELS', 'SHIP_ROOMS'),
  ('VESSELS', 'BOARD_HOSTILE'),
  ('VEDIT', 'VEDIT'),
  ('SHIPFIRE', 'SHIPFIRE'),
  ('SHIPFIRE', 'SHIPREPAIR'),
  ('SHIPFIRE', 'CLAIMSHIP'),
  ('SHIPBROWSE', 'SHIPBROWSE'),
  ('SHIPBROWSE', 'SHIPBUY'),
  ('SHIPBROWSE', 'SHIPCHRISTEN'),
  ('SHIPBROWSE', 'SHIPCUSTOMIZE'),
  ('SHIPBROWSE', 'SHIPDEED'),
  ('SHIPBROWSE', 'SHIPPERMIT'),
  ('SHIPBROWSE', 'SHIPREVOKE'),
  ('SHIPBROWSE', 'SHIPCREW'),
  ('SHIPHIRE', 'SHIPHIRE'),
  ('SHIPHIRE', 'SHIPDISMISS'),
  ('SHIPHIRE', 'SHIPWAGES'),
  ('SHIPHIRE', 'SHIPUPGRADE'),
  ('SHIPHIRE', 'SHIPINSURE'),
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
  33 AS expected,
  IF(COUNT(*) = 33, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  (
    tag IN ('VEDIT', 'SHIPLIST', 'VMERCHANT', 'VESSELDEBUG', 'VEHICLE-ADMIN')
    AND min_level = 31
  )
  OR
  (
    tag IN (
      'VESSELS', 'SHIPFIRE', 'SHIPBROWSE', 'SHIPHIRE', 'MARKET',
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
  33 AS expected,
  IF(COUNT(*) = 33, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN (
  'VESSELS', 'VEDIT', 'SHIPFIRE', 'SHIPBROWSE', 'SHIPHIRE',
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
  12 AS expected,
  IF(COUNT(*) = 12, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'VESSELS' AS tag, 'moving no faster than speed 2' AS required_pattern
  UNION ALL SELECT 'VESSELS', 'elevation or depth'
  UNION ALL SELECT 'SHIPFIRE', 'five real[[:space:]]+minutes'
  UNION ALL SELECT 'SHIPBROWSE', 'christen the[[:space:]]+ship again later'
  UNION ALL SELECT 'SHIPBROWSE', 'same room as you'
  UNION ALL SELECT 'SHIPBROWSE', 'hired crew positions'
  UNION ALL SELECT 'SHIPBROWSE', 'need not be[[:space:]]+present'
  UNION ALL SELECT 'SHIPHIRE', 'SHIPDISMISS and[[:space:]]+SHIPWAGES'
  UNION ALL SELECT 'SHIPLIST', 'evacuates occupants and loose objects'
  UNION ALL SELECT 'SHIPLIST', 'releases loaded[[:space:]]+vehicles'
  UNION ALL SELECT 'SHIPLIST', 'slots 0 and 1'
  UNION ALL SELECT 'VEHICLE-ADMIN', 'does[[:space:]]+not print its ID'
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
  OR (BINARY help_tag = 'undock' AND UPPER(keyword) = 'UNDOCK');
