from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.flags import encode_bits
from wtool_lib.models import (
    ExitRecord,
    MobileRecord,
    ObjectRecord,
    ResetCommandRecord,
    RoomRecord,
    SourceSpan,
    TriggerRecord,
    ZoneRecord,
)
from wtool_lib.rooms import parse_room_file
from wtool_lib.semantics import strip_color_codes, validate_semantics
from wtool_lib.spec_registry import extract_spec_names


def span(line: int = 1) -> SourceSpan:
  return SourceSpan("semantic-fixture", line)


def zone(vnum: int, bottom: int, top: int, **values) -> ZoneRecord:
  defaults = {
      "name": "Complete Zone",
      "bottom": bottom,
      "top": top,
      "flags": ["0", "0", "0", "0"],
      "min_level": 1,
      "max_level": 20,
  }
  defaults.update(values)
  return ZoneRecord(vnum, span(vnum), str(vnum), **defaults)


def room(vnum: int, owner: int, **values) -> RoomRecord:
  defaults = {
      "name": "Complete Room",
      "description": "A finished room.",
      "flags": ["0", "0", "0", "0"],
      "sector": 0,
      "owner_zone_vnum": owner,
  }
  defaults.update(values)
  return RoomRecord(vnum, span(vnum), str(owner), **defaults)


def exit_record(direction: int, destination: int, **values) -> ExitRecord:
  defaults = {
      "description": None,
      "keyword": None,
      "door_flags": 0,
      "key_vnum": -1,
  }
  defaults.update(values)
  return ExitRecord(
      direction,
      defaults["description"],
      defaults["keyword"],
      defaults["door_flags"],
      defaults["key_vnum"],
      destination,
      span(destination),
  )


def obj(vnum: int, item_type: int, values: list[int], **fields) -> ObjectRecord:
  vector = (values + [0] * 16)[:16]
  defaults = {
      "aliases": "finished object",
      "short_description": "a finished object",
      "description": "A finished object lies here.",
      "item_type": item_type,
      "values": vector,
  }
  defaults.update(fields)
  return ObjectRecord(vnum, span(vnum), "1", **defaults)


class SemanticTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.manifest = load_manifest()
    cls.spec_names = extract_spec_names(cls.repo_root)
    cls.item_types = {
        entry["macro"]: entry["index"]
        for entry in cls.manifest["tables"]["item-types"]["entries"]
        if entry.get("macro")
    }

  def validate(
      self,
      zones: list[ZoneRecord],
      rooms: list[RoomRecord],
      mobiles: list[MobileRecord] | None = None,
      objects: list[ObjectRecord] | None = None,
      triggers: list[TriggerRecord] | None = None,
  ):
    return validate_semantics(
        zones,
        rooms,
        mobiles or [],
        objects or [],
        triggers or [],
        None,
        self.manifest,
        6,
    )

  def test_color_stripping_placeholder_and_runtime_flags(self) -> None:
    room_flags = list(encode_bits({15, 29, 33}))
    action_flags = list(encode_bits({3, 20}))
    affect_flags = list(encode_bits({6, 9, 12}))
    test_room = room(
        100,
        1,
        name="@rAn unfinished room@n",
        flags=room_flags,
    )
    mobile = MobileRecord(
        200,
        span(200),
        "1",
        aliases="finished mobile",
        short_description="a finished mobile",
        long_description="A finished mobile stands here.",
        description="A finished mobile.",
        action_flags=action_flags,
        affect_flags=affect_flags,
    )
    charm_bit = next(
        entry["index"]
        for entry in self.manifest["tables"]["affect"]["entries"]
        if entry["macro"] == "AFF_CHARM"
    )
    charm_flags = list(encode_bits({charm_bit}))
    test_object = obj(
        300,
        self.item_types["ITEM_KEY"],
        [0, 0, 0, 0],
        affect_flags=charm_flags,
    )
    findings = self.validate(
        [zone(1, 100, 199)], [test_room], [mobile], [test_object]
    )
    messages = "\n".join(item.message for item in findings if item.code == "SEM003")
    self.assertIn("ROOM_BFS_MARK", messages)
    self.assertIn("ROOM_OCCUPIED", messages)
    self.assertIn("ROOM_HASTRAP", messages)
    self.assertNotIn("MOB_ISNPC", messages)
    self.assertIn("MOB_NOTDEADYET", messages)
    self.assertIn("AFF_POISON", messages)
    self.assertIn("AFF_CHARM", messages)
    self.assertNotIn("AFF_GROUP", messages)
    self.assertIn("SEM004", {item.code for item in findings})
    self.assertEqual("@red\ttext", strip_color_codes("@@red\t\ttext"))

  def test_exit_consistency_no_exit_fallback_and_unreachable_codes(self) -> None:
    first = room(
        100,
        1,
        exits=[
            exit_record(0, 101, keyword="door", door_flags=1, key_vnum=300),
            exit_record(1, 103),
        ],
    )
    second = room(
        101,
        1,
        exits=[exit_record(2, 100, keyword="gate", door_flags=0, key_vnum=301)],
    )
    isolated = room(102, 1)
    one_way_target = room(103, 1)
    findings = self.validate([zone(1, 100, 199)], [first, second, isolated, one_way_target])
    codes = {item.code for item in findings}
    self.assertTrue(
        {"SEM005", "SEM006", "SEM007", "SEM008", "SEM009", "SEM010", "SEM011"}
        <= codes
    )
    unreachable = [item for item in findings if item.code == "SEM011"]
    self.assertTrue(all("roots [100]" in item.message for item in unreachable))

  def test_cross_zone_incoming_exit_is_reachability_root(self) -> None:
    external = room(50, 1, exits=[exit_record(0, 100)])
    root = room(100, 2, exits=[exit_record(0, 101)])
    reachable = room(101, 2)
    unreachable = room(102, 2)
    findings = self.validate(
        [zone(1, 50, 99), zone(2, 100, 199)],
        [external, root, reachable, unreachable],
    )
    target = next(item for item in findings if item.code == "SEM011" and item.vnum == 102)
    self.assertIn("roots [100]", target.message)
    self.assertFalse(any(item.code == "SEM010" and item.vnum == 2 for item in findings))

  def test_reset_level_band_and_open_dangerous_room(self) -> None:
    death_bit = next(
        entry["index"]
        for entry in self.manifest["tables"]["room"]["entries"]
        if entry["macro"] == "ROOM_DEATH"
    )
    test_zone = zone(1, 100, 199, min_level=1, max_level=5)
    test_zone.commands.append(
        ResetCommandRecord("M", 0, [200, 1, 100], [], span(20), probability=100)
    )
    test_room = room(100, 1, flags=list(encode_bits({death_bit})))
    mobile = MobileRecord(
        200,
        span(200),
        "1",
        aliases="finished mobile",
        short_description="a finished mobile",
        long_description="A finished mobile stands here.",
        description="A finished mobile.",
        level=10,
    )
    codes = {item.code for item in self.validate([test_zone], [test_room], [mobile])}
    self.assertTrue({"SEM012", "SEM013"} <= codes)

  def test_object_value_contract_codes(self) -> None:
    objects = [
        obj(300, self.item_types["ITEM_POTION"], [35, 2001, 0, 0]),
        obj(301, self.item_types["ITEM_WAND"], [1, 2, 3, 1]),
        obj(302, self.item_types["ITEM_DRINKCON"], [-2, 5, 23, 0]),
        obj(303, self.item_types["ITEM_CONTAINER"], [-2, 16, -1, 0]),
        obj(304, self.item_types["ITEM_FURNITURE"], [11, 12, 0, 0]),
        obj(305, self.item_types["ITEM_LIGHT"], [0, 0, -2, 0]),
        obj(306, self.item_types["ITEM_PORTAL"], [1, 200, 100, 0]),
        obj(307, self.item_types["ITEM_TRAP"], [6, 0, 0, 0]),
        obj(308, self.item_types["ITEM_SWITCH"], [2, 100, 6, 3]),
        obj(309, self.item_types["ITEM_SWITCH"], [0, 100, 0, 0]),
    ]
    objects[0].weapon_spells.append(([2001, 0, 51, 2], span(40)))
    objects[0].activated_spells.append(([31, 528, 6, 5, -1], span(41)))
    findings = self.validate([zone(1, 100, 199)], [room(100, 1)], objects=objects)
    codes = {item.code for item in findings}
    self.assertTrue(
        {
            "SEM014",
            "SEM015",
            "SEM016",
            "SEM017",
            "SEM018",
            "SEM019",
            "SEM020",
            "SEM021",
            "SEM022",
        }
        <= codes
    )

  def test_sector_and_duplicate_exit_have_parser_level_semantic_codes(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "1.wld"
      sector_count = len(self.manifest["tables"]["sectors"]["entries"])
      path.write_text(
          "#100\nFinished Room~\nA finished room.~\n"
          f"1 0 0 0 0 {sector_count}\n"
          "D0\n~\n~\n0 -1 -1\nD0\n~\n~\n0 -1 -1\nS\n$~\n",
          encoding="ascii",
      )
      result = parse_room_file(path, "1.wld", self.manifest, False, self.spec_names)
    self.assertTrue({"SEM001", "SEM002"} <= {item.code for item in result.findings})


if __name__ == "__main__":
  unittest.main()
