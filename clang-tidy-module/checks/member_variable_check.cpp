#include "member_variable_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

void MemberVariableCheck::registerMatchers(ast_matchers::MatchFinder *finder)
{
  finder->addMatcher(
      ast_matchers::fieldDecl(ast_matchers::unless(ast_matchers::isImplicit()))
          .bind("decl"),
      this);
}

void MemberVariableCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result)
{
  const auto *decl = result.Nodes.getNodeAs<clang::FieldDecl>("decl");
  if(!decl) { return; }

  const std::string kName = decl->getNameAsString();
  if(kName.empty() || kName.back() != '_')
  {
    diag(decl->getLocation(),
         "Rule 7.1: member variable '%0' should end with '_' ")
        << kName;
    return;
  }

  const std::string kBase = kName.substr(0, kName.size() - 1);
  if(!kBase.empty() && !isCamelCase(kBase))
  {
    diag(decl->getLocation(),
         "Rule 7.1: member variable '%0' should be camelCase with trailing '_'")
        << kName;
  }
}
