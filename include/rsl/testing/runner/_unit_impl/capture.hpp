#pragma once
#include <cstdio>
#include <string>
#include <string_view>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <fcntl.h>
#  include <io.h>
#  include <windows.h>
#else
#  include <cerrno>
#  include <fcntl.h>
#  include <unistd.h>
#endif

namespace rsl::testing {

namespace _impl {
inline int read_pipe(int fd, char* buffer, size_t size) {
#ifdef _WIN32
  HANDLE h        = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
  DWORD available = 0;
  if (PeekNamedPipe(h, nullptr, 0, nullptr, &available, nullptr) && available > 0)
    return _read(fd, buffer, static_cast<unsigned>(size));
  return 0;
#else
  ssize_t n = read(fd, buffer, size);
  if (n > 0) {
    return static_cast<int>(n);
  }
  if (errno == EAGAIN || errno == EWOULDBLOCK) {
    return 0;
  }
  return -1;
#endif
}

}  // namespace _impl

struct RedirectedOutput {
  FILE* redirected = nullptr;
  FILE* underlying = nullptr;

  int redirected_fd = -1;
  int underlying_fd = -1;

  RedirectedOutput() = default;

  RedirectedOutput(FILE* redirected_stream, int original_fd)
      : redirected(redirected_stream)
      , underlying(fdopen(original_fd, "w"))
      , redirected_fd(fileno(redirected_stream))  // TODO suppress msvc warning C4996 or use _fileno
      , underlying_fd(original_fd) {}
};

class Capture {
  int pipe_fds_[2]{};
  std::string* target;
  bool echo;

public:
  RedirectedOutput out;

  Capture(const Capture&)            = delete;
  Capture& operator=(const Capture&) = delete;

  // TODO this can be moveable
  Capture(Capture&&)                 = delete;
  Capture& operator=(Capture&&)      = delete;

  Capture(FILE* stream, std::string& target, bool echo = false) : target(&target), echo(echo) {
    (void)fflush(stream);
    out = {stream, dup(fileno(stream))};
#ifdef _WIN32
    _pipe(pipe_fds_, 8192, _O_BINARY);
#else
    pipe(pipe_fds_);
    fcntl(pipe_fds_[0], F_SETFL, O_NONBLOCK);
#endif
    dup2(pipe_fds_[1], out.redirected_fd);
  }

  ~Capture() {
    (void)fflush(out.redirected);
    dup2(out.underlying_fd, out.redirected_fd);
    close(pipe_fds_[1]);

    drain();  // Final flush

    close(pipe_fds_[0]);
    close(out.underlying_fd);
  }

  void drain() {
    (void)fflush(out.redirected);
    char buffer[256];
    while (true) {
      int n = _impl::read_pipe(pipe_fds_[0], &buffer[0], sizeof(buffer));
      if (n > 0) {
        *target += std::string_view(&buffer[0], n);
        if (echo) {
          write(out.underlying_fd, &buffer[0], n);
        }
      } else {
        break;
      }
    }
  }
};
}  // namespace rsl::testing