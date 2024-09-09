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

#include "bisect/expected/macros.h"
#include <nmos/id.h>
#include <vector>

namespace ossrf
{
    /** Iterates over a container and returns:
     * - ok if there are no errors;
     * - the first error.
     */
    inline bisect::maybe_ok fold_maybe_ok(const std::vector<bisect::maybe_ok>& errors)
    {
        auto maybe_error = std::find_if(errors.begin(), errors.end(), [](const bisect::maybe_ok& result) {
            return bisect::core::detail::is_error(result);
        });

        if(maybe_error == std::end(errors))
        {
            return {};
        }
        return std::move(*maybe_error);
    }

    /**
     * Calls f for each of the items and returns a folded result: ok if no errors; the first error otherwise.
     */
    template <typename F, typename T> inline bisect::maybe_ok fold_call(const std::vector<T>& items, F f)
    {
        std::vector<bisect::maybe_ok> result;
        result.reserve(items.size());
        std::transform(items.begin(), items.end(), std::back_inserter(result), f);

        return fold_maybe_ok(result);
    }

    template <typename V> inline auto ret(const bisect::expected<V>&) -> V;

    /**
     * Calls f for each of the items and returns either: the results vector, if no errors; the first error otherwise.
     */
    template <typename F, typename Ib, typename Ie>
    inline auto call_many(Ib begin, Ie end, F f) -> bisect::expected<std::vector<decltype(ret(f(*begin)))>>
    {
        using R = decltype(ret(f(*begin)));

        std::vector<R> result;
        result.reserve(static_cast<size_t>(std::distance(begin, end)));

        for(; begin != end; ++begin)
        {
            BST_ASSIGN(r, f(*begin));
            result.push_back(std::move(r));
        }

        return result;
    }

    template <typename T> std::vector<nmos::id> get_ids(const T& items)
    {
        std::vector<nmos::id> id_list{};
        id_list.reserve(items.size());
        std::transform(items.begin(), items.end(), std::back_inserter(id_list),
                       [](const auto& item) { return utility::s2us(item.id); });
        return id_list;
    }
} // namespace ossrf
