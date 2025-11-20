#pragma once
#include <span>
#include <string>

#include <print>

#ifdef __unix__
#  include <unistd.h>
#else
#endif

namespace rsl::testing::_impl_main {
struct TerminalCommand {
  std::string pending;
#ifdef __unix__
  uintptr_t handle = STDIN_FILENO;
#else
  uintptr_t handle = 0;
#endif

  [[nodiscard]] uintptr_t get_handle() const { return handle; }

  void on_readable(std::span<char const> data) {
    pending.append_range(data);
    std::println("{}", pending);
  }
};
}  // namespace rsl::testing::_impl_main