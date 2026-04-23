#include "macro_name_check.hpp"

#include "common.hpp"

#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <memory>

namespace
{
class MacroNameCallbacks final : public clang::PPCallbacks
{
public:
  MacroNameCallbacks(MacroNameCheck &check,
                     const clang::SourceManager &sourceManager)
      : check_(check), sourceManager_(sourceManager)
  {
  }

  auto MacroDefined(const clang::Token &macroNameToken,
                    const clang::MacroDirective *macroDirective)
      -> void override
  {
    (void)macroDirective;

    const clang::SourceLocation kLoc = macroNameToken.getLocation();
    if(!sourceManager_.isWrittenInMainFile(kLoc))
    {
      return;
    }

    const auto *kIdentifier = macroNameToken.getIdentifierInfo();
    if(kIdentifier == nullptr)
    {
      return;
    }

    const std::string kName = kIdentifier->getName().str();
    if(!isUpperSnakeCase(kName))
    {
      check_.diag(kLoc, "macro '%0' should use UPPER_CASE") << kName;
    }
  }

private:
  MacroNameCheck &check_;
  const clang::SourceManager &sourceManager_;
};
} // namespace

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
auto MacroNameCheck::registerPPCallbacks(
    const clang::SourceManager &sourceManager,
    clang::Preprocessor *preprocessor,
    clang::Preprocessor *moduleExpanderPreprocessor) -> void
{
  (void)moduleExpanderPreprocessor;
  preprocessor->addPPCallbacks(
      std::make_unique<MacroNameCallbacks>(*this, sourceManager));
}
// NOLINTEND(bugprone-easily-swappable-parameters)
