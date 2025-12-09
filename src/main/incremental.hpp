#pragma once
#include <dlfcn.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <print>
#include <ranges>
#include <cstdio>
#include <map>

#include "compile_pool.hpp"
#include "config_parser.hpp"
#include "compdb.hpp"
#include "platform/library.hpp"

#include <rsl/testing/output.hpp>
#include <rsl/testing/_testing_impl/discovery.hpp>

namespace rsl::testing::_impl_main {
struct TestSet {
  void* handle = nullptr;
  std::set<rsl::testing::TestDef> tests;

  TestSet()                          = default;
  TestSet(TestSet const&)            = delete;
  TestSet& operator=(TestSet const&) = delete;

  TestSet(void* handle, std::set<rsl::testing::TestDef> tests)
      : handle(handle)
      , tests(std::move(tests)) {}

  TestSet(TestSet&& other) : handle(other.handle), tests(std::move(other.tests)) {
    other.handle = nullptr;
  }

  TestSet& operator=(TestSet&& other) {
    if (this == &other) {
      return *this;
    }
    handle       = other.handle;
    tests        = std::move(other.tests);
    other.handle = nullptr;
    return *this;
  }

  ~TestSet() { unload(); }

  void unload() {
    if (handle != nullptr) {
      unload_library(handle);
      tests = {};
    }
  }
};  // namespace rsl::testing::_impl_main

struct TestUnit {
  // library path -> metadata (path is different between configurations)
  std::unordered_map<std::filesystem::path, TestSet> sets;
  size_t counter = 0;

  std::set<rsl::testing::TestDef> load(std::filesystem::path const& library_path) {
    auto path = canonical(library_path);
    auto tmp_path =
        std::filesystem::path(library_path).replace_extension(".so." + std::to_string(counter++));
    while (exists(tmp_path)) {
      tmp_path =
          std::filesystem::path(library_path).replace_extension(".so." + std::to_string(counter++));
    }

    // check if we have the key already, make sure old one is unloaded
    unload(path);

    std::filesystem::rename(library_path, tmp_path);
    library_handle handle = load_library(tmp_path.string());
    if (handle == nullptr) {
      std::println("unable to load library");
      return {};
    }

    auto fnc = find_symbol<void*()>(handle, "load_tests");
    if (fnc == nullptr) {
      std::println("load_tests missing");
      return {};
    }
    auto test_sets = reinterpret_cast<std::set<rsl::testing::TestDef>*>(fnc());
    sets.insert_or_assign(path, TestSet{handle, *test_sets});
    return *test_sets;
  }

  static void remove_stale(std::filesystem::path const& path) {
    for (const auto& entry : std::filesystem::directory_iterator(path.parent_path())) {
      if (!entry.is_regular_file()) {
        continue;
      }
      if (path.filename().string() == entry.path().stem().string()) {
        remove(entry.path());
      }
    }
  }

  void unload(std::filesystem::path const& path) {
    remove_stale(path);
    if (auto it = sets.find(weakly_canonical(path)); it != sets.end()) {
      it->second.unload();
      sets.erase(it);
    }
  }
};

struct IncrementalRunner {
  CompilePool pool;  // TODO use more generic task pool?
  RunnerConfig config;

  std::map<std::filesystem::path, TestUnit> units;
  // reverse dependency map
  std::unordered_map<std::filesystem::path, std::set<std::filesystem::path const*>> dependencies;
  std::unique_ptr<rsl::testing::Reporter> reporter;
  CompileCommands compdb;
  size_t invocation_counter = 0;

  // TODO move to config?
  static constexpr std::array allowed_extensions = {".cpp"};

public:
  IncrementalRunner() = default;
  explicit IncrementalRunner(std::filesystem::path const& config_path)
      : config(load_runner_config(config_path)) {
    // TODO check build_path/compile_commands.json if this doesn't exist
    compdb   = CompileCommands(config.project.project_path / "compile_commands.json");
    reporter = rsl::testing::Reporter::make("json");
  }

  void update_compdb() {
    compdb.load();
    for (auto const& [path, unit] : units) {
      auto tu = make_invocation("default", path, false, true);
      compdb.append_if_missing({.directory = config.project.build_path,
                                .file      = tu.source_path,
                                .arguments = tu.invocation.arguments,
                                .output    = tu.out_path});
    }
    compdb.save();
  }

  [[nodiscard]]
  TestTU make_invocation(std::string_view config_name,
                         std::filesystem::path const& test_path,
                         bool dump_dependencies = false,
                         bool link              = true) {
    const std::filesystem::path build_path   = config.project.build_path;
    const std::filesystem::path project_path = config.project.project_path;

    const auto& cfg                 = config.configurations.at(std::string(config_name));
    const std::string compiler_path = cfg.compiler_path;

    std::filesystem::path out_path = build_path / relative(test_path, project_path);
    out_path.replace_extension(".so");
    out_path = weakly_canonical(out_path);

    std::vector<std::string> cmd = {compiler_path};
    cmd.append_range(config.expand_options(config_name));

    if (link && not dump_dependencies) {
      cmd.append_range(config.expand_link_options());

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

  TestRoot recompile(std::vector<std::filesystem::path> const& paths) {
    //! this function is not thread-safe, it shall only be invoked from the main thread
    auto tus = expand_tests(paths);
    for (auto&& tu : tus) {
      auto parent = std::filesystem::weakly_canonical(tu.out_path).parent_path();
      if (!parent.empty() && !exists(parent)) {
        create_directories(parent);
      }
      pool.submit(tu);
    }

    // std::println("Building {} test{}", tus.size(), tus.size() == 1 ? "" : "s");

    auto progress = [](auto done, auto total) {
      // constexpr static auto bar_width = 30;
      // auto filled = static_cast<int>((static_cast<double>(done) / total) * bar_width);
      // auto bar    = std::string(filled, '#') + std::string(bar_width - filled, ' ');
      // std::print("\r[{}] ({}/{})", bar, done, total);
      // std::fflush(stdout);
      auto doc = nlohmann::json({
          {"action", "batch_progress"},
          {  "done",             done},
          { "total",            total}
      });
      std::println("{}", doc.dump());
      std::fflush(stdout);
    };
    progress(0, tus.size());
    pool.wait(progress);
    // std::println("");

    TestRoot root;
    for (auto&& [tu, result] : pool.collect()) {
      if (result.exit_code != 0) {
        std::println("ERROR!");
        std::println("====== stdout ======\n{}", result.stdout_str);
        std::println("====== stderr ======\n{}", result.stderr_str);
        continue;
      }

      auto test_defs = units[tu.source_path].load(tu.out_path);
      for (auto def : test_defs) {
        auto expanded = def(tu.source_path);
        root.insert(expanded);
      }
    }
    return root;
  }

  void run_all() {
    auto test_inputs = discover_tests();
    auto root        = recompile(test_inputs);
    root.run(reporter.get());
    update_compdb();
  }

  void list_all() {
    auto test_inputs = discover_tests();
    auto root        = recompile(test_inputs);

    auto tests = nlohmann::json::array();
    for (auto const& test : root) {
      auto cases = nlohmann::json::array();
      for (auto const& test_case : test.get_tests()) {
        cases.push_back(test_case.name);
      }

      tests.push_back({
          {     "name",      test.name},
          {"full_name", test.full_name},
          {    "cases",          cases},
          {"path", test.sloc.file_name()}
      });
    }
    std::println("{}",
                 nlohmann::json({{"action", "list"},
                                 { "tests",  tests}})
                     .dump());
    std::fflush(stdout);
  }
};
}  // namespace rsl::testing::_impl_main