#pragma once

#include <clang-tidy/ClangTidyCheck.h>

class LineLengthCheck final : public clang::tidy::ClangTidyCheck
{
public:
  using clang::tidy::ClangTidyCheck::ClangTidyCheck;

  void registerMatchers(clang::ast_matchers::MatchFinder *finder) override;
  void check(
      const clang::ast_matchers::MatchFinder::MatchResult &result) override;
  void onEndOfTranslationUnit() override;
  void storeOptions(clang::tidy::ClangTidyOptions::OptionMap &opts) override;

private:
  const unsigned maxLength_ = Options.getLocalOrGlobal("MaxLength", 80U);
  const clang::SourceManager *sourceManager_ = nullptr;
};
