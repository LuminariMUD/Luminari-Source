"""Phase-oriented world graph loading and cross-file validation."""

from __future__ import annotations

from pathlib import Path
from typing import Any, Iterable

from .indexes import indexed_data_paths, validate_indexes
from .models import Finding, RelatedLocation, RoomRecord, SourceSpan, ValidationResult, ZoneRecord
from .parsing import finding
from .rooms import parse_room_file
from .spec_registry import extract_spec_names
from .zones import parse_zone_file


def _display_path(path: Path, world_root: Path | None, repo_root: Path) -> str:
  if world_root is not None:
    try:
      return path.resolve().relative_to(world_root.resolve()).as_posix()
    except ValueError:
      pass
  try:
    return path.resolve().relative_to(repo_root.resolve()).as_posix()
  except ValueError:
    return path.name


def _package_number(record: ZoneRecord | RoomRecord) -> int | None:
  try:
    return int(record.source_package)
  except ValueError:
    return None


def _selected(record: ZoneRecord | RoomRecord, selected_packages: set[int] | None) -> bool:
  return selected_packages is None or _package_number(record) in selected_packages


def _append_related_finding(
    findings: list[Finding],
    code: str,
    severity: str,
    message: str,
    span: SourceSpan,
    record_type: str,
    vnum: int,
    related: ZoneRecord | RoomRecord | None = None,
) -> None:
  related_location = None
  if related is not None:
    related_location = RelatedLocation(
        related.span.path,
        related.span.line,
        related.span.column,
        "zone" if isinstance(related, ZoneRecord) else "room",
        related.vnum,
    )
  findings.append(
      Finding(
          code,
          severity,
          message,
          span,
          record_type=record_type,
          vnum=vnum,
          related=related_location,
      )
  )


def _validate_zone_order(
    zones: list[ZoneRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
) -> None:
  seen: dict[int, ZoneRecord] = {}
  previous: ZoneRecord | None = None
  prior_ranges: list[ZoneRecord] = []
  for zone in zones:
    if zone.vnum in seen and _selected(zone, selected_packages):
      _append_related_finding(
          findings,
          "ZON040",
          "error",
          f"duplicate zone vnum {zone.vnum}",
          zone.span,
          "zone",
          zone.vnum,
          seen[zone.vnum],
      )
    else:
      seen[zone.vnum] = zone
    if previous is not None and zone.vnum <= previous.vnum and _selected(zone, selected_packages):
      _append_related_finding(
          findings,
          "ZON041",
          "error",
          f"zone vnum {zone.vnum} is not strictly increasing after {previous.vnum}",
          zone.span,
          "zone",
          zone.vnum,
          previous,
      )
    previous = zone
    if zone.bottom is None or zone.top is None:
      continue
    if prior_ranges:
      previous_range = prior_ranges[-1]
      assert previous_range.bottom is not None
      if zone.bottom < previous_range.bottom and _selected(zone, selected_packages):
        _append_related_finding(
            findings,
            "ZON042",
            "error",
            f"zone range starts at {zone.bottom}, before prior range start {previous_range.bottom}",
            zone.span,
            "zone",
            zone.vnum,
            previous_range,
        )
    for prior in prior_ranges:
      assert prior.bottom is not None and prior.top is not None
      if zone.bottom <= prior.top and prior.bottom <= zone.top and (
          _selected(zone, selected_packages) or _selected(prior, selected_packages)
      ):
        subject = zone if _selected(zone, selected_packages) else prior
        related = prior if subject is zone else zone
        _append_related_finding(
            findings,
            "ZON043",
            "error",
            f"zone range {subject.bottom}..{subject.top} overlaps "
            f"{related.bottom}..{related.top}",
            subject.span,
            "zone",
            subject.vnum,
            related,
        )
    prior_ranges.append(zone)


def _validate_room_graph(
    zones: list[ZoneRecord],
    rooms: list[RoomRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    direction_count: int,
) -> None:
  rooms_by_vnum: dict[int, RoomRecord] = {}
  previous: RoomRecord | None = None
  for room in rooms:
    if room.vnum in rooms_by_vnum and _selected(room, selected_packages):
      _append_related_finding(
          findings,
          "WLD040",
          "error",
          f"duplicate room vnum {room.vnum}",
          room.span,
          "room",
          room.vnum,
          rooms_by_vnum[room.vnum],
      )
    else:
      rooms_by_vnum[room.vnum] = room
    if previous is not None and room.vnum <= previous.vnum and _selected(room, selected_packages):
      _append_related_finding(
          findings,
          "WLD041",
          "error",
          f"room vnum {room.vnum} is not strictly increasing after {previous.vnum}",
          room.span,
          "room",
          room.vnum,
          previous,
      )
    previous = room

  for room in rooms:
    emit = _selected(room, selected_packages)
    owners = [
        zone
        for zone in zones
        if zone.bottom is not None and zone.top is not None and zone.bottom <= room.vnum <= zone.top
    ]
    if len(owners) != 1:
      if emit:
        findings.append(
            finding(
                "REF001",
                "error",
                f"room belongs to {len(owners)} zone ranges; expected exactly one",
                room.span,
                "room",
                room.vnum,
            )
        )
    else:
      room.owner_zone_vnum = owners[0].vnum
      if room.file_zone is not None and room.file_zone != owners[0].vnum and emit:
        _append_related_finding(
            findings,
            "REF002",
            "warning",
            f"room flags line names zone {room.file_zone}, but range ownership selects zone {owners[0].vnum}",
            room.span,
            "room",
            room.vnum,
            owners[0],
        )
    if not emit:
      continue
    for exit_record in room.exits:
      if exit_record.destination_vnum >= 0 and exit_record.destination_vnum not in rooms_by_vnum:
        findings.append(
            finding(
                "REF003",
                "error",
                f"exit direction {exit_record.direction} targets missing room {exit_record.destination_vnum}",
                exit_record.span,
                "room",
                room.vnum,
            )
        )
    for connection in room.moving_connections:
      if connection.room_vnum not in rooms_by_vnum:
        findings.append(
            finding(
                "REF004",
                "error",
                f"moving-room connection targets missing room {connection.room_vnum}",
                connection.span,
                "room",
                room.vnum,
            )
        )
      if not 0 <= connection.direction < direction_count:
        findings.append(
            finding(
                "REF005",
                "error",
                f"moving-room connection direction {connection.direction} is outside 0..{direction_count - 1}",
                connection.span,
                "room",
                room.vnum,
            )
        )

  for zone in zones:
    if not _selected(zone, selected_packages):
      continue
    scripted_rooms = {
        room.vnum for room in rooms if room.attachments
    }
    for command in zone.commands:
      room_vnum: int | None = None
      if command.command in {"M", "O"} and len(command.arguments) >= 3:
        room_vnum = command.arguments[2]
        if command.command == "O" and room_vnum == -1:
          room_vnum = None
      elif command.command in {"D", "R"} and command.arguments:
        room_vnum = command.arguments[0]
      elif command.command in {"T", "V"} and len(command.arguments) >= 3:
        room_vnum = command.arguments[2]
      if room_vnum is not None and room_vnum not in rooms_by_vnum:
        findings.append(
            finding(
                "REF006",
                "error",
                f"{command.command} reset targets missing room {room_vnum}",
                command.span,
                "zone",
                zone.vnum,
            )
        )
      if command.command == "D" and len(command.arguments) >= 2:
        target = rooms_by_vnum.get(command.arguments[0])
        direction = command.arguments[1]
        if target is not None and not any(exit_record.direction == direction for exit_record in target.exits):
          findings.append(
              finding(
                  "REF007",
                  "error",
                  f"D reset names direction {direction}, but room {target.vnum} has no such exit",
                  command.span,
                  "zone",
                  zone.vnum,
              )
          )
      if command.command in {"T", "V"} and len(command.arguments) >= 3:
        host_type = command.arguments[0]
        host_room = command.arguments[2]
        if command.command == "T" and host_type == 2 and host_room in rooms_by_vnum:
          scripted_rooms.add(host_room)
        elif (
            command.command == "V"
            and host_type == 2
            and host_room in rooms_by_vnum
            and host_room not in scripted_rooms
        ):
          findings.append(
              finding(
                  "ZON037",
                  "error",
                  f"V reset has no current scripted room host at room {host_room}",
                  command.span,
                  "zone",
                  zone.vnum,
              )
          )


def _load_files(
    zone_paths: Iterable[Path],
    room_paths: Iterable[Path],
    repo_root: Path,
    world_root: Path | None,
    manifest: dict[str, Any],
    config: dict[str, Any],
    selected_packages: set[int] | None,
) -> tuple[list[ZoneRecord], list[RoomRecord], list[Finding], bool]:
  findings: list[Finding] = []
  zones: list[ZoneRecord] = []
  rooms: list[RoomRecord] = []
  complete = True
  direction_count = 10 if config.get("diagonal_dirs") else 6
  spec_names = extract_spec_names(repo_root)
  for path in zone_paths:
    parsed = parse_zone_file(path, _display_path(path, world_root, repo_root), manifest, direction_count)
    zones.extend(parsed.records)
    package = int(path.stem) if path.stem.isdigit() else None
    if selected_packages is None or package in selected_packages:
      findings.extend(parsed.findings)
    elif not parsed.complete:
      findings.append(
          finding(
              "REF009",
              "error",
              "selected validation cannot build a complete reference graph because this "
              "unselected zone file could not be fully parsed",
              SourceSpan(_display_path(path, world_root, repo_root), 1),
          )
      )
    complete = complete and parsed.complete
  for path in room_paths:
    parsed = parse_room_file(
        path,
        _display_path(path, world_root, repo_root),
        manifest,
        bool(config.get("diagonal_dirs")),
        spec_names,
    )
    rooms.extend(parsed.records)
    package = int(path.stem) if path.stem.isdigit() else None
    if selected_packages is None or package in selected_packages:
      findings.extend(parsed.findings)
    elif not parsed.complete:
      findings.append(
          finding(
              "REF009",
              "error",
              "selected validation cannot build a complete reference graph because this "
              "unselected room file could not be fully parsed",
              SourceSpan(_display_path(path, world_root, repo_root), 1),
          )
      )
    complete = complete and parsed.complete
  _validate_zone_order(zones, findings, selected_packages)
  _validate_room_graph(zones, rooms, findings, selected_packages, direction_count)
  return zones, rooms, findings, complete


def validate_indexed_world(
    world_root: Path,
    repo_root: Path,
    manifest: dict[str, Any],
    config: dict[str, Any],
    mini: bool = False,
    selected_packages: set[int] | None = None,
) -> ValidationResult:
  result = validate_indexes(
      world_root,
      repo_root,
      mini=mini,
      selected_packages=selected_packages,
  )
  zone_paths = indexed_data_paths(world_root, "zon", mini)
  room_paths = indexed_data_paths(world_root, "wld", mini)
  if selected_packages is not None:
    for package in sorted(selected_packages):
      for extension, paths in (("zon", zone_paths), ("wld", room_paths)):
        candidate = world_root / extension / f"{package}.{extension}"
        if candidate.is_file() and candidate not in paths:
          paths.append(candidate)
    loaded_zone_packages = {
        int(path.stem) for path in zone_paths if path.stem.isdigit()
    }
    for package in sorted(selected_packages - loaded_zone_packages):
      result.findings.append(
          finding(
              "IDX013",
              "error",
              f"selected zone package {package} has no readable {package}.zon file",
              SourceSpan("zon/index", 1),
          )
      )
      result.complete = False
  _, _, graph_findings, graph_complete = _load_files(
      zone_paths,
      room_paths,
      repo_root,
      world_root,
      manifest,
      config,
      selected_packages,
  )
  result.findings.extend(graph_findings)
  result.complete = result.complete and graph_complete
  result.mode = "zone" if selected_packages is not None else ("mini" if mini else "all")
  result.config.update(config)
  if selected_packages is not None:
    result.config["selected_zones"] = sorted(selected_packages)
  return result


def _collect_explicit_paths(requested_paths: Iterable[Path]) -> tuple[list[Path], list[Path]]:
  zones: list[Path] = []
  rooms: list[Path] = []
  seen: set[Path] = set()
  for requested in requested_paths:
    candidates = [requested] if requested.is_file() else sorted(requested.rglob("*"))
    for candidate in candidates:
      if not candidate.is_file():
        continue
      resolved = candidate.resolve()
      if resolved in seen:
        continue
      seen.add(resolved)
      if candidate.name.startswith("index.") or candidate.name.startswith("index.mini."):
        continue
      if candidate.suffix == ".zon":
        zones.append(candidate)
      elif candidate.suffix == ".wld":
        rooms.append(candidate)
  return zones, rooms


def validate_explicit_paths(
    requested_paths: list[Path],
    repo_root: Path,
    manifest: dict[str, Any],
    config: dict[str, Any],
) -> ValidationResult:
  zone_paths, room_paths = _collect_explicit_paths(requested_paths)
  result = ValidationResult("isolated-paths", "paths", config=dict(config))
  if not zone_paths and not room_paths:
    result.findings.append(
        finding(
            "IDX012",
            "error",
            "explicit paths contain no .zon or .wld files supported by the current phase",
            SourceSpan("<paths>", 1),
        )
    )
    result.complete = False
    return result
  _, _, findings, complete = _load_files(
      zone_paths,
      room_paths,
      repo_root,
      None,
      manifest,
      config,
      None,
  )
  result.findings.extend(findings)
  result.complete = complete
  return result
