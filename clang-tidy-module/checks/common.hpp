#pragma once

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

inline auto hasBooleanPrefix(const std::string &name) -> bool
{
  return name.compare(0, 2, "is") == 0 || name.compare(0, 3, "has") == 0 ||
         name.compare(0, 3, "can") == 0 ||
         name.compare(0, 6, "should") == 0;
}
