#include "line_length_check.hpp"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <optional>

namespace ast_matchers = clang::ast_matchers;

void LineLengthCheck::registerMatchers(ast_matchers::MatchFinder *finder)
{
  finder->addMatcher(ast_matchers::translationUnitDecl().bind("tu"), this);
}

void LineLengthCheck::check(
    const ast_matchers::MatchFinder::MatchResult &result)
{
  if(result.SourceManager) { sourceManager_ = result.SourceManager; }
}

void LineLengthCheck::storeOptions(
    clang::tidy::ClangTidyOptions::OptionMap &opts)
{
  Options.store(opts, "MaxLength", maxLength_);
}

void LineLengthCheck::onEndOfTranslationUnit()
{
  if(!sourceManager_) return;

  const clang::FileID kMainFileID = sourceManager_->getMainFileID();
  const std::optional<llvm::StringRef> kBuffer =
      sourceManager_->getBufferDataOrNone(kMainFileID);
  if(!kBuffer) return;

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

    const unsigned kLineLength = static_cast<unsigned>(lineEnd - position);
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
    else if(lineEnd < kContent.size()) { position = lineEnd + 1; }
    else { position = lineEnd; }

    ++lineNumber;
  }
}
