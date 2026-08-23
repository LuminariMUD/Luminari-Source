"""Command-line interface for the Luminari world validator."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any, Sequence

from .config import ConfigError, resolve_config
from .constants import (
    ExtractionError,
    check_manifest,
    default_manifest_path,
    default_repo_root,
    load_manifest,
    write_manifest,
)
from .docs_check import DocumentationError, validate_docs
from .flags import FLAG_SETS, decode_tokens, decoded_entries, encode_bits, resolve_names, resolve_set
from .indexes import normalized_root_label
from .lookup import (
    CLI_RECORD_TYPES,
    find_records,
    lookup_error_findings,
    normalize_record_type,
    record_to_dict,
    refs_for_record,
    render_refs_human,
    render_show_human,
)
from .models import JSON_SCHEMA_VERSION, TOOL_VERSION
from .reporting import exit_status, render_human, render_json
from .rol_baseline import render_rol_baseline_human, write_baseline_bundle
from .rol_capability_audit import (
    render_rol_capability_audit_human,
    write_capability_audit_bundle,
)
from .rol_discovery import render_rol_discovery_human, write_discovery_bundle
from .rol_inventory import build_rol_inventory, render_rol_inventory_human
from .rol_planner import render_rol_plan_human, write_plan_bundle
from .rol_pilot import render_rol_pilot_selection_human, write_pilot_selection_bundle
from .rol_pilot_build import render_rol_pilot_build_human, write_pilot_build_bundle
from .rol_phase7 import render_rol_phase7_human, write_phase7_bundle
from .rol_phase8 import (
    apply_phase8_bundle,
    render_rol_phase8_human,
    write_phase8_bundle,
    write_phase8_completion,
)
from .rol_persistence_check import (
    audit_development_persistence,
    render_persistence_check_human,
)
from .rol_skeleton import render_rol_skeleton_human, write_skeleton_bundle
from .rol_special_reconciliation import (
    render_rol_special_reconciliation_human,
    write_special_reconciliation_bundle,
)
from .world import load_indexed_world_data, validate_explicit_paths, validate_indexed_world


def _default_world_root() -> Path:
  return default_repo_root() / "lib/world"


def _default_rol_source_root() -> Path:
  return default_repo_root() / "EXAMPLE/RealmsOfLuminari"


def _parser() -> argparse.ArgumentParser:
  parser = argparse.ArgumentParser(
      prog="wtool",
      description="Read-only validation, lookup, and source inventory tools for world data.",
  )
  parser.add_argument("--version", action="version", version=f"wtool {TOOL_VERSION}")
  parser.add_argument("--world-root", type=Path, default=_default_world_root())
  parser.add_argument("--config", type=Path)
  parser.add_argument("--json", action="store_true", dest="json_output")
  parser.add_argument("--ignore-code", action="append", default=[])
  commands = parser.add_subparsers(dest="command", required=True)

  validate = commands.add_parser("validate", help="validate indexed world data")
  selector = validate.add_mutually_exclusive_group(required=True)
  selector.add_argument("--all", action="store_true", dest="all_world")
  selector.add_argument("--mini", action="store_true")
  selector.add_argument("--zone", type=int, nargs="+")
  selector.add_argument("--paths", type=Path, nargs="+")
  validate.add_argument("--strict", action="store_true")

  flags = commands.add_parser("flags", help="list, decode, or encode flat-file flags")
  flag_commands = flags.add_subparsers(dest="flag_command", required=True)
  flag_list = flag_commands.add_parser("list")
  flag_list.add_argument("set_name", choices=FLAG_SETS)
  flag_decode = flag_commands.add_parser("decode")
  flag_decode.add_argument("set_name", choices=FLAG_SETS)
  flag_decode.add_argument("tokens", nargs="+")
  flag_encode = flag_commands.add_parser("encode")
  flag_encode.add_argument("set_name", choices=FLAG_SETS)
  flag_encode.add_argument("names", nargs="+")

  constants = commands.add_parser("constants", help="list or synchronize source constants")
  constant_commands = constants.add_subparsers(dest="constant_command", required=True)
  constant_list = constant_commands.add_parser("list")
  constant_list.add_argument(
      "table_name",
      choices=(
          "sectors",
          "item-types",
          "positions",
          "directions",
          "trigger-types",
          "quest-types",
          "hlquest-entry-types",
          "hlquest-commands",
          "mission-difficulties",
      ),
  )
  sync = constant_commands.add_parser("sync")
  sync_mode = sync.add_mutually_exclusive_group(required=True)
  sync_mode.add_argument("--check", action="store_true")
  sync_mode.add_argument("--write", action="store_true")

  show = commands.add_parser("show", help="show one typed world record")
  show.add_argument("record_type", metavar="{" + ",".join(CLI_RECORD_TYPES) + "}")
  show.add_argument("vnum", type=int)

  refs = commands.add_parser("refs", help="show typed incoming and outgoing references")
  refs.add_argument("record_type", metavar="{" + ",".join(CLI_RECORD_TYPES) + "}")
  refs.add_argument("vnum", type=int)

  docs = commands.add_parser("docs", help="check world-building documentation drift")
  docs.add_argument("--check", action="store_true", required=True)

  rol_inventory = commands.add_parser(
      "rol-inventory",
      help="inventory Realms of Luminari source manifests and data files",
  )
  rol_inventory.add_argument("--source-root", type=Path, default=_default_rol_source_root())

  rol_persistence_check = commands.add_parser(
      "rol-persistence-check",
      help="read-only check of persisted RoL VNUMs in the local development database",
  )
  rol_persistence_check.add_argument(
      "--development-lib-root",
      type=Path,
      help="explicit development lib root containing .env and mysql_config",
  )

  rol_baseline = commands.add_parser(
      "rol-baseline",
      help="write Realms of Luminari Phase 0 source and target evidence",
  )
  rol_baseline.add_argument("--source-root", type=Path, default=_default_rol_source_root())
  rol_baseline.add_argument("--output-dir", type=Path, required=True)
  rol_baseline.add_argument("--created-at")

  rol_discover = commands.add_parser(
      "rol-discover",
      help="write Realms of Luminari Phase 1 grammar and reconciliation evidence",
  )
  rol_discover.add_argument("--source-root", type=Path, default=_default_rol_source_root())
  rol_discover.add_argument("--output-dir", type=Path, required=True)
  rol_discover.add_argument("--created-at")

  rol_plan = commands.add_parser(
      "rol-plan",
      help="write a deterministic Realms of Luminari Phase 2 record-action plan",
  )
  rol_plan.add_argument("--discovery-dir", type=Path, required=True)
  rol_plan.add_argument("--output-dir", type=Path, required=True)
  rol_plan.add_argument("--created-at")

  rol_skeleton = commands.add_parser(
      "rol-skeleton",
      help="exercise the Realms of Luminari Phase 3 no-clobber delivery path",
  )
  rol_skeleton.add_argument("--plan-dir", type=Path, required=True)
  rol_skeleton.add_argument("--output-dir", type=Path, required=True)
  rol_skeleton.add_argument("--basename", default="jotun")
  rol_skeleton.add_argument("--created-at")

  rol_pilot_select = commands.add_parser(
      "rol-pilot-select",
      help="write the measured Realms of Luminari Phase 4 pilot selection",
  )
  rol_pilot_select.add_argument("--discovery-dir", type=Path, required=True)
  rol_pilot_select.add_argument("--plan-dir", type=Path, required=True)
  rol_pilot_select.add_argument("--output-dir", type=Path, required=True)
  rol_pilot_select.add_argument("--created-at")

  rol_pilot_build = commands.add_parser(
      "rol-pilot-build",
      help="build and stage the complete Realms of Luminari Phase 4 pilot",
  )
  rol_pilot_build.add_argument("--selection-dir", type=Path, required=True)
  rol_pilot_build.add_argument("--plan-dir", type=Path, required=True)
  rol_pilot_build.add_argument(
      "--source-root", type=Path, default=_default_rol_source_root()
  )
  rol_pilot_build.add_argument("--output-dir", type=Path, required=True)
  rol_pilot_build.add_argument("--created-at")

  rol_capability_audit = commands.add_parser(
      "rol-capability-audit",
      help="audit every active RoL record against the shared conversion capabilities",
  )
  rol_capability_audit.add_argument("--plan-dir", type=Path, required=True)
  rol_capability_audit.add_argument(
      "--source-root", type=Path, default=_default_rol_source_root()
  )
  rol_capability_audit.add_argument("--output-dir", type=Path, required=True)
  rol_capability_audit.add_argument("--created-at")

  rol_special_reconciliation = commands.add_parser(
      "rol-special-reconcile",
      help="write the full Phase 6 RoL special-procedure reconciliation ledger",
  )
  rol_special_reconciliation.add_argument("--discovery-dir", type=Path, required=True)
  rol_special_reconciliation.add_argument("--plan-dir", type=Path, required=True)
  rol_special_reconciliation.add_argument(
      "--capability-audit-dir", type=Path, required=True
  )
  rol_special_reconciliation.add_argument(
      "--source-root", type=Path, default=_default_rol_source_root()
  )
  rol_special_reconciliation.add_argument("--output-dir", type=Path, required=True)
  rol_special_reconciliation.add_argument("--created-at")

  rol_phase7 = commands.add_parser(
      "rol-phase7",
      help="build a cumulative Realms of Luminari Phase 7 conversion milestone",
  )
  rol_phase7.add_argument("--discovery-dir", type=Path, required=True)
  rol_phase7.add_argument("--plan-dir", type=Path, required=True)
  rol_phase7.add_argument("--capability-audit-dir", type=Path, required=True)
  rol_phase7.add_argument("--phase6-dir", type=Path, required=True)
  rol_phase7.add_argument(
      "--source-root", type=Path, default=_default_rol_source_root()
  )
  rol_phase7.add_argument("--output-dir", type=Path, required=True)
  rol_phase7.add_argument("--through-batch", type=int, required=True)
  rol_phase7.add_argument("--prior-milestone-dir", type=Path, action="append", default=[])
  rol_phase7.add_argument("--created-at")

  rol_phase8 = commands.add_parser(
      "rol-phase8",
      help="assemble and seal the final Realms of Luminari release candidate",
  )
  rol_phase8.add_argument("--phase7-dir", type=Path, required=True)
  rol_phase8.add_argument("--repeat-phase7-dir", type=Path, required=True)
  rol_phase8.add_argument("--world-tools-log", type=Path, required=True)
  rol_phase8.add_argument("--cutest-log", type=Path, required=True)
  rol_phase8.add_argument("--install-log", type=Path, required=True)
  rol_phase8.add_argument("--syntax-log", type=Path, required=True)
  rol_phase8.add_argument("--runtime-log", type=Path, required=True)
  rol_phase8.add_argument(
      "--development-lib-root",
      type=Path,
      help="explicit development lib root used only by the read-only persistence gate",
  )
  rol_phase8.add_argument("--output-dir", type=Path, required=True)
  rol_phase8.add_argument("--created-at")

  rol_phase8_apply = commands.add_parser(
      "rol-phase8-apply",
      help="apply an accepted Phase 8 release candidate to development",
  )
  rol_phase8_apply.add_argument("--bundle-dir", type=Path, required=True)
  rol_phase8_apply.add_argument(
      "--lib-root", type=Path, default=default_repo_root() / "lib"
  )

  rol_phase8_completion = commands.add_parser(
      "rol-phase8-completion",
      help="seal post-apply Phase 8 completion and idempotency evidence",
  )
  rol_phase8_completion.add_argument("--bundle-dir", type=Path, required=True)
  rol_phase8_completion.add_argument(
      "--lib-root", type=Path, default=default_repo_root() / "lib"
  )
  rol_phase8_completion.add_argument("--output-dir", type=Path, required=True)
  rol_phase8_completion.add_argument("--created-at")
  return parser


def _print_json(data: Any) -> None:
  sys.stdout.write(json.dumps(data, ensure_ascii=True, indent=2, sort_keys=True) + "\n")


def _load_default_manifest() -> dict[str, Any]:
  return load_manifest(default_manifest_path(default_repo_root()))


def _list_entries(entries: list[dict[str, Any]], json_output: bool) -> int:
  if json_output:
    _print_json(entries)
    return 0
  for entry in entries:
    macro = entry.get("macro") or "-"
    reserved = " reserved" if entry.get("reserved") else ""
    code = f" code={entry['code']}" if entry.get("code") else ""
    sys.stdout.write(f"{entry['index']:3d} {macro:<36} {entry['name']}{code}{reserved}\n")
  return 0


def _run_flags(args: argparse.Namespace) -> int:
  manifest = _load_default_manifest()
  _, table = resolve_set(manifest, args.set_name)
  entries = table["entries"]
  serialized_chunks = table.get(
      "serialized_chunks", manifest["flag_encoding"]["serialized_chunks"]
  )
  if args.flag_command == "list":
    return _list_entries(entries, args.json_output)
  if args.flag_command == "decode":
    decoded = decode_tokens(
        args.tokens,
        entry_count=len(entries),
        serialized_chunks=serialized_chunks,
    )
    data = {
        "set": args.set_name,
        "tokens": args.tokens,
        "chunks": list(decoded.chunks),
        "bits": sorted(decoded.bits),
        "entries": decoded_entries(table, decoded),
        "issues": [
            {"code": issue.code, "message": issue.message, "token_index": issue.token_index}
            for issue in decoded.issues
        ],
    }
    if args.json_output:
      _print_json(data)
    else:
      sys.stdout.write(
          " ".join(encode_bits(decoded.bits, serialized_chunks=serialized_chunks)) + "\n"
      )
      for entry in data["entries"]:
        sys.stdout.write(f"{entry['index']:3d} {entry.get('macro') or '-':<36} {entry['name']}\n")
      for issue in decoded.issues:
        sys.stderr.write(f"wtool: {issue.code}: {issue.message}\n")
    return 1 if decoded.issues else 0

  bits = resolve_names(table, args.names)
  tokens = encode_bits(bits, serialized_chunks=serialized_chunks)
  if args.json_output:
    _print_json({"set": args.set_name, "bits": sorted(bits), "tokens": list(tokens)})
  else:
    sys.stdout.write(" ".join(tokens) + "\n")
  return 0


def _run_constants(args: argparse.Namespace) -> int:
  repo_root = default_repo_root()
  if args.constant_command == "sync":
    if args.write:
      path = write_manifest(repo_root)
      message = f"wrote {path.relative_to(repo_root).as_posix()}"
      if args.json_output:
        _print_json({"current": True, "written": path.relative_to(repo_root).as_posix()})
      else:
        sys.stdout.write(message + "\n")
      return 0
    current, message = check_manifest(repo_root)
    if args.json_output:
      _print_json({"current": current, "message": message})
    else:
      sys.stdout.write(message + "\n")
    return 0 if current else 1

  manifest = _load_default_manifest()
  if args.table_name == "trigger-types":
    sections = {
        host: manifest["tables"][key]["entries"]
        for host, key in (
            ("mob", "trigger-types-mob"),
            ("object", "trigger-types-object"),
            ("world", "trigger-types-world"),
        )
    }
    if args.json_output:
      _print_json(sections)
    else:
      for host, entries in sections.items():
        sys.stdout.write(f"[{host}]\n")
        _list_entries(entries, False)
    return 0
  return _list_entries(manifest["tables"][args.table_name]["entries"], args.json_output)


def _run_validate(args: argparse.Namespace) -> int:
  repo_root = default_repo_root()
  manifest = _load_default_manifest()
  if args.paths:
    requested_paths = [path.resolve() for path in args.paths]
    inaccessible = [path for path in requested_paths if not path.exists()]
    if inaccessible:
      raise ConfigError(f"requested validation path is inaccessible: {inaccessible[0]}")
    if args.config is not None:
      config = resolve_config(Path("/__wtool_isolated__"), args.config)
    else:
      config = {"diagonal_dirs": False, "config_source": "source-default", "assumed": True}
    result = validate_explicit_paths(requested_paths, repo_root, manifest, config)
  else:
    world_root = args.world_root.resolve()
    if not world_root.is_dir():
      raise ConfigError(f"requested world root is inaccessible: {world_root}")
    config = resolve_config(world_root, args.config)
    selected_zones = set(args.zone) if args.zone else None
    if selected_zones is not None and any(zone < 0 for zone in selected_zones):
      raise ConfigError("zone selectors must be non-negative integers")
    result = validate_indexed_world(
        world_root,
        repo_root,
        manifest,
        config,
        mini=bool(args.mini),
        selected_packages=selected_zones,
    )
    result.root_label = normalized_root_label(world_root, repo_root)
  ignored_codes = set(args.ignore_code)
  if args.json_output:
    sys.stdout.write(render_json(result, ignored_codes))
  else:
    sys.stdout.write(render_human(result, ignored_codes))
  return exit_status(result, strict=args.strict, ignored_codes=ignored_codes)


def _load_lookup_world(args: argparse.Namespace):
  repo_root = default_repo_root()
  world_root = args.world_root.resolve()
  if not world_root.is_dir():
    raise ConfigError(f"requested world root is inaccessible: {world_root}")
  if args.vnum < 0:
    raise ConfigError("lookup vnums must be non-negative integers")
  config = resolve_config(world_root, args.config)
  world = load_indexed_world_data(
      world_root,
      repo_root,
      _load_default_manifest(),
      config,
  )
  return repo_root, world_root, world


def _lookup_envelope(args: argparse.Namespace, root: str, found: bool) -> dict[str, Any]:
  return {
      "schema_version": JSON_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "root": root,
      "record_type": normalize_record_type(args.record_type),
      "vnum": args.vnum,
      "found": found,
  }


def _run_show(args: argparse.Namespace) -> int:
  repo_root, world_root, world = _load_lookup_world(args)
  records = find_records(world, args.record_type, args.vnum)
  if args.json_output:
    data = _lookup_envelope(
        args,
        normalized_root_label(world_root, repo_root),
        bool(records),
    )
    data["matches"] = [record_to_dict(record) for record in records]
    data["lookup_parse_errors"] = len(lookup_error_findings(world))
    _print_json(data)
  else:
    sys.stdout.write(render_show_human(args.record_type, args.vnum, records))
  return 0 if records else 1


def _run_refs(args: argparse.Namespace) -> int:
  repo_root, world_root, world = _load_lookup_world(args)
  records = find_records(world, args.record_type, args.vnum)
  outgoing, incoming = refs_for_record(world, args.record_type, args.vnum)
  if args.json_output:
    data = _lookup_envelope(
        args,
        normalized_root_label(world_root, repo_root),
        bool(records),
    )
    data["outgoing"] = [edge.to_dict() for edge in outgoing]
    data["incoming"] = [edge.to_dict() for edge in incoming]
    data["lookup_parse_errors"] = len(lookup_error_findings(world))
    _print_json(data)
  else:
    sys.stdout.write(
        render_refs_human(
            args.record_type,
            args.vnum,
            bool(records),
            outgoing,
            incoming,
        )
    )
  return 0 if records else 1


def _run_docs(args: argparse.Namespace) -> int:
  result = validate_docs(default_repo_root(), _load_default_manifest())
  ignored_codes = set(args.ignore_code)
  if args.json_output:
    sys.stdout.write(render_json(result, ignored_codes))
  else:
    sys.stdout.write(render_human(result, ignored_codes))
  return exit_status(result, ignored_codes=ignored_codes)


def _run_rol_inventory(args: argparse.Namespace) -> int:
  source_root = args.source_root.resolve()
  if not source_root.is_dir():
    raise ConfigError(f"requested RoL source root is inaccessible: {source_root}")
  inventory = build_rol_inventory(source_root, default_repo_root())
  if args.json_output:
    _print_json(inventory)
  else:
    sys.stdout.write(render_rol_inventory_human(inventory))
  return 0


def _run_rol_persistence_check(args: argparse.Namespace) -> int:
  repo_root = default_repo_root()
  world_root = args.world_root.resolve()
  world = load_indexed_world_data(
      world_root,
      repo_root,
      load_manifest(default_manifest_path(repo_root)),
      resolve_config(world_root, None),
  )
  result = audit_development_persistence(
      world, repo_root, args.development_lib_root
  )
  if args.json_output:
    _print_json(result)
  else:
    sys.stdout.write(render_persistence_check_human(result))
  return 0 if result["summary"]["pass"] else 1


def _run_rol_baseline(args: argparse.Namespace) -> int:
  source_root = args.source_root.resolve()
  world_root = args.world_root.resolve()
  if not source_root.is_dir():
    raise ConfigError(f"requested RoL source root is inaccessible: {source_root}")
  if not world_root.is_dir():
    raise ConfigError(f"requested world root is inaccessible: {world_root}")
  summary = write_baseline_bundle(
      source_root,
      world_root,
      args.output_dir,
      default_repo_root(),
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_baseline_human(summary))
  return 0


def _run_rol_discover(args: argparse.Namespace) -> int:
  summary = write_discovery_bundle(
      args.source_root,
      args.world_root,
      args.output_dir,
      default_repo_root(),
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_discovery_human(summary))
  return 0


def _run_rol_plan(args: argparse.Namespace) -> int:
  summary = write_plan_bundle(
      args.discovery_dir,
      args.output_dir,
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_plan_human(summary))
  return 0


def _run_rol_skeleton(args: argparse.Namespace) -> int:
  summary = write_skeleton_bundle(
      args.plan_dir,
      args.world_root,
      args.output_dir,
      basename=args.basename,
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_skeleton_human(summary))
  return 0


def _run_rol_pilot_select(args: argparse.Namespace) -> int:
  summary = write_pilot_selection_bundle(
      args.discovery_dir,
      args.plan_dir,
      args.output_dir,
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_pilot_selection_human(summary))
  return 0


def _run_rol_pilot_build(args: argparse.Namespace) -> int:
  summary = write_pilot_build_bundle(
      args.selection_dir,
      args.plan_dir,
      args.source_root,
      args.world_root,
      args.output_dir,
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_pilot_build_human(summary))
  return 0


def _run_rol_capability_audit(args: argparse.Namespace) -> int:
  summary = write_capability_audit_bundle(
      args.plan_dir,
      args.source_root,
      args.output_dir,
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_capability_audit_human(summary))
  return 0


def _run_rol_special_reconciliation(args: argparse.Namespace) -> int:
  summary = write_special_reconciliation_bundle(
      args.discovery_dir,
      args.plan_dir,
      args.capability_audit_dir,
      args.source_root,
      args.output_dir,
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_special_reconciliation_human(summary))
  return 0


def _run_rol_phase7(args: argparse.Namespace) -> int:
  summary = write_phase7_bundle(
      args.discovery_dir,
      args.plan_dir,
      args.capability_audit_dir,
      args.phase6_dir,
      args.source_root,
      args.world_root,
      args.output_dir,
      args.through_batch,
      prior_milestone_dirs=args.prior_milestone_dir,
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_phase7_human(summary))
  return 0


def _run_rol_phase8(args: argparse.Namespace) -> int:
  summary = write_phase8_bundle(
      args.phase7_dir,
      args.repeat_phase7_dir,
      args.world_root,
      args.output_dir,
      {
          "world-tools": args.world_tools_log,
          "cutest": args.cutest_log,
          "install": args.install_log,
          "syntax": args.syntax_log,
          "runtime": args.runtime_log,
      },
      development_lib_root=args.development_lib_root,
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(render_rol_phase8_human(summary))
  return 0


def _run_rol_phase8_apply(args: argparse.Namespace) -> int:
  summary = apply_phase8_bundle(args.bundle_dir, args.lib_root)
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(
        f"RoL Phase 8 apply: {summary['run_id']}\n"
        f"Changed paths: {summary['changed_paths']}\n"
        f"Already current paths: {summary['already_current_paths']}\n"
        f"Idempotent no-op: {str(summary['idempotent_no_op']).lower()}\n"
    )
  return 0


def _run_rol_phase8_completion(args: argparse.Namespace) -> int:
  summary = write_phase8_completion(
      args.bundle_dir,
      args.lib_root,
      args.output_dir,
      created_at=args.created_at,
  )
  if args.json_output:
    _print_json(summary)
  else:
    sys.stdout.write(
        f"RoL Phase 8 completion: {summary['run_id']}\n"
        f"Output: {summary['output_dir']}\n"
        f"Repeat apply no-op: {str(summary['repeat_apply_no_op']).lower()}\n"
        f"Complete: {str(summary['complete']).lower()}\n"
    )
  return 0


def main(argv: Sequence[str] | None = None) -> int:
  parser = _parser()
  args = parser.parse_args(argv)
  try:
    if args.command == "validate":
      return _run_validate(args)
    if args.command == "flags":
      return _run_flags(args)
    if args.command == "constants":
      return _run_constants(args)
    if args.command == "show":
      return _run_show(args)
    if args.command == "refs":
      return _run_refs(args)
    if args.command == "docs":
      return _run_docs(args)
    if args.command == "rol-inventory":
      return _run_rol_inventory(args)
    if args.command == "rol-persistence-check":
      return _run_rol_persistence_check(args)
    if args.command == "rol-baseline":
      return _run_rol_baseline(args)
    if args.command == "rol-discover":
      return _run_rol_discover(args)
    if args.command == "rol-plan":
      return _run_rol_plan(args)
    if args.command == "rol-skeleton":
      return _run_rol_skeleton(args)
    if args.command == "rol-pilot-select":
      return _run_rol_pilot_select(args)
    if args.command == "rol-pilot-build":
      return _run_rol_pilot_build(args)
    if args.command == "rol-capability-audit":
      return _run_rol_capability_audit(args)
    if args.command == "rol-special-reconcile":
      return _run_rol_special_reconciliation(args)
    if args.command == "rol-phase7":
      return _run_rol_phase7(args)
    if args.command == "rol-phase8":
      return _run_rol_phase8(args)
    if args.command == "rol-phase8-apply":
      return _run_rol_phase8_apply(args)
    if args.command == "rol-phase8-completion":
      return _run_rol_phase8_completion(args)
  except (ConfigError, DocumentationError, ExtractionError, OSError, ValueError) as error:
    sys.stderr.write(f"wtool: error: {error}\n")
    return 2
  parser.error(f"unsupported command {args.command}")
  return 2
