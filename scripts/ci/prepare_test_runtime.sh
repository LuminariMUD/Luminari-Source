#!/usr/bin/env bash

set -euo pipefail

script_path=$(readlink -f -- "$0")
repo_root=$(dirname "$(dirname "$(dirname "$script_path")")")
runtime_dir=${1:-}

if [[ -z "$runtime_dir" ]]; then
  printf 'Usage: %s <isolated-lib-directory>\n' "$0" >&2
  exit 2
fi

runtime_dir=$(readlink -m -- "$runtime_dir")
case "$runtime_dir" in
  /|"$repo_root"|"$repo_root/lib"|"$repo_root/lib"/*)
    printf 'Refusing to prepare a protected or broad runtime directory: %s\n' "$runtime_dir" >&2
    exit 2
    ;;
esac

required_variables=(
  LUMINARI_TEST_MYSQL_ENABLE
  LUMINARI_TEST_MYSQL_HOST
  LUMINARI_TEST_MYSQL_USER
  LUMINARI_TEST_MYSQL_PASSWORD
  LUMINARI_TEST_MYSQL_DATABASE
  LUMINARI_TEST_MYSQL_PORT
)
for variable_name in "${required_variables[@]}"; do
  if [[ -z "${!variable_name:-}" ]]; then
    printf 'Required test variable is unset: %s\n' "$variable_name" >&2
    exit 2
  fi
done

if [[ "$LUMINARI_TEST_MYSQL_ENABLE" != 1 ]]; then
  printf 'LUMINARI_TEST_MYSQL_ENABLE must be 1.\n' >&2
  exit 2
fi
if [[ "$LUMINARI_TEST_MYSQL_HOST" != localhost &&
      "$LUMINARI_TEST_MYSQL_HOST" != ::1 &&
      ! "$LUMINARI_TEST_MYSQL_HOST" =~ ^127\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
  printf 'The CI runtime preparer only accepts a loopback test database host.\n' >&2
  exit 2
fi
case "$LUMINARI_TEST_MYSQL_DATABASE" in
  *test*|*ci*) ;;
  *)
    printf 'The database name must identify an isolated test or CI database.\n' >&2
    exit 2
    ;;
esac
if [[ ! "$LUMINARI_TEST_MYSQL_DATABASE" =~ ^[A-Za-z0-9_]+$ ]] ||
   [[ ! "$LUMINARI_TEST_MYSQL_PORT" =~ ^[0-9]+$ ]]; then
  printf 'The test database name or port has an invalid format.\n' >&2
  exit 2
fi

if command -v mariadb >/dev/null 2>&1; then
  database_client=mariadb
elif command -v mysql >/dev/null 2>&1; then
  database_client=mysql
else
  printf 'MariaDB client executable is required.\n' >&2
  exit 1
fi

if command -v mariadb-admin >/dev/null 2>&1; then
  database_admin=mariadb-admin
elif command -v mysqladmin >/dev/null 2>&1; then
  database_admin=mysqladmin
else
  printf 'MariaDB administration client executable is required.\n' >&2
  exit 1
fi

database_options=(
  --protocol=tcp
  --host="$LUMINARI_TEST_MYSQL_HOST"
  --port="$LUMINARI_TEST_MYSQL_PORT"
  --user="$LUMINARI_TEST_MYSQL_USER"
)
database_ready=0
for _attempt in {1..30}; do
  if MYSQL_PWD="$LUMINARI_TEST_MYSQL_PASSWORD" \
    "$database_admin" "${database_options[@]}" ping --silent >/dev/null 2>&1; then
    database_ready=1
    break
  fi
  sleep 2
done
if [[ "$database_ready" -ne 1 ]]; then
  printf 'The isolated MariaDB service did not become ready.\n' >&2
  exit 1
fi

MYSQL_PWD="$LUMINARI_TEST_MYSQL_PASSWORD" \
  "$database_client" "${database_options[@]}" "$LUMINARI_TEST_MYSQL_DATABASE" \
  < "$repo_root/sql/master_schema.sql"
MYSQL_PWD="$LUMINARI_TEST_MYSQL_PASSWORD" \
  "$database_client" "${database_options[@]}" "$LUMINARI_TEST_MYSQL_DATABASE" <<'SQL'
INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
VALUES
  (900001, 0, 'CI encounter event fixture', 2,
   ST_GeomFromText('POLYGON((0 0,0 1,1 1,1 0,0 0))'), 0, '', '2030-01-01 00:00:00')
ON DUPLICATE KEY UPDATE
  zone_vnum = VALUES(zone_vnum),
  name = VALUES(name),
  region_type = VALUES(region_type),
  region_polygon = VALUES(region_polygon),
  region_props = VALUES(region_props),
  region_reset_data = VALUES(region_reset_data),
  region_reset_time = VALUES(region_reset_time);
SQL

umask 077
mkdir -p "$runtime_dir"/{etc,house,misc,mudmail,text/help}
mkdir -p "$runtime_dir"/plrfiles/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
mkdir -p "$runtime_dir"/plrobjs/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
mkdir -p "$runtime_dir"/world/{zon,wld,mob,obj,shp,trg,qst,hlq}

append_index_entry()
{
  local entry=$1
  local index_file=$2
  local temporary_index="${index_file}.ci-runtime"

  awk -v entry="$entry" '
    $0 == "$" && !inserted {
      print entry
      inserted = 1
    }
    { print }
    END {
      if (!inserted) {
        print entry
        print "$"
      }
    }
  ' "$index_file" > "$temporary_index"
  mv "$temporary_index" "$index_file"
}

for world_type in zon wld mob obj shp trg qst hlq; do
  cp "$repo_root/lib/world/minimal/index.$world_type" \
    "$runtime_dir/world/$world_type/index"
done
for world_type in zon wld mob obj shp trg qst hlq; do
  for source_file in "$repo_root/lib/world/minimal"/*."$world_type"; do
    [[ -e "$source_file" ]] || continue
    cp "$source_file" "$runtime_dir/world/$world_type/"
  done
done
for world_type in zon wld mob obj; do
  artifact_file="1699.$world_type"
  cp "$repo_root/lib/world/artifacts/$artifact_file" "$runtime_dir/world/$world_type/"
  append_index_entry "$artifact_file" "$runtime_dir/world/$world_type/index"
done

{
  printf 'mysql_host = %s\n' "$LUMINARI_TEST_MYSQL_HOST"
  printf 'mysql_database = %s\n' "$LUMINARI_TEST_MYSQL_DATABASE"
  printf 'mysql_username = %s\n' "$LUMINARI_TEST_MYSQL_USER"
  printf 'mysql_password = %s\n' "$LUMINARI_TEST_MYSQL_PASSWORD"
} > "$runtime_dir/mysql_config"

{
  printf 'mortal_start_room = 3001\n'
  printf 'immort_start_room = 3002\n'
  printf 'frozen_start_room = 3000\n'
} > "$runtime_dir/etc/config"

printf 'Welcome to LuminariMUD\n' > "$runtime_dir/text/news"
printf 'LuminariMUD Credits\n' > "$runtime_dir/text/credits"
printf 'Message of the Day\n' > "$runtime_dir/text/motd"
printf 'Immortal MOTD\n' > "$runtime_dir/text/imotd"
printf 'Welcome\n' > "$runtime_dir/text/greetings"
printf 'Help System\n' > "$runtime_dir/text/help/help"
printf 'Immortal Help\n' > "$runtime_dir/text/help/ihelp"
printf '$\n' > "$runtime_dir/text/help/index"
cp "$repo_root/lib/world/artifacts/artifacts.hlp" "$runtime_dir/text/help/"
append_index_entry artifacts.hlp "$runtime_dir/text/help/index"
printf 'Info\n' > "$runtime_dir/text/info"
printf 'Wizlist\n' > "$runtime_dir/text/wizlist"
printf 'Immlist\n' > "$runtime_dir/text/immlist"
printf 'Policies\n' > "$runtime_dir/text/policies"
printf 'Handbook\n' > "$runtime_dir/text/handbook"
printf 'Background\n' > "$runtime_dir/text/background"
printf '*\n' > "$runtime_dir/misc/messages"
printf '$\n' > "$runtime_dir/misc/xnames"
cat > "$runtime_dir/misc/socials.new" <<'SOCIALS'
~wave wave 0 5 0 0
You wave.
$n waves.
#
#
#
#
You wave at yourself.
$n waves at $mself.
#
#
#
#
#

$
SOCIALS

printf 'Prepared isolated test runtime at %s\n' "$runtime_dir"
