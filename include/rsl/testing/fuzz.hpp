#pragma once

namespace rsl::testing {
struct FuzzTarget {
  // stringifying name is pointless here
  
  int (*run)(uint8_t const*, size_t);
  size_t (*mutate)(uint8_t*, size_t, size_t, unsigned int);

  Test const* test;

  template <std::meta::info R, std::meta::info Target>
  struct FuzzRunner {
    static int run(uint8_t const* Data, size_t Size) {
      // TODO
      return 0;
    }

    // mutator must be able to consider domains
    static size_t mutate(uint8_t* Data, size_t Size, size_t MaxSize, unsigned int Seed) {
      // TODO
      return 0;
    }
  };
};
}  // namespace rsl::testing