#include "enum_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

auto EnumNameCheck::registerMatchers(ast_matchers::MatchFinder *finder) -> void
{
  finder->addMatcher(
      ast_matchers::enumDecl(ast_matchers::isDefinition()).bind("decl"), this);
}

auto EnumNameCheck::check(const ast_matchers::MatchFinder::MatchResult &result)
    -> void
{
  const auto *decl = result.Nodes.getNodeAs<clang::EnumDecl>("decl");
  if(decl == nullptr || decl->getIdentifier() == nullptr)
  {
    return;
  }

  const std::string kName = decl->getNameAsString();
  if(!kName.empty() && !isPascalCase(kName))
  {
    diag(decl->getLocation(), "Rule 4.1: enum '%0' should use PascalCase")
        << kName;
  }
}
