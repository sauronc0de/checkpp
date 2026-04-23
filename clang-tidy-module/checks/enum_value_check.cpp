#include "enum_value_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

EnumValueCheck::EnumValueCheck(llvm::StringRef checkName,
                               clang::tidy::ClangTidyContext *context)
    : ClangTidyCheck(checkName, context), checkName_(checkName.str())
{
}

namespace ast_matchers = clang::ast_matchers;

auto EnumValueCheck::registerMatchers(ast_matchers::MatchFinder *finder) -> void
{
  finder->addMatcher(ast_matchers::enumConstantDecl().bind("decl"), this);
}

auto EnumValueCheck::check(const ast_matchers::MatchFinder::MatchResult &result)
    -> void
{
  const auto *decl = result.Nodes.getNodeAs<clang::EnumConstantDecl>("decl");
  if(decl == nullptr)
  {
    return;
  }

  const std::string kName = decl->getNameAsString();
  if(kName.empty())
  {
    return;
  }

  if(checkName_ == "company-enum-value-upper-case")
  {
    if(!isUpperSnakeCase(kName))
    {
      diag(decl->getLocation(),
           "enum value '%0' should use UPPER_CASE")
          << kName;
    }
    return;
  }

  if(!isPascalCase(kName))
  {
    diag(decl->getLocation(), "Rule 4.2: enum value '%0' should use PascalCase")
        << kName;
  }
}
