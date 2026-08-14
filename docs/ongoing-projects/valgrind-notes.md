# Valgrind Live Session Notes - 2026-08-14

## Outcome

The local development game was stopped, exercised under Valgrind Memcheck in full-world and
mini-world modes, and stopped through its normal SIGTERM cleanup path. The successful mini-world
run logged Kohdee in, requested the regular and privileged command listings, restored Kohdee's
display settings, logged out cleanly, and remained live for 8 minutes 10 seconds after logout. The
planned 10-minute live observation was shortened at the user's request.

At the end of the session:

- Both Valgrind systemd user units were `inactive/dead`, with result `success` and exit status 0.
- No `circle`, Valgrind, or autorun process remained.
- No listener remained on ports 4100, 4101, 8181, or 8182.
- The normal autorun process remained stopped and did not restart the local server.
- The temporary mini-world config and homeland-quest index were gone. The real config and world
  files were unchanged.
- The only Git worktree change was this notes file.

This was the local development environment. No remote production host was accessed or changed.

## Reproducibility

The tested executable reported:

```text
Version:      LuminariMUD 2.5060-beta (tbaMUD 3.64)
Git commit:   05c5fea3f30d734020c1fc974e67a2f9c845a51a
Git dirty:    0
ELF build ID: 45b2a1ae1ce61926aca895322974e0e77c12d0fd
SHA-256:      1c71cbf2447f83f1c1a594b97270fb5987ed3c4a8ac086c01ac9a2e7938cd84b
```

`bin/circle` resolved to the release directory named by that ELF build ID. The checkout had moved
to commit `34bd28dda374f79ada3f2b5446ed77be9d69233e` by report time, so reproduce these results with
the tested executable identity above rather than assuming the report covers the later checkout.

Valgrind 3.22.0 was run with:

```text
--tool=memcheck
--leak-check=full
--show-leak-kinds=all
--errors-for-leak-kinds=all
--track-origins=yes
--track-fds=all
--trace-children=yes
--num-callers=40
--error-limit=no
--time-stamp=yes
```

## Run chronology and coverage

### Full-world run

- Run ID: `20260814T093421Z`
- Unit: `luminari-dev-valgrind-20260814T093421Z.service`
- Game arguments: `-q -o <game.log> -d lib 4100`
- Boot began at 12:34:21 IDT and reached the game loop at 12:43:29, a 9-minute 8-second
  Valgrind boot.
- The game loop ran for 13 minutes 6 seconds before SIGTERM at 12:56:35.
- Vessel saving completed, normal termination was logged at 12:57:43, and cleanup finished at
  12:57:46. Total Memcheck elapsed time was 23 minutes 25 seconds.

The full world was too slow for reliable interactive coverage under Memcheck. It reported five
heartbeat stalls of 74, 122, 124, 121, and 127 seconds. Kohdee connected, but the client could not
complete a usable command session before timeouts. The full-world evidence is still the strongest
source for normal world activity, shutdown ordering, full-world leaks, and data diagnostics.

### Mini-world bootstrap diagnostics

Two failed starts exposed stale mini-mode assumptions:

- Run `20260814T095821Z-mini` stopped because configured mortal start room 14100 is not in the
  mini room set.
- Run `20260814T100030Z-mini` used an ephemeral full-config override with mortal start room 3001,
  then stopped because `world/hlq/index.mini` does not exist.

The successful run used the same ephemeral start-room override, an ephemeral empty homeland-quest
mini index, and `-s` to suppress special-procedure assignment. Both temporary paths were consumed
and removed without modifying `lib/etc/config` or tracked world data. Missing mini-only object,
zone, trigger, and artifact prototypes in these logs are expected consequences of the reduced
indexes and are not counted as full-world defects below.

### Successful Kohdee command run

- Run ID: `20260814T100137Z-mini`
- Unit: `luminari-dev-valgrind-20260814T100137Z-mini.service`
- Game arguments: `-m -q -s -f <ephemeral-config> -o <game.log> -d lib 4100`
- Game loop entered at 13:01:44 IDT.
- Kohdee first entered at 13:02:20. That client became caught in the original 40-line pager and
  its descriptor was closed at 13:04:58; the server stayed up.
- Kohdee reconnected at 13:05:20. The client sent the following sequence:

```text
toggle pagelength 255
toggle screenwidth 200
commands
wizhelp
toggle screenwidth 80
toggle pagelength 40
```

Kohdee quit cleanly at 13:05:22 with the original 80-column, 40-line settings restored. The saved
transcript contains 120 regular command tokens from `bite` through `zlist` and 191 privileged
command tokens. The regular transcript begins at `bite`, so its leading edge was lost by the client
capture even though the `commands` request itself is present. This exercise requested the
authoritative command-listing surfaces; it did not execute every listed command, many of which are
destructive or require arguments.

The post-logout live observation lasted 490 seconds, from 13:05:22 through SIGTERM at 13:13:32.
RSS was 297,188 KiB at every recorded live sample from 97 through 482 seconds, and the mini run
produced no invalid-read, invalid-write, uninitialized-value, or conditional-jump diagnostic.
Signal handling, vessel and vehicle saves, service-thread shutdown, socket closure, and Memcheck's
final report all completed during the same second. Total Memcheck elapsed time was 11 minutes
55 seconds.

## Memory-safety findings

### 1. Runtime use-after-free in `mag_unaffects`

Severity: high. The full-world run produced three diagnostics at Memcheck elapsed time 17:16:

```text
Invalid read size 8 - src/magic/magic.c:13626
Invalid read size 4 - src/magic/magic.c:13628
Invalid read size 8 - src/magic/magic.c:13626
```

The active stack was:

```text
mag_unaffects (magic.c:13626/13628)
mag_masses (magic.c:10959)
call_magic (spell_parser.c:989)
manifest_power (spell_parser.c:2975)
mobile_activity (mob_act.c:169)
heartbeat (comm.c:1631)
game_loop (comm.c:1422)
```

`mag_unaffects` iterates with `af = af->next`, but its body calls `affect_from_char` at line 13632.
That call reaches `affect_remove` and frees the current 72-byte affect allocation. The loop then
reads the freed affect's fields or `next` pointer. The allocation originated in
`affect_to_char_source` at `handler.c:1246`, reached through `mag_affects_full`.

Follow-up should make this a removal-safe traversal and account for the fact that
`affect_from_char` can remove more than the one current node when multiple affects share a spell.

### 2. Shutdown use-after-free in object event cleanup

Severity: high. Graceful full-world shutdown exposed three more access diagnostics at elapsed time
23:25:

```text
Invalid read size 8  - free_mud_event, mud_event.c:715
Invalid read size 8  - free_mud_event, mud_event.c:717
Invalid write size 8 - free_mud_event, mud_event.c:720
```

The event cleanup stack was:

```text
free_mud_event (mud_event.c:715/717/720)
cleanup_event_obj (dg_event.c:245)
queue_free (dg_event.c:723)
event_free_all (dg_event.c:409)
destroy_db (db.c:1223)
```

`destroy_db` frees active objects at `db.c:973-979`, then calls `event_free_all` at line 1223.
An object event still held `pMudEvent->pStruct` pointing into an already-freed 696-byte `obj_data`
allocation. `free_mud_event` dereferenced `obj->events` and then wrote `obj->events = NULL`.

Follow-up should cancel/detach object events before object destruction, or move global event cleanup
ahead of freeing the object list while preserving the ownership rules for character and room
events.

## Leak and heap results

Because `--errors-for-leak-kinds=all` was enabled, Memcheck's large `ERROR SUMMARY` values include
one context per reported loss record. They are not counts of invalid memory accesses. The full run
had exactly six non-leak access contexts, described above; the mini run had none.

| Metric | Full world | Successful mini world |
|---|---:|---:|
| Heap allocations | 2,230,556 | 64,008 |
| Heap frees | 2,202,911 | 46,627 |
| Cumulative bytes allocated | 2,337,044,666 | 55,247,706 |
| In use at exit | 3,931,086 bytes / 27,645 blocks | 3,471,559 bytes / 17,381 blocks |
| Definitely lost | 328,720 bytes / 6,492 blocks | 33,327 bytes / 1,712 blocks |
| Indirectly lost | 4,772 bytes / 158 blocks | 0 |
| Possibly lost | 0 | 0 |
| Still reachable | 3,597,594 bytes / 20,995 blocks | 3,438,232 bytes / 15,669 blocks |
| Error summary | 7,552 errors / 7,515 contexts | 7,369 errors / 7,369 contexts |

The full run's definite plus indirect loss was 333,492 bytes. Its largest records were:

- 232,079 bytes in 2,265 blocks from `fread_string` -> `parse_room` at `db.c:2179`
  (room descriptions).
- 60,245 bytes in 2,265 blocks from `fread_string` -> `parse_room` at `db.c:2178`
  (room names).
- 17,952 bytes in 561 blocks and 5,610 bytes in 561 blocks from `init_perks` at
  `character/perks.c:103` and `character/perks.c:102`.
- 2,944 direct plus 4,434 indirect bytes in 23 blocks from `create_trap` ->
  `create_rol_exit_trap` -> `parse_room`.
- 4,512 direct plus 338 indirect bytes in 94 blocks from `read_object` during `House_boot`.

The two room-string records account for 88.93 percent of all definite bytes lost in the full run.
The two perk records account for another 7.17 percent. Trace those ownership paths first.

The successful mini run repeated the same 23,562-byte `init_perks` loss and additionally lost
7,344 bytes in four `route_create` allocations loaded by the vessel database. Together those
sources account for 92.74 percent of the mini run's definite loss.

Large still-reachable records include help-system strings loaded by `load_help`, multiple
131,072-byte performance-profiler buffers allocated by `PERF_prof_sect_exit`, and MariaDB client
allocations. These are lower priority than the definite/indirect records but show that global
shutdown cleanup is incomplete.

## Open descriptors at exit

After excluding standard streams and the inherited Memcheck log descriptor:

- Both completed runs retained three AF_UNIX sockets opened by the three-connection MariaDB pool.
- The successful mini run also retained `world/qst/index.mini` and the ephemeral
  `world/hlq/index.mini` stream opened by `index_boot` at `db.c:1807`.

The full run therefore reported seven total descriptors open at exit (three standard), while the
mini run reported nine (three standard). The MariaDB pool should be explicitly closed during
normal shutdown. The mini index streams also need a balanced `fclose` path.

## World and runtime diagnostics

The full-world game log contained these recurring or actionable diagnostics:

- `mud_event_index` has 229 entries while the enum count is 228. This appeared once in every
  attempted run and warns that events may be misaligned.
- 3,602 failed object-UID lookups covering 160 unique IDs. UID 3108601 alone accounted for 2,491
  messages, so this is concentrated rather than uniformly random persistence noise.
- 510 reports that typed special procedure `RoL Command Sentinel` does not support mobile activity
  for mobile owners.
- Mob 103667 exceeded 20 echo entries four times; mob 135499 did so nine times.
- Six `daily-use-cooldown-event: 97` reports had a NULL `sVariables` field.
- Zone 20199 command 1 has an invalid backward `if_flag` reference and logged twice. Additional
  zone errors referenced missing vnum 15802 and missing equipment object 120602.

The local Ollama model was absent, and the local Intermud3 gateway at 127.0.0.1:8081 refused
connections. Those produced one Ollama warmup error per completed run and 32 full-world / 48
mini-world I3 error lines. They are local dependency noise, not Memcheck findings.

## Evidence paths

Full-world evidence:

- `log/valgrind/live-20260814T093421Z/memcheck.log`
- `log/valgrind/live-20260814T093421Z/game.log`
- `log/valgrind/live-20260814T093421Z/runtime.log`

Successful Kohdee mini-world evidence:

- `log/valgrind/live-20260814T100137Z-mini/memcheck.log`
- `log/valgrind/live-20260814T100137Z-mini/game.log`
- `log/valgrind/live-20260814T100137Z-mini/runtime.log`
- `log/valgrind/live-20260814T100137Z-mini/kohdee-command-listings.log`

Mini bootstrap evidence:

- `log/valgrind/live-20260814T095821Z-mini/`
- `log/valgrind/live-20260814T100030Z-mini/`
