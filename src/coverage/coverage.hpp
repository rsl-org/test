#pragma once
#include <cstdint>
#include <cstddef>

namespace rsl::coverage {
struct CoverageReport {
  std::uintptr_t pc;
  std::uint64_t hits;
};
}  // namespace rsl::coverage

extern "C" __attribute__((weak)) 
void _rsl_test_run_with_coverage(
    void (*fnc)(void const*),
    void const* test, 
    rsl::coverage::CoverageReport** output,
    std::size_t* output_size);
