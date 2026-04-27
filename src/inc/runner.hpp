#pragma once

#include "config.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct Finding
{
  Severity severity_ = Severity::Warning;
  std::string ruleId_;
  std::string checkName_;
  std::filesystem::path path_;
  int line_ = 0;
  int column_ = 0;
  std::string message_;
};

struct RunnerOutputOptions
{
  bool useColor_ = false;
  bool plainText_ = false;
  bool verbose_ = false;
  bool interactiveProgress_ = false;
  bool plainTextProgress_ = false;
};

class Runner
{
public:
  explicit Runner(const Config &config,
                  RunnerOutputOptions outputOptions = {});
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  auto run(const std::filesystem::path &projectRoot,
           const std::filesystem::path &compileDbDir,
           const std::filesystem::path &pluginPath) const -> int;

private:
  [[nodiscard]] auto collectFiles(const std::filesystem::path &root) const
      -> std::vector<std::filesystem::path>;
  [[nodiscard]] static auto buildChecksArgument(
      const std::vector<std::string> &checks) -> std::string;
  struct RunPaths
  {
    std::filesystem::path compileDbDir_;
    std::filesystem::path pluginPath_;
    std::string sourceChecksArg_;
    std::string headerChecksArg_;
    std::optional<std::string> configArg_;
  };
  [[nodiscard]] auto runForFile(const std::filesystem::path &file,
                                const RunPaths &paths,
                                bool &commandFailed) const
      -> std::vector<Finding>;
  [[nodiscard]] auto scanFiles(const std::vector<std::filesystem::path> &files,
                               const RunPaths &paths) const
      -> std::pair<std::vector<Finding>, bool>;
  static auto printFindings(
      const std::vector<Finding> &findings,
      const std::vector<std::filesystem::path> &checkedFiles,
      const RunnerOutputOptions &outputOptions) -> void;
  const Config &config_;
  RunnerOutputOptions outputOptions_;
};
