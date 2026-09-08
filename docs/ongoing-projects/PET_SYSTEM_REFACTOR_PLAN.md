# Pet System Refactor - Remaining Work

## Phase 1 - Gameplay parity

- [ ] Reconcile the remaining player-facing capabilities against the fixed foreign references:
  - RealmsOfLuminari commit `4f945b0e0c0485178d1dbf531d82db91ef1060be`: pet keeper,
    claiming, orders, elementalists, necromancy, and mounts.
  - Duris commit `c27968e8ffa34f7baa38e0f27d653debb7b8cad4`: undead forms and costs,
    raising, hosts, constructs, illusions, and pet combat feedback.
  - Count a native Luminari equivalent only when the capability is usable in game; matching names,
    disabled sketches, foreign internals, and exact foreign balance numbers do not establish parity.

- [ ] Complete and prove the existing bonded and class-linked roles:
  - Animal companion, familiar, paladin mount, dragon mount, shadow, eidolon, cohort, mercenary,
    epic summon, and psionic construct acquisition must remain accessible to the intended classes.
  - Preserve familiar bonuses, Boon Companion, ranger bonuses, eidolon evolutions, Deathless Touch,
    class progression, and respec behavior.
  - Progression and respec must not reroll statistics, refill resources, or stack bonuses.

- [ ] Deliver distinct elemental, planar, nature, genie, plant, and support summon choices:
  - Supply any missing world prototypes and connect them through existing spells and class access.
  - Give each choice a meaningful native role rather than a cosmetic label.
  - Preserve intentional authored batches, including eight-elemental and six-shambler casts, while
    preventing repeated casts from exceeding control limits.
  - Define acquisition cost, control cost, equipment rules, duration, cooldown, and dismissal for
    every added choice.

- [ ] Complete undead and divine pet choices:
  - Add lesser and elite martial, caster, incorporeal, support, divine, and celestial equivalents.
  - Apply explicit bounded control costs so elite choices consume more capacity than lesser ones.
  - Enforce corpse eligibility and single use; distinguish physical equipment from incorporeal
    restrictions and preserve native undead defenses and abilities.
  - Keep innate animation distinct from corpse-requiring spells.

- [ ] Add bounded opt-in raising and host gameplay:
  - Offer raising only at the native eligible-kill or corpse boundary and require player opt-in.
  - Exclude player corpses, already-used corpses, summoned targets, and summon-farming loops.
  - Reuse ordinary undead admission and costs; a full roster must fail without creating a pet.
  - Provide the bounded host ability with real acquisition, capacity, lifetime, and dismissal rules.

- [ ] Complete construct gameplay:
  - Add materially distinct material-, recipe-, and corpse-based construct variants through native
    crafting and animation paths.
  - Preserve repair and dismantling behavior and prevent repeated dismantling rewards.
  - Missing prototypes or failed admission must retain materials; authored failed crafting or
    ritual rolls must keep their intended resource cost.

- [ ] Complete ownership, control-break, illusion, and combat-feedback behavior:
  - Distinguish ownership, following, orderability, control break, hostility, natural expiry, and
    dismissal for every remaining role.
  - Preserve bonded loyalty and Call Lycanthrope's risky control behavior without introducing random
    betrayal for ordinary companions.
  - Keep illusions and decoys owned but non-orderable, attribute their damage correctly, and add
    optional owner-visible pet combat feedback through existing preferences.
  - Preserve PvP protections and ensure separation, transfer, expiry, and death produce the intended
    ownership and equipment outcome.

- [ ] Complete mount parity:
  - Enforce rider and mount size, anatomy, ability, and control restrictions.
  - Prove mounted travel, mounted combat, dismounting, recall, rider cleanup, and pet extraction.
  - Riding a creature must not grant ownership or command rights.

- [ ] Audit and repair every remaining gameplay acquisition boundary:
  - Cover spells, feats, class powers, artifacts, totems, corpses, purchases, crafting, and quests.
  - Check capacity before spending payment, materials, corpses, charges, or cooldowns unless an
    authored failed attempt intentionally consumes them.
  - Repair the legacy Xvim acquisition path so it validates the prototype and does not assign the
    owner's hit points incorrectly.
  - Prove each route in game with its intended class, source item or corpse, denial cases, commands,
    equipment behavior, lifecycle, and recovery after separation.

## Phase 2 - All other remaining work

- [ ] Preserve finite natural and control deadlines across storage, logout, restart, and copyover:
  - Store absolute deadlines so offline time cannot refresh a finite pet.
  - Leave intentionally permanent pets permanent.
  - Handle legacy rows without inventing a finite deadline that was never recorded.

- [ ] Finish bounded restore and admission correctness:
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
