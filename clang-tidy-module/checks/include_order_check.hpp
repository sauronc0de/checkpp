#pragma once
#include <clang-tidy/ClangTidyCheck.h>
#include <clang/Lex/PPCallbacks.h>
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
  void registerPPCallbacks(const clang::SourceManager &sm, clang::Preprocessor *pp, clang::Preprocessor *moduleExpanderPP) override;
  void onEndOfTranslationUnit() override;

private:
  std::vector<IncludeEntry> includes_;
};
