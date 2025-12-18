#include "../taskset.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <vector>

namespace rsl::testing::_impl_main {
namespace {
void pin_to_cpu(int cpu) {
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu, &mask);
  if (sched_setaffinity(0, sizeof(mask), &mask) != 0) {
    perror("sched_setaffinity");
    _exit(1);
  }
}

ProcessResult run_program_on_cpu(int cpu, const char* program, char* const argv[]) {
  int out_pipe[2], err_pipe[2];
  if (pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
    perror("pipe");
    throw std::runtime_error("Failed to create pipe");
  }

  pid_t pid = fork();
  if (pid == 0) {
    // redirect stdout/stderr
    dup2(out_pipe[1], STDOUT_FILENO);
    dup2(err_pipe[1], STDERR_FILENO);
    close(out_pipe[0]);
    close(out_pipe[1]);
    close(err_pipe[0]);
    close(err_pipe[1]);
    if (cpu >= 0) {
      pin_to_cpu(cpu);
    }

    execvp(program, argv);
    perror("execvp failed");
    _exit(1);
  } else if (pid < 0) {
    perror("fork failed");
    throw std::runtime_error("fork failed");
  }

  // close write ends
  close(out_pipe[1]);
  close(err_pipe[1]);

  std::string stdout_str;
  std::string stderr_str;
  char buffer[4096];
  ssize_t n = 0;
  while ((n = read(out_pipe[0], &buffer[0], sizeof(buffer))) > 0) {
    stdout_str.append(&buffer[0], n);
  }
  while ((n = read(err_pipe[0], &buffer[0], sizeof(buffer))) > 0) {
    stderr_str.append(&buffer[0], n);
  }
  close(out_pipe[0]);
  close(err_pipe[0]);

  // Wait for child
  int status = 0;
  waitpid(pid, &status, 0);

  int exit_code = 0;
  if (WIFEXITED(status)) {
    exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    exit_code = 128 + WTERMSIG(status);
  } else {
    exit_code = -1;
  }

  return {exit_code, stdout_str, stderr_str};
}
}  // namespace

ProcessResult run_on_cpu(int cpu, std::string const& program, std::span<std::string const> argv) {
  std::vector<char*> args;
  if (argv.size() != 0 && argv[0] != program) {
    args.push_back((char*)program.c_str());
  }
  for (auto&& arg : argv) {
    args.push_back((char*)arg.c_str());
  }
  args.push_back(nullptr);
  return run_program_on_cpu(cpu, program.data(), args.data());
}
}  // namespace rsl::testing::_impl_main