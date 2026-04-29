#pragma once

#include <filesystem>
#include <optional>
#include <regex>
#include <string>

inline auto isSnakeCase(const std::string &name) -> bool
{
  static const std::regex kPattern(R"(^[a-z][a-z0-9]*(?:_[a-z0-9]+)*$)");
  return std::regex_match(name, kPattern);
}

inline auto isPascalCase(const std::string &name) -> bool
{
  static const std::regex kPattern(R"(^[A-Z][A-Za-z0-9]*$)");
  return std::regex_match(name, kPattern) &&
         name.find('_') == std::string::npos;
}

inline auto isCamelCase(const std::string &name) -> bool
{
  static const std::regex kPattern(R"(^[a-z][A-Za-z0-9]*$)");
  return std::regex_match(name, kPattern) &&
         name.find('_') == std::string::npos;
}

inline auto isUpperSnakeCase(const std::string &name) -> bool
{
  static const std::regex kPattern(R"(^[A-Z][A-Z0-9]*(?:_[A-Z0-9]+)*$)");
  return std::regex_match(name, kPattern);
}

inline auto isModulePrefixedCamelCase(const std::string &name) -> bool
{
  static const std::regex kPattern(
      R"(^[a-z][a-z0-9]*(?:_[a-z0-9]+)*_[a-z][A-Za-z0-9]*$)");
  return std::regex_match(name, kPattern);
}

inline auto moduleNameFromPath(const std::filesystem::path &path)
    -> std::optional<std::string>
{
  const std::filesystem::path kParent = path.parent_path().filename();
  if((kParent == "inc" || kParent == "src") &&
     isSnakeCase(path.parent_path().parent_path().filename().string()))
  {
    return path.parent_path().parent_path().filename().string();
  }

  return std::nullopt;
}

inline auto hasExpectedModulePrefix(const std::string &name,
                                    const std::string &moduleName) -> bool
{
  const std::string kPrefix = moduleName + "_";
  if(name.rfind(kPrefix, 0) != 0)
  {
    return false;
  }

  const std::string kSuffix = name.substr(kPrefix.size());
  return !kSuffix.empty() && isCamelCase(kSuffix);
}

inline auto hasBooleanPrefix(const std::string &name) -> bool
{
  return name.compare(0, 2, "is") == 0 || name.compare(0, 3, "has") == 0 ||
         name.compare(0, 3, "can") == 0 ||
         name.compare(0, 6, "should") == 0;
}
