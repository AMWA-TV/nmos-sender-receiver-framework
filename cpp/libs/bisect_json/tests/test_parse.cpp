// Copyright (C) 2024 Advanced Media Workflow Association
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
