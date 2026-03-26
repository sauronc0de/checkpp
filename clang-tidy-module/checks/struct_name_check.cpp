#include "struct_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void StructNameCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(cxxRecordDecl(isDefinition(), isStruct(), unless(isImplicit())).bind("decl"), this);
}
void StructNameCheck::check(const MatchFinder::MatchResult& result) {
    const auto* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("decl");
    if (!decl) return;
    const std::string kName = decl->getNameAsString();
    if (!kName.empty() && !isPascalCase(kName)) {
        diag(decl->getLocation(), "Rule 3.1: struct '%0' should use PascalCase") << kName;
    }
}
