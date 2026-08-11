from __future__ import annotations

from pathlib import Path
import json
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.flags import decode_tokens
from wtool_lib.hlquests import parse_hlquest_file
from wtool_lib.mobiles import parse_mobile_file
from wtool_lib.objects import parse_object_file
from wtool_lib.rol_source import parse_active_rol_corpus, parse_rol_source_file
from wtool_lib.rol_discovery import extract_source_commands
from wtool_lib.rol_pilot import PILOT_BASENAMES
from wtool_lib.rol_soc import build_soc_prototype_comparison, compile_soc_records
from wtool_lib.rol_special import compile_special_bindings
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
        b"E\nsign~\nA sign.~\nR 15 35\nF 30\nM 0 20\nS\n",
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
    self.assertEqual(15, room.minimum_level)
    self.assertEqual(-1, room.maximum_level)
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("fall chance", diagnostics)
    self.assertIn("obsolete source room mana", diagnostics)
    self.assertIn("target maximum level is 34", diagnostics)

  def test_emitted_room_preserves_room_and_zone_compatibility(self) -> None:
    first_mask = sum(1 << (flag - 1) for flag in (6, 11, 13, 15, 18, 31, 32))
    second_mask = sum(1 << (flag - 33) for flag in (36, 48))
    source = self._source_record(
        "wld",
        (
            "<*> File Version 1 <*>\n#100\nCompatibility room~\nDescription~\n"
            f"1 {first_mask} 2 5 5 5 {second_mask}\nS\n"
        ).encode("ascii"),
    )
    zone_flags = sum(1 << bit for bit in (0, 1, 4, 5, 6, 7))
    emitted = emit_room(
        source,
        2_000_100,
        20_001,
        _resolver,
        source_zone_flags=zone_flags,
    )
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/20001.wld", self.manifest, False, set())
    self.assertTrue(result.complete)
    room = result.records[0]
    flags = decode_tokens(room.flags).bits

    self.assertEqual(9, room.sector)
    self.assertTrue({4, 5, 17, 19, 21, 23, 24, 27, 42, 43, 44, 45} <= flags)
    self.assertNotIn(7, flags)
    self.assertNotIn("room flags without target persistence", " ".join(emitted.diagnostics))

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

  def test_emitted_object_preserves_rol_compatibility_flags(self) -> None:
    source_bits = (1, 2, 4, 16, 17, 20, 21, 22, 24, 29, 30)
    source_mask = sum(1 << bit for bit in source_bits)
    source = self._source_record(
        "obj",
        (
            "#200\ncompatibility item~\na compatibility item~\n"
            "A compatibility item is here.~\n~\n"
            f"12 {source_mask} 1\n0 0 0 0\n1 1 0\n0\n0\n"
        ).encode("ascii"),
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    flags = decode_tokens(result.records[0].extra_flags).bits
    self.assertEqual({39, *range(116, 125)}, flags)
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("omitted source-inert object DARK flag", diagnostics)
    self.assertNotIn("object extra flags without direct equivalents", diagnostics)

  def test_emitted_object_preserves_source_trap_without_colliding_with_dg_trigger(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\ntrapped chest~\na trapped chest~\nA trapped chest is here.~\n~\n"
        b"15 0 1\n0 0 -1 0\n10 100 0\n0\n0\n"
        b"T\n2562 11 3 25 2 6\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver, attachments=(2_100_001,))
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    obj = result.records[0]
    self.assertEqual([2562, 11, 3, 25, 2, 6], obj.values[10:16])
    self.assertEqual([2_100_001], [attachment.trigger_vnum for attachment in obj.attachments])
    self.assertEqual(["0", "0", "0", "r"], obj.extra_flags)
    self.assertIn("converted source object trap", " ".join(emitted.diagnostics))

  def test_emitted_object_excludes_empty_source_trap(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nplain box~\na plain box~\nA plain box is here.~\n~\n"
        b"15 0 1\n0 0 -1 0\n10 100 0\n0\n0\nT\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual([0] * 6, result.records[0].values[10:16])
    self.assertNotEqual(["0", "0", "0", "r"], result.records[0].extra_flags)
    self.assertIn("inactive/malformed source object trap", " ".join(emitted.diagnostics))

  def test_emitted_object_rejects_invalid_source_trap_damage_type(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\ntrapped box~\na trapped box~\nA trapped box is here.~\n~\n"
        b"15 0 1\n0 0 -1 0\n10 100 0\n0\n0\nT 2 99 1 10 1 6\n",
    )

    with self.assertRaisesRegex(ValueError, "invalid damage type 99"):
      emit_object(source, 2_000_200, _resolver)

  def test_all_active_source_object_traps_have_explicit_dispositions(self) -> None:
    corpus = parse_active_rol_corpus(self.root / "EXAMPLE/RealmsOfLuminari", self.root)
    trapped = [
        record
        for record in corpus.records
        if record.kind == "obj" and any(row["token"] == "T" for row in record.directives)
    ]
    converted = 0
    excluded = 0

    for ordinal, record in enumerate(trapped):
      emitted = emit_object(record, 2_500_000 + ordinal, _resolver)
      path = self._target_path("obj", emitted.text)
      parsed = parse_object_file(path, "obj/25000.obj", self.manifest, set())
      self.assertTrue(parsed.complete, record.record_id)
      diagnostics = " ".join(emitted.diagnostics)
      if "converted source object trap" in diagnostics:
        converted += 1
        self.assertNotEqual([0] * 6, parsed.records[0].values[10:16])
      else:
        excluded += 1
        self.assertIn("inactive/malformed", diagnostics)

    self.assertEqual(33, len(trapped))
    self.assertEqual(29, converted)
    self.assertEqual(4, excluded)

  def test_emitted_magic_item_caps_source_level_at_target_maximum(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\npotion~\na potion~\nA potion is here.~\n~\n"
        b"10 0 1\n50 9 0 0\n1 1 0\n0\n0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(34, result.records[0].values[0])
    self.assertIn("capped source magic-item spell level 50", " ".join(emitted.diagnostics))

  def test_emitted_magic_item_maps_spells_by_source_name(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\npotion~\na potion~\nA potion is here.~\n~\n"
        b"10 0 1\n20 41 0 0\n1 1 0\n0\n0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(120, result.records[0].values[1])
    self.assertIn("source spell 41 (haste)", " ".join(emitted.diagnostics))

  def test_emitted_magic_item_disables_spell_without_target_equivalent(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nwand~\na wand~\nA wand is here.~\n~\n"
        b"3 0 1\n20 2 2 453\n1 1 0\n0\n0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(0, result.records[0].values[3])
    self.assertIn("mud to rock", " ".join(emitted.diagnostics))

  def test_emitted_zone_normalizes_extended_resets(self) -> None:
    source = self._source_record(
        "zon",
        b"#100\nfile~\nPilot~\n199 30 2 64\n"
        b"0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n"
        b"D 0 100 1 8\nD 0 100 2 3\nR 0 100 200 35\nF 2 100 300 301\n"
        b"M 0 300 1 100\nE 1 200 1 24\nT 0 2 0 0\nX 1 -1 300 1 25\nS\n",
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
        ["K", "K", "R", "F", "M", "C", "X"],
        [command.command for command in result.records[0].commands],
    )
    self.assertEqual(35, result.records[0].commands[2].probability)
    self.assertEqual(2, result.records[0].commands[3].dependency)
    self.assertEqual(25, result.records[0].commands[6].probability)
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

  def test_emitted_hlquest_adapts_extended_reward_contracts(self) -> None:
    source = self._source_record(
        "qst",
        b"#300\nQ\ncomplete\n~\n"
        b"R\nA\nR\nE 1000\nR\nP -100\nR\nS 72\nS\n",
    )
    emitted = emit_hlquest(source, 2_000_300, _resolver)
    path = self._target_path("hlq", emitted.text)
    result = parse_hlquest_file(path, "hlq/20000.hlq", self.manifest)

    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    give = result.records[0].entries[0]
    self.assertEqual(["T", "P", "E", "A"], [item.code for item in give.output_commands])
    self.assertEqual([74, -100, 1000, 0], [item.value for item in give.output_commands])
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("meteor swarm", diagnostics)

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

  def test_all_pilot_special_bindings_have_valid_native_or_dg_output(self) -> None:
    selection = self.root / "lib/rol-conversion/runs/phase4-select-e6ea7982"
    bindings = [
        json.loads(line)
        for line in (selection / "pilot-special-bindings.jsonl")
        .read_text(encoding="ascii")
        .splitlines()
    ]
    identities = [
        json.loads(line)
        for line in (
            self.root / "lib/rol-conversion/runs/phase2-e6ea7982/identity-map.jsonl"
        )
        .read_text(encoding="ascii")
        .splitlines()
    ]
    identity_map = {
        (row["source_kind"], row["source_vnum"]): row["destination_vnum"]
        for row in identities
    }

    def resolver(kind: str, vnum: int) -> int:
      return identity_map[(kind, vnum)]

    rooms = []
    for basename in PILOT_BASENAMES:
      source_path = self.root / f"EXAMPLE/RealmsOfLuminari/areas/wld/{basename}.wld"
      parsed, corpus = parse_rol_source_file(
          source_path, f"areas/wld/{basename}.wld", "wld", basename
      )
      self.assertTrue(corpus.complete)
      rooms.extend(parsed)

    compiled = compile_special_bindings(bindings, 2_055_481, resolver, rooms)
    self.assertEqual(91, compiled.source_bindings)
    self.assertEqual(46, len(compiled.native_bindings))
    self.assertEqual(13, len(compiled.triggers))
    self.assertEqual(45, len(compiled.attachments))
    self.assertEqual(91, len(compiled.dispositions))
    self.assertEqual(
        46,
        sum(row["strategy"] == "NATIVE_PERSISTED" for row in compiled.dispositions),
    )
    self.assertIn("wrolroomflag", compiled.trigger_text)
    self.assertIn("wroldamage all-pcs 50 10", compiled.trigger_text)
    self.assertIn("mrolalert %actor%", compiled.trigger_text)
    self.assertIn("flags 2049", compiled.trigger_text)

    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    target_path = Path(temporary.name) / "20554.trg"
    target_path.write_text(compiled.trigger_text, encoding="ascii", newline="\n")
    result = parse_trigger_file(target_path, "trg/20554.trg", self.manifest)
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(13, len(result.records))

    native = next(
        item for item in compiled.native_bindings if item.persisted_name == "obj_drain"
    )
    self.assertEqual((44,), native.required_flag_bits)


if __name__ == "__main__":
  unittest.main()
