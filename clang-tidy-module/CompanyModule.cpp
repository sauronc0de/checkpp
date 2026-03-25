#include "checks/BooleanVariableCheck.hpp"
#include "checks/ClassNameCheck.hpp"
#include "checks/ConstructorInitListCheck.hpp"
#include "checks/EnumNameCheck.hpp"
#include "checks/EnumValueCheck.hpp"
#include "checks/FileNamingCheck.hpp"
#include "checks/FunctionNameCheck.hpp"
#include "checks/IncludeOrderCheck.hpp"
#include "checks/MemberVariableCheck.hpp"
#include "checks/NamespaceNameCheck.hpp"
#include "checks/NoUsingNamespaceStdCheck.hpp"
#include "checks/StructNameCheck.hpp"
#include "checks/TemplateParameterCheck.hpp"
#include "checks/VariableNameCheck.hpp"

#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

using namespace clang::tidy;

namespace
{
class CompanyModule final : public ClangTidyModule
{
public:
  void addCheckFactories(ClangTidyCheckFactories &factories) override
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
    factories.registerCheck<MemberVariableCheck>("company-member-trailing-underscore");
    factories.registerCheck<NamespaceNameCheck>("company-namespace-snake-case");
    factories.registerCheck<TemplateParameterCheck>("company-template-parameter-pascal-case");
    factories.registerCheck<BooleanVariableCheck>("company-bool-prefix");
    factories.registerCheck<IncludeOrderCheck>("company-include-order");
    factories.registerCheck<NoUsingNamespaceStdCheck>("company-no-using-namespace-std");
    factories.registerCheck<ConstructorInitListCheck>("company-constructor-init-list");
  }
};

static ClangTidyModuleRegistry::Add<CompanyModule>
    g_X("company-module", "Company naming and style checks.");
} // namespace

volatile int g_CompanyModuleAnchorSource = 0;
