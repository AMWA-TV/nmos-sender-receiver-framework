#include "bisect/expected.h"
#include <thread>
#include <functional>
#include <string>
#include <vector>
#include <gst/gst.h>

namespace bisect::gst
{
    class pipeline
    {
      public:
        static bisect::expected<pipeline> create(const char* klass) noexcept;

        pipeline() noexcept;
        ~pipeline();
        pipeline(const pipeline& other) noexcept          = delete;
        pipeline& operator=(const pipeline& rhs) noexcept = delete;
        pipeline(pipeline&& other);
        pipeline& operator=(pipeline&& rhs);

        GstElement* get() noexcept;

        void run_loop();
        bisect::maybe_ok pause();
        bisect::maybe_ok play();
        void stop();

        // After calling release, the destructor will not unref the pipeline.
        void release() noexcept;

      private:
        explicit pipeline(GstElement* bin) noexcept;

        struct impl;
        std::unique_ptr<impl> impl_;
    };
} // namespace bisect::gst