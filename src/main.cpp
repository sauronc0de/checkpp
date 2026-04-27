#include "config.hpp"
#include "embedded_clang_tidy_module.hpp"
#include "runner.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef CHECKPP_VERSION
#define CHECKPP_VERSION "unknown"
#endif

namespace fs = std::filesystem;

namespace
{
constexpr std::string_view kVersion = CHECKPP_VERSION;

struct CliOptions
{
  fs::path projectRoot_;
  fs::path compileDbDir_;
  fs::path rulesPath_;
  fs::path pluginPath_;
  fs::path ignorePathsPath_;
  bool verbose_ = false;
  bool plainText_ = false;
};

struct ParsedArguments
{
  std::optional<CliOptions> options_;
  int exitCode_ = 0;
};

auto printShortHelp() -> void
{
  std::cout
      << "Run clang-tidy and company-* checks from a compilation database; "
      << "example: checkpp . ./build/release ./config/rules.yaml "
      << "--ignore-paths "
      << "./config/ignore_paths.txt\n";
}

auto printVersion() -> void
{
  std::cout << kVersion << "\n";
}

auto printUsage(std::ostream &out, const char *programName) -> void
{
  out
      << "SYNOPSIS\n"
      << "    " << programName
      << " [--plugin PATH] [--ignore-paths PATH] [--plain-text|--no-plain-text]"
      << " [--verbose] <project_root> <compile_commands_dir> <rules.yaml>\n"
      << "    " << programName << " [--help|-h] [--short-help] [--version|-v]\n"
      << "DESCRIPTION\n"
      << "    Run checkpp against a project root and compilation database\n"
      << "    using a\n"
      << "    YAML rules file. Findings are written to stdout;\n"
      << "    diagnostics and\n"
      << "    progress updates are written to stderr.\n\n"
      << "    Rich terminal output is enabled automatically when\n"
      << "    stdout/stderr look like interactive terminals. Set\n"
      << "    NO_COLOR=1 or pass --plain-text to\n"
      << "    force plain output.\n\n"
      << "OPTIONS\n"
      << "    --plugin PATH                 Load a clang-tidy module from\n"
      << "                                 PATH\n"
      << "                                 instead of the default\n"
      << "                                 embedded module.\n"
      << "    --ignore-paths PATH           Skip files whose normalized\n"
      << "                                 path contains\n"
      << "                                 any filter listed in PATH.\n"
      << "    --plain-text                  Disable ANSI colors and rich\n"
      << "                                 progress UI.\n"
      << "    --no-plain-text               Re-enable the rich UI after\n"
      << "                                 --plain-text.\n"
      << "    --verbose                     Print validation and scan\n"
      << "                                 diagnostics to stderr.\n"
      << "    -h, --help                    Print this help.\n"
      << "    --short-help                  Print a one-line summary.\n"
      << "    -v, --version                 Print the tool version.\n\n"
      << "EXAMPLES\n"
      << "    " << programName
      << " . ./build/release ./config/rules.yaml\n"
      << "    " << programName
      << " . ./build/release ./config/rules.yaml --ignore-paths\n"
      << "    ./config/ignore_paths.txt\n"
      << "    NO_COLOR=1 " << programName
      << " . ./build/release ./config/rules.yaml\n"
      << "    " << programName
      << " --plain-text . ./build/release ./config/rules.yaml\n\n"
      << "IMPLEMENTATION\n"
      << "    version         " << kVersion << "\n"
      << "    project         checkpp\n"
      << "    location        src/main.cpp\n"
      << "    dependencies    clang-tidy, yaml-cpp, compile_commands.json\n";
}

auto printError(const std::string &message) -> void
{
  std::cerr << "[ERROR] " << message << "\n";
}

auto nextArgumentValue(int &index,
                       int argc,
                       char **argv,
                       std::string_view optionName)
    -> std::optional<std::string>
{
  if(index + 1 >= argc)
  {
    printError(std::string(optionName) + " requires a value");
    return std::nullopt;
  }

  ++index;
  return std::string(argv[index]);
}

auto parsePathOption(const std::string &argument,
                     std::string_view optionName,
                     std::size_t prefixLength,
                     int &index,
                     int argc,
                     char **argv,
                     fs::path &targetPath) -> bool
{
  if(argument == optionName)
  {
    const auto kValue = nextArgumentValue(index, argc, argv, optionName);
    if(!kValue.has_value())
    {
      return false;
    }

    targetPath = *kValue;
    return true;
  }

  if(argument.starts_with(std::string(optionName) + "="))
  {
    targetPath = argument.substr(prefixLength);
    return true;
  }

  return false;
}

auto maybeHandleImmediateCommand(const std::string &argument,
                                 const char *programName)
    -> std::optional<ParsedArguments>
{
  if(argument == "-h" || argument == "--help")
  {
    printUsage(std::cout, programName);
    return ParsedArguments{{}, 0};
  }

  if(argument == "--short-help")
  {
    printShortHelp();
    return ParsedArguments{{}, 0};
  }

  if(argument == "-v" || argument == "--version")
  {
    printVersion();
    return ParsedArguments{{}, 0};
  }

  return std::nullopt;
}

auto maybeHandleOption(const std::string &argument,
                       int &index,
                       int argc,
                       char **argv,
                       CliOptions &options,
                       const char *programName)
    -> std::optional<ParsedArguments>
{
  if(const auto kImmediateCommand =
         maybeHandleImmediateCommand(argument, programName);
     kImmediateCommand.has_value())
  {
    return kImmediateCommand;
  }

  if(argument == "--verbose")
  {
    options.verbose_ = true;
    return ParsedArguments{};
  }

  if(argument == "--plain-text")
  {
    options.plainText_ = true;
    return ParsedArguments{};
  }

  if(argument == "--no-plain-text")
  {
    options.plainText_ = false;
    return ParsedArguments{};
  }

  if(parsePathOption(argument,
                     "--plugin",
                     std::string("--plugin=").size(),
                     index,
                     argc,
                     argv,
                     options.pluginPath_))
  {
    return ParsedArguments{};
  }

  if(argument == "--plugin" && options.pluginPath_.empty())
  {
    return ParsedArguments{{}, 2};
  }

  if(parsePathOption(argument,
                     "--ignore-paths",
                     std::string("--ignore-paths=").size(),
                     index,
                     argc,
                     argv,
                     options.ignorePathsPath_))
  {
    return ParsedArguments{};
  }

  if(argument == "--ignore-paths" && options.ignorePathsPath_.empty())
  {
    return ParsedArguments{{}, 2};
  }

  if(!argument.empty() && argument.front() == '-')
  {
    printError("unknown option: " + argument);
    printUsage(std::cerr, programName);
    return ParsedArguments{{}, 2};
  }

  return std::nullopt;
}

auto regularFileExists(const fs::path &path) -> bool
{
  std::error_code errorCode;
  return fs::is_regular_file(path, errorCode);
}

auto directoryExists(const fs::path &path) -> bool
{
  std::error_code errorCode;
  return fs::is_directory(path, errorCode);
}

auto parseArguments(int argc, char **argv) -> ParsedArguments
{
  CliOptions options;
  std::vector<std::string> positionalArguments;

  for(int index = 1; index < argc; ++index)
  {
    const std::string kArgument = argv[index];
    const auto kOptionResult =
        maybeHandleOption(kArgument, index, argc, argv, options, argv[0]);
    if(kOptionResult.has_value())
    {
      if(kOptionResult->options_.has_value() || kOptionResult->exitCode_ != 0 ||
         kArgument == "-h" || kArgument == "--help" ||
         kArgument == "--short-help" || kArgument == "-v" ||
         kArgument == "--version")
      {
        return *kOptionResult;
      }
      continue;
    }

    positionalArguments.push_back(kArgument);
  }

  if(positionalArguments.empty())
  {
    printUsage(std::cerr, argv[0]);
    return {{}, 1};
  }

  if(positionalArguments.size() != 3)
  {
    printError(
        "expected exactly 3 positional arguments: "
        "<project_root> <compile_commands_dir> <rules.yaml>");
    printUsage(std::cerr, argv[0]);
    return {{}, 2};
  }

  options.projectRoot_ = positionalArguments[0];
  options.compileDbDir_ = positionalArguments[1];
  options.rulesPath_ = positionalArguments[2];
  return {options, 0};
}

auto validateInputs(const CliOptions &options) -> bool
{
  if(!directoryExists(options.projectRoot_))
  {
    printError("project_root is not a readable directory: " +
               options.projectRoot_.string());
    return false;
  }

  if(!directoryExists(options.compileDbDir_))
  {
    printError("compile_commands_dir is not a readable directory: " +
               options.compileDbDir_.string());
    return false;
  }

  const fs::path kCompileCommandsPath =
      options.compileDbDir_ / "compile_commands.json";
  if(!regularFileExists(kCompileCommandsPath))
  {
    printError("compile_commands.json was not found in: " +
               options.compileDbDir_.string());
    return false;
  }

  if(!regularFileExists(options.rulesPath_))
  {
    printError("rules file does not exist: " + options.rulesPath_.string());
    return false;
  }

  if(!options.ignorePathsPath_.empty() &&
     !regularFileExists(options.ignorePathsPath_))
  {
    printError("ignore-paths file does not exist: " +
               options.ignorePathsPath_.string());
    return false;
  }

  if(!options.pluginPath_.empty() && !regularFileExists(options.pluginPath_))
  {
    printError("plugin file does not exist: " + options.pluginPath_.string());
    return false;
  }

  return true;
}
} // namespace

auto main(int argc, char **argv) -> int
{
  try
  {
    auto parsedArguments = parseArguments(argc, argv);
    if(!parsedArguments.options_.has_value())
    {
      return parsedArguments.exitCode_;
    }

    const CliOptions &options = *parsedArguments.options_;

    if(!validateInputs(options))
    {
      return 1;
    }

    fs::path pluginPath =
        options.pluginPath_.empty() ? defaultPluginPath() : options.pluginPath_;
    if(!regularFileExists(pluginPath))
    {
      printError("default plugin file does not exist: " + pluginPath.string());
      return 1;
    }

    Config config;
    std::string configError;
    if(!config.loadFromFile(
           options.rulesPath_.string(), options.ignorePathsPath_, &configError))
    {
      printError("failed to load rules file '" + options.rulesPath_.string() +
                 "': " + configError);
      return 1;
    }

    RunnerOutputOptions outputOptions;
    outputOptions.plainText_ =
        options.plainText_ || std::getenv("NO_COLOR") != nullptr;
    outputOptions.useColor_ = !outputOptions.plainText_;
    outputOptions.verbose_ = options.verbose_;
    outputOptions.interactiveProgress_ = !outputOptions.plainText_;
    outputOptions.plainTextProgress_ = outputOptions.plainText_;

    Runner runner(config, outputOptions);
    return runner.run(options.projectRoot_, options.compileDbDir_, pluginPath);
  }
  catch(const std::exception &exception)
  {
    printError(exception.what());
    return 1;
  }
}
