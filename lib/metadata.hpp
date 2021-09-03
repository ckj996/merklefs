#ifndef INCLUDE_MERKLEFS_METADATA_
#define INCLUDE_MERKLEFS_METADATA_

#include <ctime>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <sys/types.h>

#include "json.hpp"

class Inode;

class FileSystem {
  public:
    FileSystem(ino_t root = 1);
    Inode& operator[](ino_t ino);
    ino_t next_ino();
    ino_t lookup(ino_t parent, const char *name);
    int creat(const char* name, mode_t mode);
    int mkdir(const char* name, mode_t mode);
    int symlink(const char* target, const char *name);
    int link(const char* oldname, const char *name);
    int unlinkat(ino_t parent, const char *name);

  private:
    ino_t mknod(mode_t mode);
    int linkat(ino_t parent, const char *name, ino_t target);
    std::vector<Inode> inodes_;
    ino_t root_ino_;
    time_t mnt_ts_;
    friend void to_json(nlohmann::json& j, const FileSystem& fs);
    friend void from_json(const nlohmann::json& j, FileSystem& fs);
};

typedef std::unordered_map<std::string, ino_t> Dirents;

class Inode {
  public:
    Inode();
    Inode(ino_t ino, mode_t mode);
    ino_t ino() const;
    mode_t mode() const;
    size_t size() const;
    bool is_dir() const;
    bool is_lnk() const;
    bool is_reg() const;
    const std::string& gethash() const;
    const std::string& readlink() const;
    const Dirents& dirents() const;

  private:
    ino_t ino_ = 0;
    mode_t mode_ = 0;
    size_t size_ = 0;
    std::variant<std::string, Dirents> payload_;
    Dirents& dirents();
    friend FileSystem;
    friend void to_json(nlohmann::json& j, const Inode& fs);
    friend void from_json(const nlohmann::json& j, Inode& fs);
};

#endif