#include "../event_loop.hpp"

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

namespace {
void make_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int init_epoll() {
  int ep = epoll_create1(0);
  if (ep < 0) {
    perror("epoll_create1");
  }
  return ep;
}

void enable_fd(int ep, int fd) {
  make_nonblocking(fd);

  epoll_event ev{};
  ev.events  = EPOLLIN | EPOLLET;  // edge-triggered
  ev.data.fd = fd;

  if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev) < 0) {
    perror("epoll_ctl");
    return;
  }
}

// void disable_fd(int ep, int fd) {
//   epoll_ctl(ep, EPOLL_CTL_DEL, fd, nullptr);
// }

}  // namespace

namespace rsl::testing::_impl_main {

void EventLoopImpl::init() {
  ep = init_epoll();
}

void EventLoopImpl::enable(uintptr_t handle, size_t idx) {
  enable_fd((int)ep, (int)handle);
  map[handle] = idx;
}

void EventLoopImpl::run() {
  running = true;
  std::vector<epoll_event> events(32);

  while (running) {
    int n = epoll_wait((int)ep, events.data(), (int)events.size(), -1);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("epoll_wait");
      return;
    }

    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;
      std::vector<char> buffer;

      while (true) {
        char temp[4096];
        ssize_t r = read(fd, temp, sizeof(temp));

        if (r > 0) {
          buffer.append_range(std::span(temp, r));
          continue;  // try reading more
        }
        if (r == 0) {
          // EOF
          close(fd);
          break;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          break;  // drained fully
        }

        perror("read");
        close(fd);
        break;
      }
      
      dispatchers[map[fd]](buffer);
    }
  }
}

}  // namespace rsl::testing::_impl_main