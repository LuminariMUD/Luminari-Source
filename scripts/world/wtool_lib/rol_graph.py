"""Source-to-target graph parity checks for isolated RoL conversion output."""

from __future__ import annotations

from collections import defaultdict
from typing import Any, Iterable

from .rol_source import RolRecord


ROL_GRAPH_SCHEMA_VERSION = 1


def _action_disposition(action: dict[str, Any]) -> str:
  value = action.get("final_action", action.get("action", ""))
  return str(value)


def _edge_payload(
    target_room: int,
    direction: int,
    target_destination: int,
    source: tuple[str, int, int, str, int] | None = None,
) -> dict[str, Any]:
  row: dict[str, Any] = {
      "target_room": target_room,
      "direction": direction,
      "target_destination": target_destination,
  }
  if source is not None:
    basename, source_room, source_destination, path, line = source
    row.update(
        {
            "basename": basename,
            "source_room": source_room,
            "source_destination": source_destination,
            "source_path": path,
            "source_line": line,
        }
    )
  return row


def audit_connection_graph(
    source_records: Iterable[RolRecord],
    actions: Iterable[dict[str, Any]],
    target_rooms: Iterable[Any],
    target_record_groups: Iterable[tuple[str, Iterable[Any]]] = (),
) -> dict[str, Any]:
  """Compare RoL traversal connections and reject typed cross-world room edges."""

  action_rows = list(actions)
  records = list(source_records)
  room_actions = [
      action
      for action in action_rows
      if action.get("source_kind") == "wld"
      and _action_disposition(action) != "EXCLUDE"
      and action.get("destination_vnum") is not None
  ]
  destination_by_record = {
      str(action["source_record_id"]): int(action["destination_vnum"])
      for action in room_actions
  }
  destinations_by_source: defaultdict[int, set[int]] = defaultdict(set)
  for action in room_actions:
    destinations_by_source[int(action["source_vnum"])].add(
        int(action["destination_vnum"])
    )
  ambiguous_identities = [
      {"source_room": source, "target_rooms": sorted(destinations)}
      for source, destinations in sorted(destinations_by_source.items())
      if len(destinations) != 1
  ]

  expected: set[tuple[int, int, int]] = set()
  expected_source: dict[tuple[int, int, int], tuple[str, int, int, str, int]] = {}
  unresolved_source_exits: list[dict[str, Any]] = []
  ambiguous_source_exits: list[dict[str, Any]] = []
  selected_source_records = 0
  for record in records:
    if record.kind != "wld" or record.record_id not in destination_by_record:
      continue
    selected_source_records += 1
    target_room = destination_by_record[record.record_id]
    for directive in record.directives:
      arguments = list(directive.get("arguments", []))
      if (
          directive.get("token") != "D"
          or len(arguments) < 3
          or directive.get("source_defaulted_destination")
          or int(arguments[2]) <= 0
      ):
        continue
      source_destination = int(arguments[2])
      target_destinations = destinations_by_source.get(source_destination, set())
      source = {
          "basename": record.basename,
          "source_room": record.vnum,
          "direction": int(directive["direction"]),
          "source_destination": source_destination,
          "source_path": record.path,
          "source_line": int(directive["line"]),
      }
      if not target_destinations:
        unresolved_source_exits.append(source)
        continue
      if len(target_destinations) != 1:
        ambiguous_source_exits.append(
            {**source, "target_destinations": sorted(target_destinations)}
        )
        continue
      target_destination = next(iter(target_destinations))
      edge = (target_room, int(directive["direction"]), target_destination)
      expected.add(edge)
      expected_source.setdefault(
          edge,
          (
              record.basename,
              record.vnum,
              source_destination,
              record.path,
              int(directive["line"]),
          ),
      )

  object_actions = [
      action
      for action in action_rows
      if action.get("source_kind") == "obj"
      and _action_disposition(action) != "EXCLUDE"
      and action.get("destination_vnum") is not None
  ]
  object_destination_by_record = {
      str(action["source_record_id"]): int(action["destination_vnum"])
      for action in object_actions
  }
  source_object_roles = {
      "teleport_destination": "portal destination",
      "switch_room": "switch room",
  }
  expected_object_connections: set[tuple[int, str, int]] = set()
  expected_object_source: dict[
      tuple[int, str, int], tuple[str, int, int, str, int]
  ] = {}
  unresolved_source_object_connections: list[dict[str, Any]] = []
  for record in records:
    if record.kind != "obj" or record.record_id not in object_destination_by_record:
      continue
    for reference in record.references:
      target_role = source_object_roles.get(reference.role)
      if target_role is None or reference.target_vnum <= 0:
        continue
      target_destinations = destinations_by_source.get(reference.target_vnum, set())
      source = {
          "basename": record.basename,
          "source_object": record.vnum,
          "role": reference.role,
          "source_destination": reference.target_vnum,
          "source_path": reference.path,
          "source_line": reference.line,
      }
      if len(target_destinations) != 1:
        unresolved_source_object_connections.append(
            {**source, "target_destinations": sorted(target_destinations)}
        )
        continue
      edge = (
          object_destination_by_record[record.record_id],
          target_role,
          next(iter(target_destinations)),
      )
      expected_object_connections.add(edge)
      expected_object_source.setdefault(
          edge,
          (
              record.basename,
              record.vnum,
              reference.target_vnum,
              reference.path,
              reference.line,
          ),
      )

  rooms = list(target_rooms)
  record_groups = {kind: list(group) for kind, group in target_record_groups}
  mapped_rooms = set(destination_by_record.values())
  target_room_vnums = {int(room.vnum) for room in rooms}
  missing_target_rooms = sorted(mapped_rooms - target_room_vnums)
  unmapped_namespace_rooms = sorted(
      room.vnum
      for room in rooms
      if 2_000_000 <= room.vnum <= 2_999_999 and room.vnum not in mapped_rooms
  )
  actual: set[tuple[int, int, int]] = set()
  actual_locations: dict[tuple[int, int, int], dict[str, Any]] = {}
  cross_world: list[dict[str, Any]] = []
  for room in rooms:
    for exit_record in room.exits:
      destination = int(exit_record.destination_vnum)
      if destination <= 0:
        continue
      edge = (int(room.vnum), int(exit_record.direction), destination)
      if room.vnum in mapped_rooms:
        actual.add(edge)
        actual_locations[edge] = {
            "target_path": exit_record.span.path,
            "target_line": exit_record.span.line,
        }
      if room.vnum in mapped_rooms and destination not in mapped_rooms:
        cross_world.append(
            {
                "direction": "rol_to_non_rol",
                "target_room": room.vnum,
                "exit_direction": exit_record.direction,
                "target_destination": destination,
                "target_path": exit_record.span.path,
                "target_line": exit_record.span.line,
            }
        )
      elif room.vnum not in mapped_rooms and destination in mapped_rooms:
        cross_world.append(
            {
                "direction": "non_rol_to_rol",
                "target_room": room.vnum,
                "exit_direction": exit_record.direction,
                "target_destination": destination,
                "target_path": exit_record.span.path,
                "target_line": exit_record.span.line,
            }
        )

  actual_object_connections: set[tuple[int, str, int]] = set()
  actual_object_locations: dict[tuple[int, str, int], dict[str, Any]] = {}
  mapped_by_kind: defaultdict[str, set[int]] = defaultdict(set)
  for action in action_rows:
    if (
        _action_disposition(action) != "EXCLUDE"
        and action.get("destination_vnum") is not None
    ):
      mapped_by_kind[str(action.get("source_kind"))].add(
          int(action["destination_vnum"])
      )
  typed_cross_world: list[dict[str, Any]] = []
  typed_room_references = 0
  for kind, group in sorted(record_groups.items()):
    owners = mapped_by_kind[kind]
    for record in group:
      owner_is_rol = record.vnum in owners
      for reference in record.references:
        if reference.target_type != "room" or reference.target_vnum <= 0:
          continue
        target_is_rol = reference.target_vnum in mapped_rooms
        if owner_is_rol:
          typed_room_references += 1
        if kind == "obj" and owner_is_rol and reference.role in {
            "portal destination",
            "switch room",
        }:
          edge = (record.vnum, reference.role, reference.target_vnum)
          actual_object_connections.add(edge)
          actual_object_locations[edge] = {
              "target_path": reference.span.path,
              "target_line": reference.span.line,
          }
        if owner_is_rol == target_is_rol:
          continue
        typed_cross_world.append(
            {
                "direction": "rol_to_non_rol" if owner_is_rol else "non_rol_to_rol",
                "owner_kind": kind,
                "owner_vnum": record.vnum,
                "role": reference.role,
                "target_room": reference.target_vnum,
                "target_path": reference.span.path,
                "target_line": reference.span.line,
            }
        )

  missing = sorted(expected - actual)
  extra = sorted(actual - expected)
  missing_rows = [
      _edge_payload(*edge, expected_source.get(edge)) for edge in missing
  ]
  extra_rows = [
      {**_edge_payload(*edge), **actual_locations.get(edge, {})} for edge in extra
  ]
  missing_object_connections = sorted(
      expected_object_connections - actual_object_connections
  )
  extra_object_connections = sorted(
      actual_object_connections - expected_object_connections
  )
  missing_object_rows = [
      {
          "target_object": edge[0],
          "role": edge[1],
          "target_room": edge[2],
          "source": expected_object_source.get(edge),
      }
      for edge in missing_object_connections
  ]
  extra_object_rows = [
      {
          "target_object": edge[0],
          "role": edge[1],
          "target_room": edge[2],
          **actual_object_locations.get(edge, {}),
      }
      for edge in extra_object_connections
  ]
  passed = not any(
      (
          ambiguous_identities,
          ambiguous_source_exits,
          missing_target_rooms,
          unmapped_namespace_rooms,
          missing_rows,
          extra_rows,
          cross_world,
          missing_object_rows,
          extra_object_rows,
          typed_cross_world,
      )
  )
  return {
      "schema_version": ROL_GRAPH_SCHEMA_VERSION,
      "summary": {
          "source_room_actions": len(room_actions),
          "source_room_records": selected_source_records,
          "expected_resolvable_directed_exits": len(expected),
          "unresolved_source_exits": len(unresolved_source_exits),
          "actual_directed_exits": len(actual),
          "missing_expected_exits": len(missing_rows),
          "extra_target_exits": len(extra_rows),
          "missing_target_rooms": len(missing_target_rooms),
          "unmapped_namespace_rooms": len(unmapped_namespace_rooms),
          "cross_world_exits": len(cross_world),
          "expected_resolvable_object_room_connections": len(
              expected_object_connections
          ),
          "unresolved_source_object_connections": len(
              unresolved_source_object_connections
          ),
          "actual_object_room_connections": len(actual_object_connections),
          "missing_object_room_connections": len(missing_object_rows),
          "extra_object_room_connections": len(extra_object_rows),
          "typed_room_references": typed_room_references,
          "cross_world_typed_room_references": len(typed_cross_world),
          "pass": passed,
      },
      "ambiguous_identities": ambiguous_identities,
      "ambiguous_source_exits": ambiguous_source_exits,
      "unresolved_source_exits": unresolved_source_exits,
      "missing_target_rooms": missing_target_rooms,
      "unmapped_namespace_rooms": unmapped_namespace_rooms,
      "missing_expected_exits": missing_rows,
      "extra_target_exits": extra_rows,
      "cross_world_exits": sorted(
          cross_world,
          key=lambda row: (
              row["target_room"],
              row["exit_direction"],
              row["target_destination"],
          ),
      ),
      "unresolved_source_object_connections": unresolved_source_object_connections,
      "missing_object_room_connections": missing_object_rows,
      "extra_object_room_connections": extra_object_rows,
      "cross_world_typed_room_references": sorted(
          typed_cross_world,
          key=lambda row: (
              row["owner_kind"],
              row["owner_vnum"],
              row["role"],
              row["target_room"],
          ),
      ),
  }
