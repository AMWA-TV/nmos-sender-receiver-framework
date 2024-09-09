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

#include "bisect/expected.h"
#include <string>

namespace bisect::core::detail
{
    template <typename T> inline bool is_error(const expected<T>& result)
    {
        return !result.has_value();
    }

    template <typename T> inline std::string get_error_msg(const expected<T>& result)
    {
        return result.error().what();
    }

    inline std::string make_error_msg(std::string_view expression, std::string error_message)
    {
        return std::string{"error: "} + error_message + " (in '" + std::string{expression} + "').";
    }

    template <typename Result>
    inline std::unexpected<std::runtime_error> make_error(const Result& result, std::string_view expression)
    {
        return std::unexpected{std::runtime_error{make_error_msg(expression, get_error_msg(result))}};
    }

} // namespace bisect::core::detail
