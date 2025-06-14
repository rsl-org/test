#include <retest.hpp>
#include <tuple>
namespace {

[[=re::test]]
[[=re::tparams({std::tuple{^^int, 10}, {^^float, 21}})]]
constexpr inline auto tparam_gt_5 = []<typename T, int I>() static {
    ASSERT(I > 5);
};
}  // namespace
