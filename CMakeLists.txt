cmake_minimum_required(VERSION 3.20)
project(checkpp VERSION 0.1.4 LANGUAGES C CXX)

option(CHECKPP_STRIP_SYMBOLS "Strip symbols from executable" OFF)
option(CHECKPP_LTO "Enable Link-Time Optimization" OFF)
option(CHECKPP_EMBED_CLANG_TIDY_MODULE "Embed the clang-tidy module into the executable" OFF)
option(CHECKPP_ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)
option(CHECKPP_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(CHECKPP_ENABLE_COVERAGE "Enable code coverage" OFF)
option(CHECKPP_ENABLE_STATIC_ANALYSIS "Enable static analysis tools" OFF)
option(CHECKPP_ENABLE_FUZZ "Enable fuzzing instrumentation" OFF)
option(CHECKPP_BUILD_DEB "Enable .deb package generation with CPack" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CHECKPP_WARNINGS_AS_ERRORS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-Werror)
    endif()
endif()

if(CHECKPP_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        add_link_options(-fsanitize=address,undefined)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all -fno-sanitize=float-divide-by-zero -fno-sanitize=float-cast-overflow)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

if(CHECKPP_ENABLE_COVERAGE)
    add_compile_options(--coverage -fno-inline)
    add_link_options(--coverage)
endif()

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
find_package(yaml-cpp REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CLANG_CMAKE_DIR}")
include(AddLLVM)
include(AddClang)
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(clang-tidy-module)

add_executable(checkpp
    src/main.cpp
    src/config.cpp
    src/embedded_clang_tidy_module.cpp
    src/runner.cpp
)

target_include_directories(checkpp PRIVATE src/inc ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(checkpp PRIVATE CHECKPP_VERSION="${PROJECT_VERSION}")

if(CHECKPP_EMBED_CLANG_TIDY_MODULE)
    target_compile_definitions(checkpp PRIVATE CHECKPP_EMBED_CLANG_TIDY_MODULE)
    set(CHECKPP_EMBEDDED_CLANG_TIDY_MODULE_DATA ${CMAKE_CURRENT_BINARY_DIR}/embedded_clang_tidy_module_data.inc)
    add_custom_command(
        OUTPUT ${CHECKPP_EMBEDDED_CLANG_TIDY_MODULE_DATA}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary_to_cpp.py
                --input $<TARGET_FILE:CompanyClangTidyModule>
                --output ${CHECKPP_EMBEDDED_CLANG_TIDY_MODULE_DATA}
                --namespace checkpp::embedded_clang_tidy_module
                --data-name kEmbeddedClangTidyModule
        DEPENDS CompanyClangTidyModule ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary_to_cpp.py
        VERBATIM
    )
    add_custom_target(checkpp_embedded_clang_tidy_module DEPENDS ${CHECKPP_EMBEDDED_CLANG_TIDY_MODULE_DATA})
    add_dependencies(checkpp checkpp_embedded_clang_tidy_module)
endif()

if(TARGET yaml-cpp::yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
elseif(TARGET yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp)
else()
    message(FATAL_ERROR "yaml-cpp was found, but no usable CMake target is available")
endif()

target_link_libraries(checkpp
    PRIVATE
    ${YAML_CPP_TARGET}
)

if(CHECKPP_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set_target_properties(checkpp PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "IPO / LTO enabled")
    else()
        message(WARNING "IPO not supported: ${ipo_error}")
    endif()
endif()

if(CHECKPP_STRIP_SYMBOLS)
    add_custom_command(TARGET checkpp POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --strip-all $<TARGET_FILE:checkpp> ${CMAKE_CURRENT_BINARY_DIR}/checkpp-stripped
        COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/checkpp-stripped $<TARGET_FILE:checkpp>
        COMMENT "Stripping symbols from executable"
    )
endif()

include(GNUInstallDirs)

install(TARGETS checkpp
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CTest)

if(BUILD_TESTING)
    set(CHECKPP_RULES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config/rules_c_family_cpp.yaml")
    set(CHECKPP_IGNORE_PATHS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/sw_example/fixtures/ignore_paths_for_tests.txt")
    set(CHECKPP_ASSERT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunCheckppAndAssert.cmake")
    set(CHECKPP_FIXTURES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/sw_example/fixtures")
    set(CHECKPP_PASS_FIXTURE_SOURCE "${CHECKPP_FIXTURES_ROOT}/company_rules_pass")
    set(CHECKPP_FAIL_FIXTURE_SOURCE "${CHECKPP_FIXTURES_ROOT}/company_rules_fail")
    set(CHECKPP_INTENTIONAL_FAIL_FIXTURE_SOURCE "${CHECKPP_FIXTURES_ROOT}/intentional_fail_editable")
    set(CHECKPP_PASS_FIXTURE_BUILD "${CMAKE_CURRENT_BINARY_DIR}/tests/company_rules_pass")
    set(CHECKPP_FAIL_FIXTURE_BUILD "${CMAKE_CURRENT_BINARY_DIR}/tests/company_rules_fail")
    set(CHECKPP_INTENTIONAL_FAIL_FIXTURE_BUILD "${CMAKE_CURRENT_BINARY_DIR}/tests/intentional_fail_editable")

    add_test(
        NAME checkpp_fixture_pass_configure
        COMMAND ${CMAKE_COMMAND}
                -S ${CHECKPP_PASS_FIXTURE_SOURCE}
                -B ${CHECKPP_PASS_FIXTURE_BUILD}
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    )
    add_test(
        NAME checkpp_fixture_pass_build
        COMMAND ${CMAKE_COMMAND} --build ${CHECKPP_PASS_FIXTURE_BUILD}
    )
    set_tests_properties(checkpp_fixture_pass_build PROPERTIES DEPENDS checkpp_fixture_pass_configure)

    add_test(
        NAME checkpp_no_error_case
        COMMAND ${CMAKE_COMMAND}
                -DCHECKPP_BIN=$<TARGET_FILE:checkpp>
                -DIGNORE_PATHS_FILE=${CHECKPP_IGNORE_PATHS_FILE}
                -DPROJECT_ROOT=${CHECKPP_PASS_FIXTURE_SOURCE}
                -DCOMPILE_DB_DIR=${CHECKPP_PASS_FIXTURE_BUILD}
                -DRULES_FILE=${CHECKPP_RULES_FILE}
                -DEXPECT_EXIT_CODE=0
                -DEXPECT_OUTPUT_REGEX=Errors:.*0
                -P ${CHECKPP_ASSERT_SCRIPT}
    )
    set_tests_properties(checkpp_no_error_case PROPERTIES DEPENDS checkpp_fixture_pass_build)

    add_test(
        NAME checkpp_fixture_fail_configure
        COMMAND ${CMAKE_COMMAND}
                -S ${CHECKPP_FAIL_FIXTURE_SOURCE}
                -B ${CHECKPP_FAIL_FIXTURE_BUILD}
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    )
    add_test(
        NAME checkpp_fixture_fail_build
        COMMAND ${CMAKE_COMMAND} --build ${CHECKPP_FAIL_FIXTURE_BUILD}
    )
    set_tests_properties(checkpp_fixture_fail_build PROPERTIES DEPENDS checkpp_fixture_fail_configure)

    add_test(
        NAME checkpp_error_case
        COMMAND ${CMAKE_COMMAND}
                -DCHECKPP_BIN=$<TARGET_FILE:checkpp>
                -DIGNORE_PATHS_FILE=${CHECKPP_IGNORE_PATHS_FILE}
                -DPROJECT_ROOT=${CHECKPP_FAIL_FIXTURE_SOURCE}
                -DCOMPILE_DB_DIR=${CHECKPP_FAIL_FIXTURE_BUILD}
                -DRULES_FILE=${CHECKPP_RULES_FILE}
                -DEXPECT_EXIT_CODE=2
                -DEXPECT_OUTPUT_REGEX=Errors:.*[1-9][0-9]*
                -P ${CHECKPP_ASSERT_SCRIPT}
    )
    set_tests_properties(checkpp_error_case PROPERTIES DEPENDS checkpp_fixture_fail_build)

    add_test(
        NAME checkpp_error_message_case
        COMMAND ${CMAKE_COMMAND}
                -DCHECKPP_BIN=$<TARGET_FILE:checkpp>
                -DIGNORE_PATHS_FILE=${CHECKPP_IGNORE_PATHS_FILE}
                -DPROJECT_ROOT=${CHECKPP_FAIL_FIXTURE_SOURCE}
                -DCOMPILE_DB_DIR=${CHECKPP_FAIL_FIXTURE_BUILD}
                -DRULES_FILE=${CHECKPP_RULES_FILE}
                -DEXPECT_EXIT_CODE=2
                -DEXPECT_OUTPUT_REGEX=Rule 1\.1: file 'BadMain\.cpp' should use snake_case
                -P ${CHECKPP_ASSERT_SCRIPT}
    )
    set_tests_properties(checkpp_error_message_case PROPERTIES DEPENDS checkpp_fixture_fail_build)

    add_test(
        NAME checkpp_fixture_intentional_fail_configure
        COMMAND ${CMAKE_COMMAND}
                -S ${CHECKPP_INTENTIONAL_FAIL_FIXTURE_SOURCE}
                -B ${CHECKPP_INTENTIONAL_FAIL_FIXTURE_BUILD}
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    )
    add_test(
        NAME checkpp_fixture_intentional_fail_build
        COMMAND ${CMAKE_COMMAND} --build ${CHECKPP_INTENTIONAL_FAIL_FIXTURE_BUILD}
    )
    set_tests_properties(checkpp_fixture_intentional_fail_build PROPERTIES DEPENDS checkpp_fixture_intentional_fail_configure)

    add_test(
        NAME checkpp_intentional_fail_editable_case
        COMMAND ${CMAKE_COMMAND}
                -DCHECKPP_BIN=$<TARGET_FILE:checkpp>
                -DIGNORE_PATHS_FILE=${CHECKPP_IGNORE_PATHS_FILE}
                -DPROJECT_ROOT=${CHECKPP_INTENTIONAL_FAIL_FIXTURE_SOURCE}
                -DCOMPILE_DB_DIR=${CHECKPP_INTENTIONAL_FAIL_FIXTURE_BUILD}
                -DRULES_FILE=${CHECKPP_RULES_FILE}
                -DEXPECT_EXIT_CODE=2
                -DEXPECT_OUTPUT_REGEX=Rule 1\.1: file 'EditMeFail\.cpp' should use snake_case
                -P ${CHECKPP_ASSERT_SCRIPT}
    )
    set_tests_properties(checkpp_intentional_fail_editable_case PROPERTIES DEPENDS checkpp_fixture_intentional_fail_build)
endif()

if(CHECKPP_BUILD_DEB)
    message(STATUS "🧱 Creating .deb packages with CPack is enabled.")
    set(CPACK_GENERATOR "DEB")

    set(CPACK_PACKAGE_NAME "checkpp")
    set(CPACK_PROJECT_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "checkpp static analysis runner")
    set(CPACK_PACKAGE_VENDOR "sverdaguer")
    set(CPACK_PACKAGE_CONTACT "-")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "sverdaguer")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

    # Produces Debian-style names like:
    # checkpp_0.1.2_amd64.deb
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # Auto-detect shared library dependencies.
    # Requires dpkg-shlibdeps, usually from dpkg-dev.
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

    include(CPack)


endif()
