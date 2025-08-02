#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <iostream>
#include <sys/stat.h>

#define MAX_FILES 64
#define SECTOR_SIZE 512
#define ENTRY_SIZE 64

struct FileEntry {
    char name[48];
    uint32_t offset;
    uint32_t length;
};

std::vector<FileEntry> files;
int disk_fd = -1;

int mhffs_getattr(const char* path, struct stat* stbuf, struct fuse_file_info*) {
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    for (const auto& f : files) {
        std::string name = "/" + std::string(f.name);
        if (name == path) {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = f.length;
            return 0;
        }
    }
    return -ENOENT;
}

int mhffs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                  off_t, struct fuse_file_info*, enum fuse_readdir_flags) {
    if (strcmp(path, "/") != 0) return -ENOENT;
    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);
    for (const auto& f : files) {
        filler(buf, f.name, nullptr, 0, FUSE_FILL_DIR_PLUS);
    }
    return 0;
}

int mhffs_open(const char* path, struct fuse_file_info* fi) {
    for (const auto& f : files) {
        std::string name = "/" + std::string(f.name);
        if (name == path) return 0;
    }
    return -ENOENT;
}

int mhffs_read(const char* path, char* buf, size_t size, off_t offset,
               struct fuse_file_info*) {
    for (const auto& f : files) {
        std::string name = "/" + std::string(f.name);
        if (name == path) {
            if ((size_t)offset >= f.length) return 0;
            if (offset + size > f.length)
                size = f.length - offset;
            pread(disk_fd, buf, size, f.offset + offset);
            return size;
        }
    }
    return -ENOENT;
}

static struct fuse_operations mhffs_ops = {
    .getattr = mhffs_getattr,
    .readdir = mhffs_readdir,
    .open    = mhffs_open,
    .read    = mhffs_read,
};

int load_mhffs(const char* disk_path) {
    disk_fd = open(disk_path, O_RDONLY);
    if (disk_fd < 0) {
        perror("open");
        return 1;
    }

    lseek(disk_fd, SECTOR_SIZE, SEEK_SET);
    files.clear();

    for (int i = 0; i < MAX_FILES; ++i) {
        FileEntry entry;
        ssize_t r = read(disk_fd, &entry, sizeof(FileEntry));
        if (r != sizeof(FileEntry)) break;
        if (entry.name[0] == '\0') break;
        files.push_back(entry);
    }

    std::cout << "Mounted " << files.size() << " file(s) from MHFFS\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: mhffs-mount <disk.img or /dev/sdX> <mountpoint>\n";
        return 1;
    }

    char* disk_path = argv[1];
    load_mhffs(disk_path);

    // Shift args so FUSE sees only [prog, mountpoint]
    argv[1] = argv[2];
    argc--;

    return fuse_main(argc, argv, &mhffs_ops, nullptr);
}
