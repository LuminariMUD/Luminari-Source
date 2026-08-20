from __future__ import annotations

import os
from pathlib import Path
import stat
import tempfile
import textwrap
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_mob_calculator import (
    MobileCalculatorClient,
    MobileCalculatorError,
)


class RolMobileCalculatorTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()
    cls.executable = Path(
        os.environ.get("ROL_MOB_CALCULATOR", cls.root / "util/rol_mob_calculator")
    )
    if not cls.executable.is_file():
      raise unittest.SkipTest("build util/rol_mob_calculator before running calculator tests")

  def _helper(self, body: str) -> Path:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "helper"
    path.write_text(textwrap.dedent(body).lstrip(), encoding="ascii", newline="\n")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)
    return path

  def test_reuses_one_process_and_returns_deterministic_native_results(self) -> None:
    with MobileCalculatorClient(self.root, self.executable) as client:
      first = client.calculate(100, 34, 4, 3, 0)
      second = client.calculate(101, 34, 4, 3, 0)
      self.assertEqual(first.persisted, second.persisted)
      self.assertEqual(2, client.requests)
      self.assertEqual(1, first.profile_version)
      self.assertEqual(0, first.category)
      self.assertEqual((34, 4, 3, 0), (first.level, first.race, first.ch_class, first.tier))
      self.assertEqual(0, first.custom_profile)

  def test_named_custom_profile_is_calculated_in_native_code(self) -> None:
    with MobileCalculatorClient(self.root, self.executable) as client:
      living = client.calculate(19700, 34, 11, 3, 5, 1)
      dracolich = client.calculate(19701, 34, 3, 3, 5, 2)
      self.assertEqual(29999, living.persisted.hit_points)
      self.assertEqual(30000, dracolich.persisted.hit_points)
      self.assertEqual(1, living.custom_profile)
      self.assertEqual(2, dracolich.custom_profile)

  def test_rejects_stale_binary_hash(self) -> None:
    with self.assertRaisesRegex(MobileCalculatorError, "does not match required"):
      MobileCalculatorClient(self.root, self.executable, expected_sha256="0" * 64)

  def test_rejects_version_mismatch(self) -> None:
    helper = self._helper(
        """
        #!/bin/sh
        echo 'rol_mob_calculator protocol=2 profile=1 config=1'
        """
    )
    with self.assertRaisesRegex(MobileCalculatorError, "stale or malformed"):
      MobileCalculatorClient(self.root, helper)

  def test_rejects_malformed_response_without_fallback(self) -> None:
    helper = self._helper(
        """
        #!/bin/sh
        if [ "$1" = "--version" ]; then
          echo 'rol_mob_calculator protocol=1 profile=1 config=1'
          exit 0
        fi
        read header
        echo 'ROL_MOB_CALCULATOR 1'
        read request
        echo 'BROKEN'
        """
    )
    client = MobileCalculatorClient(self.root, helper)
    self.addCleanup(client._terminate)
    with self.assertRaisesRegex(MobileCalculatorError, "malformed calculator response"):
      client.calculate(1, 1, 1, 1, 0)


if __name__ == "__main__":
  unittest.main()
