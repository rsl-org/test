#pragma once

namespace rsl::testing {
class Test;
struct FuzzTarget {
  // stringifying name is pointless here, perhaps do it after failure
  
  int (*run)(uint8_t const*, size_t);
  size_t (*mutate)(uint8_t*, size_t, size_t, unsigned int);

  Test const* test;
};
}  // namespace rsl::testing