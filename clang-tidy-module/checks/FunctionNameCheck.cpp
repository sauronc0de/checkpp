#include "FunctionNameCheck.hpp"
#include "Common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void FunctionNameCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(functionDecl(unless(cxxConstructorDecl()), unless(cxxDestructorDecl()), unless(isImplicit()), unless(isMain())).bind("decl"), this);
}
void FunctionNameCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::FunctionDecl>("decl");
    if (!decl || decl->isOverloadedOperator()) return;
    const std::string name = decl->getNameAsString();
    if (!name.empty() && !isCamelCase(name)) {
        diag(decl->getLocation(), "Rule 5.1: function '%0' should use camelCase") << name;
    }
}
