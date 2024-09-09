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
