#include <rsl/coverage/hooks.hpp>

#include <unordered_map>
#include <cstring>

namespace rsl::coverage {
namespace {
void enable() {
  if (should_track != nullptr) {
    *should_track = 1;
  }
}

void disable() {
  if (should_track != nullptr) {
    *should_track = 0;
  }
}

void reset_counters() {
  if (guard_count == 0 || counters == nullptr) {
    return;
  }

  for (auto idx = 0; idx < guard_count; ++idx) {
    counters[idx] = 0;
  }
}

auto filter_traces() {
  std::unordered_map<std::uintptr_t, std::uint64_t> reached;
  for (std::size_t idx = 0; idx < guard_count; ++idx) {
    if (counters[idx] != 0) {
      reached[pc_table[idx].pc] = counters[idx];
    }
  }
  return reached;
}

}  // namespace

void run(void (*fnc)(void const*),
         void const* test,
         rsl::coverage::CoverageReport** output,
         std::size_t* output_size) {
  //! this function is not thread safe
  //? to avoid atomics it is assumed that we're in single threaded context here

  //? data races on counters are acceptable
  //? => coverage counters are only approximate
  using namespace rsl::coverage;

  auto finalize = [&] {
    disable();
    auto reached = filter_traces();
    // set output
    if (!reached.empty()) {
      *output = (CoverageReport*)malloc(sizeof(CoverageReport) * reached.size());
  
      std::size_t idx = 0;
      for (auto const& [pc, count] : reached) {
        (*output)[idx] = {pc, count};
        ++idx;
      }
    } else {
      *output = nullptr;
    }

    *output_size = reached.size();
  };

  reset_counters();
  enable();
  try {
    fnc(test);
  } catch (...) {
    finalize();
    throw;
  }
  finalize();
}
}  // namespace rsl::coverage