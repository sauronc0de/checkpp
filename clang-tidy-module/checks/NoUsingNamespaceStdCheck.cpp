#include "NoUsingNamespaceStdCheck.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void NoUsingNamespaceStdCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(usingDirectiveDecl().bind("decl"), this);
}
void NoUsingNamespaceStdCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::UsingDirectiveDecl>("decl");
    if (!decl || !decl->getNominatedNamespace()) return;
    if (decl->getNominatedNamespace()->getNameAsString() == "std") {
        diag(decl->getLocation(), "Rule 14.1: avoid 'using namespace std;'");
    }
}
