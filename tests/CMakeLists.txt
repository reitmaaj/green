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

# Config parser unit test.
add_executable(green-config-test
    config/ConfigTest.cc
    ../src/driver/Config.cc)
target_include_directories(green-config-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(green-config-test PRIVATE ${GREEN_CLANG_LIBS})
add_test(NAME config
         COMMAND $<TARGET_FILE:green-config-test>)
set_tests_properties(config PROPERTIES LABELS "unit")
