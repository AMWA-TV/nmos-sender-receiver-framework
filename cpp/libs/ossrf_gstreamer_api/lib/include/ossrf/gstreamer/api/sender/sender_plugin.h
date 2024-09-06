#pragma once

#include "bisect/expected.h"
#include <memory>

namespace ossrf::gst::plugins
{
    class gst_sender_plugin_t;
    using gst_sender_plugin_uptr = std::unique_ptr<gst_sender_plugin_t>;

    class gst_sender_plugin_t
    {
      public:
        virtual ~gst_sender_plugin_t() = default;

        virtual void stop() = 0;
    };

    bisect::expected<gst_sender_plugin_uptr> create_gst_sender_plugin(const std::string& config, int pattern) noexcept;

} // namespace ossrf::gst::plugins