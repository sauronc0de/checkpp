#include "StructNameCheck.hpp"
#include "Common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void StructNameCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(cxxRecordDecl(isDefinition(), isStruct(), unless(isImplicit())).bind("decl"), this);
}
void StructNameCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("decl");
    if (!decl) return;
    const std::string name = decl->getNameAsString();
    if (!name.empty() && !isPascalCase(name)) {
        diag(decl->getLocation(), "Rule 3.1: struct '%0' should use PascalCase") << name;
    }
}
