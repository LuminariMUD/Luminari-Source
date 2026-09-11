#!/usr/bin/env bash
#
# Regression coverage for the production build profile contract:
#   - production_profile.sh emits the four output keys and a usable flag set;
#   - a program built with those flags passes verify_hardened_binary.sh;
#   - the same program built with the compiler's default flags fails it, so
#     distribution defaults cannot masquerade as the repository profile;
#   - verify_hardened_binary.sh names every property a degraded build lacks.

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
profile="$project_root/scripts/deployment/production_profile.sh"
verify="$project_root/scripts/deployment/verify_hardened_binary.sh"
cc=${CC:-cc}
test_root=$(mktemp -d "${TMPDIR:-/tmp}/luminari-production-profile-test.XXXXXX")

fail()
{
  printf 'production profile test: %s\n' "$*" >&2
  exit 1
}

cleanup()
{
  if [[ -d "$test_root" ]] && [[ $(basename "$test_root") == luminari-production-profile-test.* ]]; then
    rm -rf -- "$test_root"
  fi
}
trap cleanup EXIT

field()
{
  awk -F= -v key="$2" '$1 == key {print substr($0, index($0, "=") + 1); exit}' <<< "$1"
}

# The helper mirrors src/constants.c: the marker section exists only when the
# profile's LUMINARI_PRODUCTION_PROFILE definition reaches the compile line.
cat > "$test_root/helper.c" <<'HELPER'
#include <stdio.h>
#include <string.h>

#ifdef LUMINARI_PRODUCTION_PROFILE
const char helper_profile[] __attribute__((used, section(".luminari.profile"))) =
  "LuminariMUD production profile";
#endif

int main(int argc, char **argv)
{
  char buffer[64];

  snprintf(buffer, sizeof(buffer), "%d", argc);
  return argv[0] != NULL && strlen(buffer) > 0 ? 0 : 1;
}
HELPER

# 1. The probe rejects unknown options and a missing compiler.
if "$profile" --cc "$cc" --bogus >/dev/null 2>&1; then
  fail "probe accepted an unknown option"
fi
if "$profile" --production >/dev/null 2>&1; then
  fail "probe ran without --cc"
fi
if "$profile" --cc "$cc" --pgo-use "$test_root/missing-profile" >/dev/null 2>&1; then
  fail "probe accepted a missing PGO profile path"
fi

# 2. The production probe emits the contract keys and the optimization policy.
output=$("$profile" --cc "$cc" --production) || fail "production probe failed for $cc"
for key in PRODUCTION_CFLAGS PRODUCTION_LDFLAGS PRODUCTION_SUPPORTED PRODUCTION_UNSUPPORTED; do
  grep -q "^$key=" <<< "$output" || fail "probe output lacks $key"
done
cflags=$(field "$output" PRODUCTION_CFLAGS)
ldflags=$(field "$output" PRODUCTION_LDFLAGS)
supported=$(field "$output" PRODUCTION_SUPPORTED)
[[ " $cflags " == *" -O2 "* ]] || fail "production CFLAGS do not select -O2: $cflags"
[[ " $cflags " == *" -g "* ]] || fail "production CFLAGS do not retain debug symbols: $cflags"
[[ " $cflags " != *"NDEBUG"* ]] || fail "production CFLAGS must keep assertions enabled"
[[ -n "$supported" ]] || fail "probe reported no supported hardening features"

# 3. Without --production the probe adds nothing.
empty=$("$profile" --cc "$cc") || fail "bare probe failed"
[[ -z "$(field "$empty" PRODUCTION_CFLAGS)" ]] || fail "bare probe emitted CFLAGS"
[[ -z "$(field "$empty" PRODUCTION_LDFLAGS)" ]] || fail "bare probe emitted LDFLAGS"

# 4. A program built with the profile passes the ELF verifier.
# shellcheck disable=SC2086
"$cc" $cflags $ldflags -o "$test_root/hardened" "$test_root/helper.c" ||
  fail "could not build the hardened helper"
"$verify" "$test_root/hardened" >"$test_root/hardened.log" 2>&1 ||
  fail "hardened helper failed verification: $(cat "$test_root/hardened.log")"
grep -q 'result: PASS' "$test_root/hardened.log" || fail "verifier did not report PASS"

# 5. The verifier rejects the same program built with default flags, whatever
#    hardening the distribution toolchain applies on its own.
"$cc" -O2 -g -o "$test_root/default" "$test_root/helper.c" ||
  fail "could not build the default helper"
if "$verify" "$test_root/default" >"$test_root/default.log" 2>&1; then
  fail "verifier accepted a binary built without the production profile"
fi
grep -q 'MISSING:.*\bproduction-profile\b' "$test_root/default.log" ||
  fail "verifier did not report the missing profile marker: $(cat "$test_root/default.log")"

# 6. The verifier rejects a degraded build and names each missing property.
"$cc" -O2 -g -fno-PIE -no-pie -fno-stack-protector -U_FORTIFY_SOURCE \
  -Wl,-z,norelro -Wl,-z,lazy -Wl,-z,execstack -Wl,-rpath,/opt/luminari-test \
  -o "$test_root/degraded" "$test_root/helper.c" ||
  fail "could not build the degraded helper"
if "$verify" "$test_root/degraded" >"$test_root/degraded.log" 2>&1; then
  fail "verifier accepted a degraded binary"
fi
for property in production-profile pie noexecstack relro bind-now stack-protector \
  fortify-source no-rpath; do
  grep -q "MISSING:.*\b$property\b" "$test_root/degraded.log" ||
    fail "verifier did not report missing $property: $(cat "$test_root/degraded.log")"
done

# 7. The verifier refuses non-ELF input.
if "$verify" "$test_root/helper.c" >/dev/null 2>&1; then
  fail "verifier accepted a non-ELF file"
fi

printf 'production profile test: PASS (%s: %s)\n' "$cc" "$supported"
