#include <rsl/test>
#include <tuple>

namespace test_discovery
{
    int called = 0;
    [[= rsl::test]]
    void test_called()
    {
        ++called;
        ASSERT(true);
    }

    void test_not_called()
    {
        --called;
        ASSERT(false);
    }

    [[= rsl::test]]
    [[= rsl::rename("renamed_test")]]
    void ensure_called()
    {
        ASSERT(called == 1);
    }


    [[= rsl::test]]
    [[=rsl::params({{'a', 10}, {'c', 12}})]]
    void test_with_params(char foo, int bar){
        ASSERT(bar > 5);
        ASSERT(foo != 'x');
    };

} // namespace test_discovery
