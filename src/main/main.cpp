#include <filesystem>

#include <nlohmann/json.hpp>

#include <rsl/testing/_testing_impl/discovery.hpp>
#include <rsl/testing/output.hpp>
#include <string>
#include <unordered_map>

#include "incremental/incremental.hpp"
#include "incremental/platform/library.hpp"
#include "incremental/platform/stdin.hpp"
#include "incremental/platform/event_loop.hpp"
#include "incremental/platform/taskset.hpp"

struct HeaderDependencies {
  std::filesystem::path file;
  std::vector<std::filesystem::path> dependencies;
};

std::string_view trim(std::string_view data) {
  auto first = data.find_first_not_of(" \t");
  auto last  = data.find_last_not_of(" \t");
  if (first == last) {
    return {};
  }
  return data.substr(first, last - first + 1);
}

HeaderDependencies parse_dependencies(std::string_view input) {
  auto colon = input.find(':');
  if (colon == std::string_view::npos) {
    return {};
  }
  input.remove_prefix(colon + 1);

  auto newline = input.find('\n');
  if (newline == std::string_view::npos) {
    return {};
  }
  auto tu = std::string_view(input.begin(), newline);
  if (!tu.empty() && tu.back() == '\\') {
    tu.remove_suffix(1);
  }
  input.remove_prefix(newline);

  std::vector<std::filesystem::path> deps;

  for (auto line : input | std::views::split('\n')) {
    auto line_sv = std::string_view(&*line.begin(), std::ranges::distance(line));
    if (!line_sv.empty() && line_sv.back() == '\\') {
      line_sv.remove_suffix(1);
    }

    if (!line_sv.empty()) {
      deps.emplace_back(trim(line_sv));
    }
  }

  return {tu, std::move(deps)};
}

bool is_relative_to(std::filesystem::path const& path, std::filesystem::path const& base) {
  auto abs_path           = std::filesystem::weakly_canonical(path);
  auto abs_base           = std::filesystem::weakly_canonical(base);
  auto [it_path, it_base] = std::ranges::mismatch(abs_path, abs_base);
  return it_base == abs_base.end();
}

int main() {
  using namespace rsl::testing::_impl_main;
  constexpr bool incremental        = false;
  constexpr bool watch_dependencies = true;

  const auto executable_path = std::filesystem::canonical("/proc/self/exe").parent_path();
  const std::filesystem::path config_path = executable_path / "test-runner.json";

  auto runner      = IncrementalRunner(config_path);
  auto test_inputs = runner.discover_tests();

  auto dep_map = std::unordered_map<std::filesystem::path, std::set<std::filesystem::path>>();
  auto dep_folders = std::set<std::filesystem::path>();

  for (auto&& tu : runner.expand_tests(test_inputs, true)) {
    runner.pool.submit(tu);
  }
  runner.pool.wait();

  for (auto [_, result] : runner.pool.collect()) {
    if (result.exit_code != 0) {
      continue;
    }
    auto [tu, dependencies] = parse_dependencies(result.stdout_str);
    for (auto const& dependency : dependencies) {
      if (not is_relative_to(dependency, runner.config.project.project_path)) {
        continue;
      }

      dep_folders.insert(std::filesystem::canonical(dependency).parent_path());
      dep_map[dependency].insert(tu);
    }
  }
 
  runner.recompile(runner.expand_tests(test_inputs));

  rsl::testing::TestRoot root;

  auto update_tree = [&](auto file_path) {
    // remove updated tests from tree
    // for (auto&& [path, _] : runner.test_sets) {
    //   root.remove_by_path(path.string());
    // }

    // rebuild root
    root = {};
    // insert
    rsl::testing::TestRoot tests;
    for (auto&& [path, test_set] : runner.test_sets) {
      // std::println("{} -> {}", path.string(), test_set.tests.size());
      for (auto test_def : test_set.tests) {
        auto test = test_def(path);
        root.insert(test);
        // if (file_path == path) {
        tests.insert(test);
        // }
      }
    }
    return tests;
  };
  update_tree("");

  std::unique_ptr<rsl::testing::Reporter> selected_reporter;
  selected_reporter = rsl::testing::Reporter::make("plain");
  root.run(selected_reporter.get());

  for (auto& [path, test_set] : runner.test_sets) {
    unload_library(test_set.handle);
    test_set.tests = {};
  }

  if (incremental) {
    Watcher watch{runner};
    for (auto const& path : runner.config.project.test_path) {
      watch.add_watch(path);
    }

    if (watch_dependencies) {
      // compile TUs with -MM to collect dependencies, dedupe and watch unique parents
      for (auto&& [path, _] : runner.test_sets) {
        // runner.collect_dependencies(path);
      }
    }
    // auto watch_fnc = [&](auto path, FileEvent event) {
    //   if ((event & FileEvent::MODIFY) == FileEvent::MODIFY) {
    //     // compile TU
    //     runner.recompile(runner.expand_tests({path}));
    //     // load TU
    //     auto updated = update_tree(path);
    //     // updated.run(selected_reporter.get(), false);
    //   }
    // };
    TerminalCommand commands{runner, watch};

    auto loop = EventLoop(commands, watch);
    loop.run();
  }
}