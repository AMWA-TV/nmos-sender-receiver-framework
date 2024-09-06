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