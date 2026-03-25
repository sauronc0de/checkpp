cmake_minimum_required(VERSION 3.20)
project(cpp_style_tool LANGUAGES C CXX)

option(CHECKPP_STRIP_SYMBOLS "Strip symbols from executable" OFF)
option(CHECKPP_LTO "Enable Link-Time Optimization" OFF)
option(CHECKPP_ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)
option(CHECKPP_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(CHECKPP_ENABLE_COVERAGE "Enable code coverage" OFF)
option(CHECKPP_ENABLE_STATIC_ANALYSIS "Enable static analysis tools" OFF)
option(CHECKPP_ENABLE_FUZZ "Enable fuzzing instrumentation" OFF)

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

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CLANG_CMAKE_DIR}")
include(AddLLVM)
include(AddClang)
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(clang-tidy-module)

add_executable(cpp-style-tool
    src/main.cpp
    src/config.cpp
    src/runner.cpp
)

target_include_directories(cpp-style-tool PRIVATE include)

if(TARGET yaml-cpp::yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
elseif(TARGET yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp)
else()
    message(FATAL_ERROR "yaml-cpp was found, but no usable CMake target is available")
endif()

target_link_libraries(cpp-style-tool
    PRIVATE
    ${YAML_CPP_TARGET}
)

if(CHECKPP_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION cpp-style-tool TRUE)
        message(STATUS "IPO / LTO enabled")
    else()
        message(WARNING "IPO not supported: ${ipo_error}")
    endif()
endif()

if(CHECKPP_STRIP_SYMBOLS)
    add_custom_command(TARGET cpp-style-tool POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --strip-all $<TARGET_FILE:cpp-style-tool> ${CMAKE_CURRENT_BINARY_DIR}/cpp-style-tool-stripped
        COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/cpp-style-tool-stripped $<TARGET_FILE:cpp-style-tool>
        COMMENT "Stripping symbols from executable"
    )
endif()

install(TARGETS cpp-style-tool)
