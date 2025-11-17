#include <string>

#include <dlfcn.h>
#include "../library.hpp"

namespace rsl::testing::_impl_main {

library_handle load_library(std::string_view path) {
  return dlopen(std::string(path).c_str(), RTLD_NOW);
}

void unload_library(library_handle handle) {
  dlclose(handle);
}

void* find_symbol(library_handle handle, std::string_view name) {
  return dlsym(handle, std::string(name).c_str());
}

}  // namespace rsl::testing::_main_impl