#pragma once

#include <clang-tidy/ClangTidyCheck.h>

class MacroNameCheck final : public clang::tidy::ClangTidyCheck
{
public:
  using clang::tidy::ClangTidyCheck::ClangTidyCheck;

  auto registerPPCallbacks(const clang::SourceManager &sourceManager,
                           clang::Preprocessor *preprocessor,
                           clang::Preprocessor *moduleExpanderPreprocessor)
      -> void override;
};
