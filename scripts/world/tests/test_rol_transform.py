from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.mobiles import parse_mobile_file
from wtool_lib.objects import parse_object_file
from wtool_lib.rol_source import parse_rol_source_file
from wtool_lib.rol_transform import convert_text, emit_mobile, emit_object, emit_room, emit_zone
from wtool_lib.rooms import parse_room_file
from wtool_lib.zones import parse_zone_file


def _resolver(kind: str, vnum: int) -> int:
  offsets = {"wld": 2_000_000, "mob": 2_000_000, "obj": 2_000_000}
  return vnum + offsets[kind]


class RolTransformTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    root = default_repo_root()
    cls.manifest = load_manifest(root / "scripts/world/wtool_constants.json")

  def _source_record(self, kind: str, data: bytes):
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / f"sample.{kind}"
    path.write_bytes(data)
    records, corpus = parse_rol_source_file(
        path, f"areas/{kind}/sample.{kind}", kind, "sample"
    )
    self.assertTrue(corpus.complete)
    return records[0]

  def _target_path(self, suffix: str, text: str) -> Path:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / f"20000.{suffix}"
    path.write_text(text + "$~\n", encoding="ascii", newline="\n")
    return path

  def test_color_and_line_endings_are_canonicalized(self) -> None:
    text, diagnostics = convert_text("&+RRed&N\r\nplain")
    self.assertEqual("@RRed@n\nplain", text)
    self.assertEqual([], diagnostics)

  def test_emitted_room_parses_as_modern_target_data(self) -> None:
    source = self._source_record(
        "wld",
        b"<*> File Version 1 <*>\n#100\n&+RRoom&N~\nDescription~\n"
        b"1 26 2 5 5 5 0\nD0\nDoor~\nkey~\n449 200 101\n"
        b"E\nsign~\nA sign.~\nS\n",
    )
    emitted = emit_room(source, 2_000_100, 20_001, _resolver)
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/20001.wld", self.manifest, False, set())
    self.assertTrue(result.complete)
    self.assertEqual(1, len(result.records))
    room = result.records[0]
    self.assertEqual(20_001, room.file_zone)
    self.assertEqual(2_000_101, room.exits[0].destination_vnum)
    self.assertEqual(2_000_200, room.exits[0].key_vnum)
    self.assertEqual(8, room.exits[0].door_flags)

  def test_emitted_mobile_maps_flags_position_class_and_race(self) -> None:
    source = self._source_record(
        "mob",
        b"<*> File Version 1 <*>\n#300\nspider~\na spider~\nA spider waits.\n~\n"
        b"A spider.\n~\n38 8 0 -100 S\nAS 0 0\n10 2 50 2d8+5 1d4+1\n"
        b"1.2.3.4 500\n131 131 2 6\n",
    )
    emitted = emit_mobile(source, 2_000_300)
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20001.mob", self.manifest, set())
    self.assertTrue(result.complete)
    mobile = result.records[0]
    self.assertEqual(9, mobile.position)
    self.assertEqual(1, int(mobile.enhanced["Class"][0]))
    self.assertEqual(15, int(mobile.enhanced["Race"][0]))
    self.assertEqual(4321, mobile.gold)

  def test_emitted_object_maps_container_key_affects_and_apply(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\ncontainer box~\na box~\nA box is here.~\n~\n"
        b"15 73 3\n0 0 201 0\n2 10 0\n2\n0\n"
        b"A\n1 2\nE\nbox~\nIt is sturdy.~\n",
    )
    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())
    self.assertTrue(result.complete)
    obj = result.records[0]
    self.assertEqual(15, obj.item_type)
    self.assertEqual(2_000_201, obj.values[2])
    self.assertEqual(9, obj.affects[0].modifier)
    self.assertEqual(1, len(obj.extra_descriptions))

  def test_emitted_zone_normalizes_extended_resets(self) -> None:
    source = self._source_record(
        "zon",
        b"#100\nfile~\nPilot~\n199 30 2 64\n"
        b"0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n"
        b"D 0 100 1 8\nD 0 100 2 3\nR 0 100 200 35\nF 2 100 300 301\n"
        b"E 1 200 1 24\nT 0 2 0 0\nX 1 -1 300 1 25\nS\n",
    )
    emitted = emit_zone(source, 20_100, 2_000_100, _resolver)
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "20100.zon"
    path.write_text(emitted.text, encoding="ascii", newline="\n")
    result = parse_zone_file(path, "zon/20100.zon", self.manifest, 6)

    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(
        ["K", "K", "R", "F", "C", "X"],
        [command.command for command in result.records[0].commands],
    )
    self.assertEqual(35, result.records[0].commands[2].probability)
    self.assertEqual(2, result.records[0].commands[3].dependency)
    self.assertEqual(25, result.records[0].commands[5].probability)
    self.assertIn("unsupported equipment position 24", " ".join(emitted.diagnostics))


if __name__ == "__main__":
  unittest.main()
