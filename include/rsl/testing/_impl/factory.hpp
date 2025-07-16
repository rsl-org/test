#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <meta>
#include <unordered_map>

#include "../annotations.hpp"

namespace rsl::testing::_impl {
template <class Base, class... Args>
class Factory {
public:
  template <class... T>
  static std::unique_ptr<Base> make(const std::string& s, T&&... args) {
    return data().at(s)(std::forward<T>(args)...);
  }

  template <class T>
  struct Registrar : Base {
    friend T;

    static consteval std::string_view get_name() {
      auto annotations = annotations_of(^^T, ^^annotations::Rename);
      if (annotations.size() == 1) {
        auto opt = extract<annotations::Rename>(constant_of(annotations[0]));
        return opt.value;
      }
      return identifier_of(^^T);
    }

    static bool registerT() {
      static constexpr std::string_view name = define_static_string(get_name());
      Factory::data()[name]                  = [](Args... args) -> std::unique_ptr<Base> {
        return std::make_unique<T>(std::forward<Args>(args)...);
      };
      return true;
    }
    inline static const std::nullptr_t registration = (registerT(), nullptr);
    static constexpr std::integral_constant<std::nullptr_t const*, &registration>
        registration_helper{};

  private:
    Registrar() : Base(Key{}) { (void)registration; }
  };

  friend Base;

private:
  class Key {
    Key() {};
    template <class T>
    friend struct Registrar;
  };
  using FuncType = std::unique_ptr<Base> (*)(Args...);
  Factory()      = default;

  static auto& data() {
    static std::unordered_map<std::string_view, FuncType> s;
    return s;
  }
};
}  // namespace rsl::testing::_impl