# Special Procedure Phase 01 Validation

## Purpose

This document is the durable acceptance and test-ownership record for Phase 01: Call-Site Gateway
Compatibility. It maps the phase exit criteria to production source, dedicated production-linked
tests, and reproducible validation commands.

Phase 01 was completed and verified on 2026-08-07. Phase 00 remains the prerequisite control plane;
see [Special Procedure Phase 00 Validation](SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md).

## Delivered Boundary

Phase 01 places one testable seam between every engine call site and the unchanged `SPECIAL`
callback ABI:

- `src/spec/spec_dispatch.h` defines `struct spec_event_context`, gateway-local flow
  (`SPEC_FLOW_CONTINUE` / `SPEC_FLOW_STOP`), and independent invalidation
  (`SPEC_INVALIDATE_NONE|OWNER|ACTOR|TARGET`).
- `src/spec/spec_dispatch.c` implements one gateway per current event plus the shared legacy
  translator `spec_dispatch_legacy()`.
- Every call site listed in the invocation matrix now builds its context at the point where complete
  event data still exists, instead of inferring it later.
- Two extraction-safety corrections ship with the routing (see below).

Phase 01 converts no handlers. Legacy procedures still receive the same `ch`, `me`, `cmd`, and
argument tokens, and each gateway returns exactly what its caller interpreted before. No typed
handler is registered, so the notification-only contract error described in the PRD has no producer
yet: legacy handlers cannot express flow, and their returns on notification-only events are
discarded exactly as before rather than logged.

Phase 01 does not add declarative assignment tables, content extraction, shared mechanics,
typed-handler conversion, or composition.

## Gateway Inventory and Flow Contracts

| Event | Gateway | Caller | Stop Contract |
|-------|---------|--------|---------------|
| Command (room) | `spec_gateway_command_room` | `special()` in `src/interpreter.c` | Consume the command, stop later owner traversal |
| Command (object) | `spec_gateway_command_object` | `special()` | Same |
| Command (mobile) | `spec_gateway_command_mobile` | `special()` | Same |
| Mobile activity | `spec_gateway_mobile_activity` | `mobile_activity()` in `src/mob/mob_act.c` | Skip remaining default activity for this mobile |
| Mobile combat turn | `spec_gateway_mobile_combat_turn` | `perform_violence()` in `src/combat/fight.c` | None; notification only |
| Object auto-pulse | `spec_gateway_object_auto_pulse` | `proc_update()` in `src/comm.c` | Skip the carried-object fallback invocation |
| Item identify | `spec_gateway_item_identify` | item display in `src/obj/act.item.c` | None; notification only |
| Weapon hit | `spec_gateway_weapon_hit` | `weapon_special()` in `src/combat/fight.c` | None; raw legacy return is passed through |
| Defense reaction | `spec_gateway_defense_reaction` | `skill_message()` and total-defense paths in `src/combat/fight.c` | None; notification only |
| Combat maneuver | `spec_gateway_combat_maneuver` | shield maneuvers in `src/combat/act.offensive.c` | None; notification only |
| Mount charge | `spec_gateway_mount_charge` | `perform_charge()` in `src/combat/act.offensive.c` | None; notification only |
| Moving room | `spec_gateway_moving_room` | `moving_rooms_update()` in `src/db.c` | None; notification only |
| Shop secondary | `spec_gateway_shop_secondary` | `shop_keeper()` in `src/obj/shop.c` | Nonzero propagates to the wrapper's caller |
| Quest secondary | `spec_gateway_quest_secondary` | `questmaster()` in `src/quest/quest.c` | Nonzero propagates to the wrapper's caller |

Preserved exactly:

- Command traversal order stays room, equipped objects in wear-slot order, carried objects, mobiles
  in room-list order, then room contents; the first nonzero result still stops later owner
  traversal.
- `mobile_activity()` still owns the `MOB_SPEC`-without-pointer diagnostic and flag removal; the
  gateway receives the already-resolved handler so that behavior is not duplicated or moved.
- `proc_update()` still gates on `ITEM_AUTOPROC` and the zero-value weapon check before the gateway
  runs, and the worn-then-carried fallback ordering is unchanged.
- The moving-room event is the only path that passes a NULL actor and a NULL argument, and it still
  passes `struct moving_room_data *` through the owner slot while selecting the destination room's
  procedure.
- `-s` / `no_specials` handling is untouched. It remains a call-site concern, not a gateway gate.

## Typed Context Coverage

Data the gateways now capture at the call site, which a handler-side wrapper could not reconstruct:

| Payload | Gateways | Source |
|---------|----------|--------|
| Actual target | weapon hit, defense reaction, combat maneuver, mount charge | The caller's own victim/attacker pointer rather than ambient `FIGHTING()` inference at the handler |
| Reaction and maneuver identity | defense reaction, combat maneuver | The existing token, now also carried as typed event identity |
| Moving-room state and destination | moving room | `struct moving_room_data *` and the destination virtual number |
| Actor versus explicit null | object auto-pulse, moving room | `worn_by` / `carried_by`, and the deliberate null relocation actor |

`damage` and `critical` fields exist in the context but stay zero: the current weapon-hit caller
never held those values, and Phase 01 does not invent them. They are populated when a consumer
supplies them.

## Extraction-Safety Corrections

Both corrections are intentional behavior changes, not translation, and each is tested.

1. `special()` in `src/interpreter.c` caches `next_content` before invoking a carried-object or
   room-content callback. Previously a handler that extracted its own object and returned zero left
   the traversal reading that object's successor pointer after removal.
2. `proc_update()` in `src/comm.c` caches `obj->next` before invoking the auto-proc gateway, for the
   same reason on the global object list.

The equipped-object traversal indexes `GET_EQ()` fresh each iteration and needed no change. The
mobile traversal in `special()` also caches `next_in_room`.

## Test Ownership

| Test Source | Tests | Contract Owner |
|-------------|------:|----------------|
| `unittests/CuTest/test_spec_dispatch.c` | 12 | Gateway translation exactness, gateway-local flow, null-safety, auto-pulse fallback, moving-room nulls, secondary forwarding, and both successor-caching corrections. |
| `unittests/CuTest/test_spec_command_pulse.c` | 13 | Unchanged Phase 00 characterization: traversal order, stop rules, `no_specials`, pulse scheduling. Passes without modification. |
| `unittests/CuTest/test_spec_combat_secondary.c` | 14 | Combat and secondary characterization. Four source-shape assertions were updated to the gateway call shape; every runtime assertion is unchanged. |

Phase 01 test coverage:

- `Test_spec_dispatch_legacy_reports_flow_only_for_flow_bearing_events` - STOP is produced only for
  command, mobile activity, and object auto-pulse; notification events keep CONTINUE while still
  reporting the raw legacy return.
- `Test_spec_dispatch_legacy_tolerates_missing_handler_context_and_owner` - null handler, null owner,
  and null context are safe and invoke nothing.
- `Test_spec_dispatch_command_gateways_translate_exactly` - exact actor, owner, command, and argument
  for all three command owners; absent owners and unbound prototypes invoke nothing.
- `Test_spec_dispatch_command_gateways_normalize_nonzero_to_one` - any nonzero legacy return, positive
  or negative, becomes the caller's `1`.
- `Test_spec_dispatch_internal_events_use_exact_legacy_tokens` - `""`, `"identify"`, hit token,
  `"shieldblock"`, `"shieldpunch"`, and `"charge"` reach handlers unchanged with `cmd == 0`.
- `Test_spec_dispatch_weapon_hit_returns_raw_legacy_value` - the weapon-hit gateway passes the raw
  legacy value through, matching `weapon_special()`.
- `Test_spec_dispatch_auto_pulse_runs_carried_fallback_only_after_zero` - worn-then-carried fallback
  with null actor and both return variations.
- `Test_spec_dispatch_moving_room_preserves_null_actor_and_argument` - NULL actor and NULL argument
  survive; an unbound or absent room invokes nothing.
- `Test_spec_dispatch_secondary_gateways_forward_context_unchanged` - shop and quest secondaries see
  the caller's own context, nonzero propagates, and a NULL secondary is safe.
- `Test_spec_command_traversal_caches_successor_before_callback` - a handler that clears its own
  successor pointers no longer truncates inventory or room-content traversal.
- `Test_spec_proc_update_caches_successor_before_callback` - the same correction on the global object
  list.
- `Test_spec_invalidate_name_reports_stable_labels` - stable diagnostic labels for single
  invalidation bits; combined or empty masks report `unknown`.

## Reproducible Validation

```bash
make clean
make -j$(nproc)            # zero new -Wall -Wextra warnings
make test                  # 563 tests, all passing
make install               # installs bin/luminari, leaves no root-level luminari
```

Build parity: `src/spec/spec_dispatch.c` and `unittests/CuTest/test_spec_dispatch.c` are listed in
both `Makefile.am` and `CMakeLists.txt`.

## Acceptance Rule

Phase 01 is accepted when characterized non-extraction behavior is unchanged, complete event data
reaches one testable seam at every caller, and each intentional extraction-safety correction is
tested. All three held on 2026-08-07.
