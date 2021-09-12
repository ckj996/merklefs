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

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "fetch.grpc.pb.h"

using fetch::FetchReply;
using fetch::FetchRequest;
using fetch::FetchService;
using namespace grpc;

class Fetcher
{
  public:
    Fetcher(std::string url) {
        const auto& channel = CreateChannel(url, InsecureChannelCredentials());
        stub_ = FetchService::NewStub(channel);
    }

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    bool Fetch(const std::string &user)
    {
        // Data we are sending to the server.
        FetchRequest request;
        request.set_key(user);

        // Container for the data we expect from the server.
        FetchReply reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub_->Fetch(&context, request, &reply);

        // Act upon its status.
        if (status.ok())
        {
            return reply.ok();
        }
        else
        {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }
    }

  private:
    std::unique_ptr<FetchService::Stub> stub_;
};

int main(int argc, char **argv)
{
    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint specified by
    // the argument "--target=" which is the only expected argument.
    // We indicate that the channel isn't authenticated (use of
    // InsecureChannelCredentials()).
    std::string url = "unix:///tmp/object-fetcher.sock";
    std::string key = "hello";
    if (argc > 1)
    {
        key = argv[1];
    }
    Fetcher fetcher(url);
    bool reply = fetcher.Fetch(key);
    std::cout << "Fetcher received: " << (reply ? "OK" : "BAD") << std::endl;

    return 0;
}
