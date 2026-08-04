"""High-confidence semantic and topology checks for parsed world records."""

from __future__ import annotations

import re
from typing import Any, Iterable

from .constants import DECODER_ALPHABET
from .flags import decode_tokens
from .models import (
    ExitRecord,
    Finding,
    MobileRecord,
    ObjectRecord,
    RelatedLocation,
    RoomRecord,
    SourceSpan,
    TriggerRecord,
    WorldRecord,
    ZoneRecord,
)


_PLACEHOLDER_TEXT = {
    "an undefined string.",
    "an unfinished mob stands here.",
    "an unfinished object is lying here.",
    "an unfinished room",
    "it looks unfinished.",
    "mob unfinished",
    "the unfinished mob",
    "undefined",
    "unfinished object",
    "you are in an unfinished room.",
}


def strip_color_codes(value: str | None) -> str:
  """Mirror modify.c:strip_colors(), including escaped marker characters."""

  if value is None:
    return ""
  output: list[str] = []
  index = 0
  while index < len(value):
    character = value[index]
    if character in {"@", "\t"}:
      if index + 1 < len(value) and value[index + 1] == character:
        output.append(character)
      index += 2
      continue
    output.append(character)
    index += 1
  return "".join(output)


def _normalized_text(value: str | None) -> str:
  return " ".join(strip_color_codes(value).split()).casefold()


def _package_number(record: WorldRecord) -> int | None:
  try:
    return int(record.source_package)
  except ValueError:
    return None


def _selected(record: WorldRecord, selected_packages: set[int] | None) -> bool:
  return selected_packages is None or _package_number(record) in selected_packages


def _table_index(manifest: dict[str, Any], table_name: str, macro: str) -> int:
  for entry in manifest["tables"][table_name]["entries"]:
    if entry.get("macro") == macro or macro in entry.get("aliases", []):
      return int(entry["index"])
  raise ValueError(f"manifest table {table_name!r} has no macro {macro!r}")


def _limit(manifest: dict[str, Any], name: str) -> int:
  return int(manifest["limits"][name]["value"])


def _related_room(room: RoomRecord) -> RelatedLocation:
  return RelatedLocation(room.span.path, room.span.line, room.span.column, "room", room.vnum)


def _legacy_affect_tokens(tokens: list[str]) -> list[str]:
  converted: list[str] = []
  for token in tokens:
    if re.fullmatch(r"[+-]?\d+", token):
      converted.append(token)
      continue
    mask = 0
    for character in token:
      index = DECODER_ALPHABET.find(character)
      if index >= 0:
        mask |= 1 << (index + 1)
    converted.append(str(mask))
  return converted


def _decoded_bits(
    tokens: list[str],
    table: dict[str, Any],
    legacy_affect: bool = False,
) -> frozenset[int]:
  encoded = _legacy_affect_tokens(tokens) if legacy_affect else tokens
  return decode_tokens(encoded, len(table["entries"])).bits


def _reserved_indices(
    table: dict[str, Any],
    extra_macros: set[str] | None = None,
    excluded_macros: set[str] | None = None,
) -> dict[int, str]:
  extra = extra_macros or set()
  excluded = excluded_macros or set()
  reserved: dict[int, str] = {}
  for entry in table["entries"]:
    macro = entry.get("macro")
    if macro in excluded:
      continue
    if entry.get("reserved") or macro in extra:
      reserved[int(entry["index"])] = macro or entry["name"]
  return reserved


def _check_flag_tokens(
    findings: list[Finding],
    record: WorldRecord,
    record_type: str,
    label: str,
    tokens: list[str],
    table: dict[str, Any],
    reserved: dict[int, str],
    legacy_affect: bool = False,
) -> None:
  if not tokens:
    return
  bits = _decoded_bits(tokens, table, legacy_affect)
  for bit in sorted(bits & reserved.keys()):
    findings.append(
        Finding(
            "SEM003",
            "warning",
            f"prototype sets reserved or runtime-managed {label} flag "
            f"{reserved[bit]} (bit {bit})",
            record.span,
            record_type=record_type,
            vnum=record.vnum,
        )
    )


def _validate_reserved_flags(
    rooms: list[RoomRecord],
    mobiles: list[MobileRecord],
    objects: list[ObjectRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
) -> None:
  room_table = manifest["tables"]["room"]
  mob_table = manifest["tables"]["mob"]
  affect_table = manifest["tables"]["affect"]
  affect2_table = manifest["tables"]["affect2"]
  extra_table = manifest["tables"]["obj-extra"]
  wear_table = manifest["tables"]["obj-wear"]
  room_reserved = _reserved_indices(
      room_table,
      {"ROOM_HASTRAP", "ROOM_OCCUPIED"},
  )
  mob_reserved = _reserved_indices(mob_table)
  affect_reserved = _reserved_indices(
      affect_table,
      {"AFF_CHARM"},
      {"AFF_GROUP"},
  )
  affect2_reserved = _reserved_indices(affect2_table)

  for room in rooms:
    if _selected(room, selected_packages):
      _check_flag_tokens(
          findings, room, "room", "room", room.flags, room_table, room_reserved
      )
  for mobile in mobiles:
    if not _selected(mobile, selected_packages):
      continue
    _check_flag_tokens(
        findings,
        mobile,
        "mobile",
        "mobile action",
        mobile.action_flags,
        mob_table,
        mob_reserved,
    )
    _check_flag_tokens(
        findings,
        mobile,
        "mobile",
        "affect",
        mobile.affect_flags,
        affect_table,
        affect_reserved,
        mobile.legacy_affect_encoding,
    )
    _check_flag_tokens(
        findings,
        mobile,
        "mobile",
        "affect2",
        mobile.affect2_flags,
        affect2_table,
        affect2_reserved,
    )
  for obj in objects:
    if not _selected(obj, selected_packages):
      continue
    _check_flag_tokens(
        findings, obj, "object", "object extra", obj.extra_flags, extra_table, {}
    )
    _check_flag_tokens(
        findings, obj, "object", "object wear", obj.wear_flags, wear_table, {}
    )
    _check_flag_tokens(
        findings,
        obj,
        "object",
        "permanent affect",
        obj.affect_flags,
        affect_table,
        affect_reserved,
        obj.legacy_affect_encoding,
    )
    _check_flag_tokens(
        findings,
        obj,
        "object",
        "permanent affect2",
        obj.affect2_flags,
        affect2_table,
        affect2_reserved,
    )


def _text_fields(
    zones: Iterable[ZoneRecord],
    rooms: Iterable[RoomRecord],
    mobiles: Iterable[MobileRecord],
    objects: Iterable[ObjectRecord],
    triggers: Iterable[TriggerRecord],
) -> Iterable[tuple[WorldRecord, str, str | None]]:
  for zone in zones:
    yield zone, "name", zone.name
  for room in rooms:
    yield room, "name", room.name
    yield room, "description", room.description
  for mobile in mobiles:
    yield mobile, "aliases", mobile.aliases
    yield mobile, "short description", mobile.short_description
    yield mobile, "long description", mobile.long_description
    yield mobile, "description", mobile.description
  for obj in objects:
    yield obj, "aliases", obj.aliases
    yield obj, "short description", obj.short_description
    yield obj, "description", obj.description
  for trigger in triggers:
    yield trigger, "name", trigger.name


def _validate_placeholder_text(
    zones: list[ZoneRecord],
    rooms: list[RoomRecord],
    mobiles: list[MobileRecord],
    objects: list[ObjectRecord],
    triggers: list[TriggerRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
) -> None:
  for record, label, value in _text_fields(zones, rooms, mobiles, objects, triggers):
    if not _selected(record, selected_packages):
      continue
    normalized = _normalized_text(value)
    if normalized and normalized not in _PLACEHOLDER_TEXT:
      continue
    record_type = type(record).__name__.removesuffix("Record").lower()
    detail = "empty" if not normalized else f"known placeholder {normalized!r}"
    findings.append(
        Finding(
            "SEM004",
            "warning",
            f"{label} is {detail} after color codes are stripped",
            record.span,
            record_type=record_type,
            vnum=record.vnum,
        )
    )


def _effective_exits(room: RoomRecord, direction_count: int) -> dict[int, ExitRecord]:
  exits: dict[int, ExitRecord] = {}
  for exit_record in room.exits:
    if 0 <= exit_record.direction < direction_count:
      exits[exit_record.direction] = exit_record
  return exits


def _door_capable(exit_record: ExitRecord) -> bool:
  return exit_record.door_flags in {1, 2, 3, 4}


def _validate_exit_topology(
    rooms: list[RoomRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
    direction_count: int,
) -> dict[int, dict[int, ExitRecord]]:
  rooms_by_vnum = {room.vnum: room for room in rooms}
  exits_by_room = {
      room.vnum: _effective_exits(room, direction_count)
      for room in rooms
  }
  directions = manifest["tables"]["directions"]["entries"]
  reverse = {
      int(entry["index"]): int(entry["reverse_index"])
      for entry in directions[:direction_count]
  }

  for room in rooms:
    if not _selected(room, selected_packages):
      continue
    physical = {
        direction: exit_record
        for direction, exit_record in exits_by_room[room.vnum].items()
        if exit_record.destination_vnum >= 0
    }
    if not physical and room.moving_room is None and not room.moving_connections:
      findings.append(
          Finding(
              "SEM005",
              "warning",
              "room has no physical exits and is not a moving-room transport",
              room.span,
              record_type="room",
              vnum=room.vnum,
          )
      )
    for direction, exit_record in physical.items():
      destination = rooms_by_vnum.get(exit_record.destination_vnum)
      if destination is None:
        continue
      reverse_direction = reverse[direction]
      reverse_exit = exits_by_room[destination.vnum].get(reverse_direction)
      if reverse_exit is None or reverse_exit.destination_vnum != room.vnum:
        findings.append(
            Finding(
                "SEM006",
                "warning",
                f"physical exit direction {direction} to room {destination.vnum} has no "
                f"reverse direction {reverse_direction} back to room {room.vnum}",
                exit_record.span,
                record_type="room",
                vnum=room.vnum,
                related=_related_room(destination),
            )
        )
        continue
      if (room.vnum, direction) >= (destination.vnum, reverse_direction):
        continue
      if exit_record.key_vnum != reverse_exit.key_vnum:
        findings.append(
            Finding(
                "SEM007",
                "warning",
                f"reverse exits use different keys {exit_record.key_vnum} and "
                f"{reverse_exit.key_vnum}",
                exit_record.span,
                record_type="room",
                vnum=room.vnum,
                related=_related_room(destination),
            )
        )
      if _normalized_text(exit_record.keyword) != _normalized_text(reverse_exit.keyword):
        findings.append(
            Finding(
                "SEM008",
                "warning",
                "reverse exits use different keywords after color stripping",
                exit_record.span,
                record_type="room",
                vnum=room.vnum,
                related=_related_room(destination),
            )
        )
      if _door_capable(exit_record) != _door_capable(reverse_exit):
        findings.append(
            Finding(
                "SEM009",
                "warning",
                "reverse exits disagree about whether the connection is door-capable",
                exit_record.span,
                record_type="room",
                vnum=room.vnum,
                related=_related_room(destination),
            )
        )
  return exits_by_room


def _validate_reachability(
    zones: list[ZoneRecord],
    rooms: list[RoomRecord],
    exits_by_room: dict[int, dict[int, ExitRecord]],
    findings: list[Finding],
    selected_packages: set[int] | None,
) -> None:
  rooms_by_vnum = {room.vnum: room for room in rooms}
  rooms_by_zone: dict[int, list[RoomRecord]] = {}
  incoming_roots: dict[int, set[int]] = {}
  for room in rooms:
    if room.owner_zone_vnum is not None:
      rooms_by_zone.setdefault(room.owner_zone_vnum, []).append(room)
  for source in rooms:
    for exit_record in exits_by_room[source.vnum].values():
      destination = rooms_by_vnum.get(exit_record.destination_vnum)
      if destination is None or destination.owner_zone_vnum is None:
        continue
      if source.owner_zone_vnum != destination.owner_zone_vnum:
        incoming_roots.setdefault(destination.owner_zone_vnum, set()).add(destination.vnum)

  for zone in zones:
    if not _selected(zone, selected_packages):
      continue
    zone_rooms = rooms_by_zone.get(zone.vnum, [])
    if not zone_rooms:
      continue
    roots = sorted(incoming_roots.get(zone.vnum, set()))
    if not roots:
      roots = [min(room.vnum for room in zone_rooms)]
      findings.append(
          Finding(
              "SEM010",
              "info",
              f"zone has no incoming cross-zone exits; reachability uses fallback root "
              f"{roots[0]}",
              zone.span,
              record_type="zone",
              vnum=zone.vnum,
          )
      )
    zone_vnums = {room.vnum for room in zone_rooms}
    reachable = set(roots)
    pending = list(roots)
    while pending:
      current = pending.pop()
      for exit_record in exits_by_room[current].values():
        destination = exit_record.destination_vnum
        if destination in zone_vnums and destination not in reachable:
          reachable.add(destination)
          pending.append(destination)
    root_text = ", ".join(str(root) for root in roots)
    for room in sorted(zone_rooms, key=lambda item: item.vnum):
      if room.vnum not in reachable:
        findings.append(
            Finding(
                "SEM011",
                "warning",
                f"room is unreachable within zone {zone.vnum} from roots [{root_text}]",
                room.span,
                record_type="room",
                vnum=room.vnum,
            )
        )


def _validate_reset_levels(
    zones: list[ZoneRecord],
    rooms: list[RoomRecord],
    mobiles: list[MobileRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
) -> None:
  zones_by_vnum = {zone.vnum: zone for zone in zones}
  rooms_by_vnum = {room.vnum: room for room in rooms}
  mobiles_by_vnum = {mobile.vnum: mobile for mobile in mobiles}
  for reset_zone in zones:
    if not _selected(reset_zone, selected_packages):
      continue
    for command in reset_zone.commands:
      if command.command != "M" or len(command.arguments) < 3:
        continue
      if command.probability is not None and command.probability <= 0:
        continue
      mobile = mobiles_by_vnum.get(command.arguments[0])
      room = rooms_by_vnum.get(command.arguments[2])
      if mobile is None or room is None or room.owner_zone_vnum is None or mobile.level is None:
        continue
      destination_zone = zones_by_vnum.get(room.owner_zone_vnum)
      if (
          destination_zone is None
          or destination_zone.min_level is None
          or destination_zone.max_level is None
          or destination_zone.min_level < 0
          or destination_zone.max_level < destination_zone.min_level
      ):
        continue
      if destination_zone.min_level <= mobile.level <= destination_zone.max_level:
        continue
      findings.append(
          Finding(
              "SEM012",
              "warning",
              f"M reset loads level {mobile.level} mobile {mobile.vnum} into room {room.vnum}, "
              f"outside destination zone {destination_zone.vnum} level band "
              f"{destination_zone.min_level}..{destination_zone.max_level}",
              command.span,
              record_type="zone",
              vnum=reset_zone.vnum,
              related=RelatedLocation(
                  mobile.span.path,
                  mobile.span.line,
                  mobile.span.column,
                  "mobile",
                  mobile.vnum,
              ),
          )
      )


def _validate_dangerous_rooms(
    zones: list[ZoneRecord],
    rooms: list[RoomRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
) -> None:
  zones_by_vnum = {zone.vnum: zone for zone in zones}
  room_table = manifest["tables"]["room"]
  zone_table = manifest["tables"]["zone"]
  dangerous = {
      _table_index(manifest, "room", "ROOM_DEATH"): "ROOM_DEATH",
      _table_index(manifest, "room", "ROOM_STAFFROOM"): "ROOM_STAFFROOM",
  }
  closed_bit = _table_index(manifest, "zone", "ZONE_CLOSED")
  for room in rooms:
    if not _selected(room, selected_packages) or room.owner_zone_vnum is None:
      continue
    zone = zones_by_vnum.get(room.owner_zone_vnum)
    if zone is None:
      continue
    zone_bits = _decoded_bits(zone.flags, zone_table)
    if closed_bit in zone_bits:
      continue
    room_bits = _decoded_bits(room.flags, room_table)
    for bit in sorted(room_bits & dangerous.keys()):
      findings.append(
          Finding(
              "SEM013",
              "warning",
              f"{dangerous[bit]} is set in zone {zone.vnum}, which is otherwise open to mortals",
              room.span,
              record_type="room",
              vnum=room.vnum,
              related=RelatedLocation(
                  zone.span.path,
                  zone.span.line,
                  zone.span.column,
                  "zone",
                  zone.vnum,
              ),
          )
      )


def _object_finding(
    findings: list[Finding],
    obj: ObjectRecord,
    code: str,
    message: str,
    severity: str = "warning",
    span: SourceSpan | None = None,
) -> None:
  findings.append(
      Finding(
          code,
          severity,
          message,
          span or obj.span,
          record_type="object",
          vnum=obj.vnum,
      )
  )


def _valid_object_spell(value: int, maximum: int) -> bool:
  return value in {-1, 0} or 1 <= value <= maximum


def _check_object_spell(
    findings: list[Finding],
    obj: ObjectRecord,
    slot: int,
    maximum: int,
) -> None:
  value = obj.values[slot]
  if not _valid_object_spell(value, maximum):
    _object_finding(
        findings,
        obj,
        "SEM015",
        f"object value[{slot}] spell {value} is outside -1, 0, or 1..{maximum}",
        "error",
    )


def _validate_object_values(
    objects: list[ObjectRecord],
    rooms: list[RoomRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
    direction_count: int,
    exits_by_room: dict[int, dict[int, ExitRecord]],
) -> None:
  item_types = {
      entry["macro"]: int(entry["index"])
      for entry in manifest["tables"]["item-types"]["entries"]
      if entry.get("macro")
  }
  rooms_by_vnum = {room.vnum: room for room in rooms}
  max_level = _limit(manifest, "LVL_IMPL")
  num_spells = _limit(manifest, "NUM_SPELLS")
  defined_spell_max = num_spells - 1
  num_liquids = _limit(manifest, "NUM_LIQ_TYPES")
  num_trap_triggers = _limit(manifest, "NUM_TRAP_TRIGGERS")
  num_trap_effects = _limit(manifest, "NUM_TRAP_SPECIAL_EFFECTS")
  num_portal_types = _limit(manifest, "NUM_PORTAL_TYPES")
  max_container_size = _limit(manifest, "MAX_CONTAINER_SIZE")
  max_people = _limit(manifest, "MAX_PEOPLE")
  max_activated_uses = _limit(manifest, "MAX_NUMBER_OF_ACTIVATED_SPELL_USES")
  portal_random = _limit(manifest, "PORTAL_RANDOM")
  portal_clanhall = _limit(manifest, "PORTAL_CLANHALL")
  portal_exact = {
      _limit(manifest, "PORTAL_NORMAL"),
      _limit(manifest, "PORTAL_CHECKFLAGS"),
  }

  for obj in objects:
    if not _selected(obj, selected_packages) or obj.item_type is None or len(obj.values) < 4:
      continue
    values = obj.values
    if obj.item_type in {item_types["ITEM_SCROLL"], item_types["ITEM_POTION"]}:
      if not 0 <= values[0] <= max_level:
        _object_finding(
            findings,
            obj,
            "SEM014",
            f"object value[0] spell level {values[0]} is outside 0..{max_level}",
        )
      for slot in (1, 2, 3):
        _check_object_spell(findings, obj, slot, defined_spell_max)
    elif obj.item_type in {item_types["ITEM_WAND"], item_types["ITEM_STAFF"]}:
      if not 0 <= values[0] <= max_level:
        _object_finding(
            findings,
            obj,
            "SEM014",
            f"object value[0] spell level {values[0]} is outside 0..{max_level}",
        )
      _check_object_spell(findings, obj, 3, defined_spell_max)
      if values[1] < 0 or values[2] < 0 or values[2] > values[1]:
        _object_finding(
            findings,
            obj,
            "SEM017",
            f"wand/staff charges use current {values[2]} and maximum {values[1]}; "
            "both must be non-negative and current must not exceed maximum",
        )
    elif obj.item_type in {item_types["ITEM_DRINKCON"], item_types["ITEM_FOUNTAIN"]}:
      if values[0] < -1 or values[1] < 0 or (values[0] >= 0 and values[1] > values[0]):
        _object_finding(
            findings,
            obj,
            "SEM016",
            f"liquid values use capacity {values[0]} and current units {values[1]}; capacity "
            "must be -1 or non-negative and current must fit a finite capacity",
        )
      if not 0 <= values[2] < num_liquids:
        _object_finding(
            findings,
            obj,
            "SEM018",
            f"liquid type {values[2]} is outside 0..{num_liquids - 1}",
            "error",
        )
      _check_object_spell(findings, obj, 3, defined_spell_max)
    elif obj.item_type in {item_types["ITEM_CONTAINER"], item_types["ITEM_AMMO_POUCH"]}:
      if not -1 <= values[0] <= max_container_size:
        _object_finding(
            findings,
            obj,
            "SEM018",
            f"container capacity {values[0]} is outside -1..{max_container_size}",
        )
      valid_mask = (1 << _limit(manifest, "NUM_CONT_FLAGS")) - 1
      if values[1] < 0 or values[1] & ~valid_mask:
        _object_finding(
            findings,
            obj,
            "SEM018",
            f"container flag mask {values[1]} contains bits outside 0x{valid_mask:x}",
        )
    elif obj.item_type == item_types["ITEM_FURNITURE"]:
      if values[0] < 0 or values[0] > max_people or values[1] < 0 or values[1] > values[0]:
        _object_finding(
            findings,
            obj,
            "SEM016",
            f"furniture uses capacity {values[0]} and current occupants {values[1]}; "
            f"capacity must be 0..{max_people} and current must fit",
        )
    elif obj.item_type == item_types["ITEM_LIGHT"]:
      if values[2] < -1:
        _object_finding(
            findings,
            obj,
            "SEM018",
            f"light burn hours {values[2]} must be -1 or non-negative",
        )
    elif obj.item_type == item_types["ITEM_PORTAL"]:
      if not 0 <= values[0] < num_portal_types:
        _object_finding(
            findings,
            obj,
            "SEM018",
            f"portal type {values[0]} is outside 0..{num_portal_types - 1}",
            "error",
        )
      elif values[0] in portal_exact and values[1] <= 0:
        _object_finding(
            findings, obj, "SEM018", "portal destination must be a positive room vnum"
        )
      elif values[0] == portal_random:
        if values[1] <= 0 or values[2] <= 0 or values[1] > values[2]:
          _object_finding(
              findings,
              obj,
              "SEM020",
              f"random portal range {values[1]}..{values[2]} must be positive and ordered",
          )
      elif values[0] == portal_clanhall:
        pass
    elif obj.item_type == item_types["ITEM_TRAP"]:
      if not 0 <= values[0] < num_trap_triggers:
        _object_finding(
            findings,
            obj,
            "SEM018",
            f"trap trigger {values[0]} is outside 0..{num_trap_triggers - 1}",
            "error",
        )
      elif values[0] in {1, 2} and not 0 <= values[1] < direction_count:
        _object_finding(
            findings,
            obj,
            "SEM018",
            f"door trap direction {values[1]} is outside 0..{direction_count - 1}",
            "error",
        )
      elif values[0] in {3, 4, 5} and values[1] <= 0:
        _object_finding(
            findings, obj, "SEM018", "container trap target must be a positive object vnum"
        )
      if not 1 <= values[2] < num_trap_effects:
        _object_finding(
            findings,
            obj,
            "SEM018",
            f"trap effect {values[2]} is outside 1..{num_trap_effects - 1}",
            "error",
        )
    elif obj.item_type == item_types["ITEM_SWITCH"]:
      if values[0] not in {0, 1}:
        _object_finding(
            findings, obj, "SEM018", f"switch activation {values[0]} must be 0 or 1"
        )
      if not 0 <= values[2] < direction_count:
        _object_finding(
            findings,
            obj,
            "SEM019",
            f"switch direction {values[2]} is outside 0..{direction_count - 1}",
            "error",
        )
      if values[3] not in {0, 1, 2}:
        _object_finding(
            findings, obj, "SEM018", f"switch action {values[3]} is outside 0..2"
        )
      target = rooms_by_vnum.get(values[1])
      if target is not None and 0 <= values[2] < direction_count:
        if values[2] not in exits_by_room[target.vnum]:
          _object_finding(
              findings,
              obj,
              "SEM019",
              f"switch targets direction {values[2]} in room {target.vnum}, but no exit exists",
          )

    for payload, span in obj.weapon_spells:
      spell, level, percent, in_combat = payload
      if not (
          1 <= spell <= defined_spell_max
          and level >= 1
          and 1 <= percent <= 50
          and in_combat in {0, 1}
      ):
        _object_finding(
            findings,
            obj,
            "SEM021",
            f"weapon spell values {payload} require spell 1..{defined_spell_max}, level >= 1, "
            "percent 1..50, and combat 0 or 1",
            span=span,
        )
    for payload, span in obj.activated_spells:
      level, spell, current, maximum, cooldown = payload
      if not (
          1 <= level <= 30
          and 1 <= spell < num_spells
          and 0 <= current <= maximum
          and 1 <= maximum <= max_activated_uses
          and cooldown >= 0
      ):
        _object_finding(
            findings,
            obj,
            "SEM022",
            f"activated spell values {payload} require level 1..30, spell "
            f"1..{num_spells - 1}, current 0..maximum, maximum "
            f"1..{max_activated_uses}, and non-negative cooldown",
            span=span,
        )


def validate_semantics(
    zones: list[ZoneRecord],
    rooms: list[RoomRecord],
    mobiles: list[MobileRecord],
    objects: list[ObjectRecord],
    triggers: list[TriggerRecord],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
    direction_count: int,
) -> list[Finding]:
  """Run only source-backed semantic checks over an already validated graph."""

  findings: list[Finding] = []
  _validate_reserved_flags(
      rooms, mobiles, objects, findings, selected_packages, manifest
  )
  _validate_placeholder_text(
      zones, rooms, mobiles, objects, triggers, findings, selected_packages
  )
  exits_by_room = _validate_exit_topology(
      rooms, findings, selected_packages, manifest, direction_count
  )
  _validate_reachability(
      zones, rooms, exits_by_room, findings, selected_packages
  )
  _validate_reset_levels(
      zones, rooms, mobiles, findings, selected_packages
  )
  _validate_dangerous_rooms(
      zones, rooms, findings, selected_packages, manifest
  )
  _validate_object_values(
      objects,
      rooms,
      findings,
      selected_packages,
      manifest,
      direction_count,
      exits_by_room,
  )
  return findings
