#pragma once

#include <expected>
#include <stdexcept>

namespace bisect
{
    template <typename T> using expected = std::expected<T, std::runtime_error>;

    struct result_ok
    {
    };

    using maybe_ok = std::expected<result_ok, std::runtime_error>;
} // namespace bisect
