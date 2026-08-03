#!/usr/bin/env bash

set -euo pipefail

# Delegate to the shared tactical acceptance harness.
script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
exec "$script_dir/test_vessel_tactical_in_game.sh" --boarding
