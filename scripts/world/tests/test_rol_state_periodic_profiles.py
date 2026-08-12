from __future__ import annotations

import re
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_state_periodic_profiles import (
    COMPOSED_STATE_PROFILE_SOURCES,
    CUMULATIVE_IDLE_STATE_PROFILES,
    STATE_PROFILE_SOURCES,
)


ALL_STATE_PROFILE_SOURCES = {**STATE_PROFILE_SOURCES, **COMPOSED_STATE_PROFILE_SOURCES}


class RolStatePeriodicProfileTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()
    cls.generated = (cls.root / "src/spec/spec_rol_state_periodic_profiles.inc").read_text(
        encoding="ascii"
    )

  def test_selected_manifest_has_unique_waterdeep_mobile_coverage(self) -> None:
    vnums = [
        vnum
        for _relative, profile_vnums, _states in ALL_STATE_PROFILE_SOURCES.values()
        for vnum in profile_vnums
    ]

    self.assertEqual(34, len(ALL_STATE_PROFILE_SOURCES))
    self.assertEqual(34, len(vnums))
    self.assertEqual(len(vnums), len(set(vnums)))
    self.assertEqual(
        {"src/specs.waterdeep.c"},
        {relative for relative, _vnums, _states in ALL_STATE_PROFILE_SOURCES.values()},
    )
    self.assertEqual({"casino_four"}, set(CUMULATIVE_IDLE_STATE_PROFILES))

  def test_checked_in_table_covers_manifest_and_source_digest(self) -> None:
    enum_names = set(
        re.findall(r"^  ROL_STATE_PERIODIC_([A-Z0-9_]+),$", self.generated, re.MULTILINE)
    )
    expected_names = {
        re.sub(r"[^A-Za-z0-9]+", "_", name).upper() for name in ALL_STATE_PROFILE_SOURCES
    }
    generated_vnums = [
        int(value)
        for value in re.findall(
            r"^    \{(\d+), ROL_STATE_PERIODIC_[A-Z0-9_]+,", self.generated, re.MULTILINE
        )
    ]
    expected_vnums = sorted(
        vnum
        for _relative, profile_vnums, _states in ALL_STATE_PROFILE_SOURCES.values()
        for vnum in profile_vnums
    )

    self.assertEqual(expected_names, enum_names)
    self.assertEqual(expected_vnums, generated_vnums)
    self.assertRegex(self.generated, r"Source digest: [0-9a-f]{64}")

  def test_generated_tables_are_sorted_for_binary_lookup(self) -> None:
    profile_names = re.findall(
        r"^  (ROL_STATE_PERIODIC_[A-Z0-9_]+),$", self.generated, re.MULTILINE
    )
    profile_order = {name: index for index, name in enumerate(profile_names)}
    state_order = {"ROL_STATE_PERIODIC_IDLE": 0, "ROL_STATE_PERIODIC_FIGHTING": 1}
    outcomes = [
        (profile_order[name], state_order[state], int(roll))
        for name, state, roll in re.findall(
            r"^    \{(ROL_STATE_PERIODIC_[A-Z0-9_]+), "
            r"(ROL_STATE_PERIODIC_(?:IDLE|FIGHTING)), (\d+), \d+, \d+\},$",
            self.generated,
            re.MULTILINE,
        )
    ]
    actions = re.findall(
        r"^    \{ROL_SOURCE_PERIODIC_(?:SPEECH|ROOM_ACTION),", self.generated, re.MULTILINE
    )

    self.assertEqual(sorted(outcomes), outcomes)
    self.assertEqual(266, len(outcomes))
    self.assertEqual(274, len(actions))


if __name__ == "__main__":
  unittest.main()
