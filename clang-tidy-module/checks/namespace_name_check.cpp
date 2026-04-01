#include "namespace_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

auto NamespaceNameCheck::registerMatchers(ast_matchers::MatchFinder *finder)
    -> void
{
  finder->addMatcher(ast_matchers::namespaceDecl(
                         ast_matchers::unless(ast_matchers::isAnonymous()))
                         .bind("decl"),
                     this);
}

auto NamespaceNameCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result) -> void
{
  const auto *decl = result.Nodes.getNodeAs<clang::NamespaceDecl>("decl");
  if(decl == nullptr)
  {
    return;
  }

  const std::string kName = decl->getNameAsString();
  if(!kName.empty() && !isSnakeCase(kName))
  {
    diag(decl->getLocation(), "Rule 10.1: namespace '%0' should use snake_case")
        << kName;
  }
}
