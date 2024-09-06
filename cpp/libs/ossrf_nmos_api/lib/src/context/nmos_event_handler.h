#include "bisect/expected.h"
#include "bisect/nmoscpp/nmos_event_handler.h"
#include "context.h"

namespace ossrf
{
    class nmos_event_handler : public bisect::nmoscpp::nmos_event_handler_t
    {
      public:
        nmos_event_handler(nmos_context_ptr context_);

        [[nodiscard]] bisect::expected<std::string>
        handle_active_state_changed(const nmos::resource& resource, const nmos::resource& connection_resource,
                                    const std::string& transport_params) override;

        [[nodiscard]] bisect::maybe_ok handle_patch_request(const nmos::resource& resource,
                                                            const nmos::resource& connection_resource,
                                                            const std::string& endpoint_staged) override;

      private:
        nmos_context_ptr const context_;
    };
} // namespace ossrf
