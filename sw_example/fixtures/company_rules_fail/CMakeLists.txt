cmake_minimum_required(VERSION 3.20)

project(company_rules_fail LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wimplicit-fallthrough)

add_library(lib_wrong
    lib_wrong/src/wrong_extra.cpp
)

target_include_directories(lib_wrong PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    lib_wrong/inc
)

add_executable(company_rules_fail BadMain.cpp)
target_link_libraries(company_rules_fail PRIVATE lib_wrong)
