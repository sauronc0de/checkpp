#include "TemplateParameterCheck.hpp"
#include "Common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void TemplateParameterCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(templateTypeParmDecl().bind("decl"), this);
}
void TemplateParameterCheck::check(const MatchFinder::MatchResult& result) {
    const auto* ttp = result.Nodes.getNodeAs<clang::TemplateTypeParmDecl>("decl");
    if (!ttp) return;
    const std::string name = ttp->getNameAsString();
    if (!name.empty() && !isPascalCase(name)) {
        diag(ttp->getLocation(), "Rule 11.1: template parameter '%0' should use PascalCase") << name;
    }
}
