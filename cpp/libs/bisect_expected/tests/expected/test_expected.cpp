#include "bisect/expected/macros.h"
#include <gtest/gtest.h>

using namespace bisect;
using namespace bisect::core::detail;

namespace
{
    expected<bool> f_ok()
    {
        return {true};
    }

    expected<bool> f_fail()
    {
        return std::unexpected(std::runtime_error("error"));
    }
} // namespace

TEST(bisect_expected, test_expected)
{

    ASSERT_TRUE(f_ok().has_value());
}

TEST(bisect_expected, test_unexpected)
{
    ASSERT_TRUE(f_fail().error().what() == std::string_view("error"));
}
