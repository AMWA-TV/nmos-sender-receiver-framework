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
#include <nlohmann/json.hpp>
#include <functional>
#include <optional>

namespace bisect
{
    inline expected<nlohmann::json> parse_json(std::string_view s)
    {
        try
        {
            return nlohmann::json::parse(s);
        }
        catch(std::exception& ex)
        {
            BST_FAIL("error parsing JSON: {}", ex.what());
        }
    }

    namespace detail
    {
        template <class C, typename T> T get_pointer_type(T C::*v);

        template <typename Tag> bool is_valid(const nlohmann::json& j);

        template <> inline bool is_valid<float>(const nlohmann::json& j)
        {
            return j.is_number();
        }

        template <> inline bool is_valid<int>(const nlohmann::json& j)
        {
            return j.is_number();
        }

        template <> inline bool is_valid<int64_t>(const nlohmann::json& j)
        {
            return j.is_number();
        }

        template <> inline bool is_valid<unsigned long>(const nlohmann::json& j)
        {
            return j.is_number();
        }

        template <> inline bool is_valid<unsigned short>(const nlohmann::json& j)
        {
            return j.is_number();
        }

        template <> inline bool is_valid<unsigned int>(const nlohmann::json& j)
        {
            return j.is_number();
        }

        template <> inline bool is_valid<uint8_t>(const nlohmann::json& j)
        {
            return j.is_number();
        }

        template <> inline bool is_valid<std::string>(const nlohmann::json& j)
        {
            return j != nullptr && j.is_string();
        }

        template <> bool inline is_valid<bool>(const nlohmann::json& j)
        {
            return j.is_boolean();
        }

        template <> bool inline is_valid<nlohmann::json>(const nlohmann::json&)
        {
            return true;
        }
    } // namespace detail

    template <typename T> [[nodiscard]] inline expected<T> get_as(const nlohmann::json& j)
    {
        BST_ENFORCE(detail::is_valid<T>(j), "JSON element does not have the right type: {}", j.dump());
        return expected<T>(j.get<T>());
    }

    template <typename R> expected<R> find(const nlohmann::json& j, std::string_view next)
    {
        const auto it = j.find(next);
        BST_ENFORCE(it != j.end(), "Value with key '{}' not found in {}", next, j.dump());
        return get_as<R>(*it);
    }

    template <typename R, typename... Ts> expected<R> find(const nlohmann::json& j, std::string_view next, Ts... rest)
    {
        const auto value_it = j.find(next);
        BST_ENFORCE(value_it != j.end(), "Value with key '{}' not found", next);
        return find<R>(*value_it, rest...);
    }

    template <typename T, typename R = T>
    [[nodiscard]] inline expected<R> find_or(const nlohmann::json& j, std::string_view name, R&& default_value = T{})
    {
        const auto it = j.find(name);
        if(it == j.end())
        {
            return default_value;
        }

        return get_as<R>(*it);
    }

    template <typename T, typename O, typename M, typename R = decltype(detail::get_pointer_type(M{})),
              typename F = std::function<R(const T&)>>
    maybe_ok assign_if(
        const nlohmann::json& j, std::string_view name, O& o, M member, F conversion = [](const auto& v) { return v; })
    {
        if(const auto it = j.find(name); it != j.end())
        {
            if(!detail::is_valid<T>(*it))
            {
                const auto message = fmt::format("Invalid type for {}", j.dump());
                fmt::print("{}\n", message);
                return std::unexpected(std::runtime_error(message));
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
            o.*member = conversion(it->get<T>());
#pragma GCC diagnostic pop
            return {};
        }

        const auto message = fmt::format("{} not found in {}", name, j.dump());
        return std::unexpected(std::runtime_error(message));
    }

    template <typename T, typename O, typename M, typename R = decltype(detail::get_pointer_type(M{})),
              typename F = std::function<R(const T&)>>
    maybe_ok assign_if_or(
        const nlohmann::json& j, std::string_view name, O& o, M member, F conversion = [](const auto& v) { return v; },
        R default_value = R{})
    {
        if(const auto it = j.find(name); it != j.end())
        {
            if(!detail::is_valid<T>(*it))
            {
                o.*member = default_value;

                const auto message = fmt::format("Invalid type for {}", j.dump());
                fmt::print("{}\n", message);
                return std::unexpected(std::runtime_error(message));
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
            o.*member = conversion(it->get<T>());
#pragma GCC diagnostic pop

            return {};
        }

        o.*member = default_value;
        return {};
    }

    template <typename T, typename O, typename M, typename R = decltype(detail::get_pointer_type(M{})),
              typename F = std::function<R(const T&)>>
    maybe_ok assign_if_or_value(
        const nlohmann::json& j, std::string_view name, O& o, M member, T value,
        F conversion = [](const auto& v) { return v; })
    {
        if(const auto it = j.find(name); it != j.end())
        {
            if(!detail::is_valid<T>(*it))
            {
                const auto message = fmt::format("Invalid type for {}", j.dump());
                fmt::print("{}\n", message);
                return std::unexpected(std::runtime_error(message));
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
            o.*member = conversion(it->get<T>());
#pragma GCC diagnostic pop
            return {};
        }

        o.*member = value;
        return {};
    }

    template <typename R> expected<std::optional<R>> maybe_find(const nlohmann::json& j, std::string_view next)
    {
        const auto it = j.find(next);
        if(it == j.end()) return std::nullopt;
        BST_ASSIGN(result, get_as<R>(*it));
        return result;
    }

    template <typename R, typename... Ts>
    expected<std::optional<R>> maybe_find(const nlohmann::json& j, std::string_view next, Ts... rest)
    {
        const auto it = j.find(next);
        if(it == j.end()) return std::nullopt;
        return maybe_find<R>(*it, rest...);
    }

    namespace selectors
    {
        using json = nlohmann::json;

        using mapper_t = std::function<expected<json>(const json&)>;

        namespace detail
        {
            template <typename R, typename T> expected<R> do_select(const json& j, T matcher)
            {
                BST_ASSIGN(level, matcher(j));
                return get_as<R>(level);
            }

            template <typename R, typename T, typename... Ts>
            expected<R> do_select(const json& j, T matcher, Ts... matchers)
            {
                BST_ASSIGN(level, matcher(j));
                return do_select<R>(level, matchers...);
            }
        } // namespace detail

        inline auto element(std::string name) -> mapper_t
        {
            return [name](const json& j) -> expected<json> { return find<json>(j, name); };
        }

        inline auto index(json::size_type idx) -> mapper_t
        {
            return [idx](const json& j) -> expected<json> {
                BST_ENFORCE(idx < j.size(), "Index is greater than array length: {} {}", j.dump(), idx);
                return expected<json>{j[idx]};
            };
        }

        inline auto name_value(std::string name) -> mapper_t
        {
            return [name](const json& j) -> expected<json> {
                BST_ENFORCE(j.is_array(), "JSON value is not an array.");

                for(const auto& element : j)
                {
                    BST_ASSIGN(actual_name, find<std::string>(element, "name"));
                    if(actual_name == name)
                    {
                        return find<json>(element, "value");
                    }
                }

                BST_FAIL("JSON does not contain {}: {}", name, j.dump());
            };
        }
    } // namespace selectors

    template <typename R, typename... Ts> expected<R> select(const nlohmann::json& j, Ts... matchers)
    {
        return selectors::detail::do_select<R>(j, matchers...);
    }
} // namespace bisect
