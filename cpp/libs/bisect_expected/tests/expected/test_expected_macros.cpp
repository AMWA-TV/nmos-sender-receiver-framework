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
    maybe_ok enforce_true(bool b)
    {
        BST_ENFORCE(b, "Failed!");
        return {};
    }

    maybe_ok do_fail()
    {
        BST_FAIL("throw");
    };

    maybe_ok do_check(bool fail)
    {
        if(fail)
        {
            BST_CHECK(do_fail());
        }

        return {};
    }

    template <typename T> expected<T> do_assign(expected<T> v)
    {
        BST_ASSIGN(a, v);
        return a;
    }

    template <typename T> expected<T> do_check_assign(expected<T> v)
    {
        int a{};
        BST_CHECK_ASSIGN(a, v);
        return a;
    }
} // namespace

TEST(bisect_expected_macros, test_enforce)
{
    ASSERT_TRUE(enforce_true(true).has_value());
    ASSERT_TRUE(is_error(enforce_true(false)));
}

TEST(bisect_expected_macros, test_fail)
{
    ASSERT_TRUE(is_error(do_fail()));
}

TEST(bisect_expected_macros, test_check)
{
    ASSERT_TRUE(is_error(do_check(true)));
    ASSERT_TRUE(do_check(false).has_value());
}

TEST(bisect_expected_macros, test_check_assign)
{
    auto v5 = do_check_assign(expected<int>{5});
    ASSERT_TRUE(v5.has_value());
    ASSERT_EQ(v5.value(), 5);

    auto ve = do_check_assign(expected<int>{std::unexpected(std::runtime_error(""))});
    ASSERT_TRUE(is_error(ve));
}
