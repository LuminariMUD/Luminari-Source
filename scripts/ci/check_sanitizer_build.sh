#!/usr/bin/env bash
#
# Prove that a sanitizer job built what it claims: the intended compiler, the
# intended -fsanitize= set in the configured flags, and the matching runtime
# actually linked into each binary. A job that installs Clang but builds with
# GCC, or that drops a flag, fails here instead of reporting a clean run.
#
# usage: check_sanitizer_build.sh --cc COMPILER --sanitizers LIST
#                                 [--makefile PATH] --binary PATH [--binary PATH...]
#
# LIST is the comma-separated -fsanitize= value the build was configured with
# (for example address,undefined or thread). The Makefile check is skipped
# when --makefile names a file that does not exist (CMake builds).

set -euo pipefail

cc=
sanitizers=
makefile=Makefile
binaries=()

usage() {
  sed -n '2,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//' >&2
  exit 2
}

fail() {
  printf 'sanitizer check: %s\n' "$*" >&2
  exit 1
}

while [[ $# -gt 0 ]]; do
  case $1 in
    --cc)
      [[ $# -ge 2 ]] || usage
      cc=$2
      shift 2
      ;;
    --sanitizers)
      [[ $# -ge 2 ]] || usage
      sanitizers=$2
      shift 2
      ;;
    --makefile)
      [[ $# -ge 2 ]] || usage
      makefile=$2
      shift 2
      ;;
    --binary)
      [[ $# -ge 2 ]] || usage
      binaries+=("$2")
      shift 2
      ;;
    *) usage ;;
  esac
done
[[ -n "$cc" && -n "$sanitizers" && ${#binaries[@]} -gt 0 ]] || usage

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
"$script_dir/check_compiler.sh" --cc "$cc"

if [[ -f "$makefile" ]]; then
  for variable in CFLAGS LDFLAGS; do
    value=$(sed -n "s/^$variable = //p" "$makefile" | head -n 1)
    printf '%s: %s\n' "$variable" "$value"
    case " $value " in
      *" -fsanitize=$sanitizers "*) ;;
      *) fail "$makefile $variable lacks -fsanitize=$sanitizers" ;;
    esac
  done
fi

# Each sanitizer leaves a runtime entry point in the binary: defined when the
# runtime is linked statically (Clang), undefined plus a shared runtime in the
# dynamic section when it is linked dynamically (GCC).
runtime_symbol() {
  case $1 in
    address) echo __asan_init ;;
    undefined) echo __ubsan_handle_add_overflow ;;
    thread) echo __tsan_init ;;
    leak) echo __lsan_init ;;
    fuzzer) echo LLVMFuzzerTestOneInput ;;
    fuzzer-no-link) echo __sanitizer_cov_trace_pc_guard ;;
    *) fail "no runtime check for sanitizer '$1'" ;;
  esac
}

runtime_library() {
  case $1 in
    address) echo 'libasan|libclang_rt.asan' ;;
    undefined) echo 'libubsan|libclang_rt.ubsan' ;;
    thread) echo 'libtsan|libclang_rt.tsan' ;;
    leak) echo 'liblsan|libclang_rt.lsan' ;;
    *) echo '' ;;
  esac
}

for binary in "${binaries[@]}"; do
  [[ -f "$binary" ]] || fail "$binary does not exist"
  symbols=$(nm "$binary" 2>/dev/null || true)
  [[ -n "$symbols" ]] || symbols=$(nm -D "$binary" 2>/dev/null || true)
  libraries=$(ldd "$binary" 2>/dev/null || true)
  printf 'binary: %s\n' "$binary"
  IFS=, read -r -a wanted <<<"$sanitizers"
  for sanitizer in "${wanted[@]}"; do
    symbol=$(runtime_symbol "$sanitizer")
    if grep -qE " [TtWw] $symbol\$" <<<"$symbols"; then
      printf '  %s: %s defined (static runtime)\n' "$sanitizer" "$symbol"
      continue
    fi
    pattern=$(runtime_library "$sanitizer")
    if grep -qE " [Uu] $symbol\$" <<<"$symbols" && [[ -n "$pattern" ]] &&
      grep -qE "$pattern" <<<"$libraries"; then
      printf '  %s: %s via %s\n' "$sanitizer" "$symbol" \
        "$(grep -oE "($pattern)[^ ]*" <<<"$libraries" | head -n 1)"
      continue
    fi
    fail "$binary does not link the $sanitizer runtime ($symbol not found)"
  done
done
printf 'result: PASS\n'
