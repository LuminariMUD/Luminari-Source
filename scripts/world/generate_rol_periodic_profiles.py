#!/usr/bin/env python3
"""Generate the checked-in RoL source-periodic profile table."""

from __future__ import annotations

import argparse
import ast
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re

from wtool_lib.rol_periodic_profiles import PROFILE_SOURCES

_C_STRING = r'(?:"(?:\\.|[^"\\])*"\s*)+'
_ACTION_CALL = re.compile(
    rf"mobsay\s*\(\s*ch\s*,\s*(?P<say>{_C_STRING})\s*\)"
    rf"|act\s*\(\s*(?P<act>{_C_STRING})\s*,\s*(?P<hide>TRUE|FALSE|1|0)\s*,"
    rf"[\s\S]*?\bTO_ROOM\s*\)"
    r"|do_action\s*\(\s*ch\s*,\s*(?:0|NULL)\s*,\s*CMD_(?P<social>[A-Za-z0-9_]+)\s*\)"
)


@dataclass(frozen=True)
class Action:
  speech: bool
  hide: bool
  message: str


@dataclass(frozen=True)
class Outcome:
  roll: int
  actions: tuple[Action, ...]


@dataclass(frozen=True)
class Profile:
  name: str
  vnums: tuple[int, ...]
  roll_min: int
  roll_max: int
  dice_count: int
  dice_sides: int
  require_awake: bool
  require_sleeping: bool
  suppress_fighting: bool
  outcomes: tuple[Outcome, ...]


def _matching_brace(text: str, opening: int) -> int:
  depth = 0
  quote: str | None = None
  escaped = False
  line_comment = False
  block_comment = False
  index = opening
  while index < len(text):
    char = text[index]
    following = text[index + 1] if index + 1 < len(text) else ""
    if line_comment:
      if char == "\n":
        line_comment = False
    elif block_comment:
      if char == "*" and following == "/":
        block_comment = False
        index += 1
    elif quote is not None:
      if escaped:
        escaped = False
      elif char == "\\":
        escaped = True
      elif char == quote:
        quote = None
    elif char == "/" and following == "/":
      line_comment = True
      index += 1
    elif char == "/" and following == "*":
      block_comment = True
      index += 1
    elif char in {'"', "'"}:
      quote = char
    elif char == "{":
      depth += 1
    elif char == "}":
      depth -= 1
      if depth == 0:
        return index
    index += 1
  raise ValueError("unclosed C brace")


def _function_body(text: str, name: str) -> str:
  match = re.search(rf"\bint\s+{re.escape(name)}\s*\([^)]*\)\s*\{{", text)
  if match is None:
    raise ValueError(f"source function not found: {name}")
  opening = text.find("{", match.start())
  return text[opening + 1 : _matching_brace(text, opening)]


def _decode_c_strings(source: str) -> str:
  tokens = re.findall(r'"(?:\\.|[^"\\])*"', source, re.DOTALL)
  if not tokens:
    raise ValueError("missing C string literal")
  return "".join(ast.literal_eval(token) for token in tokens)


def _source_social_rooms(source_root: Path) -> dict[str, tuple[bool, str] | None]:
  command_numbers: dict[str, int] = {}
  header = (source_root / "src/interp.h").read_text(encoding="ascii")
  for name, value in re.findall(r"^#define\s+CMD_([A-Za-z0-9_]+)\s+(\d+)\s*$", header, re.MULTILINE):
    command_numbers[name] = int(value)

  messages: dict[int, tuple[bool, str] | None] = {}
  actions = (source_root / "lib/misc/actions").read_text(encoding="ascii")
  for block in re.split(r"\n\s*\n", actions):
    lines = block.splitlines()
    if not lines:
      continue
    match = re.fullmatch(r"(\d+)\s+(\d+)\s+\d+", lines[0])
    if match is None or len(lines) < 3:
      continue
    command = int(match.group(1))
    room_message = lines[2]
    messages[command] = None if room_message == "#" else (bool(int(match.group(2))), room_message)

  return {name: messages.get(value) for name, value in command_numbers.items()}


def _parse_actions(segment: str, socials: dict[str, tuple[bool, str] | None]) -> tuple[Action, ...]:
  actions: list[Action] = []
  for match in _ACTION_CALL.finditer(segment):
    if match.group("say") is not None:
      actions.append(Action(True, False, _decode_c_strings(match.group("say"))))
    elif match.group("act") is not None:
      actions.append(
          Action(False, match.group("hide") in {"TRUE", "1"}, _decode_c_strings(match.group("act")))
      )
    else:
      social = socials.get(match.group("social"))
      if social is not None:
        actions.append(Action(False, social[0], social[1]))
  return tuple(actions)


def _parse_profile(source_root: Path, name: str, relative: str, vnums: tuple[int, ...],
                   socials: dict[str, tuple[bool, str] | None]) -> Profile:
  body = _function_body((source_root / relative).read_text(encoding="ascii"), name)
  switch = re.search(
      r"switch\s*\(\s*(number|dice)\s*\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)\s*\)\s*\{",
      body,
  )
  if switch is None:
    raise ValueError(f"{name}: expected one switch(number(low, high)) or switch(dice(count, sides))")
  opening = body.find("{", switch.start())
  switch_body = body[opening + 1 : _matching_brace(body, opening)]
  labels = list(re.finditer(r"\b(case\s+(-?\d+)\s*:|default\s*:)", switch_body))
  outcomes: list[Outcome] = []
  pending_rolls: list[int] = []

  for index, label in enumerate(labels):
    if label.group(2) is None:
      if pending_rolls:
        raise ValueError(f"{name}: unresolved fall-through before default")
      continue
    roll = int(label.group(2))
    segment_end = labels[index + 1].start() if index + 1 < len(labels) else len(switch_body)
    segment = switch_body[label.end() : segment_end]
    raw_calls = len(re.findall(r"\b(?:act|mobsay|do_action)\s*\(", segment))
    parsed_actions = _parse_actions(segment, socials)
    if raw_calls == 0 and "return TRUE" not in segment:
      pending_rolls.append(roll)
      continue
    if raw_calls != len(list(_ACTION_CALL.finditer(segment))):
      raise ValueError(f"{name}: unsupported action expression in case {roll}")
    for outcome_roll in (*pending_rolls, roll):
      outcomes.append(Outcome(outcome_roll, parsed_actions))
    pending_rolls.clear()

  if pending_rolls:
    raise ValueError(f"{name}: unresolved trailing fall-through cases")

  random_kind = switch.group(1)
  first_value = int(switch.group(2))
  second_value = int(switch.group(3))
  if random_kind == "dice":
    if first_value <= 0 or second_value <= 0:
      raise ValueError(f"{name}: dice values must be positive")
    roll_min = first_value
    roll_max = first_value * second_value
    dice_count = first_value
    dice_sides = second_value
  else:
    roll_min = first_value
    roll_max = second_value
    dice_count = 0
    dice_sides = 0
  if any(outcome.roll < roll_min or outcome.roll > roll_max for outcome in outcomes):
    raise ValueError(f"{name}: case outside random range")
  return Profile(
      name,
      vnums,
      roll_min,
      roll_max,
      dice_count,
      dice_sides,
      "AWAKE(ch)" in body,
      "STAT_SLEEPING" in body,
      "IS_FIGHTING(ch)" in body,
      tuple(sorted(outcomes, key=lambda outcome: outcome.roll)),
  )


def load_profiles(source_root: Path) -> tuple[Profile, ...]:
  socials = _source_social_rooms(source_root)
  return tuple(
      _parse_profile(source_root, name, relative, vnums, socials)
      for name, (relative, vnums) in sorted(PROFILE_SOURCES.items())
  )


def _identifier(name: str) -> str:
  return "ROL_SOURCE_PERIODIC_" + re.sub(r"[^A-Za-z0-9]+", "_", name).upper()


def _c_string(value: str) -> str:
  return json.dumps(value, ensure_ascii=True)


def render(source_root: Path) -> str:
  profiles = load_profiles(source_root)
  source_paths = sorted({relative for relative, _vnums in PROFILE_SOURCES.values()})
  source_paths.extend(["src/interp.h", "lib/misc/actions"])
  digest = hashlib.sha256()
  for relative in source_paths:
    digest.update(relative.encode("ascii"))
    digest.update((source_root / relative).read_bytes())

  output = [
      "/* Generated by scripts/world/generate_rol_periodic_profiles.py.",
      " * Do not edit directly; regenerate from the assessed RoL source tree.",
      f" * Source digest: {digest.hexdigest()}",
      " */",
      "",
      "enum rol_source_periodic_profile_id",
      "{",
  ]
  for profile in profiles:
    output.append(f"  {_identifier(profile.name)},")
  output.extend(["};", "", "static const struct rol_source_periodic_profile rol_source_periodic_profiles[] = {"])
  profile_rows = sorted(
      (vnum, profile)
      for profile in profiles
      for vnum in profile.vnums
  )
  for vnum, profile in profile_rows:
    require_awake = "true" if profile.require_awake else "false"
    require_sleeping = "true" if profile.require_sleeping else "false"
    suppress = "true" if profile.suppress_fighting else "false"
    output.append(
        f"    {{{vnum}, {_identifier(profile.name)}, {profile.roll_min}, {profile.roll_max}, "
        f"{profile.dice_count}, {profile.dice_sides}, {require_awake}, {require_sleeping}, "
        f"{suppress}}},"
    )
  output.extend(["};", "", "static const struct rol_source_periodic_outcome rol_source_periodic_outcomes[] = {"])

  action_index = 0
  flattened_actions: list[Action] = []
  for profile in profiles:
    output.append(f"    /* {profile.name} */")
    for outcome in profile.outcomes:
      output.append(
          f"    {{{_identifier(profile.name)}, {outcome.roll}, {action_index}, {len(outcome.actions)}}},"
      )
      flattened_actions.extend(outcome.actions)
      action_index += len(outcome.actions)
  output.extend(["};", "", "static const struct rol_source_periodic_action rol_source_periodic_actions[] = {"])
  for action in flattened_actions:
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
      print(f"stale generated profile table: {args.output}")
      return 1
    print(f"generated profile table is current: {args.output}")
    return 0
  args.output.write_text(generated, encoding="ascii", newline="\n")
  print(f"generated {args.output}")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
