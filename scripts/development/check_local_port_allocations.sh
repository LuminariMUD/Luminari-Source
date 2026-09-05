#!/bin/bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/../.." && pwd)"
port_map="${PORT_MAP_FILE:-$repo_root/../PORT-MAP.md}"

fail()
{
  echo "FAIL: $*" >&2
  exit 1
}

require_pattern()
{
  local description=$1
  local pattern=$2
  local path=$3

  grep -Eq "$pattern" "$path" || fail "$description is not configured in $path"
}

require_reservation()
{
  local port=$1

  awk -F '|' -v expected_port="$port" '
    function trim(value) {
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", value)
      return value
    }
    trim($2) == expected_port {
      count++
      if ($3 ~ /Luminari-Source/)
        owned = 1
    }
    END { exit(count == 1 && owned ? 0 : 1) }
  ' "$port_map" ||
    fail "port $port must have one inventory row reserved for Luminari-Source in $port_map"
}

[[ -r "$port_map" ]] || fail "port inventory is not readable: $port_map"

require_reservation 3306
require_reservation 4100
require_reservation 4101
require_reservation 8081
require_reservation 8181
require_reservation 8182

require_pattern "compiled local MUD default port 4101" \
  'DFLT_PORT[[:space:]]*=[[:space:]]*4101;' "$repo_root/src/config.c"
if [[ -e "$repo_root/lib/etc/config" ]]; then
  require_pattern "runtime local MUD default port 4101" \
    '^DFLT_PORT[[:space:]]*=[[:space:]]*4101$' "$repo_root/lib/etc/config"
fi
require_pattern "autorun local MUD default port 4101" \
  'MUD_PORT:-4101' "$repo_root/scripts/autorun/autorun.sh"
require_pattern "production MUD port 4100" \
  'Environment="MUD_PORT=4100"' "$repo_root/luminari.service"
require_pattern "Discord bridge port 8181" \
  '^#define DISCORD_BRIDGE_PORT 8181$' "$repo_root/src/net/discord_bridge.h"
require_pattern "terrain API port 8182" \
  '^#define TERRAIN_API_DEFAULT_PORT 8182$' "$repo_root/src/wilderness/terrain_bridge.h"
require_pattern "MariaDB port 3306" \
  'mysql_port[[:space:]]*=[[:space:]]*3306' "$repo_root/lib/mysql_config_example"
require_pattern "local I3 gateway port 8081" \
  '^#gateway_port 8081$' "$repo_root/lib/i3_config.example"

echo "PASS: Luminari local/dev ports match the shared port inventory."
echo "      Local listeners: MUD 4101, Discord 8181, terrain/health 8182."
echo "      Dependencies: MariaDB 3306, optional shared I3 gateway 8081."
echo "      Production MUD remains explicitly assigned to 4100."
