#include "runner.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sys/wait.h>
#include <regex>
#include <sstream>
#include <ranges>
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

auto shellQuote(const std::string &input) -> std::string
{
  std::string out = "'";
  for(char character : input)
  {
    if(character == '\'')
    {
      out += "'\\''";
    }
    else
    {
      out += character;
    }
  }
  out += "'";
  return out;
}

auto buildClangTidyConfigArgument(const Config &config)
    -> std::optional<std::string>
{
  const RuleSetting kRule = config.getRule("company-line-length");
  if(!kRule.maxLength_)
  {
    return std::nullopt;
  }

  std::ostringstream oss;
  oss << "{CheckOptions: {company-line-length.MaxLength: " << *kRule.maxLength_
      << "}}";
  return oss.str();
}

auto isSourceFile(const fs::path &path) -> bool
{
  static const std::unordered_set<std::string> kExts = {
      ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"};
  return kExts.contains(path.extension().string());
}

auto shouldIgnorePath(const fs::path &path,
                      const std::vector<std::string> &ignoredPathFilters)
    -> bool
{
  const std::string kNormalizedPath = path.lexically_normal().generic_string();
  return std::ranges::any_of(
      ignoredPathFilters,
      [&kNormalizedPath](const std::string &ignoredPathFilter) {
        return !ignoredPathFilter.empty() &&
               kNormalizedPath.find(ignoredPathFilter) != std::string::npos;
      });
}

auto colorForSeverity(Severity severity) -> const char *
{
  switch(severity)
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

Runner::Runner(const Config &config) : config_(config) {}

auto Runner::collectFiles(const fs::path &root) const -> std::vector<fs::path>
{
  std::vector<fs::path> kFiles;
  fs::recursive_directory_iterator iterator(root);
  const fs::recursive_directory_iterator kEnd;
  while(iterator != kEnd)
  {
    const fs::directory_entry &entry = *iterator;
    if(entry.is_directory() &&
       shouldIgnorePath(entry.path(), config_.ignoredPathFilters()))
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

auto Runner::buildChecksArgument() const -> std::string
{
  std::ostringstream oss;
  oss << "-*";
  for(const auto &check : config_.clangTidyChecks()) { oss << "," << check; }
  const auto kChecks = config_.enabledChecks();
  for(const auto &check : kChecks) { oss << "," << check; }
  return oss.str();
}

auto Runner::runForFile(const fs::path &file, const RunPaths &paths,
                        bool &isCommandFailed) const -> std::vector<Finding>
{
  std::vector<Finding> findings;
  isCommandFailed = false;

  const std::string kChecksArg = buildChecksArgument();
  const std::optional<std::string> kConfigArg =
      buildClangTidyConfigArgument(config_);
  std::ostringstream cmd;
  cmd << "env ASAN_OPTIONS=verify_asan_link_order=0 clang-tidy "
      << shellQuote(file.string()) << " "
      << "-p=" << shellQuote(paths.compileDbDir_.string()) << " "
      << "-checks=" << shellQuote(kChecksArg) << " ";
  if(kConfigArg)
  {
    cmd << "-config=" << shellQuote(*kConfigArg) << " ";
  }
  cmd << "--load=" << shellQuote(paths.pluginPath_.string()) << " 2>&1";

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

  std::regex kLinePattern(
      R"((.+):(\d+):(\d+):\s+(warning|error|note):\s+(.*)\s+\[([^\]]+)\])");
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
    if(!output.empty()) { std::cerr << output << "\n"; }
    if(WIFEXITED(kStatus))
    {
      std::cerr << "clang-tidy exit code: " << WEXITSTATUS(kStatus) << "\n";
    }
    else if(WIFSIGNALED(kStatus))
    {
      std::cerr << "clang-tidy terminated by signal: " << WTERMSIG(kStatus)
                << "\n";
    }
    return findings;
  }

  return findings;
}

auto Runner::printFindings(const std::vector<Finding> &findings,
                           const std::vector<fs::path> &checkedFiles) -> void
{
  int errors = 0;
  int warnings = 0;
  int infos = 0;

  std::unordered_map<std::string, std::vector<const Finding *>> findingsByFile;
  for(const auto &finding : findings)
  {
    findingsByFile[finding.path_.lexically_normal().generic_string()].push_back(
        &finding);
  }

  for(const auto &finding : findings)
  {
    if(finding.severity_ == Severity::Error)
      ++errors;
    else if(finding.severity_ == Severity::Warning)
      ++warnings;
    else if(finding.severity_ == Severity::Info)
      ++infos;

    const char *color = colorForSeverity(finding.severity_);
    std::cout << color << g_kBold << "[" << toString(finding.severity_) << "]"
              << g_kReset << "  ";

    std::cout << g_kBold << "Rule "
              << (finding.ruleId_.empty() ? "?" : finding.ruleId_)
              << g_kReset << "  ";

    std::cout << color << finding.checkName_ << g_kReset << "\n";
    std::cout << "         " << finding.path_.string() << ":"
              << finding.line_ << ":" << finding.column_ << "\n";
    std::cout << "         " << finding.message_ << "\n\n";
  }

  std::cout << g_kBold << "Summary" << g_kReset << "\n";
  std::cout << "  " << g_kRed << "Errors:   " << errors << g_kReset << "\n";
  std::cout << "  " << g_kYellow << "Warnings: " << warnings << g_kReset
            << "\n";
  std::cout << "  " << g_kBlue << "Infos:    " << infos << g_kReset << "\n";

  std::vector<std::string> filesWithFindings;
  std::vector<std::string> filesWithoutFindings;
  for(const auto &file : checkedFiles)
  {
    const std::string kFile = file.lexically_normal().generic_string();
    if(findingsByFile.contains(kFile))
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
        std::cout << "    " << color << g_kBold << "["
                  << toString(finding->severity_) << "]" << g_kReset << "  ";

        std::cout << g_kBold << "Rule "
                  << (finding->ruleId_.empty() ? "?" : finding->ruleId_)
                  << g_kReset << "  ";

        std::cout << color << finding->checkName_ << g_kReset << "\n";
        std::cout << "      " << finding->line_ << ":" << finding->column_
                  << "\n";
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

auto Runner::run(const fs::path &projectRoot, const fs::path &compileDbDir,
                 const fs::path &pluginPath) const -> int
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
    std::cout << "\r  " << file.string() << "                              "
              << std::flush;
    RunPaths runPaths{compileDbDir, pluginPath};
    auto current = runForFile(file, runPaths, isCommandFailed);
    findings.insert(findings.end(), current.begin(), current.end());
    if(isCommandFailed)
    {
      hasCommandFailure = true;
      isCommandFailed = false;
    }
  }

  std::cout << "\r" << std::string(80, ' ') << "\r";

  std::sort(findings.begin(), findings.end(),
            [](const Finding &lhs, const Finding &rhs) {
              if(lhs.path_ != rhs.path_)
              {
                return lhs.path_.string() < rhs.path_.string();
              }
              if(lhs.line_ != rhs.line_)
              {
                return lhs.line_ < rhs.line_;
              }
              return lhs.column_ < rhs.column_;
            });

  printFindings(findings, kFiles);

  if(hasCommandFailure) { return 1; }

  return std::any_of(findings.begin(), findings.end(),
                     [](const Finding &finding) {
                       return finding.severity_ == Severity::Error;
                     })
             ? 2
             : 0;
}
