#include "no_tabs_check.hpp"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <optional>

namespace ast_matchers = clang::ast_matchers;

auto NoTabsCheck::registerMatchers(ast_matchers::MatchFinder *finder) -> void
{
  finder->addMatcher(ast_matchers::translationUnitDecl().bind("tu"), this);
}

auto NoTabsCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result) -> void
{
  if(result.SourceManager != nullptr)
  {
    sourceManager_ = result.SourceManager;
  }
}

auto NoTabsCheck::onEndOfTranslationUnit() -> void
{
  if(sourceManager_ == nullptr)
  {
    return;
  }

  const clang::FileID kMainFileId = sourceManager_->getMainFileID();
  const std::optional<llvm::StringRef> kBuffer =
      sourceManager_->getBufferDataOrNone(kMainFileId);
  if(!kBuffer)
  {
    return;
  }

  const llvm::StringRef kContent = *kBuffer;
  std::size_t position = 0;
  unsigned lineNumber = 1;

  while(position < kContent.size())
  {
    std::size_t lineEnd = position;
    while(lineEnd < kContent.size() && kContent[lineEnd] != '\n' &&
          kContent[lineEnd] != '\r')
    {
      ++lineEnd;
    }

    const std::size_t kTabPosition =
        kContent.slice(position, lineEnd).find('\t');
    if(kTabPosition != llvm::StringRef::npos)
    {
      const clang::SourceLocation kLoc = sourceManager_->translateLineCol(
          kMainFileId, lineNumber, static_cast<unsigned>(kTabPosition + 1));
      if(kLoc.isValid())
      {
        diag(kLoc, "tab characters are not allowed");
      }
    }

    if(lineEnd < kContent.size() && kContent[lineEnd] == '\r' &&
       lineEnd + 1 < kContent.size() && kContent[lineEnd + 1] == '\n')
    {
      position = lineEnd + 2;
    }
    else if(lineEnd < kContent.size())
    {
      position = lineEnd + 1;
    }
    else
    {
      position = lineEnd;
    }

    ++lineNumber;
  }
}
