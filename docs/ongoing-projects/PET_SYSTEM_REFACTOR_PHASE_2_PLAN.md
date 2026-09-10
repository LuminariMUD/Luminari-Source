# Pet System Refactor - Phase 2

See [Phase 1](PET_SYSTEM_REFACTOR_PLAN.md) for remaining gameplay parity work.

## Phase 2 - All other remaining work

- [ ] Preserve finite natural and control deadlines across storage, logout, restart, and copyover:
  - Store absolute deadlines so offline time cannot refresh a finite pet.
  - Leave intentionally permanent pets permanent.
  - Handle legacy rows without inventing a finite deadline that was never recorded.

- [x] Finish bounded restore and admission correctness (issue 118):
  - Decode and validate all of an owner's saved pets before exposing any of them to gameplay.
  - Select a deterministic allowed set when current capacity is lower than the saved roster, with
    explicit player and pet priority.
  - Keep rejected or malformed rows saved for recovery, publish no partial inventory, and ensure a
    retry never duplicates a pet already restored by stable ID.

- [ ] Close callback and transaction edge cases:
  - Define and test equipment and mobile callback behavior while a pet is prepared outside a room.
  - Reconcile extraction or mutation during publication without losing the saved pet or its items.
  - Reconcile uncertain commit outcomes for active saves, keeper storage, and keeper reclaim.
  - Document the remaining boundary between SQL pet state, pfiles, and live world objects instead of
    claiming cross-store crash atomicity.

- [ ] Resolve legacy persistence compatibility:
  - Decide on a bounded, reviewable repair for pet container rows whose custom weights were already
    inflated before container-weight normalization was fixed.
  - Verify stable identity, owner binding, rename and name-reuse isolation, nested object ordering,
    equipment association, and schema upgrades against existing saved data.
  - Remove a superseded pet path only after tracing that it has no remaining callers.

- [ ] Extend the existing production-linked tests for each Phase 1 slice and Phase 2 boundary:
  - Reuse `test_pet_policy.c`, `test_database_persistence.c`, `test_gameplay_e2e.c`,
    and the relevant spell, class, crafting, combat, mount, and object suites.
  - Cover success, denial, resource retention or consumption, expiry, death, separation, malformed
    data, retry, duplicate prevention, callback extraction, and rollback.
  - Use isolated MariaDB for database bodies; do not add a parallel pet test harness.

- [ ] Run final integration acceptance after the remaining implementation is complete:
  - Exercise summoner, necromancer, divine, mount, construct, purchased-pet, and
    source-item gameplay in the development MUD through `autorun.sh`.
  - Include equipment and nested containers, owner death and reclaim, logout and relogin, restart,
    actual copyover, finite expiry, reduced-capacity restore, and failed-operation recovery.
  - Run `LUMINARI_TEST_MYSQL_ENABLE=1 make -j$(nproc) test`, `make install`, the
    supported CMake build, and focused memory checks for changed parsing, lifetime,
    object, and callback paths.

- [ ] Finish documentation and delivery:
  - Update command and feature help in both `lib/text/help/help.hlp` and the
    development help database, with matching SQL components where tracked publication
    is required.
  - Verify every changed topic and alias in game.
  - Update the relevant system documentation to describe final behavior and known durability limits.
  - Record evidence for every remaining gameplay capability and non-gameplay boundary
    before claiming parity or closing related tracking issues.
