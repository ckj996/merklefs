#include <iostream>
#include <string>

#include "../lib/metadata.hpp"

using namespace std;

int main()
{
    auto fs = FileSystem();

    fs.creat("/foo", 0644);
    fs.mkdir("/bar", 0755);
    fs.creat("/bar/baz", 0644);

    cout << fs.lookup(1, "/foo") << endl;
    cout << fs.lookup(1, "/bar") << endl;
    cout << fs.lookup(1, "/bar/baz") << endl;

    cout << "unlink'/foo' " << fs.unlinkat(1, "/foo") << endl;
    cout << "link '/bar/baz' '/hello' " << fs.link("/bar/baz", "/hi") << endl;

    cout << fs.lookup(1, "/foo") << endl;
    cout << fs.lookup(1, "/bar") << endl;
    cout << fs.lookup(1, "/bar/baz") << endl;
    cout << fs.lookup(1, "hi") << endl;

    return 0;
}
