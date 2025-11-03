#!/usr/bin/env bash
# Validate a specific zone's files and basic syntax
set -euo pipefail

ZONE=${1:-}
if [ -z "$ZONE" ]; then
  echo "Usage: $0 <zone_number>" >&2
  exit 1
fi

echo "Validating zone $ZONE..."

# Check file exists
for ext in wld mob obj zon; do
  if [ ! -f "lib/world/${ext}/${ZONE}.${ext}" ]; then
    echo "WARNING: Missing ${ZONE}.${ext}"
  fi
done

# Check for syntax
if [ -x ./bin/circle ]; then
  ./bin/circle -c -q 2>&1 | grep -Ei "error|warning" | grep -E "${ZONE}" || true
elif [ -x ./bin/circle.exe ]; then
  ./bin/circle.exe -c -q 2>&1 | grep -Ei "error|warning" | grep -E "${ZONE}" || true
else
  echo "WARNING: circle binary not found in ./bin, skipping syntax check"
fi

# Check for terminators
echo "Checking terminators..."
grep -L "~$" lib/world/*/${ZONE}.* 2>/dev/null || true

echo "Validation complete"


