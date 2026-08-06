# Session 04: Validated Definition Registry

**Session ID**: `phase00-session04-validated-definition-registry`
**Status**: Not Started
**Estimated Tasks**: ~20-24
**Estimated Duration**: 2-4 hours

---

## Objective

Replace the untyped name-to-function table with immutable, bounds-safe, boot-validated definitions
while preserving every stable persisted identity and legacy handler.

---

## Scope

### In Scope (MVP)

- Define canonical name, display name, explicit aliases, owner mask, supported events,
  binding-source mask, builder visibility, prerequisites, category, description, and handler
  metadata.
- Populate valid metadata for every currently registered definition.
- Preserve Guild as the compatibility canonical identity and Guildmaster as an explicit alias unless
  fresh content evidence requires a tested migration.
- Provide owner-aware, type-aware, and bounds-safe lookup and iteration accessors.
- Validate names, aliases, masks, events, descriptions, visibility, and exactly one handler before
  world parsing.
- Keep the SPECIAL callback ABI and existing public compatibility surface usable.
- Add any new production source to both Makefile.am and CMakeLists.txt.

### Out of Scope

- OLC filtering and presentation changes.
- Authored or effective binding state on prototypes.
- Runtime gateways, typed handlers, chains, or world-file syntax changes.

---

## Prerequisites

- [ ] Session 01 freezes registry identity and persistence compatibility.
- [ ] Sessions 02 and 03 freeze all event categories used to populate definition contracts.

---

## Deliverables

1. An immutable special-procedure definition registry and public access API.
2. Complete metadata for every persisted definition and explicit alias data.
3. Pre-world-load metadata validation with actionable programmer diagnostics.
4. Production-linked validation, lookup, alias, owner, event, and extreme-boundary tests.

---

## Success Criteria

- [ ] Every registered definition passes complete validation before world files are parsed.
- [ ] Duplicate or empty names, alias collisions, invalid masks, empty descriptions, and missing
  handlers fail deterministically.
- [ ] Negative and arbitrarily high accessor indexes return safely.
- [ ] Existing canonical persisted names and case-insensitive lookups remain compatible.
- [ ] Both build manifests remain synchronized and all characterization tests pass.
