#include <rsl/test>
#include <tuple>

namespace demo::params {

consteval std::vector<rsl::testing::ParamSet> make_tparams() {
    return {{^^double, 13}, {^^short, 14}};
}

[[=rsl::test]]
[[=rsl::tparams(make_tparams)]]
[[=rsl::tparams({{^^int, 10}, {^^float, 21}})]]
[[=rsl::params({{42, 'c'}, {12, 'b'}})]]
constexpr inline auto tparam_gt_5 = []<typename T, int I>(int a, char b) static {
    ASSERT(I > 5);
};
}  // namespace
