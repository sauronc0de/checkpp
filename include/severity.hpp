#pragma once

#include <string>

enum class Severity
{
    Info,
    Warning,
    Error,
    Hidden
};

inline std::string toString(Severity s)
{
    switch (s)
    {
        case Severity::Info: return "INFO";
        case Severity::Warning: return "WARNING";
        case Severity::Error: return "ERROR";
        case Severity::Hidden: return "HIDDEN";
    }
    return "WARNING";
}

inline Severity severityFromString(const std::string& value)
{
    if (value == "info") return Severity::Info;
    if (value == "warning") return Severity::Warning;
    if (value == "error") return Severity::Error;
    if (value == "hidden") return Severity::Hidden;
    return Severity::Warning;
}
