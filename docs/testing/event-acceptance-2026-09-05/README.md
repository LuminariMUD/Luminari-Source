# Native-Only Acceptance Evidence

These command transcripts were captured in a private local full-world instance
on 2026-09-05, using an agent character. They contain no login credentials,
database configuration, or copied player files. The ordinary development
server and production were not changed.

- `native-copyover-cooldown.txt`: Treat Injury remained scheduled across real
  copyover, with elapsed time deducted; the transient readied action cleared.
- `native-offscreen-war.txt`: a GLOBAL random script started an NPC duel after
  the observer left the zone. Both participants lost health before return.
- `native-vessel-builder-enabled.txt`: a temporary vessel was created, tuned,
  spawned, sailed west, and purged; its temporary prototype was removed.
- `native-vessel-autopilot.txt`: an owned vessel event completed a waypoint
  and rescheduled; purging the hull removed its event. The waypoint was within
  the existing arrival tolerance, so this is not evidence of autopilot travel
  over distance. Manual sailing is evidenced by the builder check.
- `native-clean-idle-events.txt`, `native-clean-idle-after.txt`, and
  `native-clean-idle-process.txt`: before/after scheduler snapshots and a
  two-minute process sample, without builds or gameplay commands during the
  measured interval. The first helper rejected a wait exceeding its 60-second
  limit and disconnected; the process sampler completed independently, and
  the after snapshot came from a separate login. This is not a successful
  continuous-session test or an exactly two-minute callback-count interval.

Vessel event integration is in scope. This evidence does not declare the
broader vessel feature set finished, validate all vessel gameplay, or certify
large-fleet scale. See the parent acceptance report for coverage and limits.
