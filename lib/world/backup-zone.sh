#!/usr/bin/env bash
# Backup a specific zone for LuminariMUD
set -euo pipefail

if [ -z "${1:-}" ]; then
  echo "Usage: $0 <zone_number>" >&2
  exit 1
fi

ZONE="$1"
DATE=$(date +%Y%m%d-%H%M)
BACKUP_DIR="lib/world/backups/${DATE}"

mkdir -p "${BACKUP_DIR}"

# Backup all file types for this zone
for ext in wld mob obj zon trg shp qst hlq; do
  if [ -f "lib/world/${ext}/${ZONE}.${ext}" ]; then
    cp "lib/world/${ext}/${ZONE}.${ext}" "${BACKUP_DIR}/${ZONE}.${ext}.bak"
    echo "Backed up ${ZONE}.${ext}"
  fi
done

echo "Zone ${ZONE} backed up to ${BACKUP_DIR}"


