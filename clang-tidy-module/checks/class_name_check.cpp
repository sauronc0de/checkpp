#include "class_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

auto ClassNameCheck::registerMatchers(ast_matchers::MatchFinder *finder) -> void
{
  finder->addMatcher(ast_matchers::cxxRecordDecl(
                         ast_matchers::isDefinition(), ast_matchers::isClass(),
                         ast_matchers::unless(ast_matchers::isImplicit()),
                         ast_matchers::unless(ast_matchers::isLambda()))
                         .bind("decl"),
                     this);
}

auto ClassNameCheck::check(const ast_matchers::MatchFinder::MatchResult &result)
    -> void
{
  const auto *decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("decl");
  if(decl == nullptr)
  {
    return;
  }

  const std::string kName = decl->getNameAsString();
  if(!kName.empty() && !isPascalCase(kName))
  {
    diag(decl->getLocation(), "Rule 2.1: class '%0' should use PascalCase")
        << kName;
  }
}
