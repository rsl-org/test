#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <vector>

#include "hooks.hpp"

namespace rsl::coverage {
std::uint64_t* counters      = nullptr;
PCTableEntry const* pc_table = nullptr;
std::size_t guard_count      = 0;

std::vector<std::uintptr_t>& pc_tracker() {
  static std::vector<std::uintptr_t> data;
  return data;
}

}  // namespace rsl::coverage

extern "C" {
uint64_t __sancov_should_track = 0;

void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* stop) {
  using rsl::coverage::counters;
  using rsl::coverage::guard_count;

  if (counters != nullptr) {
    // reset
    fprintf(stderr, "guard reinitialized!");
    delete[] rsl::coverage::counters;
  }
  guard_count = stop - start;
  counters    = new std::uint64_t[guard_count];
  for (size_t i = 0; i < guard_count; i++) {
    start[i] = i + 1;
  }
}

void __sanitizer_cov_pcs_init(std::uintptr_t const* pcs_beg, std::uintptr_t const* pcs_end) {
  using rsl::coverage::guard_count;
  using rsl::coverage::pc_table;
  guard_count = (pcs_end - pcs_beg) / 2;
  assert((pcs_end - pcs_beg) / 2 == guard_count);

  pc_table = reinterpret_cast<rsl::coverage::PCTableEntry const*>(pcs_beg);
}

void __sanitizer_cov_trace_pc_guard(uint32_t* guard) {
  using rsl::coverage::counters;
  if (__sancov_should_track == 0 || counters == nullptr || guard == nullptr || *guard == 0U) {
    return;
  }
  auto idx = (*guard) - 1;
  assert(idx < rsl::coverage::guard_count);
  counters[idx]++;
}
}
