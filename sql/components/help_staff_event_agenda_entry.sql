-- Native staff-event timing and restart policy.
START TRANSACTION;

INSERT INTO help_entries (tag,entry,min_level,auto_generated) VALUES ('STAFF-EVENT', 'Staff-run events are world-wide quests combining automation and staff participation.

Usage: staffevent
       staffevent info <index>
       staffevent start <index>
       staffevent end <index>

Use staffevent without an argument to list the available events and their status.
Starting and ending events requires staff access. An event must finish its
cleanup delay before another event can start. An invalid or unavailable event
is rejected before it is announced or its initial population is spawned.

Event durations and cleanup delays use mud hours (75 real seconds), aligned to
the shared world hour. The status display reports the actual time remaining.
An active event ends on expiry, or staff can end it early using its index.
Ending a different index does not end the current event.

Active events are not restored after a reboot or copyover. Staff can start a
new event after the initial three-mud-hour startup delay. Player participation
and rewards already recorded by their existing systems are unchanged.',0,FALSE)
ON DUPLICATE KEY UPDATE entry=VALUES(entry),min_level=VALUES(min_level),auto_generated=VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag,keyword) VALUES ('STAFF-EVENT', 'STAFF-EVENT');
INSERT IGNORE INTO help_keywords (help_tag,keyword) VALUES ('STAFF-EVENT', 'STAFFEVENT');
INSERT IGNORE INTO help_keywords (help_tag,keyword) VALUES ('STAFF-EVENT', 'STAFF_EVENT');

COMMIT;
