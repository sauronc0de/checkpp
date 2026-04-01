#pragma once

#include <clang-tidy/ClangTidyCheck.h>

class LineLengthCheck final : public clang::tidy::ClangTidyCheck
{
public:
  using clang::tidy::ClangTidyCheck::ClangTidyCheck;

  auto registerMatchers(clang::ast_matchers::MatchFinder *finder)
      -> void override;
  auto check(const clang::ast_matchers::MatchFinder::MatchResult &result)
      -> void override;
  auto onEndOfTranslationUnit() -> void override;
  auto storeOptions(clang::tidy::ClangTidyOptions::OptionMap &opts)
      -> void override;

private:
  const unsigned maxLength_ = Options.getLocalOrGlobal("MaxLength", 80U);
  const clang::SourceManager *sourceManager_ = nullptr;
};
