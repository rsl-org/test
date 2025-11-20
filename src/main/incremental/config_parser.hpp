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

template <std::ranges::input_range R>
auto filter_empty(R&& range) {
  return range | std::views::filter([](auto e) { return not e.empty(); }) |
         std::ranges::to<std::vector>();
}

struct Project {
  std::vector<std::filesystem::path> test_path;
  std::filesystem::path build_path;
  std::filesystem::path project_path;
  std::string namespace_;  // TODO rename via annotation
};

void to_json(nlohmann::json& doc, Project const& p) {
  doc = {
      {   "test_path",    p.test_path},
      {  "build_path",   p.build_path},
      {"project_path", p.project_path},
      {   "namespace",   p.namespace_}
  };
}
void from_json(nlohmann::json const& doc, Project& p) {
  doc.at("test_path").get_to(p.test_path);
  p.test_path = filter_empty(p.test_path);
  doc.at("build_path").get_to(p.build_path);
  doc.at("project_path").get_to(p.project_path);
  doc.at("namespace").get_to(p.namespace_);
}

struct Options {
  std::vector<std::filesystem::path> include_dirs;
  std::vector<std::string> compile_options;
  std::vector<std::string> compile_definitions;
  std::vector<std::string> link_options;
  std::vector<std::filesystem::path> link_libraries;
};
void to_json(nlohmann::json& doc, Options const& p) {
  doc = {
      {       "include_dirs",        p.include_dirs},
      {    "compile_options",     p.compile_options},
      {"compile_definitions", p.compile_definitions},
      {       "link_options",        p.link_options},
      {     "link_libraries",      p.link_libraries}
  };
}
void from_json(nlohmann::json const& doc, Options& p) {
  doc.at("include_dirs").get_to(p.include_dirs);
  p.include_dirs = filter_empty(p.include_dirs);
  doc.at("compile_options").get_to(p.compile_options);
  p.compile_options = filter_empty(p.compile_options);
  doc.at("compile_definitions").get_to(p.compile_definitions);
  p.compile_definitions = filter_empty(p.compile_definitions);
  doc.at("link_options").get_to(p.link_options);
  p.link_options = filter_empty(p.link_options);
  doc.at("link_libraries").get_to(p.link_libraries);
  p.link_libraries = filter_empty(p.link_libraries);
}

struct Configuration {
  std::filesystem::path compiler_path = "c++";
  std::string mode                    = "CXX";
  unsigned standard                   = 26;
  bool gnu_extensions                 = false;
};
void to_json(nlohmann::json& doc, Configuration const& p) {
  doc = {
      { "compiler_path",  p.compiler_path},
      {          "mode",           p.mode},
      {      "standard",       p.standard},
      {"gnu_extensions", p.gnu_extensions}
  };
}
void from_json(nlohmann::json const& doc, Configuration& p) {
  doc.at("compiler_path").get_to(p.compiler_path);
  doc.at("mode").get_to(p.mode);
  doc.at("standard").get_to(p.standard);
  doc.at("gnu_extensions").get_to(p.gnu_extensions);
}

struct RunnerConfig {
  std::string target;
  Project project;
  Options options;
  std::unordered_map<std::string, Configuration> configurations;
};

void to_json(nlohmann::json& doc, RunnerConfig const& p) {
  doc = {
      {        "target",         p.target},
      {       "project",        p.project},
      {       "options",        p.options},
      {"configurations", p.configurations}
  };
}

void from_json(nlohmann::json const& doc, RunnerConfig& p) {
  doc.at("target").get_to(p.target);
  doc.at("project").get_to(p.project);
  doc.at("options").get_to(p.options);
  doc.at("configurations").get_to(p.configurations);
}

inline RunnerConfig load_runner_config(std::filesystem::path const& path) {
  std::ifstream stream{path};
  if (!stream) {
    throw std::runtime_error("Unable to load config ");
  }

  nlohmann::json config;
  stream >> config;
  return config.get<RunnerConfig>();
}

}  // namespace rsl::testing::_impl_main