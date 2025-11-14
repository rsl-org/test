#include <rsl/coverage/hooks.hpp>

#include <cstddef>
#include <cstdint>

extern "C" {
std::uint64_t __sancov_should_track = 0;

void __sanitizer_cov_pcs_init(std::uintptr_t const* pcs_beg, std::uintptr_t const* pcs_end) {
  rsl::coverage::should_track = &__sancov_should_track;
  rsl::coverage::sanitizer_cov_pcs_init(pcs_beg, pcs_end);
}

void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* stop) {
  rsl::coverage::sanitizer_cov_trace_pc_guard_init(start, stop);
}

void __sanitizer_cov_trace_pc_guard(uint32_t* guard) {
  if (__sancov_should_track == 0) {
    return;
  }
  rsl::coverage::sanitizer_cov_trace_pc_guard(guard);
}

void _rsl_test_run_with_coverage(void (*fnc)(void const*),
                                 void const* test,
                                 rsl::coverage::CoverageReport** output,
                                 std::size_t* output_size) {
  rsl::coverage::run(fnc, test, output, output_size);
}
}