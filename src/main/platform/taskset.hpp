#pragma once
#include <print>
#include <span>
#include <string>
#include <vector>

namespace rsl::testing::_impl_main {
struct ProcessResult {
  int exit_code;
  std::string stdout_str;
  std::string stderr_str;
};

struct ProgramInvocation {
  std::string program;
  std::vector<std::string> arguments;
};

ProcessResult run_on_cpu(int cpu, std::string const& program, std::span<std::string const> argv);
inline ProcessResult run_on_cpu(int cpu, ProgramInvocation const& invocation){
  std::println("running {} {}", invocation.program, invocation.arguments);
  return run_on_cpu(cpu, invocation.program, invocation.arguments);
}
}