#include "bisect/json.h"
#include <gtest/gtest.h>

using namespace bisect;
using json = nlohmann::json;

TEST(bisect_json, test_find_root)
{
    const auto s1 = R"({ "a": 1})";
    auto j        = json::parse(s1);
    const auto v  = find<int>(j, "a").value();
    ASSERT_TRUE(v == 1);
}

TEST(bisect_json, test_find_deep)
{
    const auto s1 = R"({ "a1": {"a2": {"a3": "value"}}})";
    auto j        = json::parse(s1);
    const auto v  = find<std::string>(j, "a1", "a2", "a3").value();
    ASSERT_TRUE(v == "value");
}
