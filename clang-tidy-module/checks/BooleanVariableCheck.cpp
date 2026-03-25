#include "BooleanVariableCheck.hpp"
#include "Common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void BooleanVariableCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(varDecl(hasType(booleanType()), unless(isImplicit())).bind("decl"), this);
}
void BooleanVariableCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::VarDecl>("decl");
    if (!decl) return;
    const std::string name = decl->getNameAsString();
    if (!name.empty() && !hasBooleanPrefix(name)) {
        diag(decl->getLocation(), "Rule 12.1: boolean variable '%0' should start with is/has/can/should") << name;
    }
}
