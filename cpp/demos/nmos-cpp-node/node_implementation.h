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

#ifndef NMOS_CPP_NODE_NODE_IMPLEMENTATION_H
#define NMOS_CPP_NODE_NODE_IMPLEMENTATION_H

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;

    namespace experimental
    {
        struct node_implementation;
    }
} // namespace nmos

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// starts background tasks to emit regular events from the temperature event source, and then waits for shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate);

// This constructs all the callbacks used to integrate the example device-specific underlying implementation
// into the server instance for the NMOS Node.
nmos::experimental::node_implementation make_node_implementation(nmos::node_model& model, slog::base_gate& gate);

#endif
