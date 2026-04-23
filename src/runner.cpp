#include "runner.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <atomic>
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

auto isImplementationFile(const fs::path &path) -> bool
{
  static const std::unordered_set<std::string> kExts = {".c", ".cc", ".cpp",
                                                        ".cxx"};
  return kExts.contains(path.extension().string());
}

auto isHeaderFile(const fs::path &path) -> bool
{
  static const std::unordered_set<std::string> kExts = {".h", ".hh", ".hpp"};
  return kExts.contains(path.extension().string());
}

auto isCheckableFile(const fs::path &path) -> bool
{
  return isImplementationFile(path) || isHeaderFile(path);
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

auto printProgress(std::size_t index, std::size_t total, const fs::path &file)
    -> void
{
  const double kPercent = total == 0
                              ? 100.0
                              : (static_cast<double>(index) * 100.0) /
                                    static_cast<double>(total);

  std::ostringstream oss;
  oss << '\r' << "\033[2K" << g_kBold << "Scanning files" << g_kReset << "  "
      << "[" << index << "/" << total << "] " << std::fixed
      << std::setprecision(0) << kPercent << "%  " << file.string();
  std::cout << oss.str() << std::flush;
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

    if(entry.is_regular_file() && isCheckableFile(entry.path()))
    {
      kFiles.push_back(entry.path());
    }

    ++iterator;
  }
  std::sort(kFiles.begin(), kFiles.end());
  return kFiles;
}

auto Runner::buildChecksArgument(const std::vector<std::string> &checks)
    -> std::string
{
  std::ostringstream oss;
  oss << "-*";
  for(const auto &check : checks) { oss << "," << check; }
  return oss.str();
}

auto Runner::runForFile(const fs::path &file, const RunPaths &paths,
                        bool &isCommandFailed) const -> std::vector<Finding>
{
  std::vector<Finding> findings;
  isCommandFailed = false;

  const std::string &kChecksArg =
      isHeaderFile(file) ? paths.headerChecksArg_ : paths.sourceChecksArg_;
  if(kChecksArg == "-*")
  {
    return findings;
  }
  std::ostringstream cmd;
  cmd << "env ASAN_OPTIONS=verify_asan_link_order=0 clang-tidy "
      << shellQuote(file.string()) << " "
      << "-p=" << shellQuote(paths.compileDbDir_.string()) << " "
      << "-checks=" << shellQuote(kChecksArg) << " ";
  if(paths.configArg_)
  {
    cmd << "-config=" << shellQuote(*paths.configArg_) << " ";
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
    {
      ++errors;
    }
    else if(finding.severity_ == Severity::Warning)
    {
      ++warnings;
    }
    else if(finding.severity_ == Severity::Info)
    {
      ++infos;
    }

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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto Runner::run(const fs::path &projectRoot, const fs::path &compileDbDir,
                 const fs::path &pluginPath) const -> int
{
  const auto kFiles = collectFiles(projectRoot);
  if(kFiles.empty())
  {
    std::cout << "No C/C++ files found.\n";
    return 0;
  }

  std::vector<std::string> kSourceChecks = config_.clangTidyChecks();
  const auto kCompanyChecks = config_.enabledChecks();
  kSourceChecks.insert(kSourceChecks.end(), kCompanyChecks.begin(),
                       kCompanyChecks.end());

  const RunPaths kRunPaths{compileDbDir,
                           pluginPath,
                           buildChecksArgument(kSourceChecks),
                           buildChecksArgument(kCompanyChecks),
                           buildClangTidyConfigArgument(config_)};

  std::vector<Finding> findings;
  std::mutex findingsMutex;
  std::atomic<bool> hasCommandFailure = false;
  std::atomic<std::size_t> nextIndex = 0;
  std::atomic<std::size_t> completed = 0;

  unsigned int kWorkerCount = std::thread::hardware_concurrency();
  if(kWorkerCount == 0)
  {
    kWorkerCount = 1;
  }
  kWorkerCount = static_cast<unsigned int>(
      std::min<std::size_t>(kWorkerCount, kFiles.size()));

  std::cout << g_kBold << "Scanning files" << g_kReset << "\n";
  std::vector<std::thread> workers;
  workers.reserve(kWorkerCount);
  for(unsigned int i = 0; i < kWorkerCount; ++i)
  {
    workers.emplace_back([&]() {
      while(true)
      {
        const std::size_t kIndex = nextIndex.fetch_add(1);
        if(kIndex >= kFiles.size())
        {
          return;
        }

        bool isCommandFailed = false;
        auto current = runForFile(kFiles[kIndex], kRunPaths, isCommandFailed);
        if(!current.empty())
        {
          std::lock_guard<std::mutex> lock(findingsMutex);
          findings.insert(findings.end(), current.begin(), current.end());
        }
        if(isCommandFailed)
        {
          hasCommandFailure.store(true);
        }

        printProgress(completed.fetch_add(1) + 1,
                      kFiles.size(),
                      kFiles[kIndex]);
      }
    });
  }

  for(auto &worker : workers)
  {
    worker.join();
  }

  std::cout << "\r\033[2K\n";

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

  if(hasCommandFailure.load()) { return 1; }

  return std::any_of(findings.begin(), findings.end(),
                     [](const Finding &finding) {
                       return finding.severity_ == Severity::Error;
                     })
             ? 2
             : 0;
}
