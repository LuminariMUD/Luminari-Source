"""RoL quests source grammar and target conversion."""

from __future__ import annotations

import re
import textwrap

from .rol_conversion_types import IdentityResolver, RolRecord, RolSourceCorpus, TransformResult
from .rol_source_common import (
    _INTEGER,
    _diagnostic,
    _exclude_record,
    _new_record,
    _next_content,
    _read_tilde,
    _reference,
    _segments,
)
from .rol_transform_common import convert_text
from .source import SourceFile, SourceLine


def _quest_arguments(
    lines: list[SourceLine],
    position: int,
    end: int,
    command_line: SourceLine,
    expected_values: int,
) -> tuple[int, str, list[int], SourceLine]:
  """Read the whitespace-insensitive G/R payload used by the source fscanf loader."""

  payload = command_line.raw.strip()[1:].strip()
  argument_line = command_line
  while position < end:
    subtype_match = re.match(br"\s*([A-Za-z])", payload)
    values = [int(value) for value in _INTEGER.findall(payload[subtype_match.end():])] if subtype_match else []
    minimum = expected_values
    if subtype_match is not None and expected_values == 0:
      minimum = 0 if subtype_match.group(1).upper() == b"A" else 1
    if subtype_match is not None and len(values) >= minimum:
      return position, subtype_match.group(1).decode("ascii").upper(), values, argument_line
    next_position, line = _next_content(lines, position, end)
    if line is None:
      break
    payload += b" " + line.raw.strip()
    position = next_position
    argument_line = line
  subtype_match = re.match(br"\s*([A-Za-z])", payload)
  subtype = subtype_match.group(1).decode("ascii").upper() if subtype_match else ""
  values = [int(value) for value in _INTEGER.findall(payload[subtype_match.end():])] if subtype_match else []
  return position, subtype, values, argument_line


def _parse_qst(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  corpus.file_versions[("qst", "legacy")] += 1
  for start, end, vnum in _segments(source):
    position = start + 1
    position, peek = _next_content(source.lines, position, end)
    if peek is not None and peek.raw.strip().startswith(b"$"):
      corpus.file_terminators[("qst", "sentinel")] += 1
      continue
    record = _new_record(source, basename, "qst", start, end, vnum)
    record.identity = f"host {vnum}"
    _reference(record, "mobile", vnum, "quest_host", source.lines[start])
    position = start + 1
    terminated = False
    while position < end:
      position, line = _next_content(source.lines, position, end)
      if line is None:
        break
      token = line.raw.strip()[:1].decode("ascii", errors="replace")
      if token == "S":
        record.directives.append({"token": "S", "line": line.number})
        terminated = True
        break
      if token == "M":
        position, keyword, first_ok = _read_tilde(source.lines, position, end)
        position, message, second_ok = _read_tilde(source.lines, position, end)
        record.directives.append(
            {
                "token": "M",
                "line": line.number,
                "keyword": keyword,
                "message": message,
            }
        )
        if not first_ok or not second_ok:
          record.complete = False
        continue
      if token in {"Q", "D"}:
        position, message, ok = _read_tilde(source.lines, position, end)
        record.directives.append(
            {"token": token, "line": line.number, "message": message}
        )
        if not ok:
          record.complete = False
        continue
      if token in {"G", "R"}:
        expected_values = 1 if token == "G" else 0
        position, subtype, values, argument_line = _quest_arguments(
            source.lines, position, end, line, expected_values
        )
        record.directives.append(
            {"token": token, "subtype": subtype, "line": line.number, "arguments": values}
        )
        minimum = 0 if token == "R" and subtype == "A" else 1
        if not subtype or len(values) < minimum:
          record.complete = False
          _diagnostic(corpus, "ROLQST001", "error", "quest goal row is incomplete", line, "qst", vnum)
          continue
        number = values[0] if values else 0
        destination = values[1] if len(values) > 1 else 0
        if token == "G" and subtype == "I":
          _reference(record, "object", number, "quest_required_item", argument_line)
        elif token == "R" and subtype == "I":
          _reference(record, "object", number, "quest_reward_item", argument_line)
          if destination > number and destination - number < 100:
            _reference(record, "object", destination, "quest_random_reward_end", argument_line)
        elif token == "R" and subtype == "M":
          _reference(record, "mobile", number, "quest_load_mobile", argument_line)
          _reference(record, "room", destination, "quest_load_destination", argument_line)
        elif token == "R" and subtype == "O":
          _reference(record, "object", number, "quest_load_object", argument_line)
          _reference(record, "room", destination, "quest_load_destination", argument_line)
        continue
      record.complete = False
      _diagnostic(corpus, "ROLQST002", "error", f"unknown quest directive {token!r}", line, "qst", vnum)
    if not terminated:
      _exclude_record(
          corpus,
          record,
          "ROLQST003",
          "source quest block is incomplete or lacks its S terminator",
          source.lines[start],
      )
    records.append(record)
  corpus.file_terminators[("qst", "present" if any(line.raw.strip().startswith(b"$") for line in source.lines) else "absent")] += 1
  return records


_SOURCE_QUEST_REWARD_MAP: dict[int, tuple[str, int | None]] = {
    72: ("meteor swarm", 74),
    73: ("creeping doom", 292),
    94: ("relocate", 2),
    111: ("plane shift", 239),
    112: ("gate", 205),
    113: ("resurrect", 319),
    172: ("vampiric curse", 113),
    194: ("globe of invulnerability", 172),
    237: ("moonwell", 443),
    239: ("group heal", 48),
    264: ("battle trance", 305),
    267: ("ultrablast", None),
    279: ("planar rift", 239),
    285: ("globe of darkness", 93),
    325: ("mind blank", 200),
    327: ("dragonscales", 201),
    329: ("sandstorm", None),
    330: ("inferno", 293),
    359: ("dimension shift", 239),
    363: ("shadow walk", 392),
    437: ("spirit walk", 392),
    438: ("ancestral shield", 89),
    465: ("scry remains", 294),
    477: ("nightmare", 154),
    483: ("elemental fire embodiment", 610),
    484: ("elemental earth embodiment", 611),
    504: ("time stop", 213),
    517: ("phantasmal tendrils", 174),
    521: ("song of recovery", 440),
}


def _bounded_tilde(value: str | None, context: str) -> tuple[str, list[str]]:
  """Emit a target string without exceeding the runtime's physical-line buffer."""

  text, diagnostics = convert_text(value)
  output: list[str] = []
  wrapped = 0
  for line in text.split("\n"):
    if len(line.encode("ascii")) <= 480:
      output.append(line)
      continue
    chunks = textwrap.wrap(
        line,
        width=480,
        break_long_words=True,
        break_on_hyphens=False,
        replace_whitespace=False,
        drop_whitespace=True,
    )
    output.extend(chunks or [""])
    wrapped += 1
  if wrapped:
    diagnostics.append(
        f"wrapped {wrapped} overlong {context} physical line(s) for the target reader"
    )
  return "\n".join(output) + "~\n", diagnostics


def _quest_command(
    directive: dict[str, object],
    resolve: IdentityResolver,
) -> tuple[str | None, str | None]:
  token = str(directive["token"])
  subtype = str(directive.get("subtype", ""))
  arguments = [int(value) for value in directive.get("arguments", [])]
  line = int(directive["line"])
  direction = "I" if token == "G" else "O"

  if token == "R" and subtype == "A":
    return f"{direction} A 0 0\n", None
  if not arguments:
    return None, f"excluded incomplete {token}:{subtype} quest direction at source line {line}"
  value = arguments[0]
  location = arguments[1] if len(arguments) > 1 else 0

  if token == "G" and subtype == "I":
    try:
      target = resolve("obj", value)
    except (KeyError, ValueError) as error:
      return None, f"excluded unresolved required item {value} at source line {line}: {error}"
    return f"{direction} I {target} 0\n", None
  if token == "G" and subtype == "C":
    return f"{direction} C {value} 0\n", None
  if token == "R" and subtype == "I":
    if location > value and location - value < 100:
      return (
          None,
          f"excluded random item reward {value}-{location} at source line {line}; "
          "the target HLQ item command has no range contract",
      )
    try:
      target = resolve("obj", value)
    except (KeyError, ValueError) as error:
      return None, f"excluded unresolved item reward {value} at source line {line}: {error}"
    return f"{direction} I {target} 0\n", None
  if token == "R" and subtype == "C":
    return f"{direction} C {value} 0\n", None
  if token == "R" and subtype == "E":
    return f"{direction} E {value} 0\n", None
  if token == "R" and subtype == "P":
    return f"{direction} P {value} 0\n", None
  if token == "R" and subtype == "S":
    mapped = _SOURCE_QUEST_REWARD_MAP.get(value)
    if mapped is None:
      return (
          "",
          f"omitted unmapped source spell or skill reward {value} at source line {line}",
      )
    source_name, target_spell = mapped
    if target_spell is None:
      return (
          "",
          f"omitted source-only quest reward {value} ({source_name}) at source line {line}; "
          "the target has no equivalent teachable spell contract",
      )
    diagnostic = None
    if value != target_spell:
      diagnostic = (
          f"mapped source quest reward {value} ({source_name}) to target spell "
          f"{target_spell} at source line {line}"
      )
    return f"{direction} T {target_spell} 0\n", diagnostic
  if token == "R" and subtype in {"M", "O"}:
    target_kind = "mob" if subtype == "M" else "obj"
    try:
      target_value = resolve(target_kind, value)
      target_location = resolve("wld", location) if location > 0 else 0
    except (KeyError, ValueError) as error:
      return (
          None,
          f"excluded unresolved {subtype} reward at source line {line}: {error}",
      )
    return f"{direction} {subtype} {target_value} {target_location}\n", None
  return (
      None,
      f"excluded unsupported {token}:{subtype} quest direction at source line {line}",
  )


def emit_hlquest(
    record: RolRecord,
    destination_vnum: int,
    resolve: IdentityResolver,
) -> TransformResult:
  """Compile one source quest-host block into canonical target HLQ entries."""

  diagnostics: list[str] = []
  entries: list[tuple[int, str, dict[str, object] | list[dict[str, object]]]] = []
  current_completion: list[dict[str, object]] | None = None
  resolved_host = resolve("mob", record.vnum)
  if resolved_host != destination_vnum:
    diagnostics.append(
        f"quest action destination {destination_vnum} differs from resolved host "
        f"{resolved_host}; used the resolved host identity"
    )

  for directive in record.directives:
    token = str(directive["token"])
    if token == "M":
      entries.append((int(directive["line"]), "M", directive))
    elif token == "Q":
      current_completion = [directive]
      entries.append((int(directive["line"]), "Q", current_completion))
    elif token in {"G", "R", "D"}:
      if current_completion is None:
        diagnostics.append(
            f"excluded {token} quest direction before any completion at source line "
            f"{directive['line']}"
        )
      else:
        current_completion.append(directive)

  lines = [f"#{resolved_host}\n"]
  for _, entry_type, payload in sorted(entries, key=lambda item: item[0]):
    if entry_type == "M":
      assert isinstance(payload, dict)
      keyword_value = str(payload.get("keyword", ""))
      message_value = str(payload.get("message", ""))
      if not keyword_value.strip():
        keyword_value = "unknown"
        diagnostics.append(
            f"replaced an empty quest keyword at source line {payload['line']}"
        )
      if not message_value.strip():
        message_value = "."
        diagnostics.append(
            f"replaced an empty ASK reply at source line {payload['line']}"
        )
      keyword, text_diagnostics = _bounded_tilde(keyword_value, "quest keyword")
      diagnostics.extend(text_diagnostics)
      message, text_diagnostics = _bounded_tilde(message_value, "quest reply")
      diagnostics.extend(text_diagnostics)
      lines.extend(["A!\n", keyword, message])
      continue

    assert isinstance(payload, list)
    completion = payload[0]
    inputs = [directive for directive in payload[1:] if directive["token"] == "G"]
    compiled_inputs = [_quest_command(directive, resolve) for directive in inputs]
    input_diagnostics = [item[1] for item in compiled_inputs if item[1] is not None]
    if any(command is None for command, _ in compiled_inputs):
      diagnostics.extend(str(item) for item in input_diagnostics)
      diagnostics.append(
          f"excluded completion at source line {completion['line']} because a required "
          "input cannot be staged"
      )
      continue
    reply = str(completion.get("message", ""))
    disappear_messages = [
        str(directive.get("message", ""))
        for directive in payload[1:]
        if directive["token"] == "D"
    ]
    if disappear_messages:
      reply += "".join(disappear_messages)
      diagnostics.append(
          f"folded {len(disappear_messages)} disappear message(s) into the completion "
          f"reply at source line {completion['line']}"
      )
    if not reply.strip():
      reply = "."
      diagnostics.append(
          f"replaced an empty completion reply at source line {completion['line']}"
      )
    reply_text, text_diagnostics = _bounded_tilde(reply, "quest reply")
    diagnostics.extend(text_diagnostics)
    lines.extend(["Q!\n", reply_text])

    rewards = [directive for directive in payload[1:] if directive["token"] == "R"]
    for command, command_diagnostic in compiled_inputs:
      assert command is not None
      lines.append(command)
      if command_diagnostic is not None:
        diagnostics.append(command_diagnostic)
    for directive in reversed(rewards):
      command, command_diagnostic = _quest_command(directive, resolve)
      if command is not None:
        lines.append(command)
      if command_diagnostic is not None:
        diagnostics.append(command_diagnostic)
    if disappear_messages:
      lines.append("O D 0 0\n")
    lines.append("S\n")

  return TransformResult("".join(lines), diagnostics)
