#include "template_parameter_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

void TemplateParameterCheck::registerMatchers(ast_matchers::MatchFinder *finder)
{
  finder->addMatcher(ast_matchers::templateTypeParmDecl().bind("decl"), this);
}

void TemplateParameterCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result)
{
  const auto *ttp = result.Nodes.getNodeAs<clang::TemplateTypeParmDecl>("decl");
  if(!ttp) { return; }

  const std::string kName = ttp->getNameAsString();
  if(!kName.empty() && !isPascalCase(kName))
  {
    diag(ttp->getLocation(),
         "Rule 11.1: template parameter '%0' should use PascalCase")
        << kName;
  }
}
