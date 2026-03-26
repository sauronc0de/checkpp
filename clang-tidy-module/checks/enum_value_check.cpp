#include "enum_value_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void EnumValueCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(enumConstantDecl().bind("decl"), this);
}
void EnumValueCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::EnumConstantDecl>("decl");
    if (!decl) return;
    const std::string kName = decl->getNameAsString();
    if (!kName.empty() && !isPascalCase(kName)) {
        diag(decl->getLocation(), "Rule 4.2: enum value '%0' should use PascalCase") << kName;
    }
}
