#include "capture.hpp"

#include <string>
#include <string_view>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif


namespace rsl::testing {

RedirectedOutput::RedirectedOutput(FILE* redirected_stream, int original_fd)
    : redirected(redirected_stream)
    , underlying_fd(original_fd) {
  underlying    = fdopen(original_fd, "w");
  redirected_fd = fileno(redirected_stream);
}

Capture::Capture(FILE* stream, std::string& target, bool echo )
    : target(&target)
    , echo(echo) {
  fflush(stream);
  out = {stream, dup(fileno(stream))};
#ifdef _WIN32
  _pipe(pipe_fds_, 8192, _O_BINARY);
#else
  pipe(pipe_fds_);
  fcntl(pipe_fds_[0], F_SETFL, O_NONBLOCK);
#endif
  dup2(pipe_fds_[1], out.redirected_fd);
}

Capture::~Capture() {
  fflush(out.redirected);
  dup2(out.underlying_fd, out.redirected_fd);
  close(pipe_fds_[1]);

  drain();  // Final flush

  close(pipe_fds_[0]);
  close(out.underlying_fd);
}

static int read_pipe(int fd, char* buffer, size_t size) {
#ifdef _WIN32
  HANDLE h        = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
  DWORD available = 0;
  if (PeekNamedPipe(h, nullptr, 0, nullptr, &available, nullptr) && available > 0)
    return _read(fd, buffer, static_cast<unsigned>(size));
  return 0;
#else
  ssize_t n = read(fd, buffer, size);
  if (n > 0)
    return static_cast<int>(n);
  if (errno == EAGAIN || errno == EWOULDBLOCK)
    return 0;
  return -1;
#endif
}

void Capture::drain() {
  fflush(out.redirected);
  char buffer[256];
  while (true) {
    int n = read_pipe(pipe_fds_[0], buffer, sizeof(buffer));
    if (n > 0) {
      *target += std::string_view(buffer, n);
      if (echo) {
        write(out.underlying_fd, buffer, n);
      }
    } else {
      break;
    }
  }
}

}  // namespace rsl::testing