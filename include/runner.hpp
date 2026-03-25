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
  int run(const std::filesystem::path &projectRoot,
          const std::filesystem::path &compileDbDir,
          const std::filesystem::path &pluginPath) const;

private:
  std::vector<std::filesystem::path> collectFiles(const std::filesystem::path &root) const;
  std::string buildChecksArgument() const;
  std::vector<Finding> runForFile(const std::filesystem::path &file,
                                  const std::filesystem::path &compileDbDir,
                                  const std::filesystem::path &pluginPath,
                                  bool &commandFailed) const;
  void printFindings(const std::vector<Finding> &findings) const;
  const Config &config_;
};
