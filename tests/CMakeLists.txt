# green tests wired through CTest.

# Fixture acceptance suite (pass/fail fixtures + driver end-to-end).
add_test(NAME fixtures
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/harness/run_all.sh)
set_tests_properties(fixtures PROPERTIES LABELS "e2e")

# Driver smoke test: --version exits 0.
add_test(NAME smoke-version
         COMMAND $<TARGET_FILE:green> --version)
set_tests_properties(smoke-version PROPERTIES LABELS "unit")

# Doctor reports a healthy environment (tools + plugin present).
add_test(NAME doctor
         COMMAND $<TARGET_FILE:green> doctor)
set_tests_properties(doctor PROPERTIES LABELS "unit")
