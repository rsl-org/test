#include <functional>
#include <print>

#include "hooks.hpp"
#include "coverage.hpp"

namespace {
void reset_counters() {
  using rsl::coverage::counters;
  using rsl::coverage::guard_count;

  if (guard_count == 0 || counters == nullptr) {
    return;
  }

  for (auto idx = 0; idx < guard_count; ++idx) {
    counters[idx] = 0;
  }
}

}  // namespace

extern "C" __attribute__((no_sanitize("coverage"))) void _rsl_test_run_with_coverage(
    void (*fnc)(void const*),
    void const* test,
    rsl::coverage::CoverageReport** output,
    std::size_t* output_size) {
  //! this function is not thread safe
  //? to avoid atomics it is assumed that we're in single threaded context here

  //? data races on counters are acceptable
  //? => coverage counters are only approximate
  using namespace rsl::coverage;

  std::println("running with coverage");
  reset_counters();
  __sancov_should_track = 1;

  auto finalize = [&] {

  };

  // try {
  fnc(test);
  // } catch (...) {
  // finalize();
  // throw;
  // }
  // finalize();
  __sancov_should_track = 0;
  std::unordered_map<std::uintptr_t, std::uint64_t> reached;
  for (std::size_t idx = 0; idx < guard_count; ++idx) {
    if (counters[idx] != 0) {
      reached[pc_table[idx].pc] = counters[idx];
    }
  }
  std::vector<uintptr_t> snapshot = auto{pc_tracker()};
  for (auto pc : snapshot) {
    reached[pc]++;
  }

  // set output
  *output = (CoverageReport*)malloc(sizeof(CoverageReport) * reached.size());
  
  std::size_t idx = 0;
  for (auto const&[pc, count] : reached) {
    // char PcDescr[1024];
    // __sanitizer_symbolize_pc(pc + 4, "%s:%l:%c", PcDescr, sizeof(PcDescr));
    // char PcDescr2[1024];
    // __sanitizer_symbolize_global(pc + 4, "%s:%l:0", PcDescr2, sizeof(PcDescr2));
    // std::println("{} - {}", PcDescr, PcDescr);
    (*output)[idx] = {pc, count};
    ++idx;
  }

  *output_size = reached.size();
}