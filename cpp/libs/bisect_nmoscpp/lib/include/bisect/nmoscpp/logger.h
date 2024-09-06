#pragma once

#include "bisect/nmoscpp/detail/internal.h"
#include <nmos/log_gate.h>
#include <nmos/model.h>

namespace bisect::nmoscpp
{
    class logger_t
    {
      public:
        logger_t();

        nmos::experimental::log_gate& gate();
        nmos::experimental::log_model& model();

      private:
        nmos::experimental::log_model model_;
        std::ostream error_;
        std::ostream access_;
        // Before initialize gate, log_model and ostream error/access need to be initialized
        nmos::experimental::log_gate gate_;
        std::filebuf error_buf;
        std::filebuf access_buf_;
    };
} // namespace bisect::nmoscpp
