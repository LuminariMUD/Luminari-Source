# Session 05: Owner-Aware OLC

**Session ID**: `phase00-session05-owner-aware-olc`
**Status**: Not Started
**Estimated Tasks**: ~18-22
**Estimated Duration**: 2-4 hours

---

## Objective

Make medit, oedit, and redit expose only compatible builder-visible definitions and explain the
events and prerequisites required for each selection.

---

## Scope

### In Scope (MVP)

- Filter mobile, object, and room selection lists by definition owner compatibility and visibility.
- Preserve stable menu numbering within each filtered view and correct selection-to-definition
  mapping.
- Show display name, category, description, supported events, required flags, placement, and combat
  prerequisites in a readable builder flow.
- Preserve explicit clear behavior and current single-handler editing semantics.
- Reject owner-incompatible selections through the production OLC paths.
- Add production-linked tests for all three editors, empty lists, selection bounds, and clear paths.

### Out of Scope

- Automatic MOB_SPEC or ITEM_AUTOPROC mutation.
- Multiple procedures per prototype or changes to outer command traversal.
- Authored raw-name ownership, writer provenance, and boot collision diagnostics.

---

## Prerequisites

- [ ] Session 04 provides validated owner, event, visibility, description, and prerequisite
  metadata.
- [ ] Session 01 provides baseline OLC selection and clear characterization.

---

## Deliverables

1. Owner-filtered definition selection for medit, oedit, and redit.
2. Builder-visible descriptions, event support, and runtime prerequisites.
3. Shared selection helpers that retain editor-specific owner context.
4. Production-linked three-editor compatibility and boundary tests.

---

## Success Criteria

- [ ] Each editor lists only definitions valid for its owner type and allowed binding source.
- [ ] Builders can inspect why and when a definition runs before selecting it.
- [ ] Select and clear operations map to the intended definition in all three editors.
- [ ] Incompatible selections fail explicitly without changing activation flags or world syntax.
