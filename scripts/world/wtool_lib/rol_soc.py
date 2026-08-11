"""Compile legacy Realms of Luminari SOC behavior into target DG triggers."""

from __future__ import annotations

from collections import Counter, defaultdict
from dataclasses import dataclass, field
from typing import Callable, Iterable

from .flags import encode_bits
from .rol_source import RolRecord
from .rol_transform import convert_text


IdentityResolver = Callable[[str, int], int]

NON_COMBAT_ONLY = 1
COMBAT_ONLY = 2
USE_INITIATOR = 4
USE_SELF = 8
USE_TARGET = 16
DIRECTED = 32
BLOCK_OTHER = 64
FROM_MOB = 128
FROM_PC = 256

_MODE_ORDER = {"ACTION": 0, "TIME": 1, "PATH": 2}
_COMMAND_OVERRIDES = {
    "vis": "visible",
    # Every selected use is documented as aborting a spell. The source command
    # table drifted while the numeric SOC data remained fixed.
    "wiznews": "abort",
}
_SOURCE_ONLY_TRIGGER_COMMANDS = frozenset({"snoogie"})


@dataclass(slots=True)
class SocAction:
  """One normalized source SOC action."""

  code: int
  argument: str
  flag: int
  chance: int
  delay: int
  line: int


@dataclass(slots=True)
class SocPath:
  """One normalized source SOC path."""

  basename: str
  source_mobile: int
  path_id: int
  path_type: int
  delay: int
  rooms: list[int]
  directions: list[int]
  line: int


@dataclass(slots=True)
class SocTrigger:
  """One emitted target DG trigger and its mobile attachment."""

  vnum: int
  host_mobile_vnum: int
  source_mobile_vnum: int
  trigger_kind: str
  source_modes: tuple[str, ...]
  text: str


@dataclass(slots=True)
class SocCompilation:
  """Deterministic SOC compilation result."""

  triggers: list[SocTrigger]
  attachments: dict[int, list[int]]
  diagnostics: list[str] = field(default_factory=list)
  source_records: int = 0
  source_actions: int = 0

  @property
  def trigger_text(self) -> str:
    return "".join(trigger.text for trigger in self.triggers) + "$~\n"


def build_soc_prototype_comparison(
    records: Iterable[RolRecord], compilation: SocCompilation
) -> dict[str, object]:
  """Return measured native-projection versus DG-pilot decision evidence."""

  selected = list(records)
  modes = Counter(record.format_version for record in selected)
  path_records = modes["PATH"]
  special_actions = Counter(
      int(directive["arguments"][0])
      for record in selected
      for directive in record.directives
      if directive["token"] == "ACTION"
      and directive.get("arguments")
      and 1000 <= int(directive["arguments"][0]) <= 1004
  )
  trigger_kinds = Counter(trigger.trigger_kind for trigger in compilation.triggers)
  reduction = (
      100.0 * (len(selected) - len(compilation.triggers)) / len(selected)
      if selected
      else 0.0
  )
  return {
      "selection": "dg_compilation",
      "measured_source": {
          "records": len(selected),
          "actions": compilation.source_actions,
          "modes": dict(sorted(modes.items())),
          "path_records": path_records,
          "special_actions": dict(sorted(special_actions.items())),
      },
      "native_compatibility_projection": {
          "persisted_behavior_records": len(selected),
          "new_loader_and_storage": True,
          "new_runtime_scheduler": True,
          "new_olc_surface": True,
          "uses_existing_trigger_parser_and_validator": False,
          "fidelity": "direct source data model with source defects requiring repair",
      },
      "dg_compilation_pilot": {
          "persisted_trigger_records": len(compilation.triggers),
          "trigger_records_by_kind": dict(sorted(trigger_kinds.items())),
          "attached_mobiles": len(compilation.attachments),
          "record_reduction_percent": round(reduction, 6),
          "bounded_runtime_helpers": ["mrolwalkto", "mrolzoneecho"],
          "new_olc_surface": False,
          "uses_existing_trigger_parser_and_validator": True,
          "fidelity": "all observed modes and action codes with explicit source-defect repairs",
      },
      "decision": {
          "fidelity": "DG preserves the observed pilot contract with bounded helpers",
          "olc_maintainability": "DG uses the existing trigger editor and attachment model",
          "observability": "DG uses existing trigger logs, stat, and validation",
          "performance": "DG reuses heartbeat, command, time, and load trigger dispatch",
          "generated_volume": (
              f"{len(compilation.triggers)} DG triggers versus {len(selected)} native rows"
          ),
          "testing_cost": "DG reuses structural parsing and needs only compiler/helper fixtures",
      },
  }


def _single(directive: dict[str, object], default: int = -1) -> int:
  arguments = directive.get("arguments", [])
  return int(arguments[0]) if arguments else default


def _clean_argument(value: str) -> tuple[str, list[str]]:
  converted, diagnostics = convert_text(value)
  converted = converted.replace("\\n", " ")
  converted = " ".join(converted.replace("\r", "\n").splitlines()).strip()
  return converted, diagnostics


def _parse_action_groups(
    record: RolRecord,
    diagnostics: list[str],
) -> list[list[SocAction]]:
  groups: list[list[SocAction]] = [[]]
  state: dict[str, int] = {}
  for directive in record.directives:
    token = str(directive["token"])
    if token == "LISTDONE":
      break
    if token in {"FLAG", "CHANCE", "DELAY", "HOUR", "TRIGGER"}:
      state[token] = _single(directive)
      continue
    if token == "ACTION":
      missing = [name for name in ("FLAG", "CHANCE", "DELAY") if name not in state]
      if missing:
        diagnostics.append(
            f"SOC {record.basename}:{record.vnum}:{directive['line']} lacks "
            + ", ".join(missing)
        )
        continue
      argument, text_diagnostics = _clean_argument(str(directive.get("argument", "")))
      diagnostics.extend(
          f"SOC {record.basename}:{record.vnum}:{directive['line']}: {item}"
          for item in text_diagnostics
      )
      groups[-1].append(
          SocAction(
              code=_single(directive),
              argument=argument,
              flag=state["FLAG"],
              chance=state["CHANCE"],
              delay=state["DELAY"],
              line=int(directive["line"]),
          )
      )
      for name in ("FLAG", "CHANCE", "DELAY"):
        state.pop(name, None)
      continue
    if record.format_version == "LIST" and token == "DONE":
      groups.append([])
  return [group for group in groups if group]


def _parse_path(record: RolRecord, diagnostics: list[str]) -> SocPath | None:
  values: dict[str, list[int]] = {}
  for directive in record.directives:
    values[str(directive["token"])] = [
        int(value) for value in directive.get("arguments", [])
    ]
  required = ("ID", "TYPE", "DELAY", "ROOMS", "DIRS")
  missing = [name for name in required if not values.get(name)]
  if missing:
    diagnostics.append(
        f"SOC path {record.basename}:{record.vnum}:{record.line} lacks "
        + ", ".join(missing)
    )
    return None
  rooms = values["ROOMS"]
  directions = values["DIRS"]
  expected_directions = 1 if values["TYPE"][0] == 4 else max(0, len(rooms) - 1)
  if len(directions) < expected_directions:
    diagnostics.append(
        f"SOC path {record.basename}:{record.vnum}:{record.line} has "
        f"{len(directions)} direction(s) for {len(rooms)} room(s)"
    )
    return None
  return SocPath(
      basename=record.basename,
      source_mobile=record.vnum,
      path_id=values["ID"][0],
      path_type=values["TYPE"][0],
      delay=values["DELAY"][0],
      rooms=rooms,
      directions=directions,
      line=record.line,
  )


def _indent(lines: Iterable[str], amount: int = 2) -> list[str]:
  prefix = " " * amount
  return [prefix + line if line else line for line in lines]


def _chance_lines(chance: int, body: list[str]) -> list[str]:
  if chance <= 0:
    return body
  return [f"if %random.{chance + 1}% == 1", *_indent(body), "end"]


def _flag_condition(flag: int) -> str | None:
  if flag & NON_COMBAT_ONLY:
    return "!%self.fighting%"
  if flag & COMBAT_ONLY:
    return "%self.fighting%"
  return None


def _standard_command(
    action: SocAction,
    source_commands: dict[int, str],
    diagnostics: list[str],
) -> str:
  source_name = source_commands.get(action.code)
  if source_name is None:
    diagnostics.append(f"SOC action {action.code} at source line {action.line} is unmapped")
    return f"* unmapped source SOC action {action.code}"
  command = _COMMAND_OVERRIDES.get(source_name, source_name)
  if source_name == "wiznews":
    diagnostics.append(
        f"SOC action {action.code} at source line {action.line} repaired from "
        "drifted wiznews index to intended abort command"
    )
  if source_name == "consent" and not action.argument:
    diagnostics.append(
        f"SOC NPC consent query at source line {action.line} retained as an explicit no-op"
    )
    return "* source NPC consent query has no player-visible target effect"

  if source_name == "ack":
    return "emote acks around like Bill the Cat."
  if source_name == "eyebrow":
    target = f" at {action.argument}" if action.argument else ""
    return f"emote raises an eyebrow{target}."
  if source_name == "roar":
    return "emote lets out a huge thunderous roar!"
  if source_name == "salute":
    target = f" and salutes {action.argument}" if action.argument else ""
    return f"emote snaps to attention{target}."
  if source_name == "tip":
    target = f" toward {action.argument}" if action.argument else ""
    return f"emote tips a hat{target}."

  arguments = [action.argument] if action.argument else []
  if action.flag & USE_INITIATOR:
    arguments.append("%actor.name%")
  elif action.flag & USE_SELF:
    arguments.append("me")
  elif action.flag & USE_TARGET:
    arguments.append("%self.fighting%")
  return " ".join([command, *arguments]).rstrip()


def _path_lines(path: SocPath, resolve: IdentityResolver) -> list[str]:
  rooms = [resolve("wld", room) for room in path.rooms]
  if path.path_type == 4:
    destination = rooms[0]
    delay = max(1, path.delay)
    return [
        f"while %self.room.vnum% != {destination}",
        "  set rol_path_before %self.room.vnum%",
        f"  wait {delay} s",
        f"  mrolwalkto {destination}",
        "  if %self.room.vnum% == %rol_path_before%",
        "    return 0",
        "  end",
        "done",
    ]

  delay = max(5, path.delay)
  lines = ["set rol_path_before %self.room.vnum%"]
  for current, destination in zip(rooms, rooms[1:]):
    lines.extend(
        [
            f"if %self.room.vnum% == {current}",
            f"  wait {delay} s",
            f"  mrolwalkto {destination}",
            "end",
        ]
    )
  lines.extend(
      [
          "if %self.room.vnum% == %rol_path_before%",
          "  return 0",
          "end",
      ]
  )
  return lines


def _action_lines(
    action: SocAction,
    record: RolRecord,
    paths: dict[tuple[str, int, int], SocPath],
    resolve: IdentityResolver,
    source_commands: dict[int, str],
    diagnostics: list[str],
) -> tuple[list[str], bool]:
  if action.code == 1004:
    try:
      path_id = int(action.argument)
    except ValueError:
      diagnostics.append(
          f"SOC path action at {record.basename}:{record.vnum}:{action.line} has invalid id "
          f"{action.argument!r}"
      )
      return ["* invalid source SOC path id; action disabled"], True
    path = paths.get((record.basename, record.vnum, path_id))
    if path is None:
      diagnostics.append(
          f"SOC path action at {record.basename}:{record.vnum}:{action.line} references "
          f"missing path {path_id}"
      )
      return ["* unresolved source SOC path; action disabled"], True
    return _path_lines(path, resolve), True

  if action.code in {1000, 1001, 1002, 1003}:
    commands = {
        1000: "mrolzoneecho indoors %self.room.vnum%",
        1001: "mrolzoneecho outdoors %self.room.vnum%",
        1002: "mrolzoneecho all %self.room.vnum%",
        1003: "mecho",
    }
    command = f"{commands[action.code]} {action.argument}".rstrip()
  else:
    command = _standard_command(action, source_commands, diagnostics)

  condition = _flag_condition(action.flag)
  if condition is None:
    return [command], False
  return [f"if {condition}", f"  {command}", "end"], False


def _chain_lines(
    actions: list[SocAction],
    record: RolRecord,
    paths: dict[tuple[str, int, int], SocPath],
    resolve: IdentityResolver,
    source_commands: dict[int, str],
    diagnostics: list[str],
) -> list[str]:
  lines: list[str] = []
  elapsed = 0
  for action in actions:
    wait = max(0, action.delay - elapsed)
    if action.delay < elapsed:
      diagnostics.append(
          f"SOC action delay regressed from {elapsed} to {action.delay} at source line "
          f"{action.line}; emitted without an additional wait"
      )
    if wait:
      lines.append(f"wait {wait}")
    emitted, resets_delay = _action_lines(
        action, record, paths, resolve, source_commands, diagnostics
    )
    lines.extend(emitted)
    elapsed = 0 if resets_delay else max(elapsed, action.delay)
  return lines


def _record_random_lines(
    record: RolRecord,
    paths: dict[tuple[str, int, int], SocPath],
    resolve: IdentityResolver,
    source_commands: dict[int, str],
    diagnostics: list[str],
) -> list[str]:
  groups = _parse_action_groups(record, diagnostics)
  if not groups:
    return []
  if record.format_version == "LIST":
    lines = [f"switch %random.{len(groups)}%"]
    for index, actions in enumerate(groups, start=1):
      body = _chain_lines(actions, record, paths, resolve, source_commands, diagnostics)
      if len(actions) > 1:
        diagnostics.append(
            f"SOC list {record.basename}:{record.vnum}:{actions[0].line} preserved "
            "its intended action chain instead of the source flat-array truncation"
        )
      if not (actions[0].flag & BLOCK_OTHER):
        body.append("return 0")
      body = _chance_lines(actions[0].chance, body)
      lines.extend([f"case {index}", *_indent(body), "  break"])
    lines.append("done")
    return lines
  body = _chain_lines(groups[0], record, paths, resolve, source_commands, diagnostics)
  if not (groups[0][0].flag & BLOCK_OTHER):
    body.append("return 0")
  return _chance_lines(groups[0][0].chance, body)


def _command_guards(action: SocAction) -> list[str]:
  guards: list[str] = []
  if action.flag & DIRECTED:
    guards.extend(
        [
            "if !%arg% || !(%self.name% /= %arg.car%)",
            "  return 0",
            "end",
        ]
    )
  if action.flag & FROM_MOB:
    guards.extend(
        [
            "if %actor.is_pc%",
            "  return 0",
            "end",
            "set rol_actor_master %actor.master%",
            "if %rol_actor_master.is_pc%",
            "  return 0",
            "end",
        ]
    )
  if action.flag & FROM_PC:
    guards.extend(["if !%actor.is_pc%", "  return 0", "end"])
  return guards


def _command_record_lines(
    record: RolRecord,
    paths: dict[tuple[str, int, int], SocPath],
    resolve: IdentityResolver,
    source_commands: dict[int, str],
    diagnostics: list[str],
) -> tuple[str, list[str]] | None:
  groups = _parse_action_groups(record, diagnostics)
  if not groups:
    return None
  trigger_rows = [
      directive for directive in record.directives if directive["token"] == "TRIGGER"
  ]
  if not trigger_rows:
    diagnostics.append(
        f"SOC trigger {record.basename}:{record.vnum}:{record.line} has no source command"
    )
    return None
  trigger_code = _single(trigger_rows[0])
  trigger_name = source_commands.get(trigger_code)
  if trigger_name is None:
    diagnostics.append(
        f"SOC trigger {record.basename}:{record.vnum}:{record.line} uses unmapped command "
        f"{trigger_code}"
    )
    return None
  actions = groups[0]
  body = _command_guards(actions[0])
  body.extend(_chain_lines(actions, record, paths, resolve, source_commands, diagnostics))
  if trigger_name in _SOURCE_ONLY_TRIGGER_COMMANDS:
    body.append("return 1")
  body = _chance_lines(actions[0].chance, body)
  condition = f"%cmd.mudcommand% == {trigger_name} || %cmd% == {trigger_name}"
  return condition, body


def _trigger_text(
    vnum: int,
    name: str,
    flags: set[int],
    numeric_argument: int,
    argument: str,
    body: list[str],
) -> str:
  encoded = encode_bits(flags)[0]
  return (
      f"#{vnum}\n{name}~\n0 {encoded} {numeric_argument}\n{argument}~\n"
      + "\n".join(body)
      + "\n~\n"
  )


def _action_trigger(
    vnum: int,
    host: int,
    source_mobile: int,
    records: list[RolRecord],
    paths: dict[tuple[str, int, int], SocPath],
    resolve: IdentityResolver,
    source_commands: dict[int, str],
    diagnostics: list[str],
) -> SocTrigger:
  random_records = [
      record for record in records if record.format_version in {"LIST", "PERIODIC"}
  ]
  command_records = [record for record in records if record.format_version == "TRIGGER"]
  flags = ({1} if random_records else set()) | ({2} if command_records else set())
  body = [f"* Compiled RoL SOC action trigger for mobile {source_mobile}."]
  if command_records:
    body.append("if %cmd%")
    for record in command_records:
      compiled = _command_record_lines(
          record, paths, resolve, source_commands, diagnostics
      )
      if compiled is None:
        continue
      condition, commands = compiled
      body.extend([f"  if {condition}", *_indent(commands, 4), "  end"])
    body.extend(["  return 0", "end"])
  for record in random_records:
    compiled = _record_random_lines(
        record, paths, resolve, source_commands, diagnostics
    )
    if not compiled:
      continue
    body.extend(compiled)
  modes = tuple(sorted({record.format_version for record in records if record.format_version != "PATH"}))
  return SocTrigger(
      vnum=vnum,
      host_mobile_vnum=host,
      source_mobile_vnum=source_mobile,
      trigger_kind="ACTION",
      source_modes=modes,
      text=_trigger_text(
          vnum,
          f"RoL SOC {source_mobile} action",
          flags,
          100,
          "*" if command_records else "",
          body,
      ),
  )


def _time_trigger(
    vnum: int,
    host: int,
    source_mobile: int,
    hour: int,
    records: list[RolRecord],
    paths: dict[tuple[str, int, int], SocPath],
    resolve: IdentityResolver,
    source_commands: dict[int, str],
    diagnostics: list[str],
) -> SocTrigger:
  body = [f"* Compiled RoL SOC time trigger for mobile {source_mobile} at hour {hour}."]
  for record in records:
    groups = _parse_action_groups(record, diagnostics)
    if not groups:
      continue
    chain = _chain_lines(groups[0], record, paths, resolve, source_commands, diagnostics)
    body.extend(_chance_lines(groups[0][0].chance, chain))
  return SocTrigger(
      vnum=vnum,
      host_mobile_vnum=host,
      source_mobile_vnum=source_mobile,
      trigger_kind="TIME",
      source_modes=("TIMED",),
      text=_trigger_text(
          vnum, f"RoL SOC {source_mobile} time {hour}", {19}, hour, "", body
      ),
  )


def _path_trigger(
    vnum: int,
    host: int,
    source_mobile: int,
    path: SocPath,
    resolve: IdentityResolver,
) -> SocTrigger:
  path_body = _path_lines(path, resolve)
  if path.path_type == 3:
    body = [
        f"* Compiled RoL SOC autonomous loop path for mobile {source_mobile}.",
        "while 1",
        "  if %self.fighting%",
        "    wait 30 s",
        "  else",
        *_indent(path_body, 4),
        "  end",
        "done",
    ]
  else:
    body = [
        f"* Compiled RoL SOC autonomous seek path for mobile {source_mobile}.",
        *path_body,
    ]
  return SocTrigger(
      vnum=vnum,
      host_mobile_vnum=host,
      source_mobile_vnum=source_mobile,
      trigger_kind="PATH",
      source_modes=("PATH",),
      text=_trigger_text(vnum, f"RoL SOC {source_mobile} path", {13}, 100, "", body),
  )


def compile_soc_records(
    records: Iterable[RolRecord],
    trigger_start: int,
    resolve: IdentityResolver,
    source_commands: dict[int, str],
    trigger_vnums: Iterable[int] | None = None,
) -> SocCompilation:
  """Compile source SOC records into deterministic, attachable DG triggers."""

  selected = sorted(records, key=lambda record: (record.basename, record.vnum, record.line))
  diagnostics: list[str] = []
  paths: dict[tuple[str, int, int], SocPath] = {}
  source_actions = 0
  for record in selected:
    source_actions += sum(directive["token"] == "ACTION" for directive in record.directives)
    if record.format_version != "PATH":
      continue
    path = _parse_path(record, diagnostics)
    if path is not None:
      paths[(record.basename, record.vnum, path.path_id)] = path

  by_host: dict[int, list[RolRecord]] = defaultdict(list)
  source_mobile_by_host: dict[int, int] = {}
  for record in selected:
    host = resolve("mob", record.vnum)
    by_host[host].append(record)
    source_mobile_by_host.setdefault(host, record.vnum)

  work: list[tuple[int, str, int, object]] = []
  for host, host_records in by_host.items():
    action_records = [
        record
        for record in host_records
        if record.format_version in {"LIST", "PERIODIC", "TRIGGER"}
    ]
    if action_records:
      work.append((host, "ACTION", 0, action_records))
    timed: dict[int, list[RolRecord]] = defaultdict(list)
    for record in host_records:
      if record.format_version != "TIMED":
        continue
      hours = {
          _single(directive)
          for directive in record.directives
          if directive["token"] == "HOUR"
      }
      for hour in hours:
        timed[hour].append(record)
    for hour, timed_records in timed.items():
      work.append((host, "TIME", hour, timed_records))
    autonomous = [
        path
        for path in paths.values()
        if resolve("mob", path.source_mobile) == host and path.path_id == 0
    ]
    if len(autonomous) > 1:
      diagnostics.append(
          f"target mobile {host} has {len(autonomous)} autonomous source paths; "
          "only the first source path can own the legacy mobile path state"
      )
    if autonomous:
      work.append((host, "PATH", autonomous[0].line, autonomous[0]))

  work.sort(key=lambda item: (item[0], _MODE_ORDER[item[1]], item[2]))
  triggers: list[SocTrigger] = []
  allocated_vnums = iter(trigger_vnums) if trigger_vnums is not None else None
  for offset, (host, kind, qualifier, payload) in enumerate(work):
    if allocated_vnums is None:
      vnum = trigger_start + offset
    else:
      try:
        vnum = next(allocated_vnums)
      except StopIteration as error:
        raise ValueError("SOC trigger VNUM allocator was exhausted") from error
    source_mobile = source_mobile_by_host[host]
    if kind == "ACTION":
      assert isinstance(payload, list)
      trigger = _action_trigger(
          vnum,
          host,
          source_mobile,
          payload,
          paths,
          resolve,
          source_commands,
          diagnostics,
      )
    elif kind == "TIME":
      assert isinstance(payload, list)
      trigger = _time_trigger(
          vnum,
          host,
          source_mobile,
          qualifier,
          payload,
          paths,
          resolve,
          source_commands,
          diagnostics,
      )
    else:
      assert isinstance(payload, SocPath)
      trigger = _path_trigger(vnum, host, source_mobile, payload, resolve)
    triggers.append(trigger)

  attachments: dict[int, list[int]] = defaultdict(list)
  for trigger in triggers:
    attachments[trigger.host_mobile_vnum].append(trigger.vnum)
  return SocCompilation(
      triggers=triggers,
      attachments=dict(sorted(attachments.items())),
      diagnostics=diagnostics,
      source_records=len(selected),
      source_actions=source_actions,
  )
