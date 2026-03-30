#pragma once

#include "severity.hpp"
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
  bool loadFromFile(const std::string &path,
                    const std::string &ignorePathsPath = {});
  RuleSetting getRule(const std::string &checkName) const;
  const std::vector<std::string> &clangTidyChecks() const;
  std::vector<std::string> enabledChecks() const;
  const std::vector<std::string> &ignoredPathFilters() const;

private:
  std::vector<std::string> clangTidyChecks_;
  std::vector<std::string> ignoredPathFilters_;
  std::unordered_map<std::string, RuleSetting> rules_;
};
