#include <cassert>
#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <cstring>
#include <vector>
#include <print>
#include <ranges>

#define RSL_DO_NOT_COVER __attribute__((no_sanitize("coverage")))

extern "C" {
std::uint64_t __sancov_should_track = 0;

int __sanitizer_get_module_and_offset_for_pc(void* pc,
                                             char* module_name,
                                             std::uintptr_t module_name_len,
                                             void** pc_offset);

void __sanitizer_symbolize_pc(std::uintptr_t pc,
                              char const* fmt,
                              char* out_buf,
                              std::uintptr_t out_buf_size);
}

namespace rsl::coverage {

struct CoverageReport {
  std::uintptr_t pc;
  std::uint64_t hits;
};

struct PCTableEntry {
  std::uintptr_t pc;
  std::uintptr_t flags;
  [[nodiscard]] bool is_function_entry() const { return flags != 0; }
};

struct TracePoint {
  PCTableEntry const* pc_entry;
  std::uint64_t counter;
};

PCTableEntry const* pc_table = nullptr;
static std::vector<TracePoint> traces{};

namespace {
RSL_DO_NOT_COVER void enable() {
  __sancov_should_track = 1;
}

RSL_DO_NOT_COVER void disable() {
  __sancov_should_track = 0;
}

RSL_DO_NOT_COVER void reset_counters() {
  for (auto& trace : traces) {
    trace.counter = 0;
  }
}

RSL_DO_NOT_COVER auto filter_traces() {
  std::unordered_map<std::uintptr_t, std::uint64_t> reached;
  for (auto const& trace : traces) {
    if (trace.pc_entry == nullptr || trace.counter == 0) {
      continue;
    }
    reached[trace.pc_entry->pc] = trace.counter;
  }
  return reached;
}

RSL_DO_NOT_COVER 
/// Note: the logic is copied from compiler-rt `sanitizer_common/sanitizer_stacktrace.cpp`
std::uintptr_t next_instruction(uintptr_t PC) {
#if defined(__mips__)
  return PC + 8;
#elif defined(__powerpc__) || defined(__sparc__) || defined(__arm__) ||        \
    defined(__aarch64__) || defined(__loongarch__)
  return PC + 4;
#else
  return PC + 1;
#endif
}

}  // namespace

RSL_DO_NOT_COVER std::uintptr_t get_module_ptr(std::uintptr_t pc) {
  void* offset = nullptr;
  __sanitizer_get_module_and_offset_for_pc(reinterpret_cast<void*>(pc), nullptr, 0, &offset);
  return pc - reinterpret_cast<uintptr_t>(offset);
}

RSL_DO_NOT_COVER void sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* stop) {
  std::size_t old_count = traces.size();
  if (old_count != 0) {
    std::println("guard reinitialized! new {} old {}", old_count + (stop - start), old_count);
  }

  for (size_t i = 0; i < (stop - start); ++i) {
    start[i] = old_count + i + 1;
    traces.push_back({nullptr, 0});
    // std::println("guard {} value {}", i, old_count + i + 1);
  }
  std::println("{}", traces.size());
}

RSL_DO_NOT_COVER void sanitizer_cov_pcs_init(std::uintptr_t const* pcs_beg,
                                             std::uintptr_t const* pcs_end) {
  std::println("pcs init {} {} {}",
               (pcs_end - pcs_beg) / 2,
               traces.size(),
               traces.size() - ((pcs_end - pcs_beg) / 2));
  auto count        = (pcs_end - pcs_beg) / 2;
  auto start_offset = traces.size() - count;
  assert(traces.size() >= count);

  auto* pcs = reinterpret_cast<PCTableEntry const*>(pcs_beg);
  for (auto idx = 0; idx < count; ++idx) {
    traces.at(start_offset + idx).pc_entry = &pcs[idx];
  }

  std::uint64_t total   = 0;
  uintptr_t last_module = 0;
  std::uint64_t modules = 0;

  struct PCEntry {
    std::size_t guard_id;
    std::uintptr_t pc;
    std::size_t size = 0;
  };

  std::vector<PCEntry> sorted_pcs;
  for (auto idx = start_offset; idx < traces.size(); ++idx) {
    sorted_pcs.emplace_back(idx, traces.at(idx).pc_entry->pc);
  }
  std::ranges::sort(sorted_pcs, [](auto const& lhs, auto const& rhs) { return lhs.pc < rhs.pc; });

  for (int idx = 0; idx < sorted_pcs.size(); ++idx) {
    PCEntry const& pc_entry = sorted_pcs[idx];
    if (pc_entry.pc == 1) {
      continue;
    }

    uintptr_t module_ptr = get_module_ptr(pc_entry.pc);
    if (last_module != module_ptr) {
      ++modules;
      last_module = module_ptr;

      char name[1024] = {};
      void* offset    = nullptr;
      __sanitizer_get_module_and_offset_for_pc(reinterpret_cast<void*>(pc_entry.pc),
                                               name,
                                               1024,
                                               &offset);
      std::println("module {}: {} {:x} {}", modules, name, pc_entry.pc, total);
    }

    PCEntry const* next = &pc_entry;
    for (auto i = idx + 1; i < count; ++i) {
      if (sorted_pcs[i].pc != 1) {
        next = &sorted_pcs[i];
        break;
      }
    }

    std::uintptr_t next_module    = get_module_ptr(next->pc);
    std::uintptr_t current_module = get_module_ptr(pc_entry.pc);

    int diff = int(next->pc - pc_entry.pc);
    if (next_module != current_module || diff < 0) {
      // likely a module boundary - search for the last valid PC in the current page
      char buf[8];
      constexpr std::string_view invalid = "<null>";
      
      auto last_valid = 0;
      for (auto offset = 0; offset < 4096; offset += 8) {
        __sanitizer_symbolize_pc(next_instruction(pc_entry.pc + offset), "%s", buf, 8);
        if (buf != invalid) {
          last_valid = offset;
        }
      }
      diff = last_valid + 1;
    }
    total += diff;

    // std::println("{:x} {:x} {:x}", pc_entry.pc, next->pc, diff / 8);
  }
  std::println("total: {} {}", total, modules);
}

RSL_DO_NOT_COVER void sanitizer_cov_trace_pc_guard(uint32_t* guard) {
  if (__sancov_should_track == 0 || guard == nullptr || *guard == 0U) {
    return;
  }

  disable();
  auto idx = (*guard) - 1;
  if (idx > traces.size()) {
    return;
  }

  traces.at(idx).counter++;
  enable();
}

RSL_DO_NOT_COVER void run(void (*fnc)(void const*),
                          void const* test,
                          rsl::coverage::CoverageReport** output,
                          std::size_t* output_size) {
  //! this function is not thread safe
  //? to avoid atomics it is assumed that we're in single threaded context here

  //? data races on counters are acceptable
  //? => coverage counters are only approximate
  using namespace rsl::coverage;

  auto finalize = [&] RSL_DO_NOT_COVER {
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

extern "C" {
RSL_DO_NOT_COVER void __sanitizer_cov_pcs_init(std::uintptr_t const* pcs_beg,
                                               std::uintptr_t const* pcs_end) {
  rsl::coverage::sanitizer_cov_pcs_init(pcs_beg, pcs_end);
}

RSL_DO_NOT_COVER void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* stop) {
  rsl::coverage::sanitizer_cov_trace_pc_guard_init(start, stop);
}

RSL_DO_NOT_COVER void __sanitizer_cov_trace_pc_guard(uint32_t* guard) {
  if (__sancov_should_track == 0) {
    return;
  }
  rsl::coverage::sanitizer_cov_trace_pc_guard(guard);
}

RSL_DO_NOT_COVER void _rsl_test_run_with_coverage(void (*fnc)(void const*),
                                                  void const* test,
                                                  rsl::coverage::CoverageReport** output,
                                                  std::size_t* output_size) {
  rsl::coverage::run(fnc, test, output, output_size);
}
}