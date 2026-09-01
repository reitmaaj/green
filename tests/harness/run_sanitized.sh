#!/bin/sh -eu
# Run the fixture suite and driver under AddressSanitizer + UBSan.
#
# The driver is sanitized natively. The green-tidy plugin is loaded into
# uninstrumented clang-tidy, so libasan/libubsan must be preloaded first.
# Any sanitizer report aborts the run (halt_on_error, error exit code).

ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/../.." && pwd)
SAN_BUILD="$ROOT/.agent/tmp/build-sanitize"
GREEN="$SAN_BUILD/green"
PLUGIN="$SAN_BUILD/libgreen-tidy.so"
TIDY=$(command -v clang-tidy)
CHECKS="-checks=-*,green-hidden-control,green-transition-boundary,green-effect-boundary,green-pure-contract,green-cast-boundary,green-null,green-declaration,green-fallthrough,green-preprocessor,green-toolchain-branching,readability-braces-around-statements"

export UBSAN_OPTIONS=halt_on_error=1

fail() {
    echo "FAIL (sanitize): $1"
    exit 1
}

[ -x "$GREEN" ] || fail "sanitized driver not built; run: just sanitize"
[ -f "$PLUGIN" ] || fail "sanitized plugin not built; run: just sanitize"

# LD_PRELOAD the actual sanitizer runtime the driver links (gcc's -print-file-name
# returns a tiny linker script; the real shared object must be preloaded so the
# asan-instrumented plugin, loaded into uninstrumented clang-tidy, sees the runtime
# first). Match only versioned runtime .so files.
RUNTIMES=$(ldd "$GREEN" | grep -oE '/[^ ]*lib(asan|ubsan)\.so\.[0-9]+' | sort -u | tr '\n' ' ')
[ -n "$RUNTIMES" ] || fail "sanitized driver links no libasan/libubsan"
export LD_PRELOAD="$RUNTIMES"
# Leak detection is handled by valgrind, not asan: the driver's uninstrumented
# subprocess compilers (gcc/cc1) inherit libasan and would otherwise emit
# LeakSanitizer noise. ASan/UBSan here focus on memory and undefined-behavior
# errors in green's own code.
export ASAN_OPTIONS=halt_on_error=1:abort_on_error=1:detect_leaks=0

# A sanitizer report is any AddressSanitizer/UBSan marker in stderr.
is_sanitizer_report() {
    grep -qE "AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|LeakSanitizer|Sanitizer" "$1"
}

status=0

# Sanitizer verification runs over lint-tagged fixtures only: matrix and
# format fixtures are not clang-tidy semantic fixtures and do not fit the
# zero-or-targeted diagnostic model.
parse_lint() {
    LINT=""
    line=$(sed -n '1p' "$1" 2>/dev/null)
    case "$line" in
    "/* green: "*)
        LINT=$(printf '%s' "$line" | sed -n 's/.*lint=\([^ ]*\).*/\1/p')
        ;;
    esac
}

# 1. pass fixtures (lint=pass): zero diagnostics, no sanitizer report
for f in "$ROOT"/tests/fixtures/pass/*.c; do
    [ -e "$f" ] || continue
    parse_lint "$f"
    [ "$LINT" = "pass" ] || continue
    if "$TIDY" -load="$PLUGIN" "$CHECKS" --extra-arg=-std=c89 "$f" 2>"$ROOT/.agent/tmp/san-pass.err" |
        grep -qE "error:|warning:"; then
        echo "FAIL pass fixture: $(basename "$f")"
        status=1
    fi
    if is_sanitizer_report "$ROOT/.agent/tmp/san-pass.err"; then
        echo "SANITIZER in pass fixture: $(basename "$f")"
        cat "$ROOT/.agent/tmp/san-pass.err"
        status=1
    fi
done

# 2. fail fixtures (lint=<check>): at least one diagnostic, no sanitizer report
for f in "$ROOT"/tests/fixtures/fail/*.c; do
    [ -e "$f" ] || continue
    parse_lint "$f"
    [ -n "$LINT" ] && [ "$LINT" != "pass" ] || continue
    if ! "$TIDY" -load="$PLUGIN" "$CHECKS" --extra-arg=-std=c89 "$f" 2>"$ROOT/.agent/tmp/san-fail.err" |
        grep -qE "error:|warning:"; then
        echo "FAIL fail fixture (no diagnostic): $(basename "$f")"
        status=1
    fi
    if is_sanitizer_report "$ROOT/.agent/tmp/san-fail.err"; then
        echo "SANITIZER in fail fixture: $(basename "$f")"
        cat "$ROOT/.agent/tmp/san-fail.err"
        status=1
    fi
done

# 3. driver end-to-end on the sample under asan/ubsan
if ! (cd "$ROOT/tests/driver/sample" && "$GREEN" check) \
    >"$ROOT/.agent/tmp/san-driver.out" 2>&1; then
    if grep -qE "AddressSanitizer|runtime error:|LeakSanitizer|Sanitizer" \
        "$ROOT/.agent/tmp/san-driver.out"; then
        echo "SANITIZER in driver check"
        cat "$ROOT/.agent/tmp/san-driver.out"
        status=1
    else
        echo "FAIL: driver check on sample project"
        cat "$ROOT/.agent/tmp/san-driver.out"
        status=1
    fi
fi

rm -f "$ROOT/.agent/tmp/san-pass.err" "$ROOT/.agent/tmp/san-fail.err" \
      "$ROOT/.agent/tmp/san-driver.out"

if [ "$status" -eq 0 ]; then
    echo "green sanitize suite: PASS"
else
    echo "green sanitize suite: FAIL"
fi
exit "$status"
