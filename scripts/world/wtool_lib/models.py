"""Typed models shared by the world-data parsers and reporters."""

from __future__ import annotations

from dataclasses import dataclass, field, replace
from typing import Any


TOOL_VERSION = "0.8.0"
JSON_SCHEMA_VERSION = 1


@dataclass(frozen=True, slots=True)
class SourceSpan:
  """A physical source range using one-based line and column numbers."""

  path: str
  line: int
  column: int = 1
  end_line: int | None = None
  end_column: int | None = None


@dataclass(frozen=True, slots=True)
class RelatedLocation:
  """The other endpoint involved in a diagnostic."""

  path: str
  line: int
  column: int = 1
  record_type: str | None = None
  vnum: int | None = None

  def to_dict(self) -> dict[str, Any]:
    data: dict[str, Any] = {
        "path": self.path,
        "line": self.line,
        "column": self.column,
    }
    if self.record_type is not None:
      data["record_type"] = self.record_type
    if self.vnum is not None:
      data["vnum"] = self.vnum
    return data


@dataclass(frozen=True, slots=True)
class Finding:
  """A stable, source-located validation result."""

  code: str
  severity: str
  message: str
  span: SourceSpan
  record_type: str | None = None
  vnum: int | None = None
  related: RelatedLocation | None = None
  suppressed: bool = False

  def sort_key(self) -> tuple[str, int, int, str, str]:
    return (
        self.span.path,
        self.span.line,
        self.span.column,
        self.code,
        self.message,
    )

  def suppress(self, ignored_codes: set[str]) -> "Finding":
    if self.code in ignored_codes and self.severity in {"warning", "info"}:
      return replace(self, suppressed=True)
    return self

  def to_dict(self) -> dict[str, Any]:
    data: dict[str, Any] = {
        "code": self.code,
        "severity": self.severity,
        "message": self.message,
        "path": self.span.path,
        "line": self.span.line,
        "column": self.span.column,
        "suppressed": self.suppressed,
    }
    if self.span.end_line is not None:
      data["end_line"] = self.span.end_line
    if self.span.end_column is not None:
      data["end_column"] = self.span.end_column
    if self.record_type is not None:
      data["record_type"] = self.record_type
    if self.vnum is not None:
      data["vnum"] = self.vnum
    if self.related is not None:
      data["related"] = self.related.to_dict()
    return data


@dataclass(slots=True)
class IndexRecord:
  name: str
  extension: str
  span: SourceSpan
  exists: bool


@dataclass(slots=True)
class ExitRecord:
  direction: int
  description: str | None
  keyword: str | None
  door_flags: int
  key_vnum: int
  destination_vnum: int
  span: SourceSpan


@dataclass(slots=True)
class AttachmentRecord:
  trigger_vnum: int
  host_type: str
  host_vnum: int
  span: SourceSpan


@dataclass(slots=True)
class VnumReference:
  target_type: str
  target_vnum: int
  role: str
  span: SourceSpan


@dataclass(slots=True)
class ResetCommandRecord:
  command: str
  dependency: int
  arguments: list[int]
  string_arguments: list[str]
  span: SourceSpan
  effective: bool = True
  probability: int | None = None


@dataclass(slots=True)
class ZoneRecord:
  vnum: int
  span: SourceSpan
  source_package: str
  name: str | None = None
  builders: str | None = None
  bottom: int | None = None
  top: int | None = None
  lifespan: int | None = None
  reset_mode: int | None = None
  flags: list[str] = field(default_factory=list)
  min_level: int | None = None
  max_level: int | None = None
  show_weather: int | None = None
  region: int | None = None
  faction: int | None = None
  city: int | None = None
  header_field_count: int = 0
  commands: list[ResetCommandRecord] = field(default_factory=list)
  complete: bool = True


@dataclass(slots=True)
class MovingConnectionRecord:
  room_vnum: int
  direction: int
  repeat: int
  span: SourceSpan


@dataclass(slots=True)
class ExtraDescriptionRecord:
  keywords: str | None
  description: str | None
  span: SourceSpan


@dataclass(slots=True)
class MovingRoomRecord:
  inbound_direction: int
  reset_pulses: int
  random_move: int
  exit_info: int
  key_vnum: int
  messages: list[str | None]
  connections: list[MovingConnectionRecord]
  span: SourceSpan


@dataclass(frozen=True, slots=True)
class RolExitTrapRecord:
  direction: int
  state: int
  trap_type: int
  minimum_damage: int
  maximum_damage: int
  area_effect: int
  hardness: int
  load_percent: int
  span: SourceSpan


@dataclass(slots=True)
class RoomRecord:
  vnum: int
  span: SourceSpan
  source_package: str
  name: str | None = None
  description: str | None = None
  file_zone: int | None = None
  flags: list[str] = field(default_factory=list)
  sector: int | None = None
  minimum_level: int = -1
  maximum_level: int = -1
  exits: list[ExitRecord] = field(default_factory=list)
  extra_descriptions: list[ExtraDescriptionRecord] = field(default_factory=list)
  attachments: list[AttachmentRecord] = field(default_factory=list)
  moving_connections: list[MovingConnectionRecord] = field(default_factory=list)
  moving_room: MovingRoomRecord | None = None
  rol_exit_traps: list[RolExitTrapRecord] = field(default_factory=list)
  spec_proc: str | None = None
  coordinates: tuple[int, int] | None = None
  owner_zone_vnum: int | None = None
  complete: bool = True


@dataclass(slots=True)
class MobileRecord:
  vnum: int
  span: SourceSpan
  source_package: str
  aliases: str | None = None
  short_description: str | None = None
  long_description: str | None = None
  description: str | None = None
  action_flags: list[str] = field(default_factory=list)
  affect_flags: list[str] = field(default_factory=list)
  affect2_flags: list[str] = field(default_factory=list)
  legacy_affect_encoding: bool = False
  alignment: int | None = None
  record_kind: str | None = None
  level: int | None = None
  hit_roll: int | None = None
  armor_class: int | None = None
  hit_dice: tuple[int, int, int] | None = None
  damage_dice: tuple[int, int, int] | None = None
  gold: int | None = None
  experience: int | None = None
  position: int | None = None
  default_position: int | None = None
  sex: int | None = None
  spec_proc: str | None = None
  enhanced: dict[str, list[str]] = field(default_factory=dict)
  path_rooms: list[int] = field(default_factory=list)
  references: list[VnumReference] = field(default_factory=list)
  attachments: list[AttachmentRecord] = field(default_factory=list)
  complete: bool = True


@dataclass(slots=True)
class ObjectAffectRecord:
  location: int
  modifier: int
  bonus_type: int | None
  specific: int | None
  span: SourceSpan


@dataclass(slots=True)
class ObjectSpecialAbilityRecord:
  ability: int
  level: int
  activation_method: int
  values: list[int]
  command_word: str | None
  span: SourceSpan


@dataclass(slots=True)
class ObjectRecord:
  vnum: int
  span: SourceSpan
  source_package: str
  aliases: str | None = None
  short_description: str | None = None
  description: str | None = None
  action_description: str | None = None
  item_type: int | None = None
  extra_flags: list[str] = field(default_factory=list)
  wear_flags: list[str] = field(default_factory=list)
  affect_flags: list[str] = field(default_factory=list)
  affect2_flags: list[str] = field(default_factory=list)
  legacy_affect_encoding: bool = False
  values: list[int] = field(default_factory=list)
  weight: int | None = None
  cost: int | None = None
  rent: int | None = None
  level: int | None = None
  timer: int | None = None
  affects: list[ObjectAffectRecord] = field(default_factory=list)
  spellbook: list[tuple[int, int, SourceSpan]] = field(default_factory=list)
  special_abilities: list[ObjectSpecialAbilityRecord] = field(default_factory=list)
  extra_descriptions: list[ExtraDescriptionRecord] = field(default_factory=list)
  activated_spells: list[tuple[list[int], SourceSpan]] = field(default_factory=list)
  weapon_spells: list[tuple[list[int], SourceSpan]] = field(default_factory=list)
  attachments: list[AttachmentRecord] = field(default_factory=list)
  references: list[VnumReference] = field(default_factory=list)
  spec_proc: str | None = None
  recipient_vnum: int | None = None
  restring_identifier: str | None = None
  complete: bool = True


@dataclass(slots=True)
class TriggerRecord:
  vnum: int
  span: SourceSpan
  source_package: str
  name: str | None = None
  attach_type: int | None = None
  type_flags: str | None = None
  type_bits: set[int] = field(default_factory=set)
  numeric_argument: int | None = None
  argument_list: str | None = None
  commands: str | None = None
  complete: bool = True


@dataclass(slots=True)
class ShopBuyTypeRecord:
  item_type: int
  keywords: str | None
  span: SourceSpan


@dataclass(slots=True)
class ShopRecord:
  vnum: int
  span: SourceSpan
  source_package: str
  modern: bool = False
  keeper_vnum: int | None = None
  product_vnums: list[int] = field(default_factory=list)
  buy_types: list[ShopBuyTypeRecord] = field(default_factory=list)
  room_vnums: list[int] = field(default_factory=list)
  open_hours: list[int] = field(default_factory=list)
  profit_buy: float | None = None
  profit_sell: float | None = None
  temper: int | None = None
  shop_flags: int | None = None
  customer_restrictions: int | None = None
  rol_cheat_restrictions: int = 0
  messages: list[str] = field(default_factory=list)
  references: list[VnumReference] = field(default_factory=list)
  complete: bool = True


@dataclass(slots=True)
class QuestRecord:
  vnum: int
  span: SourceSpan
  source_package: str
  name: str | None = None
  description: str | None = None
  accept_message: str | None = None
  completion_message: str | None = None
  quit_message: str | None = None
  quest_type: int | None = None
  questmaster_vnum: int | None = None
  flag_token: str | None = None
  flag_bits: set[int] = field(default_factory=set)
  target: int | None = None
  previous_quest_vnum: int | None = None
  next_quest_vnum: int | None = None
  prerequisite_object_vnum: int | None = None
  points: int | None = None
  quit_penalty: int | None = None
  min_level: int | None = None
  max_level: int | None = None
  time_limit: int | None = None
  return_mobile_vnum: int | None = None
  quantity: int | None = None
  gold_reward: int | None = None
  experience_reward: int | None = None
  reward_object_vnum: int | None = None
  reward_row_width: int = 0
  race_reward: int | None = None
  wilderness_x: int | None = None
  wilderness_y: int | None = None
  follower_mobile_vnum: int | None = None
  diplomacy_dc: int | None = None
  intimidate_dc: int | None = None
  bluff_dc: int | None = None
  dialogue_alternative_quest_vnum: int | None = None
  dialogue_block_count: int = 0
  raw_values: dict[str, int] = field(default_factory=dict)
  field_spans: dict[str, SourceSpan] = field(default_factory=dict)
  references: list[VnumReference] = field(default_factory=list)
  complete: bool = True


@dataclass(slots=True)
class HlQuestCommandRecord:
  direction_marker: str
  direction: str | None
  code_token: str | None
  code: str | None
  command_type: int | None
  value: int | None
  location: int | None
  span: SourceSpan
  physical_ordinal: int
  effective_runtime_ordinal: int | None = None
  effective: bool = True
  field_spans: dict[str, SourceSpan] = field(default_factory=dict)
  complete: bool = True


@dataclass(slots=True)
class HlQuestEntryRecord:
  entry_type: int
  marker: str
  approval_suffix: str
  approved: bool
  span: SourceSpan
  physical_ordinal: int
  effective_runtime_ordinal: int | None = None
  keywords: str | None = None
  reply_message: str | None = None
  room_vnum: int | None = None
  commands: list[HlQuestCommandRecord] = field(default_factory=list)
  chain_terminated: bool = False
  field_spans: dict[str, SourceSpan] = field(default_factory=dict)
  complete: bool = True

  @property
  def input_commands(self) -> list[HlQuestCommandRecord]:
    return [command for command in self.commands if command.direction == "input"]

  @property
  def output_commands(self) -> list[HlQuestCommandRecord]:
    return [command for command in self.commands if command.direction == "output"]


@dataclass(slots=True)
class HlQuestRecord:
  vnum: int
  span: SourceSpan
  source_package: str
  entries: list[HlQuestEntryRecord] = field(default_factory=list)
  field_spans: dict[str, SourceSpan] = field(default_factory=dict)
  references: list[VnumReference] = field(default_factory=list)
  complete: bool = True

  @property
  def host_mobile_vnum(self) -> int:
    return self.vnum


WorldRecord = (
    ZoneRecord
    | RoomRecord
    | MobileRecord
    | ObjectRecord
    | TriggerRecord
    | ShopRecord
    | QuestRecord
    | HlQuestRecord
)


@dataclass(slots=True)
class WorldData:
  zones: list[ZoneRecord] = field(default_factory=list)
  rooms: list[RoomRecord] = field(default_factory=list)
  mobiles: list[MobileRecord] = field(default_factory=list)
  objects: list[ObjectRecord] = field(default_factory=list)
  triggers: list[TriggerRecord] = field(default_factory=list)
  shops: list[ShopRecord] = field(default_factory=list)
  quests: list[QuestRecord] = field(default_factory=list)
  hlquests: list[HlQuestRecord] = field(default_factory=list)
  findings: list[Finding] = field(default_factory=list)
  complete: bool = True

  def records(self, record_type: str) -> list[WorldRecord]:
    collections: dict[str, list[WorldRecord]] = {
        "zone": list(self.zones),
        "room": list(self.rooms),
        "mobile": list(self.mobiles),
        "object": list(self.objects),
        "trigger": list(self.triggers),
        "shop": list(self.shops),
        "quest": list(self.quests),
        "hlquest": list(self.hlquests),
    }
    if record_type not in collections:
      raise ValueError(f"unknown world record type {record_type!r}")
    return collections[record_type]


@dataclass(slots=True)
class ValidationResult:
  root_label: str
  mode: str
  findings: list[Finding] = field(default_factory=list)
  complete: bool = True
  config: dict[str, Any] = field(default_factory=dict)

  def normalized_findings(self, ignored_codes: set[str] | None = None) -> list[Finding]:
    ignored = ignored_codes or set()
    return sorted((finding.suppress(ignored) for finding in self.findings), key=Finding.sort_key)
