#include "namespace_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void NamespaceNameCheck::registerMatchers(MatchFinder *finder)
{
  finder->addMatcher(namespaceDecl(unless(isAnonymous())).bind("decl"), this);
}
void NamespaceNameCheck::check(const MatchFinder::MatchResult &result)
{
  const auto *decl = result.Nodes.getNodeAs<clang::NamespaceDecl>("decl");
  if(!decl) return;
  const std::string kName = decl->getNameAsString();
  if(!kName.empty() && !isSnakeCase(kName))
  {
    diag(decl->getLocation(), "Rule 10.1: namespace '%0' should use snake_case") << kName;
  }
}
