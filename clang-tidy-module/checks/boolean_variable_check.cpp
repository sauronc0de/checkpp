#include "boolean_variable_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

void BooleanVariableCheck::registerMatchers(ast_matchers::MatchFinder *finder)
{
  finder->addMatcher(
      ast_matchers::varDecl(ast_matchers::hasType(ast_matchers::booleanType()),
                            ast_matchers::unless(ast_matchers::isImplicit()))
          .bind("decl"),
      this);
}

void BooleanVariableCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result)
{
  const auto *decl = result.Nodes.getNodeAs<clang::VarDecl>("decl");
  if(!decl) { return; }

  const std::string kName = decl->getNameAsString();
  if(!kName.empty() && !hasBooleanPrefix(kName))
  {
    diag(decl->getLocation(),
         "Rule 12.1: boolean variable '%0' should start with is/has/can/should")
        << kName;
  }
}
