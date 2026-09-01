cmake_minimum_required(VERSION 3.20)
project(green VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Optional sanitizers: -DGREEN_SANITIZE=address,undefined (comma-separated).
set(GREEN_SANITIZE "" CACHE STRING
    "Sanitizer flags, e.g. address,undefined (applied to driver and plugin)")
if(GREEN_SANITIZE)
    string(REPLACE "," ";" GREEN_SANITIZE_LIST "${GREEN_SANITIZE}")
    foreach(SAN ${GREEN_SANITIZE_LIST})
        add_compile_options(-fsanitize=${SAN} -fno-omit-frame-pointer)
        add_link_options(-fsanitize=${SAN})
    endforeach()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -g")
endif()

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "Clang version: ${CLANG_PACKAGE_VERSION}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_definitions(-D_GNU_SOURCE)

set(CLANG_MAJOR ${LLVM_VERSION_MAJOR})

# Fedora/LLVM 21 ships a monolithic libclang-cpp.so and libLLVM.so.
link_directories(${LLVM_LIBRARY_DIR})
set(GREEN_CLANG_LIBS clang-cpp LLVM)

# --- green-tidy plugin (loadable module) ------------------------------------
add_library(green-tidy MODULE src/tidy/CgreenModule.cc src/tidy/GreenChecks.cc)
target_link_libraries(green-tidy PRIVATE ${GREEN_CLANG_LIBS})
target_compile_definitions(green-tidy PRIVATE
    GREEN_CLANG_VERSION="${CLANG_VERSION_STRING}"
    GREEN_LLVM_VERSION="${LLVM_PACKAGE_VERSION}")

# --- green driver -----------------------------------------------------------
add_executable(green
    src/driver/main.cc
    src/driver/Config.cc
    src/driver/CompileDB.cc
    src/driver/Matrix.cc
    src/driver/Process.cc
    src/driver/Format.cc
    src/driver/Fix.cc
    src/driver/Doctor.cc
    src/driver/Driver.cc)
target_link_libraries(green PRIVATE ${GREEN_CLANG_LIBS})
target_compile_definitions(green PRIVATE
    GREEN_CLANG_VERSION="${CLANG_VERSION_STRING}"
    GREEN_LLVM_VERSION="${LLVM_PACKAGE_VERSION}"
    GREEN_PLUGIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/libgreen-tidy${CMAKE_SHARED_LIBRARY_SUFFIX}"
    GREEN_FORMAT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/share/green/clang-format.yaml")
set_target_properties(green PROPERTIES
    BUILD_RPATH "${LLVM_LIBRARY_DIR}" INSTALL_RPATH "${LLVM_LIBRARY_DIR}")
set_target_properties(green-tidy PROPERTIES
    BUILD_RPATH "${LLVM_LIBRARY_DIR}" INSTALL_RPATH "${LLVM_LIBRARY_DIR}")

# --- unit tests -------------------------------------------------------------
enable_testing()
add_subdirectory(tests)

install(TARGETS green RUNTIME DESTINATION bin)
install(TARGETS green-tidy LIBRARY DESTINATION lib/green/${CLANG_MAJOR})
install(FILES share/green/clang-format.yaml
              share/green/default-config.yaml
        DESTINATION share/green)
