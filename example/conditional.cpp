#include <rsl/test>
#include <rsl/config>

namespace demo::conditional {

[[=rsl::test, =rsl::skip]] 
void never_run() {
  ASSERT(false);
}

static constexpr bool skip = true;
[[=rsl::test, =rsl::skip_if(skip)]] 
void skip_constant() {
  ASSERT(false);
}

bool should_skip() {
  // determine at runtime whether to skip or not
  return true;
}

[[=rsl::test, =rsl::skip_if(should_skip)]] 
void skip_runtime() {
  ASSERT(false);
}

struct CLI : rsl::cli_extension<CLI> {
  [[=option, =flag]] bool skip_cli = false;
};

[[=rsl::test, =rsl::skip_if([] { return CLI::value.skip_cli; })]] 
void skip_cli() {
  ASSERT(false, "Pass --skip-cli to disable this test.");
}

[[=rsl::test, =rsl::skip_if(&CLI::skip_cli)]] 
void skip_cli2() {
  ASSERT(false, "Pass --skip-cli to disable this test.");
}
}  // namespace
