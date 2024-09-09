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

#include <variant>

namespace bisect
{
    template <class... Ts> struct overload : Ts...
    {
        using Ts::operator()...;
    };
    template <class... Ts> overload(Ts...) -> overload<Ts...>;

    template <typename Variant, typename... Matchers> auto match(Variant&& variant, Matchers&&... matchers)
    {
        return std::visit(overload{std::forward<Matchers>(matchers)...}, std::forward<Variant>(variant));
    }
} // namespace bisect
