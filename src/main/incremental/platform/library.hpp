#pragma once
#include <string_view>

namespace rsl::testing::_impl_main {

  using library_handle = void*;

  library_handle load_library(std::string_view path);
  void unload_library(library_handle& handle);
  void* find_symbol(library_handle handle, std::string_view name);

  template <typename T>
  T* find_symbol(library_handle handle, std::string_view name) {
    return reinterpret_cast<T*>(find_symbol(handle, name));
  }
}