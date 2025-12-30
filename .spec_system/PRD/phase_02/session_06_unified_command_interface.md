# Session 06: Unified Command Interface

**Session ID**: `phase02-session06-unified-command-interface`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Create an abstraction layer that provides consistent commands across all transport types (vehicles and vessels), allowing players to use common commands regardless of transport type.

---

## Scope

### In Scope (MVP)
- Transport abstraction structure (transport_data)
- Unified `enter` command (detects vehicle vs vessel)
- Unified `exit` command (dismount/disembark)
- Unified `go <direction>` command (drive/sail)
- Unified `transportstatus` command
- Transport type detection helpers
- Backward compatibility with existing commands

### Out of Scope
- Unified autopilot commands (future)
- Unified combat interface
- Transport switching while moving
- GUI/client protocol extensions

---

## Prerequisites

- [ ] Session 05 complete (vehicle-in-vehicle)
- [ ] All vehicle commands functional
- [ ] All vessel commands functional

---

## Deliverables

1. Transport abstraction structure definition
2. get_transport_type() helper function
3. do_enter() unified command handler
4. do_exit_transport() unified command handler
5. do_transport_go() unified movement command
6. do_transportstatus() unified status command
7. Documentation for unified interface

---

## Success Criteria

- [ ] Single `enter` command works for vehicles and vessels
- [ ] Single `exit` command works for all transport types
- [ ] Movement command abstracts drive/sail differences
- [ ] Status command shows appropriate info per type
- [ ] Existing commands still work (backward compatible)
- [ ] Clean abstraction with no type-specific conditionals in commands
- [ ] Well-documented interface for future extensions
