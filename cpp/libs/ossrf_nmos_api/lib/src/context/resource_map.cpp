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

#include "resource_map.h"
#include "bisect/nmoscpp/detail/internal.h"
#include "bisect/expected/macros.h"

using namespace ossrf;
using namespace bisect;

void resource_map_t::insert(std::string id, nmos_resource_ptr&& entry)
{
    lock_t lock(mutex_);
    auto iter = map_.find(id);
    if(iter != map_.end())
    {
        iter->second.push_back(entry);
    }
    else
    {
        map_.insert({id, {entry}});
    }
}

void resource_map_t::erase(std::string id)
{
    lock_t lock(mutex_);
    // Check if id is a device and removes it
    auto it = map_.find(id);
    if(it != map_.end())
    {
        map_.erase(it);
    }
    // Check if id is a receiver/sender and removes it
    for(auto& [device_id, entry] : map_)
    {
        entry.erase(
            std::remove_if(entry.begin(), entry.end(), [id](const nmos_resource_ptr& r) { return r->get_id() == id; }),
            entry.end());
    }
}

expected<nmos_resource_ptr> resource_map_t::find_resource(const std::string& resource_id)
{
    lock_t lock(mutex_);
    for(auto& [device_id, entry] : map_)
    {
        for(auto& r : entry)
        {
            if(r->get_id() == resource_id)
            {
                return r;
            }
        }
    }

    BST_FAIL("didn't find any resource with ID {}", resource_id);
    return {};
}

std::vector<std::string> resource_map_t::get_sender_ids() const
{
    std::vector<std::string> ids;
    lock_t lock(mutex_);
    for(const auto& [id, resources] : map_)
    {
        for(const auto& resource : resources)
        {
            if(resource->get_resource_type() == nmos::types::sender)
            {
                ids.push_back(resource->get_id());
            }
        }
    }

    return ids;
}

std::vector<std::string> resource_map_t::get_receiver_ids() const
{
    std::vector<std::string> ids;
    lock_t lock(mutex_);
    for(const auto& [id, resources] : map_)
    {
        for(const auto& resource : resources)
        {
            if(resource->get_resource_type() == nmos::types::receiver)
            {
                ids.push_back(resource->get_id());
            }
        }
    }

    return ids;
}
