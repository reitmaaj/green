#!/bin/sh -eu
# green e2e fixture acceptance suite.
#
# Each fixture declares its expected outcome per checked dimension in a
# metadata header comment:
#
#   /* green: lint=pass matrix=pass format=pass */
#   /* green: lint=green-hidden-control matrix=pass format=pass */
#   /* green: lint=pass matrix=c23 format=pass */
#   /* green: lint=pass matrix=pass format=fail */
#
#   lint   =pass        no clang-tidy diagnostics under C89 and C23
#   lint   =<check>     exactly that check fires under C89 and C23
#   matrix =pass        gcc+clang, C89+C23, all compile clean with the green
#                       baseline (-fsyntax-only)
#   matrix =c89|c23     that standard's two cells fail; the other passes
#   format =pass        canonical clang-format output (dry-run, no diff)
#   format =fail        clang-format output differs
#
# A dimension is asserted only when tagged. Pass fixtures tag all three.

ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/../.." && pwd)
BUILD="$ROOT/.agent/tmp/build"
GREEN="$BUILD/green"
PLUGIN="$BUILD/libgreen-tidy.so"
TIDY=$(command -v clang-tidy)
GCC=$(command -v gcc)
CLANG=$(command -v clang)
CLANG_FORMAT=$(command -v clang-format)
PROFILE="$ROOT/share/green/clang-format.yaml"
CHECKS="-checks=-*,green-hidden-control,green-transition-boundary,green-effect-boundary,green-pure-contract,green-cast-boundary,green-null,green-declaration,green-fallthrough,green-preprocessor,green-toolchain-branching,green-outline,readability-braces-around-statements"
BASELINE="-pedantic-errors -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition -Wundef -Wshadow -Wformat=2 -Wcast-qual -fsyntax-only"

fail() {
    echo "FAIL: $1"
    exit 1
}

[ -f "$PLUGIN" ] || fail "plugin not built; run: just build"
[ -x "$GREEN" ] || fail "driver not built; run: just build"

# Parse a fixture's metadata header. Sets: LINT MATRIX FORMAT.
parse_header() {
    LINT=""
    MATRIX=""
    FORMAT=""
    line=$(sed -n '1p' "$1" 2>/dev/null)
    case "$line" in
    "/* green: "*)
        LINT=$(printf '%s' "$line" | sed -n 's/.*lint=\([^ ]*\).*/\1/p')
        MATRIX=$(printf '%s' "$line" | sed -n 's/.*matrix=\([^ ]*\).*/\1/p')
        FORMAT=$(printf '%s' "$line" | sed -n 's/.*format=\([^ ]*\).*/\1/p')
        ;;
    esac
}

# clang-tidy under a given standard; emits diagnostics to stdout.
lint_std() {
    "$TIDY" -load="$PLUGIN" "$CHECKS" --extra-arg=-std="$1" "$2" 2>&1 || true
}

assert_lint() {
    case "$LINT" in
    pass)
        if lint_std c89 "$1" | grep -qE "error:|warning:" ||
            lint_std c23 "$1" | grep -qE "error:|warning:"; then
            fail "lint=pass fixture has diagnostics: $(basename "$1")"
        fi
        ;;
    green-*)
        if ! lint_std c89 "$1" | grep -q "\[$LINT\]" ||
            ! lint_std c23 "$1" | grep -q "\[$LINT\]"; then
            fail "lint=$LINT did not fire in both modes: $(basename "$1")"
        fi
        ;;
    esac
}

# Run one compiler cell; exit 0 when clean.
matrix_cell() {
    # shellcheck disable=SC2086  # BASELINE is intentionally split into flags
    "$1" "$3" $BASELINE -std="$2" -fsyntax-only
}

assert_matrix() {
    case "$MATRIX" in
    pass)
        for std in c89 c23; do
            for cc in "$GCC" "$CLANG"; do
                if ! matrix_cell "$cc" "$std" "$1" >/dev/null 2>&1; then
                    fail "matrix=pass failed ($cc $std): $(basename "$1")"
                fi
            done
        done
        ;;
    c89 | c23)
        other=""
        [ "$MATRIX" = "c89" ] && other="c23" || other="c89"
        for cc in "$GCC" "$CLANG"; do
            if matrix_cell "$cc" "$MATRIX" "$1" >/dev/null 2>&1; then
                fail "matrix=$MATRIX cell unexpectedly passed ($cc): $(basename "$1")"
            fi
            if ! matrix_cell "$cc" "$other" "$1" >/dev/null 2>&1; then
                fail "matrix=$MATRIX other standard ($other) failed ($cc): $(basename "$1")"
            fi
        done
        ;;
    esac
}

assert_format() {
    case "$FORMAT" in
    pass)
        if ! "$CLANG_FORMAT" --style="file:$PROFILE" --dry-run --Werror "$1" \
            >/dev/null 2>&1; then
            fail "format=pass fixture is not canonical: $(basename "$1")"
        fi
        ;;
    fail)
        if "$CLANG_FORMAT" --style="file:$PROFILE" --dry-run --Werror "$1" \
            >/dev/null 2>&1; then
            fail "format=fail fixture is canonical: $(basename "$1")"
        fi
        ;;
    esac
}

status=0
check_fixture() {
    parse_header "$1"
    [ -n "$LINT$MATRIX$FORMAT" ] || return 0
    assert_lint "$1" || status=1
    assert_matrix "$1" || status=1
    assert_format "$1" || status=1
}

count=0
for dir in pass fail; do
    for f in "$ROOT"/tests/fixtures/"$dir"/*.c; do
        [ -e "$f" ] || continue
        check_fixture "$f"
        count=$((count + 1))
    done
done

# Final: driver end-to-end on the sample project.
if ! (cd "$ROOT/tests/driver/sample" && "$GREEN" check) >/dev/null 2>&1; then
    echo "FAIL: driver check on sample project"
    status=1
fi

# Outline smoke test (transform + green-clean + idempotence).
if ! "$ROOT/tests/harness/run_outline.sh" >/dev/null 2>&1; then
    echo "FAIL: green outline smoke suite"
    status=1
fi

echo "green e2e fixtures checked: $count"
if [ "$status" -eq 0 ]; then
    echo "green e2e suite: PASS"
else
    echo "green e2e suite: FAIL"
fi
exit "$status"
