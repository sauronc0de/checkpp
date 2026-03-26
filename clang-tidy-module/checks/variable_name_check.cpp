#include "variable_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <cctype>
using namespace clang::ast_matchers;
namespace
{
bool isKPascalCase(const std::string &name)
{
  return name.size() > 1 && name[0] == 'k' && std::isupper(static_cast<unsigned char>(name[1])) && isPascalCase(name.substr(1));
}
} // namespace
VariableNameCheck::VariableNameCheck(llvm::StringRef checkName, clang::tidy::ClangTidyContext *context)
    : ClangTidyCheck(checkName, context), checkName_(checkName.str()) {}

void VariableNameCheck::registerMatchers(MatchFinder *finder)
{
  finder->addMatcher(varDecl(unless(parmVarDecl()), unless(hasAncestor(cxxRecordDecl())), unless(isImplicit())).bind("decl"), this);
}
void VariableNameCheck::check(const MatchFinder::MatchResult &result)
{
  const auto *decl = result.Nodes.getNodeAs<clang::VarDecl>("decl");
  if(!decl || decl->isStaticDataMember()) return;
  const std::string kName = decl->getNameAsString();
  if(kName.empty()) return;
  bool isConstantRule = checkName_ == "company-constant-k-prefix";
  bool isGlobalRule = checkName_ == "company-global-g-prefix";
  bool isVariableRule = checkName_ == "company-variable-camel-case";
  bool isConstant = decl->getType().isConstQualified() || decl->isConstexpr();
  bool isGlobal = decl->hasGlobalStorage() && decl->isFileVarDecl();

  if(isConstantRule)
  {
    if(isConstant && !isKPascalCase(kName))
    {
      diag(decl->getLocation(), "Rule 8.1: constant '%0' should use kPascalCase") << kName;
    }
    return;
  }

  if(isGlobalRule)
  {
    if(isGlobal && !isConstant && kName.rfind("g_", 0) != 0)
    {
      diag(decl->getLocation(), "Rule 9.1: global variable '%0' should use g_ prefix") << kName;
    }
    return;
  }

  if(isVariableRule)
  {
    if(!isConstant && !isGlobal && !isCamelCase(kName))
    {
      diag(decl->getLocation(), "Rule 6.1: variable '%0' should use camelCase") << kName;
    }
    return;
  }

  if(!isCamelCase(kName))
  {
    diag(decl->getLocation(), "Rule 6.1: variable '%0' should use camelCase") << kName;
  }
}
