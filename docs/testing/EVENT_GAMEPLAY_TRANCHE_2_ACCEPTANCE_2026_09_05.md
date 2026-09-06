# Casting activity tranche 2 acceptance - 2026-09-05

Branch: `refactor/fight-combat-safety`

Scope and migration rationale:
[casting activity contracts](../systems/MUD_EVENTS.md#casting-activities).

## Results

| Check | Result |
| --- | --- |
| Autotools `make -j8 test` | PASS: 1,108 production-linked CuTests, including 13 new tests, plus architecture and tooling targets |
| `make install` | PASS; installed server and no root-level `luminari` artifact |
| Compiler diagnostics | No compiler warnings or errors in the final Autotools build |
| CMake Debug, BUILD_TESTS=ON | PASS: all 19 CTest targets |
| CMake production suite with LUMINARI_IO_DRIVER=select | PASS |
| Valgrind, full suite with child tracing | PASS: 33 process logs, zero reported errors and zero definite/indirect leaks; no suppressions |
| Retired event API guard | PASS; rejects the old casting ID and handler as well as retired scheduler APIs |
| Help | ACTIVITY, CASTING-TIME and CONCENTRATION updated in the flat file and local development database; previous ACTIVITY content retained in help_versions |
| Full-world boot smoke | PASS: isolated development runtime reached the game loop; both fixture administrators logged in and event diagnostics responded |
| Runtime cleanup | Isolated MUD, supervisor and watchdog stopped after testing |

The final installed and smoke-tested executable has ELF build ID
`9517dcb0814ca96d722afc0084bb092a03185649` and SHA-256
`4d0d7136ef4687bc6af236d4bf553e9880fe6bd679d55892c64827e63f752bf8`.

## Behavior covered

- Production NPC casting completes exactly once on its existing pulse clock,
  even after combat begins. Semantic turns do not advance the cast early.
- Production player casting consumes the prepared spell once. A rejected
  concurrent cast consumes no second spell; interruption does not refund one.
- Positive damage can interrupt outside combat. Successful concentration keeps
  the same activity ID and deadline. Zero damage requests no check.
- Damage committed at the final deadline prevents later spell resolution when
  concentration fails, including an INT_MAX damage case.
- Alchemist and shadowdancer concentration exemptions remain.
- Movement, target movement/death/extraction, actor extraction, disabling
  conditions and manager shutdown clear casting state and pending work.
- Reset followed by immediate recasting receives a new ID; stale work cannot
  complete the replacement cast.
- Object disposal publishes extraction before releasing target memory.
- Damage callbacks receive amount/type once, and progress does not reroll them.
- Raw damage publishes actual loss after a nonlethal floor. Healing and zero
  loss publish nothing; DG script damage reaches the same activity consumer.

## Evidence and limits

Local logs (not committed): `/tmp/tranche2-final-make-test.log`,
`/tmp/tranche2-final-install.log`, `/tmp/tranche2-ctest.log`,
`/tmp/tranche2-ctest-select.log`, `/tmp/tranche2-valgrind-tests.log`,
`/tmp/tranche2-valgrind.*.log`, `/tmp/tranche2-live-autorun.log`,
`/tmp/tranche2-live-probe.log`, and `/tmp/tranche2-live-stop.log`.

The live smoke used the existing isolated `.ci-runtime/acceptance-20260905`
namespace on port 4103 and its separate database. It did not restart the normal
local development server. The optional terrain API could not bind its already
occupied port; the MUD entered its game loop normally. Casting behavior was
verified through production-linked tests, not a manual campaign balance pass.

No new latency SLA or gameplay-balance claim is made. This is not a deployment
or a migration of every remaining gameplay countdown. Those are recorded in
`docs/systems/EVENT_MECHANISM_INVENTORY.md`.
