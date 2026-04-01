#pragma once
#include <clang-tidy/ClangTidyCheck.h>
#include <clang/Lex/PPCallbacks.h>
#include <string>
#include <vector>

class IncludeOrderCheck : public clang::tidy::ClangTidyCheck
{
public:
  using clang::tidy::ClangTidyCheck::ClangTidyCheck;
  struct IncludeEntry
  {
    clang::SourceLocation loc_;
    std::string name_;
    int group_ = 0;
  };
  auto registerPPCallbacks(const clang::SourceManager &sourceManager,
                           clang::Preprocessor *preprocessor,
                           clang::Preprocessor *moduleExpanderPreprocessor)
      -> void override;
  auto onEndOfTranslationUnit() -> void override;

private:
  std::vector<IncludeEntry> includes_;
};
