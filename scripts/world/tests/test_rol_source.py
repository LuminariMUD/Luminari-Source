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
    self.assertEqual("jotunheim", normalize_identity("@WJotunheim@n"))

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
    self.assertEqual("Room", records[0].values["strings"]["name"])
    self.assertEqual("Door", records[0].directives[1]["description"])
    self.assertEqual("key", records[0].directives[1]["keyword"])

  def test_object_parser_types_known_value_array_references(self) -> None:
    records, corpus = parse_fixture(
        "obj",
        b"#200\nkey words~\na container~\nA container lies here.~\n~\n"
        b"15 0 1\n0 0 201 0\n1 2 3\n",
    )
    self.assertTrue(corpus.complete)
    self.assertEqual("container_key", records[0].references[0].role)
    self.assertEqual(201, records[0].references[0].target_vnum)
    self.assertEqual([15, 0, 1], records[0].values["flags"])
    self.assertEqual([1, 2, 3], records[0].values["economy"])

  def test_object_affect_masks_are_only_read_before_extensions(self) -> None:
    records, corpus = parse_fixture(
        "obj",
        b"#200\nodd mace~\nan odd mace~\nAn odd mace lies here.~\n~\n"
        b"5 0 8193\n0 1 6 7\n10 100 1\n32768\n0\n"
        b"E\nmace~\nIt is odd.~\nA\n1 1\n18 -3\n",
    )

    affect_rows = [
        directive
        for directive in records[0].directives
        if directive["token"] == "AFFECT_FLAGS"
    ]
    self.assertEqual([[32768], [0]], [row["arguments"] for row in affect_rows])
    self.assertEqual(
        1,
        sum(
            directive["token"] == "IGNORED_SOURCE_CONTENT"
            for directive in records[0].directives
        ),
    )
    self.assertTrue(corpus.complete)
    self.assertIn("ignores numeric content after extensions", corpus.diagnostics[-1].message)

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

  def test_zone_reset_arguments_stop_before_numeric_comments(self) -> None:
    records, corpus = parse_fixture(
        "zon",
        b"#553\nfile~\nCemetery~\n55399 20 2 0\n"
        b"0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n"
        b"D 0 55302 1 1 * East exit from room 55302\n"
        b"R 1 55310 55301 35 * DC2 has a 5 percent chance\n"
        b"T 0 2 0 0 * at 2am\nS\n",
    )

    commands = [
        item for item in records[0].directives if item["token"] in {"D", "R", "T"}
    ]
    self.assertEqual([0, 55302, 1, 1], commands[0]["arguments"])
    self.assertEqual([1, 55310, 55301, 35], commands[1]["arguments"])
    self.assertEqual([0, 2, 0, 0], commands[2]["arguments"])
    self.assertTrue(corpus.complete)

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

  def test_quest_shop_and_soc_payloads_are_preserved(self) -> None:
    quests, _ = parse_fixture(
        "qst",
        b"#300\nM\nhello~\nWelcome, traveler.~\nQ\nFarewell.~\nS\n",
    )
    self.assertEqual("hello", quests[0].directives[0]["keyword"])
    self.assertEqual("Welcome, traveler.", quests[0].directives[0]["message"])
    self.assertEqual("Farewell.", quests[0].directives[1]["message"])

    shops, _ = parse_fixture(
        "shp", b"SHOP: 300\nMOPEN: Welcome to my shop!\nPROFIT: 120 80\n"
    )
    self.assertEqual("Welcome to my shop!", shops[0].directives[1]["text"])

    socials, _ = parse_fixture(
        "soc",
        b"MOB: 300 PERIODIC\nFLAG: 0\nCHANCE: 3\nDELAY: 0\n"
        b"ACTION: 1003\nA room echo.\n~\nDONE\n",
    )
    action = next(item for item in socials[0].directives if item["token"] == "ACTION")
    self.assertEqual("A room echo.\r\n", action["argument"])

  def test_soc_list_continues_through_done_until_listdone(self) -> None:
    socials, corpus = parse_fixture(
        "soc",
        b"MOB: 300 LIST\nFLAG: 0\nCHANCE: 3\nDELAY: 0\n"
        b"ACTION: 1003\nFirst.\n~\nDONE\n"
        b"FLAG: 0\nCHANCE: 4\nDELAY: 2\n"
        b"ACTION: 1002\nSecond.\n~\nLISTDONE\n",
    )

    self.assertTrue(corpus.complete)
    self.assertEqual(
        [1003, 1002],
        [
            item["arguments"][0]
            for item in socials[0].directives
            if item["token"] == "ACTION"
        ],
    )
    self.assertEqual("LISTDONE", socials[0].directives[-1]["token"])

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
