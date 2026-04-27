#include "runner.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <ranges>
#include <regex>
#include <sstream>
#include <string_view>
#include <sys/wait.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <unistd.h>

namespace fs = std::filesystem;

namespace
{
constexpr std::array<std::string_view, 12> kSpinnerFrames = {
    "⠁", "⠃", "⠇", "⠧", "⠷", "⠿",
    "⠷", "⠯", "⠮", "⠟", "⠻", "⠽"};
constexpr std::array<std::string_view, 4> kPlainSpinnerFrames = {"-",
                                                                 "\\",
                                                                 "|",
                                                                 "/"};

const char *g_kReset = "\033[0m";
const char *g_kBold = "\033[1m";
const char *g_kRed = "\033[31m";
const char *g_kYellow = "\033[33m";
const char *g_kBlue = "\033[34m";
const char *g_kGray = "\033[90m";
const char *g_kAccent = "\033[38;5;45m";
const char *g_kBar = "\033[38;5;111m";

auto styleEnabled(const RunnerOutputOptions &outputOptions) -> bool
{
  return outputOptions.useColor_ && !outputOptions.plainText_;
}

auto styled(const RunnerOutputOptions &outputOptions, std::string_view text,
            const char *color, bool isBold = false) -> std::string
{
  if(!styleEnabled(outputOptions))
  {
    return std::string(text);
  }

  std::string result;
  if(isBold)
  {
    result += g_kBold;
  }
  result += color;
  result += text;
  result += g_kReset;
  return result;
}

auto supportsInteractiveStream(int fileDescriptor) -> bool
{
  const char *forcedColor = std::getenv("CLICOLOR_FORCE");
  if(forcedColor != nullptr && std::string_view(forcedColor) != "0")
  {
    return true;
  }

  if(std::getenv("NO_COLOR") != nullptr)
  {
    return false;
  }

  if(::isatty(fileDescriptor) == 0)
  {
    return false;
  }

  const char *term = std::getenv("TERM");
  return term != nullptr && std::string_view(term) != "dumb";
}

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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto formatProgressBar(std::size_t filled, std::size_t width,
                       const RunnerOutputOptions &outputOptions) -> std::string
{
  std::string bar;
  bar.reserve(width);
  for(std::size_t index = 0; index < filled; ++index)
  {
    bar += outputOptions.plainText_ ? "#" : "█";
  }
  for(std::size_t index = filled; index < width; ++index)
  {
    bar += outputOptions.plainText_ ? "-" : "░";
  }
  return bar;
}

auto progressFrame(std::size_t index,
                   const RunnerOutputOptions &outputOptions) -> std::string
{
  if(outputOptions.plainText_)
  {
    return std::string(kPlainSpinnerFrames[index % kPlainSpinnerFrames.size()]);
  }

  return std::string(kSpinnerFrames[index % kSpinnerFrames.size()]);
}

auto progressEnabled(const RunnerOutputOptions &outputOptions) -> bool
{
  return outputOptions.interactiveProgress_ || outputOptions.plainTextProgress_;
}

auto renderProgress(std::size_t completed, std::size_t frameIndex,
                    std::size_t total, const fs::path &file,
                    const RunnerOutputOptions &outputOptions,
                    const std::chrono::steady_clock::time_point &startTime)
    -> void
{
  if(!progressEnabled(outputOptions))
  {
    return;
  }

  constexpr std::size_t kBarWidth = 24;
  const double kPercent = total == 0
                              ? 100.0
                              : (static_cast<double>(completed) * 100.0) /
                                    static_cast<double>(total);
  const std::size_t kFilled =
      total == 0 ? kBarWidth : (completed * kBarWidth) / total;
  const auto kElapsed = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - startTime);

  if(outputOptions.plainTextProgress_)
  {
    std::cerr << "[INFO] " << progressFrame(frameIndex, outputOptions)
              << " Scanning ["
              << formatProgressBar(kFilled, kBarWidth, outputOptions) << "] "
              << std::setw(3) << std::lround(kPercent) << "% [" << completed
              << '/' << total << "] " << file.filename().string() << " ("
              << kElapsed.count() << "s elapsed)\n"
              << std::flush;
    return;
  }

  std::ostringstream oss;
  oss << '\r';
  if(styleEnabled(outputOptions))
  {
    oss << "\033[2K";
  }
  oss << styled(outputOptions,
                progressFrame(frameIndex, outputOptions),
                g_kAccent,
                true)
      << ' ' << styled(outputOptions, "Scanning", g_kAccent, true) << ' '
      << '['
      << styled(outputOptions,
                formatProgressBar(kFilled, kBarWidth, outputOptions),
                g_kBar)
      << "] " << std::setw(3) << std::lround(kPercent) << "% [" << completed
      << '/' << total << "] " << file.filename().string() << ' '
      << styled(outputOptions,
                "(" + std::to_string(kElapsed.count()) + "s elapsed)",
                g_kGray);
  std::cerr << oss.str() << std::flush;
}

auto clearProgressLine(const RunnerOutputOptions &outputOptions) -> void
{
  if(!outputOptions.interactiveProgress_)
  {
    return;
  }

  std::cerr << '\r';
  if(styleEnabled(outputOptions))
  {
    std::cerr << "\033[2K";
  }
  std::cerr << "\n";
}

auto determineWorkerCount(std::size_t fileCount) -> unsigned int
{
  constexpr unsigned int kMaxWorkerCount = 4;

  unsigned int kWorkerCount = std::thread::hardware_concurrency();
  if(kWorkerCount == 0)
  {
    kWorkerCount = 1;
  }

  kWorkerCount = std::min(kWorkerCount, kMaxWorkerCount);
  kWorkerCount = static_cast<unsigned int>(
      std::min<std::size_t>(kWorkerCount, fileCount));
  return kWorkerCount == 0 ? 1U : kWorkerCount;
}

struct RunLogContext
{
  std::size_t fileCount_ = 0;
  unsigned int workerCount_ = 0;
  fs::path pluginPath_;
  fs::path compileDbDir_;
};

struct ScanProgressState
{
  std::atomic<bool> running_ = true;
  std::atomic<std::size_t> completed_ = 0;
  std::mutex currentFileMutex_;
  fs::path currentFile_;
};

auto startProgressRenderer(ScanProgressState &progressState,
                           std::size_t totalFiles,
                           const RunnerOutputOptions &outputOptions,
                           const std::chrono::steady_clock::time_point
                               &startTime)
    -> std::thread
{
  const auto kRefreshInterval = outputOptions.plainTextProgress_
                                    ? std::chrono::milliseconds(500)
                                    : std::chrono::milliseconds(100);

  return std::thread([&progressState,
                      totalFiles,
                      &outputOptions,
                      startTime,
                      kRefreshInterval]() {
    std::size_t frameIndex = 0;
    while(progressState.running_.load())
    {
      fs::path currentFile;
      {
        std::lock_guard<std::mutex> lock(progressState.currentFileMutex_);
        currentFile = progressState.currentFile_;
      }

      renderProgress(progressState.completed_.load(),
                     frameIndex++,
                     totalFiles,
                     currentFile,
                     outputOptions,
                     startTime);
      std::this_thread::sleep_for(kRefreshInterval);
    }
  });
}

auto logRunConfiguration(const RunnerOutputOptions &outputOptions,
                         const RunLogContext &context) -> void
{
  if(!outputOptions.verbose_)
  {
    return;
  }

  std::cerr << "[INFO] checking " << context.fileCount_ << " files with "
            << context.workerCount_ << " worker(s)\n";
  std::cerr << "[INFO] using plugin: " << context.pluginPath_ << "\n";
  std::cerr << "[INFO] compile commands: "
            << (context.compileDbDir_ / "compile_commands.json") << "\n";
}

auto sortFindings(std::vector<Finding> &findings) -> void
{
  auto byLocation = [](const Finding &lhs, const Finding &rhs) {
    if(lhs.path_ != rhs.path_)
    {
      return lhs.path_.string() < rhs.path_.string();
    }
    if(lhs.line_ != rhs.line_)
    {
      return lhs.line_ < rhs.line_;
    }
    return lhs.column_ < rhs.column_;
  };

  std::sort(findings.begin(), findings.end(), byLocation);
}

auto exitCodeForRun(const std::vector<Finding> &findings,
                    bool hasCommandFailure) -> int
{
  if(hasCommandFailure)
  {
    return 1;
  }

  return std::any_of(findings.begin(),
                     findings.end(),
                     [](const Finding &finding) {
                       return finding.severity_ == Severity::Error;
                     })
             ? 2
             : 0;
}
} // namespace

Runner::Runner(const Config &config, RunnerOutputOptions outputOptions)
    : config_(config), outputOptions_(outputOptions)
{
  outputOptions_.useColor_ = outputOptions_.useColor_ &&
                             supportsInteractiveStream(STDOUT_FILENO);
  outputOptions_.interactiveProgress_ =
      outputOptions_.interactiveProgress_ && !outputOptions_.plainText_ &&
      supportsInteractiveStream(STDERR_FILENO);
  outputOptions_.plainTextProgress_ = outputOptions_.plainTextProgress_ &&
                                      outputOptions_.plainText_;
}

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
  for(const auto &check : checks)
  {
    oss << "," << check;
  }
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
      << shellQuote(file.string()) << ' '
      << "-p=" << shellQuote(paths.compileDbDir_.string()) << ' '
      << "-checks=" << shellQuote(kChecksArg) << ' ';
  if(paths.configArg_)
  {
    cmd << "-config=" << shellQuote(*paths.configArg_) << ' ';
  }
  cmd << "--load=" << shellQuote(paths.pluginPath_.string()) << " 2>&1";

  FILE *pipe = popen(cmd.str().c_str(), "r");
  if(pipe == nullptr)
  {
    isCommandFailed = true;
    std::cerr << "failed to execute clang-tidy for " << file << "\n";
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
      std::cerr << "clang-tidy terminated by signal: " << WTERMSIG(kStatus)
                << "\n";
    }
    return findings;
  }

  return findings;
}

auto Runner::scanFiles(const std::vector<fs::path> &files,
                       const RunPaths &paths) const
    -> std::pair<std::vector<Finding>, bool>
{
  std::vector<Finding> findings;
  std::mutex findingsMutex;
  std::atomic<bool> hasCommandFailure = false;
  std::atomic<std::size_t> nextIndex = 0;
  const unsigned int kWorkerCount = determineWorkerCount(files.size());
  const auto kStartTime = std::chrono::steady_clock::now();
  ScanProgressState progressState;
  if(!files.empty())
  {
    progressState.currentFile_ = files.front();
  }

  std::optional<std::thread> progressRenderer;
  if(progressEnabled(outputOptions_))
  {
    progressRenderer.emplace(startProgressRenderer(progressState,
                                                   files.size(),
                                                   outputOptions_,
                                                   kStartTime));
  }

  std::vector<std::thread> workers;
  workers.reserve(kWorkerCount);
  for(unsigned int index = 0; index < kWorkerCount; ++index)
  {
    workers.emplace_back([&]() {
      while(true)
      {
        const std::size_t kIndex = nextIndex.fetch_add(1);
        if(kIndex >= files.size())
        {
          return;
        }

        {
          std::lock_guard<std::mutex> lock(progressState.currentFileMutex_);
          progressState.currentFile_ = files[kIndex];
        }

        bool isCommandFailed = false;
        auto current = runForFile(files[kIndex], paths, isCommandFailed);
        if(!current.empty())
        {
          std::lock_guard<std::mutex> lock(findingsMutex);
          findings.insert(findings.end(), current.begin(), current.end());
        }
        if(isCommandFailed)
        {
          hasCommandFailure.store(true);
        }

        progressState.completed_.fetch_add(1);
      }
    });
  }

  for(auto &worker : workers)
  {
    worker.join();
  }

  progressState.completed_.store(files.size());
  progressState.running_.store(false);
  if(progressRenderer.has_value())
  {
    progressRenderer->join();
  }

  if(outputOptions_.interactiveProgress_)
  {
    clearProgressLine(outputOptions_);
  }
  else if(outputOptions_.plainTextProgress_ || outputOptions_.verbose_)
  {
    std::cerr << "[INFO] completed scan of " << files.size() << " files\n";
  }

  return {std::move(findings), hasCommandFailure.load()};
}

auto Runner::printFindings(const std::vector<Finding> &findings,
                           const std::vector<fs::path> &checkedFiles,
                           const RunnerOutputOptions &outputOptions) -> void
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
    std::cout << styled(outputOptions,
                        "[" + std::string(toString(finding.severity_)) + "]",
                        color,
                        true)
              << "  " << styled(outputOptions, "Rule", g_kGray, true) << ' '
              << (finding.ruleId_.empty() ? "?" : finding.ruleId_) << "  "
              << styled(outputOptions, finding.checkName_, color) << "\n";
    std::cout << "         " << finding.path_.string() << ':' << finding.line_
              << ':' << finding.column_ << "\n";
    std::cout << "         " << finding.message_ << "\n\n";
  }

  std::cout << styled(outputOptions, "Summary", g_kAccent, true) << "\n";
  std::cout << "  "
            << styled(outputOptions,
                      "Errors:   " + std::to_string(errors),
                      g_kRed)
            << "\n";
  std::cout << "  "
            << styled(outputOptions,
                      "Warnings: " + std::to_string(warnings),
                      g_kYellow)
            << "\n";
  std::cout << "  "
            << styled(outputOptions,
                      "Infos:    " + std::to_string(infos),
                      g_kBlue)
            << "\n";

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
    std::cout << styled(outputOptions, "Files with findings", g_kAccent, true)
              << "\n";
    for(const auto &file : filesWithFindings)
    {
      std::cout << "  " << file << "\n";
      for(const Finding *finding : findingsByFile[file])
      {
        const char *color = colorForSeverity(finding->severity_);
        std::cout << "    "
                  << styled(outputOptions,
                            "[" +
                                std::string(toString(finding->severity_)) +
                                "]",
                            color,
                            true)
                  << "  " << styled(outputOptions, "Rule", g_kGray, true)
                  << ' ' << (finding->ruleId_.empty() ? "?" : finding->ruleId_)
                  << "  " << styled(outputOptions, finding->checkName_, color)
                  << "\n";
        std::cout << "      " << finding->line_ << ':' << finding->column_
                  << "\n";
        std::cout << "      " << finding->message_ << "\n";
      }
      std::cout << "\n";
    }
  }

  if(!filesWithoutFindings.empty())
  {
    std::cout
        << styled(outputOptions, "Files without findings", g_kAccent, true)
        << "\n";
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

  const unsigned int kWorkerCount = determineWorkerCount(kFiles.size());
  const RunLogContext kLogContext{kFiles.size(),
                                  kWorkerCount,
                                  pluginPath,
                                  compileDbDir};
  logRunConfiguration(outputOptions_, kLogContext);

  auto [findings, hasCommandFailure] = scanFiles(kFiles, kRunPaths);
  sortFindings(findings);
  printFindings(findings, kFiles, outputOptions_);
  return exitCodeForRun(findings, hasCommandFailure);
}
