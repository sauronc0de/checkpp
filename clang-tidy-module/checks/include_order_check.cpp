#include "include_order_check.hpp"
#include <clang/Lex/Preprocessor.h>
#include <memory>

namespace
{
class IncludeOrderCallbacks : public clang::PPCallbacks
{
public:
  explicit IncludeOrderCallbacks(
      std::vector<IncludeOrderCheck::IncludeEntry> &includes)
      : includes_(includes)
  {
  }
  auto InclusionDirective(
      clang::SourceLocation hashLoc, const clang::Token &token,
      llvm::StringRef fileName, bool isAngled, clang::CharSourceRange range,
      clang::OptionalFileEntryRef fileEntry, llvm::StringRef searchPath,
      llvm::StringRef relativePath, const clang::Module *importedModule,
      clang::SrcMgr::CharacteristicKind kind) -> void override
  {
    const int kGroup = [&fileName, isAngled]() {
      if(!isAngled && fileName.find('/') == llvm::StringRef::npos)
      {
        return 0;
      }
      if(isAngled && fileName.find('/') == llvm::StringRef::npos)
      {
        return 1;
      }
      if(isAngled)
      {
        return 2;
      }
      return 3;
    }();
    const std::string kName = fileName.str();
    includes_.push_back({hashLoc, kName, kGroup});
    (void)token;
    (void)range;
    (void)fileEntry;
    (void)searchPath;
    (void)relativePath;
    (void)importedModule;
    (void)kind;
  }

private:
  std::vector<IncludeOrderCheck::IncludeEntry> &includes_;
};
} // namespace

auto IncludeOrderCheck::registerPPCallbacks(
    const clang::SourceManager &sourceManager,
    clang::Preprocessor *preprocessor,
    clang::Preprocessor *moduleExpanderPreprocessor)
    -> void
{
  (void)sourceManager;
  (void)moduleExpanderPreprocessor;
  preprocessor->addPPCallbacks(
      std::make_unique<IncludeOrderCallbacks>(includes_));
}

auto IncludeOrderCheck::onEndOfTranslationUnit() -> void
{
  int previousGroup = -1;
  for(const auto &include : includes_)
  {
    if(include.group_ < previousGroup)
    {
      diag(include.loc_, "Rule 13.1: include order should be local, standard,"
                         " third-party, project");
      break;
    }
    previousGroup = include.group_;
  }
  includes_.clear();
}
