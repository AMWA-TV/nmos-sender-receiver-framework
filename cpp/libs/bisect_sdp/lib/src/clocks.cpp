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

#include "bisect/sdp/clocks.h"
#include "bisect/expected/match.h"
#include "bisect/expected/macros.h"
#include "bisect/expected.h"

using namespace bisect::sdp;
using namespace bisect;

std::string sdp::ethernet::to_string(const mac_address_t& m, char separator)
{
    return fmt::format("{:02x}{}{:02x}{}{:02x}{}{:02x}{}{:02x}{}{:02x}", static_cast<int>(m[0]), separator,
                       static_cast<int>(m[1]), separator, static_cast<int>(m[2]), separator, static_cast<int>(m[3]),
                       separator, static_cast<int>(m[4]), separator, static_cast<int>(m[5]));
}

expected<ethernet::mac_address_t> sdp::ethernet::to_mac_address(std::string_view address)
{
    BST_ENFORCE(address.size() == 17, "invalid MAC address size for: {}", address);

    ethernet::mac_address_t mac;
    for(size_t i = 0; i < sizeof(ethernet::mac_address_t); ++i)
    {
        auto s       = std::string(address.data() + i * 3, address.data() + i * 3 + 2);
        size_t count = 0;

        try
        {
            std::byte x = static_cast<std::byte>(std::stoi(s, &count, 16));

            BST_ENFORCE(count == 2, "error parsing mac address: {}", address);
            mac[i] = x;
        }
        catch(std::exception& ex)
        {
            BST_FAIL("error parsing mac address '{}': {}", address, ex.what());
        }
    }

    return mac;
}

bool sdp::ethernet::operator<(const mac_address_t& lhs, const mac_address_t& rhs)
{
    uint64_t nlhs = 0, nrhs = 0, base = 1;

    for(size_t i = 0; i < mac_address_len; i++)
    {
        nlhs += std::to_integer<uint64_t>(lhs[i]) * base;
        nrhs += std::to_integer<uint64_t>(rhs[i]) * base;
        base *= 10;
    }

    return nlhs < nrhs;
}

bool sdp::ethernet::operator>(const mac_address_t& lhs, const mac_address_t& rhs)
{
    uint64_t nlhs = 0, nrhs = 0, base = 1;

    for(size_t i = 0; i < mac_address_len; i++)
    {
        nlhs += std::to_integer<uint64_t>(lhs[i]) * base;
        nrhs += std::to_integer<uint64_t>(rhs[i]) * base;
        base *= 10;
    }

    return nlhs > nrhs;
}

bool sdp::ethernet::operator==(const mac_address_t& lhs, const mac_address_t& rhs)
{
    const bool result = lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2] && lhs[3] == rhs[3] &&
                        lhs[4] == rhs[4] && lhs[5] == rhs[5];
    return result;
}

bool sdp::ethernet::operator!=(const mac_address_t& lhs, const mac_address_t& rhs)
{
    const bool r = lhs[0] != rhs[0] && lhs[1] != rhs[1] && lhs[2] != rhs[2] && lhs[3] != rhs[3] && lhs[4] != rhs[4] &&
                   lhs[5] != rhs[5];
    return r;
}

bool sdp::ethernet::operator<=(const mac_address_t& lhs, const mac_address_t& rhs)
{
    const bool r = lhs < rhs || lhs == rhs;
    return r;
}

bool sdp::ethernet::operator>=(const mac_address_t& lhs, const mac_address_t& rhs)
{
    const bool r = lhs > rhs || lhs == rhs;
    return r;
}

std::string sdp::to_string(const refclks::localmac_t& v)
{
    return fmt::format("localmac={}", ethernet::to_string(v.address, '-'));
}

std::string sdp::to_string(const refclks::ptp_t& v)
{
    const auto domain = v.domain.has_value() ? fmt::format(":{}", v.domain.value()) : std::string{};
    return fmt::format("ptp=IEEE1588-2008:{}{}", v.gmid, domain);
}

std::string sdp::to_string(const refclk_t& v)
{
    return match(v, overload([&](const auto& r) { return to_string(r); }));
}

std::string sdp::to_string(const mediaclks::sender_t&)
{
    return "sender";
}

std::string sdp::to_string(const mediaclks::direct_t& mediaclk)
{
    return fmt::format("direct={}", mediaclk.offset);
}

std::string sdp::to_string(const mediaclk_t& v)
{
    return match(v, overload([&](const auto& r) { return to_string(r); }));
}
