#include "checks/boolean_variable_check.hpp"
#include "checks/class_name_check.hpp"
#include "checks/constructor_init_list_check.hpp"
#include "checks/enum_name_check.hpp"
#include "checks/enum_value_check.hpp"
#include "checks/file_naming_check.hpp"
#include "checks/function_name_check.hpp"
#include "checks/include_order_check.hpp"
#include "checks/line_length_check.hpp"
#include "checks/member_variable_check.hpp"
#include "checks/namespace_name_check.hpp"
#include "checks/no_using_namespace_std_check.hpp"
#include "checks/struct_name_check.hpp"
#include "checks/template_parameter_check.hpp"
#include "checks/variable_name_check.hpp"

#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

namespace tidy = clang::tidy;

namespace
{
class CompanyModule final : public tidy::ClangTidyModule
{
public:
  void addCheckFactories(tidy::ClangTidyCheckFactories &factories) override
  {
    factories.registerCheck<FileNamingCheck>("company-file-snake-case");
    factories.registerCheck<ClassNameCheck>("company-class-pascal-case");
    factories.registerCheck<StructNameCheck>("company-struct-pascal-case");
    factories.registerCheck<EnumNameCheck>("company-enum-pascal-case");
    factories.registerCheck<EnumValueCheck>("company-enum-value-pascal-case");
    factories.registerCheck<FunctionNameCheck>("company-function-camel-case");
    factories.registerCheck<VariableNameCheck>("company-variable-camel-case");
    factories.registerCheck<VariableNameCheck>("company-constant-k-prefix");
    factories.registerCheck<VariableNameCheck>("company-global-g-prefix");
    factories.registerCheck<MemberVariableCheck>(
        "company-member-trailing-underscore");
    factories.registerCheck<NamespaceNameCheck>("company-namespace-snake-case");
    factories.registerCheck<TemplateParameterCheck>(
        "company-template-parameter-pascal-case");
    factories.registerCheck<BooleanVariableCheck>("company-bool-prefix");
    factories.registerCheck<IncludeOrderCheck>("company-include-order");
    factories.registerCheck<NoUsingNamespaceStdCheck>(
        "company-no-using-namespace-std");
    factories.registerCheck<LineLengthCheck>("company-line-length");
    factories.registerCheck<ConstructorInitListCheck>(
        "company-constructor-init-list");
  }
};

static tidy::ClangTidyModuleRegistry::Add<CompanyModule> g_X(
    "company-module", "Company naming and style checks.");
} // namespace

volatile int g_CompanyModuleAnchorSource = 0;
