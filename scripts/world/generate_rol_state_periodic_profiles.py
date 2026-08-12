#!/usr/bin/env python3
"""Generate the checked-in RoL state-aware periodic profile table."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
from pathlib import Path
import re

from generate_rol_periodic_profiles import (
    Action,
    Outcome,
    _ACTION_CALL,
    _c_string,
    _function_body,
    _matching_brace,
    _parse_actions,
    _source_social_rooms,
)
from wtool_lib.rol_state_periodic_profiles import (
    COMPOSED_STATE_PROFILE_SOURCES,
    STATE_PROFILE_SOURCES,
)


@dataclass(frozen=True)
class StateTable:
  state: str
  dice_count: int
  dice_sides: int
  outcomes: tuple[Outcome, ...]


@dataclass(frozen=True)
class StateProfile:
  name: str
  vnums: tuple[int, ...]
  tables: tuple[StateTable, ...]


def _strip_comments(text: str) -> str:
  return re.sub(r"/\*[\s\S]*?\*/|//[^\n]*", "", text)


def _parse_switch(segment: str, name: str,
                  socials: dict[str, tuple[bool, str] | None]) -> tuple[StateTable, int]:
  switch = re.search(
      r"switch\s*\(\s*dice\s*\(\s*(\d+)\s*,\s*(\d+)\s*\)\s*\)\s*\{", segment
  )
  if switch is None:
    raise ValueError(f"{name}: expected switch(dice(count, sides))")
  opening = segment.find("{", switch.start())
  closing = _matching_brace(segment, opening)
  switch_body = segment[opening + 1 : closing]
  calls = set(re.findall(r"\b([a-z][A-Za-z0-9_]*)\s*\(", switch_body))
  allowed_calls = {"act", "dice", "do_action", "mobsay", "switch"}
  if not calls.issubset(allowed_calls):
    raise ValueError(f"{name}: unsupported state-table calls: {sorted(calls - allowed_calls)}")
  labels = list(re.finditer(r"\b(case\s+(-?\d+)\s*:|default\s*:)", switch_body))
  outcomes: list[Outcome] = []
  pending_rolls: list[int] = []

  for index, label in enumerate(labels):
    if label.group(2) is None:
      if pending_rolls:
        raise ValueError(f"{name}: unresolved fall-through before default")
      continue
    roll = int(label.group(2))
    case_end = labels[index + 1].start() if index + 1 < len(labels) else len(switch_body)
    case_body = switch_body[label.end() : case_end]
    raw_calls = len(re.findall(r"\b(?:act|mobsay|do_action)\s*\(", case_body))
    parsed_actions = _parse_actions(case_body, socials)
    if raw_calls == 0:
      pending_rolls.append(roll)
      continue
    if raw_calls != len(list(_ACTION_CALL.finditer(case_body))):
      raise ValueError(f"{name}: unsupported action expression in case {roll}")
    for outcome_roll in (*pending_rolls, roll):
      outcomes.append(Outcome(outcome_roll, parsed_actions))
    pending_rolls.clear()

  if pending_rolls:
    raise ValueError(f"{name}: unresolved trailing fall-through cases")
  dice_count = int(switch.group(1))
  dice_sides = int(switch.group(2))
  if any(
      outcome.roll < dice_count or outcome.roll > dice_count * dice_sides
      for outcome in outcomes
  ):
    raise ValueError(f"{name}: case outside dice range")
  table = StateTable(
      "", dice_count, dice_sides, tuple(sorted(outcomes, key=lambda item: item.roll))
  )
  return table, closing + 1


def _parse_profile(source_root: Path, name: str, relative: str, vnums: tuple[int, ...],
                   states: tuple[str, ...],
                   socials: dict[str, tuple[bool, str] | None],
                   composed: bool = False) -> StateProfile:
  body = _function_body((source_root / relative).read_text(encoding="ascii"), name)
  calls = set(re.findall(r"\b([a-z][A-Za-z0-9_]*)\s*\(", _strip_comments(body)))
  calls.difference_update({"if", "return", "switch"})
  allowed_calls = {name, "act", "mobsay", "dice"}
  if ((not composed and not calls.issubset(allowed_calls)) or "dice" not in calls or
      "act" not in calls):
    raise ValueError(f"{name}: unsupported calls: {sorted(calls - allowed_calls)}")

  tables: list[StateTable] = []
  remainder = body
  while "switch" in remainder:
    table, consumed = _parse_switch(remainder, name, socials)
    tables.append(table)
    remainder = remainder[consumed:]
  if len(tables) != len(states):
    raise ValueError(f"{name}: expected {len(states)} state tables, found {len(tables)}")
  return StateProfile(
      name,
      vnums,
      tuple(
          StateTable(state, table.dice_count, table.dice_sides, table.outcomes)
          for state, table in zip(states, tables, strict=True)
      ),
  )


def load_profiles(source_root: Path) -> tuple[StateProfile, ...]:
  socials = _source_social_rooms(source_root)
  direct = tuple(
      _parse_profile(source_root, name, relative, vnums, states, socials)
      for name, (relative, vnums, states) in sorted(STATE_PROFILE_SOURCES.items())
  )
  composed = tuple(
      _parse_profile(source_root, name, relative, vnums, states, socials, composed=True)
      for name, (relative, vnums, states) in sorted(COMPOSED_STATE_PROFILE_SOURCES.items())
  )
  return tuple(sorted((*direct, *composed), key=lambda profile: profile.name))


def _identifier(name: str) -> str:
  return "ROL_STATE_PERIODIC_" + re.sub(r"[^A-Za-z0-9]+", "_", name).upper()


def render(source_root: Path) -> str:
  profiles = load_profiles(source_root)
  all_sources = {**STATE_PROFILE_SOURCES, **COMPOSED_STATE_PROFILE_SOURCES}
  source_paths = sorted({relative for relative, _vnums, _states in all_sources.values()})
  source_paths.extend(["src/interp.h", "lib/misc/actions"])
  digest = hashlib.sha256()
  for relative in source_paths:
    digest.update(relative.encode("ascii"))
    digest.update((source_root / relative).read_bytes())

  output = [
      "/* Generated by scripts/world/generate_rol_state_periodic_profiles.py.",
      " * Do not edit directly; regenerate from the assessed RoL source tree.",
      f" * Source digest: {digest.hexdigest()}",
      " */",
      "",
      "enum rol_state_periodic_profile_id",
      "{",
  ]
  for profile in profiles:
    output.append(f"  {_identifier(profile.name)},")
  output.extend(
      [
          "};",
          "",
          "static const struct rol_state_periodic_profile "
          "rol_state_periodic_profiles[] = {",
      ]
  )
  for vnum, profile in sorted(
      (vnum, profile) for profile in profiles for vnum in profile.vnums
  ):
    tables = {table.state: table for table in profile.tables}
    idle = tables.get("idle")
    fighting = tables.get("fighting")
    output.append(
        f"    {{{vnum}, {_identifier(profile.name)}, "
        f"{idle.dice_count if idle else 0}, {idle.dice_sides if idle else 0}, "
        f"{fighting.dice_count if fighting else 0}, {fighting.dice_sides if fighting else 0}}},"
    )
  output.extend(
      [
          "};",
          "",
          "static const struct rol_state_periodic_outcome "
          "rol_state_periodic_outcomes[] = {",
      ]
  )

  action_index = 0
  actions: list[Action] = []
  for profile in profiles:
    for table in sorted(profile.tables, key=lambda item: item.state == "fighting"):
      state = (
          "ROL_STATE_PERIODIC_FIGHTING"
          if table.state == "fighting"
          else "ROL_STATE_PERIODIC_IDLE"
      )
      output.append(f"    /* {profile.name}: {table.state} */")
      for outcome in table.outcomes:
        output.append(
            f"    {{{_identifier(profile.name)}, {state}, {outcome.roll}, "
            f"{action_index}, {len(outcome.actions)}}},"
        )
        actions.extend(outcome.actions)
        action_index += len(outcome.actions)
  output.extend(
      [
          "};",
          "",
          "static const struct rol_source_periodic_action "
          "rol_state_periodic_actions[] = {",
      ]
  )
  for action in actions:
    kind = "ROL_SOURCE_PERIODIC_SPEECH" if action.speech else "ROL_SOURCE_PERIODIC_ROOM_ACTION"
    hide = "true" if action.hide else "false"
    output.append(f"    {{{kind}, {hide}, {_c_string(action.message)}}},")
  output.extend(["};", ""])
  return "\n".join(output)


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--source-root", type=Path, required=True)
  parser.add_argument("--output", type=Path)
  parser.add_argument("--check", action="store_true")
  args = parser.parse_args()
  generated = render(args.source_root.resolve())

  if args.output is None:
    print(generated, end="")
    return 0
  if args.check:
    if not args.output.is_file() or args.output.read_text(encoding="ascii") != generated:
      print(f"stale generated state profile table: {args.output}")
      return 1
    print(f"generated state profile table is current: {args.output}")
    return 0
  args.output.write_text(generated, encoding="ascii", newline="\n")
  print(f"generated {args.output}")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
