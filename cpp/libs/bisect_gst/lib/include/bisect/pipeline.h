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