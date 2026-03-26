#include "embedded_clang_tidy_module.hpp"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <unistd.h>

namespace fs = std::filesystem;

#ifdef CHECKPP_EMBED_CLANG_TIDY_MODULE
#include "embedded_clang_tidy_module_data.inc"

namespace
{
fs::path writeEmbeddedModuleToTempFile()
{
  std::array<char, 256> kTemplate{};
  std::snprintf(kTemplate.data(), kTemplate.size(), "%s/checkpp-embedded-XXXXXX", fs::temp_directory_path().c_str());

  const int kFd = ::mkstemp(kTemplate.data());
  if(kFd == -1)
  {
    throw std::runtime_error("failed to create embedded clang-tidy module file template");
  }
  ::close(kFd);

  const fs::path kOutputPath = kTemplate.data();
  std::ofstream out(kOutputPath, std::ios::binary);
  if(!out)
  {
    throw std::runtime_error("failed to create embedded clang-tidy module file: " + kOutputPath.string());
  }

  out.write(reinterpret_cast<const char *>(checkpp::embedded_clang_tidy_module::kEmbeddedClangTidyModule),
            static_cast<std::streamsize>(checkpp::embedded_clang_tidy_module::kEmbeddedClangTidyModuleSize));
  if(!out)
  {
    throw std::runtime_error("failed to write embedded clang-tidy module file: " + kOutputPath.string());
  }

  return kOutputPath;
}
} // namespace
#endif

fs::path defaultPluginPath()
{
#ifdef CHECKPP_EMBED_CLANG_TIDY_MODULE
  return writeEmbeddedModuleToTempFile();
#else
  return fs::path{"./build/clang-tidy-module/libCompanyClangTidyModule.so"};
#endif
}
