#pragma once

#include "nmos_resource.h"
#include "bisect/nmoscpp/configuration.h"
#include <functional>
#include <optional>

namespace ossrf
{
    class nmos_resource_sender_t : public nmos_resource_t
    {
      public:
        nmos_resource_sender_t(const std::string& device_id, const bisect::nmoscpp::nmos_sender_t& config,
                               bisect::nmoscpp::sender_activation_callback_t callback);

        bisect::maybe_ok handle_activation(bool master_enable, nlohmann::json& transport_params) override;
        bisect::maybe_ok handle_patch(bool master_enable, const nlohmann::json& configuration) override;

        const std::string& get_id() const override;

        const std::string& get_device_id() const override;

      private:
        const bisect::nmoscpp::nmos_sender_t config_;
        bisect::nmoscpp::sender_activation_callback_t activation_callback_;
        bool master_enable_ = true;
        std::optional<std::string> sdp_;
        std::string device_id_;
    };

    using nmos_sender_ptr = std::shared_ptr<nmos_resource_sender_t>;
} // namespace ossrf
