#include "variable_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <cctype>
#include <cstdint>

namespace ast_matchers = clang::ast_matchers;
namespace
{
enum class VariableRuleKind : std::uint8_t
{
  Constant,
  Global,
  Variable,
  LocalSnakeCase,
  ModuleGlobal,
  DefaultCamelCase,
};

auto isKPascalCase(const std::string &name) -> bool
{
  return name.size() > 1 && name[0] == 'k' &&
         std::isupper(static_cast<unsigned char>(name[1])) != 0 &&
         isPascalCase(name.substr(1));
}

auto detectRuleKind(const std::string &checkName) -> VariableRuleKind
{
  if(checkName == "company-constant-k-prefix")
  {
    return VariableRuleKind::Constant;
  }
  if(checkName == "company-global-g-prefix")
  {
    return VariableRuleKind::Global;
  }
  if(checkName == "company-variable-camel-case")
  {
    return VariableRuleKind::Variable;
  }
  if(checkName == "company-local-variable-snake-case")
  {
    return VariableRuleKind::LocalSnakeCase;
  }
  if(checkName == "company-global-variable-module-prefix")
  {
    return VariableRuleKind::ModuleGlobal;
  }
  return VariableRuleKind::DefaultCamelCase;
}

auto isConstantVariable(const clang::VarDecl &decl) -> bool
{
  return decl.getType().isConstQualified() || decl.isConstexpr();
}

auto isGlobalVariable(const clang::VarDecl &decl) -> bool
{
  return decl.hasGlobalStorage() && decl.isFileVarDecl();
}

auto shouldDiagnose(VariableRuleKind ruleKind,
                    const clang::VarDecl &decl,
                    const std::string &name) -> bool
{
  bool isConstant = isConstantVariable(decl);
  bool isGlobal = isGlobalVariable(decl);

  switch(ruleKind)
  {
  case VariableRuleKind::Constant:
    return isConstant && !isKPascalCase(name);
  case VariableRuleKind::Global:
    return isGlobal && !isConstant && name.compare(0, 2, "g_") != 0;
  case VariableRuleKind::Variable:
    return !isConstant && !isGlobal && !isCamelCase(name);
  case VariableRuleKind::LocalSnakeCase:
    return !isGlobal && !isSnakeCase(name);
  case VariableRuleKind::ModuleGlobal:
    return isGlobal && !isConstant &&
           decl.getStorageClass() != clang::SC_Static &&
           decl.getDeclContext()->getRedeclContext()->isTranslationUnit() &&
           !isModulePrefixedCamelCase(name);
  case VariableRuleKind::DefaultCamelCase:
    return !isCamelCase(name);
  }

  return false;
}

auto diagnosticMessage(VariableRuleKind ruleKind) -> const char *
{
  switch(ruleKind)
  {
  case VariableRuleKind::Constant:
    return "Rule 8.1: constant '%0' should use kPascalCase";
  case VariableRuleKind::Global:
    return "Rule 9.1: global variable '%0' should use g_ prefix";
  case VariableRuleKind::Variable:
  case VariableRuleKind::DefaultCamelCase:
    return "Rule 6.1: variable '%0' should use camelCase";
  case VariableRuleKind::LocalSnakeCase:
    return "local variable '%0' should use snake_case";
  case VariableRuleKind::ModuleGlobal:
    return "global variable '%0' should use moduleName_variableName";
  }

  return "variable '%0' has an invalid name";
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

  auto ruleKind = detectRuleKind(checkName_);
  if(shouldDiagnose(ruleKind, *decl, kName))
  {
    diag(decl->getLocation(), diagnosticMessage(ruleKind)) << kName;
  }
}
