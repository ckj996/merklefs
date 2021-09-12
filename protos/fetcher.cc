/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "fetcher.hpp"

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "fetch.grpc.pb.h"

using fetch::FetchReply;
using fetch::FetchRequest;
using fetch::FetchService;
using namespace grpc;

Fetcher::Fetcher(const std::string& url) {
    const auto& channel = CreateChannel(url, InsecureChannelCredentials());
    stub_ = FetchService::NewStub(channel);
}

bool Fetcher::fetch(const std::string &key)
{
    // Data we are sending to the server.
    FetchRequest request;
    request.set_key(key);

    // Container for the data we expect from the server.
    FetchReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Fetch(&context, request, &reply);

    // Act upon its status.
    return status.ok() && reply.ok();
}