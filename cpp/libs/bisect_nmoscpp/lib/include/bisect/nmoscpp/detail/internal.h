#pragma once

//////////////////////////////////////////////////////////////////////////////
// Work around a missing forward declaration in cpprest
#include <cpprest/json.h>

namespace web
{
    namespace json
    {
        bool operator<(const web::json::value& lhs, const web::json::value& rhs);
    }
} // namespace web

#include <cpprest/json_ops.h>
//////////////////////////////////////////////////////////////////////////////
