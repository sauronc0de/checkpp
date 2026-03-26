#pragma once
#include <clang-tidy/ClangTidyCheck.h>
class ClassNameCheck : public clang::tidy::ClangTidyCheck {
public:
    using clang::tidy::ClangTidyCheck::ClangTidyCheck;
    void registerMatchers(clang::ast_matchers::MatchFinder* finder) override;
    void check(const clang::ast_matchers::MatchFinder::MatchResult& result) override;
};
