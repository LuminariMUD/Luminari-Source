#!/usr/bin/env python3
"""Tests for the canonical help catalog and merge engine."""

from __future__ import annotations

from pathlib import Path
import sys
import unittest


MODULE_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(MODULE_ROOT))

from catalog import (  # noqa: E402
    Catalog,
    HelpEntry,
    HelpSyncError,
    RelatedTopic,
    Rename,
    build_plan_core,
    merge_catalogs,
    parse_help_hlp,
    render_help_hlp,
    render_legacy_entries,
    seal_plan,
    validate_help_hlp,
    validate_plan,
)


def entry(tag: str, body: str = "body", **changes: object) -> HelpEntry:
    values = {
        "tag": tag,
        "body": body,
        "keywords": (tag,),
        "aliases": (),
        "min_level": 0,
        "max_level": 1000,
        "category": "general",
        "auto_generated": False,
        "related_topics": (),
    }
    values.update(changes)
    return HelpEntry(**values)


class CatalogTests(unittest.TestCase):
    def test_catalog_hash_is_independent_of_row_order(self) -> None:
        first = Catalog((entry("beta"), entry("alpha")))
        second = Catalog((entry("alpha"), entry("beta")))
        self.assertEqual(first.content_hash, second.content_hash)
        self.assertEqual(first.to_dict(), second.to_dict())

    def test_catalog_round_trip_preserves_every_semantic_field(self) -> None:
        original = Catalog(
            (
                entry(
                    "alpha",
                    "first\r\nsecond",
                    keywords=("alpha", "Alpha Topic"),
                    aliases=("old-alpha",),
                    min_level=3,
                    max_level=42,
                    category="Spells",
                    auto_generated=True,
                    related_topics=(RelatedTopic("beta", "0.75"),),
                ),
                entry("beta"),
            )
        )
        restored = Catalog.from_dict(original.to_dict())
        self.assertEqual(original, restored)
        self.assertEqual(original.content_hash, restored.content_hash)

    def test_help_hlp_render_and_parse_are_deterministic(self) -> None:
        catalog = Catalog(
            (
                entry("two words", "Line one\r\nLine two", keywords=("phrase alias",)),
                entry("alpha", "Other"),
            )
        )
        rendered = render_help_hlp(catalog)
        parsed = parse_help_hlp(rendered)
        self.assertEqual(rendered, render_legacy_entries(parsed))
        validate_help_hlp(rendered, catalog)
        self.assertTrue(rendered.endswith(b"$~\n"))

    def test_help_hlp_rejects_body_level_marker(self) -> None:
        catalog = Catalog((entry("alpha", "safe\n#not-safe\n"),))
        with self.assertRaises(HelpSyncError):
            render_help_hlp(catalog)

    def test_development_and_production_only_additions_merge(self) -> None:
        base = Catalog.empty()
        development = Catalog((entry("development"),))
        production = Catalog((entry("production"),))
        result = merge_catalogs(base, development, production)
        self.assertTrue(result.sealed)
        self.assertEqual(set(result.candidate.by_tag), {"development", "production"})

    def test_identical_concurrent_change_is_accepted_once(self) -> None:
        base = Catalog((entry("alpha", "old"),))
        changed = Catalog((entry("alpha", "new"),))
        result = merge_catalogs(base, changed, changed)
        self.assertTrue(result.sealed)
        self.assertEqual(result.candidate.by_tag["alpha"].body, "new\n")

    def test_independent_fields_and_keyword_additions_merge(self) -> None:
        base_entry = entry("alpha", "old", keywords=("alpha",))
        base = Catalog((base_entry,))
        development = Catalog(
            (entry("alpha", "new", keywords=("alpha", "development")),)
        )
        production = Catalog(
            (
                entry(
                    "alpha",
                    "old",
                    keywords=("alpha", "production"),
                    category="commands",
                ),
            )
        )
        result = merge_catalogs(base, development, production)
        merged = result.candidate.by_tag["alpha"]
        self.assertTrue(result.sealed)
        self.assertEqual(merged.body, "new\n")
        self.assertEqual(merged.category, "commands")
        self.assertEqual(merged.keywords, ("ALPHA", "DEVELOPMENT", "PRODUCTION"))

    def test_same_field_change_is_a_conflict(self) -> None:
        base = Catalog((entry("alpha", "old"),))
        development = Catalog((entry("alpha", "development"),))
        production = Catalog((entry("alpha", "production"),))
        result = merge_catalogs(base, development, production)
        self.assertFalse(result.sealed)
        self.assertEqual(result.conflicts[0]["reason"], "field_conflict")
        self.assertEqual(result.conflicts[0]["fields"][0]["field"], "body")

    def test_delete_requires_tombstone_and_conflicts_with_edit(self) -> None:
        base = Catalog((entry("alpha", "old"),))
        development = Catalog.empty()
        production = Catalog((entry("alpha", "new"),))
        missing = merge_catalogs(base, development, production)
        self.assertEqual(missing.conflicts[0]["reason"], "missing_without_tombstone")
        explicit = merge_catalogs(
            base,
            development,
            production,
            tombstones={"development": ("alpha",)},
        )
        self.assertEqual(explicit.conflicts[0]["reason"], "delete_vs_edit")

    def test_unchanged_other_side_allows_explicit_deletion(self) -> None:
        base = Catalog((entry("alpha"),))
        result = merge_catalogs(
            base,
            Catalog.empty(),
            base,
            tombstones={"development": ("alpha",)},
        )
        self.assertTrue(result.sealed)
        self.assertEqual(result.candidate, Catalog.empty())
        self.assertEqual(result.authorized_deletions, ("alpha",))

    def test_explicit_rename_can_merge_an_edit_from_the_other_side(self) -> None:
        base = Catalog((entry("old", "base"),))
        development = Catalog((entry("new", "base"),))
        production = Catalog((entry("old", "production edit"),))
        rename = Rename("development", "old", "new")
        result = merge_catalogs(base, development, production, renames=(rename,))
        self.assertTrue(result.sealed)
        self.assertEqual(set(result.candidate.by_tag), {"new"})
        self.assertEqual(result.candidate.by_tag["new"].body, "production edit\n")
        self.assertIn("old", result.authorized_deletions)

    def test_plan_is_content_addressed_and_detects_tampering(self) -> None:
        base = Catalog((entry("alpha"),))
        result = merge_catalogs(base, base, base)
        plan = seal_plan(build_plan_core(base, base, base, result))
        validate_plan(plan)
        plan["candidate_hash"] = "0" * 64
        with self.assertRaises(HelpSyncError):
            validate_plan(plan)


if __name__ == "__main__":
    unittest.main()
