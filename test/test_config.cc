#include <iostream>
#include <string>

#include "../lib/config.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    Config cfg;

    if (argc == 2) {
        string path = argv[1];
        cfg = Config(path);
    } else {
        cfg = Config();
    }
    
    cout << "pool: " << cfg.pool() << endl
        << "remote: " << cfg.remote() << endl
        << "fetcher: " << cfg.fetcher() << endl;

    return 0;
}
