#pragma once
#include "resource_map.h"
#include "ossrf/nmos/api/nmos_impl.h"

namespace ossrf
{
    class nmos_context
    {
      public:
        nmos_context(const std::string& node_id);
        ~nmos_context()                         = default;
        nmos_context(nmos_context&)             = delete;
        nmos_context(nmos_context&&)            = delete;
        nmos_context& operator=(nmos_context&)  = delete;
        nmos_context& operator=(nmos_context&&) = delete;

        nmos_t& nmos();
        resource_map_t& resources();

      private:
        resource_map_uptr resources_;
        nmos_uptr nmos_api_;
    };

    using nmos_context_ptr  = std::shared_ptr<nmos_context>;
    using nmos_context_uptr = std::unique_ptr<nmos_context>;

} // namespace ossrf
