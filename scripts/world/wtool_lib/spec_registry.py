"""Extract persisted special-procedure names from the source registry."""

from __future__ import annotations

import ast
from pathlib import Path
import re


class SpecRegistryError(ValueError):
  pass


def extract_spec_names(repo_root: Path) -> set[str]:
  path = repo_root / "src/spec_assign.c"
  text = path.read_text(encoding="utf-8")
  declaration = re.search(r"static\s+const\s+struct\s+spec_func_data\s+spec_func_list\[\]\s*=\s*\{", text)
  if declaration is None:
    raise SpecRegistryError("cannot find spec_func_list in src/spec_assign.c")
  end = text.find("};", declaration.end())
  if end < 0:
    raise SpecRegistryError("unterminated spec_func_list in src/spec_assign.c")
  names: set[str] = set()
  for token in re.findall(r'\{\s*("(?:\\.|[^"\\])*")\s*,', text[declaration.end() : end]):
    value = ast.literal_eval(token)
    if value != "\n":
      names.add(value.casefold())
  if not names:
    raise SpecRegistryError("spec_func_list contains no persisted names")
  return names
