"""Fresh corpus integration inputs, independent of ignored historical run folders."""

from __future__ import annotations

import atexit
from functools import cache
from pathlib import Path
import shutil
import tempfile
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_baseline import load_rol_policy
from wtool_lib.rol_capability_audit import write_capability_audit_bundle
from wtool_lib.rol_discovery import write_discovery_bundle
from wtool_lib.rol_identity import canonical_destination
from wtool_lib.rol_pilot import write_pilot_selection_bundle
from wtool_lib.rol_planner import write_plan_bundle
from wtool_lib.rol_source import parse_rol_source_file
from wtool_lib.rol_transform import emit_room


@cache
def reference_run() -> Path:
  """Build one isolated current-policy run per test process from the real corpus."""
  root = default_repo_root()
  source = root / "EXAMPLE/RealmsOfLuminari"
  if not source.is_dir():
    raise unittest.SkipTest("ignored RoL source corpus is not installed")
  temporary = tempfile.TemporaryDirectory(prefix="rol-reference-")
  atexit.register(temporary.cleanup)
  run = Path(temporary.name)
  world = run / "world"
  minimal = root / "lib/world/minimal"
  for index in sorted(minimal.glob("index.*")):
    kind = index.suffix[1:]
    directory = world / kind
    directory.mkdir(parents=True)
    shutil.copyfile(index, directory / "index")
    for path in minimal.glob(f"*.{kind}"):
      if not path.name.startswith("index."):
        shutil.copyfile(path, directory / path.name)

  # A real, already converted pilot room exercises canonical KEEP/reuse coverage.
  # Only the temporary target is populated; installed world files are never touched.
  basename = "hulburg"
  relative = f"areas/wld/{basename}.wld"
  records, corpus = parse_rol_source_file(source / relative, relative, "wld", basename)
  if not corpus.complete or not records:
    raise AssertionError("Hulburg source rooms must parse completely")
  room = records[0]
  policy = load_rol_policy(root)

  def resolve(kind: str, vnum: int) -> int:
    return canonical_destination(kind, vnum, basename, policy)

  destination = resolve("wld", room.vnum)
  zone = resolve("zon", room.vnum // 100)
  converted = emit_room(room, destination, zone, resolve)
  filename = f"{zone}.wld"
  (world / "wld" / filename).write_text(converted.text + "$~\n", encoding="ascii")
  index = world / "wld/index"
  lines = index.read_text(encoding="ascii").splitlines()
  terminator = lines.index("$")
  lines.insert(terminator, filename)
  index.write_text("\n".join(lines) + "\n", encoding="ascii")

  write_discovery_bundle(source, world, run / "discovery", root)
  write_plan_bundle(run / "discovery", run / "plan")
  write_pilot_selection_bundle(run / "discovery", run / "plan", run / "selection")
  return run


@cache
def reference_audit() -> Path:
  """Generate the capability audit only for tests that need Phase 5 evidence."""
  run = reference_run()
  write_capability_audit_bundle(
      run / "plan", default_repo_root() / "EXAMPLE/RealmsOfLuminari", run / "audit"
  )
  return run / "audit"
