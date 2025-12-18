#include <string>

#include <dlfcn.h>
#include "../library.hpp"

namespace rsl::testing::_impl_main {

library_handle load_library(std::string_view path) {
  return dlopen(std::string(path).c_str(), RTLD_NOW | RTLD_LOCAL);
}

void unload_library(library_handle& handle) {
  if (handle != nullptr) {
    dlclose(handle);
  }
  handle = nullptr;
}

void* find_symbol(library_handle handle, std::string const& name) {
  return dlsym(handle, name.c_str());
}

}  // namespace rsl::testing::_impl_main