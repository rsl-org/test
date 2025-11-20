#pragma once
#include <filesystem>
#include <span>
#include <string>

#include <print>
#include "../incremental.hpp"

#ifdef __unix__
#  include <unistd.h>
#else
#endif

namespace rsl::testing::_impl_main {
class TerminalCommand {
  std::string pending;
#ifdef __unix__
  uintptr_t handle = STDIN_FILENO;
#else
  uintptr_t handle = 0;
#endif
  IncrementalRunner* runner;

public:
  explicit TerminalCommand(IncrementalRunner& runner) : runner(&runner) {}
  [[nodiscard]] uintptr_t get_handle() const { return handle; }

  void on_readable(std::span<char const> data) {
    pending.append_range(data);
    std::println("{}", pending);
  }

  
};
}  // namespace rsl::testing::_impl_main