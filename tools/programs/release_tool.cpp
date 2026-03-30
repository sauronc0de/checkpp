#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <regex>
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

void write_file(const std::string &path, const std::string &content)
{
  std::ofstream out(path, std::ios::trunc);
  if(!out)
  {
    fail("failed to write file: " + path);
  }
  out << content;
  if(!out)
  {
    fail("failed to flush file: " + path);
  }
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

std::string bump_version(const std::string &release_type)
{
  std::string text = read_file("CMakeLists.txt");
  const std::regex pattern(R"((project\(checkpp\s+VERSION\s+)([0-9]+)\.([0-9]+)\.([0-9]+)(\s+LANGUAGES\s+C\s+CXX\)))");
  std::smatch match;
  if(!std::regex_search(text, match, pattern))
  {
    fail("could not find project version in CMakeLists.txt");
  }

  int major = std::stoi(match[2].str());
  int minor = std::stoi(match[3].str());
  int patch = std::stoi(match[4].str());

  if(release_type == "patch")
  {
    ++patch;
  }
  else if(release_type == "minor")
  {
    ++minor;
    patch = 0;
  }
  else if(release_type == "major")
  {
    ++major;
    minor = 0;
    patch = 0;
  }
  else
  {
    fail("invalid release type: " + release_type);
  }

  const std::string new_version = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
  text.replace(match.position(0), match.length(0), match[1].str() + new_version + match[5].str());
  write_file("CMakeLists.txt", text);
  return new_version;
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
  const std::regex errors_regex(R"(^Errors:\s+([0-9]+))", std::regex_constants::multiline);
  const std::regex warnings_regex(R"(^Warnings:\s+([0-9]+))", std::regex_constants::multiline);

  std::smatch errors_match;
  std::smatch warnings_match;
  if(!std::regex_search(text, errors_match, errors_regex) || !std::regex_search(text, warnings_match, warnings_regex))
  {
    fail("could not parse checker summary");
  }

  const int errors = std::stoi(errors_match[1].str());
  const int warnings = std::stoi(warnings_match[1].str());
  if(errors != 0 || warnings != 0)
  {
    fail("checker reported errors=" + std::to_string(errors) + " warnings=" + std::to_string(warnings));
  }
}

void print_usage()
{
  std::cout << "Usage: checkpp-release-tool <command> [args]\n"
            << "Commands:\n"
            << "  project-version\n"
            << "  bump-version <patch|minor|major>\n"
            << "  previous-release-ref <tag>\n"
            << "  assert-no-warning-lines <log-file>\n"
            << "  verify-checker-output <log-file>\n";
}

} // namespace

int main(int argc, char **argv)
{
  try
  {
    if(argc < 2)
    {
      print_usage();
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

    if(command == "bump-version")
    {
      if(argc != 3)
      {
        fail("bump-version requires one argument: patch, minor, or major");
      }
      std::cout << bump_version(argv[2]) << '\n';
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

    if(command == "--help" || command == "-h")
    {
      print_usage();
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
