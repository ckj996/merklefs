#include "fs.hpp"

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
    inodes_.push_back(Inode(*this, mode));
    return inodes_.back().ino();
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

Inode::Inode(FileSystem& fs, mode_t mode)
    : fs_(fs), ino_(fs.next_ino()), mode_(mode)
{
    if (is_dir()) {
        payload_ = Dirents{};
    }
}

bool Inode::is_reg()
{
    return S_ISREG(mode_);
}

bool Inode::is_dir()
{
    return S_ISDIR(mode_);
}

bool Inode::is_lnk()
{
    return S_ISLNK(mode_);
}

ino_t Inode::ino() {
    return ino_;
}

mode_t Inode::mode()
{
    return mode_;
}

size_t Inode::size() {
    return size_;
}

Dirents& Inode::dirents() {
    return get<Dirents>(payload_);
}