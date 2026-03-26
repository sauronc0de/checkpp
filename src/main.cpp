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
    std::cerr << "Usage: checkpp <project_root> <compile_commands_dir> <rules.yaml> [--plugin <plugin_path>] [--ignore-paths <ignore_paths.txt>]\n";
    return 1;
  }

  const fs::path kProjectRoot = argv[1];
  const fs::path kCompileDbDir = argv[2];
  const fs::path kRulesPath = argv[3];
  fs::path kPluginPath = defaultPluginPath();
  std::string kIgnorePathsPath;

  for(int i = 4; i < argc; ++i)
  {
    const std::string kArg = argv[i];
    if(kArg == "--plugin")
    {
      if(i + 1 >= argc)
      {
        std::cerr << "Missing value for --plugin\n";
        return 1;
      }
      kPluginPath = argv[++i];
    }
    else if(kArg == "--ignore-paths")
    {
      if(i + 1 >= argc)
      {
        std::cerr << "Missing value for --ignore-paths\n";
        return 1;
      }
      kIgnorePathsPath = argv[++i];
    }
    else
    {
      std::cerr << "Unknown argument: " << kArg << "\n";
      return 1;
    }
  }

  Config config;
  if(!config.loadFromFile(kRulesPath.string(), kIgnorePathsPath))
  {
    std::cerr << "Failed to load rules file: " << kRulesPath << "\n";
    return 1;
  }

  Runner runner(config);
  return runner.run(kProjectRoot, kCompileDbDir, kPluginPath);
}
