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
        std::mutex mutex_;
        std::unordered_map<std::string, std::vector<nmos_resource_ptr>> map_;
        using lock_t = std::unique_lock<std::mutex>;

      public:
        void insert(std::string, nmos_resource_ptr&&);
        void replace(std::string, nmos_resource_ptr&&);
        void erase(std::string);

        bisect::expected<nmos_resource_ptr> find_resource(const std::string& resource_id);
    };

    using resource_map_ptr  = std::shared_ptr<resource_map_t>;
    using resource_map_uptr = std::unique_ptr<resource_map_t>;
} // namespace ossrf
