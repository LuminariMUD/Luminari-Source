from pathlib import Path
import tempfile
import threading
import time
import unittest
from unittest import mock

import sys

HELP_SYNC_DIRECTORY = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HELP_SYNC_DIRECTORY))

from catalog import Catalog, HelpEntry, MergeResult  # noqa: E402
from endpoint import (  # noqa: E402
    EndpointError,
    HelpWriteBarrier,
    atomic_write,
    normalize_integrity_repairs,
    repair_missing_keywords,
    request_runtime_reload,
)
from help_sync import make_plan, prepare_merge_catalogs  # noqa: E402


def entry(tag="alpha", body="Alpha help.\n", keywords=("ALPHA",)):
    return HelpEntry(tag=tag, body=body, keywords=keywords)


class EndpointUnitTests(unittest.TestCase):
    def test_integrity_repairs_are_canonical_and_counted(self):
        repairs = normalize_integrity_repairs(
            {
                "orphan_keywords": [
                    {"help_tag": "ghost", "keyword": "OLD", "count": 1},
                    {"help_tag": "ghost", "keyword": "OLD", "count": 2},
                ],
                "missing_keyword_tags": ["Beta", " beta "],
            }
        )
        self.assertEqual(
            repairs,
            {
                "orphan_keywords": [
                    {"help_tag": "ghost", "keyword": "OLD", "count": 3}
                ],
                "missing_keyword_tags": ["beta"],
            },
        )

    def test_explicit_integrity_repair_adds_lookup_keyword(self):
        repaired = repair_missing_keywords(Catalog((entry(keywords=()),)))
        self.assertEqual(repaired.entries[0].keywords, ("ALPHA",))

    def test_missing_keywords_preserve_baseline_and_peer_values_before_merge(self):
        base = Catalog((entry(keywords=("ALPHA",)),))
        development = Catalog((entry(keywords=()),))
        production = Catalog((entry(keywords=("ALPHA", "NEW")),))
        _, repaired_development, repaired_production = prepare_merge_catalogs(
            base, development, production, True
        )
        self.assertEqual(repaired_development.entries[0].keywords, ("ALPHA", "NEW"))
        self.assertEqual(repaired_production, production)

    def test_barrier_is_exclusive_and_owner_checked(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "lib" / "text" / "help").mkdir(parents=True)
            owner = "a" * 64
            first = HelpWriteBarrier(root, owner)
            second = HelpWriteBarrier(root, "b" * 64)
            first.acquire()
            with self.assertRaises(EndpointError):
                second.acquire()
            first.release()
            self.assertFalse(first.path.exists())

    def test_atomic_write_refuses_symbolic_link(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target = root / "target"
            link = root / "link"
            target.write_bytes(b"old")
            link.symlink_to(target)
            with self.assertRaises(EndpointError):
                atomic_write(link, b"new")
            self.assertEqual(target.read_bytes(), b"old")

    def test_runtime_reload_requires_matching_acknowledgment(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            help_directory = root / "lib" / "text" / "help"
            help_directory.mkdir(parents=True)
            token = "c" * 64

            def acknowledge():
                request = help_directory / ".help_sync.reload.request"
                deadline = time.monotonic() + 2
                while time.monotonic() < deadline and not request.exists():
                    time.sleep(0.01)
                (help_directory / ".help_sync.reload.ack").write_text(
                    f"{token} ok\n", encoding="ascii"
                )

            worker = threading.Thread(target=acknowledge)
            worker.start()
            with mock.patch("endpoint._process_is_running", return_value=True):
                result = request_runtime_reload(root, token, 2)
            worker.join(timeout=2)
            self.assertEqual(result, "runtime cache and fallback table reloaded")
            self.assertFalse((help_directory / ".help_sync.reload.ack").exists())
            self.assertFalse((help_directory / ".help_sync.reload.request").exists())

    def test_timeout_removes_pending_reload_request(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            help_directory = root / "lib" / "text" / "help"
            help_directory.mkdir(parents=True)
            with mock.patch("endpoint._process_is_running", return_value=True):
                with self.assertRaises(EndpointError):
                    request_runtime_reload(root, "d" * 64, 0.01)
            self.assertFalse((help_directory / ".help_sync.reload.request").exists())

    def test_plan_is_blocked_until_integrity_repair_is_explicit(self):
        catalog = Catalog((entry(keywords=()),))
        result = MergeResult(catalog, (), (), ())
        state = {
            "schema_write_ready": True,
            "integrity_issues": ["help entry 'alpha' has no help_keywords rows"],
            "integrity_repairs": {
                "orphan_keywords": [],
                "missing_keyword_tags": ["alpha"],
            },
            "file_matches": True,
        }
        blocked = make_plan(
            catalog,
            catalog,
            catalog,
            result,
            {"development": [], "production": []},
            [],
            state,
            state,
            repair_integrity=False,
        )
        self.assertFalse(blocked["sealed"])
        repaired = make_plan(
            catalog,
            catalog,
            catalog,
            result,
            {"development": [], "production": []},
            [],
            state,
            state,
            repair_integrity=True,
        )
        self.assertTrue(repaired["sealed"])
        self.assertEqual(
            Catalog.from_dict(repaired["candidate"]).entries[0].keywords, ("ALPHA",)
        )

    def test_plan_refuses_integrity_issue_without_supported_repair(self):
        catalog = Catalog((entry(),))
        result = MergeResult(catalog, (), (), ())
        state = {
            "schema_write_ready": True,
            "integrity_issues": ["orphan related topic 'missing' -> 'alpha'"],
            "integrity_repairs": {
                "orphan_keywords": [],
                "missing_keyword_tags": [],
            },
            "file_matches": True,
        }
        plan = make_plan(
            catalog,
            catalog,
            catalog,
            result,
            {"development": [], "production": []},
            [],
            state,
            state,
            repair_integrity=True,
        )
        self.assertFalse(plan["sealed"])
        self.assertIn(
            "development has integrity issues without a supported explicit repair",
            plan["integrity_blockers"],
        )

    def test_layer_drift_requires_explicit_projection_repair(self):
        catalog = Catalog((entry(),))
        result = MergeResult(catalog, (), (), ())
        state = {
            "schema_write_ready": True,
            "integrity_issues": [],
            "integrity_repairs": {
                "orphan_keywords": [],
                "missing_keyword_tags": [],
            },
            "file_matches": False,
        }
        blocked = make_plan(
            catalog,
            catalog,
            catalog,
            result,
            {"development": [], "production": []},
            [],
            state,
            state,
            repair_integrity=False,
        )
        self.assertFalse(blocked["sealed"])
        authorized = make_plan(
            catalog,
            catalog,
            catalog,
            result,
            {"development": [], "production": []},
            [],
            state,
            state,
            repair_integrity=False,
            repair_layers=True,
        )
        self.assertTrue(authorized["sealed"])


if __name__ == "__main__":
    unittest.main()
