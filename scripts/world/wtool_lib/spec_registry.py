"""Extract persisted special-procedure names from the source registry."""

from __future__ import annotations

import ast
from pathlib import Path
import re


class SpecRegistryError(ValueError):
  pass


_C_STRING = r'"(?:\\.|[^"\\])*"'


def _initializer_body(text: str, declaration: re.Match[str], label: str) -> str:
  start = declaration.end()
  body: list[str] = []
  depth = 1
  quote = ""
  escaped = False
  index = start
  while index < len(text):
    character = text[index]
    next_character = text[index + 1] if index + 1 < len(text) else ""
    if quote:
      body.append(character)
      if escaped:
        escaped = False
      elif character == "\\":
        escaped = True
      elif character == quote:
        quote = ""
    elif character in {'"', "'"}:
      quote = character
      body.append(character)
    elif character == "/" and next_character == "/":
      newline = text.find("\n", index + 2)
      if newline < 0:
        break
      body.append("\n")
      index = newline + 1
      continue
    elif character == "/" and next_character == "*":
      comment_end = text.find("*/", index + 2)
      if comment_end < 0:
        raise SpecRegistryError(f"unterminated comment in {label}")
      body.append(" ")
      index = comment_end + 2
      continue
    elif character == "{":
      depth += 1
      body.append(character)
    elif character == "}":
      depth -= 1
      if depth == 0:
        return "".join(body)
      body.append(character)
    else:
      body.append(character)
    index += 1
  raise SpecRegistryError(f"unterminated {label}")


def _decode_strings(text: str) -> set[str]:
  return {ast.literal_eval(token).casefold() for token in re.findall(_C_STRING, text)}


def extract_spec_names(repo_root: Path) -> set[str]:
  path = repo_root / "src/spec/spec_registry.c"
  text = path.read_text(encoding="utf-8")
  declaration = re.search(
      r"static\s+const\s+struct\s+spec_definition\s+spec_definitions\[\]\s*=\s*\{",
      text,
  )
  if declaration is None:
    raise SpecRegistryError("cannot find spec_definitions in src/spec/spec_registry.c")
  definitions = _initializer_body(text, declaration, "spec_definitions")
  names = {
      ast.literal_eval(token).casefold()
      for token in re.findall(rf"\.canonical_name\s*=\s*({_C_STRING})", definitions)
  }

  alias_arrays: dict[str, set[str]] = {}
  alias_pattern = re.compile(
      r"static\s+const\s+char\s+\*const\s+([A-Za-z_]\w*_aliases)\[\]\s*=\s*\{"
  )
  for alias_declaration in alias_pattern.finditer(text):
    alias_arrays[alias_declaration.group(1)] = _decode_strings(
        _initializer_body(text, alias_declaration, alias_declaration.group(1))
    )

  alias_symbols = set(
      re.findall(r"\.aliases\s*=\s*([A-Za-z_]\w*_aliases)\b", definitions)
  )
  missing_aliases = alias_symbols - alias_arrays.keys()
  if missing_aliases:
    missing = ", ".join(sorted(missing_aliases))
    raise SpecRegistryError(f"cannot find registry alias initializer(s): {missing}")
  for symbol in alias_symbols:
    names.update(alias_arrays[symbol])

  if not names:
    raise SpecRegistryError("spec_definitions contains no persisted names")
  return names
