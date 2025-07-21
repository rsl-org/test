#include <fstream>
#include <stdexcept>
#include <cstdio>
#include <rsl/testing/output.hpp>

namespace rsl::testing {
class ConsoleOutput : public Output {
public:
  void print(std::string_view message) override {
    (void)std::fwrite(message.data(), sizeof(char), message.size(), stdout);
  }
};

class FileOutput : public Output {
public:
  explicit FileOutput(std::string const& filename)
      : file_(filename, std::ios::out | std::ios::trunc) {
    if (!file_.is_open()) {
      throw std::runtime_error("Failed to open file: " + filename);
    }
  }

  void print(std::string_view message) override { file_ << message; }

  ~FileOutput() override {
    if (file_.is_open()) {
      file_.flush();
      file_.close();
    }
  }

private:
  std::ofstream file_;
};
}  // namespace rsl::testing