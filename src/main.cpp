#include "config.hpp"
#include "runner.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
  if(argc < 4)
  {
    std::cerr << "Usage: cpp-style-tool <project_root> <compile_commands_dir> <rules.yaml> [plugin_path]\n";
    return 1;
  }

  const fs::path kProjectRoot = argv[1];
  const fs::path kCompileDbDir = argv[2];
  const fs::path kRulesPath = argv[3];
  const fs::path kPluginPath = (argc >= 5)
                                   ? fs::path(argv[4])
                                   : fs::path("./build/clang-tidy-module/libCompanyClangTidyModule.so");

  Config config;
  if(!config.loadFromFile(kRulesPath.string()))
  {
    std::cerr << "Failed to load rules file: " << kRulesPath << "\n";
    return 1;
  }

  Runner runner(config);
  return runner.run(kProjectRoot, kCompileDbDir, kPluginPath);
}
