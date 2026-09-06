#!/bin/bash
#
# Regenerate the builder guide pages under docs/web/guides/ from their Markdown
# sources. The Markdown files are authoritative; the HTML is build output and
# must never be hand-edited.
#
# Usage:
#   ./scripts/development/generate-web-guides.sh          # regenerate
#   ./scripts/development/generate-web-guides.sh --check   # fail if stale
#
# Run from the project root. Requires pandoc (apt install pandoc).

set -euo pipefail

TEMPLATE="docs/web/assets/pandoc-template.html"
OUTDIR="docs/web/guides"

# source_markdown|output_basename|title|subtitle
GUIDES=(
  "docs/world_game-data/OEDIT_GUIDE.md|oedit.html|OEDIT Guide for Builders|Complete reference for the object editor"
  "docs/world_game-data/MOB_FLAGS.md|mob_flags.html|MOB Flags Reference|Complete reference for mobile behavior flags"
  "docs/world_game-data/ROOM_FLAGS.md|room_flags.html|Room Flags Reference|Complete reference for room behavior flags"
)

CHECK_ONLY=0
if [ "${1:-}" = "--check" ]; then
  CHECK_ONLY=1
fi

if ! command -v pandoc >/dev/null 2>&1; then
  echo "error: pandoc is not installed (apt install pandoc)" >&2
  exit 2
fi

if [ ! -f "$TEMPLATE" ]; then
  echo "error: $TEMPLATE not found - run from the project root" >&2
  exit 2
fi

stale=0
for entry in "${GUIDES[@]}"; do
  IFS='|' read -r source basename title subtitle <<< "$entry"
  target="$OUTDIR/$basename"

  if [ ! -f "$source" ]; then
    echo "error: missing source $source" >&2
    exit 2
  fi

  tmp="$(mktemp)"
  pandoc "$source" \
    -f markdown -t html \
    --ascii \
    -o "$tmp" \
    --template="$TEMPLATE" \
    --metadata title="$title" \
    --metadata subtitle="$subtitle" \
    --toc --toc-depth=2

  # Strip trailing whitespace so the output matches what the repository's
  # pre-commit hook would produce. Without this the hook rewrites the file
  # after generation and --check then reports it stale forever.
  sed -i 's/[[:space:]]*$//' "$tmp"

  if [ "$CHECK_ONLY" -eq 1 ]; then
    if ! cmp -s "$tmp" "$target"; then
      echo "stale: $target does not match $source"
      stale=1
    fi
    rm -f "$tmp"
  else
    mv "$tmp" "$target"
    chmod 644 "$target"
    echo "wrote $target"
  fi
done

if [ "$CHECK_ONLY" -eq 1 ]; then
  if [ "$stale" -ne 0 ]; then
    echo "" >&2
    echo "Run ./scripts/development/generate-web-guides.sh to regenerate." >&2
    exit 1
  fi
  echo "all guide pages are current"
fi
