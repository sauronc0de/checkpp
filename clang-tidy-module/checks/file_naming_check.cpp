#include "file_naming_check.hpp"
#include "common.hpp"
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <filesystem>

namespace ast_matchers = clang::ast_matchers;

void FileNamingCheck::registerMatchers(ast_matchers::MatchFinder *finder)
{
  finder->addMatcher(ast_matchers::translationUnitDecl().bind("tu"), this);
}

void FileNamingCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result)
{
  const auto *tu = result.Nodes.getNodeAs<clang::TranslationUnitDecl>("tu");
  if(!tu || !result.SourceManager) { return; }

  auto loc = result.SourceManager->getLocForStartOfFile(
      result.SourceManager->getMainFileID());
  const std::string kFullPath = result.SourceManager->getFilename(loc).str();
  if(kFullPath.empty()) { return; }

  std::filesystem::path p(kFullPath);
  if(!isSnakeCase(p.stem().string()))
  {
    diag(loc, "Rule 1.1: file '%0' should use snake_case")
        << p.filename().string();
  }
}
