#pragma once
#include <print>
#include <ranges>
#include <cstdio>
#include <map>

#include "compile_pool.hpp"
#include "config_parser.hpp"
#include "compdb.hpp"
#include "platform/library.hpp"

#include <rsl/testing/_testing_impl/discovery.hpp>

namespace rsl::testing::_impl_main {
struct TestSet {
  void* handle = nullptr;
  std::set<rsl::testing::TestDef> tests;

  void unload() {
    if (handle != nullptr) {
      unload_library(handle);
      tests = {};
    }
  }
};

struct TestUnit {
  std::filesystem::path source_path;

  // library path -> metadata (path is different between configurations)
  std::unordered_map<std::filesystem::path, TestSet> sets;
};

struct IncrementalRunner {
  CompilePool pool;  // TODO use more generic task pool?
  RunnerConfig config;

  std::map<std::filesystem::path, TestUnit> units;
  // reverse dependency map
  std::unordered_map<std::filesystem::path, std::set<std::filesystem::path const*>> dependencies;

  // TODO map source -> testset
  std::unordered_map<std::filesystem::path, TestSet> test_sets;

  CompileCommands compdb;

  // TODO move to config?
  static constexpr std::array allowed_extensions = {".cpp"};

public:
  IncrementalRunner() = default;
  explicit IncrementalRunner(std::filesystem::path const& config_path)
      : config(load_runner_config(config_path)) {
    compdb = CompileCommands(config.project.build_path / "compile_commands.json");
  }

  void update_compdb() {
    compdb.load();
    for (auto const& path : test_sets) {
      auto tu = make_invocation("default", path.first);
      compdb.append_if_missing({.directory = tu.source_path.parent_path(),
                                .file      = tu.source_path,
                                .arguments = tu.invocation.arguments,
                                .output    = tu.out_path});
    }
    compdb.save();
  }

  std::vector<std::string> expand_options(std::string_view config_name) const {
    const auto& options = config.options;
    const auto& cfg = config.configurations.at(std::string(config_name));

    const bool ext                  = cfg.gnu_extensions;
    const auto ver                  = cfg.standard;
    const std::string standard      = std::format("-std={}++{}", (ext ? "gnu" : "c"), ver);
    std::vector<std::string> cmd;
    cmd.push_back(standard);

    for (auto const& o : options.compile_options) {
      cmd.push_back(o);
    }

    for (auto const& dir : options.include_dirs) {
      cmd.push_back(std::format("-I{}", dir.string()));
    }

    for (auto const& d : options.compile_definitions) {
      cmd.push_back(std::format("-D{}", d));
    }

    if (auto ns = config.project.namespace_; not ns.empty()) {
      cmd.emplace_back("-DRSL_TEST_NAMESPACE=" + ns);
    }
    return cmd;
  }

  std::vector<std::string> expand_link_options() const {
    std::vector<std::string> cmd;
    for (auto const& lib : config.options.link_libraries) {
        if (lib.is_absolute()) {
          cmd.push_back(std::format("-L{}", lib.string()));
        } else {
          cmd.push_back(std::format("-l{}", lib.string()));
        }
      }

      // cmd.push_back(std::format("-Wl,-rpath,{}", build_path.string()));
      // cmd.push_back(std::format("-L{}", build_path.string()));
      cmd.emplace_back("-fPIC");
      cmd.emplace_back("-shared");
      return cmd;
  }

  [[nodiscard]]
  TestTU make_invocation(std::string_view config_name,
                         std::filesystem::path const& test_path,
                         bool dump_dependencies = false) const {
    const std::filesystem::path build_path   = config.project.build_path;
    const std::filesystem::path project_path = config.project.project_path;

    const auto& cfg = config.configurations.at(std::string(config_name));
    const std::string compiler_path = cfg.compiler_path;

    std::filesystem::path out_path = build_path / std::filesystem::relative(test_path, project_path);
    out_path.replace_extension(".so");

    std::vector<std::string> cmd = {compiler_path};
    cmd.append_range(expand_options(config_name));

    if (not dump_dependencies) {
      cmd.append_range(expand_link_options());

      // out file
      cmd.emplace_back("-o");
      cmd.push_back(out_path.string());
    }

    // input file
    cmd.push_back(std::string(test_path.string()));

    if (dump_dependencies) {
      cmd.emplace_back("-MM");
    }

    return {compiler_path, cmd, out_path, test_path};
  }

  static std::vector<std::filesystem::path> discover_tests(std::filesystem::path const& root) {
    std::vector<std::filesystem::path> result;

    for (auto const& entry : std::filesystem::recursive_directory_iterator(root)) {
      if (not entry.is_regular_file()) {
        continue;
      }

      auto const ext = entry.path().extension().string();
      if (std::ranges::contains(allowed_extensions, ext)) {
        result.push_back(entry.path());
      }
    }
    return result;
  }

  std::vector<std::filesystem::path> discover_tests() {
    std::vector<std::filesystem::path> result;
    for (auto const& path : config.project.test_path) {
      result.append_range(discover_tests(path));
    }
    return result;
  }

  std::vector<TestTU> expand_tests(std::vector<std::filesystem::path> const& tests,
                                   bool dump = false) {
    std::vector<TestTU> test_tus;
    for (auto const& test : tests) {
      for (auto const& [name, _] : config.configurations) {
        test_tus.push_back(make_invocation(name, test, dump));
      }
    }
    return test_tus;
  }

  void recompile(std::vector<TestTU> const& tus) {
    //! this function is not thread-safe, it shall only be invoked from the main thread

    for (auto&& tu : tus) {
      pool.submit(tu);
    }

    std::println("Building {} test{}", tus.size(), tus.size() == 1 ? "" : "s");

    auto progress = [](auto done, auto total) {
      constexpr static auto bar_width = 30;
      auto filled = static_cast<int>((static_cast<double>(done) / total) * bar_width);
      auto bar    = std::string(filled, '#') + std::string(bar_width - filled, ' ');
      std::print("\r[{}] ({}/{})", bar, done, total);
      std::fflush(stdout);
    };
    progress(0, tus.size());
    pool.wait(progress);
    std::println("");

    for (auto&& [tu, result] : pool.collect()) {
      if (result.exit_code != 0) {
        std::println("ERROR!");
        std::println("====== stdout ======\n{}", result.stdout_str);
        std::println("====== stderr ======\n{}", result.stderr_str);
        continue;
      }
      if (not rsl::testing::_testing_impl::registry().empty()) {
        std::println("test registry not empty");
        rsl::testing::_testing_impl::registry().clear();
      }
      // check if we have the key already, make sure old one is unloaded
      unload(tu.out_path);

      library_handle handle = load_library(tu.out_path.string());
      if (handle != nullptr) {
        test_sets[tu.out_path] = {handle, rsl::testing::_testing_impl::registry()};
      }
      rsl::testing::_testing_impl::registry().clear();
    }

    if (not rsl::testing::_testing_impl::registry().empty()) {
      std::println("test registry not empty");
    }
  }

  void unload(std::filesystem::path const& file) {
    if (auto it = test_sets.find(file); it != test_sets.end()) {
      it->second.unload();
    }
  }
};
}  // namespace rsl::testing::_impl_main