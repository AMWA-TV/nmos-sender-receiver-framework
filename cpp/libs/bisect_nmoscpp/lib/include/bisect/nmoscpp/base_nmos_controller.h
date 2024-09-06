#pragma once

#include "bisect/nmoscpp/detail/internal.h"
#include "bisect/expected.h"
#include "bisect/sdp/settings.h"
#include "bisect/nmoscpp/detail/expected.h"
#include "bisect/nmoscpp/nmos_event_handler.h"
#include <nmos/model.h>
#include <nmos/log_gate.h>
#include <nmos/node_server.h>
#include <nmos/process_utils.h>
#include <nmos/server.h>

namespace bisect::nmoscpp
{
    class nmos_base_controller_t
    {
      public:
        nmos_base_controller_t(nmos::experimental::log_gate& gate, web::json::value configuration,
                               nmos_event_handler_t* event_handler);

        nmos::experimental::log_gate& gate_;
        nmos_event_handler_t* const event_handler_;
        nmos::node_model node_model_;
        // Must be initialized after node_model and gate
        nmos::experimental::node_implementation node_implementation_;

      private:
        [[nodiscard]] static maybe_ok init(nmos::node_model& node_model_, nmos::experimental::log_gate& gate_,
                                           nmos::experimental::node_implementation& node_implementation_,
                                           web::json::value configuration, nmos_event_handler_t* event_handler);
    };
} // namespace bisect::nmoscpp
