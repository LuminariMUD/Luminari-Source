#!/usr/bin/env bash
#
# Verify that a linked LuminariMUD executable was built with the production
# profile and carries its required ELF properties.  Uses only readelf so the
# same check runs on a developer machine, in deploy.sh, and in CI.  Exit
# status 1 lists every missing property; nothing is optional on the
# architectures this checks.
#
# The hardening properties alone cannot distinguish the profile from a
# distribution whose compiler defaults already include them, so the profile
# also embeds a marker section (see LUMINARI_PRODUCTION_PROFILE in
# src/constants.c) and this check requires it.

set -euo pipefail

binary=${1:?usage: verify_hardened_binary.sh EXECUTABLE}

fail()
{
  printf 'hardened binary check: %s\n' "$*" >&2
  exit 1
}

command -v readelf >/dev/null 2>&1 || fail "readelf is required"
[[ -f "$binary" ]] || fail "not a regular file: $binary"

header=$(readelf -hW "$binary") || fail "not an ELF file: $binary"
program_headers=$(readelf -lW "$binary")
dynamic=$(readelf -dW "$binary")
notes=$(readelf -nW "$binary")
dynamic_symbols=$(readelf --dyn-syms -W "$binary")

machine=$(awk -F: '/^ *Machine:/ {sub(/^ +/, "", $2); print $2; exit}' <<< "$header")
missing=()
present=()

check()
{
  local name=$1
  local ok=$2

  if [[ "$ok" == 1 ]]; then
    present+=("$name")
  else
    missing+=("$name")
  fi
}

# Profile marker: src/constants.c places this section only when the compile
# line carried the profile's LUMINARI_PRODUCTION_PROFILE definition.
profile_marker=0
if readelf -W -p .luminari.profile "$binary" 2>/dev/null | grep -q 'LuminariMUD production profile'; then
  profile_marker=1
fi
check production-profile "$profile_marker"

# PIE: a position-independent executable is ET_DYN with a program interpreter.
pie=0
if grep -q '^ *Type: *DYN' <<< "$header" && grep -q 'INTERP' <<< "$program_headers"; then
  pie=1
fi
check pie "$pie"

# Non-executable stack: PT_GNU_STACK must exist and must not carry E.
nx_stack=0
if stack_line=$(grep 'GNU_STACK' <<< "$program_headers"); then
  if ! grep -Eq 'RWE|R E' <<< "$stack_line"; then
    nx_stack=1
  fi
fi
check noexecstack "$nx_stack"

# Full RELRO: a PT_GNU_RELRO segment plus immediate binding.
relro=0
grep -q 'GNU_RELRO' <<< "$program_headers" && relro=1
check relro "$relro"

bind_now=0
if grep -Eq '\(BIND_NOW\)|\(FLAGS\).*BIND_NOW|\(FLAGS_1\).*NOW' <<< "$dynamic"; then
  bind_now=1
fi
check bind-now "$bind_now"

# Stack protector: the strong protector always references __stack_chk_fail in
# a program with local arrays, which every game source file has.
stack_protector=0
grep -q '__stack_chk_fail' <<< "$dynamic_symbols" && stack_protector=1
check stack-protector "$stack_protector"

# FORTIFY_SOURCE: fortified libc wrappers (excluding the stack protector's own
# __stack_chk_* symbols) must be referenced.
fortify=0
if grep -E '__[A-Za-z0-9_]+_chk(@|$)' <<< "$dynamic_symbols" | grep -vq '__stack_chk_'; then
  fortify=1
fi
check fortify-source "$fortify"

# Build ID: required by the versioned installer and crash symbolization.
build_id=0
grep -q 'Build ID:' <<< "$notes" && build_id=1
check build-id "$build_id"

# Control-flow protection is only verifiable on x86-64, where the linked
# property note must advertise both indirect-branch tracking and shadow stack.
if [[ "$machine" == *X86-64* ]]; then
  cf_protection=0
  if grep -q 'IBT' <<< "$notes" && grep -q 'SHSTK' <<< "$notes"; then
    cf_protection=1
  fi
  check cf-protection "$cf_protection"
fi

# Dangerous link-time properties that must be absent.
no_rpath=1
grep -Eq '\((RPATH|RUNPATH)\)' <<< "$dynamic" && no_rpath=0
check no-rpath "$no_rpath"

no_textrel=1
grep -Eq '\(TEXTREL\)|\(FLAGS\).*TEXTREL' <<< "$dynamic" && no_textrel=0
check no-textrel "$no_textrel"

printf 'hardened binary check: %s\n' "$binary"
printf '  machine: %s\n' "$machine"
printf '  present: %s\n' "${present[*]-}"
if [[ ${#missing[@]} -gt 0 ]]; then
  printf '  MISSING: %s\n' "${missing[*]}"
  fail "required production properties are missing: ${missing[*]}"
fi
printf '  result: PASS\n'
