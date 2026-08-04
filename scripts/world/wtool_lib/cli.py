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
from .world import load_indexed_world_data, validate_explicit_paths, validate_indexed_world


def _default_world_root() -> Path:
  return default_repo_root() / "lib/world"


def _parser() -> argparse.ArgumentParser:
  parser = argparse.ArgumentParser(
      prog="wtool",
      description="Read-only validation and lookup tools for Luminari world data.",
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
      choices=("sectors", "item-types", "positions", "directions", "trigger-types"),
  )
  sync = constant_commands.add_parser("sync")
  sync_mode = sync.add_mutually_exclusive_group(required=True)
  sync_mode.add_argument("--check", action="store_true")
  sync_mode.add_argument("--write", action="store_true")

  show = commands.add_parser("show", help="show one typed world record")
  show.add_argument("record_type", choices=CLI_RECORD_TYPES)
  show.add_argument("vnum", type=int)

  refs = commands.add_parser("refs", help="show typed incoming and outgoing references")
  refs.add_argument("record_type", choices=CLI_RECORD_TYPES)
  refs.add_argument("vnum", type=int)

  docs = commands.add_parser("docs", help="check world-building documentation drift")
  docs.add_argument("--check", action="store_true", required=True)
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
    sys.stdout.write(f"{entry['index']:3d} {macro:<36} {entry['name']}{reserved}\n")
  return 0


def _run_flags(args: argparse.Namespace) -> int:
  manifest = _load_default_manifest()
  _, table = resolve_set(manifest, args.set_name)
  entries = table["entries"]
  if args.flag_command == "list":
    return _list_entries(entries, args.json_output)
  if args.flag_command == "decode":
    decoded = decode_tokens(args.tokens, entry_count=len(entries))
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
      sys.stdout.write(" ".join(encode_bits(decoded.bits)) + "\n")
      for entry in data["entries"]:
        sys.stdout.write(f"{entry['index']:3d} {entry.get('macro') or '-':<36} {entry['name']}\n")
      for issue in decoded.issues:
        sys.stderr.write(f"wtool: {issue.code}: {issue.message}\n")
    return 1 if decoded.issues else 0

  bits = resolve_names(table, args.names)
  tokens = encode_bits(bits)
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
  except (ConfigError, DocumentationError, ExtractionError, OSError, ValueError) as error:
    sys.stderr.write(f"wtool: error: {error}\n")
    return 2
  parser.error(f"unsupported command {args.command}")
  return 2
