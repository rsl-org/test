#include <string>
#include <ranges>
#include <vector>
#include <functional>
#include <chrono>
#include <print>

#include <rsl/source_location>
#include <rsl/testing/assert.hpp>
#include <rsl/testing/test.hpp>
#include <rsl/testing/result.hpp>
#include <rsl/testing/output.hpp>
#include <rsl/testing/util.hpp>

// #include <cpptrace/basic.hpp>
// #include <cpptrace/utils.hpp>

#include "capture.hpp"
#include <rsl/coverage/hooks.hpp>

namespace {
// void cleanup_frames(cpptrace::stacktrace& trace, std::string_view test_name) {
//   std::vector<cpptrace::stacktrace_frame> frames;
//   for (auto const& frame : trace.frames | std::views::drop(1)) {
//     frames.push_back(frame);
//     if (cpptrace::prune_symbol(frame.symbol) == test_name) {
//       break;
//     }
//   }
//   trace.frames = frames;
// }
}  // namespace

// void failure_handler(libassert::assertion_info const& info) {
//   // libassert::enable_virtual_terminal_processing_if_needed();  // for terminal colors on windows
//   constexpr bool Colorize = false;
//   auto width              = libassert::terminal_width(libassert::stderr_fileno);
//   const auto& scheme  = Colorize ? libassert::get_color_scheme() : libassert::color_scheme::blank;
//   std::string message = std::string(info.action()) + " at " + info.location() + ":";
//   if (info.message) {
//     message += " " + *info.message;
//   }
//   message += "\n";
//   message +=
//       info.statement(scheme) + info.print_binary_diagnostics(width, scheme) +
//       info.print_extra_diagnostics(width, scheme);  // + info.print_stacktrace(width, scheme);

//   auto trace = info.get_stacktrace();
//   cleanup_frames(trace, rsl::testing::_testing_impl::assertion_counter().test_name);
//   message += trace.to_string(Colorize);
//   throw rsl::testing::assertion_failure(
//       message,
//       rsl::source_location(info.file_name, info.function, info.line));
// }


namespace rsl::testing {
namespace {


// auto resolve_pc(std::uintptr_t pc) {
//   auto raw_trace = cpptrace::raw_trace{{pc}};
//   auto trace     = raw_trace.resolve();
//   return trace.frames[0];
// }

auto filter_coverage(rsl::coverage::CoverageReport* data, std::size_t size) {
  std::unordered_map<std::string, std::vector<LineCoverage>> coverage;

  // for (std::size_t idx = 0; idx < size; ++idx) {
  //   auto resolved = resolve_pc(data[idx].pc);
  //   if (resolved.filename.empty() || (int)resolved.line.value() < 0) {
  //     continue;
  //   }
  //   if (resolved.filename.contains("/../include/c++/")) {
  //     continue;
  //   }
  //   coverage[resolved.filename].push_back({resolved.line.value(), data[idx].hits});
  // }

  std::vector<FileCoverage> result;
  for (auto const& [name, cov] : coverage) {
    result.emplace_back(name, cov);
  }
  return result;
}
}  // namespace

}  // namespace rsl::testing