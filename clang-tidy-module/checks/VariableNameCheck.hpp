#pragma once
#include <clang-tidy/ClangTidyCheck.h>
class VariableNameCheck : public clang::tidy::ClangTidyCheck {
public:
    VariableNameCheck(llvm::StringRef checkName, clang::tidy::ClangTidyContext* context);
    void registerMatchers(clang::ast_matchers::MatchFinder* finder) override;
    void check(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    std::string checkName_;
};
