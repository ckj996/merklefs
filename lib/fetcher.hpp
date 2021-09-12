#ifndef INCLUDE_MERKLE_FETCHER_
#define INCLUDE_MERKLE_FETCHER_

#include <memory>
#include <string>

#include "object.grpc.pb.h"

class Fetcher {
  public:
    Fetcher(const std::string& url);

    bool fetch(const std::string& key);

  private:
    std::unique_ptr<object::Fetcher::Stub> stub_;
};

#endif