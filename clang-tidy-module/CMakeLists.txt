set(LLVM_LINK_COMPONENTS Support)

add_clang_library(CompanyClangTidyModule MODULE
    CompanyModule.cpp
    checks/BooleanVariableCheck.cpp
    checks/ClassNameCheck.cpp
    checks/ConstructorInitListCheck.cpp
    checks/EnumNameCheck.cpp
    checks/EnumValueCheck.cpp
    checks/FileNamingCheck.cpp
    checks/FunctionNameCheck.cpp
    checks/IncludeOrderCheck.cpp
    checks/MemberVariableCheck.cpp
    checks/NamespaceNameCheck.cpp
    checks/NoUsingNamespaceStdCheck.cpp
    checks/StructNameCheck.cpp
    checks/TemplateParameterCheck.cpp
    checks/VariableNameCheck.cpp

    PLUGIN_TOOL
    clang-tidy
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
