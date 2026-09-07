# Committed world operations

Object transfers and character relocation publish typed facts after their
compound operation finishes. These facts are observations, not veto requests.
Entry, greet, give and receive scripts keep their existing decision boundaries.

## Movement

`domain_relocation_begin/finish` brackets walking through placement, entry and
greet decisions, wall checks and movement hazards. A rollback to the original
room emits nothing. A nested relocation of the same character folds into the
outer final destination, retaining the nested cause. Extraction suppresses the
move; a new character at the same address cannot finish an old operation.

`char_to_room_cause` supplies an actor, direction and cause for direct placement.
Walking, teleportation, forced movement, DG scripts, creation, restoration,
staff relocation and transport have distinct causes. Unclassified low-level
placements remain UNKNOWN; placement without an origin defaults to SPAWN.
Temporary staff/mobile `at` operations returning to their origin are silent.
Origin rooms use generation-safe handles, not retained room-array indices.

A caller must finish every started operation in reverse order, including early
returns. Publication can invoke arbitrary handlers. Resolve retained character
handles again before continuing after callbacks or publishing the final fact.

## Object transfers

Holders distinguish rooms, inventories, equipment slots, containers and the
character bag lists. Actor and cause describe the known initiator. Each
published transfer has a process-local monotonic identity; it is not a durable
transaction ID and is not reused as a database idempotency key.

Removal captures the old holder without publishing. Placement supplies the
new holder and publishes one complete fact. Get, put, drop, give, wear and
remove bracket their full operation so provisional mutations are not reported.
Nested operations on one object fold into the outer outcome. Same-holder
operations and rollbacks are silent. Detached objects awaiting placement emit
nothing until placed or disposed. Extraction supplies a NONE destination.

Command, DG, magic, shop, reset and restore contexts supply causes for transfers
made below those entry points. Contexts are synchronous stack scopes, not
persistent state. Neither facts nor scopes own the referenced entities.
Subscribers must tolerate an unavailable source holder or object, especially
when an earlier subscriber consumes a delivery or extracts an entity.

New compound writers should use `domain_object_transfer_begin/finish`. Merely
calling a removal primitive is not a committed transfer. After a callback,
resolve each retained entity and validate its expected holder before proceeding.
The give path does this after both object and receiver veto scripts.

## Quest integration

Location and acquired-object checks consume committed facts. Rejected room
entry therefore cannot grant discovery credit. Returning an item requires a
resolved player actor, that player's source inventory, and a resolved NPC
receiver holding the item. The existing objective matcher schedules completion
and consumes the item once. Extraction is a separate causal fact; it cannot
award another delivery. The matching loop stops after consuming the item.

This establishes the existing autoquest consumers. It does not implement the
new perception, rescue, negotiation or witness objectives tracked in #108.

## Validation

Production-linked tests cover actual entry-script rejection, accepted walking,
no-op relocation, nested script redirection, character extraction/address reuse,
room/inventory/container transfers, actual give and rejected repeat give,
provisional rollback, character bags, nested transfers, scoped/unscoped disposal
and one committed quest-item consumption. The current full suite has 1,141 tests.
