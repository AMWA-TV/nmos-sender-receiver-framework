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

#include "bisect/expected.h"
#include <memory>

namespace ossrf::gst::plugins
{
    class gst_receiver_plugin_t;
    using gst_receiver_plugin_uptr = std::unique_ptr<gst_receiver_plugin_t>;

    class gst_receiver_plugin_t
    {
      public:
        virtual ~gst_receiver_plugin_t() = default;

        virtual void stop() = 0;
    };

    bisect::expected<gst_receiver_plugin_uptr> create_gst_receiver_plugin(const std::string& config,
                                                                          const std::string& sdp) noexcept;

} // namespace ossrf::gst::plugins