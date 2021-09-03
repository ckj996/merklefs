#include "metadata.hpp"

#include <ctime>
#include <cassert>

#include <sys/stat.h>

#include "path.hpp"

using namespace std;

FileSystem::FileSystem(ino_t root) : root_ino_(root), mnt_ts_(time(nullptr)) {
    mknod(S_IFDIR | 0755); // create root directory
};

int FileSystem::creat(const char *name, mode_t mode) {
    ino_t ino = mknod(S_IFREG | mode);
    return linkat(root_ino_, name, ino);
}

int FileSystem::mkdir(const char *name, mode_t mode) {
    ino_t ino = mknod(S_IFDIR | mode);
    return linkat(root_ino_, name, ino);
}

int FileSystem::symlink(const char *target, const char *name) {
    (void) target;
    ino_t ino = mknod(S_IFLNK | ACCESSPERMS);
    return linkat(root_ino_, name, ino);
}

int FileSystem::link(const char *oldname, const char *name) {
    ino_t ino = lookup(root_ino_, oldname);
    if (ino == 0) {
        return -ENONET;
    }
    int err = linkat(root_ino_, name, ino);
    if (err != 0) {
        return err;
    }
    return unlinkat(root_ino_, oldname);
}

ino_t FileSystem::mknod(mode_t mode) {
    ino_t ino = next_ino();
    inodes_.push_back(Inode{ino, mode});
    return ino;
}

ino_t FileSystem::next_ino() { return inodes_.size() + root_ino_; }

Inode& FileSystem::operator[](ino_t ino)
{
    ino_t i = ino - root_ino_;
    assert(ino >= root_ino_ && i < inodes_.size());
    return inodes_[i];
}

ino_t FileSystem::lookup(ino_t parent, const char *name)
{
    while (name != nullptr && *name) {
        Inode& dir = (*this)[parent];
        if (!dir.is_dir()) {
            return 0;
        }
        auto step = pathsep(name);
        parent = dir.dirents()[step];
    }
    return parent;
}

int FileSystem::linkat(ino_t parent, const char *name, ino_t target)
{
    while (name != nullptr && *name) {
        if (parent == 0) {
            return -ENOENT;
        }
        Inode& dir = (*this)[parent];
        if (!dir.is_dir()) {
            return -ENOTDIR;
        }
        auto step = pathsep(name);
        if (name == nullptr) {
            dir.dirents()[step] = target;
        } else {
            parent = dir.dirents()[step];
        }
    }
    return 0;
}

int FileSystem::unlinkat(ino_t parent, const char *name)
{
    while (name != nullptr && *name) {
        if (parent == 0) {
            return -ENOENT;
        }
        Inode& dir = (*this)[parent];
        if (!dir.is_dir()) {
            return -ENOTDIR;
        }
        auto step = pathsep(name);
        if (name == nullptr) {
            if (dir.dirents().erase(step) == 0) {
                return -ENOENT;
            }
        } else {
            parent = dir.dirents()[step];
        }
    }
    return 0;
}

Inode::Inode() {}

Inode::Inode(ino_t ino, mode_t mode) : ino_(ino), mode_(mode)
{
    if (is_dir()) {
        payload_ = Dirents{};
    }
}

bool Inode::is_reg() const
{
    return S_ISREG(mode_);
}

bool Inode::is_dir() const
{
    return S_ISDIR(mode_);
}

bool Inode::is_lnk() const
{
    return S_ISLNK(mode_);
}

ino_t Inode::ino() const
{
    return ino_;
}

mode_t Inode::mode() const
{
    return mode_;
}

size_t Inode::size() const
{
    return size_;
}

Dirents& Inode::dirents()
{
    return get<Dirents>(payload_);
}

const Dirents& Inode::dirents() const
{
    return get<Dirents>(payload_);
}

const string& Inode::readlink() const
{
    return get<string>(payload_);
}

const string& Inode::gethash() const
{
    return get<string>(payload_);
}

using nlohmann::json;

void to_json(json& j, const FileSystem& fs)
{
    j = fs.inodes_;
}

void from_json(const json& j, FileSystem& fs)
{
    j.get_to(fs.inodes_);
    fs.root_ino_ = fs.inodes_[0].ino();
}

void to_json(json& j, const Inode& i)
{
    j = json{{"ino", i.ino_}, {"mode", i.mode_}, {"size", i.size_}};
    if (i.is_dir()) {
        j["dirents"] = i.dirents();
    } else if (i.is_reg()) {
        j["value"] = i.gethash();
    } else if (i.is_lnk()) {
        j["value"] = i.readlink();
    }
}

void from_json(const json& j, Inode& i)
{
    j.at("ino").get_to(i.ino_);
    j.at("mode").get_to(i.mode_);
    j.at("size").get_to(i.size_);
    if (i.is_dir()) {
        Dirents dirents;
        j.at("dirents").get_to(dirents);
        i.payload_ = dirents;
    } else {
        string value;
        j.at("value").get_to(value);
        i.payload_ = value;
    }
}