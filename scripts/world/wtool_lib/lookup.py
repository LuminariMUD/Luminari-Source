"""Shared-model record lookup and typed reference traversal."""

from __future__ import annotations

from dataclasses import dataclass, fields, is_dataclass
import json
from typing import Any, Iterable

from .models import (
    Finding,
    HlQuestRecord,
    SourceSpan,
    WorldData,
    WorldRecord,
    ZoneRecord,
)


RECORD_TYPE_ALIASES = {
    "zone": "zone",
    "room": "room",
    "mob": "mobile",
    "mobile": "mobile",
    "obj": "object",
    "object": "object",
    "shop": "shop",
    "trigger": "trigger",
    "qst": "quest",
    "quest": "quest",
    "hlq": "hlquest",
    "hlquest": "hlquest",
}
CLI_RECORD_TYPES = ("zone", "room", "mob", "obj", "shop", "trigger", "quest", "hlquest")


@dataclass(frozen=True, slots=True)
class ReferenceEdge:
  source_type: str
  source_vnum: int
  target_type: str
  target_vnum: int
  role: str
  span: SourceSpan

  def sort_key(self) -> tuple[str, int, str, int, str, str, int, int]:
    return (
        self.source_type,
        self.source_vnum,
        self.target_type,
        self.target_vnum,
        self.role,
        self.span.path,
        self.span.line,
        self.span.column,
    )

  def to_dict(self) -> dict[str, Any]:
    return {
        "source_type": self.source_type,
        "source_vnum": self.source_vnum,
        "target_type": self.target_type,
        "target_vnum": self.target_vnum,
        "role": self.role,
        "path": self.span.path,
        "line": self.span.line,
        "column": self.span.column,
    }


def normalize_record_type(record_type: str) -> str:
  try:
    return RECORD_TYPE_ALIASES[record_type]
  except KeyError as error:
    raise ValueError(f"unknown world record type {record_type!r}") from error


def find_records(world: WorldData, record_type: str, vnum: int) -> list[WorldRecord]:
  canonical = normalize_record_type(record_type)
  return [record for record in world.records(canonical) if record.vnum == vnum]


def _model_value(value: Any) -> Any:
  if is_dataclass(value):
    return {
        field.name: _model_value(getattr(value, field.name))
        for field in fields(value)
        if getattr(value, field.name) is not None
    }
  if isinstance(value, dict):
    return {str(key): _model_value(item) for key, item in sorted(value.items())}
  if isinstance(value, (list, tuple)):
    return [_model_value(item) for item in value]
  if isinstance(value, (set, frozenset)):
    return sorted(_model_value(item) for item in value)
  return value


def record_to_dict(record: WorldRecord) -> dict[str, Any]:
  data = _model_value(record)
  assert isinstance(data, dict)
  data["record_type"] = type(record).__name__.removesuffix("Record").lower()
  return data


def _record_type(record: WorldRecord) -> str:
  return type(record).__name__.removesuffix("Record").lower()


def _add_edge(
    edges: list[ReferenceEdge],
    source: WorldRecord,
    target_type: str,
    target_vnum: int,
    role: str,
    span: SourceSpan,
) -> None:
  edges.append(
      ReferenceEdge(
          _record_type(source),
          source.vnum,
          target_type,
          target_vnum,
          role,
          span,
      )
  )


def _zone_edges(zone: ZoneRecord, edges: list[ReferenceEdge]) -> None:
  for command in zone.commands:
    arguments = command.arguments
    if command.command == "M" and arguments:
      _add_edge(edges, zone, "mobile", arguments[0], "M reset prototype", command.span)
    elif command.command in {"O", "E", "G", "P"} and arguments:
      _add_edge(
          edges,
          zone,
          "object",
          arguments[0],
          f"{command.command} reset prototype",
          command.span,
      )
    if command.command == "P" and len(arguments) >= 3:
      _add_edge(edges, zone, "object", arguments[2], "P reset container", command.span)
    elif command.command == "R" and len(arguments) >= 2:
      _add_edge(edges, zone, "object", arguments[1], "R reset object", command.span)
    if command.command in {"M", "O"} and len(arguments) >= 3:
      if command.command != "O" or arguments[2] >= 0:
        _add_edge(
            edges,
            zone,
            "room",
            arguments[2],
            f"{command.command} reset destination",
            command.span,
        )
    elif command.command in {"D", "R"} and arguments:
      _add_edge(
          edges,
          zone,
          "room",
          arguments[0],
          f"{command.command} reset room",
          command.span,
      )
    elif command.command in {"T", "V"} and len(arguments) >= 3:
      _add_edge(
          edges,
          zone,
          "room",
          arguments[2],
          f"{command.command} reset host room",
          command.span,
      )
    if command.command == "T" and len(arguments) >= 2:
      _add_edge(edges, zone, "trigger", arguments[1], "T reset trigger", command.span)


def reference_edges(world: WorldData) -> list[ReferenceEdge]:
  edges: list[ReferenceEdge] = []
  for zone in world.zones:
    _zone_edges(zone, edges)
  for room in world.rooms:
    for exit_record in room.exits:
      if exit_record.destination_vnum >= 0:
        _add_edge(
            edges,
            room,
            "room",
            exit_record.destination_vnum,
            f"exit direction {exit_record.direction}",
            exit_record.span,
        )
      if exit_record.key_vnum >= 0:
        _add_edge(edges, room, "object", exit_record.key_vnum, "exit key", exit_record.span)
    for connection in room.moving_connections:
      _add_edge(
          edges,
          room,
          "room",
          connection.room_vnum,
          "moving-room connection",
          connection.span,
      )
    if room.moving_room is not None and room.moving_room.key_vnum >= 0:
      _add_edge(
          edges,
          room,
          "object",
          room.moving_room.key_vnum,
          "moving-room key",
          room.moving_room.span,
      )
    for attachment in room.attachments:
      _add_edge(
          edges,
          room,
          "trigger",
          attachment.trigger_vnum,
          "inline trigger attachment",
          attachment.span,
      )
  for record in [
      *world.mobiles,
      *world.objects,
      *world.shops,
      *world.quests,
      *world.hlquests,
  ]:
    for reference in record.references:
      _add_edge(
          edges,
          record,
          reference.target_type,
          reference.target_vnum,
          reference.role,
          reference.span,
      )
  for record in [*world.mobiles, *world.objects]:
    for attachment in record.attachments:
      _add_edge(
          edges,
          record,
          "trigger",
          attachment.trigger_vnum,
          "inline trigger attachment",
          attachment.span,
      )
  unique = {edge.sort_key(): edge for edge in edges}
  return [unique[key] for key in sorted(unique)]


def refs_for_record(
    world: WorldData,
    record_type: str,
    vnum: int,
) -> tuple[list[ReferenceEdge], list[ReferenceEdge]]:
  canonical = normalize_record_type(record_type)
  edges = reference_edges(world)
  outgoing = [
      edge
      for edge in edges
      if edge.source_type == canonical and edge.source_vnum == vnum
  ]
  incoming = [
      edge
      for edge in edges
      if edge.target_type == canonical and edge.target_vnum == vnum
  ]
  return outgoing, incoming


def render_show_human(record_type: str, vnum: int, records: list[WorldRecord]) -> str:
  canonical = normalize_record_type(record_type)
  if not records:
    return f"{canonical} {vnum}: not found\n"
  lines: list[str] = []
  for index, record in enumerate(records):
    if index:
      lines.append("")
    lines.append(f"{canonical} {vnum} ({record.span.path}:{record.span.line})")
    data = record_to_dict(record)
    entries = data.pop("entries", None) if isinstance(record, HlQuestRecord) else None
    for name, value in data.items():
      if name in {"record_type", "span", "vnum"}:
        continue
      rendered = json.dumps(value, ensure_ascii=True, sort_keys=True)
      lines.append(f"{name}: {rendered}")
    if entries is not None:
      lines.append("entries (physical order):")
      for entry in record.entries:
        runtime = entry.effective_runtime_ordinal
        lines.append(
            f"  entry physical={entry.physical_ordinal} runtime={runtime} "
            f"type={entry.entry_type} marker={entry.marker + entry.approval_suffix!r} "
            f"approved={str(entry.approved).lower()} "
            f"({entry.span.path}:{entry.span.line})"
        )
        for name in ("keywords", "reply_message", "room_vnum"):
          value = getattr(entry, name)
          if value is not None:
            rendered = json.dumps(value, ensure_ascii=True, sort_keys=True)
            lines.append(f"    {name}: {rendered}")
        if entry.marker != "A":
          lines.append(f"    chain_terminated: {str(entry.chain_terminated).lower()}")
        for command in entry.commands:
          lines.append(
              f"    command physical={command.physical_ordinal} "
              f"runtime={command.effective_runtime_ordinal} "
              f"direction={command.direction or command.direction_marker} "
              f"code={command.code!r} type={command.command_type} "
              f"value={command.value} location={command.location} "
              f"({command.span.path}:{command.span.line})"
          )
  return "\n".join(lines) + "\n"


def render_refs_human(
    record_type: str,
    vnum: int,
    found: bool,
    outgoing: Iterable[ReferenceEdge],
    incoming: Iterable[ReferenceEdge],
) -> str:
  canonical = normalize_record_type(record_type)
  lines = [f"{canonical} {vnum}: {'found' if found else 'not found'}", "outgoing:"]
  outgoing_list = list(outgoing)
  incoming_list = list(incoming)
  if not outgoing_list:
    lines.append("  (none)")
  for edge in outgoing_list:
    lines.append(
        f"  {edge.target_type} {edge.target_vnum}: {edge.role} "
        f"({edge.span.path}:{edge.span.line})"
    )
  lines.append("incoming:")
  if not incoming_list:
    lines.append("  (none)")
  for edge in incoming_list:
    lines.append(
        f"  {edge.source_type} {edge.source_vnum}: {edge.role} "
        f"({edge.span.path}:{edge.span.line})"
    )
  return "\n".join(lines) + "\n"


def lookup_error_findings(world: WorldData) -> list[Finding]:
  """Expose parse errors for callers that want lookup health metadata."""

  return [finding for finding in world.findings if finding.severity == "error"]
