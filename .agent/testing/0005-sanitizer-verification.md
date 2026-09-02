# Testing: sanitizer verification of green itself

SCENARIO asan/ubsan pass over fixtures
GIVEN `green` and `green-tidy` built with `-DGREEN_SANITIZE=address,undefined`
WHEN `just sanitize` runs the pass and fail fixture suites and the driver
     `check` on the sample project
THEN no AddressSanitizer or UBSan report is emitted and the suite exits 0

SCENARIO plugin is verified under sanitizers despite an uninstrumented host
GIVEN clang-tidy is not sanitized but the plugin is
WHEN clang-tidy loads the plugin
THEN libasan/libubsan are preloaded (LD_PRELOAD) so the runtime is first
     and sanitizer reports in plugin code are still caught

SCENARIO leak detection is delegated to valgrind
GIVEN the driver spawns uninstrumented subprocess compilers (gcc/cc1)
WHEN `green check` runs under ASan
THEN LeakSanitizer is disabled (detect_leaks=0) to avoid noise from
     unrelated processes

SCENARIO valgrind degrades gracefully on this toolchain
GIVEN valgrind hangs while loading the machine's monolithic libclang-cpp.so
WHEN `just valgrind` runs its capability probe
THEN it reports UNSUPPORTED and exits 0, pointing to `just sanitize`

SCENARIO a defect surfaced by a sanitizer is fixed by reproduction
GIVEN ASan/UBSan reports a real defect in green code
WHEN fixing
THEN a failing reproduction fixture and acceptance/BDD doc are added first,
     then the defect is fixed, then the reproduction and full suite pass

SCENARIO every semantic check is exercised under sanitizers
GIVEN the sanitizer harness mirrors the e2e semantic check set
WHEN the run_sanitized.sh CHECKS list omits one registered green-* check
     (e.g. green-flat) that the e2e suite exercises
THEN green-flat fail fixtures report "no diagnostic" and the suite fails;
     the omission is a defect fixed by adding the check to the CHECKS list
