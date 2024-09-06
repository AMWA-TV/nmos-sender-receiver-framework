#include "context.h"

using namespace ossrf;

nmos_context::nmos_context(const std::string& node_id)
{
    resources_ = std::make_unique<resource_map_t>();
    nmos_api_  = nmos_impl::create(node_id);
}

nmos_t& nmos_context::nmos()
{
    return *nmos_api_;
}

resource_map_t& nmos_context::resources()
{
    return *resources_;
}
