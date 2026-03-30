#include "namespace_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

void NamespaceNameCheck::registerMatchers(ast_matchers::MatchFinder *finder)
{
  finder->addMatcher(ast_matchers::namespaceDecl(
                         ast_matchers::unless(ast_matchers::isAnonymous()))
                         .bind("decl"),
                     this);
}

void NamespaceNameCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result)
{
  const auto *decl = result.Nodes.getNodeAs<clang::NamespaceDecl>("decl");
  if(!decl) { return; }

  const std::string kName = decl->getNameAsString();
  if(!kName.empty() && !isSnakeCase(kName))
  {
    diag(decl->getLocation(), "Rule 10.1: namespace '%0' should use snake_case")
        << kName;
  }
}
