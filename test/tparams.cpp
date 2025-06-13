#include <cpptest.hpp>
#include <tuple>
namespace {

[[=cpptest::test]]
[[=cpptest::tparams({std::tuple{^^int, 10}, {^^float, 2}})]]
constexpr inline auto tparam_gt_5 = []<typename T, int I>() static {
    ASSERT(I > 5);
};
}  // namespace
