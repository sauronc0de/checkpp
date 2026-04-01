#include "no_using_namespace_std_check.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

auto NoUsingNamespaceStdCheck::registerMatchers(
    ast_matchers::MatchFinder *finder) -> void
{
  finder->addMatcher(ast_matchers::usingDirectiveDecl().bind("decl"), this);
}

auto NoUsingNamespaceStdCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result) -> void
{
  const auto *decl = result.Nodes.getNodeAs<clang::UsingDirectiveDecl>("decl");
  if(decl == nullptr || decl->getNominatedNamespace() == nullptr)
  {
    return;
  }

  if(decl->getNominatedNamespace()->getNameAsString() == "std")
  {
    diag(decl->getLocation(), "Rule 14.1: avoid 'using namespace std;'");
  }
}
