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

if [ ! -f "$phar" ]; then
  mkdir -p "$cache"
  curl -fsSL -o "$phar.tmp" \
    "https://github.com/PHP-CS-Fixer/PHP-CS-Fixer/releases/download/v$version/php-cs-fixer.phar"
  echo "$sha256  $phar.tmp" | sha256sum --check --quiet
  mv "$phar.tmp" "$phar"
fi

exec php "$phar" fix --config=.php-cs-fixer.dist.php --using-cache=no --quiet -- "$@"
