#!/bin/sh -eu
# green outline smoke test: extract a multi-block function, assert the result is
# green-clean (lint + matrix + format) and that re-outlining is a no-op.
ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/../.." && pwd)
BUILD="$ROOT/.agent/tmp/build"
GREEN="$BUILD/green"
GREEN_OUTLINE="$BUILD/green-outline"
PLUGIN="$BUILD/libgreen-tidy.so"
TIDY=$(command -v clang-tidy)

WORK="$ROOT/.agent/tmp/goutline-smoke"

fail() {
    echo "FAIL: $1"
    exit 1
}

[ -x "$GREEN" ] || fail "green not built; run: just build"
[ -x "$GREEN_OUTLINE" ] || fail "green-outline not built; run: just build"
[ -f "$PLUGIN" ] || fail "plugin not built; run: just build"

rm -rf "$WORK"
mkdir -p "$WORK/src" "$WORK/build/gcc" "$WORK/build/clang"

cat > "$WORK/src/demo.c" <<'EOF'
int sum(int n)
{
    int i;
    int s;

    s = 0;
    for (i = 0; i < n; ++i)
    {
        s = s + i;
    }
    return s;
}
EOF

printf '[\n  {"directory":"%s","arguments":["clang","-std=c17","-c","src/demo.c"],"file":"%s/src/demo.c"}\n]\n' \
    "$WORK" "$WORK" > "$WORK/build/clang/compile_commands.json"
printf '[\n  {"directory":"%s","arguments":["gcc","-std=c17","-c","src/demo.c"],"file":"%s/src/demo.c"}\n]\n' \
    "$WORK" "$WORK" > "$WORK/build/gcc/compile_commands.json"

cat > "$WORK/green.yaml" <<'EOF'
version: 1

compile_commands:
    gcc: build/gcc/compile_commands.json
    clang: build/clang/compile_commands.json

project_roots:
    - src

exclude: []
compatibility_paths: []
pure_functions: []
EOF

cd "$WORK"

# Before outlining, the driver lint must report green-outline.
if "$GREEN" lint src/demo.c 2>&1 | grep -q '\[green-outline\]'; then
    :
else
    fail "un-outlined fixture did not trigger green-outline"
fi

# Outline it.
"$GREEN" outline src/demo.c >/dev/null 2>&1 || fail "green outline exited nonzero"

# The result must be clean under the full check.
(cd "$WORK" && "$GREEN" check) >/dev/null 2>&1 || fail "outlined output is not green"

# Re-outlining must be a no-op.
if "$GREEN" outline src/demo.c 2>&1 | grep -q 'already outlined'; then
    :
else
    fail "re-outline was not reported as already outlined"
fi

# The output must be green-clean under the tidy suite in both standards.
for std in c89 c23; do
    if "$TIDY" -load="$PLUGIN" \
        -checks=-*,green-hidden-control,green-transition-boundary,green-effect-boundary,green-pure-contract,green-cast-boundary,green-null,green-declaration,green-fallthrough,green-preprocessor,green-toolchain-branching,green-outline,readability-braces-around-statements \
        --extra-arg=-std="$std" src/demo.c 2>&1 | grep -qE 'error:|warning:'; then
        fail "outlined output has diagnostics ($std)"
    fi
done

rm -rf "$WORK"
echo "green outline smoke suite: PASS"
