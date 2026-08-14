"""Canonical Realms of Luminari VNUM identity resolution."""

from __future__ import annotations

from typing import Any


_CORE_KINDS = frozenset({"zon", "wld", "mob", "obj"})
_ENTITY_TARGET_TYPES = frozenset({"room", "mobile", "object", "shop", "quest", "hlquest"})


class RolIdentityError(ValueError):
  """Raised when a source identity cannot satisfy the canonical VNUM contract."""


def _integer(value: Any, label: str) -> int:
  if isinstance(value, bool) or not isinstance(value, int):
    raise RolIdentityError(f"{label} must be an integer")
  return value


def _identity_policy(policy: dict[str, Any]) -> dict[str, Any]:
  identity = policy.get("identity")
  if not isinstance(identity, dict):
    raise RolIdentityError("policy is missing the identity object")
  formula = identity.get("canonical_formula")
  if formula is None:
    formula = {
        "zone_offset": identity.get("new_zone_range", {}).get("offset"),
        "entity_offset": identity.get("new_entity_range", {}).get("offset"),
    }
  if not isinstance(formula, dict):
    raise RolIdentityError("identity.canonical_formula must be an object")
  zone_offset = _integer(formula.get("zone_offset"), "canonical zone offset")
  entity_offset = _integer(formula.get("entity_offset"), "canonical entity offset")
  if zone_offset != 20000 or entity_offset != 2000000:
    raise RolIdentityError("canonical offsets must be zone +20000 and entity +2000000")
  return identity


def _normalization(
    policy: dict[str, Any], kind: str, vnum: int, basename: str
) -> dict[str, Any] | None:
  if kind != "zon":
    return None
  identity = _identity_policy(policy)
  matches = [
      item
      for item in identity.get("normalizations", [])
      if item.get("source_basename") == basename
      or (
          item.get("source_header_vnum") == vnum
          and item.get("source_basename") == basename
      )
  ]
  if len(matches) > 1:
    raise RolIdentityError(f"multiple normalizations match zone {basename} #{vnum}")
  if not matches:
    return None
  item = matches[0]
  required = {
      "source_basename",
      "source_header_vnum",
      "source_top_vnum",
      "logical_source_zone",
      "logical_source_zones",
      "target_zone_vnum",
      "evidence",
  }
  missing = sorted(required - set(item))
  if missing:
    raise RolIdentityError(
        f"normalization for {basename} is missing: {', '.join(missing)}"
    )
  if item["source_header_vnum"] != vnum:
    raise RolIdentityError(
        f"normalization for {basename} expects header {item['source_header_vnum']}, not {vnum}"
    )
  logical = _integer(item["logical_source_zone"], "logical source zone")
  target = _integer(item["target_zone_vnum"], "normalized target zone")
  if target != logical + 20000:
    raise RolIdentityError(
        f"normalization for {basename} violates the canonical zone formula"
    )
  logical_zones = item["logical_source_zones"]
  if not isinstance(logical_zones, list) or logical not in logical_zones:
    raise RolIdentityError(
        f"normalization for {basename} must include its logical source zone"
    )
  evidence = item["evidence"]
  if not isinstance(evidence, list) or len(evidence) < 3 or not all(evidence):
    raise RolIdentityError(
        f"normalization for {basename} requires manifest, range, and contained-record evidence"
    )
  return item


def canonical_destination(
    kind: str,
    vnum: int,
    basename: str,
    policy: dict[str, Any],
) -> int:
  """Resolve one core source record to its only permitted canonical destination."""

  identity = _identity_policy(policy)
  if kind not in _CORE_KINDS:
    raise RolIdentityError(f"unsupported canonical source kind {kind!r}")
  if vnum < 0:
    raise RolIdentityError(f"source {kind} VNUM must be non-negative")
  if kind == "zon":
    normalized = _normalization(policy, kind, vnum, basename)
    destination = (
        normalized["target_zone_vnum"] if normalized is not None else vnum + 20000
    )
    selected_range = identity.get("new_zone_range", {})
  else:
    destination = vnum + 2000000
    selected_range = identity.get("new_entity_range", {})
  start = _integer(selected_range.get("start"), f"{kind} destination range start")
  end = _integer(selected_range.get("end"), f"{kind} destination range end")
  if not start <= destination <= end:
    raise RolIdentityError(
        f"canonical destination {destination} for {kind} {vnum} is outside {start}..{end}"
    )
  return destination


def canonical_reference_vnum(target_type: str, vnum: int) -> int:
  """Map an active source reference to the canonical target typed namespace."""

  if target_type == "zone":
    destination = vnum + 20000
    if not 20000 <= destination <= 29999:
      raise RolIdentityError(
          f"canonical zone reference {destination} is outside 20000..29999"
      )
    return destination
  if target_type in _ENTITY_TARGET_TYPES:
    destination = vnum + 2000000
    if not 2000000 <= destination <= 2999999:
      raise RolIdentityError(
          f"canonical {target_type} reference {destination} is outside 2000000..2999999"
      )
    return destination
  raise RolIdentityError(f"unsupported canonical reference type {target_type!r}")


def legacy_lineage_vnum(target_type: str, vnum: int) -> int:
  """Return the historic offset candidate used only as content-lineage evidence."""

  return vnum + (1000 if target_type == "zone" else 100000)
