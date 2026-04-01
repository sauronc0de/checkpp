#include "struct_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

auto StructNameCheck::registerMatchers(ast_matchers::MatchFinder *finder)
    -> void
{
  finder->addMatcher(ast_matchers::cxxRecordDecl(
                         ast_matchers::isDefinition(), ast_matchers::isStruct(),
                         ast_matchers::unless(ast_matchers::isImplicit()))
                         .bind("decl"),
                     this);
}

auto StructNameCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result) -> void
{
  const auto *decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("decl");
  if(decl == nullptr)
  {
    return;
  }

  const std::string kName = decl->getNameAsString();
  if(!kName.empty() && !isPascalCase(kName))
  {
    diag(decl->getLocation(), "Rule 3.1: struct '%0' should use PascalCase")
        << kName;
  }
}
