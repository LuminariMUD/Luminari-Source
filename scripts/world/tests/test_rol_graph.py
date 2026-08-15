from __future__ import annotations

from types import SimpleNamespace
import unittest

from wtool_lib.models import SourceSpan
from wtool_lib.rol_graph import audit_connection_graph
from wtool_lib.rol_source import RolRecord


def source_room(vnum: int, destination: int) -> RolRecord:
  return RolRecord(
      kind="wld",
      vnum=vnum,
      basename="graph",
      path="areas/wld/graph.wld",
      line=vnum,
      end_line=vnum + 1,
      sha256="0" * 64,
      directives=[
          {
              "token": "D",
              "line": vnum,
              "direction": 0,
              "arguments": [0, -1, destination],
          }
      ],
  )


def action(vnum: int) -> dict[str, object]:
  return {
      "source_kind": "wld",
      "source_vnum": vnum,
      "source_record_id": f"wld:{vnum}:areas/wld/graph.wld:{vnum}",
      "destination_vnum": 2_000_000 + vnum,
      "action": "ADD",
  }


def target_room(vnum: int, destinations: list[int]) -> SimpleNamespace:
  span = SourceSpan("wld/20000.wld", vnum)
  exits = [
      SimpleNamespace(
          direction=index,
          destination_vnum=destination,
          span=span,
      )
      for index, destination in enumerate(destinations)
  ]
  return SimpleNamespace(vnum=vnum, exits=exits, span=span)


class RolGraphTests(unittest.TestCase):
  def test_exact_isolated_graph_passes_and_records_unresolved_source_exit(self) -> None:
    records = [source_room(100, 101), source_room(101, 999)]
    actions = [action(100), action(101)]
    rooms = [target_room(2_000_100, [2_000_101]), target_room(2_000_101, [])]

    audit = audit_connection_graph(records, actions, rooms)

    self.assertTrue(audit["summary"]["pass"])
    self.assertEqual(1, audit["summary"]["expected_resolvable_directed_exits"])
    self.assertEqual(1, audit["summary"]["unresolved_source_exits"])

  def test_missing_extra_and_cross_world_edges_fail(self) -> None:
    records = [source_room(100, 101), source_room(101, -1)]
    actions = [action(100), action(101)]
    rooms = [
        target_room(50, [2_000_100]),
        target_room(2_000_100, [42]),
        target_room(2_000_101, []),
    ]

    audit = audit_connection_graph(records, actions, rooms)

    self.assertFalse(audit["summary"]["pass"])
    self.assertEqual(1, audit["summary"]["missing_expected_exits"])
    self.assertEqual(1, audit["summary"]["extra_target_exits"])
    self.assertEqual(2, audit["summary"]["cross_world_exits"])

  def test_typed_room_reference_cross_world_edge_fails(self) -> None:
    records = [source_room(100, -1)]
    actions = [
        action(100),
        {
            "source_kind": "obj",
            "source_vnum": 200,
            "source_record_id": "obj:200:areas/obj/graph.obj:200",
            "destination_vnum": 2_000_200,
            "action": "ADD",
        },
    ]
    rooms = [target_room(2_000_100, [])]
    reference = SimpleNamespace(
        target_type="room",
        target_vnum=42,
        role="portal destination",
        span=SourceSpan("obj/20002.obj", 1),
    )
    objects = [SimpleNamespace(vnum=2_000_200, references=[reference])]

    audit = audit_connection_graph(
        records, actions, rooms, (("obj", objects),)
    )

    self.assertFalse(audit["summary"]["pass"])
    self.assertEqual(1, audit["summary"]["cross_world_typed_room_references"])


if __name__ == "__main__":
  unittest.main()
