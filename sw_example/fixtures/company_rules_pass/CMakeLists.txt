cmake_minimum_required(VERSION 3.20)

project(company_rules_pass LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wimplicit-fallthrough)

add_library(lib_portable
    lib_portable/src/lib_portable.cpp
    lib_portable/src/lib_portable_math.cpp
)

target_include_directories(lib_portable PUBLIC lib_portable/inc)

add_executable(company_rules_pass main.cpp)
target_link_libraries(company_rules_pass PRIVATE lib_portable)
