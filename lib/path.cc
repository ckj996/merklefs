#include "path.hpp"

#include <cstring>

const int SEP = '/';

std::string pathsep(const char *&path)
{
    const char *p = path;
    std::string s;

    if (path == nullptr) {
        return "";
    }
    while (*path == SEP) {
        ++path;
    }
    p = strchr(path, SEP);
    if (p == nullptr) {
        s = std::string(path);
        path = nullptr;
    } else {
        s = std::string(path, p);
        while (*++p == SEP) {
        }
        path = *p ? p : nullptr;
    }
    return s;
}