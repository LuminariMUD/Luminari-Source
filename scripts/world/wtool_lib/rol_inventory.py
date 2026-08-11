"""Deterministic inventory of the Realms of Luminari authoring corpus."""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
from pathlib import Path
from typing import Any

from .indexes import is_safe_path_component, normalized_root_label
from .models import SourceSpan, TOOL_VERSION
from .source import SourceFile, parse_c_integer_token


ROL_INVENTORY_SCHEMA_VERSION = 1
BUILD_LINE_BYTES = 8190
BUILD_BASENAME_BYTES = 79
SOURCE_KINDS = ("zon", "wld", "mob", "obj", "shp", "qst", "soc")
KIND_MANIFEST = {
    "zon": "AREA",
    "wld": "AREA",
    "mob": "AREA.mobobj",
    "obj": "AREA.mobobj",
    "shp": "SHOP",
    "qst": "QUEST",
    "soc": "AREA",
}
MANIFEST_KINDS = {
    manifest: tuple(kind for kind in SOURCE_KINDS if KIND_MANIFEST[kind] == manifest)
    for manifest in ("AREA", "AREA.mobobj", "SHOP", "QUEST")
}


class RolInventoryError(ValueError):
  """Raised when source manifests or inventory inputs are malformed."""


@dataclass(frozen=True, slots=True)
class ManifestEntry:
  basename: str
  line: int

  def to_dict(self) -> dict[str, Any]:
    return {"basename": self.basename, "line": self.line}


@dataclass(frozen=True, slots=True)
class ParsedManifest:
  name: str
  path: str
  byte_size: int
  sha256: str
  active: tuple[ManifestEntry, ...]
  disabled: tuple[ManifestEntry, ...]

  @property
  def active_by_basename(self) -> dict[str, ManifestEntry]:
    return {entry.basename: entry for entry in self.active}

  @property
  def disabled_by_basename(self) -> dict[str, ManifestEntry]:
    return {entry.basename: entry for entry in self.disabled}

  def to_dict(self) -> dict[str, Any]:
    return {
        "name": self.name,
        "path": self.path,
        "byte_size": self.byte_size,
        "sha256": self.sha256,
        "kinds": list(MANIFEST_KINDS[self.name]),
        "active": [entry.to_dict() for entry in self.active],
        "disabled": [entry.to_dict() for entry in self.disabled],
    }


@dataclass(frozen=True, slots=True)
class InventoryIssue:
  message: str
  span: SourceSpan

  def render(self) -> str:
    return f"{self.span.path}:{self.span.line}: {self.message}"


def _digest_bytes(data: bytes) -> str:
  return hashlib.sha256(data).hexdigest()


def _digest_path(path: Path) -> str:
  digest = hashlib.sha256()
  with path.open("rb") as source:
    while chunk := source.read(1024 * 1024):
      digest.update(chunk)
  return digest.hexdigest()


def _decode_basename(token: bytes, span: SourceSpan) -> tuple[str | None, InventoryIssue | None]:
  try:
    basename = token.decode("ascii")
  except UnicodeDecodeError:
    return None, InventoryIssue("manifest basename must contain only ASCII bytes", span)
  if len(token) > BUILD_BASENAME_BYTES:
    return None, InventoryIssue(
        f"manifest basename is {len(token)} bytes; build_areas.c buffers at most "
        f"{BUILD_BASENAME_BYTES}",
        span,
    )
  if not is_safe_path_component(basename):
    return None, InventoryIssue(
        f"manifest basename {basename!r} is not a safe path component",
        span,
    )
  return basename, None


def _raise_issues(label: str, issues: list[InventoryIssue]) -> None:
  if not issues:
    return
  rendered = "\n".join(issue.render() for issue in issues)
  raise RolInventoryError(f"{label}:\n{rendered}")


def parse_rol_manifest(path: Path, display_path: str) -> ParsedManifest:
  """Parse one source build list with build_areas.c first-column rules."""

  source = SourceFile.from_path(path, display_path)
  active: list[ManifestEntry] = []
  disabled: list[ManifestEntry] = []
  active_seen: dict[str, int] = {}
  disabled_seen: set[str] = set()
  issues: list[InventoryIssue] = []

  for line in source.lines:
    if len(line.raw) + len(line.newline) > BUILD_LINE_BYTES:
      issues.append(
          InventoryIssue(
              f"physical manifest line exceeds the {BUILD_LINE_BYTES}-byte build reader limit",
              line.span,
          )
      )
      continue

    if line.raw.startswith(b"*"):
      remainder = line.raw[1:]
      if not remainder or remainder[:1].isspace():
        continue
      token = remainder.split(maxsplit=1)[0]
      basename, issue = _decode_basename(token, line.span)
      if issue is not None:
        issues.append(issue)
      elif basename not in disabled_seen:
        disabled_seen.add(basename)
        disabled.append(ManifestEntry(basename, line.number))
      continue

    tokens = line.raw.split(maxsplit=1)
    if not tokens:
      issues.append(
          InventoryIssue(
              "active manifest line does not contain a basename; build_areas.c would reuse "
              "stale scan data",
              line.span,
          )
      )
      continue
    basename, issue = _decode_basename(tokens[0], line.span)
    if issue is not None:
      issues.append(issue)
      continue
    if basename in active_seen:
      issues.append(
          InventoryIssue(
              f"duplicate active basename {basename!r}; first selected on line "
              f"{active_seen[basename]}",
              line.span,
          )
      )
      continue
    active_seen[basename] = line.number
    active.append(ManifestEntry(basename, line.number))

  _raise_issues("malformed RoL manifest entries", issues)
  return ParsedManifest(
      name=path.name,
      path=display_path,
      byte_size=len(source.data),
      sha256=_digest_bytes(source.data),
      active=tuple(active),
      disabled=tuple(disabled),
  )


def _zone_records(path: Path, display_path: str) -> list[dict[str, int]]:
  source = SourceFile.from_path(path, display_path)
  records: list[dict[str, int]] = []
  issues: list[InventoryIssue] = []
  for line in source.lines:
    if not line.raw.startswith(b"#"):
      continue
    parsed = parse_c_integer_token(line.text[1:].strip())
    if parsed.error is not None or parsed.value is None or parsed.value < 0:
      reason = parsed.error or "zone record number must be non-negative"
      issues.append(InventoryIssue(f"malformed zone record header: {reason}", line.span))
      continue
    records.append({"vnum": parsed.value, "line": line.number})
  _raise_issues("malformed RoL zone headers", issues)
  return records


def _load_manifests(areas_root: Path) -> dict[str, ParsedManifest]:
  manifests: dict[str, ParsedManifest] = {}
  missing = [name for name in MANIFEST_KINDS if not (areas_root / name).is_file()]
  if missing:
    paths = ", ".join(f"areas/{name}" for name in missing)
    raise RolInventoryError(f"required RoL build manifest is missing: {paths}")
  for name in MANIFEST_KINDS:
    manifests[name] = parse_rol_manifest(areas_root / name, f"areas/{name}")
  return manifests


def _enumerate_files(
    areas_root: Path,
    manifests: dict[str, ParsedManifest],
) -> tuple[list[dict[str, Any]], dict[str, dict[str, dict[str, Any]]]]:
  files: list[dict[str, Any]] = []
  by_kind: dict[str, dict[str, dict[str, Any]]] = {}
  issues: list[InventoryIssue] = []

  for kind in SOURCE_KINDS:
    directory = areas_root / kind
    if not directory.is_dir():
      raise RolInventoryError(f"required RoL data directory is missing: areas/{kind}")
    suffix = f".{kind}"
    manifest = manifests[KIND_MANIFEST[kind]]
    active = manifest.active_by_basename
    disabled = manifest.disabled_by_basename
    by_kind[kind] = {}

    paths = sorted(
        (path for path in directory.iterdir() if path.is_file() and path.name.endswith(suffix)),
        key=lambda item: item.name,
    )
    for path in paths:
      basename = path.name[: -len(suffix)]
      display_path = f"areas/{kind}/{path.name}"
      if not is_safe_path_component(basename):
        issues.append(
            InventoryIssue(
                f"data basename {basename!r} is not a safe path component",
                SourceSpan(display_path, 1),
            )
        )
        continue
      if basename in active:
        status = "active"
        manifest_line = active[basename].line
      elif basename in disabled:
        status = "disabled"
        manifest_line = disabled[basename].line
      else:
        status = "unlisted"
        manifest_line = None

      zone_records = _zone_records(path, display_path) if kind == "zon" else []
      classifications = [status]
      if len(zone_records) > 1:
        classifications.append("multi-zone")
      record: dict[str, Any] = {
          "basename": basename,
          "kind": kind,
          "path": display_path,
          "byte_size": path.stat().st_size,
          "sha256": _digest_path(path),
          "manifest": manifest.name,
          "manifest_line": manifest_line,
          "status": status,
          "included": status == "active",
          "classifications": classifications,
      }
      if kind == "zon":
        record["zone_records"] = zone_records
      files.append(record)
      by_kind[kind][basename] = record

  _raise_issues("malformed RoL inventory paths", issues)
  return files, by_kind


def _build_packages(
    manifests: dict[str, ParsedManifest],
    by_kind: dict[str, dict[str, dict[str, Any]]],
) -> list[dict[str, Any]]:
  selected: dict[str, set[str]] = {
      kind: set(manifests[KIND_MANIFEST[kind]].active_by_basename) for kind in SOURCE_KINDS
  }
  basenames = set().union(*(set(entries) for entries in by_kind.values()), *selected.values())
  packages: list[dict[str, Any]] = []

  for basename in sorted(basenames):
    present_kinds = [kind for kind in SOURCE_KINDS if basename in by_kind[kind]]
    selected_kinds = [kind for kind in SOURCE_KINDS if basename in selected[kind]]
    included_kinds = [
        kind
        for kind in present_kinds
        if by_kind[kind][basename]["status"] == "active"
    ]
    disabled_kinds = [
        kind
        for kind in present_kinds
        if by_kind[kind][basename]["status"] == "disabled"
    ]
    unlisted_kinds = [
        kind
        for kind in present_kinds
        if by_kind[kind][basename]["status"] == "unlisted"
    ]
    missing_kinds = [kind for kind in selected_kinds if kind not in present_kinds]
    zone_records = (
        by_kind["zon"][basename].get("zone_records", []) if basename in by_kind["zon"] else []
    )
    classifications: list[str] = []
    if selected_kinds:
      classifications.append("active")
    if disabled_kinds:
      classifications.append("disabled")
    if unlisted_kinds:
      classifications.append("unlisted")
    if missing_kinds:
      classifications.append("missing-companion")
    if len(zone_records) > 1:
      classifications.append("multi-zone")

    packages.append(
        {
            "basename": basename,
            "included": bool(included_kinds),
            "present_kinds": present_kinds,
            "selected_kinds": selected_kinds,
            "included_kinds": included_kinds,
            "disabled_kinds": disabled_kinds,
            "unlisted_kinds": unlisted_kinds,
            "missing_companion_kinds": missing_kinds,
            "zone_records": zone_records,
            "classifications": classifications,
        }
    )
  return packages


def _kind_summaries(
    manifests: dict[str, ParsedManifest],
    by_kind: dict[str, dict[str, dict[str, Any]]],
) -> list[dict[str, Any]]:
  summaries: list[dict[str, Any]] = []
  for kind in SOURCE_KINDS:
    manifest = manifests[KIND_MANIFEST[kind]]
    files = list(by_kind[kind].values())
    present = set(by_kind[kind])
    active_selected = set(manifest.active_by_basename)
    summaries.append(
        {
            "kind": kind,
            "manifest": manifest.name,
            "files": len(files),
            "active_files": sum(record["status"] == "active" for record in files),
            "disabled_files": sum(record["status"] == "disabled" for record in files),
            "unlisted_files": sum(record["status"] == "unlisted" for record in files),
            "missing_companions": len(active_selected - present),
        }
    )
  return summaries


def build_rol_inventory(source_root: Path, repo_root: Path) -> dict[str, Any]:
  """Build a stable, read-only inventory for one RoL source checkout."""

  source_root = source_root.resolve()
  areas_root = source_root / "areas"
  if not areas_root.is_dir():
    raise RolInventoryError(f"requested RoL source root has no areas directory: {source_root}")

  manifests = _load_manifests(areas_root)
  files, by_kind = _enumerate_files(areas_root, manifests)
  packages = _build_packages(manifests, by_kind)
  kind_summaries = _kind_summaries(manifests, by_kind)

  zone_files = list(by_kind["zon"].values())
  active_zone_files = [record for record in zone_files if record["status"] == "active"]
  disabled_zone_files = sorted(
      record["basename"] for record in zone_files if record["status"] == "disabled"
  )
  unlisted_zone_files = sorted(
      record["basename"] for record in zone_files if record["status"] == "unlisted"
  )
  active_without_zone = sorted(
      package["basename"]
      for package in packages
      if package["selected_kinds"] and "zon" not in package["present_kinds"]
  )
  active_multi_zone = [
      {
          "basename": record["basename"],
          "record_vnums": [zone["vnum"] for zone in record["zone_records"]],
      }
      for record in active_zone_files
      if len(record["zone_records"]) > 1
  ]
  companion_only_files = sorted(
      record["path"]
      for record in files
      if record["status"] == "active"
      and record["kind"] != "zon"
      and record["basename"] not in by_kind["zon"]
  )
  classifications = {
      "active_basenames_without_zone": active_without_zone,
      "active_companion_only_files": companion_only_files,
      "active_multi_zone_files": active_multi_zone,
      "disabled_zone_files": disabled_zone_files,
      "unlisted_zone_files": unlisted_zone_files,
  }
  summary = {
      "files": {
          "total": len(files),
          "active": sum(record["status"] == "active" for record in files),
          "disabled": sum(record["status"] == "disabled" for record in files),
          "unlisted": sum(record["status"] == "unlisted" for record in files),
      },
      "zones": {
          "physical_files": len(zone_files),
          "active_files": len(active_zone_files),
          "active_records": sum(len(record["zone_records"]) for record in active_zone_files),
          "disabled_files": len(disabled_zone_files),
          "unlisted_files": len(unlisted_zone_files),
          "excluded_files": len(disabled_zone_files) + len(unlisted_zone_files),
          "active_basenames_without_zone": len(active_without_zone),
          "active_multi_zone_files": len(active_multi_zone),
          "active_companion_only_files": len(companion_only_files),
      },
      "kinds": kind_summaries,
  }
  return {
      "schema_version": ROL_INVENTORY_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "root": normalized_root_label(source_root, repo_root),
      "source_kinds": list(SOURCE_KINDS),
      "manifests": [manifests[name].to_dict() for name in MANIFEST_KINDS],
      "files": files,
      "packages": packages,
      "classifications": classifications,
      "summary": summary,
  }


def render_rol_inventory_human(inventory: dict[str, Any]) -> str:
  """Render the compact human summary for a machine inventory."""

  summary = inventory["summary"]
  zones = summary["zones"]
  classified = inventory["classifications"]
  lines = [
      f"Realms of Luminari source inventory: {inventory['root']}",
      "",
      "Kind  Files  Active  Disabled  Unlisted  Missing",
  ]
  for kind in summary["kinds"]:
    lines.append(
        f"{kind['kind']:<4}  {kind['files']:>5}  {kind['active_files']:>6}  "
        f"{kind['disabled_files']:>8}  {kind['unlisted_files']:>8}  "
        f"{kind['missing_companions']:>7}"
    )
  lines.extend(
      (
          "",
          f"Active zone scope: {zones['active_files']} files, {zones['active_records']} records",
          f"Excluded zone files: {zones['disabled_files']} disabled, "
          f"{zones['unlisted_files']} unlisted",
          f"Active basenames without .zon: {zones['active_basenames_without_zone']} "
          f"({', '.join(classified['active_basenames_without_zone']) or 'none'})",
          f"Active multi-zone files: {zones['active_multi_zone_files']} "
          f"({', '.join(item['basename'] for item in classified['active_multi_zone_files']) or 'none'})",
          f"Included active companion-only files: {zones['active_companion_only_files']}",
      )
  )
  return "\n".join(lines) + "\n"
