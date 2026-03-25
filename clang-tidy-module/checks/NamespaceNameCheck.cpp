#include "NamespaceNameCheck.hpp"
#include "Common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void NamespaceNameCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(namespaceDecl(unless(isAnonymous())).bind("decl"), this);
}
void NamespaceNameCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::NamespaceDecl>("decl");
    if (!decl) return;
    const std::string name = decl->getNameAsString();
    if (!name.empty() && !isSnakeCase(name)) {
        diag(decl->getLocation(), "Rule 10.1: namespace '%0' should use snake_case") << name;
    }
}
