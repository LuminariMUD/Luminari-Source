from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.rol_pilot_build import (
    RolPilotBuildError,
    _patch_mobile_block,
    _pilot_runtime_contract,
    _stage_overlay,
)
from wtool_lib.models import (
    ExitRecord,
    MobileRecord,
    ObjectRecord,
    ResetCommandRecord,
    RoomRecord,
    SourceSpan,
    WorldData,
    ZoneRecord,
)


class RolPilotBuildTests(unittest.TestCase):
  def test_mobile_patch_adds_spec_flag_name_and_unique_triggers(self) -> None:
    block = [
        "#2058601\n",
        "keywords~\n",
        "a mobile~\n",
        "A mobile stands here.~\n",
        "Description.~\n",
        "0 0 0 0 0 0 0 0 0 E\n",
        "E\n",
        "T 2026100\n",
    ]

    patched = _patch_mobile_block(
        block,
        2_058_601,
        "rol_mobile_spec",
        (2_026_100, 2_026_101),
    )

    self.assertIn("a 0 0 0 0 0 0 0 0 E\n", patched)
    self.assertIn("SpecProc: rol_mobile_spec\n", patched)
    self.assertEqual(1, patched.count("T 2026100\n"))
    self.assertEqual(1, patched.count("T 2026101\n"))

  def test_mobile_patch_rejects_incompatible_existing_spec(self) -> None:
    block = [
        "#2058601\n",
        "0 0 0 0 0 0 0 0 0 E\n",
        "SpecProc: another_spec\n",
        "E\n",
    ]

    with self.assertRaisesRegex(RolPilotBuildError, "incompatible SpecProc"):
      _patch_mobile_block(block, 2_058_601, "rol_mobile_spec", ())

  def test_stage_overlay_refuses_to_overwrite_target_file(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      target = root / "target"
      stage = root / "stage"
      (target / "mob").mkdir(parents=True)
      (target / "mob/20586.mob").write_text("$~\n", encoding="ascii")

      with self.assertRaisesRegex(RolPilotBuildError, "would overwrite"):
        _stage_overlay(
            target,
            stage,
            {"mob/20586.mob": b"$~\n"},
            {},
            {},
        )

  def test_runtime_contract_covers_disconnected_rooms_and_reset_references(self) -> None:
    span = SourceSpan("test", 1)
    rooms = [
        RoomRecord(
            100,
            span,
            "100",
            file_zone=1,
            exits=[ExitRecord(0, None, None, 0, -1, 101, span)],
        ),
        RoomRecord(101, span, "100", file_zone=1),
        RoomRecord(102, span, "100", file_zone=1),
    ]
    zone = ZoneRecord(
        1,
        span,
        "1",
        bottom=100,
        commands=[
            ResetCommandRecord("M", 0, [200, 1, 100, 100], [], span),
            ResetCommandRecord("G", 1, [300, 1, 100], [], span),
        ],
    )
    world = WorldData(
        zones=[zone],
        rooms=rooms,
        mobiles=[MobileRecord(200, span, "200")],
        objects=[ObjectRecord(300, span, "300")],
    )

    contract = _pilot_runtime_contract(world, [1])

    self.assertTrue(contract["all_reset_observations_pass"])
    self.assertTrue(contract["all_walkthroughs_pass"])
    self.assertEqual(2, contract["zones"][0]["walkthrough_root_count"])
    self.assertEqual(3, contract["zones"][0]["walkthrough_rooms_covered"])


if __name__ == "__main__":
  unittest.main()
