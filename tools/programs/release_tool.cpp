#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/wait.h>

namespace
{

struct CommandResult
{
  int exit_code = 1;
  std::string output;
};

[[noreturn]] void fail(const std::string &message)
{
  throw std::runtime_error(message);
}

std::string read_file(const std::string &path)
{
  std::ifstream in(path);
  if(!in)
  {
    fail("failed to open file: " + path);
  }
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

CommandResult run_command_capture(const std::string &command)
{
  CommandResult result;
  FILE *pipe = popen(command.c_str(), "r");
  if(!pipe)
  {
    fail("failed to run command: " + command);
  }

  char buffer[256];
  while(fgets(buffer, sizeof(buffer), pipe) != nullptr)
  {
    result.output += buffer;
  }

  const int status = pclose(pipe);
  if(WIFEXITED(status))
  {
    result.exit_code = WEXITSTATUS(status);
  }
  else
  {
    result.exit_code = 1;
  }
  return result;
}

std::string trim(std::string value)
{
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

std::string current_version()
{
  const std::string text = read_file("CMakeLists.txt");
  const std::regex pattern(R"(project\(checkpp\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\b)");
  std::smatch match;
  if(!std::regex_search(text, match, pattern))
  {
    fail("could not find project version in CMakeLists.txt");
  }
  return match[1].str();
}

void print_version()
{
  std::cout << current_version() << '\n';
}

std::string previous_release_ref(const std::string &tag)
{
  const std::string describe_cmd = "git describe --tags --abbrev=0 --match 'v[0-9]*' '" + tag + "^' 2>/dev/null";
  CommandResult result = run_command_capture(describe_cmd);
  if(result.exit_code == 0)
  {
    return trim(result.output);
  }

  result = run_command_capture("git rev-list --max-parents=0 HEAD");
  if(result.exit_code != 0)
  {
    fail("failed to determine first commit");
  }
  return trim(result.output);
}

void assert_no_warning_lines(const std::string &path)
{
  std::ifstream in(path);
  if(!in)
  {
    fail("failed to open file: " + path);
  }

  std::string line;
  std::regex warning_regex(R"(\bwarning\b)", std::regex_constants::icase);
  std::vector<std::string> warning_lines;
  while(std::getline(in, line))
  {
    if(std::regex_search(line, warning_regex))
    {
      warning_lines.push_back(line);
    }
  }

  if(!warning_lines.empty())
  {
    std::cerr << "warning(s) found in build log:\n";
    for(size_t i = 0; i < warning_lines.size() && i < 20; ++i)
    {
      std::cerr << warning_lines[i] << '\n';
    }
    fail("warnings found in build log");
  }
}

std::string strip_ansi(const std::string &text)
{
  std::string result;
  result.reserve(text.size());
  for(size_t i = 0; i < text.size(); ++i)
  {
    if(text[i] == '\x1b' && i + 1 < text.size() && text[i + 1] == '[')
    {
      i += 2;
      while(i < text.size() && (std::isdigit(static_cast<unsigned char>(text[i])) || text[i] == ';'))
      {
        ++i;
      }
      if(i < text.size())
      {
        continue;
      }
    }
    result.push_back(text[i]);
  }
  return result;
}

void verify_checker_output(const std::string &path)
{
  const std::string text = strip_ansi(read_file(path));
  const std::regex errors_regex(R"(Errors:\s*([0-9]+))");
  const std::regex warnings_regex(R"(Warnings:\s*([0-9]+))");

  int errors = -1;
  int warnings = -1;
  std::istringstream stream(text);
  std::string line;
  while(std::getline(stream, line))
  {
    if(!line.empty() && line.back() == '\r')
    {
      line.pop_back();
    }

    std::smatch match;
    if(std::regex_search(line, match, errors_regex))
    {
      errors = std::stoi(match[1].str());
    }
    if(std::regex_search(line, match, warnings_regex))
    {
      warnings = std::stoi(match[1].str());
    }
  }

  if(errors < 0 || warnings < 0)
  {
    fail("could not parse checker summary");
  }

  if(errors != 0 || warnings != 0)
  {
    fail("checker reported errors=" + std::to_string(errors) + " warnings=" + std::to_string(warnings));
  }
}

void print_short_help()
{
  std::cout << "checkpp release helper commands; example: checkpp-release-tool project-version; dependency: git for previous-release-ref\n";
}

void print_usage(std::ostream &out, const std::string &program_name)
{
  out << "SYNOPSIS:\n"
      << "    " << program_name << " [--help|-h] [--version|-v] <command> [args]\n"
      << "DESCRIPTION:\n"
      << "    Provide helper commands for the checkpp release flow.\n"
      << "===============================================================\n"
      << "OPTIONS:\n"
      << "    -h, --help                    Print this help.\n"
      << "    -v, --version                 Print the tool version.\n"
      << "===============================================================\n"
      << "PARAMETERS:\n"
      << "    <command>                     One of the commands listed below.\n"
      << "    <tag>                         Release tag used by previous-release-ref.\n"
      << "                                  Example: v0.1.0\n"
      << "    <log-file>                    Readable build or checker output file used\n"
      << "                                  by the log validation commands.\n"
      << "                                  Example: build/release/build.log\n"
      << "===============================================================\n"
      << "COMMANDS:\n"
      << "    project-version\n"
      << "        Print the project version from CMakeLists.txt.\n"
      << "    previous-release-ref <tag>\n"
      << "        Print the previous release tag or the first commit if none exists.\n"
      << "    assert-no-warning-lines <log-file>\n"
      << "        Fail if the build log contains warning lines.\n"
      << "    verify-checker-output <log-file>\n"
      << "        Fail if the checker summary reports non-zero errors or warnings.\n"
      << "===============================================================\n"
      << "EXAMPLES:\n"
      << "    " << program_name << " project-version\n"
      << "    " << program_name << " previous-release-ref v0.1.0\n"
      << "    " << program_name << " assert-no-warning-lines build/release/build.log\n"
      << "    " << program_name << " verify-checker-output build/release/checkpp_style_check.log\n"
      << "===============================================================\n"
      << "DEPENDENCIES:\n"
      << "    git (for previous-release-ref), readable log files, project root\n"
      << "    CMakeLists.txt\n"
      << "===============================================================\n"
      << "IMPLEMENTATION:\n"
      << "    version         " << current_version() << "\n"
      << "    project         checkpp\n"
      << "    location        tools/programs/release_tool.cpp\n";
}

} // namespace

int main(int argc, char **argv)
{
  try
  {
    if(argc >= 2)
    {
      const std::string first_arg = argv[1];
      if(first_arg == "--help" || first_arg == "-h")
      {
        print_usage(std::cout, argv[0]);
        return 0;
      }
      if(first_arg == "--short-help")
      {
        print_short_help();
        return 0;
      }
      if(first_arg == "--version" || first_arg == "-v")
      {
        print_version();
        return 0;
      }
    }

    if(argc < 2)
    {
      print_usage(std::cerr, argv[0]);
      return 1;
    }

    const std::string command = argv[1];
    if(command == "project-version")
    {
      if(argc != 2)
      {
        fail("project-version takes no arguments");
      }
      std::cout << current_version() << '\n';
      return 0;
    }

    if(command == "previous-release-ref")
    {
      if(argc != 3)
      {
        fail("previous-release-ref requires a tag argument");
      }
      std::cout << previous_release_ref(argv[2]) << '\n';
      return 0;
    }

    if(command == "assert-no-warning-lines")
    {
      if(argc != 3)
      {
        fail("assert-no-warning-lines requires a log file argument");
      }
      assert_no_warning_lines(argv[2]);
      return 0;
    }

    if(command == "verify-checker-output")
    {
      if(argc != 3)
      {
        fail("verify-checker-output requires a log file argument");
      }
      verify_checker_output(argv[2]);
      return 0;
    }
    fail("unknown command: " + command);
  }
  catch(const std::exception &ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
