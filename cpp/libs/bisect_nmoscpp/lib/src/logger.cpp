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

#include "bisect/nmoscpp/logger.h"

using namespace bisect::nmoscpp;

logger_t::logger_t() : error_(std::cerr.rdbuf()), access_(&access_buf_), gate_(error_, access_, model_)
{
    // nmos::insert_node_default_settings(node_model.settings);

    // // copy to the logging settings
    // // hmm, this is a bit icky, but simplest for now
    // log_model.settings = node_model.settings;

    // // the logging level is a special case because we want to turn it into an atomic value
    // // that can be read by logging statements without locking the mutex protecting the settings
    // log_model.level = nmos::fields::logging_level(log_model.settings);

    // // Reconfigure the logging streams according to settings
    // // (obviously, until this point, the logging gateway has its default behaviour...)

    // if(!nmos::fields::error_log(node_model.settings).empty())
    // {
    //     error_log_buf.open(nmos::fields::error_log(node_model.settings), std::ios_base::out | std::ios_base::app);
    //     auto lock = log_model.write_lock();
    //     error_log.rdbuf(&error_log_buf);
    // }

    // if(!nmos::fields::access_log(node_model.settings).empty())
    // {
    //     access_log_buf.open(nmos::fields::access_log(node_model.settings), std::ios_base::out | std::ios_base::app);
    //     auto lock = log_model.write_lock();
    //     access_log.rdbuf(&access_log_buf);
    // }
}

nmos::experimental::log_gate& logger_t::gate()
{
    return gate_;
}

nmos::experimental::log_model& logger_t::model()
{
    return model_;
}
