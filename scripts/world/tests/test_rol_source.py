from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.rol_source import (
    RolSourceCorpus,
    _parse_obj,
    _parse_qst,
    _parse_shp,
    _parse_soc,
    _parse_wld,
    _parse_zon,
    normalize_identity,
    source_corpus_payload,
)
from wtool_lib.source import SourceFile


def parse_fixture(kind: str, data: bytes):
  with tempfile.TemporaryDirectory() as temporary:
    path = Path(temporary) / f"sample.{kind}"
    path.write_bytes(data)
    source = SourceFile.from_path(path, f"areas/{kind}/sample.{kind}")
    corpus = RolSourceCorpus()
    parser = {
        "wld": _parse_wld,
        "obj": _parse_obj,
        "zon": _parse_zon,
        "qst": _parse_qst,
        "shp": _parse_shp,
        "soc": _parse_soc,
    }[kind]
    records = parser(source, "sample", corpus)
    return records, corpus


class RolSourceTests(unittest.TestCase):
  def test_identity_normalization_strips_legacy_color_and_punctuation(self) -> None:
    self.assertEqual("the old trail", normalize_identity("&+LThe Old-Trail&N\r\n"))

  def test_room_parser_types_exit_key_and_destination(self) -> None:
    records, corpus = parse_fixture(
        "wld",
        b"<*> File Version 1 <*>\n#100\nRoom~ \nDescription~\n1 0 2 10 10 10\n"
        b"D0\nDoor~\nkey~\n1 200 101\nS\n",
    )
    self.assertTrue(corpus.complete)
    self.assertEqual(
        [("object", 200), ("room", 101)],
        [(reference.target_type, reference.target_vnum) for reference in records[0].references],
    )

  def test_object_parser_types_known_value_array_references(self) -> None:
    records, corpus = parse_fixture(
        "obj",
        b"#200\nkey words~\na container~\nA container lies here.~\n~\n"
        b"15 0 1\n0 0 201 0\n1 2 3\n",
    )
    self.assertTrue(corpus.complete)
    self.assertEqual("container_key", records[0].references[0].role)
    self.assertEqual(201, records[0].references[0].target_vnum)

  def test_zone_parser_rejects_false_a_and_preserves_source_defects(self) -> None:
    records, corpus = parse_fixture(
        "zon",
        b"#1\n> filename: sample.zon~\nA Halruaan Airship 1~\n199 15 2 0\n"
        b"0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n"
        b"F2 100 200 201 50\nGROUPING mobs\nS\n",
    )
    self.assertTrue(corpus.complete)
    self.assertEqual(0, sum(item["token"] == "A" for item in records[0].directives))
    follow = next(item for item in records[0].directives if item["token"] == "F")
    self.assertTrue(follow["no_space_opcode"])
    self.assertEqual([2, 100, 200, 201, 50], follow["arguments"])
    self.assertEqual("ROLZON003", corpus.diagnostics[0].code)

  def test_quest_shop_and_soc_references_are_typed(self) -> None:
    quests, quest_corpus = parse_fixture(
        "qst",
        b"#300\nQ\nThanks~\nG I 200\nRM 301 100\nR A\nS\n",
    )
    self.assertTrue(quest_corpus.complete)
    self.assertEqual(
        {("mobile", 300), ("object", 200), ("mobile", 301), ("room", 100)},
        {(reference.target_type, reference.target_vnum) for reference in quests[0].references},
    )

    shops, shop_corpus = parse_fixture(
        "shp", b"SHOP: 300\nROOM: 100\nPO: 200\nBT: 5\n"
    )
    self.assertTrue(shop_corpus.complete)
    self.assertEqual(3, len(shops[0].references))

    socials, soc_corpus = parse_fixture(
        "soc",
        b"MOB: 300 PATH\nID: 1\nTYPE: 0\nDELAY: 2\nROOMS: 100 101\n"
        b"DIRS: 0 1\nDONE\n",
    )
    self.assertTrue(soc_corpus.complete)
    self.assertEqual(3, len(socials[0].references))

  def test_summary_is_stable_and_counts_tokens(self) -> None:
    records, corpus = parse_fixture(
        "shp", b"SHOP: 300\nROOM: 100\nPO: 200\n"
    )
    corpus.records.extend(records)
    payload = source_corpus_payload(corpus)
    self.assertEqual(1, payload["summary"]["records"])
    self.assertEqual(3, payload["summary"]["references"])
    self.assertEqual("PO", payload["directives"][0]["token"])


if __name__ == "__main__":
  unittest.main()
