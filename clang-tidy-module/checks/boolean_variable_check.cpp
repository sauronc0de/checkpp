#include "boolean_variable_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void BooleanVariableCheck::registerMatchers(MatchFinder *finder)
{
  finder->addMatcher(varDecl(hasType(booleanType()), unless(isImplicit())).bind("decl"), this);
}
void BooleanVariableCheck::check(const MatchFinder::MatchResult &result)
{
  const auto *decl = result.Nodes.getNodeAs<clang::VarDecl>("decl");
  if(!decl) return;
  const std::string kName = decl->getNameAsString();
  if(!kName.empty() && !hasBooleanPrefix(kName))
  {
    diag(decl->getLocation(), "Rule 12.1: boolean variable '%0' should start with is/has/can/should") << kName;
  }
}
