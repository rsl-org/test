// #pragma once
// struct Reporter;
// struct TestNamespace {
//   std::string_view name;
//   std::vector<Test> tests;
//   std::vector<TestNamespace> children;

//   class iterator {
//     struct single_iterator {
//       std::vector<Test>::const_iterator it;
//       std::vector<Test>::const_iterator end;

//       bool operator==(single_iterator const& other) const {
//         return it == other.it && end == other.end;
//       }
//     };

//     single_iterator current;
//     std::deque<single_iterator> elements;

//     void flatten(TestNamespace const& current) {
//       for (auto const& ns : current.children) {
//         flatten(ns);
//       }
//       if (!current.tests.empty()) {
//         elements.push_back({current.tests.begin(), current.tests.end()});
//       }
//     }

//   public:
//     using iterator_category = std::input_iterator_tag;
//     using value_type        = Test;
//     using difference_type   = std::ptrdiff_t;
//     using pointer           = Test const*;
//     using reference         = Test const&;

//     iterator() = default;
//     explicit iterator(TestNamespace const& ns) {
//       flatten(ns);
//       current = elements.front();
//       elements.pop_front();
//     }

//     Test const& operator*() const { return *current.it; }
//     Test const* operator->() const { return &operator*(); }
//     iterator& operator++() {
//       if (current.it == current.end) {
//         if (elements.empty()) {
//           current = {};
//           return *this;
//         }

//         current = elements.front();
//         elements.pop_front();
//       } else {
//         ++current.it;
//         if (current.it == current.end) {
//           return ++*this;
//         }
//       }
//       return *this;
//     }
//     bool operator==(iterator const& other) const {
//       if (current.it != other.current.it || current.end != other.current.end) {
//         return false;
//       }
//       return elements == other.elements;
//     }
//   };

//   [[nodiscard]] bool is_empty() const { return tests.empty() && children.empty(); }
//   [[nodiscard]] iterator begin() const { return iterator{*this}; }
//   [[nodiscard]] static iterator end() { return {}; }
//   void insert(Test const& test, size_t i = 0) {
//     if (i == test.full_name.size() - 1) {
//       tests.push_back(test);
//       return;
//     }

//     auto it = std::ranges::find_if(children, [&](const TestNamespace& ns) {
//       return ns.name == test.full_name[i];
//     });

//     if (it == children.end()) {
//       children.emplace_back(test.full_name[i]);
//       it = std::prev(children.end());
//     }

//     it->insert(test, i + 1);
//   }
//   void remove_by_path(std::string_view path) {
//     auto matches_module_path = [&](Test const& test) {
//       return std::filesystem::path(test.module_path) == std::filesystem::path(path);
//     };

//     auto is_empty_namespace = [](TestNamespace const& ns) { return ns.is_empty(); };

//     for (auto& ns : children) {
//       ns.remove_by_path(path);
//     }
//     std::erase_if(children, is_empty_namespace);
//     std::erase_if(tests, matches_module_path);
//   }

//   [[nodiscard]] std::size_t count() const {
//     std::size_t total = tests.size();
//     for (auto const& ns : children) {
//       total += ns.count();
//     }
//     return total;
//   }

//   void filter(std::span<std::string const> parts) {
//     if (parts.empty()) {
//       return;
//     }

//     std::string_view current          = parts.front();
//     std::span<std::string const> next = parts.subspan(1);

//     auto it = std::ranges::find_if(children, [&](TestNamespace& ns) { return ns.name == current; });

//     if (it != children.end()) {
//       tests.clear();
//       it->filter(next);
//       if (it->children.empty() && it->tests.empty()) {
//         children.clear();
//       } else {
//         children = {*it};
//       }
//       return;
//     } else {
//       std::erase_if(tests, [&](const Test& t) { return t.name != current; });
//       children.clear();
//     }
//   }
//   bool run(Reporter* reporter) {
//     if (!name.empty()) {
//       reporter->enter_namespace(name);
//     }
//     bool status = true;
//     for (auto& ns : children) {
//       status &= ns.run(reporter);
//     }

//     for (auto& test : tests) {
//       auto runs = test.get_tests();
//       reporter->before_test_group(test);
//       std::vector<Result> results;
//       if (!test.skip()) {
//         for (auto const& test_run : test.get_tests()) {
//           auto& tracker      = _testing_impl::assertion_counter();
//           tracker.assertions = {};
//           tracker.test_name  = join_str(test.full_name, "::");

//           reporter->before_test(test_run);
//           auto result       = test_run.run();
//           result.assertions = tracker.assertions;

//           reporter->after_test(result);
//           results.push_back(result);
//         }
//       } else {
//         reporter->before_test(TestCase{&test, +[] {}, std::string(test.name)});

//         // TODO stringify skipped tests properly
//         auto result = Result{&test, std::string(test.name) + "(...)", TestOutcome::SKIP};
//         reporter->after_test(result);
//         results.push_back(result);
//       }

//       reporter->after_test_group(results);
//     }
//     if (!name.empty()) {
//       reporter->exit_namespace(name);
//     }
//     return status;
//   }
// };

// struct TestRoot : TestNamespace {
//   bool run(Reporter* reporter, bool summarize = true) {
//     // libassert::set_failure_handler(failure_handler);
//     // std::println("failure handler set");
//     reporter->before_run(*this);
//     bool status = TestNamespace::run(reporter);
//     // libassert::set_failure_handler(libassert::default_failure_handler);
//     // TODO after_run
//     reporter->after_run();
//     return status;
//   }
// };

// TestRoot get_tests() {
//   TestRoot root;
//   for (auto test_def : rsl::testing::_testing_impl::registry()) {
//     auto test = test_def({});
//     root.insert(test);
//   }
//   return root;
// }