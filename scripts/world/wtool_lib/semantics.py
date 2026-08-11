"""High-confidence semantic and topology checks for parsed world records."""

from __future__ import annotations

import re
from typing import Any, Iterable

from .constants import DECODER_ALPHABET
from .flags import decode_tokens
from .models import (
  ExitRecord,
  Finding,
  HlQuestCommandRecord,
  HlQuestRecord,
  MobileRecord,
  ObjectRecord,
  QuestRecord,
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
  # write_mobile_record() persists MOB_ISNPC on every canonical mobile, while
  # MOB_SPEC and MOB_PLANAR_ALLY are usable behavior flags rather than (R)
  # prototype-state bits. MOB_NOTDEADYET and true UNUSED entries remain linted.
  mob_reserved = _reserved_indices(
      mob_table,
      excluded_macros={"MOB_ISNPC", "MOB_PLANAR_ALLY", "MOB_SPEC"},
  )
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


def _validate_room_level_ranges(
    rooms: list[RoomRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
) -> None:
  maximum = _limit(manifest, "LVL_IMPL")
  for room in rooms:
    if not _selected(room, selected_packages):
      continue
    minimum_level = room.minimum_level
    maximum_level = room.maximum_level
    invalid_minimum = minimum_level != -1 and not 1 <= minimum_level <= maximum
    invalid_maximum = maximum_level != -1 and not 1 <= maximum_level <= maximum
    reversed_range = (
        minimum_level > 0 and maximum_level > 0 and minimum_level > maximum_level
    )
    if invalid_minimum or invalid_maximum or reversed_range:
      findings.append(
          Finding(
              "SEM034",
              "error",
              f"room level range {minimum_level}..{maximum_level} must use -1 or "
              f"1..{maximum}, with the finite bounds ordered",
              room.span,
              record_type="room",
              vnum=room.vnum,
          )
      )


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


def _quest_finding(
    findings: list[Finding],
    quest: QuestRecord,
    code: str,
    message: str,
    severity: str = "error",
    field_name: str | None = None,
    related: RelatedLocation | None = None,
) -> None:
  findings.append(
      Finding(
          code,
          severity,
          message,
          quest.field_spans.get(field_name, quest.span) if field_name else quest.span,
          record_type="quest",
          vnum=quest.vnum,
          related=related,
      )
  )


def _hlquest_finding(
    findings: list[Finding],
    quest: HlQuestRecord,
    code: str,
    message: str,
    severity: str = "error",
    span: SourceSpan | None = None,
) -> None:
  findings.append(
      Finding(
          code,
          severity,
          message,
          span or quest.span,
          record_type="hlquest",
          vnum=quest.vnum,
      )
  )


def _encoded_length(value: str) -> int:
  return len(value.encode("utf-8", errors="surrogateescape"))


def _raw_quest_value(quest: QuestRecord, field_name: str, default: int = -1) -> int:
  value = quest.raw_values.get(field_name)
  if value is not None:
    return value
  normalized = getattr(quest, field_name)
  return normalized if normalized is not None else default


def _quest_type_values(manifest: dict[str, Any]) -> dict[str, int]:
  return {
      entry["macro"]: int(entry["index"])
      for entry in manifest["tables"]["quest-types"]["entries"]
      if entry.get("macro")
  }


def _validate_quest_scalars(
    quests: list[QuestRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
) -> None:
  quest_types = _quest_type_values(manifest)
  valid_types = set(quest_types.values())
  undefined_type = _limit(manifest, "AQ_UNDEFINED")
  valid_types.add(undefined_type)
  max_level = _limit(manifest, "LVL_IMPL")
  mission_difficulties = _limit(manifest, "NUM_MISSION_DIFFICULTIES")
  valid_races = {
      _limit(manifest, "RACE_UNDEFINED"),
      _limit(manifest, "RACE_LICH"),
      _limit(manifest, "RACE_VAMPIRE"),
  }
  string_limits = (
      ("name", "name", _limit(manifest, "MAX_QUEST_NAME")),
      ("description", "description", _limit(manifest, "MAX_QUEST_DESC")),
      ("accept_message", "accept message", _limit(manifest, "MAX_QUEST_MSG")),
      (
          "completion_message",
          "completion message",
          _limit(manifest, "MAX_QUEST_MSG"),
      ),
      ("quit_message", "quit message", _limit(manifest, "MAX_QUEST_MSG")),
  )
  scalar_limits = (
      ("quantity", "quantity", 1, 50),
      ("points", "completion points", 0, 999999),
      ("quit_penalty", "quit penalty", 0, 999999),
      ("min_level", "minimum level", 0, max_level),
      ("max_level", "maximum level", 0, max_level),
      ("time_limit", "time limit", -1, 100),
      ("gold_reward", "gold reward", 0, 99999),
      ("experience_reward", "experience reward", 0, 999999),
  )
  typed_targets = {
      quest_types["AQ_OBJ_FIND"],
      quest_types["AQ_OBJ_RETURN"],
      quest_types["AQ_ROOM_FIND"],
      quest_types["AQ_ROOM_CLEAR"],
      quest_types["AQ_MOB_FIND"],
      quest_types["AQ_MOB_KILL"],
      quest_types["AQ_MOB_SAVE"],
      quest_types["AQ_DIALOGUE"],
  }

  for quest in quests:
    if not quest.complete or not _selected(quest, selected_packages):
      continue
    if quest.quest_type not in valid_types:
      _quest_finding(
          findings,
          quest,
          "SEM023",
          f"quest type {quest.quest_type} is outside the source-defined type table",
          field_name="quest_type",
      )
    if quest.quest_type == undefined_type:
      continue

    for field_name, label, limit in string_limits:
      value = getattr(quest, field_name)
      if value is not None and _encoded_length(value) >= limit:
        _quest_finding(
            findings,
            quest,
            "SEM024",
            f"quest {label} is {_encoded_length(value)} bytes; QEDIT stores at most "
            f"{limit - 1}",
            "warning",
            field_name,
        )

    for field_name, label, minimum, maximum in scalar_limits:
      value = getattr(quest, field_name)
      if value is not None and not minimum <= value <= maximum:
        _quest_finding(
            findings,
            quest,
            "SEM025",
            f"quest {label} {value} is outside {minimum}..{maximum}",
            field_name=field_name,
        )
    if (
        quest.min_level is not None
        and quest.max_level is not None
        and quest.min_level > quest.max_level
    ):
      _quest_finding(
          findings,
          quest,
          "SEM025",
          f"minimum level {quest.min_level} exceeds maximum level {quest.max_level}",
          field_name="min_level",
      )
    if quest.questmaster_vnum is None:
      _quest_finding(
          findings,
          quest,
          "SEM026",
          "quest has no questmaster mobile; players cannot discover or join it normally",
          field_name="questmaster_vnum",
      )
    if quest.quest_type in typed_targets and quest.target is None:
      _quest_finding(
          findings,
          quest,
          "SEM026",
          "quest type requires a typed target, but the target uses the -1 sentinel",
          field_name="target",
      )
    if quest.quest_type in {
        quest_types["AQ_OBJ_RETURN"],
        quest_types["AQ_GIVE_GOLD"],
    } and quest.return_mobile_vnum is None:
      _quest_finding(
          findings,
          quest,
          "SEM026",
          "quest type requires a return-recipient mobile",
          field_name="return_mobile_vnum",
      )

    if quest.quest_type == quest_types["AQ_COMPLETE_MISSION"]:
      target = _raw_quest_value(quest, "target")
      if not 0 <= target < mission_difficulties:
        _quest_finding(
            findings,
            quest,
            "SEM026",
            f"mission difficulty {target} is outside 0..{mission_difficulties - 1}",
            field_name="target",
        )
    elif quest.quest_type == quest_types["AQ_WILD_FIND"]:
      if quest.wilderness_x is None or quest.wilderness_y is None:
        _quest_finding(
            findings,
            quest,
            "SEM026",
            "wilderness quest requires both persisted coordinate fields",
            field_name=(
                "wilderness_x" if quest.wilderness_x is None else "wilderness_y"
            ),
        )
    elif quest.quest_type == quest_types["AQ_GIVE_GOLD"]:
      target = _raw_quest_value(quest, "target")
      if target < 0:
        _quest_finding(
            findings,
            quest,
            "SEM026",
            f"gold threshold {target} must be non-negative",
            field_name="target",
        )
    elif quest.quest_type == quest_types["AQ_MOB_MULTI_KILL"]:
      _quest_finding(
          findings,
          quest,
          "SEM026",
          "Luminari QST files cannot persist the mobile list required by "
          "AQ_MOB_MULTI_KILL",
          field_name="quest_type",
      )

    if "race_reward" in quest.raw_values:
      race = quest.raw_values["race_reward"]
      if race not in valid_races:
        _quest_finding(
            findings,
            quest,
            "SEM026",
            f"race reward {race} must be -1, RACE_LICH, or RACE_VAMPIRE",
            field_name="race_reward",
        )

    dialogue_values = {
        field_name: _raw_quest_value(quest, field_name)
        for field_name in ("diplomacy_dc", "intimidate_dc", "bluff_dc")
    }
    for field_name, value in dialogue_values.items():
      if not -1 <= value <= 100:
        _quest_finding(
            findings,
            quest,
            "SEM027",
            f"{field_name.replace('_', ' ')} {value} is outside -1..100",
            field_name=field_name,
        )
    if quest.quest_type == quest_types["AQ_DIALOGUE"]:
      if not any(value > 0 for value in dialogue_values.values()):
        _quest_finding(
            findings,
            quest,
            "SEM027",
            "dialogue quest has no positive skill DC and cannot be completed by a "
            "dialogue command",
            field_name="quest_type",
        )
    else:
      configured_dcs = [value for value in dialogue_values.values() if value != -1]
      if configured_dcs:
        _quest_finding(
            findings,
            quest,
            "SEM027",
            "non-dialogue quest persists dialogue DCs that the runtime ignores",
            "warning",
            "diplomacy_dc",
        )
      if quest.dialogue_alternative_quest_vnum is not None:
        _quest_finding(
            findings,
            quest,
            "SEM027",
            "non-dialogue quest defines an alternative quest; the target becomes an "
            "unjoinable dialogue alternative",
            field_name="dialogue_alternative_quest_vnum",
        )


def _quest_cycles(
    quests_by_vnum: dict[int, QuestRecord], field_name: str
) -> list[list[int]]:
  """Return functional-graph cycles in linear time."""

  visited: set[int] = set()
  cycles: list[list[int]] = []
  for start in sorted(quests_by_vnum):
    if start in visited:
      continue
    trail: list[int] = []
    positions: dict[int, int] = {}
    current: int | None = start
    while current is not None and current in quests_by_vnum and current not in visited:
      if current in positions:
        cycles.append(trail[positions[current] :])
        break
      positions[current] = len(trail)
      trail.append(current)
      current = getattr(quests_by_vnum[current], field_name)
    visited.update(trail)
  return cycles


def _validate_quest_topology(
    quests: list[QuestRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
) -> None:
  quests_by_vnum: dict[int, QuestRecord] = {}
  undefined_type = _limit(manifest, "AQ_UNDEFINED")
  for quest in quests:
    if (
        quest.complete
        and quest.quest_type != undefined_type
        and quest.vnum not in quests_by_vnum
    ):
      quests_by_vnum[quest.vnum] = quest
  dialogue_type = _quest_type_values(manifest)["AQ_DIALOGUE"]

  for quest in quests:
    if (
        not quest.complete
        or quest.quest_type == undefined_type
        or not _selected(quest, selected_packages)
    ):
      continue
    for field_name, label in (
        ("previous_quest_vnum", "previous"),
        ("next_quest_vnum", "next"),
    ):
      target_vnum = getattr(quest, field_name)
      if target_vnum is None:
        continue
      if target_vnum == quest.vnum:
        severity = "error" if field_name == "previous_quest_vnum" else "warning"
        impact = (
            "prevents a new player from joining the quest"
            if field_name == "previous_quest_vnum"
            else "is explicitly ignored when the quest completes"
        )
        _quest_finding(
            findings,
            quest,
            "SEM028",
            f"quest has a self-referential {label} link that {impact}",
            severity,
            field_name,
        )
        continue
      target = quests_by_vnum.get(target_vnum)
      if target is None:
        continue
      reciprocal_field = (
          "next_quest_vnum" if field_name == "previous_quest_vnum" else "previous_quest_vnum"
      )
      if getattr(target, reciprocal_field) != quest.vnum:
        _quest_finding(
            findings,
            quest,
            "SEM028",
            f"quest {label} link to {target_vnum} is not reciprocated by "
            f"{reciprocal_field.replace('_quest_vnum', '')}",
            "warning",
            field_name,
            RelatedLocation(
                target.span.path,
                target.span.line,
                target.span.column,
                "quest",
                target.vnum,
            ),
        )

    alternative = quest.dialogue_alternative_quest_vnum
    if alternative is None or quest.quest_type != dialogue_type:
      continue
    if alternative == quest.vnum:
      _quest_finding(
          findings,
          quest,
          "SEM027",
          "dialogue quest cannot use itself as its alternative quest",
          field_name="dialogue_alternative_quest_vnum",
      )
      continue
    target = quests_by_vnum.get(alternative)
    if target is not None and target.previous_quest_vnum != quest.vnum:
      _quest_finding(
          findings,
          quest,
          "SEM027",
          f"dialogue alternative {alternative} must name quest {quest.vnum} as its "
          "previous quest",
          field_name="dialogue_alternative_quest_vnum",
          related=RelatedLocation(
              target.span.path,
              target.span.line,
              target.span.column,
              "quest",
              target.vnum,
          ),
      )

  for field_name, label, severity in (
      ("previous_quest_vnum", "previous", "error"),
      ("next_quest_vnum", "next", "warning"),
  ):
    for cycle in _quest_cycles(quests_by_vnum, field_name):
      if len(cycle) < 2:
        continue
      selected = [
          quests_by_vnum[vnum]
          for vnum in cycle
          if _selected(quests_by_vnum[vnum], selected_packages)
      ]
      if not selected:
        continue
      owner = min(selected, key=lambda record: record.vnum)
      ordered = cycle[cycle.index(owner.vnum) :] + cycle[: cycle.index(owner.vnum)]
      path = " -> ".join(str(vnum) for vnum in (*ordered, ordered[0]))
      impact = (
          "blocks entry into the cycle until an impossible prerequisite is completed"
          if field_name == "previous_quest_vnum"
          else "can reinstall an earlier stage until completion history breaks the loop"
      )
      _quest_finding(
          findings,
          owner,
          "SEM029",
          f"{label}-quest cycle {path} {impact}",
          severity,
          field_name,
      )


def _hlquest_command_context(entry_ordinal: int, command: HlQuestCommandRecord) -> str:
  return (
      f"entry {entry_ordinal} {command.direction or command.direction_marker} "
      f"command {command.physical_ordinal}"
  )


def _validate_hlquest_semantics(
    hlquests: list[HlQuestRecord],
    rooms: list[RoomRecord],
    findings: list[Finding],
    selected_packages: set[int] | None,
    manifest: dict[str, Any],
    direction_count: int,
    exits_by_room: dict[int, dict[int, ExitRecord]],
) -> None:
  entry_types = {
      entry["macro"]: int(entry["index"])
      for entry in manifest["tables"]["hlquest-entry-types"]["entries"]
  }
  command_types = {
      entry["macro"]: int(entry["index"])
      for entry in manifest["tables"]["hlquest-commands"]["entries"]
  }
  valid_entry_types = set(entry_types.values())
  valid_command_types = set(command_types.values())
  coin_type = command_types["QUEST_COMMAND_COINS"]
  quest_points_type = command_types["QUEST_COMMAND_QUEST_POINTS"]
  experience_type = command_types["QUEST_COMMAND_EXPERIENCE"]
  item_type = command_types["QUEST_COMMAND_ITEM"]
  load_types = {
      command_types["QUEST_COMMAND_LOAD_OBJECT_INROOM"],
      command_types["QUEST_COMMAND_LOAD_MOB_INROOM"],
  }
  parameter_free = {
      command_types["QUEST_COMMAND_ATTACK_QUESTOR"],
      command_types["QUEST_COMMAND_DISAPPEAR"],
      command_types["QUEST_COMMAND_FOLLOW"],
  }
  spell_types = {
      command_types["QUEST_COMMAND_TEACH_SPELL"],
      command_types["QUEST_COMMAND_CAST_SPELL"],
  }
  open_door_type = command_types["QUEST_COMMAND_OPEN_DOOR"]
  kit_type = command_types["QUEST_COMMAND_KIT"]
  church_type = command_types["QUEST_COMMAND_CHURCH"]
  location_unused = {
      coin_type,
      quest_points_type,
      experience_type,
      item_type,
      church_type,
      *spell_types,
  }
  num_spells = _limit(manifest, "NUM_SPELLS")
  reserved_spell = _limit(manifest, "SPELL_RESERVED_DBC")
  num_classes = _limit(manifest, "NUM_CLASSES")
  num_churches = _limit(manifest, "NUM_CHURCHES")
  rooms_by_vnum = {room.vnum: room for room in rooms}

  for quest in hlquests:
    if not quest.complete or not _selected(quest, selected_packages):
      continue
    for entry in quest.entries:
      if not entry.complete:
        continue
      if entry.entry_type not in valid_entry_types:
        _hlquest_finding(
            findings,
            quest,
            "SEM030",
            f"entry {entry.physical_ordinal} has invalid type {entry.entry_type}",
            span=entry.span,
        )
        continue
      if entry.entry_type == entry_types["QUEST_ASK"]:
        if not entry.keywords or not entry.keywords.strip():
          _hlquest_finding(
              findings,
              quest,
              "SEM030",
              f"ASK entry {entry.physical_ordinal} requires non-empty keywords",
              span=entry.field_spans.get("keywords", entry.span),
          )
        if not entry.reply_message or not entry.reply_message.strip():
          _hlquest_finding(
              findings,
              quest,
              "SEM030",
              f"ASK entry {entry.physical_ordinal} requires a non-empty reply",
              span=entry.field_spans.get("reply_message", entry.span),
          )
        if entry.commands or entry.chain_terminated:
          _hlquest_finding(
              findings,
              quest,
              "SEM030",
              f"ASK entry {entry.physical_ordinal} cannot contain a command chain",
              span=entry.span,
          )
      else:
        if not entry.reply_message or not entry.reply_message.strip():
          _hlquest_finding(
              findings,
              quest,
              "SEM030",
              f"entry {entry.physical_ordinal} requires a non-empty reply",
              span=entry.field_spans.get("reply_message", entry.span),
          )
        if not entry.chain_terminated:
          _hlquest_finding(
              findings,
              quest,
              "SEM030",
              f"entry {entry.physical_ordinal} requires an S command-chain terminator",
              span=entry.span,
          )
        if (
            entry.entry_type == entry_types["QUEST_ROOM"]
            and entry.room_vnum is None
        ):
          _hlquest_finding(
              findings,
              quest,
              "SEM030",
              f"ROOM entry {entry.physical_ordinal} requires a room VNUM",
              span=entry.field_spans.get("room_vnum", entry.span),
          )

      for command in entry.commands:
        if not command.complete or not command.effective:
          continue
        context = _hlquest_command_context(entry.physical_ordinal, command)
        command_type = command.command_type
        if command_type not in valid_command_types:
          _hlquest_finding(
              findings,
              quest,
              "SEM031",
              f"{context} has invalid command type {command_type}",
              span=command.field_spans.get("code", command.span),
          )
          continue
        if command.direction == "input" and (
            entry.entry_type != entry_types["QUEST_GIVE"]
            or command_type not in {coin_type, item_type}
        ):
          _hlquest_finding(
              findings,
              quest,
              "SEM031",
              f"{context} is ignored; only GIVE-entry COINS and ITEM inputs are "
              "consumed by the runtime",
              span=command.field_spans.get("direction", command.span),
          )
        elif command.direction not in {"input", "output"}:
          _hlquest_finding(
              findings,
              quest,
              "SEM031",
              f"{context} has invalid command direction {command.direction_marker!r}",
              span=command.field_spans.get("direction", command.span),
          )

        if command.value is None or command.location is None:
          continue
        value = command.value
        location = command.location
        value_span = command.field_spans.get("value", command.span)
        location_span = command.field_spans.get("location", command.span)
        if command_type == coin_type:
          if value < 0:
            _hlquest_finding(
                findings,
                quest,
                "SEM032",
                f"{context} coin value {value} must be non-negative",
                span=value_span,
            )
          elif value > 2140000000:
            _hlquest_finding(
                findings,
                quest,
                "SEM032",
                f"{context} coin value {value} exceeds MAX_GOLD 2140000000",
                span=value_span,
            )
        elif command_type == quest_points_type and not -100000000 <= value <= 100000000:
          _hlquest_finding(
              findings,
              quest,
              "SEM032",
              f"{context} quest-point delta {value} is outside -100000000..100000000",
              span=value_span,
          )
        elif command_type == experience_type and not 0 <= value <= 2140000000:
          _hlquest_finding(
              findings,
              quest,
              "SEM032",
              f"{context} experience value {value} is outside 0..2140000000",
              span=value_span,
          )
        elif command_type in spell_types and not reserved_spell < value < num_spells:
          _hlquest_finding(
              findings,
              quest,
              "SEM032",
              f"{context} spell/skill {value} is outside the runtime-safe range "
              f"{reserved_spell + 1}..{num_spells - 1}",
              span=value_span,
          )
        elif command_type == open_door_type:
          if not 0 <= value < direction_count:
            _hlquest_finding(
                findings,
                quest,
                "SEM032",
                f"{context} direction {value} is outside 0..{direction_count - 1}",
                span=value_span,
            )
          elif location in rooms_by_vnum and value not in exits_by_room.get(location, {}):
            _hlquest_finding(
                findings,
                quest,
                "SEM032",
                f"{context} targets direction {value} in room {location}, but no exit exists",
                span=value_span,
            )
        elif command_type == kit_type and value != 9999 and location != 9999:
          if not 0 <= value < num_classes:
            _hlquest_finding(
                findings,
                quest,
                "SEM032",
                f"{context} target class {value} is outside 0..{num_classes - 1}",
                span=value_span,
            )
          if not 0 <= location < num_classes:
            _hlquest_finding(
                findings,
                quest,
                "SEM032",
                f"{context} prerequisite class {location} is outside "
                f"0..{num_classes - 1}",
                span=location_span,
            )
        elif command_type == church_type and not 0 <= value < num_churches:
          _hlquest_finding(
              findings,
              quest,
              "SEM032",
              f"{context} church {value} is outside 0..{num_churches - 1}",
              span=value_span,
          )
        elif command_type in load_types and location < 0:
          _hlquest_finding(
              findings,
              quest,
              "SEM032",
              f"{context} load location {location} must be 0 or a room VNUM",
              span=location_span,
          )

        if command_type in parameter_free and (value != 0 or location != 0):
          _hlquest_finding(
              findings,
              quest,
              "SEM033",
              f"{context} persists unused parameters {value} and {location}; the "
              "canonical writer emits zeroes",
              "warning",
              command.span,
          )
        elif command_type in location_unused and location != 0:
          _hlquest_finding(
              findings,
              quest,
              "SEM033",
              f"{context} persists unused location parameter {location}; the canonical "
              "writer emits zero",
              "warning",
              location_span,
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
  max_spell = _limit(manifest, "MAX_SPELLS")
  num_spells = _limit(manifest, "NUM_SPELLS")
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
  rol_trap_bit = _table_index(manifest, "obj-extra", "ITEM_TRAPPED")
  rol_trap_damage_types = {*range(1, 8), *range(11, 17), 30, 31}

  for obj in objects:
    if not _selected(obj, selected_packages) or obj.item_type is None or len(obj.values) < 4:
      continue
    values = obj.values
    extra_bits = _decoded_bits(obj.extra_flags, manifest["tables"]["obj-extra"])
    if rol_trap_bit in extra_bits:
      trap_valid = len(values) >= 16
      if trap_valid:
        effect, damage_type, charges, trap_level, dice_count, dice_size = values[10:16]
        trap_valid = (
            effect > 0
            and effect & ~0xFFF == 0
            and damage_type in rol_trap_damage_types
            and -1 <= charges <= 32767
            and 0 <= trap_level <= 100
            and 0 <= dice_count <= 32767
            and 0 <= dice_size <= 32767
            and bool(dice_count) == bool(dice_size)
        )
      if not trap_valid:
        payload = values[10:16] if len(values) >= 16 else values[10:]
        _object_finding(
            findings,
            obj,
            "SEM035",
            "ITEM_TRAPPED requires values[10..15] as effect 1..4095, supported "
            "damage type, charges -1..32767, level 0..100, and either 0d0 or "
            f"positive dice; found {payload}",
            "error",
        )
    if obj.item_type in {item_types["ITEM_SCROLL"], item_types["ITEM_POTION"]}:
      if not 0 <= values[0] <= max_level:
        _object_finding(
            findings,
            obj,
            "SEM014",
            f"object value[0] spell level {values[0]} is outside 0..{max_level}",
        )
      for slot in (1, 2, 3):
        _check_object_spell(findings, obj, slot, max_spell)
    elif obj.item_type in {item_types["ITEM_WAND"], item_types["ITEM_STAFF"]}:
      if not 0 <= values[0] <= max_level:
        _object_finding(
            findings,
            obj,
            "SEM014",
            f"object value[0] spell level {values[0]} is outside 0..{max_level}",
        )
      _check_object_spell(findings, obj, 3, max_spell)
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
      _check_object_spell(findings, obj, 3, max_spell)
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
          1 <= spell <= max_spell
          and level >= 1
          and 1 <= percent <= 50
          and in_combat in {0, 1}
      ):
        _object_finding(
            findings,
            obj,
            "SEM021",
            f"weapon spell values {payload} require spell 1..{max_spell}, level >= 1, "
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
    quests: list[QuestRecord] | None = None,
    hlquests: list[HlQuestRecord] | None = None,
) -> list[Finding]:
  """Run only source-backed semantic checks over an already validated graph."""

  findings: list[Finding] = []
  _validate_reserved_flags(
      rooms, mobiles, objects, findings, selected_packages, manifest
  )
  _validate_placeholder_text(
      zones, rooms, mobiles, objects, triggers, findings, selected_packages
  )
  _validate_room_level_ranges(rooms, findings, selected_packages, manifest)
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
  _validate_quest_scalars(quests or [], findings, selected_packages, manifest)
  _validate_quest_topology(quests or [], findings, selected_packages, manifest)
  _validate_hlquest_semantics(
      hlquests or [],
      rooms,
      findings,
      selected_packages,
      manifest,
      direction_count,
      exits_by_room,
  )
  return findings
