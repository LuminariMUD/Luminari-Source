import tempfile
import unittest
from pathlib import Path

from wtool_lib.rol_completion_audit import (
    _consumer_ledger,
    _documentation_audit,
    _numbered_items,
    _old_target,
    _operation,
    _reference_evidence,
    _requirements_matrix,
)


class RolCompletionAuditTests(unittest.TestCase):

  def test_permanent_documentation_owns_the_maintenance_contract(self):
    repo_root = Path(__file__).resolve().parents[3]

    requirements = _requirements_matrix(repo_root)
    documentation = _documentation_audit(repo_root)

    self.assertEqual(14, requirements["summary"]["total"])
    self.assertTrue(documentation["canonical_contract_present"])
    self.assertTrue(documentation["maintenance_gate_present"])
    self.assertTrue(documentation["phase_6_5_changelog_present"])

  def test_old_target_and_operation_cover_phase_6_5_special_cases(self):
    core = {"basename": "trail", "source_kind": "wld", "source_vnum": 50700}
    addition = {"basename": "jotun", "source_kind": "obj", "source_vnum": 96092}
    artifact = {"basename": "quests", "source_kind": "obj", "source_vnum": 1009}
    mytheast = {"basename": "mytheast", "source_kind": "zon", "source_vnum": 81700}

    self.assertEqual(150700, _old_target(core))
    self.assertEqual("REHOME", _operation(core))
    self.assertIsNone(_old_target(addition))
    self.assertEqual("CANONICAL_ADD", _operation(addition))
    self.assertEqual(169906, _old_target(artifact))
    self.assertEqual("RESTORE_DISTINCT_IDENTITY", _operation(artifact))
    self.assertEqual(20002, _old_target(mytheast))
    self.assertEqual("NORMALIZE_SOURCE_ZONE", _operation(mytheast))

  def test_numbered_items_preserve_wrapped_text(self):
    text = """prefix
START
1. first line
   wrapped line
2. second line
END
"""
    self.assertEqual(
        ["first line wrapped line", "second line"],
        _numbered_items(text, "START", "END"),
    )

  def test_consumer_ledger_rejects_active_retired_runtime_match(self):
    selected = [
        {
            "basename": "trail",
            "source_kind": "zon",
            "source_vnum": 507,
            "destination_vnum": 20507,
        }
    ]
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory)
      (root / "src").mkdir()
      (root / "src/active.c").write_text("int zone = 1507;\n", encoding="ascii")
      rows, report = _consumer_ledger(root, selected)

    self.assertEqual(1, len(rows))
    self.assertEqual("active_retired_runtime_consumer", rows[0]["classification"])
    self.assertEqual(1, report["unclassified_or_active_retired_consumers"])

  def test_reference_evidence_preserves_excluded_source_without_destination(self):
    selected = [
        {
            "basename": "trail",
            "source_kind": "wld",
            "source_vnum": 50700,
            "destination_vnum": 2050700,
            "target_type": "room",
            "source_record_id": "selected",
        }
    ]
    reconciliation = [
        *selected,
        {
            "basename": "excluded",
            "source_kind": "wld",
            "source_vnum": 1,
            "destination_vnum": None,
            "target_type": "room",
            "source_record_id": "excluded",
        },
    ]
    references = [
        {
            "line": 2,
            "owned": True,
            "path": "areas/wld/excluded.wld",
            "resolution": "excluded_source",
            "resolution_action": "exclude_dependent_instruction",
            "role": "exit_destination",
            "source_kind": "wld",
            "source_line": 1,
            "source_path": "areas/wld/excluded.wld",
            "source_record_id": "excluded",
            "source_vnum": 1,
            "target_type": "room",
            "target_vnum": 50700,
        }
    ]

    edges, per_record, report = _reference_evidence(
        reconciliation, selected, references, []
    )

    self.assertEqual(1, len(edges))
    self.assertEqual(1, len(per_record["selected"]["incoming"]))
    self.assertEqual(1, edges[0]["canonical_source_vnum"])
    self.assertEqual(0, report["unresolved_required"])

  def test_reference_evidence_includes_pre_cutover_world_rewrites(self):
    selected = [
        {
            "basename": "quests",
            "source_kind": "obj",
            "source_vnum": 1043,
            "destination_vnum": 2001043,
            "target_type": "object",
            "source_record_id": "artifact",
        }
    ]
    world_rewrites = [
        {
            "action": "rewrite",
            "line": 5,
            "path": "zon/1699.zon",
            "role": "typed zone header, range, or reset field",
            "source_vnum": 169901,
            "target_vnum": 2001043,
        },
        {
            "action": "rewrite",
            "line": 5,
            "path": "zon/1699.zon",
            "role": "typed zone header, range, or reset field",
            "source_vnum": 169901,
            "target_vnum": 2001043,
        },
        {
            "action": "rewrite",
            "line": 9,
            "path": "zon/1960.zon",
            "role": "typed zon field in owned legacy package",
            "source_vnum": 196297,
            "target_vnum": 2096297,
        },
    ]

    edges, per_record, report = _reference_evidence(
        selected, selected, [], world_rewrites
    )

    self.assertEqual(3, len(edges))
    self.assertEqual("artifacts", edges[0]["package"])
    self.assertEqual("precutover_world_rewrite", edges[0]["direction"])
    self.assertEqual(2, len(per_record["artifact"]["target_world_consumers"]))
    self.assertEqual(3, report["precutover_world_rewrite_rows"])
    self.assertEqual(1, report["packages"]["jotun"]["total_edges"])


if __name__ == "__main__":
  unittest.main()
