#ifndef INCLUDE_MERKLEFS_
#define INCLUDE_MERKLEFS_

#include <ctime>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <sys/types.h>

class Inode;

class FileSystem {
  public:
    FileSystem(ino_t root = 1);
    Inode& operator[](ino_t ino);
    ino_t lookup(ino_t parent, const char *name);
    ino_t next_ino();
    //int creat(const char* name, mode_t mode);
    //int mkdir(const char* name, mode_t mode);
    //int mknod_symlink(const char* name, const char *link);
    //int link(const char* oldpath, const char *newpath);
  private:
    ino_t mknod(mode_t mode);
    std::vector<Inode> inodes_;
    ino_t root_ino_;
    time_t mnt_ts_;
};

typedef std::unordered_map<std::string, ino_t> Dirents;

class Inode {
  public:
    Inode(FileSystem& fs, mode_t mode);
    ino_t ino();
    mode_t mode();
    size_t size();
    bool is_dir();
    bool is_lnk();
    bool is_reg();
    std::string& gethash();
    std::string& readlink();
    Dirents& dirents();

  private:
    FileSystem& fs_;
    ino_t ino_;
    mode_t mode_;
    size_t size_;
    std::variant<std::string, Dirents> payload_;
};

#endif