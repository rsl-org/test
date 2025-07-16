#pragma once
#include <string_view>
#include <memory>
#include <unordered_map>
#include <meta>

#include "../annotations.hpp"

namespace rsl::testing {
template <class Base, class... Args>
class Factory {
 public:
  template <class... T>
  static std::unique_ptr<Base> make(const std::string &s, T &&...args) {
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
      Factory::data()[name] = [](Args... args) -> std::unique_ptr<Base> {
        return std::make_unique<T>(std::forward<Args>(args)...);
      };
      return true;
    }
    [[maybe_unused]] static inline bool registered = Factory<Base, Args...>::Registrar<T>::registerT();

   private:
    Registrar() : Base(Key{}) { (void)registered; }
  };

  friend Base;

 private:
  class Key {
    Key() {};
    template <class T>
    friend struct Registrar;
  };
  Factory() = default;
  public:
  using FuncType = std::unique_ptr<Base> (*)(Args...);
  static std::unordered_map<std::string_view, FuncType>& data();
};


}