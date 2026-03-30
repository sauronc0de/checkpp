#pragma once

#include <regex>
#include <string>

inline bool isSnakeCase(const std::string &name)
{
  static const std::regex kPattern(R"(^[a-z][a-z0-9]*(?:_[a-z0-9]+)*$)");
  return std::regex_match(name, kPattern);
}

inline bool isPascalCase(const std::string &name)
{
  static const std::regex kPattern(R"(^[A-Z][A-Za-z0-9]*$)");
  return std::regex_match(name, kPattern) &&
         name.find('_') == std::string::npos;
}

inline bool isCamelCase(const std::string &name)
{
  static const std::regex kPattern(R"(^[a-z][A-Za-z0-9]*$)");
  return std::regex_match(name, kPattern) &&
         name.find('_') == std::string::npos;
}

inline bool hasBooleanPrefix(const std::string &name)
{
  return name.rfind("is", 0) == 0 || name.rfind("has", 0) == 0 ||
         name.rfind("can", 0) == 0 || name.rfind("should", 0) == 0;
}
