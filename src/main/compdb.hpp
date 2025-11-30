#pragma once
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <optional>

namespace rsl::testing::_impl_main {
struct CompileCommand {
  std::string directory;
  std::string file;

  std::optional<std::vector<std::string>> arguments;
  std::optional<std::string> command;

  std::optional<std::string> output;

  [[nodiscard]] nlohmann::json to_json() const {
    auto obj = nlohmann::json{
        {"directory", directory},
        {     "file",      file}
    };
    if (arguments.has_value()) {
      auto cmd_range = *arguments | std::views::join_with(std::string_view(" "));
      std::string cmd(cmd_range.begin(), cmd_range.end());
      obj.emplace("command", cmd);
    } else if (command.has_value()) {
      obj.emplace("command", *command);
    }

    if (output.has_value()) {
      obj.emplace("output", *output);
    }
    return obj;
  }

  static CompileCommand from_json(nlohmann::json const& doc) {
    CompileCommand out{.directory = doc.at("directory").get<std::string>(),
                       .file      = doc.at("file").get<std::string>()};
    if (doc.count("arguments") != 0) {
      out.arguments = doc.at("arguments").get<std::vector<std::string>>();
    } else if (doc.count("command") != 0) {
      out.command = doc.at("command").get<std::string>();
    }

    if (doc.count("output") != 0) {
      out.output = doc.at("output").get<std::string>();
    }
    return out;
  }

  bool operator==(CompileCommand const& other) const {
    return directory == other.directory && file == other.file;
  }
};

class CompileCommands {
  std::filesystem::path path_;
  std::vector<CompileCommand> entries_;

public:
  explicit CompileCommands(std::filesystem::path path = "compile_commands.json") : path_(std::move(path)) {}

  void load() {
    if (std::filesystem::exists(path_)) {
      entries_.clear();
      std::ifstream f(path_);
      nlohmann::json j;
      f >> j;
      for (auto& el : j) {
        entries_.push_back(CompileCommand::from_json(el));
      }
    }
  }

  void save() const {
    std::ofstream f(path_);
    nlohmann::json j = nlohmann::json::array();
    for (auto const& e : entries_) {
      j.push_back(e.to_json());
    }
    f << j.dump(2) << "\n";
  }

  void append_if_missing(CompileCommand cmd) {
    auto it = std::ranges::find(entries_, cmd);
    if (it == entries_.end()) {
      entries_.push_back(std::move(cmd));
    }
  }

  std::optional<CompileCommand> find_for_file(const std::string& file) const {
    auto it = std::ranges::find_if(entries_, [&](auto const& e) { return e.file == file; });
    if (it != entries_.end()) {
      return *it;
    }
    return std::nullopt;
  }

  const std::vector<CompileCommand>& entries() const { return entries_; }
};

}  // namespace rsl::testing::_impl_main