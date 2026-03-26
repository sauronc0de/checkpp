#include "class_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void ClassNameCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(cxxRecordDecl(isDefinition(), isClass(), unless(isImplicit()), unless(isLambda())).bind("decl"), this);
}
void ClassNameCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("decl");
    if (!decl) return;
    const std::string kName = decl->getNameAsString();
    if (!kName.empty() && !isPascalCase(kName)) {
        diag(decl->getLocation(), "Rule 2.1: class '%0' should use PascalCase") << kName;
    }
}
