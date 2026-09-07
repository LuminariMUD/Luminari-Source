#!/usr/bin/env python3
"""Unit tests for the event-core performance gate's fixture accounting."""

import importlib.util
import pathlib
import unittest


SCRIPT = pathlib.Path(__file__).with_name("run_event_core_performance_gate.py")
SPEC = importlib.util.spec_from_file_location("event_core_performance_gate", SCRIPT)
assert SPEC and SPEC.loader
GATE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GATE)


def fixture_snapshots(cleanup_live: tuple[int, int, int] = (21, 19, 24)) -> dict[str, dict[str, int]]:
    snapshots: dict[str, dict[str, int]] = {}
    for index, cleanup in enumerate(cleanup_live, start=1):
        baseline = 20
        snapshots[f"dg-{index}-baseline"] = {GATE.DG_WAIT_IDENTITY: baseline}
        snapshots[f"dg-{index}"] = {GATE.DG_WAIT_IDENTITY: baseline + 100}
        snapshots[f"dg-{index}-cleanup"] = {GATE.DG_WAIT_IDENTITY: cleanup}
    return snapshots


class DgFixtureAnalysisTests(unittest.TestCase):
    def test_background_world_churn_does_not_fail_clean_fixture_owners(self) -> None:
        result = GATE.dg_fixture_analysis(fixture_snapshots())

        self.assertTrue(result["passed"])
        self.assertEqual(result["missing_fields"], [])
        self.assertEqual([segment["active_delta"] for segment in result["segments"]], [100, 100, 100])

    def test_retained_fixture_waits_fail_cleanup(self) -> None:
        result = GATE.dg_fixture_analysis(fixture_snapshots((21, 35, 24)))

        self.assertFalse(result["passed"])
        self.assertEqual(result["segments"][1]["cleanup_delta"], 15)

    def test_missing_phase_fails_closed(self) -> None:
        snapshots = fixture_snapshots()
        del snapshots["dg-3-cleanup"]

        result = GATE.dg_fixture_analysis(snapshots)

        self.assertFalse(result["passed"])
        self.assertEqual(result["missing_fields"], ["dg-3-cleanup:dg.trigger.wait"])


class ReadyQueueAnalysisTests(unittest.TestCase):
    def test_transient_current_tick_work_is_allowed_when_steady_state_drains(self) -> None:
        diagnostics = {
            "dg-1-cleanup": {"ready": 0},
            "dg-2-cleanup": {"ready": 0},
            "dg-3-cleanup": {"ready": 1295},
            "steady": {"ready": 0},
        }

        result = GATE.ready_queue_analysis(diagnostics)

        self.assertTrue(result["passed"])
        self.assertTrue(result["drained_at_steady"])
        self.assertEqual(1295, result["maximum_transient"])

    def test_retained_ready_work_fails_the_final_snapshot(self) -> None:
        diagnostics = {
            "dg-1-cleanup": {"ready": 0},
            "steady": {"ready": 3},
        }

        result = GATE.ready_queue_analysis(diagnostics)

        self.assertFalse(result["passed"])
        self.assertFalse(result["drained_at_steady"])
        self.assertEqual(3, result["final_ready"])


if __name__ == "__main__":
    unittest.main()
