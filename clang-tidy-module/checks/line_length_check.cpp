#include "line_length_check.hpp"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <optional>

namespace ast_matchers = clang::ast_matchers;

auto LineLengthCheck::registerMatchers(ast_matchers::MatchFinder *finder)
    -> void
{
  finder->addMatcher(ast_matchers::translationUnitDecl().bind("tu"), this);
}

auto LineLengthCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result) -> void
{
  if(result.SourceManager != nullptr)
  {
    sourceManager_ = result.SourceManager;
  }
}

auto LineLengthCheck::storeOptions(
    clang::tidy::ClangTidyOptions::OptionMap &opts) -> void
{
  Options.store(opts, "MaxLength", maxLength_);
}

auto LineLengthCheck::onEndOfTranslationUnit() -> void
{
  if(sourceManager_ == nullptr)
  {
    return;
  }

  const clang::FileID kMainFileID = sourceManager_->getMainFileID();
  const std::optional<llvm::StringRef> kBuffer =
      sourceManager_->getBufferDataOrNone(kMainFileID);
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

    const auto kLineLength = static_cast<unsigned>(lineEnd - position);
    if(kLineLength > maxLength_)
    {
      const clang::SourceLocation kLoc =
          sourceManager_->translateLineCol(kMainFileID, lineNumber, 1);
      if(kLoc.isValid())
      {
        diag(kLoc, "Rule 15.1: line length %0 exceeds %1 characters")
            << kLineLength << maxLength_;
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
