#!/bin/sh -eu
# green fixture acceptance suite.
#
#   pass/*.c      each must produce zero diagnostics under clang-tidy
#   fail/*.c      each must produce at least one diagnostic
#   driver/sample a runnable project exercised via the green driver

ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/../.." && pwd)
BUILD="$ROOT/.agent/tmp/build"
GREEN="$BUILD/green"
TIDY=$(command -v clang-tidy)
PLUGIN="$BUILD/libgreen-tidy.so"
CHECKS="-checks=-*,green-hidden-control,green-transition-boundary,green-effect-boundary,green-pure-contract,green-cast-boundary,green-null,green-declaration,green-fallthrough,green-preprocessor,green-toolchain-branching,readability-braces-around-statements"

fail() {
    echo "FAIL: $1"
    exit 1
}

[ -x "$GREEN" ] || fail "driver not built; run: just build"
[ -f "$PLUGIN" ] || fail "plugin not built; run: just build"

status=0

# 1. pass fixtures: zero diagnostics
for f in "$ROOT"/tests/fixtures/pass/*.c; do
    [ -e "$f" ] || continue
    if "$TIDY" -load="$PLUGIN" "$CHECKS" --extra-arg=-std=c89 "$f" 2>/dev/null |
        grep -qE "error:|warning:"; then
        echo "FAIL pass fixture: $(basename "$f")"
        status=1
    fi
done

# 2. fail fixtures: at least one diagnostic
for f in "$ROOT"/tests/fixtures/fail/*.c; do
    [ -e "$f" ] || continue
    if ! "$TIDY" -load="$PLUGIN" "$CHECKS" --extra-arg=-std=c89 "$f" 2>/dev/null |
        grep -qE "error:|warning:"; then
        echo "FAIL fail fixture (no diagnostic): $(basename "$f")"
        status=1
    fi
done

# 3. driver end-to-end on the sample project
if ! (cd "$ROOT/tests/driver/sample" && "$GREEN" check) >/dev/null 2>&1; then
    echo "FAIL: driver check on sample project"
    status=1
fi

if [ "$status" -eq 0 ]; then
    echo "green fixture suite: PASS"
else
    echo "green fixture suite: FAIL"
fi
exit "$status"
