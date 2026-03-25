#include "MemberVariableCheck.hpp"
#include "Common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void MemberVariableCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(fieldDecl(unless(isImplicit())).bind("decl"), this);
}
void MemberVariableCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::FieldDecl>("decl");
    if (!decl) return;
    const std::string name = decl->getNameAsString();
    if (name.empty() || name.back() != '_') {
        diag(decl->getLocation(), "Rule 7.1: member variable '%0' should end with '_' ") << name;
        return;
    }
    const std::string base = name.substr(0, name.size() - 1);
    if (!base.empty() && !isCamelCase(base)) {
        diag(decl->getLocation(), "Rule 7.1: member variable '%0' should be camelCase with trailing '_' ") << name;
    }
}
