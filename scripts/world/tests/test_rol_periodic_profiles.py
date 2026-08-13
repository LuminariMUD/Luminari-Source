from __future__ import annotations

from pathlib import Path
import re
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_periodic_profiles import (
    COMPOSED_PROFILE_TARGETS,
    DEVOUR_PROFILE_ORDER,
    PROFILE_SOURCES,
)


class RolPeriodicProfileTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()

  def test_selected_manifest_has_unique_converted_mobile_coverage(self) -> None:
    vnums = [vnum for _relative, handler_vnums in PROFILE_SOURCES.values() for vnum in handler_vnums]

    self.assertEqual(126, len(PROFILE_SOURCES))
    self.assertEqual(136, len(vnums))
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
            "src/specs.scornubel.c",
            "src/specs.towerofsorc.c",
            "src/specs.undermountain.c",
            "src/specs.waterdeep.c",
            "src/specs.zhentilkeep.c",
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
        r"^    \{ROL_SOURCE_PERIODIC_(?:SPEECH|ROOM_ACTION|TARGET_ACTION),",
        generated,
        re.MULTILINE,
    )

    self.assertEqual(sorted(profile_vnums), profile_vnums)
    self.assertEqual(sorted(outcomes), outcomes)
    self.assertEqual(527, len(outcomes))
    self.assertEqual(790, len(actions))

  def test_scornubel_profiles_preserve_composition_and_source_ranges(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )
    compact = " ".join(generated.split())

    self.assertEqual({"sc_parchimil": "RoL Guild Guard"}, COMPOSED_PROFILE_TARGETS)
    self.assertRegex(
        compact,
        r"\{2006002, ROL_SOURCE_PERIODIC_SC_MERCHANT, 0, 20, 0, 0, true, false, true, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_NONE\}",
    )
    self.assertRegex(
        compact,
        r"\{2006051, ROL_SOURCE_PERIODIC_SC_COMMONER, 0, 15, 0, 0, true, false, true, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_NONE\}",
    )
    self.assertIn(
        '{ROL_SOURCE_PERIODIC_ROOM_ACTION, true, "$n says: \'You may plead your case now.\'", '
        "NULL, NULL}",
        compact,
    )
    self.assertIn(
        '{ROL_SOURCE_PERIODIC_ROOM_ACTION, true, "$n says: \'Can we get on with this?\'", '
        "NULL, NULL}",
        compact,
    )

  def test_zhentil_profiles_preserve_switch_and_zero_roll_conditionals(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )
    compact = " ".join(generated.split())

    self.assertRegex(
        compact,
        r"\{2081021, ROL_SOURCE_PERIODIC_ZK_MINSTREL, 0, 20, 0, 0, true, false, true, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_NONE\}",
    )
    self.assertRegex(
        compact,
        r"\{2081054, ROL_SOURCE_PERIODIC_ZK_LITTLE_GIRL, 0, 10, 0, 0, true, false, true, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_NONE\}",
    )
    self.assertIn(
        '{ROL_SOURCE_PERIODIC_ROOM_ACTION, false, "$n wiggles $s bottom.", NULL, NULL}',
        compact,
    )
    self.assertIn(
        '{ROL_SOURCE_PERIODIC_ROOM_ACTION, true, "$n pauses and scribbles some figures in a '
        'notebook.", NULL, NULL}',
        compact,
    )
    self.assertIn(
        '{ROL_SOURCE_PERIODIC_ROOM_ACTION, false, "$n scratches at an itch.", NULL, NULL}',
        compact,
    )

  def test_undermountain_socials_preserve_source_ranges_gates_and_messages(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )
    compact = " ".join(generated.split())

    for vnum, handler in (
        (2093012, "UM2_MADMAGESOCIALS"),
        (2093021, "UM2_JURISSOCIALS"),
        (2093022, "UM2_DERIAHSOCIALS"),
        (2093023, "UM2_TALUGENSOCIALS"),
        (2093202, "UM2_SUCCUBUSSOCIALS"),
        (2093211, "UM2_SHATARSOCIALS"),
        (2093225, "UM2_IMPSOCIALS"),
        (2093304, "UM2_DEVILSOCIALS"),
    ):
      self.assertRegex(
          compact,
          rf"\{{{vnum}, ROL_SOURCE_PERIODIC_{handler}, 0, 100, 0, 0, true, false, true, "
          r"ROL_SOURCE_PERIODIC_DEVOUR_NONE\}",
      )
    self.assertIn(
        '{ROL_SOURCE_PERIODIC_SPEECH, false, "Now where did I place that vial...", '
        "NULL, NULL}",
        compact,
    )
    self.assertIn(
        '{ROL_SOURCE_PERIODIC_ROOM_ACTION, false, "$n frowns.", NULL, NULL}',
        compact,
    )
    self.assertIn(
        "{ROL_SOURCE_PERIODIC_ROOM_ACTION, false, \"$n rasps, 'So you've come to torment "
        "Bhara'Tir!'\", NULL, NULL}",
        compact,
    )

  def test_targeted_socials_preserve_source_targets_and_messages(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )
    compact = " ".join(generated.split())

    self.assertIn(
        '{ROL_SOURCE_PERIODIC_TARGET_ACTION, false, "$n pinches $N\'s cheeks, leaving a '
        'bright-red blemish there.", "wench", "$n pinches your cheeks, and you reflexively '
        'jump up in the air."}',
        compact,
    )
    self.assertIn(
        '{ROL_SOURCE_PERIODIC_TARGET_ACTION, false, "$n pokes $N in the ribs.", "magus", '
        '"$n pokes you in the ribs. What!?"}',
        compact,
    )

  def test_devour_composition_is_explicit_and_ordered(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )
    compact = " ".join(generated.split())

    self.assertEqual({"bs_wolf": "before", "dog_one": "after"}, DEVOUR_PROFILE_ORDER)
    self.assertRegex(
        compact,
        r"\{2007140, ROL_SOURCE_PERIODIC_BS_WOLF, 0, 100, 0, 0, true, false, true, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_BEFORE\}",
    )
    self.assertRegex(
        compact,
        r"\{2003062, ROL_SOURCE_PERIODIC_DOG_ONE, 2, 8, 2, 4, true, false, false, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_AFTER\}",
    )

  def test_dice_and_sleeping_profiles_preserve_source_gates(self) -> None:
    generated = (self.root / "src/spec/spec_rol_periodic_profiles.inc").read_text(
        encoding="ascii"
    )
    compact = " ".join(generated.split())

    self.assertRegex(
        compact,
        r"\{2003212, ROL_SOURCE_PERIODIC_GUARD_TWO, 2, 8, 2, 4, false, true, false, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_NONE\}",
    )
    self.assertRegex(
        compact,
        r"\{2012000, ROL_SOURCE_PERIODIC_SNOWBEAST, 3, 18, 3, 6, false, false, true, "
        r"ROL_SOURCE_PERIODIC_DEVOUR_NONE\}",
    )


if __name__ == "__main__":
  unittest.main()
