"""Phase-oriented world graph loading and cross-file validation."""

from __future__ import annotations

from pathlib import Path
from typing import Any, Iterable

from .flags import decode_tokens
from .hlquests import parse_hlquest_file
from .indexes import DATA_EXTENSIONS, indexed_data_paths, validate_indexes
from .mobiles import parse_mobile_file
from .models import (
    Finding,
    HlQuestRecord,
    MobileRecord,
    ObjectRecord,
    QuestRecord,
    RelatedLocation,
    RoomRecord,
    ShopRecord,
    SourceSpan,
    TriggerRecord,
    ValidationResult,
    WorldData,
    WorldRecord,
    ZoneRecord,
)
from .objects import parse_object_file
from .parsing import finding
from .quests import parse_quest_file
from .rooms import parse_room_file
from .semantics import validate_semantics
from .shops import parse_shop_file
from .spec_registry import extract_spec_names
from .triggers import parse_trigger_file
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


def _package_number(record: WorldRecord) -> int | None:
  try:
    return int(record.source_package)
  except ValueError:
    return None


def _selected(record: WorldRecord, selected_packages: set[int] | None) -> bool:
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
  owners_by_room: list[list[ZoneRecord]] = [[] for _ in rooms]
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

  ranged_zones = sorted(
      (zone for zone in zones if zone.bottom is not None and zone.top is not None),
      key=lambda zone: (zone.bottom, zone.top),
  )
  active_zones: list[ZoneRecord] = []
  zone_index = 0
  for room_index, room in sorted(enumerate(rooms), key=lambda item: item[1].vnum):
    active_zones = [zone for zone in active_zones if zone.top is not None and zone.top >= room.vnum]
    while zone_index < len(ranged_zones):
      candidate = ranged_zones[zone_index]
      assert candidate.bottom is not None
      if candidate.bottom > room.vnum:
        break
      assert candidate.top is not None
      if candidate.top >= room.vnum:
        active_zones.append(candidate)
      zone_index += 1
    owners_by_room[room_index] = list(active_zones)

  for room_index, room in enumerate(rooms):
    emit = _selected(room, selected_packages)
    owners = owners_by_room[room_index]
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

  initial_scripted_rooms = {room.vnum for room in rooms if room.attachments}
  for zone in zones:
    if not _selected(zone, selected_packages):
      continue
    scripted_rooms = set(initial_scripted_rooms)
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


def _related(record: WorldRecord, record_type: str) -> RelatedLocation:
  return RelatedLocation(
      record.span.path,
      record.span.line,
      record.span.column,
      record_type,
      record.vnum,
  )


def _validate_record_order(
    records: list[WorldRecord],
    record_type: str,
    code_prefix: str,
    findings: list[Finding],
    selected_packages: set[int] | None,
) -> None:
  seen: dict[int, WorldRecord] = {}
  previous: WorldRecord | None = None
  for record in records:
    emit = _selected(record, selected_packages)
    if record.vnum in seen and emit:
      findings.append(
          Finding(
              f"{code_prefix}040",
              "error",
              f"duplicate {record_type} vnum {record.vnum}",
              record.span,
              record_type=record_type,
              vnum=record.vnum,
              related=_related(seen[record.vnum], record_type),
          )
      )
    else:
      seen[record.vnum] = record
    if (
        previous is not None
        and record.source_package == previous.source_package
        and record.vnum <= previous.vnum
        and emit
    ):
      findings.append(
          Finding(
              f"{code_prefix}041",
              "error",
              f"{record_type} vnum {record.vnum} is not strictly increasing after "
              f"{previous.vnum}",
              record.span,
              record_type=record_type,
              vnum=record.vnum,
              related=_related(previous, record_type),
          )
      )
    previous = record


def _owning_zones(vnum: int, zones: list[ZoneRecord]) -> list[ZoneRecord]:
  return [
      zone
      for zone in zones
      if zone.bottom is not None and zone.top is not None and zone.bottom <= vnum <= zone.top
  ]


def _validate_packaging(
    records: list[WorldRecord],
    record_type: str,
    zones: list[ZoneRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
) -> None:
  for record in records:
    if not _selected(record, selected_packages):
      continue
    package = _package_number(record)
    owners = _owning_zones(record.vnum, zones)
    if package is not None and len(owners) == 1 and package != owners[0].vnum:
      findings.append(
          Finding(
              "REF010",
              "warning",
              f"{record_type} {record.vnum} is packaged in {package}, but zone range ownership "
              f"selects {owners[0].vnum}",
              record.span,
              record_type=record_type,
              vnum=record.vnum,
              related=_related(owners[0], "zone"),
          )
      )


def _all_types_for_vnum(
    vnum: int,
    maps: dict[str, dict[int, WorldRecord]],
) -> list[str]:
  return sorted(record_type for record_type, records in maps.items() if vnum in records)


def _validate_reference(
    findings: list[Finding],
    source: WorldRecord,
    target_type: str,
    target_vnum: int,
    role: str,
    span: SourceSpan,
    maps: dict[str, dict[int, WorldRecord]],
    missing_code: str = "REF022",
    wrong_type_code: str = "REF023",
    include_related: bool = False,
) -> None:
  if target_type not in maps:
    return
  if target_vnum in maps[target_type]:
    return
  record_type = type(source).__name__.removesuffix("Record").lower()
  other_types = _all_types_for_vnum(target_vnum, maps)
  if include_related:
    other_types = [item for item in other_types if item != record_type]
  related = None
  if other_types:
    code = wrong_type_code
    message = (
        f"{role} requires {target_type} {target_vnum}, but that vnum exists only as "
        f"{', '.join(other_types)}"
    )
    if include_related:
      related_type = other_types[0]
      related = _related(maps[related_type][target_vnum], related_type)
  else:
    code = missing_code
    message = f"{role} targets missing {target_type} {target_vnum}"
  findings.append(
      Finding(
          code,
          "error",
          message,
          span,
          record_type=record_type,
          vnum=source.vnum,
          related=related,
      )
  )


def _validate_attachment(
    findings: list[Finding],
    source: WorldRecord,
    host_type: str,
    trigger_vnum: int,
    span: SourceSpan,
    triggers: dict[int, TriggerRecord],
    maps: dict[str, dict[int, WorldRecord]],
) -> None:
  if "trigger" not in maps:
    return
  expected = {"mobile": 0, "object": 1, "room": 2}[host_type]
  target = triggers.get(trigger_vnum)
  if target is None:
    _validate_reference(
        findings,
        source,
        "trigger",
        trigger_vnum,
        f"inline {host_type} trigger",
        span,
        maps,
    )
  elif target.attach_type != expected:
    record_type = type(source).__name__.removesuffix("Record").lower()
    findings.append(
        Finding(
            "REF021",
            "error",
            f"trigger {trigger_vnum} declares attach type {target.attach_type}; "
            f"{host_type} hosts require {expected}",
            span,
            record_type=record_type,
            vnum=source.vnum,
            related=_related(target, "trigger"),
        )
    )


def _validate_full_graph(
    zones: list[ZoneRecord],
    rooms: list[RoomRecord],
    mobiles: list[MobileRecord],
    objects: list[ObjectRecord],
    triggers: list[TriggerRecord],
    shops: list[ShopRecord],
    quests: list[QuestRecord],
    hlquests: list[HlQuestRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
    reference_types: set[str],
) -> None:
  typed_lists: list[tuple[list[WorldRecord], str, str]] = [
      (list(mobiles), "mobile", "MOB"),
      (list(objects), "object", "OBJ"),
      (list(triggers), "trigger", "TRG"),
      (list(shops), "shop", "SHP"),
      (list(quests), "quest", "QST"),
      (list(hlquests), "hlquest", "HLQ"),
  ]
  for records, record_type, prefix in typed_lists:
    _validate_record_order(records, record_type, prefix, findings, selected_packages)
    _validate_packaging(records, record_type, zones, findings, selected_packages)

  all_maps: dict[str, dict[int, WorldRecord]] = {
      "zone": {record.vnum: record for record in zones},
      "room": {record.vnum: record for record in rooms},
      "mobile": {record.vnum: record for record in mobiles},
      "object": {record.vnum: record for record in objects},
      "trigger": {record.vnum: record for record in triggers},
      "shop": {record.vnum: record for record in shops},
      "quest": {record.vnum: record for record in quests},
      "hlquest": {record.vnum: record for record in hlquests},
  }
  maps = {
      record_type: records
      for record_type, records in all_maps.items()
      if record_type in reference_types
  }
  triggers_by_vnum = {record.vnum: record for record in triggers}
  objects_by_vnum = {record.vnum: record for record in objects}
  item_key = next(
      entry["index"]
      for entry in manifest["tables"]["item-types"]["entries"]
      if entry["macro"] == "ITEM_KEY"
  )
  container_types = {
      entry["index"]
      for entry in manifest["tables"]["item-types"]["entries"]
      if entry["macro"] in {"ITEM_CONTAINER", "ITEM_AMMO_POUCH"}
  }
  key_uses: dict[int, list[tuple[RoomRecord, SourceSpan, str]]] = {}

  for record in [*mobiles, *objects, *shops]:
    if not _selected(record, selected_packages):
      continue
    for reference in record.references:
      _validate_reference(
          findings,
          record,
          reference.target_type,
          reference.target_vnum,
          reference.role,
          reference.span,
          maps,
      )
      if reference.role.endswith("key"):
        key = objects_by_vnum.get(reference.target_vnum)
        if key is not None and key.item_type != item_key:
          findings.append(
              Finding(
                  "REF025",
                  "warning",
                  f"{reference.role} {key.vnum} has item type {key.item_type}, not ITEM_KEY",
                  reference.span,
                  record_type=type(record).__name__.removesuffix("Record").lower(),
                  vnum=record.vnum,
                  related=_related(key, "object"),
              )
          )

  for records, missing_code, wrong_type_code in (
      (quests, "REF032", "REF033"),
      (hlquests, "REF034", "REF035"),
  ):
    for record in records:
      if not _selected(record, selected_packages):
        continue
      for reference in record.references:
        _validate_reference(
            findings,
            record,
            reference.target_type,
            reference.target_vnum,
            reference.role,
            reference.span,
            maps,
            missing_code,
            wrong_type_code,
            include_related=True,
        )

  for room in rooms:
    if not _selected(room, selected_packages):
      continue
    for attachment in room.attachments:
      _validate_attachment(
          findings,
          room,
          "room",
          attachment.trigger_vnum,
          attachment.span,
          triggers_by_vnum,
          maps,
      )
    keys = [(exit_record.key_vnum, exit_record.span, "exit key") for exit_record in room.exits]
    if room.moving_room is not None:
      keys.append((room.moving_room.key_vnum, room.moving_room.span, "moving-room key"))
    for key_vnum, span, role in keys:
      if key_vnum < 0:
        continue
      key_uses.setdefault(key_vnum, []).append((room, span, role))

  for key_vnum, uses in key_uses.items():
    room, span, role = uses[0]
    key = objects_by_vnum.get(key_vnum)
    if key is None:
      for source_room, source_span, source_role in uses:
        _validate_reference(
            findings,
            source_room,
            "object",
            key_vnum,
            source_role,
            source_span,
            maps,
        )
    elif key.item_type != item_key:
      count_suffix = f" ({len(uses)} key uses)" if len(uses) > 1 else ""
      findings.append(
          Finding(
              "REF025",
              "warning",
              f"{role} {key_vnum} has item type {key.item_type}, not ITEM_KEY{count_suffix}",
              span,
              record_type="room",
              vnum=room.vnum,
              related=_related(key, "object"),
          )
      )

  for record, host_type in [
      *((record, "mobile") for record in mobiles),
      *((record, "object") for record in objects),
  ]:
    if not _selected(record, selected_packages):
      continue
    for attachment in record.attachments:
      _validate_attachment(
          findings,
          record,
          host_type,
          attachment.trigger_vnum,
          attachment.span,
          triggers_by_vnum,
          maps,
      )

  wear_slots = manifest["tables"]["wear-slots"]["entries"]
  wear_by_macro = {
      entry["macro"]: entry["index"]
      for entry in manifest["tables"]["obj-wear"]["entries"]
      if entry.get("macro")
  }
  finger_wear = wear_by_macro.get("ITEM_WEAR_FINGER")
  tail_wear = wear_by_macro.get("ITEM_WEAR_TAIL")
  for zone in zones:
    if not _selected(zone, selected_packages):
      continue
    for command in zone.commands:
      prototype: tuple[str, int, str] | None = None
      if command.command == "M" and command.arguments:
        prototype = ("mobile", command.arguments[0], "M reset prototype")
      elif command.command in {"O", "E", "G", "P"} and command.arguments:
        prototype = ("object", command.arguments[0], f"{command.command} reset prototype")
      elif command.command == "R" and len(command.arguments) >= 2:
        prototype = ("object", command.arguments[1], "R reset object")
      if prototype is not None:
        _validate_reference(
            findings,
            zone,
            prototype[0],
            prototype[1],
            prototype[2],
            command.span,
            maps,
        )
      if command.command == "P" and len(command.arguments) >= 3:
        _validate_reference(
            findings,
            zone,
            "object",
            command.arguments[2],
            "P reset container",
            command.span,
            maps,
        )
        container = objects_by_vnum.get(command.arguments[2])
        if container is not None and container.item_type not in container_types:
          findings.append(
              Finding(
                  "REF031",
                  "warning",
                  f"P reset target {container.vnum} has item type {container.item_type}; "
                  "expected ITEM_CONTAINER or ITEM_AMMO_POUCH",
                  command.span,
                  record_type="zone",
                  vnum=zone.vnum,
                  related=_related(container, "object"),
              )
          )
      if command.command == "E" and len(command.arguments) >= 3:
        obj = objects_by_vnum.get(command.arguments[0])
        position = command.arguments[2]
        if obj is not None and 0 <= position < len(wear_slots):
          wear_bits = decode_tokens(obj.wear_flags, len(manifest["tables"]["obj-wear"]["entries"]))
          required = wear_slots[position]["required_wear_index"]
          ring_on_tail = required == tail_wear and finger_wear in wear_bits.bits
          if required not in wear_bits.bits and not ring_on_tail:
            findings.append(
                Finding(
                    "REF030",
                    "warning",
                    f"E reset equips object {obj.vnum} in slot {position}, which requires "
                    f"{wear_slots[position]['required_wear_macro']}",
                    command.span,
                    record_type="zone",
                    vnum=zone.vnum,
                    related=_related(obj, "object"),
                )
            )
      if command.command == "T" and len(command.arguments) >= 2:
        host_type = command.arguments[0]
        trigger_vnum = command.arguments[1]
        target = triggers_by_vnum.get(trigger_vnum)
        if target is None:
          _validate_reference(
              findings,
              zone,
              "trigger",
              trigger_vnum,
              "T reset trigger",
              command.span,
              maps,
          )
        elif host_type in {0, 1, 2} and target.attach_type != host_type:
          findings.append(
              Finding(
                  "REF021",
                  "error",
                  f"T reset host type {host_type} disagrees with trigger {trigger_vnum} "
                  f"attach type {target.attach_type}",
                  command.span,
                  record_type="zone",
                  vnum=zone.vnum,
                  related=_related(target, "trigger"),
              )
          )


def _load_files(
    zone_paths: Iterable[Path],
    room_paths: Iterable[Path],
    mobile_paths: Iterable[Path],
    object_paths: Iterable[Path],
    trigger_paths: Iterable[Path],
    shop_paths: Iterable[Path],
    quest_paths: Iterable[Path],
    hlquest_paths: Iterable[Path],
    repo_root: Path,
    world_root: Path | None,
    manifest: dict[str, Any],
    config: dict[str, Any],
    selected_packages: set[int] | None,
    reference_types: set[str],
) -> tuple[
    list[ZoneRecord],
    list[RoomRecord],
    list[MobileRecord],
    list[ObjectRecord],
    list[TriggerRecord],
    list[ShopRecord],
    list[QuestRecord],
    list[HlQuestRecord],
    list[Finding],
    bool,
]:
  findings: list[Finding] = []
  zones: list[ZoneRecord] = []
  rooms: list[RoomRecord] = []
  mobiles: list[MobileRecord] = []
  objects: list[ObjectRecord] = []
  triggers: list[TriggerRecord] = []
  shops: list[ShopRecord] = []
  quests: list[QuestRecord] = []
  hlquests: list[HlQuestRecord] = []
  complete = True
  direction_count = 10 if config.get("diagonal_dirs") else 6
  spec_names = extract_spec_names(repo_root)

  def merge_parse(path: Path, label: str, parsed: Any, records: list[Any]) -> None:
    nonlocal complete
    records.extend(parsed.records)
    package = int(path.stem) if path.stem.isdigit() else None
    if selected_packages is None or package in selected_packages:
      findings.extend(parsed.findings)
    elif not parsed.complete:
      findings.append(
          finding(
              "REF009",
              "error",
              "selected validation cannot build a complete reference graph because this "
              f"unselected {label} file could not be fully parsed",
              SourceSpan(_display_path(path, world_root, repo_root), 1),
          )
      )
    complete = complete and parsed.complete

  for path in zone_paths:
    parsed = parse_zone_file(
        path, _display_path(path, world_root, repo_root), manifest, direction_count
    )
    merge_parse(path, "zone", parsed, zones)
  for path in room_paths:
    parsed = parse_room_file(
        path,
        _display_path(path, world_root, repo_root),
        manifest,
        bool(config.get("diagonal_dirs")),
        spec_names,
    )
    merge_parse(path, "room", parsed, rooms)
  for path in mobile_paths:
    parsed = parse_mobile_file(
        path,
        _display_path(path, world_root, repo_root),
        manifest,
        spec_names,
    )
    merge_parse(path, "mobile", parsed, mobiles)
  for path in object_paths:
    parsed = parse_object_file(
        path,
        _display_path(path, world_root, repo_root),
        manifest,
        spec_names,
    )
    merge_parse(path, "object", parsed, objects)
  for path in trigger_paths:
    parsed = parse_trigger_file(path, _display_path(path, world_root, repo_root), manifest)
    merge_parse(path, "trigger", parsed, triggers)
  for path in shop_paths:
    parsed = parse_shop_file(path, _display_path(path, world_root, repo_root), manifest)
    merge_parse(path, "shop", parsed, shops)
  for path in quest_paths:
    parsed = parse_quest_file(path, _display_path(path, world_root, repo_root), manifest)
    merge_parse(path, "quest", parsed, quests)
  for path in hlquest_paths:
    parsed = parse_hlquest_file(path, _display_path(path, world_root, repo_root), manifest)
    merge_parse(path, "hlquest", parsed, hlquests)
  _validate_zone_order(zones, findings, selected_packages)
  _validate_room_graph(zones, rooms, findings, selected_packages, direction_count)
  _validate_full_graph(
      zones,
      rooms,
      mobiles,
      objects,
      triggers,
      shops,
      quests,
      hlquests,
      findings,
      selected_packages,
      manifest,
      reference_types,
  )
  findings.extend(
      validate_semantics(
          zones,
          rooms,
          mobiles,
          objects,
          triggers,
          selected_packages,
          manifest,
          direction_count,
          quests,
          hlquests,
      )
  )
  return (
      zones,
      rooms,
      mobiles,
      objects,
      triggers,
      shops,
      quests,
      hlquests,
      findings,
      complete,
  )


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
  mobile_paths = indexed_data_paths(world_root, "mob", mini)
  object_paths = indexed_data_paths(world_root, "obj", mini)
  trigger_paths = indexed_data_paths(world_root, "trg", mini)
  shop_paths = indexed_data_paths(world_root, "shp", mini)
  quest_paths = indexed_data_paths(world_root, "qst", mini)
  hlquest_paths = indexed_data_paths(world_root, "hlq", mini)
  if selected_packages is not None:
    for package in sorted(selected_packages):
      for extension, paths in (
          ("zon", zone_paths),
          ("wld", room_paths),
          ("mob", mobile_paths),
          ("obj", object_paths),
          ("trg", trigger_paths),
          ("shp", shop_paths),
          ("qst", quest_paths),
          ("hlq", hlquest_paths),
      ):
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
  world = load_indexed_world_data(
      world_root,
      repo_root,
      manifest,
      config,
      mini=mini,
      selected_packages=selected_packages,
  )
  result.findings.extend(world.findings)
  result.complete = result.complete and world.complete
  result.mode = "zone" if selected_packages is not None else ("mini" if mini else "all")
  result.config.update(config)
  if selected_packages is not None:
    result.config["selected_zones"] = sorted(selected_packages)
  return result


def load_indexed_world_data(
    world_root: Path,
    repo_root: Path,
    manifest: dict[str, Any],
    config: dict[str, Any],
    mini: bool = False,
    selected_packages: set[int] | None = None,
) -> WorldData:
  """Load the same indexed parsed model consumed by validation and lookup."""

  zone_paths = indexed_data_paths(world_root, "zon", mini)
  room_paths = indexed_data_paths(world_root, "wld", mini)
  mobile_paths = indexed_data_paths(world_root, "mob", mini)
  object_paths = indexed_data_paths(world_root, "obj", mini)
  trigger_paths = indexed_data_paths(world_root, "trg", mini)
  shop_paths = indexed_data_paths(world_root, "shp", mini)
  quest_paths = indexed_data_paths(world_root, "qst", mini)
  hlquest_paths = indexed_data_paths(world_root, "hlq", mini)
  if selected_packages is not None:
    for package in sorted(selected_packages):
      for extension, paths in (
          ("zon", zone_paths),
          ("wld", room_paths),
          ("mob", mobile_paths),
          ("obj", object_paths),
          ("trg", trigger_paths),
          ("shp", shop_paths),
          ("qst", quest_paths),
          ("hlq", hlquest_paths),
      ):
        candidate = world_root / extension / f"{package}.{extension}"
        if candidate.is_file() and candidate not in paths:
          paths.append(candidate)
  (
      zones,
      rooms,
      mobiles,
      objects,
      triggers,
      shops,
      quests,
      hlquests,
      findings,
      complete,
  ) = _load_files(
      zone_paths,
      room_paths,
      mobile_paths,
      object_paths,
      trigger_paths,
      shop_paths,
      quest_paths,
      hlquest_paths,
      repo_root,
      world_root,
      manifest,
      config,
      selected_packages,
      {"zone", "room", "mobile", "object", "trigger", "shop", "quest", "hlquest"},
  )
  return WorldData(
      zones=zones,
      rooms=rooms,
      mobiles=mobiles,
      objects=objects,
      triggers=triggers,
      shops=shops,
      quests=quests,
      hlquests=hlquests,
      findings=findings,
      complete=complete,
  )


def _collect_explicit_paths(requested_paths: Iterable[Path]) -> dict[str, list[Path]]:
  paths = {extension: [] for extension in DATA_EXTENSIONS}
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
      extension = candidate.suffix.removeprefix(".")
      if extension in paths:
        paths[extension].append(candidate)
  return paths


def validate_explicit_paths(
    requested_paths: list[Path],
    repo_root: Path,
    manifest: dict[str, Any],
    config: dict[str, Any],
) -> ValidationResult:
  paths = _collect_explicit_paths(requested_paths)
  result = ValidationResult("isolated-paths", "paths", config=dict(config))
  if not any(paths.values()):
    result.findings.append(
        finding(
            "IDX012",
            "error",
            "explicit paths contain no supported .zon, .wld, .mob, .obj, .shp, .trg, .qst, "
            "or .hlq files",
            SourceSpan("<paths>", 1),
        )
    )
    result.complete = False
    return result
  *_, findings, complete = _load_files(
      paths["zon"],
      paths["wld"],
      paths["mob"],
      paths["obj"],
      paths["trg"],
      paths["shp"],
      paths["qst"],
      paths["hlq"],
      repo_root,
      None,
      manifest,
      config,
      None,
      {
          {
              "zon": "zone",
              "wld": "room",
              "mob": "mobile",
              "obj": "object",
              "shp": "shop",
              "trg": "trigger",
              "qst": "quest",
              "hlq": "hlquest",
          }[
              extension
          ]
          for extension, selected_paths in paths.items()
          if selected_paths
      },
  )
  result.findings.extend(findings)
  result.complete = complete
  return result
