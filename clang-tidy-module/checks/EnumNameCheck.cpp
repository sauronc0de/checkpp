#include "EnumNameCheck.hpp"
#include "Common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void EnumNameCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(enumDecl(isDefinition()).bind("decl"), this);
}
void EnumNameCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::EnumDecl>("decl");
    if (!decl || decl->getIdentifier() == nullptr) return;
    const std::string name = decl->getNameAsString();
    if (!name.empty() && !isPascalCase(name)) {
        diag(decl->getLocation(), "Rule 4.1: enum '%0' should use PascalCase") << name;
    }
}
