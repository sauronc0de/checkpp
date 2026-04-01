#pragma once

#include "config.hpp"
#include <filesystem>
#include <string>
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

class Runner
{
public:
  explicit Runner(const Config &config);
  auto run(const std::filesystem::path &projectRoot,
           const std::filesystem::path &compileDbDir,
           const std::filesystem::path &pluginPath) const -> int;

private:
  [[nodiscard]] auto collectFiles(const std::filesystem::path &root) const
      -> std::vector<std::filesystem::path>;
  [[nodiscard]] auto buildChecksArgument() const -> std::string;
  struct RunPaths
  {
    std::filesystem::path compileDbDir_;
    std::filesystem::path pluginPath_;
  };
  [[nodiscard]] auto runForFile(const std::filesystem::path &file,
                                const RunPaths &paths,
                                bool &commandFailed) const
      -> std::vector<Finding>;
  static auto printFindings(
      const std::vector<Finding> &findings,
      const std::vector<std::filesystem::path> &checkedFiles) -> void;
  const Config &config_;
};
