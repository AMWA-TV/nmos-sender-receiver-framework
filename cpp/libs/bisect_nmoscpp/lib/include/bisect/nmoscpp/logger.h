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
