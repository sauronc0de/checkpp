#pragma once
#include <clang-tidy/ClangTidyCheck.h>
class EnumValueCheck : public clang::tidy::ClangTidyCheck
{
public:
  EnumValueCheck(llvm::StringRef checkName,
                 clang::tidy::ClangTidyContext *context);
  auto registerMatchers(clang::ast_matchers::MatchFinder *finder)
      -> void override;
  auto check(const clang::ast_matchers::MatchFinder::MatchResult &result)
      -> void override;

private:
  std::string checkName_;
};
