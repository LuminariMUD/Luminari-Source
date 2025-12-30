# Session 05: NPC Pilot Integration

**Session ID**: `phase01-session05-npc-pilot-integration`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Enable NPC characters to serve as vessel pilots, operating vessels along assigned routes with appropriate behavior and responses.

---

## Scope

### In Scope (MVP)
- NPC pilot assignment data structure
- `assignpilot <npc>` command for assigning NPC to vessel
- `unassignpilot` command to remove pilot
- NPC pilot takes over autopilot control when assigned
- Basic pilot behavior (announce arrivals, status reports)
- Pilot remains with vessel during travel
- Database persistence for pilot assignments
- Pilot presence check for autopilot operations

### Out of Scope
- Complex NPC AI (combat, evasion)
- NPC crew beyond pilot role
- Pilot training/skill system
- Dynamic NPC hiring/wages

---

## Prerequisites

- [ ] Sessions 01-04 complete
- [ ] Understanding of NPC (mob) data structures
- [ ] Understanding of ship_crew_roster table

---

## Deliverables

1. NPC pilot data structure and association
2. `do_assignpilot()` command handler
3. `do_unassignpilot()` command handler
4. Pilot behavior functions (announcements)
5. Database persistence for pilot assignments
6. Integration with autopilot tick (pilot controls vessel)
7. Commands registered in interpreter.c
8. Help file entries

---

## Success Criteria

- [ ] NPCs can be assigned as vessel pilots
- [ ] Pilot assignment persists across reboots
- [ ] Piloted vessels operate autopilot automatically
- [ ] Pilot announces waypoint arrivals
- [ ] Pilot can be unassigned by authorized crew
- [ ] Invalid NPC assignments rejected with message
- [ ] Pilot stays with vessel during travel
- [ ] Unit tests verify pilot assignment logic
