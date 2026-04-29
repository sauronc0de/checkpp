#include "function_name_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>

FunctionNameCheck::FunctionNameCheck(
    llvm::StringRef checkName, clang::tidy::ClangTidyContext *context)
    : ClangTidyCheck(checkName, context), checkName_(checkName.str())
{
}

namespace ast_matchers = clang::ast_matchers;

auto FunctionNameCheck::registerMatchers(ast_matchers::MatchFinder *finder)
    -> void
{
  finder->addMatcher(
      ast_matchers::functionDecl(
          ast_matchers::unless(ast_matchers::cxxConstructorDecl()),
          ast_matchers::unless(ast_matchers::cxxDestructorDecl()),
          ast_matchers::unless(ast_matchers::isImplicit()),
          ast_matchers::unless(ast_matchers::isMain()),
          ast_matchers::unless(
              ast_matchers::cxxMethodDecl(ast_matchers::isOverride())))
          .bind("decl"),
      this);
}

auto FunctionNameCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result) -> void
{
  const auto *decl = result.Nodes.getNodeAs<clang::FunctionDecl>("decl");
  if(decl == nullptr || decl->isOverloadedOperator())
  {
    return;
  }

  const std::string kName = decl->getNameAsString();
  if(kName.empty())
  {
    return;
  }

  if(checkName_ == "company-global-function-module-prefix")
  {
    if(!decl->getDeclContext()->getRedeclContext()->isTranslationUnit() ||
       decl->getStorageClass() == clang::SC_Static)
    {
      return;
    }

    if(result.SourceManager != nullptr)
    {
      const clang::SourceLocation kExpansionLoc =
          result.SourceManager->getExpansionLoc(decl->getLocation());
      const auto kExpectedModuleName =
          moduleNameFromPath(result.SourceManager->getFilename(kExpansionLoc).str());
      if(kExpectedModuleName.has_value())
      {
        if(!hasExpectedModulePrefix(kName, *kExpectedModuleName))
        {
          diag(decl->getLocation(),
               "global function '%0' should use moduleName_functionName")
              << kName;
        }
        return;
      }
    }

    if(!isModulePrefixedCamelCase(kName))
    {
      diag(decl->getLocation(),
           "global function '%0' should use moduleName_functionName")
          << kName;
    }
    return;
  }

  if(!isCamelCase(kName))
  {
    diag(decl->getLocation(), "Rule 5.1: function '%0' should use camelCase")
        << kName;
  }
}
