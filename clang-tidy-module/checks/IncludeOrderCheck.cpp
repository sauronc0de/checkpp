#include "IncludeOrderCheck.hpp"
#include <clang/Lex/Preprocessor.h>
#include <memory>

namespace
{
class IncludeOrderCallbacks : public clang::PPCallbacks
{
public:
  explicit IncludeOrderCallbacks(std::vector<IncludeOrderCheck::IncludeEntry> &includes) : includes_(includes) {}
  void InclusionDirective(clang::SourceLocation hashLoc,
                          const clang::Token &, llvm::StringRef fileName, bool isAngled,
                          clang::CharSourceRange, const clang::FileEntry *,
                          llvm::StringRef, llvm::StringRef, const clang::Module *,
                          clang::SrcMgr::CharacteristicKind) override
  {
    const int kGroup = [&fileName, isAngled]() {
      if(!isAngled && fileName.find('/') == llvm::StringRef::npos) return 0;
      if(isAngled && fileName.find('/') == llvm::StringRef::npos) return 1;
      if(isAngled) return 2;
      return 3;
    }();
    const std::string kName = fileName.str();
    includes_.push_back({hashLoc, kName, kGroup});
  }

private:
  std::vector<IncludeOrderCheck::IncludeEntry> &includes_;
};
} // namespace

void IncludeOrderCheck::registerPPCallbacks(const clang::SourceManager &, clang::Preprocessor *pp, clang::Preprocessor *)
{
  pp->addPPCallbacks(std::make_unique<IncludeOrderCallbacks>(includes_));
}

void IncludeOrderCheck::onEndOfTranslationUnit()
{
  int previousGroup = -1;
  for(const auto &include : includes_)
  {
    if(include.group_ < previousGroup)
    {
      diag(include.loc_, "Rule 13.1: include order should be local, standard, third-party, project");
      break;
    }
    previousGroup = include.group_;
  }
  includes_.clear();
}
