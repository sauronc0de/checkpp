add_library(CompanyClangTidyModule MODULE
    company_module.cpp
    checks/boolean_variable_check.cpp
    checks/class_name_check.cpp
    checks/constructor_init_list_check.cpp
    checks/enum_name_check.cpp
    checks/enum_value_check.cpp
    checks/file_naming_check.cpp
    checks/function_name_check.cpp
    checks/include_order_check.cpp
    checks/member_variable_check.cpp
    checks/namespace_name_check.cpp
    checks/no_using_namespace_std_check.cpp
    checks/struct_name_check.cpp
    checks/template_parameter_check.cpp
    checks/variable_name_check.cpp
)

target_include_directories(CompanyClangTidyModule PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(CompanyClangTidyModule
    PRIVATE
    clangAST
    clangASTMatchers
    clangBasic
    clangFrontend
    clangLex
    clangTidy
    clangTooling
)
