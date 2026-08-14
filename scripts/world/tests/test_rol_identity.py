from __future__ import annotations

import copy
import unittest

from wtool_lib.rol_identity import (
    RolIdentityError,
    canonical_destination,
    canonical_reference_vnum,
)


class RolIdentityTests(unittest.TestCase):
  def setUp(self) -> None:
    self.policy = {
        "identity": {
            "canonical_formula": {"zone_offset": 20000, "entity_offset": 2000000},
            "new_zone_range": {"start": 20000, "end": 29999, "offset": 20000},
            "new_entity_range": {
                "start": 2000000,
                "end": 2999999,
                "offset": 2000000,
            },
            "normalizations": [
                {
                    "source_basename": "mytheast",
                    "source_header_vnum": 81700,
                    "source_top_vnum": 81899,
                    "logical_source_zone": 817,
                    "logical_source_zones": [817, 818],
                    "target_zone_vnum": 20817,
                    "evidence": ["manifest membership", "top range", "contained records"],
                }
            ],
        }
    }

  def test_low_typical_and_maximum_formula_values(self) -> None:
    self.assertEqual(20000, canonical_destination("zon", 0, "low", self.policy))
    self.assertEqual(20591, canonical_destination("zon", 591, "hulburg", self.policy))
    self.assertEqual(29999, canonical_destination("zon", 9999, "max", self.policy))
    self.assertEqual(2000000, canonical_destination("wld", 0, "low", self.policy))
    self.assertEqual(2059433, canonical_destination("wld", 59433, "hulburg", self.policy))
    self.assertEqual(2999999, canonical_destination("obj", 999999, "max", self.policy))

  def test_multi_band_package_preserves_sparse_entity_formula(self) -> None:
    self.assertEqual(2059100, canonical_destination("mob", 59100, "hulburg", self.policy))
    self.assertEqual(2059599, canonical_destination("obj", 59599, "hulburg", self.policy))

  def test_range_overflow_is_rejected_without_fallback_allocation(self) -> None:
    with self.assertRaisesRegex(RolIdentityError, "outside"):
      canonical_destination("zon", 10000, "overflow", self.policy)
    with self.assertRaisesRegex(RolIdentityError, "outside"):
      canonical_destination("obj", 1000000, "overflow", self.policy)

  def test_mytheast_requires_exact_evidence_backed_normalization(self) -> None:
    self.assertEqual(
        20817, canonical_destination("zon", 81700, "mytheast", self.policy)
    )
    self.assertEqual(
        2081700, canonical_destination("wld", 81700, "mytheast", self.policy)
    )
    self.assertEqual(
        2081899, canonical_destination("mob", 81899, "mytheast", self.policy)
    )
    with self.assertRaisesRegex(RolIdentityError, "expects header"):
      canonical_destination("zon", 817, "mytheast", self.policy)

  def test_ambiguous_or_malformed_normalization_is_rejected(self) -> None:
    ambiguous = copy.deepcopy(self.policy)
    ambiguous["identity"]["normalizations"].append(
        copy.deepcopy(ambiguous["identity"]["normalizations"][0])
    )
    with self.assertRaisesRegex(RolIdentityError, "multiple normalizations"):
      canonical_destination("zon", 81700, "mytheast", ambiguous)

    malformed = copy.deepcopy(self.policy)
    malformed["identity"]["normalizations"][0]["evidence"] = ["only one"]
    with self.assertRaisesRegex(RolIdentityError, "requires manifest"):
      canonical_destination("zon", 81700, "mytheast", malformed)

  def test_historic_lineage_cannot_override_formula(self) -> None:
    self.policy["identity"]["historic_target_lineage"] = [
        {
            "source_kind": "obj",
            "source_vnum": 1007,
            "historic_target_vnum": 169906,
        }
    ]
    self.assertEqual(
        2001007, canonical_destination("obj", 1007, "quests", self.policy)
    )
    self.assertEqual(2001007, canonical_reference_vnum("object", 1007))

  def test_bad_offsets_and_unsupported_types_are_rejected(self) -> None:
    bad = copy.deepcopy(self.policy)
    bad["identity"]["canonical_formula"]["zone_offset"] = 1000
    with self.assertRaisesRegex(RolIdentityError, "must be zone"):
      canonical_destination("zon", 507, "trail", bad)
    with self.assertRaisesRegex(RolIdentityError, "unsupported"):
      canonical_reference_vnum("command", 1)


if __name__ == "__main__":
  unittest.main()
