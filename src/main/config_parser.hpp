#pragma once
#include <ranges>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <format>
#include <fstream>

#include <nlohmann/json.hpp>
#include "platform/taskset.hpp"

namespace rsl::testing::_impl_main {
struct TestTU {
  ProgramInvocation invocation;
  std::filesystem::path out_path;
};

class ConfigParser {
  static nlohmann::json json_from_file(std::filesystem::path const& path) {
    std::ifstream stream{path};
    if (!stream) {
      throw std::runtime_error("Unable to load config ");
    }

    nlohmann::json config;
    stream >> config;
    return config;
  }

public:
  explicit ConfigParser(std::filesystem::path const& path) : ConfigParser(json_from_file(path)) {}

  explicit ConfigParser(nlohmann::json cfg) : config_(std::move(cfg)) {
    validate_project_section();
  }

  static std::vector<std::filesystem::path> discover_tests(const std::filesystem::path& root) {
    static constexpr std::array allowed{".cpp"};
    std::vector<std::filesystem::path> result;

    for (auto const& entry : std::filesystem::recursive_directory_iterator(root)) {
      if (entry.is_regular_file()) {
        auto const ext = entry.path().extension().string();
        if (std::ranges::contains(allowed, ext))
          result.push_back(entry.path());
      }
    }
    return result;
  }

  [[nodiscard]]
  TestTU make_invocation(std::string_view config_name,
                         const std::filesystem::path& test_path) const {
    const auto& project        = config_.at("project");
    const auto& options        = config_.value("options", nlohmann::json::object());
    const auto& configurations = config_.at("configurations");

    const std::filesystem::path build_path   = project.at("build_path").get<std::string>();
    const std::filesystem::path project_path = project.at("project_path").get<std::string>();

    const nlohmann::json& cfg = configurations.at(config_name.data());

    const bool ext                  = cfg["CXX"].value("extensions", false);
    const int ver                   = cfg["CXX"].value("standard", 17);
    const std::string standard      = std::format("-std={}++{}", (ext ? "gnu" : "c"), ver);
    const std::string compiler_path = cfg.value("compiler_path", "c++");

    std::filesystem::path out_path =
        build_path / std::filesystem::relative(test_path, project_path);
    out_path.replace_extension(".so");

    std::vector<std::string> cmd;

    cmd.push_back(compiler_path);
    cmd.push_back(standard);

    if (options.contains("compile_options")) {
      for (auto const& o : options["compile_options"]) {
        auto value = o.get<std::string>();
        if (!value.empty()) {
          cmd.push_back(value);
        }
      }
    }

    if (options.contains("include_dirs")) {
      for (auto const& p : options["include_dirs"]) {
        auto s = p.get<std::string>();
        if (!s.empty()) {
          cmd.push_back(std::format("-I{}", s));
        }
      }
    }

    if (options.contains("compile_definitions")) {
      for (auto const& d : options["compile_definitions"]) {
        auto value = d.get<std::string>();
        if (!value.empty()) {
          cmd.push_back(std::format("-D{}", value));
        }
      }
    }

    if (options.contains("link_libraries")) {
      for (auto const& lib : options["link_libraries"]) {
        auto value = lib.get<std::string>();
        if (!value.empty()) {
          if (value[0] == '/') {
            cmd.push_back(std::format("-L{}", value));
          } else {
            cmd.push_back(std::format("-l{}", value));
          }
        }
      }
    }

    // cmd.push_back(std::format("-Wl,-rpath,{}", build_path.string()));
    // cmd.push_back(std::format("-L{}", build_path.string()));
    cmd.emplace_back("-fPIC");
    cmd.emplace_back("-shared");

    // out file
    cmd.emplace_back("-o");
    cmd.push_back(out_path.string());

    // input file
    cmd.push_back(test_path.string());

    return {compiler_path, cmd, out_path};
  }

  std::vector<TestTU> expand() const {
    const auto& project = config_.at("project");
    std::vector<std::filesystem::path> tests;

    for (auto const& p : project.at("test_path")) {
      for (auto&& t : discover_tests(std::filesystem::path(p.get<std::string>()))) {
        tests.push_back(std::move(t));
      }
    }

    std::vector<TestTU> test_tus;
    for (auto const& [name, _] : config_.at("configurations").items()) {
      for (auto const& test : tests) {
        test_tus.push_back(make_invocation(name, test));
      }
    }
    return test_tus;
  }

  nlohmann::json config_;
private:

  void validate_project_section() {
    if (!config_.contains("project")) {
      throw std::runtime_error("Missing required 'project' section in test-runner.nlohmann::json");
    }

    const auto& project = config_["project"];
    for (std::string_view key : {"build_path", "project_path", "test_path"}) {
      if (!project.contains(key)) {
        throw std::runtime_error(std::format("Missing required project.{} field", key));
      }
    }
  }
};

}  // namespace rsl::testing::_impl_main