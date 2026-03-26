#include "runner.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace
{
const char *g_kReset = "\033[0m";
const char *g_kBold = "\033[1m";
const char *g_kRed = "\033[31m";
const char *g_kYellow = "\033[33m";
const char *g_kBlue = "\033[34m";
const char *g_kGray = "\033[90m";

std::string shellQuote(const std::string &s)
{
  std::string out = "'";
  for(char c : s)
  {
    if(c == '\'')
    {
      out += "'\\''";
    }
    else
    {
      out += c;
    }
  }
  out += "'";
  return out;
}

bool isSourceFile(const fs::path &path)
{
  static const std::unordered_set<std::string> kExts = {
      ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"};
  return kExts.count(path.extension().string()) > 0;
}

bool shouldIgnorePath(const fs::path &path, const std::vector<std::string> &ignoredPathFilters)
{
  const std::string kNormalizedPath = path.lexically_normal().generic_string();
  for(const std::string &ignoredPathFilter : ignoredPathFilters)
  {
    if(!ignoredPathFilter.empty() && kNormalizedPath.find(ignoredPathFilter) != std::string::npos)
    {
      return true;
    }
  }

  return false;
}

const char *colorForSeverity(Severity s)
{
  switch(s)
  {
  case Severity::Error:
    return g_kRed;
  case Severity::Warning:
    return g_kYellow;
  case Severity::Info:
    return g_kBlue;
  case Severity::Hidden:
    return g_kGray;
  }
  return g_kGray;
}
} // namespace

Runner::Runner(const Config &config)
    : config_(config)
{
}

std::vector<fs::path> Runner::collectFiles(const fs::path &root) const
{
  std::vector<fs::path> kFiles;
  fs::recursive_directory_iterator iterator(root);
  const fs::recursive_directory_iterator kEnd;
  while(iterator != kEnd)
  {
    const fs::directory_entry &entry = *iterator;
    if(entry.is_directory() && shouldIgnorePath(entry.path(), config_.ignoredPathFilters()))
    {
      iterator.disable_recursion_pending();
      ++iterator;
      continue;
    }

    if(shouldIgnorePath(entry.path(), config_.ignoredPathFilters()))
    {
      ++iterator;
      continue;
    }

    if(entry.is_regular_file() && isSourceFile(entry.path()))
    {
      kFiles.push_back(entry.path());
    }

    ++iterator;
  }
  std::sort(kFiles.begin(), kFiles.end());
  return kFiles;
}

std::string Runner::buildChecksArgument() const
{
  const auto kChecks = config_.enabledChecks();
  std::ostringstream oss;
  oss << "-*";
  for(const auto &check : kChecks)
  {
    oss << "," << check;
  }
  return oss.str();
}

std::vector<Finding> Runner::runForFile(const fs::path &file,
                                        const fs::path &compileDbDir,
                                        const fs::path &pluginPath,
                                        bool &isCommandFailed) const
{
  std::vector<Finding> findings;
  isCommandFailed = false;

  const std::string kChecksArg = buildChecksArgument();
  std::ostringstream cmd;
  cmd << "clang-tidy "
      << shellQuote(file.string()) << " "
      << "-p=" << shellQuote(compileDbDir.string()) << " "
      << "-checks=" << shellQuote(kChecksArg) << " "
      << "--load=" << shellQuote(pluginPath.string()) << " 2>&1";

  FILE *pipe = popen(cmd.str().c_str(), "r");
  if(pipe == nullptr)
  {
    return findings;
  }

  std::array<char, 4096> buffer{};
  std::string output;
  while(fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
  {
    output += buffer.data();
  }
  const int kStatus = pclose(pipe);

  std::regex kLinePattern(R"((.+):(\d+):(\d+):\s+(warning|error|note):\s+(.*)\s+\[([^\]]+)\])");
  std::smatch match;

  std::istringstream iss(output);
  std::string line;
  while(std::getline(iss, line))
  {
    if(!std::regex_match(line, match, kLinePattern))
    {
      continue;
    }

    Finding finding;
    finding.path_ = match[1].str();
    finding.line_ = std::stoi(match[2].str());
    finding.column_ = std::stoi(match[3].str());
    finding.message_ = match[5].str();
    finding.checkName_ = match[6].str();

    const RuleSetting kRule = config_.getRule(finding.checkName_);
    finding.ruleId_ = kRule.ruleId_;
    finding.severity_ = kRule.severity_;

    if(!kRule.enabled_ || kRule.severity_ == Severity::Hidden)
    {
      continue;
    }

    findings.push_back(std::move(finding));
  }

  if(kStatus != 0)
  {
    isCommandFailed = true;
    std::cerr << "clang-tidy failed for " << file << "\n";
    if(!output.empty())
    {
      std::cerr << output << "\n";
    }
    if(WIFEXITED(kStatus))
    {
      std::cerr << "clang-tidy exit code: " << WEXITSTATUS(kStatus) << "\n";
    }
    else if(WIFSIGNALED(kStatus))
    {
      std::cerr << "clang-tidy terminated by signal: " << WTERMSIG(kStatus) << "\n";
    }
    return findings;
  }

  return findings;
}

void Runner::printFindings(const std::vector<Finding> &findings,
                           const std::vector<fs::path> &checkedFiles) const
{
  int errors = 0;
  int warnings = 0;
  int infos = 0;

  std::unordered_map<std::string, std::vector<const Finding *>> findingsByFile;
  for(const auto &finding : findings)
  {
    findingsByFile[finding.path_.lexically_normal().generic_string()].push_back(&finding);
  }

  for(const auto &f : findings)
  {
    if(f.severity_ == Severity::Error)
      ++errors;
    else if(f.severity_ == Severity::Warning)
      ++warnings;
    else if(f.severity_ == Severity::Info)
      ++infos;

    const char *color = colorForSeverity(f.severity_);
    std::cout << color << g_kBold
              << "[" << toString(f.severity_) << "]"
              << g_kReset << "  ";

    std::cout << g_kBold
              << "Rule " << (f.ruleId_.empty() ? "?" : f.ruleId_)
              << g_kReset << "  ";

    std::cout << color << f.checkName_ << g_kReset << "\n";
    std::cout << "         " << f.path_.string() << ":" << f.line_ << ":" << f.column_ << "\n";
    std::cout << "         " << f.message_ << "\n\n";
  }

  std::cout << g_kBold << "Summary" << g_kReset << "\n";
  std::cout << "  " << g_kRed << "Errors:   " << errors << g_kReset << "\n";
  std::cout << "  " << g_kYellow << "Warnings: " << warnings << g_kReset << "\n";
  std::cout << "  " << g_kBlue << "Infos:    " << infos << g_kReset << "\n";

  std::vector<std::string> filesWithFindings;
  std::vector<std::string> filesWithoutFindings;
  for(const auto &file : checkedFiles)
  {
    const std::string kFile = file.lexically_normal().generic_string();
    if(findingsByFile.count(kFile) > 0)
    {
      filesWithFindings.push_back(kFile);
    }
    else
    {
      filesWithoutFindings.push_back(kFile);
    }
  }

  if(!filesWithFindings.empty())
  {
    std::cout << g_kBold << "Files with findings" << g_kReset << "\n";
    for(const auto &file : filesWithFindings)
    {
      std::cout << "  " << file << "\n";
      for(const Finding *finding : findingsByFile[file])
      {
        const char *color = colorForSeverity(finding->severity_);
        std::cout << "    " << color << g_kBold
                  << "[" << toString(finding->severity_) << "]"
                  << g_kReset << "  ";

        std::cout << g_kBold
                  << "Rule " << (finding->ruleId_.empty() ? "?" : finding->ruleId_)
                  << g_kReset << "  ";

        std::cout << color << finding->checkName_ << g_kReset << "\n";
        std::cout << "      " << finding->line_ << ":" << finding->column_ << "\n";
        std::cout << "      " << finding->message_ << "\n";
      }
      std::cout << "\n";
    }
  }

  if(!filesWithoutFindings.empty())
  {
    std::cout << g_kBold << "Files without findings" << g_kReset << "\n";
    for(const auto &file : filesWithoutFindings)
    {
      std::cout << "  " << file << "\n";
    }
  }
}

int Runner::run(const fs::path &projectRoot,
                const fs::path &compileDbDir,
                const fs::path &pluginPath) const
{
  const auto kFiles = collectFiles(projectRoot);
  if(kFiles.empty())
  {
    std::cout << "No C++ files found.\n";
    return 0;
  }

  std::vector<Finding> findings;
  bool isCommandFailed = false;
  bool hasCommandFailure = false;
  std::cout << g_kBold << "Scanning files" << g_kReset << "\n";
  for(const auto &file : kFiles)
  {
    std::cout << "\r  " << file.string() << "                              " << std::flush;
    auto current = runForFile(file, compileDbDir, pluginPath, isCommandFailed);
    findings.insert(findings.end(), current.begin(), current.end());
    if(isCommandFailed)
    {
      hasCommandFailure = true;
      isCommandFailed = false;
    }
  }

  std::cout << "\r" << std::string(80, ' ') << "\r";

  std::sort(findings.begin(), findings.end(), [](const Finding &a, const Finding &b) {
    if(a.path_ != b.path_) return a.path_.string() < b.path_.string();
    if(a.line_ != b.line_) return a.line_ < b.line_;
    return a.column_ < b.column_;
  });

  printFindings(findings, kFiles);

  if(hasCommandFailure)
  {
    return 1;
  }

  return std::any_of(findings.begin(), findings.end(), [](const Finding &f) {
    return f.severity_ == Severity::Error;
  })
             ? 2
             : 0;
}
