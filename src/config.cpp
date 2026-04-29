#include "config.hpp"

#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace
{
auto trim(const std::string &value) -> std::string
{
  const std::size_t kBegin = value.find_first_not_of(" \t\r\n");
  if(kBegin == std::string::npos)
  {
    return {};
  }

  const std::size_t kEnd = value.find_last_not_of(" \t\r\n");
  return value.substr(kBegin, kEnd - kBegin + 1);
}
auto loadClangTidyChecks(const YAML::Node &root,
                         std::vector<std::string> &clangTidyChecks) -> void
{
  const YAML::Node kChecksNode = root["clang_tidy_checks"];
  if(kChecksNode.IsScalar())
  {
    const auto kCheck = trim(kChecksNode.as<std::string>());
    if(!kCheck.empty())
    {
      clangTidyChecks.push_back(kCheck);
    }
    return;
  }

  if(!kChecksNode.IsSequence())
  {
    return;
  }

  for(const auto &item : kChecksNode)
  {
    const auto kCheck = trim(item.as<std::string>());
    if(!kCheck.empty())
    {
      clangTidyChecks.push_back(kCheck);
    }
  }
}

auto loadRules(const YAML::Node &root,
               std::unordered_map<std::string, RuleSetting> &rules) -> void
{
  if(!root["checks"])
  {
    return;
  }

  for(const auto &item : root["checks"])
  {
    const auto kCheckName = item.first.as<std::string>();
    const YAML::Node kNode = item.second;

    RuleSetting setting;
    if(kNode["rule_id"])
    {
      setting.ruleId_ = kNode["rule_id"].as<std::string>();
    }
    if(kNode["enabled"])
    {
      setting.enabled_ = kNode["enabled"].as<bool>();
    }
    if(kNode["severity"])
    {
      setting.severity_ =
          severityFromString(kNode["severity"].as<std::string>());
    }
    if(kNode["max_length"])
    {
      setting.maxLength_ = kNode["max_length"].as<unsigned>();
    }

    rules[kCheckName] = setting;
  }
}

auto loadIgnorePaths(const std::filesystem::path &ignorePathsPath,
                     std::vector<std::string> &ignoredPathFilters,
                     std::string *errorMessage) -> bool
{
  if(ignorePathsPath.empty())
  {
    return true;
  }

  std::ifstream ignorePathsFile(ignorePathsPath);
  if(!ignorePathsFile.is_open())
  {
    if(errorMessage != nullptr)
    {
      *errorMessage = "failed to open ignore-paths file: " +
                      ignorePathsPath.string();
    }
    return false;
  }

  std::string line;
  while(std::getline(ignorePathsFile, line))
  {
    const auto kTrimmedLine = trim(line);
    if(kTrimmedLine.empty() || kTrimmedLine.front() == '#')
    {
      continue;
    }

    ignoredPathFilters.push_back(kTrimmedLine);
  }

  return true;
}
} // namespace

auto Config::loadFromFile(const std::string &path,
                          const std::filesystem::path &ignorePathsPath,
                          std::string *errorMessage) -> bool
{
  try
  {
    YAML::Node root = YAML::LoadFile(path);
    if(!root["checks"] && !root["clang_tidy_checks"])
    {
      if(errorMessage != nullptr)
      {
        *errorMessage =
            "rules file must define at least one of: checks, clang_tidy_checks";
      }
      return false;
    }

    rules_.clear();
    clangTidyChecks_.clear();
    ignoredPathFilters_.clear();

    loadClangTidyChecks(root, clangTidyChecks_);
    loadRules(root, rules_);
    return loadIgnorePaths(ignorePathsPath, ignoredPathFilters_, errorMessage);
  }
  catch(const YAML::Exception &exception)
  {
    if(errorMessage != nullptr)
    {
      *errorMessage = exception.what();
    }
    return false;
  }
}

auto Config::clangTidyChecks() const -> const std::vector<std::string> &
{
  return clangTidyChecks_;
}

auto Config::hasRule(const std::string &checkName) const -> bool
{
  return rules_.contains(checkName);
}

auto Config::getRule(const std::string &checkName) const -> RuleSetting
{
  const auto kIt = rules_.find(checkName);
  if(kIt != rules_.end())
  {
    return kIt->second;
  }
  return {};
}

auto Config::enabledChecks() const -> std::vector<std::string>
{
  std::vector<std::string> result;
  for(const auto &[name, setting] : rules_)
  {
    if(setting.enabled_ && setting.severity_ != Severity::Hidden)
    {
      result.push_back(name);
    }
  }
  return result;
}

auto Config::ignoredPathFilters() const -> const std::vector<std::string> &
{
  return ignoredPathFilters_;
}
