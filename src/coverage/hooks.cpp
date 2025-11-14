#include <rsl/coverage/hooks.hpp>
#include <cassert>
#include <algorithm>

#include <cstdio>
#include <print>

extern "C" __attribute__((weak)) int __sanitizer_get_module_and_offset_for_pc(void* pc,
                                                                   char* module_name,
                                                                   std::uintptr_t module_name_len,
                                                                   void** pc_offset);
namespace rsl::coverage {

std::uint64_t* counters      = nullptr;
PCTableEntry const* pc_table = nullptr;
std::size_t guard_count      = 0;

void sanitizer_cov_pcs_init(std::uintptr_t const* pcs_beg, std::uintptr_t const* pcs_end) {
  std::println("pcs init");
  using namespace rsl::coverage;
  guard_count = (pcs_end - pcs_beg) / 2;
  assert((pcs_end - pcs_beg) / 2 == guard_count);

  pc_table = reinterpret_cast<rsl::coverage::PCTableEntry const*>(pcs_beg);

  auto count            = (pcs_end - pcs_beg) / 2;
  auto* pcs             = reinterpret_cast<PCTableEntry const*>(pcs_beg);
  std::uint64_t total   = 0;
  uintptr_t last_module = 0;
  std::uint64_t modules = 0;

  struct PCEntry {
    std::size_t guard_id;
    std::uintptr_t pc;
    std::size_t size       = 0;
    std::uint8_t* counters = nullptr;
  };
  std::vector<PCEntry> sorted_pcs;
  for (auto idx = 0; idx < count; ++idx) {
    auto const& entry = pc_table[idx];
    sorted_pcs.emplace_back(idx, entry.pc);
  }

  std::ranges::sort(sorted_pcs, [](auto const& lhs, auto const& rhs) { return lhs.pc < rhs.pc; });

  for (int idx = 0; idx < count; ++idx) {
    PCEntry const& pc_entry = sorted_pcs[idx];
    if (pc_entry.pc == 1)
      continue;

    void* offset = nullptr;
    __sanitizer_get_module_and_offset_for_pc(reinterpret_cast<void*>(pc_entry.pc),
                                             nullptr,
                                             0,
                                             &offset);
    uintptr_t module_ptr = pc_entry.pc - reinterpret_cast<uintptr_t>(offset);
    if (last_module != module_ptr) {
      ++modules;
      last_module     = module_ptr;
      char name[1024] = {};
      __sanitizer_get_module_and_offset_for_pc(reinterpret_cast<void*>(pc_entry.pc),
                                               name,
                                               1024,
                                               &offset);
      std::println("module {}: {} {:x}", modules, name, pc_entry.pc);
    }

    PCEntry const* next = &pc_entry;
    for (auto i = idx + 1; i < count; ++i) {
      if (sorted_pcs[i].pc != 1) {
        next = &sorted_pcs[i];
        break;
      }
    }

    int diff = int(next->pc - pc_entry.pc);
    if (diff < 0) {
      std::println("DIFF < 0 = {} .. {:x} {:x}", diff, pc_entry.pc - module_ptr, next->pc -
      module_ptr);
      diff = 0;
    }
    total += diff;
    // std::println("{:x} {:x} {:x}", pc_entry.pc, next->pc, diff/8);
  }
  std::println("total: {} {}", total, modules);
}

void sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* stop) {
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

void sanitizer_cov_trace_pc_guard(uint32_t* guard) {
  if (counters == nullptr || guard == nullptr || *guard == 0U) {
    return;
  }

  auto idx = (*guard) - 1;
  assert(idx < rsl::coverage::guard_count);
  counters[idx]++;
}
}  // namespace rsl::coverage
