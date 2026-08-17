"""Strict persistent bridge to the authoritative C mobile-stat calculator."""

from __future__ import annotations

import atexit
from dataclasses import asdict, dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from typing import Any

from .constants import default_repo_root


PROTOCOL_VERSION = 1
PROFILE_VERSION = 1
CONFIG_VERSION = 1
_VERSION = re.compile(
    r"^rol_mob_calculator protocol=(\d+) profile=(\d+) config=(\d+)$"
)
_STAT_FIELDS = (
    "hit_points",
    "hitroll",
    "armor_class",
    "damage_dice_count",
    "damage_dice_size",
    "damage_bonus",
    "experience",
    "gold",
    "strength",
    "strength_add",
    "intelligence",
    "wisdom",
    "dexterity",
    "constitution",
    "charisma",
    "saving_fortitude",
    "saving_reflex",
    "saving_will",
    "saving_poison",
    "saving_death",
    "spell_resistance",
)


class MobileCalculatorError(RuntimeError):
  """Raised when the C helper is unavailable, stale, or violates its protocol."""


@dataclass(frozen=True, slots=True)
class MobileStats:
  hit_points: int
  hitroll: int
  armor_class: int
  damage_dice_count: int
  damage_dice_size: int
  damage_bonus: int
  experience: int
  gold: int
  strength: int
  strength_add: int
  intelligence: int
  wisdom: int
  dexterity: int
  constitution: int
  charisma: int
  saving_fortitude: int
  saving_reflex: int
  saving_will: int
  saving_poison: int
  saving_death: int
  spell_resistance: int


@dataclass(frozen=True, slots=True)
class MobileCalculation:
  identifier: int
  profile_version: int
  category: int
  level: int
  race: int
  ch_class: int
  tier: int
  custom_profile: int
  persisted: MobileStats
  expected_post_load: MobileStats


def _sha256(path: Path) -> str:
  digest = hashlib.sha256()
  with path.open("rb") as handle:
    for block in iter(lambda: handle.read(1024 * 1024), b""):
      digest.update(block)
  return digest.hexdigest()


class MobileCalculatorClient:
  """One checked helper process serving any number of mobile requests."""

  def __init__(
      self,
      repo_root: Path | None = None,
      executable: Path | None = None,
      expected_sha256: str | None = None,
  ) -> None:
    self.repo_root = (repo_root or default_repo_root()).resolve()
    if executable is None and os.environ.get("ROL_MOB_CALCULATOR"):
      executable = Path(os.environ["ROL_MOB_CALCULATOR"])
    policy_path = self.repo_root / "scripts/world/rol_conversion_policy.json"
    try:
      policy = json.loads(policy_path.read_text(encoding="ascii"))
      calculator_policy = policy["mobile"]["calculator"]
      policy_versions = (
          int(calculator_policy["protocol_version"]),
          int(calculator_policy["profile_version"]),
          int(calculator_policy["config_version"]),
      )
      if policy_versions != (PROTOCOL_VERSION, PROFILE_VERSION, CONFIG_VERSION):
        raise MobileCalculatorError(
            f"mobile calculator policy versions {policy_versions} do not match bridge versions"
        )
      if expected_sha256 is None and calculator_policy.get("binary_sha256") is not None:
        expected_sha256 = str(calculator_policy["binary_sha256"])
      if executable is None:
        names = calculator_policy["binary_names"]
        executable = next(
            (self.repo_root / str(name) for name in names if (self.repo_root / str(name)).is_file()),
            self.repo_root / str(names[0]),
        )
    except MobileCalculatorError:
      raise
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
      raise MobileCalculatorError(f"cannot resolve mobile calculator policy: {error}") from error
    if executable is None:
      raise MobileCalculatorError("mobile calculator policy has no executable")
    self.executable = executable.resolve()
    self.process: subprocess.Popen[str] | None = None
    self.requests = 0
    if not self.executable.is_file():
      raise MobileCalculatorError(
          f"mobile calculator is missing: {self.executable}; build util/rol_mob_calculator"
      )
    self.sha256 = _sha256(self.executable)
    if expected_sha256 is not None and self.sha256 != expected_sha256:
      raise MobileCalculatorError(
          f"mobile calculator hash {self.sha256} does not match required {expected_sha256}"
      )
    try:
      version = subprocess.run(
          [str(self.executable), "--version"],
          cwd=self.repo_root,
          check=False,
          capture_output=True,
          text=True,
          timeout=10,
      )
    except (OSError, subprocess.SubprocessError) as error:
      raise MobileCalculatorError(f"cannot execute mobile calculator: {error}") from error
    match = _VERSION.fullmatch(version.stdout.strip())
    expected = (PROTOCOL_VERSION, PROFILE_VERSION, CONFIG_VERSION)
    actual = tuple(int(value) for value in match.groups()) if match is not None else None
    if version.returncode != 0 or actual != expected or version.stderr:
      raise MobileCalculatorError(
          "mobile calculator has a stale or malformed version identity: "
          f"status={version.returncode} stdout={version.stdout!r} stderr={version.stderr!r}"
      )
    try:
      executable_identity = str(self.executable.relative_to(self.repo_root))
    except ValueError:
      executable_identity = str(self.executable)
    self.identity = {
        "protocol_version": PROTOCOL_VERSION,
        "profile_version": PROFILE_VERSION,
        "config_version": CONFIG_VERSION,
        "policy_sha256": _sha256(policy_path),
        "executable": executable_identity,
        "executable_sha256": self.sha256,
    }

  def __enter__(self) -> MobileCalculatorClient:
    self.start()
    return self

  def __exit__(self, *_args: object) -> None:
    self.close()

  def start(self) -> None:
    if self.process is not None:
      return
    try:
      self.process = subprocess.Popen(
          [str(self.executable)],
          cwd=self.repo_root,
          stdin=subprocess.PIPE,
          stdout=subprocess.PIPE,
          stderr=subprocess.PIPE,
          text=True,
          bufsize=1,
      )
      self._write(f"ROL_MOB_CALCULATOR {PROTOCOL_VERSION}\n")
      header = self._readline()
    except (OSError, MobileCalculatorError) as error:
      self._terminate()
      if isinstance(error, MobileCalculatorError):
        raise
      raise MobileCalculatorError(f"cannot start mobile calculator: {error}") from error
    if header != f"ROL_MOB_CALCULATOR {PROTOCOL_VERSION}":
      self._fail(f"invalid calculator protocol header {header!r}")

  def calculate(
      self,
      identifier: int,
      level: int,
      race: int,
      ch_class: int,
      tier: int,
      custom_profile: int = 0,
  ) -> MobileCalculation:
    self.start()
    self._write(
        f"MOB {identifier} {level} {race} {ch_class} {tier} {custom_profile}\n"
    )
    row = self._readline()
    tokens = row.split()
    expected_count = 9 + 2 * len(_STAT_FIELDS)
    if len(tokens) != expected_count or tokens[0] != "MOB":
      self._fail(f"malformed calculator response {row!r}")
    try:
      values = [int(value) for value in tokens[1:]]
    except ValueError:
      self._fail(f"non-integer calculator response {row!r}")
    (
        returned_identifier,
        profile_version,
        category,
        returned_level,
        returned_race,
        returned_class,
        returned_tier,
        returned_custom_profile,
    ) = values[:8]
    if returned_identifier != identifier:
      self._fail(
          f"calculator returned identifier {returned_identifier}, expected {identifier}"
      )
    if profile_version != PROFILE_VERSION:
      self._fail(
          f"calculator returned profile {profile_version}, expected {PROFILE_VERSION}"
      )
    returned_inputs = (returned_level, returned_race, returned_class, returned_tier)
    submitted_inputs = (level, race, ch_class, tier)
    if returned_inputs != submitted_inputs:
      self._fail(
          f"calculator returned inputs {returned_inputs}, expected {submitted_inputs}"
      )
    if returned_custom_profile != custom_profile:
      self._fail(
          "calculator returned custom profile "
          f"{returned_custom_profile}, expected {custom_profile}"
      )
    persisted_values = values[8 : 8 + len(_STAT_FIELDS)]
    expected_values = values[8 + len(_STAT_FIELDS) :]
    self.requests += 1
    return MobileCalculation(
        identifier=identifier,
        profile_version=profile_version,
        category=category,
        level=returned_level,
        race=returned_race,
        ch_class=returned_class,
        tier=returned_tier,
        custom_profile=returned_custom_profile,
        persisted=MobileStats(**dict(zip(_STAT_FIELDS, persisted_values, strict=True))),
        expected_post_load=MobileStats(
            **dict(zip(_STAT_FIELDS, expected_values, strict=True))
        ),
    )

  def evidence(self) -> dict[str, Any]:
    return {**self.identity, "requests": self.requests}

  def close(self) -> None:
    if self.process is None:
      return
    process = self.process
    self.process = None
    try:
      if process.poll() is None and process.stdin is not None and process.stdout is not None:
        process.stdin.write("END\n")
        process.stdin.flush()
        response = process.stdout.readline().rstrip("\r\n")
        status = process.wait(timeout=10)
        stderr = process.stderr.read() if process.stderr is not None else ""
        if response != "END" or status != 0 or stderr:
          raise MobileCalculatorError(
              "mobile calculator did not close cleanly: "
              f"response={response!r} status={status} stderr={stderr!r}"
          )
    except (OSError, subprocess.SubprocessError) as error:
      process.kill()
      process.wait()
      raise MobileCalculatorError(f"mobile calculator close failed: {error}") from error
    finally:
      for stream in (process.stdin, process.stdout, process.stderr):
        if stream is not None:
          stream.close()

  def _write(self, value: str) -> None:
    if self.process is None or self.process.stdin is None:
      raise MobileCalculatorError("mobile calculator is not running")
    try:
      self.process.stdin.write(value)
      self.process.stdin.flush()
    except (BrokenPipeError, OSError) as error:
      self._fail(f"calculator request failed: {error}")

  def _readline(self) -> str:
    if self.process is None or self.process.stdout is None:
      raise MobileCalculatorError("mobile calculator is not running")
    row = self.process.stdout.readline()
    if row == "":
      self._fail("calculator closed its output before responding")
    return row.rstrip("\r\n")

  def _fail(self, message: str) -> None:
    stderr = ""
    if self.process is not None and self.process.poll() is not None and self.process.stderr:
      stderr = self.process.stderr.read()
    self._terminate()
    raise MobileCalculatorError(f"{message}; stderr={stderr!r}")

  def _terminate(self) -> None:
    if self.process is None:
      return
    process = self.process
    self.process = None
    if process.poll() is None:
      process.kill()
    process.wait()
    for stream in (process.stdin, process.stdout, process.stderr):
      if stream is not None:
        stream.close()


_DEFAULT_CLIENT: MobileCalculatorClient | None = None


def default_mobile_calculator() -> MobileCalculatorClient:
  """Return the process-wide calculator so bulk generation starts it only once."""

  global _DEFAULT_CLIENT
  if _DEFAULT_CLIENT is None:
    _DEFAULT_CLIENT = MobileCalculatorClient()
  return _DEFAULT_CLIENT


def close_default_mobile_calculator() -> None:
  global _DEFAULT_CLIENT
  if _DEFAULT_CLIENT is not None:
    _DEFAULT_CLIENT.close()
    _DEFAULT_CLIENT = None


atexit.register(close_default_mobile_calculator)


def calculation_to_dict(calculation: MobileCalculation) -> dict[str, Any]:
  """Return stable JSON-ready calculator evidence without reimplementing formulas."""

  return {
      "identifier": calculation.identifier,
      "profile_version": calculation.profile_version,
      "category": calculation.category,
      "level": calculation.level,
      "race": calculation.race,
      "class": calculation.ch_class,
      "tier": calculation.tier,
      "custom_profile": calculation.custom_profile,
      "persisted": asdict(calculation.persisted),
      "expected_post_load": asdict(calculation.expected_post_load),
  }
