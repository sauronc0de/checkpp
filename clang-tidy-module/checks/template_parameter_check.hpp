#pragma once
#include <clang-tidy/ClangTidyCheck.h>
class TemplateParameterCheck : public clang::tidy::ClangTidyCheck
{
public:
  using clang::tidy::ClangTidyCheck::ClangTidyCheck;
  auto registerMatchers(clang::ast_matchers::MatchFinder *finder)
      -> void override;
  auto check(const clang::ast_matchers::MatchFinder::MatchResult &result)
      -> void override;
};
