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
