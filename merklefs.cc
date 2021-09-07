/*
  MarkleFS: markle-tree filesystem (FUSE)
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2017       Nikolaus Rath <Nikolaus@rath.org>
  Copyright (C) 2018       Valve, Inc
  Copyright (C) 2021       Kaijie Chen <chen@kaijie.org>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/** @file
 *
 * MerkleFS builds a read-only filesystem from a "metadata" file.
 * OverlayFS should be used to support writes.
 *
 * 3 types of filemodes are supported: REG, DIR, and SYMLINK.
 * The content of REG files is stored as blobs in a "pool".
 * Those blobs are names in their hash value, and were referenced
 * by the hash value in "metadata". File contents are served using
 * FUSE passthrough to be as efficient and correct as possible.
 *
 * If any blob is missing in this "pool", MerkleFS will try to
 * fetch it from "remote", and file contents can be loaded lazily.
 *
 * ## Source code ##
 * \include merklefs.cc
 */

#define FUSE_USE_VERSION 35

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// C includes
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <ftw.h>
#include <fuse_lowlevel.h>
#include <inttypes.h>
#include <string.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/xattr.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

// C++ includes
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <list>
#include "cxxopts.hpp"
#include <mutex>
#include <fstream>
#include <thread>
#include <iomanip>
#include "lib/metadata.hpp"
#include "lib/config.hpp"

using namespace std;
using nlohmann::json;

/* We are re-using pointers to our `struct sfs_inode` and `struct
   sfs_dirp` elements as inodes and file handles. This means that we
   must be able to store pointer a pointer in both a fuse_ino_t
   variable and a uint64_t variable (used for file handles). */
static_assert(sizeof(fuse_ino_t) >= sizeof(void*),
              "void* must fit into fuse_ino_t");
static_assert(sizeof(fuse_ino_t) >= sizeof(uint64_t),
              "fuse_ino_t must be at least 64 bits");


/* Forward declarations */
struct Inode;
static Inode& get_inode(fuse_ino_t ino);


// Maps files in the source directory tree to inodes
typedef std::unordered_map<fuse_ino_t, Inode> InodeMap;

struct Inode {
    int fd {-1};
    dev_t src_dev {0};
    ino_t src_ino {0};
    int generation {0};
    uint64_t nopen {0};
    uint64_t nlookup {0};
    std::mutex m;

    // Delete copy constructor and assignments. We could implement
    // move if we need it.
    Inode() = default;
    Inode(const Inode&) = delete;
    Inode(Inode&& inode) = delete;
    Inode& operator=(Inode&& inode) = delete;
    Inode& operator=(const Inode&) = delete;

    ~Inode() {
        if(fd > 0)
            close(fd);
    }
};

struct Fs {
    // Must be acquired *after* any Inode.m locks.
    std::mutex mutex;
    InodeMap inodes; // protected by mutex
    Inode root;
    metadata::FileSystem meta;
    Config cfg;
    double timeout;
    bool debug;
    std::string source;
    size_t blksize = 4096;
    dev_t src_dev;
    dev_t dev = 0;
    uid_t uid = 1000;
    gid_t gid = 1000;
    timespec mnt_time = {};
    bool nosplice;
    bool nocache;
    int getattr(fuse_ino_t ino, struct stat& stat);
    int lookup(fuse_ino_t parent, const char *name, fuse_entry_param& e);
};
static Fs fs{};


int Fs::getattr(fuse_ino_t ino, struct stat& attr)
{
    if (debug)
        cerr << "DEBUG: getattr(): ino=" << ino << endl;

    if (ino == 0) {
        return ENOENT;
    }

    const auto& inode = meta[ino];
    attr.st_dev = dev;
    attr.st_ino = ino;
    attr.st_mode = inode.mode();
    attr.st_nlink = 1;
    attr.st_uid = uid;
    attr.st_gid = gid;
    attr.st_rdev = 0;
    attr.st_size = inode.size();
    attr.st_atim = mnt_time;
    attr.st_mtim = mnt_time;
    attr.st_ctim = mnt_time;
    attr.st_blksize = blksize;
    attr.st_blocks = inode.size() / 512 + bool(inode.size() % 512);
    return 0;
}


int Fs::lookup(fuse_ino_t parent, const char *name, fuse_entry_param& e)
{
    if (debug)
        cerr << "DEBUG: lookup(): parent=" << parent
             << ", name=" << name << endl;

    memset(&e, 0, sizeof(e));
    e.attr_timeout = timeout;
    e.entry_timeout = timeout;
    e.ino = meta.lookup(parent, name);
    e.generation = 0;
    return getattr(e.ino, e.attr);
}


#define FUSE_BUF_COPY_FLAGS                      \
        (fs.nosplice ?                           \
            FUSE_BUF_NO_SPLICE :                 \
            static_cast<fuse_buf_copy_flags>(0))


static Inode& get_inode(fuse_ino_t ino) {
    if (ino == FUSE_ROOT_ID)
        return fs.root;

    Inode* inode = reinterpret_cast<Inode*>(ino);
    if(inode->fd == -1) {
        cerr << "INTERNAL ERROR: Unknown inode " << ino << endl;
        abort();
    }
    return *inode;
}


static int get_fs_fd(fuse_ino_t ino) {
    int fd = get_inode(ino).fd;
    return fd;
}


static void mfs_init(void *userdata, fuse_conn_info *conn) {
    (void)userdata;
    if (conn->capable & FUSE_CAP_EXPORT_SUPPORT)
        conn->want |= FUSE_CAP_EXPORT_SUPPORT;

    if (fs.timeout && conn->capable & FUSE_CAP_WRITEBACK_CACHE)
        conn->want |= FUSE_CAP_WRITEBACK_CACHE;

    if (conn->capable & FUSE_CAP_FLOCK_LOCKS)
        conn->want |= FUSE_CAP_FLOCK_LOCKS;

    // Use splicing if supported. Since we are using writeback caching
    // and readahead, individual requests should have a decent size so
    // that splicing between fd's is well worth it.
    if (conn->capable & FUSE_CAP_SPLICE_WRITE && !fs.nosplice)
        conn->want |= FUSE_CAP_SPLICE_WRITE;
    if (conn->capable & FUSE_CAP_SPLICE_READ && !fs.nosplice)
        conn->want |= FUSE_CAP_SPLICE_READ;
}


static void mfs_getattr(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    if (fs.debug)
        cerr << "DEBUG: " << __func__ << "(): ino=" << ino << endl;

    (void)fi;
    struct stat attr;
    auto err = fs.getattr(ino, attr);
    if (err) {
        fuse_reply_err(req, err);
        return;
    }
    fuse_reply_attr(req, &attr, fs.timeout);
}


static void mfs_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    if (fs.debug)
        cerr << "DEBUG: " << __func__ << "(): parent=" << parent
             << ", name=" << name << endl;

    fuse_entry_param e {};
    auto err = fs.lookup(parent, name, e);
    if (err == ENOENT) {
        e.attr_timeout = fs.timeout;
        e.entry_timeout = fs.timeout;
        e.ino = e.attr.st_ino = 0;
        fuse_reply_entry(req, &e);
    } else {
        fuse_reply_entry(req, &e);
    }
}


static void mfs_readlink(fuse_req_t req, fuse_ino_t ino) {
    if (fs.debug)
        cerr << "DEBUG: " << __func__ << "(): ino=" << ino << endl;

    const auto& inode = fs.meta[ino];
    fuse_reply_readlink(req, inode.readlink().c_str());
}


struct Dir {
    const metadata::Inode &inode;
    metadata::Dirents::const_iterator it;
    off_t offset;

    Dir(const metadata::Inode& inode) : inode(inode), offset(0) {
        it = inode.dirents().cbegin();
    }

    void seek(off_t off) {
        if (off < offset) {
            it = inode.dirents().cbegin();
            offset = 0;
        }
        while (offset < off && next()) {
        }
    }

    bool hasNext() {
        return it != inode.dirents().cend();
    }

    bool get(const char **namep, ino_t *inop, off_t *offp) {
        if (!hasNext()) {
            return false;
        }

        *namep = it->first.c_str();
        *inop = it->second;
        *offp = offset + 1;
        return true;
    }

    bool next() {
        if (!hasNext()) {
            return false;
        }

        ++it;
        ++offset;
        return true;
    }

};


static Dir *get_dir(fuse_file_info *fi) {
    return reinterpret_cast<Dir*>(fi->fh);
}


static void mfs_opendir(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    if (fs.debug)
        cerr << "DEBUG: " << __func__ << "(): ino=" << ino << endl;

    const auto& inode = fs.meta[ino];
    if (!inode.is_dir()) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    auto d = new (nothrow) Dir{inode};
    if (d == nullptr) {
        fuse_reply_err(req, ENOMEM);
        return;
    }

    fi->fh = reinterpret_cast<uint64_t>(d);
    if(fs.timeout) {
        fi->keep_cache = 1;
        fi->cache_readdir = 1;
    }
    fuse_reply_open(req, fi);
}


static bool is_dot_or_dotdot(const char *name) {
    return name[0] == '.' &&
           (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}


void do_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
        off_t offset, fuse_file_info *fi, bool plus)
{
    auto d = get_dir(fi);
    auto rem = size;
    char *p;
    int err = 0, count = 0;

    if (fs.debug)
        cerr << "DEBUG: readdir(): started with ino "
             << ino << " offset " << offset << endl;

    auto buf = new (nothrow) char[size];
    if (!buf) {
        fuse_reply_err(req, ENOMEM);
        return;
    }
    p = buf;
    
    if (offset != d->offset) {
        if (fs.debug)
            cerr << "DEBUG: readdir(): seeking to " << offset << endl;
        d->seek(offset);
    }

    const char *d_name;
    ino_t d_ino;
    off_t d_off;
    for (; d->get(&d_name, &d_ino, &d_off); d->next()) {
        if (is_dot_or_dotdot(d_name))
            continue;
        
        fuse_entry_param e {};
        size_t entsize;
        if (plus) {
            err = fs.lookup(ino, d_name, e);
            if (err)
                goto error;
            entsize = fuse_add_direntry_plus(req, p, rem, d_name, &e, d_off);
        } else {
            err = fs.getattr(d_ino, e.attr);
            if (err)
                goto error;
            entsize = fuse_add_direntry(req, p, rem, d_name, &e.attr, d_off);
        }
        if (entsize > rem) {
            if (fs.debug)
                cerr << "DEBUG: readdir(): buffer full, returning data. " << endl;
            break;
        }

        p += entsize;
        rem -= entsize;
        count++;
        if (fs.debug) {
            cerr << "DEBUG: readdir(): added to buffer: " << d_name
                 << ", ino " << e.attr.st_ino << ", offset " << d_off << endl;
        }
    }
    err = 0;
error:

    // If there's an error, we can only signal it if we haven't stored
    // any entries yet - otherwise we'd end up with wrong lookup
    // counts for the entries that are already in the buffer. So we
    // return what we've collected until that point.
    if (err && rem == size) {
        switch (err) {
        case ENOENT:
            cerr << "ERROR: readdir(): no such file or directory" << endl;
            break;
        case ENOTDIR:
            cerr << "ERROR: readdir(): not a directory" << endl;
            break;
        default:
            cerr << "ERROR: readdir(): error code " << err << endl;
        }
        fuse_reply_err(req, err);
    } else {
        if (fs.debug)
            cerr << "DEBUG: readdir(): returning " << count
                 << " entries, curr offset " << d->offset << endl;
        fuse_reply_buf(req, buf, size - rem);
    }
    delete[] buf;
    return;
}


static void mfs_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                        off_t offset, fuse_file_info *fi) {
    // operation logging is done in readdir to reduce code duplication
    do_readdir(req, ino, size, offset, fi, false);
}


static void mfs_readdirplus(fuse_req_t req, fuse_ino_t ino, size_t size,
                            off_t offset, fuse_file_info *fi) {
    // operation logging is done in readdir to reduce code duplication
    do_readdir(req, ino, size, offset, fi, true);
}


static void mfs_releasedir(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    if (fs.debug)
        cerr << "DEBUG: " << __func__ << "(): ino=" << ino << endl;

    auto d = get_dir(fi);
    delete d;
    fuse_reply_err(req, 0);
}


static void mfs_open(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    if (fs.debug)
        cerr << "DEBUG: " << __func__ << "(): ino=" << ino << endl;

    const auto& inode = fs.meta[ino];

    /* With writeback cache, kernel may send read requests even
       when userspace opened write-only */
    if (fs.timeout && (fi->flags & O_ACCMODE) == O_WRONLY) {
        fi->flags &= ~O_ACCMODE;
        fi->flags |= O_RDWR;
    }

    /* With writeback cache, O_APPEND is handled by the kernel.  This
       breaks atomicity (since the file may change in the underlying
       filesystem, so that the kernel's idea of the end of the file
       isn't accurate anymore). However, no process should modify the
       file in the underlying filesystem once it has been read, so
       this is not a problem. */
    if (fs.timeout && fi->flags & O_APPEND)
        fi->flags &= ~O_APPEND;

    string path = fs.cfg.pool() + "/" + inode.gethash();
    auto fd = open(path.c_str(), fi->flags & ~O_NOFOLLOW);
    if (fd == -1) {
        // TODO: lazy load object
        auto err = errno;
        fuse_reply_err(req, err);
    }
    fi->keep_cache = (fs.timeout != 0);
    fi->fh = fd;
    fuse_reply_open(req, fi);
}


static void mfs_release(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    if (fs.debug)
        cerr << "DEBUG: " << __func__ << "(): ino=" << ino << endl;

    close(fi->fh);
    fuse_reply_err(req, 0);
}


static void mfs_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                     fuse_file_info *fi) {
    if (fs.debug)
        cerr << "DEBUG: " << __func__ << "(): ino=" << ino << endl;

    fuse_bufvec buf = FUSE_BUFVEC_INIT(size);
    buf.buf[0].flags = static_cast<fuse_buf_flags>(
        FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
    buf.buf[0].fd = fi->fh;
    buf.buf[0].pos = off;

    fuse_reply_data(req, &buf, FUSE_BUF_COPY_FLAGS);
}


static void sfs_statfs(fuse_req_t req, fuse_ino_t ino) {
    struct statvfs stbuf;

    auto res = fstatvfs(get_fs_fd(ino), &stbuf);
    if (res == -1)
        fuse_reply_err(req, errno);
    else
        fuse_reply_statfs(req, &stbuf);
}


static void sfs_flock(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi,
                      int op) {
    (void) ino;
    auto res = flock(fi->fh, op);
    fuse_reply_err(req, res == -1 ? errno : 0);
}


static void assign_operations(fuse_lowlevel_ops &sfs_oper) {
    sfs_oper.init = mfs_init;
    sfs_oper.lookup = mfs_lookup;
//    sfs_oper.mkdir = sfs_mkdir;
//    sfs_oper.mknod = sfs_mknod;
//    sfs_oper.symlink = sfs_symlink;
//    sfs_oper.link = sfs_link;
//    sfs_oper.unlink = sfs_unlink;
//    sfs_oper.rmdir = sfs_rmdir;
//    sfs_oper.rename = sfs_rename;
//    sfs_oper.forget = sfs_forget;
//    sfs_oper.forget_multi = sfs_forget_multi;
    sfs_oper.getattr = mfs_getattr;
//    sfs_oper.setattr = sfs_setattr;
    sfs_oper.readlink = mfs_readlink;
    sfs_oper.opendir = mfs_opendir;
    sfs_oper.readdir = mfs_readdir;
    sfs_oper.readdirplus = mfs_readdirplus;
    sfs_oper.releasedir = mfs_releasedir;
//    sfs_oper.fsyncdir = sfs_fsyncdir;
//    sfs_oper.create = sfs_create;
    sfs_oper.open = mfs_open;
    sfs_oper.release = mfs_release;
//    sfs_oper.flush = sfs_flush;
//    sfs_oper.fsync = sfs_fsync;
    sfs_oper.read = mfs_read;
//    sfs_oper.write_buf = sfs_write_buf;
//    sfs_oper.statfs = sfs_statfs;
#ifdef HAVE_POSIX_FALLOCATE
//    sfs_oper.fallocate = sfs_fallocate;
#endif
//    sfs_oper.flock = sfs_flock;
#ifdef HAVE_SETXATTR
//    sfs_oper.setxattr = sfs_setxattr;
//    sfs_oper.getxattr = sfs_getxattr;
//    sfs_oper.listxattr = sfs_listxattr;
//    sfs_oper.removexattr = sfs_removexattr;
#endif
}

static void print_usage(char *prog_name) {
    cout << "Usage: " << prog_name << " --help\n"
         << "       " << prog_name << " [options] <metadata> <mountpoint>\n";
}

static cxxopts::ParseResult parse_wrapper(cxxopts::Options& parser, int& argc, char**& argv) {
    try {
        return parser.parse(argc, argv);
    } catch (cxxopts::option_not_exists_exception& exc) {
        std::cout << argv[0] << ": " << exc.what() << std::endl;
        print_usage(argv[0]);
        exit(2);
    }
}


static cxxopts::ParseResult parse_options(int argc, char **argv) {
    cxxopts::Options opt_parser(argv[0]);
    opt_parser.add_options()
        ("debug", "Enable filesystem debug messages")
        ("debug-fuse", "Enable libfuse debug messages")
        ("help", "Print help")
        ("nocache", "Disable all caching")
        ("nosplice", "Do not use splice(2) to transfer data")
        ("single", "Run single-threaded")
        ("o", "FUSE mount option", cxxopts::value<std::vector<std::string>>());

    // FIXME: Find a better way to limit the try clause to just
    // opt_parser.parse() (cf. https://github.com/jarro2783/cxxopts/issues/146)
    auto options = parse_wrapper(opt_parser, argc, argv);

    if (options.count("help")) {
        print_usage(argv[0]);
        // Strip everything before the option list from the
        // default help string.
        auto help = opt_parser.help();
        std::cout << std::endl << "options:"
                  << help.substr(help.find("\n\n") + 1, string::npos);
        exit(0);

    } else if (argc != 3) {
        std::cout << argv[0] << ": invalid number of arguments\n";
        print_usage(argv[0]);
        exit(2);
    }

    fs.debug = options.count("debug") != 0;
    fs.nosplice = options.count("nosplice") != 0;
    ifstream i {argv[1]};
    json j;
    i >> j;
    fs.meta = j.get<metadata::FileSystem>();

    return options;
}


static void maximize_fd_limit() {
    struct rlimit lim {};
    auto res = getrlimit(RLIMIT_NOFILE, &lim);
    if (res != 0) {
        warn("WARNING: getrlimit() failed with");
        return;
    }
    lim.rlim_cur = lim.rlim_max;
    res = setrlimit(RLIMIT_NOFILE, &lim);
    if (res != 0)
        warn("WARNING: setrlimit() failed with");
}


int main(int argc, char *argv[]) {

    // Parse command line options
    auto options {parse_options(argc, argv)};

    // We need an fd for every dentry in our the filesystem that the
    // kernel knows about. This is way more than most processes need,
    // so try to get rid of any resource softlimit.
    maximize_fd_limit();

    // Initialize filesystem root
    fs.root.fd = -1;
    fs.root.nlookup = 9999;
    fs.timeout = options.count("nocache") ? 0 : 86400.0;

    // Initialize fuse
    fuse_args args = FUSE_ARGS_INIT(0, nullptr);
    if (fuse_opt_add_arg(&args, argv[0]) ||
        fuse_opt_add_arg(&args, "-o") ||
        fuse_opt_add_arg(&args, "default_permissions,fsname=cafs") ||
        (options.count("debug-fuse") && fuse_opt_add_arg(&args, "-odebug")))
        errx(3, "ERROR: Out of memory");

    auto fuseopts = options.count("o")
	    ? options["o"].as<std::vector<std::string>>()
	    : std::vector<std::string>();

    for (auto& opt : fuseopts) {
        if (fuse_opt_add_arg(&args, "-o") ||
            fuse_opt_add_arg(&args, opt.c_str()))
            errx(3, "ERROR: Out of memory");
    }

    int ret = 0;
    fuse_lowlevel_ops sfs_oper {};
    assign_operations(sfs_oper);
    auto se = fuse_session_new(&args, &sfs_oper, sizeof(sfs_oper), &fs);
    if (se == nullptr)
        goto err_out1;

    if (fuse_set_signal_handlers(se) != 0)
        goto err_out2;

    // Don't apply umask, use modes exactly as specified
    umask(0);

    // Mount and run main loop
    struct fuse_loop_config loop_config;
    loop_config.clone_fd = 0;
    loop_config.max_idle_threads = 10;
    if (fuse_session_mount(se, argv[2]) != 0)
        goto err_out3;
    if (options.count("single"))
        ret = fuse_session_loop(se);
    else
        ret = fuse_session_loop_mt(se, &loop_config);

    fuse_session_unmount(se);

err_out3:
    fuse_remove_signal_handlers(se);
err_out2:
    fuse_session_destroy(se);
err_out1:
    fuse_opt_free_args(&args);

    return ret ? 1 : 0;
}
