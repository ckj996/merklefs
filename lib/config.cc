#include "config.hpp"

#include <fstream>

#include "json.hpp"

using namespace std;

constexpr char DEFAULT_CONFIG[] = "/etc/merklefs/config.json";

Config::Config() : Config::Config(DEFAULT_CONFIG) {}

Config::Config(const string& path) {
    ifstream i(path);
    nlohmann::json j;
    i >> j;
    pool_ = j["pool"];
    remote_ = j["remote"];
    fetcher_ = j["fetcher"];
}

Config::~Config() {}

const string& Config::pool() { return pool_; }
const string& Config::remote() { return remote_; }
const string& Config::fetcher() { return fetcher_; }