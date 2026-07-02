cmake_minimum_required(VERSION 3.14)
project(rsa_pki_edu CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Coverage flags
option(ENABLE_COVERAGE "Enable coverage reporting" ON)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Fetch cpp-httplib (header-only)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)
FetchContent_MakeAvailable(httplib)

# Core library (BigInteger + RSA + Education + Session + Certificate + CA + Hash + Document)
add_library(rsa_core
    src/BigInteger.cpp
    src/RSA.cpp
    src/education.cpp
    src/Session.cpp
    src/SessionHandler.cpp
    src/Certificate.cpp
    src/CAHandler.cpp
    src/Hash.cpp
    src/DocumentHandler.cpp
)
target_include_directories(rsa_core PUBLIC include)

# Main server executable
add_executable(rsa_server src/main.cpp)
target_include_directories(rsa_server PRIVATE include)
target_link_libraries(rsa_server PRIVATE rsa_core httplib::httplib)

# Tests
enable_testing()
add_executable(rsa_tests
    tests/test_biginteger.cpp
    tests/test_rsa.cpp
    tests/test_education.cpp
    tests/test_session.cpp
    tests/test_certificate.cpp
    tests/test_hash.cpp
    tests/test_document.cpp
)
target_include_directories(rsa_tests PRIVATE include)
target_link_libraries(rsa_tests PRIVATE rsa_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(rsa_tests)
