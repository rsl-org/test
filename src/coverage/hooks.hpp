#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

namespace rsl::coverage {

struct PCTableEntry {
  std::uintptr_t pc;
  std::uintptr_t flags;
  [[nodiscard]] bool is_function_entry() const { return flags != 0; }
};

//? this isn't wrapped in a unique_ptr for more direct access
//? otherwise the hook might become recursive
extern std::uint64_t* counters;
extern PCTableEntry const* pc_table;
extern std::size_t guard_count;
std::vector<std::uintptr_t>& pc_tracker();
}  // namespace rsl::coverage

extern "C" {
//? utilities from compiler-rt
int __sanitizer_get_module_and_offset_for_pc(void* pc,
                                             char* module_name,
                                             std::uintptr_t module_name_len,
                                             void** pc_offset);

void __sanitizer_symbolize_pc(std::uintptr_t pc,
                              char const* fmt,
                              char* out_buf,
                              std::uintptr_t out_buf_size);

void __sanitizer_symbolize_global(std::uintptr_t data_addr,
                                  char const* fmt,
                                  char* out_buf,
                                  std::uintptr_t out_buf_size);

//? -mllvm -sanitizer-coverage-gated-trace-callbacks
extern uint64_t __sancov_should_track;

//? -fsanitize-coverage=stack-depth
// extern thread_local uintptr_t __sancov_lowest_stack;
// void __sanitizer_cov_stack_depth();

//? -fsanitize-coverage=trace-pc-guard
void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* end);
void __sanitizer_cov_trace_pc_guard(uint32_t* guard);

//? -fsanitize-coverage=inline-8bit-counters
// void __sanitizer_cov_8bit_counters_init(uint8_t* start, uint8_t* end);

//? -fsanitize-coverage=inline-bool-flag
// void __sanitizer_cov_bool_flag_init(bool* start, bool* end);

//? -fsanitize-coverage=pc-table
//? requires one of inline-8bit-counters, inline-bool-flag, trace-pc-guard
void __sanitizer_cov_pcs_init(uintptr_t const* pcs_beg, uintptr_t const* pcs_end);

//? -fsanitize-coverage=trace-pc
void __sanitizer_cov_trace_pc(void* callee);  // also available in GCC?

//? -fsanitize-coverage=indirect-calls
// void __sanitizer_cov_trace_pc_indir(uintptr_t callee);

//? -fsanitize-coverage=trace-cmp
//? requires one of trace-pc, inline-8bit-counters, inline-bool-flag
// void __sanitizer_cov_trace_cmp8(uint64_t Arg1, uint64_t Arg2);
// void __sanitizer_cov_trace_const_cmp8(uint64_t Arg1, uint64_t Arg2);
// void __sanitizer_cov_trace_cmp4(uint32_t Arg1, uint32_t Arg2);
// void __sanitizer_cov_trace_const_cmp4(uint32_t Arg1, uint32_t Arg2);
// void __sanitizer_cov_trace_cmp2(uint16_t Arg1, uint16_t Arg2);
// void __sanitizer_cov_trace_const_cmp2(uint16_t Arg1, uint16_t Arg2);
// void __sanitizer_cov_trace_cmp1(uint8_t Arg1, uint8_t Arg2);
// void __sanitizer_cov_trace_const_cmp1(uint8_t Arg1, uint8_t Arg2);
// void __sanitizer_cov_trace_switch(uint64_t Val, uint64_t* Cases);

//? -fsanitize-coverage=trace-div
//? requires one of trace-pc, inline-8bit-counters, inline-bool-flag
// void __sanitizer_cov_trace_div4(uint32_t Val);
// void __sanitizer_cov_trace_div8(uint64_t Val);

//? -fsanitize-coverage=trace-gep
//? requires one of trace-pc, inline-8bit-counters, inline-bool-flag
// void __sanitizer_cov_trace_gep(uintptr_t Idx);

//? -fsanitize-coverage=trace-loads
//? requires one of trace-pc, inline-8bit-counters, inline-bool-flag
// void __sanitizer_cov_load1(uint8_t* addr);
// void __sanitizer_cov_load2(uint16_t* addr);
// void __sanitizer_cov_load4(uint32_t* addr);
// void __sanitizer_cov_load8(uint64_t* addr);
// void __sanitizer_cov_load16(__int128* addr);

//? -fsanitize-coverage=trace-stores
//? requires one of trace-pc, inline-8bit-counters, inline-bool-flag
// void __sanitizer_cov_store1(uint8_t* addr);
// void __sanitizer_cov_store2(uint16_t* addr);
// void __sanitizer_cov_store4(uint32_t* addr);
// void __sanitizer_cov_store8(uint64_t* addr);
// void __sanitizer_cov_store16(__int128* addr);
}  // extern "C"