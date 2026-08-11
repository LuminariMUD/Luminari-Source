from __future__ import annotations

from pathlib import Path
import json
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.hlquests import parse_hlquest_file
from wtool_lib.mobiles import parse_mobile_file
from wtool_lib.objects import parse_object_file
from wtool_lib.rol_source import parse_rol_source_file
from wtool_lib.rol_discovery import extract_source_commands
from wtool_lib.rol_pilot import PILOT_BASENAMES
from wtool_lib.rol_soc import build_soc_prototype_comparison, compile_soc_records
from wtool_lib.rol_transform import (
    convert_text,
    emit_mobile,
    emit_hlquest,
    emit_object,
    emit_room,
    emit_shop,
    emit_zone,
)
from wtool_lib.rooms import parse_room_file
from wtool_lib.shops import parse_shop_file
from wtool_lib.triggers import parse_trigger_file
from wtool_lib.zones import parse_zone_file


def _resolver(kind: str, vnum: int) -> int:
  offsets = {"wld": 2_000_000, "mob": 2_000_000, "obj": 2_000_000}
  return vnum + offsets[kind]


class RolTransformTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    root = default_repo_root()
    cls.root = root
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

  def test_emitted_shop_maps_products_prices_hours_and_roaming(self) -> None:
    source = self._source_record(
        "shp",
        b"SHOP: 300\nHOURS: 8-12 13-17 18-20\nROOM: 100\nGREED: 140\n"
        b"PROFIT: 90\nCASTING:\nDEADBEAT: 1\nOFFENSE: 2\nCHEATS: GOODS\n"
        b"HATES: NPC\nPO: 200\nBT: 5 11\nMBCASH: $n says 'No cash, $N.'\n"
        b"MBHAVE: $n says '$N does not have that.'\nMBIGOT: Go away.\n"
        b"MBUY: $n says 'Here is your %s.'\nMCLOSE: Closed.\n"
        b"MNBUY: $n says 'I do not buy $p.'\nMOPEN: Open.\n"
        b"MSCASH: $n says 'I cannot afford $p, $N.'\n"
        b"MSELL: $n says 'Sold to $N for %s.'\nMSHAVE: $n says 'I do not have $p.'\n",
    )
    emitted = emit_shop(source, 2_000_300, _resolver)
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "20003.shp"
    path.write_text(
        "CircleMUD v3.0 Shop File~\n" + emitted.text + "$~\n",
        encoding="ascii",
        newline="\n",
    )
    result = parse_shop_file(path, "shp/20003.shp", self.manifest)

    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    shop = result.records[0]
    self.assertEqual([2_000_200], shop.product_vnums)
    self.assertEqual([5, 11], [entry.item_type for entry in shop.buy_types])
    self.assertEqual([2_000_100], shop.room_vnums)
    self.assertEqual([8, 20, 0, 0], shop.open_hours)
    self.assertEqual(1.4, shop.profit_buy)
    self.assertAlmostEqual(0.5263, shop.profit_sell or 0.0, places=4)
    self.assertIn("%d coins", shop.messages[5])
    self.assertIn("source-only shop behavior", " ".join(emitted.diagnostics))

  def test_emitted_roaming_shop_sets_native_compatibility_flag(self) -> None:
    source = self._source_record(
        "shp",
        b"SHOP: 300\nROAMING:\nHOURS: 1-23\nGREED: 100\nPROFIT: 100\n",
    )
    emitted = emit_shop(source, 2_000_300, _resolver)
    lines = emitted.text.splitlines()
    self.assertEqual("32", lines[-8])

  def test_emitted_hlquest_preserves_runtime_direction_order(self) -> None:
    source = self._source_record(
        "qst",
        b"#300\nM\nquestion~\nanswer~\nQ\ncomplete\n~\n"
        b"G\nI 201\nG\nI 201\nG\nC 5000000\n"
        b"R\nI 202\nR\nC 25\nD\nvanishes\n~\nS\n",
    )
    emitted = emit_hlquest(source, 2_000_300, _resolver)
    path = self._target_path("hlq", emitted.text)
    result = parse_hlquest_file(path, "hlq/20000.hlq", self.manifest)

    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    record = result.records[0]
    self.assertEqual(2_000_300, record.host_mobile_vnum)
    self.assertTrue(all(entry.approved for entry in record.entries))
    give = record.entries[1]
    self.assertEqual([2_000_201, 2_000_201, 5000000], [item.value for item in give.commands[:3]])
    self.assertEqual(["C", "I", "D"], [item.code for item in give.output_commands])
    self.assertIn("vanishes", give.reply_message)

  def test_selected_pilot_quests_all_emit_valid_target_records(self) -> None:
    selection = self.root / "lib/rol-conversion/runs/phase4-select-e6ea7982"
    actions = [
        json.loads(line)
        for line in (selection / "pilot-actions.jsonl").read_text(encoding="ascii").splitlines()
    ]
    identities = [
        json.loads(line)
        for line in (
            self.root / "lib/rol-conversion/runs/phase2-e6ea7982/identity-map.jsonl"
        ).read_text(encoding="ascii").splitlines()
    ]
    identity_map = {
        (row["source_kind"], row["source_vnum"]): row["destination_vnum"]
        for row in identities
    }
    selected = {
        (row["basename"], row["source_vnum"]): row["destination_vnum"]
        for row in actions
        if row["source_kind"] == "qst" and row["action"] == "ADD"
    }

    def resolver(kind: str, vnum: int) -> int:
      source_kind = {"wld": "wld", "mob": "mob", "obj": "obj"}[kind]
      return identity_map[(source_kind, vnum)]

    emitted_count = 0
    for basename in sorted({basename for basename, _ in selected}):
      source_path = self.root / f"EXAMPLE/RealmsOfLuminari/areas/qst/{basename}.qst"
      records, corpus = parse_rol_source_file(
          source_path, f"areas/qst/{basename}.qst", "qst", basename
      )
      self.assertTrue(corpus.complete)
      text = ""
      for record in records:
        destination = selected.get((basename, record.vnum))
        if destination is None:
          continue
        converted = emit_hlquest(record, destination, resolver)
        self.assertFalse(any("excluded" in item for item in converted.diagnostics))
        text += converted.text
        emitted_count += 1
      temporary = tempfile.TemporaryDirectory()
      self.addCleanup(temporary.cleanup)
      target_path = Path(temporary.name) / f"{basename}.hlq"
      target_path.write_text(text + "$~\n", encoding="ascii", newline="\n")
      result = parse_hlquest_file(target_path, f"hlq/{basename}.hlq", self.manifest)
      self.assertTrue(result.complete)
      self.assertEqual([], result.findings)

    self.assertEqual(57, emitted_count)

  def test_selected_pilot_shops_all_emit_valid_target_records(self) -> None:
    cases = (("hulburg", 100_000), ("muspel", 2_000_000))
    for basename, offset in cases:
      with self.subTest(basename=basename):
        source_path = self.root / f"EXAMPLE/RealmsOfLuminari/areas/shp/{basename}.shp"
        records, corpus = parse_rol_source_file(
            source_path,
            f"areas/shp/{basename}.shp",
            "shp",
            basename,
        )
        self.assertTrue(corpus.complete)

        def resolver(kind: str, vnum: int) -> int:
          del kind
          return vnum + offset

        text = "CircleMUD v3.0 Shop File~\n"
        text += "".join(emit_shop(record, record.vnum + offset, resolver).text for record in records)
        text += "$~\n"
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        target_path = Path(temporary.name) / f"{basename}.shp"
        target_path.write_text(text, encoding="ascii", newline="\n")
        result = parse_shop_file(target_path, f"shp/{basename}.shp", self.manifest)
        self.assertTrue(result.complete)
        self.assertEqual([], result.findings)
        self.assertEqual(len(records), len(result.records))

  def test_soc_compiler_emits_exact_chance_flags_paths_and_filtered_echoes(self) -> None:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    source_path = Path(temporary.name) / "sample.soc"
    source_path.write_bytes(
        b"MOB: 300 PATH\nID: 1\nTYPE: 1\nDELAY: 2\n"
        b"ROOMS: 100 101\nDIRS: 1\nDONE\n"
        b"MOB: 300 TRIGGER\nTRIGGER: 23\nFLAG: 165\nCHANCE: 3\nDELAY: 0\n"
        b"ACTION: 1000\nAn indoor echo.\n~\nFLAG: 0\nCHANCE: 0\nDELAY: 4\n"
        b"ACTION: 1004\n1\n~\nDONE\n"
    )
    records, corpus = parse_rol_source_file(
        source_path, "areas/soc/sample.soc", "soc", "sample"
    )
    self.assertTrue(corpus.complete)
    compiled = compile_soc_records(
        records,
        2_030_000,
        _resolver,
        {23: "smile"},
    )

    self.assertEqual(2, compiled.source_records)
    self.assertEqual(2, compiled.source_actions)
    self.assertEqual(1, len(compiled.triggers))
    text = compiled.trigger_text
    self.assertIn("if %random.4% == 1", text)
    self.assertIn("if !%arg% || !(%self.name% /= %arg.car%)", text)
    self.assertIn("if %actor.is_pc%", text)
    self.assertIn("mrolzoneecho indoors %self.room.vnum% An indoor echo.", text)
    self.assertIn("wait 4", text)
    self.assertIn("wait 5 s\n        mrolwalkto 2000101", text)

  def test_all_pilot_soc_compiles_to_valid_target_triggers(self) -> None:
    records = []
    for basename in PILOT_BASENAMES:
      source_path = self.root / f"EXAMPLE/RealmsOfLuminari/areas/soc/{basename}.soc"
      if not source_path.is_file():
        continue
      parsed, corpus = parse_rol_source_file(
          source_path, f"areas/soc/{basename}.soc", "soc", basename
      )
      self.assertTrue(corpus.complete)
      records.extend(parsed)

    command_evidence = extract_source_commands(
        self.root / "EXAMPLE/RealmsOfLuminari"
    )
    commands = {
        row["action_code"]: row["command"]
        for row in command_evidence["commands"]
    }
    compiled = compile_soc_records(records, 2_055_300, _resolver, commands)
    self.assertEqual(245, compiled.source_records)
    self.assertEqual(553, compiled.source_actions)
    self.assertEqual(181, len(compiled.triggers))
    self.assertEqual(163, len(compiled.attachments))
    self.assertFalse(
        any(
            word in diagnostic
            for diagnostic in compiled.diagnostics
            for word in ("lacks", "invalid", "missing", "unmapped")
        )
    )

    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    target_path = Path(temporary.name) / "20553.trg"
    target_path.write_text(compiled.trigger_text, encoding="ascii", newline="\n")
    result = parse_trigger_file(
        target_path, "trg/20553.trg", self.manifest
    )
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(181, len(result.records))

    comparison = build_soc_prototype_comparison(records, compiled)
    self.assertEqual("dg_compilation", comparison["selection"])
    self.assertEqual(
        245,
        comparison["native_compatibility_projection"]["persisted_behavior_records"],
    )
    self.assertEqual(
        26.122449,
        comparison["dg_compilation_pilot"]["record_reduction_percent"],
    )
    self.assertEqual(
        {1000: 7, 1001: 31, 1002: 2, 1003: 394, 1004: 14},
        comparison["measured_source"]["special_actions"],
    )


if __name__ == "__main__":
  unittest.main()
