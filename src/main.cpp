#include "config.hpp"
#include "embedded_clang_tidy_module.hpp"
#include "runner.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
  if(argc < 4)
  {
    std::cerr << "Usage: checkpp <project_root> <compile_commands_dir> <rules.yaml> [plugin_path]\n";
    return 1;
  }

  const fs::path kProjectRoot = argv[1];
  const fs::path kCompileDbDir = argv[2];
  const fs::path kRulesPath = argv[3];
  const fs::path kPluginPath = (argc >= 5) ? fs::path(argv[4]) : defaultPluginPath();

  Config config;
  if(!config.loadFromFile(kRulesPath.string()))
  {
    std::cerr << "Failed to load rules file: " << kRulesPath << "\n";
    return 1;
  }

  Runner runner(config);
  return runner.run(kProjectRoot, kCompileDbDir, kPluginPath);
}
