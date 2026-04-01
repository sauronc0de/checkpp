#pragma once

#include "severity.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct RuleSetting
{
  std::string ruleId_;
  bool enabled_ = true;
  Severity severity_ = Severity::Warning;
  std::optional<unsigned> maxLength_;
};

class Config
{
public:
  auto loadFromFile(const std::string &path,
                    std::filesystem::path ignorePathsPath = {}) -> bool;
  auto getRule(const std::string &checkName) const -> RuleSetting;
  auto clangTidyChecks() const -> const std::vector<std::string> &;
  auto enabledChecks() const -> std::vector<std::string>;
  auto ignoredPathFilters() const -> const std::vector<std::string> &;

private:
  std::vector<std::string> clangTidyChecks_;
  std::vector<std::string> ignoredPathFilters_;
  std::unordered_map<std::string, RuleSetting> rules_;
};
