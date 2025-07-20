#pragma once
#include <vector>

#include <rsl/span>
#include <rsl/string_view>
#include <rsl/testing/annotations.hpp>

#include "util.hpp"

namespace rsl::testing::_testing_impl {
struct Annotations {  // consteval-only
  rsl::span<ParamSet const> targets;
  rsl::span<annotations::Params const> params;

  bool expect_failure = false;
  bool (*skip)()      = nullptr;  // this is a function to support conditional skipping
  rsl::string_view name;          // custom base name

  bool is_property_test = false;  // expand missing args as fixtures if false,
                                  // otherwise parameterize with domain
  bool is_fuzz_test = false;

  consteval explicit Annotations(std::meta::info fnc) {
    std::vector<ParamSet> tp_sets;
    std::vector<annotations::Params> p;

    for (auto annotation : annotations_of(fnc)) {
      auto type = remove_cvref(type_of(annotation));
      if (type == ^^annotations::TParams) {
        tp_sets.append_range(extract<annotations::TParams>(constant_of(annotation)).value);
      } else if (type == ^^annotations::Params) {
        p.push_back(extract<annotations::Params>(constant_of(annotation)));
      } else if (type == ^^annotations::ExpectFailureTag) {
        constexpr_assert(!expect_failure, "Cannot annotate with expect_failure more than once.");
        expect_failure = true;
      } else if (type == ^^annotations::Skip) {
        // constexpr_assert(skip == nullptr, "Cannot have more than one skip annotation.");
        skip = extract<annotations::Skip>(constant_of(annotation)).value;
      } else if (type == ^^annotations::Rename) {
        constexpr_assert(name.empty(), "Cannot rename more than once.");
        name = extract<annotations::Rename>(constant_of(annotation)).value;
      } else if (type == ^^annotations::PropertyTag) {
        is_property_test = true;
      } else if (type == ^^annotations::FuzzTag) {
        is_fuzz_test = true;
      }
    }

    targets = define_static_array(tp_sets);
    params  = define_static_array(p);

    if (skip == nullptr) {
      skip = &constant_predicate<false>;
    }
  }
};
}  // namespace rsl::testing::_testing_impl