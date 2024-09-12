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
#include "resources/nmos_resource.h"
#include <nmos/id.h>
#include <unordered_map>
#include <mutex>

namespace ossrf
{
    class resource_map_t
    {
        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::vector<nmos_resource_ptr>> map_;
        using lock_t = std::unique_lock<std::mutex>;

      public:
        void insert(std::string, nmos_resource_ptr&&);
        void replace(std::string, nmos_resource_ptr&&);
        void erase(std::string);

        bisect::expected<nmos_resource_ptr> find_resource(const std::string& resource_id);
        std::vector<std::string> get_sender_ids() const;
        std::vector<std::string> get_receiver_ids() const;
    };
    using resource_map_ptr  = std::shared_ptr<resource_map_t>;
    using resource_map_uptr = std::unique_ptr<resource_map_t>;
} // namespace ossrf
