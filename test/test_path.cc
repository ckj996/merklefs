#include <iostream>
#include <string>

#include "../lib/path.hpp"

using namespace std;

int main()
{
    const char *path = "/usr/bin//env";
    string s;

    while (path != nullptr) {
        s = pathsep(path);
        cout << s << endl;
    }

    return 0;
}