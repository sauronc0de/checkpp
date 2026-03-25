#include "ConstructorInitListCheck.hpp"
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
using namespace clang::ast_matchers;
void ConstructorInitListCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(cxxConstructorDecl(isDefinition(), unless(isImplicit())).bind("ctor"), this);
}
void ConstructorInitListCheck::check(const MatchFinder::MatchResult& result) {
    const auto* ctor = result.Nodes.getNodeAs<clang::CXXConstructorDecl>("ctor");
    if (!ctor || ctor->getNumCtorInitializers() > 0 || !ctor->hasBody()) return;
    const auto* body = llvm::dyn_cast<clang::CompoundStmt>(ctor->getBody());
    if (!body) return;
    for (const auto* stmt : body->body()) {
        const auto* expr = llvm::dyn_cast<clang::BinaryOperator>(stmt);
        if (expr && expr->isAssignmentOp()) {
            diag(ctor->getLocation(), "Rule 16.1: constructor '%0' may prefer an initializer list") << ctor->getNameAsString();
            return;
        }
    }
}
