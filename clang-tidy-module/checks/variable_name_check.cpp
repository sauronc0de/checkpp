#include "variable_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <cctype>

namespace ast_matchers = clang::ast_matchers;
namespace
{
auto isKPascalCase(const std::string &name) -> bool
{
  return name.size() > 1 && name[0] == 'k' &&
         std::isupper(static_cast<unsigned char>(name[1])) != 0 &&
         isPascalCase(name.substr(1));
}
} // namespace

VariableNameCheck::VariableNameCheck(llvm::StringRef checkName,
                                     clang::tidy::ClangTidyContext *context)
    : ClangTidyCheck(checkName, context), checkName_(checkName.str())
{
}

auto VariableNameCheck::registerMatchers(ast_matchers::MatchFinder *finder)
    -> void
{
  finder->addMatcher(
      ast_matchers::varDecl(ast_matchers::unless(ast_matchers::parmVarDecl()),
                            ast_matchers::unless(ast_matchers::hasAncestor(
                                ast_matchers::cxxRecordDecl())),
                            ast_matchers::unless(ast_matchers::isImplicit()))
          .bind("decl"),
      this);
}

auto VariableNameCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result) -> void
{
  const auto *decl = result.Nodes.getNodeAs<clang::VarDecl>("decl");
  if(decl == nullptr || decl->isStaticDataMember())
  {
    return;
  }

  const std::string kName = decl->getNameAsString();
  if(kName.empty())
  {
    return;
  }

  bool isConstantRule = checkName_ == "company-constant-k-prefix";
  bool isGlobalRule = checkName_ == "company-global-g-prefix";
  bool isVariableRule = checkName_ == "company-variable-camel-case";
  bool isLocalSnakeCaseRule = checkName_ == "company-local-variable-snake-case";
  bool isModuleGlobalRule =
      checkName_ == "company-global-variable-module-prefix";
  bool isConstant = decl->getType().isConstQualified() || decl->isConstexpr();
  bool isGlobal = decl->hasGlobalStorage() && decl->isFileVarDecl();

  if(isConstantRule)
  {
    if(isConstant && !isKPascalCase(kName))
    {
      diag(decl->getLocation(),
           "Rule 8.1: constant '%0' should use kPascalCase")
          << kName;
    }
    return;
  }

  if(isGlobalRule)
  {
    if(isGlobal && !isConstant && kName.compare(0, 2, "g_") != 0)
    {
      diag(decl->getLocation(),
           "Rule 9.1: global variable '%0' should use g_ prefix")
          << kName;
    }
    return;
  }

  if(isVariableRule)
  {
    if(!isConstant && !isGlobal && !isCamelCase(kName))
    {
      diag(decl->getLocation(), "Rule 6.1: variable '%0' should use camelCase")
          << kName;
    }
    return;
  }

  if(isLocalSnakeCaseRule)
  {
    if(!isGlobal && !isSnakeCase(kName))
    {
      diag(decl->getLocation(),
           "local variable '%0' should use snake_case")
          << kName;
    }
    return;
  }

  if(isModuleGlobalRule)
  {
    if(isGlobal && !isConstant && decl->getStorageClass() != clang::SC_Static &&
       decl->getDeclContext()->getRedeclContext()->isTranslationUnit() &&
       !isModulePrefixedCamelCase(kName))
    {
      diag(decl->getLocation(),
           "global variable '%0' should use moduleName_variableName")
          << kName;
    }
    return;
  }

  if(!isCamelCase(kName))
  {
    diag(decl->getLocation(), "Rule 6.1: variable '%0' should use camelCase")
        << kName;
  }
}
