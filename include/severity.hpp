#pragma once

#include <cstdint>
#include <string>

enum class Severity : std::uint8_t
{
  Info,
  Warning,
  Error,
  Hidden
};

inline auto toString(Severity severity) -> std::string
{
  switch(severity)
  {
  case Severity::Info:
    return "INFO";
  case Severity::Warning:
    return "WARNING";
  case Severity::Error:
    return "ERROR";
  case Severity::Hidden:
    return "HIDDEN";
  }
  return "WARNING";
}

inline auto severityFromString(const std::string &value) -> Severity
{
  if(value == "info")
  {
    return Severity::Info;
  }
  if(value == "warning")
  {
    return Severity::Warning;
  }
  if(value == "error")
  {
    return Severity::Error;
  }
  if(value == "hidden")
  {
    return Severity::Hidden;
  }
  return Severity::Warning;
}
