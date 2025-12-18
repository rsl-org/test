#pragma once
#include <span>
#include <ranges>
#include <atomic>
#include <concepts>
#include <unordered_map>
namespace rsl::testing::_impl_main {

template <typename T>
concept handler_like = requires(T& obj, std::span<char const> data) {
  { obj.on_readable(data) } -> std::same_as<void>;
  { obj.get_handle() } -> std::same_as<uintptr_t>;
};

struct Dispatcher {
  using dispatch_t = void(*)(void*, std::span<char const>);
  void* obj;
  dispatch_t dispatcher;

  void operator()(std::span<char const> data) const {
    dispatcher(obj, data);
  }
};

struct EventLoopImpl {
protected:
  using dispatch_t = void(*)(void*, std::span<char const>); 
  std::span<Dispatcher> dispatchers;

  std::unordered_map<uintptr_t, size_t> map;
  uintptr_t ep;
  std::atomic<bool> running;
  void init();
  void enable(uintptr_t handle, size_t idx);
public:
  void run();
  void stop() { running = false; }
};

template <handler_like... Ts>
struct EventLoop : EventLoopImpl {
  std::array<void*, sizeof...(Ts)> callbacks;
  std::array<Dispatcher, sizeof...(Ts)> handler_fncs;

  EventLoop(Ts&... values) : callbacks({&values...}) {
    template for (constexpr auto Idx : std::views::iota(0ZU, sizeof...(Ts))) {
      handler_fncs[Idx] = {&values...[Idx], &dispatch<Idx>};
    }
    dispatchers = handler_fncs;
    init();

    size_t idx = 0;
    (enable(values.get_handle(), idx++), ...);
  }

  ~EventLoop() {
    // close open handles
  }

  template <size_t Idx>
  static void dispatch(void* item, std::span<char const> data) {
    static_cast<Ts...[Idx]*>(item)->on_readable(data);
  }
};

}  // namespace rsl::testing::_impl_main