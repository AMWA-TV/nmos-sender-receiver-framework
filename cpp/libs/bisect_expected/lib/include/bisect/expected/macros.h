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

#pragma once

#include "bisect/expected/helpers.h"
#include "bisect/fmt.h"

#define BISECT_TOKEN_PASTE_1(x, y) x##y
#define BISECT_TOKEN_PASTE(x, y) BISECT_TOKEN_PASTE_1(x, y)

#define BST_CHECK_IMPL(COUNTER, EXPR)                                                                                  \
    {                                                                                                                  \
        const auto& BISECT_TOKEN_PASTE(result_, COUNTER) = (EXPR);                                                     \
        if(bisect::core::detail::is_error(BISECT_TOKEN_PASTE(result_, COUNTER)))                                       \
        {                                                                                                              \
            return bisect::core::detail::make_error(BISECT_TOKEN_PASTE(result_, COUNTER), #EXPR);                      \
        }                                                                                                              \
    }

#define BST_CHECK(EXPR) BST_CHECK_IMPL(__COUNTER__, EXPR)

#define BST_ASSIGN(VAR, EXPR)                                                                                          \
    auto BISECT_TOKEN_PASTE(Unique_, __LINE__) = (EXPR);                                                               \
    BST_CHECK(BISECT_TOKEN_PASTE(Unique_, __LINE__));                                                                  \
    const auto VAR = std::move(BISECT_TOKEN_PASTE(Unique_, __LINE__)).value();

#define BST_ASSIGN_MUT(VAR, EXPR)                                                                                      \
    auto BISECT_TOKEN_PASTE(Unique_, __LINE__) = (EXPR);                                                               \
    BST_CHECK(BISECT_TOKEN_PASTE(Unique_, __LINE__));                                                                  \
    auto VAR = std::move(BISECT_TOKEN_PASTE(Unique_, __LINE__)).value();

#define BST_CHECK_ASSIGN(VAR, EXPR)                                                                                    \
    auto BISECT_TOKEN_PASTE(Unique_, __LINE__) = (EXPR);                                                               \
    BST_CHECK(BISECT_TOKEN_PASTE(Unique_, __LINE__));                                                                  \
    VAR = std::move(BISECT_TOKEN_PASTE(Unique_, __LINE__)).value();

#define BST_ENFORCE_IMPL(COUNTER, EXPR, ...)                                                                           \
    {                                                                                                                  \
        if(!(EXPR))                                                                                                    \
        {                                                                                                              \
            const auto BISECT_TOKEN_PASTE(core_message, COUNTER) = fmt::format(__VA_ARGS__);                           \
            const auto BISECT_TOKEN_PASTE(t, COUNTER) =                                                                \
                fmt::format("{} - at '{}', line {}, '{}'", BISECT_TOKEN_PASTE(core_message, COUNTER), __FILE__,        \
                            __LINE__, __PRETTY_FUNCTION__);                                                            \
            return std::unexpected(std::runtime_error(BISECT_TOKEN_PASTE(t, COUNTER)));                                \
        }                                                                                                              \
    }

#define BST_ENFORCE(EXPR, ...) BST_ENFORCE_IMPL(__COUNTER__, EXPR, __VA_ARGS__)

#define BST_FAIL(...)                                                                                                  \
    {                                                                                                                  \
        const auto core_message = fmt::format(__VA_ARGS__);                                                            \
        const auto t =                                                                                                 \
            fmt::format("{} - at '{}', line {}, '{}'", core_message, __FILE__, __LINE__, __PRETTY_FUNCTION__);         \
        return std::unexpected(std::runtime_error(t));                                                                 \
    }
