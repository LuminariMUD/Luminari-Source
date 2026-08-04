"""Read the small subset of server configuration that changes world grammar."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any


class ConfigError(RuntimeError):
  pass


def resolve_config(world_root: Path, requested: Path | None) -> dict[str, Any]:
  if requested is not None:
    path = requested
    if not path.is_file():
      raise ConfigError(f"requested config file is inaccessible: {path}")
    assumed = False
  else:
    candidate = world_root.parent / "etc/config"
    if not candidate.is_file():
      return {
          "diagonal_dirs": False,
          "config_source": "source-default",
          "assumed": True,
      }
    path = candidate
    assumed = False

  try:
    text = path.read_text(encoding="utf-8", errors="surrogateescape")
  except OSError as error:
    raise ConfigError(f"cannot read config file {path}: {error}") from error

  value: bool | None = None
  for raw_line in text.splitlines():
    line = raw_line.strip()
    if not line or line.startswith("#") or line.startswith("*"):
      continue
    match = re.fullmatch(r"diagonal_dirs\s*=\s*([^\s#*]+).*$", line)
    if match is None:
      continue
    token = match.group(1).casefold()
    if token in {"1", "yes", "true", "on"}:
      value = True
    elif token in {"0", "no", "false", "off"}:
      value = False
    else:
      raise ConfigError(f"invalid diagonal_dirs value {match.group(1)!r} in {path}")

  if value is None:
    value = False
    assumed = True
  return {
      "diagonal_dirs": value,
      "config_source": path.name,
      "assumed": assumed,
  }
