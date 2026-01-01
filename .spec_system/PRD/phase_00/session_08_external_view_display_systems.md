# Session 08: External View & Display Systems

**Session ID**: `phase00-session08-external-view-display-systems`
**Status**: Not Started
**Estimated Tasks**: ~20-25
**Estimated Duration**: 3-4 hours

---

## Objective

Complete the external view and display systems: integrate look_outside with weather/wilderness view, implement tactical display rendering, implement contact detection, and complete disembark mechanics.

---

## Scope

### In Scope (MVP)
- Implement look_outside with get_weather() integration
- Render wilderness view from ship interior
- Implement tactical display for shipstatus
- Implement contact detection (nearby vessels)
- Implement contacts command display
- Complete disembark mechanics

### Out of Scope
- Combat display systems
- Advanced radar/sonar features
- Weather effects on visibility (beyond basic)

---

## Prerequisites

- [ ] Sessions 01-07 completed
- [ ] Interior rooms working
- [ ] Movement working
- [ ] Weather system accessible (get_weather())

---

## Deliverables

1. look_outside shows exterior with weather
2. tactical command shows formatted tactical display
3. contacts command shows nearby vessels
4. disembark command fully functional

---

## Success Criteria

- [ ] look_outside shows wilderness description from interior
- [ ] look_outside includes current weather conditions
- [ ] tactical command renders ASCII tactical map
- [ ] contacts command lists vessels within detection range
- [ ] Contact info includes distance and bearing
- [ ] disembark properly exits vessel to dock/water
- [ ] All displays render correctly in terminal

---

## Technical Notes

### look_outside Current State

From PRD:
> do_look_outside() prints placeholder; does not call get_weather() or render wilderness view.

Needs to:
1. Get current ship coordinates
2. Get weather for those coordinates
3. Render wilderness terrain description
4. Show other vessels in view

### Tactical Display

From PRD Section 5:
> tactical - Display tactical map (not implemented)

Should show:
- Ship position on grid
- Nearby terrain
- Other vessels
- Compass directions

### Contact Detection

From PRD Section 5:
> contacts - Show nearby vessels (not implemented)

Should:
- Scan for vessels within range
- Calculate distance and bearing
- Filter by detection capability
- Display formatted list

### Disembark

Currently placeholder. Needs to:
- Check if valid disembark point
- Handle dock vs water exit
- Move character to appropriate room
- Handle swimming if water exit

### Files to Modify

- src/vessels_docking.c - look_outside, disembark
- src/vessels.c - tactical, contacts
- Integration with src/weather.c for get_weather()
