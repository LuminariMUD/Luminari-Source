from __future__ import annotations

from pathlib import Path
import re
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_periodic_profiles import DEVOUR_PROFILE_ORDER, PROFILE_SOURCES


class RolPeriodicProfileTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()

  def test_selected_manifest_has_unique_converted_mobile_coverage(self) -> None:
    vnums = [vnum for _relative, handler_vnums in PROFILE_SOURCES.values() for vnum in handler_vnums]

    self.assertEqual(98, len(PROFILE_SOURCES))
    self.assertEqual(104, len(vnums))
    self.assertEqual(len(vnums), len(set(vnums)))
    self.assertEqual(
        {
            "src/specs.bloodstone.c",
            "src/specs.fun.c",
            "src/specs.icecrag.c",
            "src/specs.lavatubes.c",
            "src/specs.menden.c",
            "src/specs.mobile.c",
            "src/specs.realm.c",
            "src/specs.towerofsorc.c",
            "src/specs.waterdeep.c",
        },
        {relative for relative, _vnums in PROFILE_SOURCES.values()},
    )

  def test_checked_in_profile_table_covers_manifest_exactly(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )
    enum_names = set(re.findall(r"^  ROL_SOURCE_PERIODIC_([A-Z0-9_]+),$", generated, re.MULTILINE))
    expected_names = {re.sub(r"[^A-Za-z0-9]+", "_", name).upper() for name in PROFILE_SOURCES}
    generated_vnums = {
        int(value)
        for value in re.findall(
            r"^    \{(\d+), ROL_SOURCE_PERIODIC_[A-Z0-9_]+,", generated, re.MULTILINE
        )
    }
    expected_vnums = {
        vnum for _relative, handler_vnums in PROFILE_SOURCES.values() for vnum in handler_vnums
    }

    self.assertEqual(expected_names, enum_names)
    self.assertEqual(expected_vnums, generated_vnums)
    self.assertRegex(generated, r"Source digest: [0-9a-f]{64}")

  def test_generated_tables_are_sorted_for_binary_lookup(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )
    profile_names = re.findall(
        r"^  (ROL_SOURCE_PERIODIC_[A-Z0-9_]+),$", generated, re.MULTILINE
    )
    profile_order = {name: index for index, name in enumerate(profile_names)}
    profile_vnums = [
        int(value)
        for value in re.findall(
            r"^    \{(\d+), ROL_SOURCE_PERIODIC_[A-Z0-9_]+,", generated, re.MULTILINE
        )
    ]
    outcomes = [
        (profile_order[name], int(roll))
        for name, roll in re.findall(
            r"^    \{(ROL_SOURCE_PERIODIC_[A-Z0-9_]+), (\d+), \d+, \d+\},$",
            generated,
            re.MULTILINE,
        )
    ]
    actions = re.findall(
        r"^    \{ROL_SOURCE_PERIODIC_(?:SPEECH|ROOM_ACTION),", generated, re.MULTILINE
    )

    self.assertEqual(sorted(profile_vnums), profile_vnums)
    self.assertEqual(sorted(outcomes), outcomes)
    self.assertEqual(380, len(outcomes))
    self.assertEqual(621, len(actions))

  def test_devour_composition_is_explicit_and_ordered(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )

    self.assertEqual({"bs_wolf": "before", "dog_one": "after"}, DEVOUR_PROFILE_ORDER)
    self.assertRegex(
        generated,
        r"\{2007140, ROL_SOURCE_PERIODIC_BS_WOLF, 0, 100, 0, 0, true, false, true, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_BEFORE\}",
    )
    self.assertRegex(
        generated,
        r"\{2003062, ROL_SOURCE_PERIODIC_DOG_ONE, 2, 8, 2, 4, true, false, false, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_AFTER\}",
    )

  def test_dice_and_sleeping_profiles_preserve_source_gates(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )

    self.assertRegex(
        generated,
        r"\{2003212, ROL_SOURCE_PERIODIC_GUARD_TWO, 2, 8, 2, 4, false, true, false, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_NONE\}",
    )
    self.assertRegex(
        generated,
        r"\{2012000, ROL_SOURCE_PERIODIC_SNOWBEAST, 3, 18, 3, 6, false, false, true, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_NONE\}",
    )


if __name__ == "__main__":
  unittest.main()
