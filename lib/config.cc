#include "config.hpp"

#include <fstream>

#include "json.hpp"

using namespace std;

constexpr char DEFAULT_CONFIG[] = "/etc/cafs/config.json";

Config::Config() : Config::Config(DEFAULT_CONFIG) {}

Config::Config(const string& path) {
    ifstream i(path);
    nlohmann::json j;
    i >> j;
    pool_ = j["pool"];
    remote_ = j["remote"];
}

Config::~Config() {}

string& Config::pool() { return pool_; }
string& Config::remote() { return remote_; }