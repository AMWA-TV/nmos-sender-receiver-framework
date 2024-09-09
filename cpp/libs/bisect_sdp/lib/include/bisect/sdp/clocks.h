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

#include "bisect/expected/macros.h"
#include "bisect/expected.h"

#include <array>
#include <string>
#include <variant>
#include <optional>

namespace bisect::sdp
{

    namespace ethernet
    {
        constexpr size_t mac_address_len = 6;
        using mac_address_t              = std::array<std::byte, mac_address_len>;

        std::string to_string(const mac_address_t& m, char separator = ':');
        expected<mac_address_t> to_mac_address(std::string_view address);

        bool operator>(const mac_address_t& lhs, const mac_address_t& rhs);
        bool operator<(const mac_address_t& lhs, const mac_address_t& rhs);
        bool operator==(const mac_address_t& lhs, const mac_address_t& rhs);
        bool operator!=(const mac_address_t& lhs, const mac_address_t& rhs);
        bool operator>=(const mac_address_t& lhs, const mac_address_t& rhs);
        bool operator<=(const mac_address_t& lhs, const mac_address_t& rhs);
    } // namespace ethernet

    /// See https://www.rfc-editor.org/rfc/rfc7273.html
    namespace refclks
    {
        /// e.g. ts-refclk:localmac=98-03-9b-8d-7e-5c
        struct localmac_t
        {
            ethernet::mac_address_t address;
        };

        /// e.g. ts-refclk:ptp=IEEE1588-2008:ec-46-70-ff-fe-10-ff-b0:127
        struct ptp_t
        {
            /// e.g. "ec-46-70-ff-fe-10-ff-b0"
            std::string gmid;
            /// e.g. 127
            std::optional<uint8_t> domain;
        };

    } // namespace refclks

    using refclk_t = std::variant<refclks::localmac_t, refclks::ptp_t>;

    std::string to_string(const refclks::ptp_t&);
    std::string to_string(const refclks::localmac_t&);
    std::string to_string(const refclk_t& refclk);

    namespace mediaclks
    {
        struct sender_t
        {
        };

        struct direct_t
        {
            uint64_t offset;
        };
    } // namespace mediaclks

    using mediaclk_t = std::variant<mediaclks::direct_t, mediaclks::sender_t>;

    std::string to_string(const mediaclks::sender_t&);
    std::string to_string(const mediaclks::direct_t&);
    std::string to_string(const mediaclk_t& mediaclk);
} // namespace bisect::sdp
