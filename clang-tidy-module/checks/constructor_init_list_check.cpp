#include "constructor_init_list_check.hpp"
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace ast_matchers = clang::ast_matchers;

void ConstructorInitListCheck::registerMatchers(
    ast_matchers::MatchFinder *finder)
{
  finder->addMatcher(ast_matchers::cxxConstructorDecl(
                         ast_matchers::isDefinition(),
                         ast_matchers::unless(ast_matchers::isImplicit()))
                         .bind("ctor"),
                     this);
}

void ConstructorInitListCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result)
{
  const auto *ctor = result.Nodes.getNodeAs<clang::CXXConstructorDecl>("ctor");
  if(!ctor || ctor->getNumCtorInitializers() > 0 || !ctor->hasBody())
  {
    return;
  }

  const auto *body = llvm::dyn_cast<clang::CompoundStmt>(ctor->getBody());
  if(!body) { return; }

  for(const auto *stmt : body->body())
  {
    const auto *expr = llvm::dyn_cast<clang::BinaryOperator>(stmt);
    if(expr && expr->isAssignmentOp())
    {
      diag(ctor->getLocation(),
           "Rule 16.1: constructor '%0' may prefer an initializer list")
          << ctor->getNameAsString();
      return;
    }
  }
}
