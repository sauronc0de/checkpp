#include "config.hpp"

#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace
{
std::string trim(const std::string &value)
{
  const std::size_t kBegin = value.find_first_not_of(" \t\r\n");
  if(kBegin == std::string::npos)
  {
    return {};
  }

  const std::size_t kEnd = value.find_last_not_of(" \t\r\n");
  return value.substr(kBegin, kEnd - kBegin + 1);
}
} // namespace

bool Config::loadFromFile(const std::string &path, const std::string &ignorePathsPath)
{
  YAML::Node root = YAML::LoadFile(path);
  if(!root["checks"])
  {
    return false;
  }

  for(const auto &item : root["checks"])
  {
    const std::string kCheckName = item.first.as<std::string>();
    const YAML::Node kNode = item.second;

    RuleSetting setting;
    if(kNode["rule_id"]) setting.ruleId_ = kNode["rule_id"].as<std::string>();
    if(kNode["enabled"]) setting.enabled_ = kNode["enabled"].as<bool>();
    if(kNode["severity"]) setting.severity_ = severityFromString(kNode["severity"].as<std::string>());

    rules_[kCheckName] = setting;
  }

  ignoredPathFilters_.clear();
  if(!ignorePathsPath.empty())
  {
    const std::filesystem::path kIgnorePathsPath(ignorePathsPath);
    std::ifstream ignorePathsFile(kIgnorePathsPath);
    if(ignorePathsFile.is_open())
    {
      std::string line;
      while(std::getline(ignorePathsFile, line))
      {
        const std::string kTrimmedLine = trim(line);
        if(kTrimmedLine.empty() || kTrimmedLine.rfind("#", 0) == 0)
        {
          continue;
        }

        ignoredPathFilters_.push_back(kTrimmedLine);
      }
    }
  }

  return true;
}

RuleSetting Config::getRule(const std::string &checkName) const
{
  const auto kIt = rules_.find(checkName);
  if(kIt != rules_.end())
  {
    return kIt->second;
  }
  return {};
}

std::vector<std::string> Config::enabledChecks() const
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

const std::vector<std::string> &Config::ignoredPathFilters() const
{
  return ignoredPathFilters_;
}
