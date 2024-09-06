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
