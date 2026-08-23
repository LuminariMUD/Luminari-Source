from __future__ import annotations

from dataclasses import replace
from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.flags import encode_bits
from wtool_lib.models import (
    ExitRecord,
    HlQuestCommandRecord,
    HlQuestEntryRecord,
    HlQuestRecord,
    MobileRecord,
    ObjectRecord,
    QuestRecord,
    ResetCommandRecord,
    RoomRecord,
    SourceSpan,
    TriggerRecord,
    ZoneRecord,
)
from wtool_lib.rooms import parse_room_file
from wtool_lib.semantics import _quest_cycles, strip_color_codes, validate_semantics
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


def quest_record(vnum: int, quest_type: int, **fields) -> QuestRecord:
  defaults = {
      "name": "Finished quest",
      "description": "A finished quest",
      "accept_message": "Accept.",
      "completion_message": "Complete.",
      "quit_message": "Quit.",
      "quest_type": quest_type,
      "questmaster_vnum": 200,
      "flag_token": "0",
      "target": None,
      "points": 0,
      "quit_penalty": 0,
      "min_level": 0,
      "max_level": 34,
      "time_limit": -1,
      "quantity": 1,
      "gold_reward": 0,
      "experience_reward": 0,
      "reward_row_width": 7,
      "raw_values": {"race_reward": -1},
  }
  defaults.update(fields)
  return QuestRecord(vnum, span(vnum), "1", **defaults)


def hlq_command(
    command_type: int,
    value: int,
    location: int,
    direction: str = "output",
    ordinal: int = 1,
) -> HlQuestCommandRecord:
  marker = "I" if direction == "input" else "O"
  return HlQuestCommandRecord(
      marker,
      direction,
      "?",
      "?",
      command_type,
      value,
      location,
      span(ordinal),
      ordinal,
  )


def hlq_entry(entry_type: int, marker: str, **fields) -> HlQuestEntryRecord:
  defaults = {
      "approval_suffix": "!",
      "approved": True,
      "span": span(entry_type + 1),
      "physical_ordinal": 1,
      "reply_message": "A finished reply.",
      "chain_terminated": marker != "A",
  }
  defaults.update(fields)
  return HlQuestEntryRecord(entry_type, marker, **defaults)


class SemanticTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.manifest = load_manifest()
    cls.spec_names = extract_spec_names(cls.repo_root)
    cls.num_spells = cls.manifest["limits"]["NUM_SPELLS"]["value"]
    cls.item_types = {
        entry["macro"]: entry["index"]
        for entry in cls.manifest["tables"]["item-types"]["entries"]
        if entry.get("macro")
    }
    cls.quest_types = {
        entry["macro"]: entry["index"]
        for entry in cls.manifest["tables"]["quest-types"]["entries"]
    }
    cls.hlq_entry_types = {
        entry["macro"]: entry["index"]
        for entry in cls.manifest["tables"]["hlquest-entry-types"]["entries"]
    }
    cls.hlq_command_types = {
        entry["macro"]: entry["index"]
        for entry in cls.manifest["tables"]["hlquest-commands"]["entries"]
    }

  def validate(
      self,
      zones: list[ZoneRecord],
      rooms: list[RoomRecord],
      mobiles: list[MobileRecord] | None = None,
      objects: list[ObjectRecord] | None = None,
      triggers: list[TriggerRecord] | None = None,
      quests: list[QuestRecord] | None = None,
      hlquests: list[HlQuestRecord] | None = None,
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
        quests,
        hlquests,
    )

  def quest_findings(self, quest: QuestRecord):
    return self.validate([], [], quests=[quest])

  def hlquest_findings(
      self,
      entry: HlQuestEntryRecord,
      rooms: list[RoomRecord] | None = None,
  ):
    record = HlQuestRecord(200, span(200), "1", entries=[entry])
    return self.validate([], rooms or [], hlquests=[record])

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
    self.assertNotIn("SEM004", {item.code for item in findings})
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
        {"SEM005", "SEM007", "SEM008", "SEM009", "SEM010", "SEM011"} <= codes
    )
    self.assertNotIn("SEM006", codes)
    unreachable = [item for item in findings if item.code == "SEM011"]
    self.assertTrue(all("roots [100]" in item.message for item in unreachable))

  def test_portal_destination_is_a_reachability_root(self) -> None:
    root = room(100, 1, exits=[exit_record(0, 101)])
    reachable = room(101, 1)
    portal_only = room(102, 1)
    portal = obj(
        300,
        self.item_types["ITEM_PORTAL"],
        [self.manifest["limits"]["PORTAL_NORMAL"]["value"], 102, 0, 0],
    )
    findings = self.validate(
        [zone(1, 100, 199)], [root, reachable, portal_only], objects=[portal]
    )
    self.assertFalse(any(item.code == "SEM011" and item.vnum == 102 for item in findings))

  def test_random_portal_range_covers_a_reachability_root(self) -> None:
    root = room(100, 1, exits=[exit_record(0, 101)])
    reachable = room(101, 1)
    in_range = room(102, 1)
    outside_range = room(150, 1)
    portal = obj(
        300,
        self.item_types["ITEM_PORTAL"],
        [self.manifest["limits"]["PORTAL_RANDOM"]["value"], 102, 110, 0],
    )
    findings = self.validate(
        [zone(1, 100, 199)],
        [root, reachable, in_range, outside_range],
        objects=[portal],
    )
    unreachable = {item.vnum for item in findings if item.code == "SEM011"}
    self.assertNotIn(102, unreachable)
    self.assertIn(150, unreachable)

  def test_script_destination_is_a_reachability_root(self) -> None:
    root = room(100, 1, exits=[exit_record(0, 101)])
    reachable = room(101, 1)
    teleport_target = room(102, 1, exits=[exit_record(0, 103)])
    onward = room(103, 1)
    door_target = room(104, 1)
    still_unreachable = room(105, 1)
    trigger = TriggerRecord(
        500,
        span(500),
        "1",
        name="teleport trigger",
        commands=(
            "* move the actor onward\n"
            "wait 1 sec\n"
            "mteleport %actor% 102\n"
            "%door% 100 north room 104\n"
            "%teleport% %actor% %destination%\n"
        ),
    )
    findings = self.validate(
        [zone(1, 100, 199)],
        [root, reachable, teleport_target, onward, door_target, still_unreachable],
        triggers=[trigger],
    )
    unreachable = {item.vnum for item in findings if item.code == "SEM011"}
    self.assertEqual({105}, unreachable)

  def test_closed_zone_suppresses_unreachable_rooms(self) -> None:
    closed_bit = next(
        entry["index"]
        for entry in self.manifest["tables"]["zone"]["entries"]
        if entry["macro"] == "ZONE_CLOSED"
    )
    root = room(100, 1, exits=[exit_record(0, 101)])
    reachable = room(101, 1)
    isolated = room(102, 1)
    locked = zone(1, 100, 199, flags=list(encode_bits({closed_bit})))
    findings = self.validate([locked], [root, reachable, isolated])
    codes = {item.code for item in findings}
    self.assertNotIn("SEM011", codes)
    self.assertNotIn("SEM010", codes)

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

  def test_room_level_range_bounds_and_order(self) -> None:
    valid = room(100, 1, minimum_level=15, maximum_level=-1)
    invalid_low = room(101, 1, minimum_level=0, maximum_level=20)
    invalid_high = room(102, 1, minimum_level=1, maximum_level=35)
    reversed_range = room(103, 1, minimum_level=20, maximum_level=10)
    findings = self.validate(
        [zone(1, 100, 199)], [valid, invalid_low, invalid_high, reversed_range]
    )
    level_findings = [item for item in findings if item.code == "SEM034"]
    self.assertEqual({101, 102, 103}, {item.vnum for item in level_findings})

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
    objects[0].activated_spells.append(([31, self.num_spells, 6, 5, -1], span(41)))
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

  def test_rol_object_trap_value_contract(self) -> None:
    trapped_bit = next(
        entry["index"]
        for entry in self.manifest["tables"]["obj-extra"]["entries"]
        if entry["macro"] == "ITEM_TRAPPED"
    )
    trap_flags = list(encode_bits({trapped_bit}))
    valid = obj(
        310,
        self.item_types["ITEM_CONTAINER"],
        [0] * 10 + [516, 30, -1, 40, 15, 15],
        extra_flags=trap_flags,
    )
    invalid = obj(
        311,
        self.item_types["ITEM_CONTAINER"],
        [0] * 10 + [8192, 99, -2, 101, 1, 0],
        extra_flags=trap_flags,
    )

    findings = self.validate(
        [zone(1, 100, 199)], [room(100, 1)], objects=[valid, invalid]
    )
    trap_findings = [item for item in findings if item.code == "SEM035"]

    self.assertEqual([311], [item.vnum for item in trap_findings])

  def test_qst_editor_scalar_and_string_boundaries(self) -> None:
    base = quest_record(100, self.quest_types["AQ_HOUSE_FIND"])
    scalar_cases = (
        ("quantity", 1, 50),
        ("points", 0, 999999),
        ("quit_penalty", 0, 999999),
        ("min_level", 0, 34),
        ("max_level", 0, 34),
        ("time_limit", -1, 100),
        ("gold_reward", 0, 99999),
        ("experience_reward", 0, 999999),
    )
    for field_name, minimum, maximum in scalar_cases:
      for value in (minimum, maximum):
        with self.subTest(field=field_name, valid=value):
          candidate = replace(base, **{field_name: value})
          if field_name == "min_level":
            candidate.max_level = maximum
          elif field_name == "max_level":
            candidate.min_level = minimum
          self.assertNotIn(
              "SEM025", {item.code for item in self.quest_findings(candidate)}
          )
      for value in (minimum - 1, maximum + 1):
        with self.subTest(field=field_name, invalid=value):
          candidate = replace(base, **{field_name: value})
          self.assertIn(
              "SEM025", {item.code for item in self.quest_findings(candidate)}
          )

    for field_name, limit in (
        ("name", 40),
        ("description", 75),
        ("accept_message", 4096),
        ("completion_message", 4096),
        ("quit_message", 4096),
    ):
      with self.subTest(field=field_name, boundary="maximum"):
        candidate = replace(base, **{field_name: "x" * (limit - 1)})
        self.assertNotIn("SEM024", {item.code for item in self.quest_findings(candidate)})
      with self.subTest(field=field_name, boundary="above"):
        candidate = replace(base, **{field_name: "x" * limit})
        finding = next(
            item for item in self.quest_findings(candidate) if item.code == "SEM024"
        )
        self.assertEqual("warning", finding.severity)

    inverted = replace(base, min_level=20, max_level=10)
    self.assertTrue(
        any("exceeds maximum" in item.message for item in self.quest_findings(inverted))
    )

  def test_qst_type_flags_and_type_specific_domains(self) -> None:
    base = quest_record(99, self.quest_types["AQ_HOUSE_FIND"])
    unavailable = quest_record(
        98, -1, questmaster_vnum=None, previous_quest_vnum=98
    )
    self.assertFalse(
        {item.code for item in self.quest_findings(unavailable)}
        & {"SEM023", "SEM024", "SEM025", "SEM026", "SEM027", "SEM028", "SEM029"}
    )
    invalid_type = quest_record(100, 25, flag_bits={2})
    type_findings = self.quest_findings(invalid_type)
    self.assertEqual(1, sum(item.code == "SEM023" for item in type_findings))

    missing_target = quest_record(101, self.quest_types["AQ_MOB_FIND"])
    missing_master = replace(missing_target, questmaster_vnum=None)
    messages = "\n".join(item.message for item in self.quest_findings(missing_master))
    self.assertIn("no questmaster", messages)
    self.assertIn("requires a typed target", messages)

    for value in (0, 5):
      mission = quest_record(102, self.quest_types["AQ_COMPLETE_MISSION"], target=value)
      self.assertNotIn("SEM026", {item.code for item in self.quest_findings(mission)})
    for value in (-1, 6):
      mission = quest_record(102, self.quest_types["AQ_COMPLETE_MISSION"], target=value)
      self.assertIn("SEM026", {item.code for item in self.quest_findings(mission)})

    wilderness = quest_record(103, self.quest_types["AQ_WILD_FIND"])
    self.assertIn("SEM026", {item.code for item in self.quest_findings(wilderness)})
    wilderness.wilderness_x = -999
    wilderness.wilderness_y = 999
    self.assertNotIn("SEM026", {item.code for item in self.quest_findings(wilderness)})

    for race in (-1, 45, 46):
      candidate = replace(base, raw_values={"race_reward": race})
      self.assertNotIn("SEM026", {item.code for item in self.quest_findings(candidate)})
    bad_race = replace(base, raw_values={"race_reward": 44})
    self.assertIn("SEM026", {item.code for item in self.quest_findings(bad_race)})

    give_gold = quest_record(
        104,
        self.quest_types["AQ_GIVE_GOLD"],
        target=-1,
        return_mobile_vnum=None,
    )
    self.assertGreaterEqual(
        sum(item.code == "SEM026" for item in self.quest_findings(give_gold)), 2
    )
    multi = quest_record(105, self.quest_types["AQ_MOB_MULTI_KILL"])
    self.assertTrue(
        any("cannot persist" in item.message for item in self.quest_findings(multi))
    )

  def test_qst_dialogue_domains_and_alternative_topology(self) -> None:
    base = quest_record(99, self.quest_types["AQ_HOUSE_FIND"])
    dialogue = quest_record(
        100,
        self.quest_types["AQ_DIALOGUE"],
        target=200,
        diplomacy_dc=1,
        intimidate_dc=-1,
        bluff_dc=-1,
    )
    self.assertNotIn("SEM027", {item.code for item in self.quest_findings(dialogue)})
    for value in (-1, 100):
      candidate = replace(dialogue, diplomacy_dc=value, intimidate_dc=1)
      self.assertNotIn("SEM027", {item.code for item in self.quest_findings(candidate)})
    for value in (-2, 101):
      candidate = replace(dialogue, diplomacy_dc=value, intimidate_dc=1)
      self.assertIn("SEM027", {item.code for item in self.quest_findings(candidate)})

    impossible = replace(dialogue, diplomacy_dc=-1, intimidate_dc=-1, bluff_dc=-1)
    self.assertTrue(
        any("cannot be completed" in item.message for item in self.quest_findings(impossible))
    )
    ignored = replace(base, diplomacy_dc=10)
    ignored_finding = next(
        item for item in self.quest_findings(ignored) if item.code == "SEM027"
    )
    self.assertEqual("warning", ignored_finding.severity)

    parent = replace(dialogue, dialogue_alternative_quest_vnum=101)
    bad_child = quest_record(101, self.quest_types["AQ_HOUSE_FIND"])
    findings = self.validate([], [], quests=[parent, bad_child])
    self.assertTrue(any("must name quest 100" in item.message for item in findings))
    good_child = replace(bad_child, previous_quest_vnum=100)
    findings = self.validate([], [], quests=[parent, good_child])
    self.assertFalse(any(item.code == "SEM027" for item in findings))

    poisoned = replace(base, dialogue_alternative_quest_vnum=101)
    self.assertTrue(
        any("unjoinable" in item.message for item in self.quest_findings(poisoned))
    )

  def test_qst_chain_self_links_reciprocity_and_cycles(self) -> None:
    previous_self = quest_record(
        100, self.quest_types["AQ_HOUSE_FIND"], previous_quest_vnum=100
    )
    finding = next(
        item for item in self.quest_findings(previous_self) if item.code == "SEM028"
    )
    self.assertEqual("error", finding.severity)
    next_self = replace(previous_self, previous_quest_vnum=None, next_quest_vnum=100)
    finding = next(item for item in self.quest_findings(next_self) if item.code == "SEM028")
    self.assertEqual("warning", finding.severity)

    first = quest_record(100, self.quest_types["AQ_HOUSE_FIND"], next_quest_vnum=101)
    second = quest_record(101, self.quest_types["AQ_HOUSE_FIND"])
    findings = self.validate([], [], quests=[first, second])
    self.assertTrue(any("not reciprocated" in item.message for item in findings))

    previous_cycle = [
        replace(first, next_quest_vnum=None, previous_quest_vnum=101),
        replace(second, previous_quest_vnum=100),
    ]
    cycle = next(
        item
        for item in self.validate([], [], quests=previous_cycle)
        if item.code == "SEM029"
    )
    self.assertEqual("error", cycle.severity)
    self.assertIn("100 -> 101 -> 100", cycle.message)

    next_cycle = [
        replace(first, next_quest_vnum=101),
        replace(second, next_quest_vnum=100),
    ]
    cycle = next(
        item
        for item in self.validate([], [], quests=next_cycle)
        if item.code == "SEM029"
    )
    self.assertEqual("warning", cycle.severity)

  def test_quest_cycle_scan_has_linear_map_probe_budget(self) -> None:
    class CountingQuestMap(dict[int, QuestRecord]):
      def __init__(self, *args):
        super().__init__(*args)
        self.probes = 0

      def __contains__(self, key):
        self.probes += 1
        return super().__contains__(key)

      def __getitem__(self, key):
        self.probes += 1
        return super().__getitem__(key)

    count = 10000
    records = CountingQuestMap(
        (
            vnum,
            QuestRecord(
                vnum,
                span(vnum),
                "1",
                next_quest_vnum=(vnum + 1 if vnum < count else None),
            ),
        )
        for vnum in range(1, count + 1)
    )
    self.assertEqual([], _quest_cycles(records, "next_quest_vnum"))
    self.assertLessEqual(records.probes, count * 3)

  def test_hlq_entry_shape_and_command_legality(self) -> None:
    ask_type = self.hlq_entry_types["QUEST_ASK"]
    give_type = self.hlq_entry_types["QUEST_GIVE"]
    room_type = self.hlq_entry_types["QUEST_ROOM"]
    coin_type = self.hlq_command_types["QUEST_COMMAND_COINS"]
    item_type = self.hlq_command_types["QUEST_COMMAND_ITEM"]
    load_type = self.hlq_command_types["QUEST_COMMAND_LOAD_OBJECT_INROOM"]

    ask = hlq_entry(ask_type, "A", keywords="topic", chain_terminated=False)
    self.assertFalse(
        {item.code for item in self.hlquest_findings(ask)} & {"SEM030", "SEM031"}
    )
    broken_ask = replace(ask, keywords="", reply_message="")
    self.assertEqual(
        2, sum(item.code == "SEM030" for item in self.hlquest_findings(broken_ask))
    )
    give = hlq_entry(
        give_type,
        "Q",
        commands=[hlq_command(coin_type, 10, 0, "input")],
    )
    self.assertNotIn("SEM031", {item.code for item in self.hlquest_findings(give)})
    item_input = replace(
        give, commands=[hlq_command(item_type, 300, 0, "input")]
    )
    self.assertNotIn("SEM031", {item.code for item in self.hlquest_findings(item_input)})
    ignored_input = replace(
        give, commands=[hlq_command(load_type, 300, 100, "input")]
    )
    self.assertIn("SEM031", {item.code for item in self.hlquest_findings(ignored_input)})

    room_entry = hlq_entry(
        room_type,
        "R",
        room_vnum=100,
        commands=[hlq_command(coin_type, 10, 0, "input")],
    )
    self.assertIn("SEM031", {item.code for item in self.hlquest_findings(room_entry)})
    self.assertIn(
        "SEM030",
        {item.code for item in self.hlquest_findings(replace(room_entry, room_vnum=None))},
    )
    self.assertIn(
        "SEM030",
        {item.code for item in self.hlquest_findings(replace(give, chain_terminated=False))},
    )

  def test_hlq_runtime_safe_command_boundaries_and_parameters(self) -> None:
    give_type = self.hlq_entry_types["QUEST_GIVE"]
    command_types = self.hlq_command_types

    def findings(command: HlQuestCommandRecord):
      entry = hlq_entry(give_type, "Q", commands=[command])
      return self.hlquest_findings(entry)

    coin = command_types["QUEST_COMMAND_COINS"]
    for value in (0, 100000, 2140000000):
      self.assertNotIn("SEM032", {item.code for item in findings(hlq_command(coin, value, 0))})
    self.assertIn("SEM032", {item.code for item in findings(hlq_command(coin, -1, 0))})
    high_coin = next(
        item for item in findings(hlq_command(coin, 2140000001, 0)) if item.code == "SEM032"
    )
    self.assertEqual("error", high_coin.severity)

    quest_points = command_types["QUEST_COMMAND_QUEST_POINTS"]
    for value in (-100000000, -1, 0, 1, 100000000):
      self.assertNotIn(
          "SEM032", {item.code for item in findings(hlq_command(quest_points, value, 0))}
      )
    for value in (-100000001, 100000001):
      self.assertIn(
          "SEM032", {item.code for item in findings(hlq_command(quest_points, value, 0))}
      )

    experience = command_types["QUEST_COMMAND_EXPERIENCE"]
    for value in (0, 1000, 2140000000):
      self.assertNotIn(
          "SEM032", {item.code for item in findings(hlq_command(experience, value, 0))}
      )
    for value in (-1, 2140000001):
      self.assertIn(
          "SEM032", {item.code for item in findings(hlq_command(experience, value, 0))}
      )

    for macro in ("QUEST_COMMAND_TEACH_SPELL", "QUEST_COMMAND_CAST_SPELL"):
      command_type = command_types[macro]
      for value in (1, self.num_spells - 1):
        self.assertNotIn(
            "SEM032", {item.code for item in findings(hlq_command(command_type, value, 0))}
        )
      for value in (0, self.num_spells):
        self.assertIn(
            "SEM032", {item.code for item in findings(hlq_command(command_type, value, 0))}
        )

    open_door = command_types["QUEST_COMMAND_OPEN_DOOR"]
    for value in (0, 5):
      self.assertNotIn(
          "SEM032", {item.code for item in findings(hlq_command(open_door, value, 100))}
      )
    for value in (-1, 6):
      self.assertIn(
          "SEM032", {item.code for item in findings(hlq_command(open_door, value, 100))}
      )

    kit = command_types["QUEST_COMMAND_KIT"]
    for value, location in ((0, 37), (37, 0), (9999, -999), (-999, 9999)):
      self.assertNotIn(
          "SEM032", {item.code for item in findings(hlq_command(kit, value, location))}
      )
    for value, location in ((-1, 0), (38, 0), (0, -1), (0, 38)):
      self.assertIn(
          "SEM032", {item.code for item in findings(hlq_command(kit, value, location))}
      )

    church = command_types["QUEST_COMMAND_CHURCH"]
    for value in (0, 12):
      self.assertNotIn(
          "SEM032", {item.code for item in findings(hlq_command(church, value, 0))}
      )
    for value in (-1, 13):
      self.assertIn(
          "SEM032", {item.code for item in findings(hlq_command(church, value, 0))}
      )

    attack = command_types["QUEST_COMMAND_ATTACK_QUESTOR"]
    self.assertIn("SEM033", {item.code for item in findings(hlq_command(attack, 1, 2))})
    self.assertIn("SEM033", {item.code for item in findings(hlq_command(coin, 1, 2))})

    room_with_no_exit = room(100, 1)
    entry = hlq_entry(give_type, "Q", commands=[hlq_command(open_door, 0, 100)])
    self.assertTrue(
        any(
            item.code == "SEM032" and "no exit exists" in item.message
            for item in self.hlquest_findings(entry, [room_with_no_exit])
        )
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
