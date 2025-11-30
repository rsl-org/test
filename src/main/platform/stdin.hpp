#pragma once
#include <span>
#include <string>
#include <string_view>

#include <print>
#include "watch.hpp"
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
  IncrementalRunner* runner = nullptr;
  Watcher* watcher = nullptr;

public:
TerminalCommand() = default;
  explicit TerminalCommand(IncrementalRunner& runner, Watcher& watcher)
      : runner(&runner)
      , watcher(&watcher) {}

  [[nodiscard]] uintptr_t get_handle() const { return handle; }

  void on_readable(std::span<char const> data) {
    pending.append_range(data);
    if (auto it = pending.find('\n'); it != pending.npos) {
      dispatch(std::string_view(pending.data(), it));
      pending = pending.substr(it + 1);
    }
  }

  void dispatch(std::string_view data) {
    if (data == "exit") {
      std::exit(0);
      return;
    } else if (data == "list") {
      runner->list_all();
      return;
    } else if (data == "run") {
      runner->run_all();
      return;
    }
    // all other commands require an argument, try splitting
    std::string_view cmd;
    std::string_view argument;
    if (auto it = data.find(' '); it != data.npos) {
      cmd      = data.substr(0, it);
      argument = data.substr(it + 1);
    } else {
      std::println("Missing argument or unrecognized command");
      return;
    }

    if (cmd == "add_watch") {
      watcher->add_watch(argument);
    } else if (cmd == "rm_watch") {
      watcher->rm_watch(argument);
    } else if (cmd == "coverage") {
      bool value = argument == "on";
      if (!value && argument != "off") {
        std::println("Invalid argument - expected `on` or `off`");
      }
      // TODO
    } else {
      std::println("Unrecognized command");
    }
  }
};
}  // namespace rsl::testing::_impl_main