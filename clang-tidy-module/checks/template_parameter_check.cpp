#include "template_parameter_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

auto TemplateParameterCheck::registerMatchers(ast_matchers::MatchFinder *finder)
    -> void
{
  finder->addMatcher(ast_matchers::templateTypeParmDecl().bind("decl"), this);
}

auto TemplateParameterCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result) -> void
{
  const auto *templateParameter =
      result.Nodes.getNodeAs<clang::TemplateTypeParmDecl>("decl");
  if(templateParameter == nullptr)
  {
    return;
  }

  const std::string kName = templateParameter->getNameAsString();
  if(!kName.empty() && !isPascalCase(kName))
  {
    diag(templateParameter->getLocation(),
         "Rule 11.1: template parameter '%0' should use PascalCase")
        << kName;
  }
}
