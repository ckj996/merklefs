#ifndef INCLUDE_MERKLEFS_CONFIG_
#define INCLUDE_MERKLEFS_CONFIG_

#include <string>

class Config {
  public:
    Config();
    Config(const std::string& file);
    ~Config();
    const std::string& pool();
    const std::string& remote();
    const std::string& fetcher();

  private:
    std::string pool_;
    std::string remote_;
    std::string fetcher_;
};

#endif