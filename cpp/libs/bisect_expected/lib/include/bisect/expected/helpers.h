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
