#!/usr/bin/env bash
# Format PHP files in place with the pinned php-cs-fixer release and the rules in
# .php-cs-fixer.dist.php. The php-cs-fixer pre-commit hook runs this with the
# staged file names; it needs php (8.3 on Ubuntu 24.04) on PATH.
set -euo pipefail

version=3.95.25
sha256=80cad475fc5112fdbfab8bd66e51665ed78c1b849b918dab81fb63b7a7003b41
cache="${LUMINARI_FORMATTER_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/luminari-formatters}"
phar="$cache/php-cs-fixer-$version.phar"

if ! command -v php >/dev/null 2>&1; then
  echo "php-cs-fixer hook: php is not on PATH (Ubuntu: sudo apt-get install php8.3-cli)" >&2
  exit 1
fi

# php runs any file it is given and exits 0 when that file is not a phar, which
# would pass the hook without formatting, so the phar must match the pinned
# sha256 before every run. Runs sharing the cache take turns on its lock; php
# inherits the lock, so no other run replaces the phar while it formats.
phar_matches() {
  [ -f "$phar" ] && [ "$(sha256sum <"$phar")" = "$sha256  -" ]
}

mkdir -p "$cache"
exec 9>"$cache/php-cs-fixer.lock"
flock 9
if ! phar_matches; then
  download="$(mktemp "$phar.XXXXXX")"
  trap 'rm -f "$download"' EXIT
  curl -fsSL -o "$download" \
    "https://github.com/PHP-CS-Fixer/PHP-CS-Fixer/releases/download/v$version/php-cs-fixer.phar"
  mv -f "$download" "$phar"
  if ! phar_matches; then
    rm -f "$phar"
    echo "php-cs-fixer hook: the downloaded phar does not match sha256 $sha256" >&2
    exit 1
  fi
fi

exec php "$phar" fix --config=.php-cs-fixer.dist.php --using-cache=no --quiet -- "$@"
