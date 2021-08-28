#ifndef INCLUDE_MERKLEFS_CONFIG_
#define INCLUDE_MERKLEFS_CONFIG_

#include <string>

class Config {
  public:
    Config();
    Config(const std::string& file);
    ~Config();
    std::string& pool();
    std::string& remote();

  private:
    std::string pool_;
    std::string remote_;
};

#endif