set shell := ["sh", "-eu", "-c"]

BUILD := ".agent/tmp/build"
BUILD_SAN := ".agent/tmp/build-sanitize"

default:
    @just --list

# Configure the CMake build
setup:
    cmake -S . -B {{BUILD}} -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Build the driver and the green-tidy plugin
build: setup
    cmake --build {{BUILD}} -j

# Run the CTest unit tests
unit: build
    ctest --test-dir {{BUILD}} -L unit --output-on-failure

# Run the fixture acceptance suite
e2e: build
    tests/harness/run_all.sh

# Run every test
test: unit e2e

# Build with AddressSanitizer + UndefinedBehaviorSanitizer and run the suite.
sanitize:
    cmake -S . -B {{BUILD_SAN}} -DGREEN_SANITIZE=address,undefined
    cmake --build {{BUILD_SAN}} -j
    tests/harness/run_sanitized.sh

# Run the driver and plugin under valgrind (memcheck).
valgrind:
    tests/harness/run_valgrind.sh

# Lint shell scripts and check C++ formatting
lint:
    @for f in $(find tests scripts -name '*.sh' 2>/dev/null); do \
        shellcheck -s sh "$f" && shellcheck -s bash "$f"; done
    @for f in $(find src include -name '*.cc' -o -name '*.h' 2>/dev/null | \
        grep -v '/bblift/'); do \
        clang-format --dry-run --Werror "$f" || { echo "needs formatting: $f"; exit 1; }; done

# Format all C++ sources (vendored outline core keeps its upstream style)
format:
    @for f in $(find src include -name '*.cc' -o -name '*.h' 2>/dev/null | \
        grep -v '/bblift/'); do \
        clang-format -i "$f"; done

# Build, run all tests, and lint
check: test lint

# Run the driver with arbitrary arguments
run ARGS:
    ./{{BUILD}}/green {{ARGS}}

# Remove the build directory
clean:
    rm -rf {{BUILD}}
