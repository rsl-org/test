#pragma once
#include <cstdio>
#include <string>

namespace rsl::testing {

struct RedirectedOutput {
  FILE* redirected = nullptr;
  FILE* underlying = nullptr;

  int redirected_fd = -1;
  int underlying_fd = -1;

  RedirectedOutput() = default;

  RedirectedOutput(FILE* redirected_stream, int original_fd);
};

class Capture {
  int pipe_fds_[2]{};
  std::string* target;
  bool echo;

public:
  RedirectedOutput out;

  Capture(FILE* stream, std::string& target, bool echo = false);
  ~Capture();
  void drain();
};
}  // namespace rsl::testing