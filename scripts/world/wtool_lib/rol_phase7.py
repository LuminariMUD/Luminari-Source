"""Build deterministic Phase 7 Realms of Luminari corpus milestones."""

from __future__ import annotations

from collections import Counter, defaultdict
from dataclasses import replace
import hashlib
import json
from pathlib import Path
import re
import shutil
import textwrap
from typing import Any, Iterable

from .config import resolve_config
from .constants import default_repo_root, load_manifest
from .flags import decode_tokens, encode_bits
from .models import TOOL_VERSION
from .reporting import result_payload
from .rol_discovery import extract_source_commands
from .rol_graph import audit_connection_graph
from .rol_pilot_build import (
    _artifact,
    _canonical_json,
    _canonical_line,
    _created_at,
    _load_json,
    _load_jsonl,
    _reset_references,
    _source_records,
    _verify_bundle,
)
from .rol_skeleton import tree_manifest
from .rol_soc import SocCompilation, compile_soc_records
from .rol_source import RolRecord
from .rol_special import NativeSpecialBinding, SpecialCompilation, compile_special_bindings
from .rol_transform import (
    TransformResult,
    emit_hlquest,
    emit_mobile,
    emit_object,
    emit_room,
    emit_shop,
    emit_zone,
)
from .world import load_indexed_world_data, validate_indexed_world


ROL_PHASE7_SCHEMA_VERSION = 1
_SOURCE_ROOT_PREFIX = "EXAMPLE/RealmsOfLuminari"
_KIND_OWNER = {"mob": "mob", "obj": "obj", "wld": "wld"}
_KIND_DIRECTORY = {
    "mob": ("mob", "mob"),
    "obj": ("obj", "obj"),
    "qst": ("hlq", "hlq"),
    "shp": ("shp", "shp"),
    "wld": ("wld", "wld"),
    "zon": ("zon", "zon"),
}
_TYPE_KIND = {"mobile": "mob", "object": "obj", "room": "wld"}
_SOC_OWNER_EXCLUSIONS = frozenset(
    {
        "soc:22496:areas/soc/bs3.soc:1",
        "soc:22496:areas/soc/bs3.soc:72",
        "soc:89128:areas/soc/bandits.soc:1",
    }
)
_SPECIAL_TRIGGER_START = 2_026_167
_COMPOSITE_SPECIALS = {
    ("mob", 2007110): ("RoL Bloodstone Critter", "RoL Corpse Devourer"),
    ("mob", 2007111): ("RoL Bloodstone Critter", "RoL Corpse Devourer"),
    ("mob", 2007112): ("RoL Bloodstone Critter", "RoL Corpse Devourer"),
    ("mob", 2007141): ("RoL Bloodstone Critter", "RoL Corpse Devourer"),
    ("mob", 2007154): ("RoL Corpse Devourer", "RoL Source Periodic"),
    ("mob", 2007177): ("Receptionist", "RoL Source Periodic"),
    ("mob", 2007180): ("RoL Monster Combat", "RoL Source Periodic"),
    ("mob", 2007190): ("RoL Source Periodic", "money_changer"),
    ("mob", 2019701): (
        "RoL Lich Energy Drain",
        "RoL Monster Combat",
        "breath_weapon_acid",
    ),
    ("mob", 2019750): ("RoL Guild Guard", "RoL Monster Combat"),
    ("mob", 2025400): ("RoL Guild Guard", "breath_weapon_gas"),
    ("mob", 2080220): ("RoL Monster Combat", "RoL Poison Bite"),
    ("mob", 2093202): ("RoL Monster Combat", "RoL Source Periodic"),
    ("obj", 2003088): ("RoL Travel Portal", "RoL Utility Object"),
}


class RolPhase7Error(ValueError):
  """Raised when a Phase 7 milestone violates a frozen conversion invariant."""


class _UnionFind:
  def __init__(self, values: Iterable[str]):
    self.parent = {value: value for value in values}

  def find(self, value: str) -> str:
    parent = self.parent[value]
    if parent != value:
      self.parent[value] = self.find(parent)
    return self.parent[value]

  def union(self, left: str, right: str) -> None:
    left_root = self.find(left)
    right_root = self.find(right)
    if left_root != right_root:
      self.parent[max(left_root, right_root)] = min(left_root, right_root)


def _write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> int:
  count = 0
  path.parent.mkdir(parents=True, exist_ok=True)
  with path.open("wb") as output:
    for row in rows:
      output.write(_canonical_line(row))
      count += 1
  return count


def _augment_actions(
    discovery_dir: Path, plan_dir: Path
) -> tuple[list[dict[str, Any]], dict[str, dict[str, Any]]]:
  metadata = {
      str(row["record_id"]): row
      for row in _load_jsonl(discovery_dir / "source-records.jsonl")
  }
  actions: list[dict[str, Any]] = []
  for planned in _load_jsonl(plan_dir / "change-plan.jsonl"):
    record_id = str(planned["source_record_id"])
    try:
      source = metadata[record_id]
    except KeyError as error:
      raise RolPhase7Error(f"plan record is absent from discovery: {record_id}") from error
    actions.append(
        {
            **planned,
            "basename": str(source["basename"]),
            "source_kind": str(source["kind"]),
            "source_path": str(source["path"]),
            "source_vnum": int(source["vnum"]),
        }
    )
  if len(actions) != len(metadata):
    raise RolPhase7Error("Phase 7 requires one planned action for every discovered record")
  return actions, metadata


def _package_batches(
    inventory: dict[str, Any],
    metadata: dict[str, dict[str, Any]],
    actions: Iterable[dict[str, Any]],
    references: Iterable[dict[str, Any]],
) -> list[list[str]]:
  packages = sorted(
      str(row["basename"])
      for row in inventory["packages"]
      if row.get("included") is True
  )
  union = _UnionFind(packages)
  owners: defaultdict[tuple[str, int], set[str]] = defaultdict(set)
  for row in metadata.values():
    owners[(str(row["kind"]), int(row["vnum"]))].add(str(row["basename"]))
  target_kind = {"mobile": "mob", "object": "obj", "room": "wld"}
  for reference in references:
    if reference.get("resolution") != "active_source":
      continue
    source = metadata.get(str(reference.get("source_record_id")))
    kind = target_kind.get(str(reference.get("target_type")))
    if source is None or kind is None:
      continue
    destinations = owners.get((kind, int(reference["target_vnum"])), set())
    for destination in destinations:
      union.union(str(source["basename"]), destination)

  zones: defaultdict[int, set[str]] = defaultdict(set)
  for action in actions:
    if action["source_kind"] == "zon" and action["destination_vnum"] is not None:
      zones[int(action["destination_vnum"])].add(str(action["basename"]))
  for basenames in zones.values():
    first = min(basenames)
    for basename in basenames:
      union.union(first, basename)

  for alias, owner in (
      ("foggy_woods2", "foggy_woods"),
      ("northern_highroad2", "northern_highroad"),
      ("northern_highroad3", "northern_highroad"),
  ):
    if alias in union.parent and owner in union.parent:
      union.union(alias, owner)

  components: defaultdict[str, list[str]] = defaultdict(list)
  for package in packages:
    components[union.find(package)].append(package)
  result = sorted(
      (sorted(component) for component in components.values()),
      key=lambda component: (-len(component), component[0]),
  )
  if len(result) != 12 or sum(len(component) for component in result) != 258:
    raise RolPhase7Error(
        f"frozen dependency graph expected 12 batches and 258 packages, got "
        f"{len(result)} batches and {sum(len(component) for component in result)} packages"
    )
  return result


def _typed_resolver(
    plan_dir: Path, references: Iterable[dict[str, Any]]
):
  identities = {
      (str(row["source_kind"]), int(row["source_vnum"])): row["destination_vnum"]
      for row in _load_jsonl(plan_dir / "identity-map.jsonl")
  }
  exact: set[tuple[str, int]] = set()
  kind_for_type = {"mobile": "mob", "object": "obj", "room": "wld"}
  for row in references:
    if row.get("resolution") != "target_exact":
      continue
    kind = kind_for_type.get(str(row.get("target_type")))
    if kind is not None:
      exact.add((kind, int(row["target_vnum"])))

  def resolve(kind: str, vnum: int) -> int:
    key = (kind, vnum)
    if key in identities:
      destination = identities[key]
      if destination is None:
        raise RolPhase7Error(f"required identity {kind} {vnum} is excluded")
      return int(destination)
    if (key in exact and kind != "wld") or vnum <= 0:
      return vnum
    raise RolPhase7Error(f"no typed identity for {kind} {vnum}")

  return resolve


def _zone_intervals(
    actions: Iterable[dict[str, Any]], records: dict[str, RolRecord]
) -> dict[str, list[tuple[int, int, int]]]:
  result: defaultdict[str, list[tuple[int, int, int]]] = defaultdict(list)
  for action in actions:
    if action["source_kind"] != "zon" or action["destination_vnum"] is None:
      continue
    record = records[str(action["source_record_id"])]
    header = list(record.values.get("header", []))
    if not header:
      raise RolPhase7Error(f"zone {record.record_id} lacks a top VNUM")
    bottom = 81700 if record.basename == "mytheast" else record.vnum * 100
    result[record.basename].append((bottom, int(header[0]), int(action["destination_vnum"])))
  normalized: dict[str, list[tuple[int, int, int]]] = {}
  for basename, source_rows in result.items():
    rows = sorted(source_rows)
    output: list[tuple[int, int, int]] = []
    for index, (bottom, top, destination_zone) in enumerate(rows):
      if index + 1 < len(rows):
        top = min(top, rows[index + 1][0] - 1)
      if top < bottom:
        top = bottom + 99
        if index + 1 < len(rows):
          top = min(top, rows[index + 1][0] - 1)
      output.append((bottom, top, destination_zone))
    normalized[basename] = output
  return normalized


def _record_zone(
    basename: str,
    source_vnum: int,
    destination_vnum: int,
    intervals: dict[str, list[tuple[int, int, int]]],
) -> int:
  rows = intervals.get(basename, [])
  for bottom, top, destination_zone in rows:
    if bottom <= source_vnum <= top:
      return destination_zone
  if len(rows) == 1:
    return rows[0][2]
  return destination_vnum // 100


def _source_zone_flags(
    basename: str,
    source_vnum: int,
    intervals: dict[str, list[tuple[int, int, int]]],
    zone_records: dict[tuple[str, int], RolRecord],
) -> int:
  rows = intervals.get(basename, [])
  selected: RolRecord | None = None
  for bottom, top, destination_zone in rows:
    del destination_zone
    if bottom <= source_vnum <= top:
      selected = zone_records.get((basename, bottom))
      break
  if selected is None and len(rows) == 1:
    selected = zone_records.get((basename, rows[0][0]))
  if selected is None:
    return 0
  header = list(selected.values.get("header", []))
  return int(header[3]) if len(header) > 3 else 0


def _target_zone_ranges(
    actions: Iterable[dict[str, Any]],
    records: dict[str, RolRecord],
    intervals: dict[str, list[tuple[int, int, int]]],
) -> dict[int, tuple[int, int]]:
  selected = [
      action
      for action in actions
      if action["action"] != "EXCLUDE" and action["destination_vnum"] is not None
  ]
  rooms: defaultdict[int, list[int]] = defaultdict(list)
  for action in selected:
    if action["source_kind"] != "wld":
      continue
    owner = _record_zone(
        str(action["basename"]),
        int(action["source_vnum"]),
        int(action["destination_vnum"]),
        intervals,
    )
    rooms[owner].append(int(action["destination_vnum"]))

  provisional: defaultdict[int, list[tuple[int, int]]] = defaultdict(list)
  for action in selected:
    if action["source_kind"] != "zon":
      continue
    record = records[str(action["source_record_id"])]
    destination = int(action["destination_vnum"])
    source_bottom = 81700 if record.basename == "mytheast" else record.vnum * 100
    interval = next(
        (
            (bottom, top)
            for bottom, top, zone in intervals[record.basename]
            if bottom == source_bottom and zone == destination
        ),
        None,
    )
    if interval is None:
      raise RolPhase7Error(f"cannot recover target range for {record.record_id}")
    owned = rooms.get(destination, [])
    bottom = min(owned) if owned else destination * 100
    top = destination * 100 + (interval[1] - interval[0])
    if owned:
      top = max(top, max(owned))
    provisional[destination].append((bottom, top))

  combined = {
      destination: (
          min(bottom for bottom, _ in rows),
          max(top for _, top in rows),
      )
      for destination, rows in provisional.items()
  }
  ordered = sorted(
      ((bottom, top, destination) for destination, (bottom, top) in combined.items()),
      key=lambda row: (row[0], row[2]),
  )
  result: dict[int, tuple[int, int]] = {}
  for index, (bottom, top, destination) in enumerate(ordered):
    if index + 1 < len(ordered):
      next_bottom = ordered[index + 1][0]
      if next_bottom <= bottom:
        raise RolPhase7Error(
            f"target zones {destination} and {ordered[index + 1][2]} share bottom {bottom}"
        )
      top = min(top, next_bottom - 1)
    result[destination] = (bottom, max(bottom, top))
  return result


def _target_room_zone(
    destination_vnum: int,
    fallback: int,
    ranges: dict[int, tuple[int, int]],
) -> int:
  owners = [
      zone for zone, (bottom, top) in ranges.items() if bottom <= destination_vnum <= top
  ]
  if len(owners) == 1:
    return owners[0]
  return fallback


def _binding_rows(phase6_dir: Path) -> list[dict[str, Any]]:
  rows: list[dict[str, Any]] = []
  for source in _load_jsonl(phase6_dir / "binding-ledger.jsonl"):
    consumers = source.get("consumers", [])
    if source.get("status") != "resolved" or not consumers:
      continue
    row = dict(source)
    row["basename"] = str(consumers[0]["basename"])
    rows.append(row)
  return rows


def _aggregate_native_bindings(
    compilation: SpecialCompilation,
) -> dict[tuple[str, int], NativeSpecialBinding]:
  grouped: defaultdict[tuple[str, int], list[NativeSpecialBinding]] = defaultdict(list)
  for binding in compilation.native_bindings:
    grouped[(binding.target_kind, binding.target_vnum)].append(binding)

  result: dict[tuple[str, int], NativeSpecialBinding] = {}
  measured_conflicts: set[tuple[str, int]] = set()
  for key, bindings in sorted(grouped.items()):
    persisted: list[str] = []
    for binding in bindings:
      if binding.persisted_name is not None and binding.persisted_name not in persisted:
        persisted.append(binding.persisted_name)
    if len(persisted) > 1:
      expected = _COMPOSITE_SPECIALS.get(key)
      if expected is None or tuple(persisted) != expected:
        raise RolPhase7Error(
            f"unmeasured composite special profile {key}: {persisted!r}"
        )
      persisted_name = "RoL Composite Mobile" if key[0] == "mob" else "RoL Composite Object"
      measured_conflicts.add(key)
    else:
      persisted_name = persisted[0] if persisted else None
    first = bindings[0]
    result[key] = replace(
        first,
        persisted_name=persisted_name,
        required_flag_bits=tuple(
            sorted({bit for binding in bindings for bit in binding.required_flag_bits})
        ),
        required_affect_bits=tuple(
            sorted({bit for binding in bindings for bit in binding.required_affect_bits})
        ),
        value_reference_slots=tuple(
            sorted(
                {
                    slot
                    for binding in bindings
                    for slot in binding.value_reference_slots
                }
            )
        ),
    )
  if measured_conflicts != set(_COMPOSITE_SPECIALS):
    missing = sorted(set(_COMPOSITE_SPECIALS) - measured_conflicts)
    raise RolPhase7Error(f"measured composite profiles were not reproduced: {missing}")
  return result


def _trigger_vnums(path: Path) -> set[int]:
  result: set[int] = set()
  if not path.is_dir():
    return result
  for source in sorted(path.glob("*.trg")):
    text = source.read_text(encoding="utf-8")
    result.update(int(value) for value in re.findall(r"(?m)^#(\d+)\s*$", text))
  return result


def _compile_soc_corpus(
    records: Iterable[RolRecord],
    resolve,
    source_commands: dict[int, str],
    intervals: dict[str, list[tuple[int, int, int]]],
    occupied: set[int],
) -> SocCompilation:
  by_basename: defaultdict[str, list[RolRecord]] = defaultdict(list)
  for record in records:
    if record.record_id not in _SOC_OWNER_EXCLUSIONS:
      by_basename[record.basename].append(record)

  fixed: dict[str, tuple[int, ...]] = {
      "hulburg": tuple(range(2_026_100, 2_026_150)),
      "theswamp": tuple(range(2_040_900, 2_041_000)),
      "cemetery": tuple(range(2_055_300, 2_055_400)),
      "muspel": (*range(2_058_600, 2_058_700), *range(2_026_150, 2_026_167)),
  }
  reserved = set(occupied)
  reserved.update(range(_SPECIAL_TRIGGER_START, _SPECIAL_TRIGGER_START + 14))
  compilations: list[SocCompilation] = []
  for basename in sorted(by_basename):
    source_records = by_basename[basename]
    if basename in fixed:
      candidates = [value for value in fixed[basename] if value not in occupied]
    else:
      sample = source_records[0]
      destination = resolve("mob", sample.vnum)
      package_intervals = intervals.get(basename, [])
      zone = (
          package_intervals[0][2]
          if len(package_intervals) == 1
          else _record_zone(basename, sample.vnum, destination, intervals)
      )
      candidates = [value for value in range(zone * 100, zone * 100 + 100) if value not in reserved]
    try:
      compilation = compile_soc_records(
          source_records,
          0,
          resolve,
          source_commands,
          trigger_vnums=candidates,
      )
    except (IndexError, StopIteration, ValueError) as error:
      raise RolPhase7Error(f"SOC trigger range exhausted for {basename}: {error}") from error
    used = {trigger.vnum for trigger in compilation.triggers}
    if used & reserved:
      raise RolPhase7Error(f"SOC trigger collision for {basename}: {sorted(used & reserved)[0]}")
    reserved.update(used)
    compilations.append(compilation)

  triggers = sorted(
      (trigger for compilation in compilations for trigger in compilation.triggers),
      key=lambda trigger: trigger.vnum,
  )
  if len({trigger.vnum for trigger in triggers}) != len(triggers):
    raise RolPhase7Error("full-corpus SOC compilation emitted duplicate trigger VNUMs")
  attachments: defaultdict[int, list[int]] = defaultdict(list)
  for compilation in compilations:
    for owner, trigger_ids in compilation.attachments.items():
      attachments[owner].extend(trigger_ids)
  return SocCompilation(
      triggers=triggers,
      attachments={owner: sorted(ids) for owner, ids in sorted(attachments.items())},
      diagnostics=[item for compilation in compilations for item in compilation.diagnostics],
      source_records=sum(item.source_records for item in compilations),
      source_actions=sum(item.source_actions for item in compilations),
  )


def _merge_zone_blocks(blocks: list[tuple[int, str, str]]) -> str:
  if not blocks:
    raise RolPhase7Error("cannot merge an empty zone group")
  parsed: list[tuple[list[str], list[str]]] = []
  for destination, text, record_id in blocks:
    del record_id
    lines = text.splitlines(keepends=True)
    if len(lines) < 6 or not lines[0].startswith(f"#{destination}"):
      raise RolPhase7Error(f"malformed emitted zone {destination}")
    parsed.append((lines[:4], lines[4:-2]))
  header = list(parsed[0][0])
  tokens = [item[0][3].split() for item in parsed]
  for item in tokens:
    if len(item) < 4:
      raise RolPhase7Error("emitted zone header is missing required fields")
    if len(item) < 8:
      item.extend(["0", "0", "0", "0", "-1", "-1", "1", "0", "0", "0"])
  base = list(tokens[0])
  base[0] = str(min(int(item[0]) for item in tokens))
  base[1] = str(max(int(item[1]) for item in tokens))
  base[2] = str(max(int(item[2]) for item in tokens))
  base[3] = str(max(int(item[3]) for item in tokens))
  flag_bits: set[int] = set()
  for item in tokens:
    decoded = decode_tokens(item[4:8])
    if decoded.issues:
      raise RolPhase7Error("cannot decode emitted zone flags during MERGE")
    flag_bits.update(decoded.bits)
  base[4:8] = list(encode_bits(flag_bits))
  header[3] = " ".join(base) + "\n"
  commands = [line for _, rows in parsed for line in rows]
  return "".join([*header, *commands, "S\n", "$\n"])


def _merge_hlquest_blocks(blocks: list[tuple[int, str, str]]) -> str:
  if not blocks:
    raise RolPhase7Error("cannot merge an empty hlquest group")
  destination = blocks[0][0]
  bodies: list[str] = []
  seen: set[str] = set()
  for block_destination, block, record_id in blocks:
    lines = block.splitlines(keepends=True)
    if block_destination != destination or not lines or lines[0].strip() != f"#{destination}":
      raise RolPhase7Error(f"malformed emitted hlquest {record_id}")
    body = "".join(lines[1:])
    if body not in seen:
      seen.add(body)
      bodies.append(body)
  return f"#{destination}\n" + "".join(bodies)


def _filter_zone_door_resets(
    text: str,
    valid_exit_directions: dict[int, set[int]],
) -> tuple[str, list[str]]:
  output: list[str] = []
  diagnostics: list[str] = []
  for line in text.splitlines(keepends=True):
    tokens = line.split()
    if len(tokens) >= 4 and tokens[0] in {"D", "K"}:
      room = int(tokens[2])
      direction = int(tokens[3])
      if room in valid_exit_directions and direction not in valid_exit_directions[room]:
        diagnostics.append(
            f"excluded {tokens[0]} reset for absent exit {room} direction {direction}"
        )
        continue
    output.append(line)
  return "".join(output), diagnostics


def _bound_trigger_text(vnum: int, source: str) -> tuple[str, list[str]]:
  output: list[str] = []
  diagnostics: list[str] = []
  for line_number, line in enumerate(source.splitlines(keepends=True), start=1):
    body = line.rstrip("\n")
    if len(body.encode("ascii")) <= 480:
      output.append(line)
      continue
    indentation = body[: len(body) - len(body.lstrip())]
    stripped = body.lstrip()
    if stripped.startswith("mecho "):
      prefix = indentation + "mecho "
      payload = stripped[len("mecho "):]
    elif stripped.startswith("mrolzoneecho "):
      words = stripped.split(maxsplit=3)
      if len(words) != 4:
        raise RolPhase7Error(f"cannot safely bound trigger {vnum} line {line_number}")
      prefix = indentation + " ".join(words[:3]) + " "
      payload = words[3]
    else:
      raise RolPhase7Error(
          f"trigger {vnum} line {line_number} exceeds 480 bytes in an unsupported command"
      )
    chunks = textwrap.wrap(
        payload,
        width=480 - len(prefix),
        break_long_words=True,
        break_on_hyphens=False,
        replace_whitespace=True,
        drop_whitespace=True,
    )
    output.extend(prefix + chunk + "\n" for chunk in chunks)
    diagnostics.append(
        f"split overlong echo at generated trigger line {line_number} into {len(chunks)} echoes"
    )
  return "".join(output), diagnostics


def _split_world_file(path: Path, kind: str) -> tuple[str, list[tuple[int, str]]]:
  text = path.read_text(encoding="utf-8")
  matches = list(re.finditer(r"(?m)^#(\d+)~?\s*$", text))
  if not matches:
    prefix = text
    if kind == "shp" and "$~" in prefix:
      prefix = prefix[: prefix.rfind("$~")]
    return prefix, []
  prefix = text[: matches[0].start()]
  blocks: list[tuple[int, str]] = []
  for index, match in enumerate(matches):
    end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
    block = text[match.start():end]
    if index + 1 == len(matches):
      lines = block.splitlines(keepends=True)
      while lines and not lines[-1].strip():
        lines.pop()
      if lines and lines[-1].strip() == "$~":
        lines.pop()
      elif kind != "zon" and lines and lines[-1].strip() == "$":
        lines.pop()
      block = "".join(lines)
    blocks.append((int(match.group(1)), block))
  return prefix, blocks


def _render_world_file(kind: str, prefix: str, blocks: Iterable[tuple[int, str]]) -> str:
  body = "".join(text if text.endswith("\n") else text + "\n" for _, text in blocks)
  if kind == "shp":
    if not prefix.strip():
      prefix = "CircleMUD v3.0 Shop File~\n"
    elif not prefix.endswith("\n"):
      prefix += "\n"
    return prefix + body + "$~\n"
  if kind == "zon":
    return body
  return body + "$~\n"


def _merge_world_records(
    path: Path,
    kind: str,
    additions: list[tuple[int, str, str]],
    replacements: set[int] = frozenset(),
) -> None:
  if path.exists():
    prefix, existing = _split_world_file(path, kind)
  else:
    prefix, existing = ("CircleMUD v3.0 Shop File~\n" if kind == "shp" else ""), []
  existing_counts = Counter(vnum for vnum, _ in existing)
  replacement_text: defaultdict[int, list[tuple[str, str]]] = defaultdict(list)
  appended: list[tuple[int, str, str]] = []
  for vnum, text, record_id in additions:
    if vnum in replacements:
      replacement_text[vnum].append((text, record_id))
    else:
      if existing_counts[vnum] and kind != "hlq":
        raise RolPhase7Error(f"ADD {record_id} collides with existing {kind} {vnum}")
      appended.append((vnum, text, record_id))
  rendered: list[tuple[int, str]] = []
  replaced: set[int] = set()
  for vnum, text in existing:
    if vnum not in replacement_text:
      rendered.append((vnum, text))
      continue
    if vnum in replaced:
      continue
    rows = replacement_text[vnum]
    if len(rows) != 1:
      raise RolPhase7Error(f"replacement {kind} {vnum} is not unique")
    rendered.append((vnum, rows[0][0]))
    replaced.add(vnum)
  missing_replacements = set(replacement_text) - replaced
  if missing_replacements:
    missing = min(missing_replacements)
    raise RolPhase7Error(f"replacement target {kind} {missing} is absent from {path}")
  rendered.extend((vnum, text) for vnum, text, _ in sorted(appended))
  rendered.sort(key=lambda row: row[0])
  path.parent.mkdir(parents=True, exist_ok=True)
  path.write_text(_render_world_file(kind, prefix, rendered), encoding="utf-8", newline="\n")


def _block_flag_row(lines: list[str], kind: str, vnum: int) -> int:
  if kind == "mob":
    for index, line in enumerate(lines):
      tokens = line.strip().split()
      if len(tokens) >= 10 and tokens[-1] == "E":
        return index
    raise RolPhase7Error(f"mobile {vnum} lacks an enhanced flag row")
  required_tildes = 4 if kind == "obj" else 2
  terminators = 0
  for index, line in enumerate(lines):
    if line.rstrip().endswith("~"):
      terminators += 1
      if terminators == required_tildes:
        if index + 1 >= len(lines):
          break
        return index + 1
  raise RolPhase7Error(f"{kind} {vnum} lacks a canonical flag row")


def _merge_flag_chunks(tokens: list[str], start: int, required: Iterable[int]) -> None:
  required_bits = set(required)
  if not required_bits:
    return
  decoded = decode_tokens(tokens[start:start + 4])
  if decoded.issues:
    raise RolPhase7Error(f"cannot decode persisted flags {tokens[start:start + 4]!r}")
  tokens[start:start + 4] = encode_bits(set(decoded.bits) | required_bits)


def _patch_record_block(
    kind: str,
    vnum: int,
    text: str,
    special_proc: str | None,
    attachments: Iterable[int],
    required_flags: Iterable[int],
    required_affects: Iterable[int],
) -> str:
  lines = text.splitlines(keepends=True)
  flag_index = _block_flag_row(lines, kind, vnum)
  tokens = lines[flag_index].strip().split()
  if kind == "mob":
    action_bits = set(required_flags)
    if special_proc is not None:
      action_bits.add(0)
    _merge_flag_chunks(tokens, 0, action_bits)
    _merge_flag_chunks(tokens, 4, required_affects)
  elif kind == "obj":
    _merge_flag_chunks(tokens, 1, required_flags)
  else:
    _merge_flag_chunks(tokens, 1, required_flags)
  lines[flag_index] = " ".join(tokens) + "\n"

  if kind == "mob":
    lines = [line for line in lines if not line.startswith("SpecProc:")]
    if special_proc is not None:
      enhanced_end = max(
          (index for index, line in enumerate(lines) if line.strip() == "E"),
          default=-1,
      )
      if enhanced_end < 0:
        raise RolPhase7Error(f"mobile {vnum} lacks an enhanced E terminator")
      lines.insert(enhanced_end, f"SpecProc: {special_proc}\n")
  else:
    stripped: list[str] = []
    index = 0
    while index < len(lines):
      if lines[index].strip() == "Z" and index + 1 < len(lines):
        index += 2
        continue
      stripped.append(lines[index])
      index += 1
    lines = stripped
    if special_proc is not None:
      if kind == "wld":
        room_end = max(
            (index for index, line in enumerate(lines) if line.strip() == "S"),
            default=-1,
        )
        if room_end < 0:
          raise RolPhase7Error(f"room {vnum} lacks an S terminator")
        lines[room_end:room_end] = ["Z\n", f"{special_proc}\n"]
      else:
        lines.extend(["Z\n", f"{special_proc}\n"])

  existing_attachments = {
      int(match.group(1))
      for line in lines
      if (match := re.fullmatch(r"T\s+(\d+)\s*\n?", line)) is not None
  }
  lines.extend(
      f"T {trigger_vnum}\n"
      for trigger_vnum in sorted(set(attachments) - existing_attachments)
  )
  return "".join(lines)


def _record_file_index(world_root: Path, kind: str, requested: set[int]) -> dict[int, Path]:
  result: dict[int, Path] = {}
  extension = kind
  for path in sorted((world_root / kind).glob(f"*.{extension}")):
    headers = {
        int(value)
        for value in re.findall(rb"(?m)^#(\d+)~?\s*$", path.read_bytes())
    }
    for vnum in headers:
      if vnum not in requested:
        continue
      if vnum in result:
        raise RolPhase7Error(f"preserved {kind} {vnum} is defined in multiple files")
      result[vnum] = path
  return result


def _patch_preserved_records(
    staging_world: Path,
    patches: dict[
        tuple[str, int],
        tuple[str | None, tuple[int, ...], tuple[int, ...], tuple[int, ...]],
    ],
) -> set[str]:
  by_file: defaultdict[tuple[str, Path], dict[int, tuple[Any, ...]]] = defaultdict(dict)
  requested = {
      kind: {vnum for patch_kind, vnum in patches if patch_kind == kind}
      for kind in ("mob", "obj", "wld")
  }
  indexes = {
      kind: _record_file_index(staging_world, kind, requested[kind])
      for kind in ("mob", "obj", "wld")
  }
  for (kind, vnum), patch in sorted(patches.items()):
    try:
      path = indexes[kind][vnum]
    except KeyError as error:
      raise RolPhase7Error(f"preserved patch target {kind} {vnum} is absent") from error
    by_file[(kind, path)][vnum] = patch

  touched: set[str] = set()
  for (kind, path), file_patches in sorted(by_file.items(), key=lambda item: item[0][1].as_posix()):
    prefix, blocks = _split_world_file(path, kind)
    output: list[tuple[int, str]] = []
    seen: set[int] = set()
    for vnum, text in blocks:
      patch = file_patches.get(vnum)
      if patch is not None:
        text = _patch_record_block(kind, vnum, text, *patch)
        seen.add(vnum)
      output.append((vnum, text))
    if seen != set(file_patches):
      raise RolPhase7Error(f"not every preserved patch was applied in {path}")
    path.write_text(_render_world_file(kind, prefix, output), encoding="utf-8", newline="\n")
    touched.add(path.relative_to(staging_world).as_posix())
  return touched


def _normalized_finding(row: dict[str, Any]) -> str:
  return json.dumps(
      {
          "severity": row.get("severity"),
          "path": row.get("path"),
          "record_type": row.get("record_type"),
          "vnum": row.get("vnum"),
          "suppressed": row.get("suppressed", False),
      },
      ensure_ascii=True,
      sort_keys=True,
      separators=(",", ":"),
  )


def _validation_delta(
    baseline: dict[str, Any], staged: dict[str, Any]
) -> dict[str, Any]:
  before = Counter(_normalized_finding(row) for row in baseline["findings"])
  after = Counter(_normalized_finding(row) for row in staged["findings"])
  new = [json.loads(value) for value in (after - before).elements()]
  resolved = [json.loads(value) for value in (before - after).elements()]
  return {
      "normalization": (
          "severity, path, record type, vnum, suppression; source line, code, and message excluded"
      ),
      "new_findings": sorted(
          new,
          key=lambda row: (
              row["severity"],
              row.get("path") or "",
              row.get("record_type") or "",
              row.get("vnum") if row.get("vnum") is not None else -1,
          ),
      ),
      "resolved_findings": sorted(
          resolved,
          key=lambda row: (
              row["severity"],
              row.get("path") or "",
              row.get("record_type") or "",
              row.get("vnum") if row.get("vnum") is not None else -1,
          ),
      ),
      "new_active_errors": sum(
          row["severity"] == "error" and not row.get("suppressed", False) for row in new
      ),
  }


def _runtime_contract(world, zone_vnums: Iterable[int]) -> dict[str, Any]:
  definitions = {
      "mob": {record.vnum for record in world.mobiles},
      "obj": {record.vnum for record in world.objects},
      "wld": {record.vnum for record in world.rooms},
  }
  zones = {record.vnum: record for record in world.zones}
  rooms = {record.vnum: record for record in world.rooms}
  evidence: list[dict[str, Any]] = []
  for vnum in sorted(set(zone_vnums)):
    zone = zones.get(vnum)
    unresolved: list[dict[str, Any]] = []
    commands: Counter[str] = Counter()
    if zone is not None:
      for ordinal, command in enumerate(zone.commands, start=1):
        commands[command.command] += 1
        for kind, target in _reset_references(command):
          if target > 0 and target not in definitions[kind]:
            unresolved.append(
                {
                    "ordinal": ordinal,
                    "command": command.command,
                    "kind": kind,
                    "vnum": target,
                }
            )
    owned_rooms = [room for room in rooms.values() if room.file_zone == vnum]
    missing_exits = sorted(
        {
            exit_record.destination_vnum
            for room in owned_rooms
            for exit_record in room.exits
            if exit_record.destination_vnum > 0
            and exit_record.destination_vnum not in definitions["wld"]
        }
    )
    evidence.append(
        {
            "zone_vnum": vnum,
            "zone_present": zone is not None,
            "room_count": len(owned_rooms),
            "reset_counts": dict(sorted(commands.items())),
            "unresolved_resets": unresolved,
            "missing_exit_targets": missing_exits,
            "pass": zone is not None and not unresolved and not missing_exits,
        }
    )
  return {
      "zones": evidence,
      "all_pass": all(row["pass"] for row in evidence),
      "zone_count": len(evidence),
      "unresolved_reset_count": sum(len(row["unresolved_resets"]) for row in evidence),
      "missing_exit_count": sum(len(row["missing_exit_targets"]) for row in evidence),
  }


def _preservation_check(
    baseline: dict[str, Any], staged: dict[str, Any], touched: set[str]
) -> dict[str, Any]:
  before = {str(row["path"]): str(row["sha256"]) for row in baseline["files"]}
  after = {str(row["path"]): str(row["sha256"]) for row in staged["files"]}
  changed = sorted(path for path in before.keys() & after.keys() if before[path] != after[path])
  removed = sorted(before.keys() - after.keys())
  undeclared = sorted(set(changed) - touched)
  return {
      "baseline_files": len(before),
      "staged_files": len(after),
      "changed_paths": changed,
      "removed_paths": removed,
      "declared_touched_paths": sorted(touched),
      "undeclared_changed_paths": undeclared,
      "pass": not removed and not undeclared,
  }


def _selected_reference_exceptions(
    references: Iterable[dict[str, Any]], selected_record_ids: set[str]
) -> list[dict[str, Any]]:
  result: list[dict[str, Any]] = []
  for row in references:
    if str(row.get("source_record_id")) not in selected_record_ids:
      continue
    resolution = str(row.get("resolution"))
    if resolution not in {"excluded_source", "unresolved", "target_lineage_candidate"}:
      continue
    disposition = (
        "EXCLUDE_DEPENDENT_INSTRUCTION"
        if resolution in {"excluded_source", "unresolved"}
        else "EXCLUDE_AMBIGUOUS_LINEAGE_INSTRUCTION"
    )
    result.append({**row, "phase7_disposition": disposition, "final": True})
  return result


def _ensure_index_entry(path: Path, filename: str) -> bool:
  lines = path.read_text(encoding="utf-8").splitlines()
  entries = [line.strip() for line in lines if line.strip() and line.strip() != "$"]
  if filename not in entries:
    entries.append(filename)

  def index_key(entry: str) -> tuple[int, int | str, str]:
    stem = entry.split(".", 1)[0]
    return (0, int(stem), entry) if stem.isdigit() else (1, stem, entry)

  rendered = "".join(f"{entry}\n" for entry in sorted(set(entries), key=index_key)) + "$\n"
  previous = path.read_text(encoding="utf-8")
  if rendered == previous:
    return False
  path.write_text(rendered, encoding="utf-8", newline="\n")
  return True


def write_phase7_bundle(
    discovery_dir: Path,
    plan_dir: Path,
    capability_audit_dir: Path,
    phase6_dir: Path,
    completion_dir: Path,
    source_root: Path,
    target_world: Path,
    output_dir: Path,
    through_batch: int,
    prior_milestone_dirs: Iterable[Path] = (),
    created_at: str | None = None,
) -> dict[str, Any]:
  """Regenerate one cumulative Phase 7 milestone from the sealed Phase 6.5 baseline."""

  repo_root = default_repo_root()
  discovery_dir = discovery_dir.resolve()
  plan_dir = plan_dir.resolve()
  capability_audit_dir = capability_audit_dir.resolve()
  phase6_dir = phase6_dir.resolve()
  completion_dir = completion_dir.resolve()
  source_root = source_root.resolve()
  target_world = target_world.resolve()
  output_dir = output_dir.resolve()
  prior_milestone_dirs = [path.resolve() for path in prior_milestone_dirs]
  if output_dir.exists():
    raise RolPhase7Error(f"Phase 7 output directory already exists: {output_dir}")
  if source_root != (repo_root / _SOURCE_ROOT_PREFIX).resolve():
    raise RolPhase7Error("Phase 7 requires the inventoried repository RoL source root")
  if not target_world.is_dir():
    raise RolPhase7Error(f"Phase 6.5 target world is inaccessible: {target_world}")
  if through_batch < 1 or through_batch > 12:
    raise RolPhase7Error("--through-batch must be in the frozen range 1..12")

  discovery_manifest = _verify_bundle(discovery_dir, 1)
  plan_manifest = _verify_bundle(plan_dir, 2)
  capability_manifest = _verify_bundle(
      capability_audit_dir, 5, "full-corpus-capability-audit"
  )
  phase6_manifest = _verify_bundle(phase6_dir, 6, "special-procedure-reconciliation")
  completion_manifest = _verify_bundle(completion_dir, "6.5-completion-audit")
  if not completion_manifest.get("acceptance", {}).get("complete"):
    raise RolPhase7Error("Phase 7 requires the accepted Phase 6.5 completion bundle")
  if plan_manifest.get("discovery_run_id") != discovery_manifest["run_id"]:
    raise RolPhase7Error("Phase 1 and Phase 2 inputs do not share a run lineage")
  if phase6_manifest.get("discovery_run_id") != discovery_manifest["run_id"]:
    raise RolPhase7Error("Phase 6 input does not share the Phase 1 run lineage")

  actions, metadata = _augment_actions(discovery_dir, plan_dir)
  records, source_diagnostics = _source_records(repo_root, actions)
  references = _load_jsonl(discovery_dir / "reference-report.jsonl")
  inventory = _load_json(discovery_dir / "source-inventory.json")
  batches = _package_batches(inventory, metadata, actions, references)
  selected_packages = {
      package for batch in batches[:through_batch] for package in batch
  }
  selected_actions = [
      action for action in actions if str(action["basename"]) in selected_packages
  ]
  selected_record_ids = {str(action["source_record_id"]) for action in selected_actions}
  selected_records = [records[record_id] for record_id in sorted(selected_record_ids)]
  resolve = _typed_resolver(plan_dir, references)
  intervals = _zone_intervals(actions, records)
  target_ranges = _target_zone_ranges(selected_actions, records, intervals)
  zone_records: dict[tuple[str, int], RolRecord] = {}
  for record in records.values():
    if record.kind != "zon":
      continue
    bottom = 81700 if record.basename == "mytheast" else record.vnum * 100
    zone_records[(record.basename, bottom)] = record

  room_records = [record for record in records.values() if record.kind == "wld"]
  valid_exit_directions: defaultdict[int, set[int]] = defaultdict(set)
  for room_record in room_records:
    try:
      target_room = resolve("wld", room_record.vnum)
    except RolPhase7Error:
      continue
    valid_exit_directions[target_room]
    for directive in room_record.directives:
      arguments = list(directive.get("arguments", []))
      if (
          directive.get("token") != "D"
          or len(arguments) < 3
          or directive.get("source_defaulted_destination")
      ):
        continue
      try:
        if int(arguments[2]) > 0:
          resolve("wld", int(arguments[2]))
      except RolPhase7Error:
        continue
      valid_exit_directions[target_room].add(int(directive["direction"]))
  special_compilation = compile_special_bindings(
      _binding_rows(phase6_dir),
      _SPECIAL_TRIGGER_START,
      resolve,
      room_records,
  )
  if (
      special_compilation.source_bindings != 1721
      or len(special_compilation.native_bindings) != 1562
      or len(special_compilation.triggers) != 14
  ):
    raise RolPhase7Error("full special compilation drifted from the sealed Phase 6 measurements")
  native = _aggregate_native_bindings(special_compilation)

  command_evidence = extract_source_commands(source_root)
  source_commands = {
      int(row["action_code"]): str(row["command"])
      for row in command_evidence["commands"]
  }
  soc_records = [record for record in selected_records if record.kind == "soc"]
  soc = _compile_soc_corpus(
      soc_records,
      resolve,
      source_commands,
      intervals,
      _trigger_vnums(target_world / "trg"),
  )

  attachments: defaultdict[tuple[str, int], list[int]] = defaultdict(list)
  for owner, trigger_ids in soc.attachments.items():
    attachments[("mob", owner)].extend(trigger_ids)
  for owner, trigger_ids in special_compilation.attachments.items():
    attachments[owner].extend(trigger_ids)
  for owner in list(attachments):
    attachments[owner] = sorted(set(attachments[owner]))

  inert_special_sources = {
      (str(row["source_record_type"]), int(row["source_vnum"]))
      for row in special_compilation.dispositions
      if row["strategy"] in {"SOURCE_INERT_EXCLUDED", "MINIMAL_DEPENDENCY_EXCLUSION"}
  }
  generated: defaultdict[str, list[tuple[int, str, str]]] = defaultdict(list)
  quest_fragments: defaultdict[int, list[tuple[str, str, str]]] = defaultdict(list)
  zone_outputs: defaultdict[int, list[tuple[int, str, str]]] = defaultdict(list)
  transform_diagnostics: list[dict[str, Any]] = []
  record_outputs: dict[str, str] = {}

  for action in selected_actions:
    kind = str(action["source_kind"])
    record_id = str(action["source_record_id"])
    if kind in {"soc", "zon"} or action["action"] == "EXCLUDE":
      continue
    if action["destination_vnum"] is None:
      continue
    destination = int(action["destination_vnum"])
    record = records[record_id]
    target_kind = _KIND_OWNER.get(kind, kind)
    binding = native.get((target_kind, destination))
    owner_attachments = tuple(attachments.get((target_kind, destination), ()))
    emitted: TransformResult
    if kind == "mob":
      emitted = emit_mobile(
          record,
          destination,
          special_proc=binding.persisted_name if binding is not None else None,
          special_resolved=("mobile", record.vnum) in inert_special_sources
          or binding is not None,
          attachments=owner_attachments,
          required_action_bits=binding.required_flag_bits if binding is not None else (),
          required_affect_bits=binding.required_affect_bits if binding is not None else (),
      )
    elif kind == "obj":
      emitted = emit_object(
          record,
          destination,
          resolve,
          special_proc=binding.persisted_name if binding is not None else None,
          attachments=owner_attachments,
          required_extra_bits=binding.required_flag_bits if binding is not None else (),
          required_value_references=(
              binding.value_reference_slots if binding is not None else ()
          ),
      )
    elif kind == "wld":
      source_zone = _record_zone(record.basename, record.vnum, destination, intervals)
      zone = _target_room_zone(destination, source_zone, target_ranges)
      emitted = emit_room(
          record,
          destination,
          zone,
          resolve,
          special_proc=binding.persisted_name if binding is not None else None,
          attachments=owner_attachments,
          source_zone_flags=_source_zone_flags(
              record.basename, record.vnum, intervals, zone_records
          ),
          required_flag_bits=binding.required_flag_bits if binding is not None else (),
      )
    elif kind == "qst":
      emitted = emit_hlquest(record, destination, resolve)
    elif kind == "shp":
      emitted = emit_shop(record, destination, resolve)
    else:
      raise RolPhase7Error(f"no Phase 7 emitter for {kind}")
    zone = _record_zone(record.basename, record.vnum, destination, intervals)
    directory, extension = _KIND_DIRECTORY[kind]
    relative = f"{directory}/{zone}.{extension}"
    if kind == "qst":
      quest_fragments[destination].append((relative, emitted.text, record_id))
    else:
      generated[relative].append((destination, emitted.text, record_id))
      record_outputs[record_id] = relative
    transform_diagnostics.extend(
        {
            "source_record_id": record_id,
            "target": relative,
            "message": message,
        }
        for message in emitted.diagnostics
    )

  for destination, fragments in sorted(quest_fragments.items()):
    ordered = sorted(fragments, key=lambda row: (row[0], row[2]))
    relative = ordered[0][0]
    blocks = [(destination, text, record_id) for _, text, record_id in ordered]
    generated[relative].append(
        (
            destination,
            _merge_hlquest_blocks(blocks),
            "+".join(record_id for _, _, record_id in blocks),
        )
    )
    for _, _, record_id in ordered:
      record_outputs[record_id] = relative

  for action in selected_actions:
    if (
        action["source_kind"] != "zon"
        or action["action"] == "EXCLUDE"
        or action["destination_vnum"] is None
    ):
      continue
    record_id = str(action["source_record_id"])
    record = records[record_id]
    destination = int(action["destination_vnum"])
    interval = next(
        (
            (bottom, top)
            for bottom, top, zone in intervals[record.basename]
            if zone == destination
            and (bottom == (81700 if record.basename == "mytheast" else record.vnum * 100))
        ),
        None,
    )
    if interval is None:
      raise RolPhase7Error(f"cannot recover source interval for {record_id}")
    source_top = int(list(record.values.get("header", []))[0])
    destination_bottom, destination_top = target_ranges[destination]

    def zone_resolve(kind: str, source_vnum: int) -> int:
      if kind == "wld" and source_vnum == source_top:
        return destination_top
      return resolve(kind, source_vnum)

    emitted = emit_zone(
        record,
        destination,
        destination_bottom,
        zone_resolve,
    )
    filtered_text, filter_diagnostics = _filter_zone_door_resets(
        emitted.text, valid_exit_directions
    )
    emitted = TransformResult(filtered_text, [*emitted.diagnostics, *filter_diagnostics])
    zone_outputs[destination].append((destination, emitted.text, record_id))
    transform_diagnostics.extend(
        {
            "source_record_id": record_id,
            "target": f"zon/{destination}.zon",
            "message": message,
        }
        for message in emitted.diagnostics
    )
  for destination, blocks in sorted(zone_outputs.items()):
    relative = f"zon/{destination}.zon"
    generated[relative].append(
        (
            destination,
            _merge_zone_blocks(blocks),
            "+".join(record_id for _, _, record_id in blocks),
        )
    )
    for _, _, record_id in blocks:
      record_outputs[record_id] = relative

  for trigger in [*soc.triggers, *special_compilation.triggers]:
    relative = f"trg/{trigger.vnum // 100}.trg"
    trigger_text, trigger_diagnostics = _bound_trigger_text(trigger.vnum, trigger.text)
    generated[relative].append(
        (trigger.vnum, trigger_text, f"trigger:{trigger.vnum}")
    )
    transform_diagnostics.extend(
        {
            "source_record_id": f"trigger:{trigger.vnum}",
            "target": relative,
            "message": message,
        }
        for message in trigger_diagnostics
    )

  output_dir.mkdir(parents=True)
  staging_world = output_dir / "staging/world"
  shutil.copytree(target_world, staging_world, copy_function=shutil.copy2)
  existing_trigger_ids = _trigger_vnums(staging_world / "trg")
  touched: set[str] = set()
  for relative, rows in sorted(generated.items()):
    kind = relative.split("/", 1)[0]
    replacements: set[int] = (
        {vnum for vnum, _, _ in rows if vnum in existing_trigger_ids}
        if kind == "trg"
        else set()
    )
    target_path = staging_world / relative
    if kind == "zon" and target_path.exists():
      _, baseline_blocks = _split_world_file(target_path, kind)
      baseline_by_vnum: defaultdict[int, list[str]] = defaultdict(list)
      for vnum, text in baseline_blocks:
        baseline_by_vnum[vnum].append(text)
      merged_rows: list[tuple[int, str, str]] = []
      for vnum, text, record_id in rows:
        if vnum in baseline_by_vnum:
          if len(baseline_by_vnum[vnum]) != 1:
            raise RolPhase7Error(f"baseline zone {vnum} is not unique in {target_path}")
          text = _merge_zone_blocks(
              [(vnum, baseline_by_vnum[vnum][0], "phase6.5-baseline"),
               (vnum, text, record_id)]
          )
          replacements.add(vnum)
        merged_rows.append((vnum, text, record_id))
      rows = merged_rows
    _merge_world_records(target_path, kind, rows, replacements)
    touched.add(relative)
    filename = relative.split("/", 1)[1]
    index_path = staging_world / kind / "index"
    if _ensure_index_entry(index_path, filename):
      touched.add(f"{kind}/index")

  patches: dict[
      tuple[str, int],
      tuple[str | None, tuple[int, ...], tuple[int, ...], tuple[int, ...]],
  ] = {}

  for relative in sorted(touched):
    source = staging_world / relative
    destination = output_dir / "output/world" / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)

  baseline_tree = tree_manifest(target_world)
  staged_tree = tree_manifest(staging_world)
  preservation = _preservation_check(baseline_tree, staged_tree, touched)

  manifest_constants = load_manifest(repo_root / "scripts/world/wtool_constants.json")
  validation_config = resolve_config(target_world, None)
  baseline_validation_result = validate_indexed_world(
      target_world, repo_root, manifest_constants, validation_config
  )
  staged_validation_result = validate_indexed_world(
      staging_world, repo_root, manifest_constants, validation_config
  )
  baseline_validation = result_payload(baseline_validation_result)
  staged_validation = result_payload(staged_validation_result)
  baseline_validation["root"] = "sealed-phase6-5/world"
  staged_validation["root"] = "phase7-candidate/world"
  delta = _validation_delta(baseline_validation, staged_validation)
  staged_model = load_indexed_world_data(
      staging_world, repo_root, manifest_constants, validation_config
  )
  selected_zones = {
      int(action["destination_vnum"])
      for action in selected_actions
      if action["source_kind"] == "zon" and action["destination_vnum"] is not None
  }
  runtime = _runtime_contract(staged_model, selected_zones)
  connection_graph = audit_connection_graph(
      selected_records,
      selected_actions,
      staged_model.rooms,
      (
          ("mob", staged_model.mobiles),
          ("obj", staged_model.objects),
          ("shp", staged_model.shops),
          ("qst", staged_model.hlquests),
      ),
  )

  dispositions: list[dict[str, Any]] = []
  for action in selected_actions:
    record_id = str(action["source_record_id"])
    kind = str(action["source_kind"])
    destination = action["destination_vnum"]
    if record_id in _SOC_OWNER_EXCLUSIONS or action["action"] == "EXCLUDE":
      final_action = "EXCLUDE"
      strategy = "SMALLEST_UNIT_EXCLUSION"
    elif action["action"] == "MERGE":
      final_action = "MERGE"
      strategy = "GENERATED_CANONICAL_MERGE"
    elif kind == "soc":
      final_action = "ADD"
      strategy = "DG_COMPILED"
    else:
      final_action = "ADD"
      strategy = "GENERATED_CANONICAL_ADD"
    row = {
        "batch": next(
            index
            for index, batch in enumerate(batches, start=1)
            if str(action["basename"]) in batch
        ),
        "basename": action["basename"],
        "source_record_id": record_id,
        "source_kind": kind,
        "source_vnum": action["source_vnum"],
        "destination_vnum": destination,
        "planned_action": action["action"],
        "final_action": final_action,
        "strategy": strategy,
    }
    if record_id in record_outputs:
      row["output"] = record_outputs[record_id]
    dispositions.append(row)

  reference_exceptions = _selected_reference_exceptions(references, selected_record_ids)
  batch_rows = [
      {
          "batch": index,
          "packages": batch,
          "package_count": len(batch),
          "selected": index <= through_batch,
          "milestone_after": index in {4, 8, 12},
          "record_count": sum(
              1 for action in actions if str(action["basename"]) in set(batch)
          ),
      }
      for index, batch in enumerate(batches, start=1)
  ]
  package_rows = [
      {
          "basename": package,
          "batch": next(index for index, batch in enumerate(batches, start=1) if package in batch),
          "selected": package in selected_packages,
          "record_count": sum(1 for action in actions if action["basename"] == package),
      }
      for package in sorted(package for batch in batches for package in batch)
  ]

  freeze = {
      "discovery_run_id": discovery_manifest["run_id"],
      "plan_run_id": plan_manifest["run_id"],
      "capability_audit_run_id": capability_manifest["run_id"],
      "phase6_run_id": phase6_manifest["run_id"],
      "phase6_5_completion_run_id": completion_manifest["run_id"],
      "source_tree": tree_manifest(source_root),
      "target_tree": baseline_tree,
  }
  evidence: dict[str, Any] = {
      "freeze.json": freeze,
      "dependency-batches.json": {"batch_count": len(batches), "batches": batch_rows},
      "compiler-summary.json": {
          "selected_packages": len(selected_packages),
          "selected_records": len(selected_actions),
          "source_diagnostics": len(source_diagnostics),
          "transform_diagnostics": len(transform_diagnostics),
          "soc_source_records": soc.source_records,
          "soc_source_actions": soc.source_actions,
          "soc_triggers": len(soc.triggers),
          "soc_attachment_owners": len(soc.attachments),
          "special_source_bindings": special_compilation.source_bindings,
          "native_special_rows": len(special_compilation.native_bindings),
          "native_special_targets": len(native),
          "special_triggers": len(special_compilation.triggers),
          "composite_special_targets": len(_COMPOSITE_SPECIALS),
      },
      "change-plan.json": {
          "through_batch": through_batch,
          "generated_files": sorted(generated),
          "patched_targets": [f"{kind}:{vnum}" for kind, vnum in sorted(patches)],
          "touched_paths": sorted(touched),
          "live_target_writes": 0,
      },
      "diagnostics/source.json": source_diagnostics,
      "diagnostics/transforms.json": transform_diagnostics,
      "diagnostics/soc.json": soc.diagnostics,
      "diagnostics/specials.json": special_compilation.diagnostics,
      "validation/baseline.json": baseline_validation,
      "validation/staged.json": staged_validation,
      "validation/delta.json": delta,
      "validation/runtime-contract.json": runtime,
      "validation/connection-graph.json": connection_graph,
      "validation/preservation.json": preservation,
      "validation/staged-tree.json": staged_tree,
  }

  artifacts: list[dict[str, Any]] = []
  for relative, payload in sorted(evidence.items()):
    path = output_dir / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(_canonical_json(payload))
    artifacts.append(_artifact(path, output_dir))
  for relative in sorted(touched):
    artifacts.append(_artifact(output_dir / "output/world" / relative, output_dir))
  for relative, rows in (
      ("action-ledger.jsonl", dispositions),
      ("reference-exceptions.jsonl", reference_exceptions),
      ("package-ledger.jsonl", package_rows),
  ):
    path = output_dir / relative
    count = _write_jsonl(path, rows)
    artifacts.append(_artifact(path, output_dir, count))

  prior_runs: list[dict[str, Any]] = []
  for prior_dir in prior_milestone_dirs:
    prior = _verify_bundle(prior_dir, 7, "cumulative-milestone")
    if prior.get("through_batch", 0) >= through_batch:
      raise RolPhase7Error("prior Phase 7 milestone is not earlier than this milestone")
    if prior.get("frozen_target_tree_sha256") != baseline_tree["tree_sha256"]:
      raise RolPhase7Error("prior milestone does not share the sealed Phase 6.5 baseline")
    prior_runs.append(
        {"run_id": prior["run_id"], "through_batch": prior["through_batch"]}
    )

  seed = "\n".join(
      artifact["sha256"]
      for artifact in sorted(artifacts, key=lambda item: item["path"])
      if not artifact["path"].startswith("validation/")
  ).encode("ascii")
  run_id = f"rol-phase7-b{through_batch}-{hashlib.sha256(seed).hexdigest()[:16]}"
  final = through_batch == 12
  source_parse_complete = not any(
      row.get("severity") == "error" for row in source_diagnostics
  )
  manifest = {
      "schema_version": ROL_PHASE7_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 7,
      "stage": "cumulative-milestone",
      "through_batch": through_batch,
      "final_milestone": final,
      "frozen_target_tree_sha256": baseline_tree["tree_sha256"],
      "staged_tree_sha256": staged_tree["tree_sha256"],
      "input_runs": freeze | {"prior_milestones": prior_runs},
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": {
          "dependency_batches": len(batches),
          "selected_packages": len(selected_packages),
          "selected_records": len(selected_actions),
          "all_selected_records_disposed": len(dispositions) == len(selected_actions),
          "reference_exceptions_final": all(row["final"] for row in reference_exceptions),
          "source_parse_complete": source_parse_complete,
          "special_bindings_accounted": special_compilation.source_bindings == 1721,
          "composite_profiles_accounted": len(_COMPOSITE_SPECIALS) == 14,
          "soc_records_accounted": soc.source_records
          == sum(record.kind == "soc" for record in selected_records) - sum(
              record.record_id in _SOC_OWNER_EXCLUSIONS for record in selected_records
          ),
          "staged_new_active_errors": delta["new_active_errors"],
          "staged_without_new_errors": delta["new_active_errors"] == 0,
          "runtime_contract_pass": runtime["all_pass"],
          "connection_graph_pass": connection_graph["summary"]["pass"],
          "preservation_pass": preservation["pass"],
          "live_target_writes": 0,
          "final_records": len(dispositions) if final else None,
          "final_packages": len(selected_packages) if final else None,
          "phase7_complete": final
          and len(dispositions) == 71680
          and len(selected_packages) == 258
          and source_parse_complete
          and delta["new_active_errors"] == 0
          and runtime["all_pass"]
          and connection_graph["summary"]["pass"]
          and preservation["pass"],
      },
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(manifest))
  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "through_batch": through_batch,
      "selected_packages": len(selected_packages),
      "selected_records": len(selected_actions),
      "generated_files": len(generated),
      "patched_records": len(patches),
      "soc_triggers": len(soc.triggers),
      "special_triggers": len(special_compilation.triggers),
      "new_active_errors": delta["new_active_errors"],
      "runtime_contract_pass": runtime["all_pass"],
      "connection_graph_pass": connection_graph["summary"]["pass"],
      "preservation_pass": preservation["pass"],
      "phase7_complete": manifest["acceptance"]["phase7_complete"],
      "artifacts": len(artifacts) + 1,
  }


def render_rol_phase7_human(summary: dict[str, Any]) -> str:
  return (
      f"RoL Phase 7 milestone: {summary['run_id']}\n"
      f"Output: {summary['output_dir']}\n"
      f"Batches complete: {summary['through_batch']}/12\n"
      f"Packages: {summary['selected_packages']}/258\n"
      f"Records disposed: {summary['selected_records']}/71680\n"
      f"Generated files: {summary['generated_files']}\n"
      f"Patched preserved records: {summary['patched_records']}\n"
      f"SOC triggers: {summary['soc_triggers']}\n"
      f"Special triggers: {summary['special_triggers']}\n"
      f"New active errors: {summary['new_active_errors']}\n"
      f"Runtime contract pass: {str(summary['runtime_contract_pass']).lower()}\n"
      f"Preservation pass: {str(summary['preservation_pass']).lower()}\n"
      f"Phase 7 complete: {str(summary['phase7_complete']).lower()}\n"
  )
