#include <iostream>
#include <fstream>
#include <string>

#include "../lib/metadata.hpp"

using namespace std;
using namespace metadata;
using nlohmann::json;

void test_creation()
{
    auto fs = FileSystem();

    fs.creat("/foo", 0644);
    fs.mkdir("/bar", 0755);
    fs.creat("/bar/baz", 0644);

    cout << fs.lookup(1, "/foo") << endl;
    cout << fs.lookup(1, "/bar") << endl;
    cout << fs.lookup(1, "/bar/baz") << endl;
    cout << fs.lookup(1, "hi") << endl;

    cout << "unlink'/foo' " << fs.unlinkat(1, "/foo") << endl;
    cout << "link '/bar/baz' '/hello' " << fs.link("/bar/baz", "/hi") << endl;

    cout << fs.lookup(1, "/foo") << endl;
    cout << fs.lookup(1, "/bar") << endl;
    cout << fs.lookup(1, "/bar/baz") << endl;
    cout << fs.lookup(1, "hi") << endl;

    json j = fs;
    cout << j << endl;

    auto tmp = j.get<FileSystem>();
    cout << tmp.lookup(1, "/foo") << endl;
    cout << tmp.lookup(1, "/bar") << endl;
    cout << tmp.lookup(1, "/bar/baz") << endl;
    cout << tmp.lookup(1, "hi") << endl;

}

void test_load(const char *metadata)
{
    ifstream i {metadata};
    json j;

    i >> j;
    auto fs = j.get<FileSystem>();

    auto usr = fs.lookup(1, "usr");
    auto bin = fs.lookup(usr, "bin");
    auto env = fs.lookup(bin, "env");
    cout << "/usr/bin/env = /" << usr << "/" << bin << "/" << env << endl;

    cout << "listing /" << endl;
    const auto& root = fs[1];
    for (auto it = root.dirents().cbegin(); it != root.dirents().cend(); ++it)
        cout << it->first << ":" << it->second << endl;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        test_creation();
    } else {
        test_load(argv[1]);
    }
    return 0;
}