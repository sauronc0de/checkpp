#include "file_naming_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <filesystem>

namespace ast_matchers = clang::ast_matchers;

auto FileNamingCheck::registerMatchers(ast_matchers::MatchFinder *finder)
    -> void
{
  finder->addMatcher(ast_matchers::translationUnitDecl().bind("tu"), this);
}

auto FileNamingCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result) -> void
{
  const auto *tu = result.Nodes.getNodeAs<clang::TranslationUnitDecl>("tu");
  if(tu == nullptr || result.SourceManager == nullptr)
  {
    return;
  }

  auto loc = result.SourceManager->getLocForStartOfFile(
      result.SourceManager->getMainFileID());
  const std::string kFullPath = result.SourceManager->getFilename(loc).str();
  if(kFullPath.empty()) { return; }

  std::filesystem::path filePath(kFullPath);
  if(!isSnakeCase(filePath.stem().string()))
  {
    diag(loc, "Rule 1.1: file '%0' should use snake_case")
        << filePath.filename().string();
  }
}
