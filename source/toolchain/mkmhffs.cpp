#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>

#define SECTOR_SIZE 512
#define MAX_FILES 64
#define ENTRY_SIZE 64

struct FileEntry {
    char name[48];      // UTF-8 name (null-terminated)
    uint32_t offset;    // in bytes from start
    uint32_t length;    // file size
};

bool is_block_device(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISBLK(st.st_mode);
}

void write_zeroes(std::ofstream& out, size_t count) {
    static const char zero[SECTOR_SIZE] = {};
    while (count > 0) {
        size_t write_now = std::min(count, (size_t)SECTOR_SIZE);
        out.write(zero, write_now);
        count -= write_now;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: mkmhffs -f [144|200] -o [disk.img|/dev/sdX] file1 [file2...]\n";
        return 1;
    }

    int disk_size_kb = 0;
    std::string out_path;
    std::vector<std::string> files;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f") {
            if (++i >= argc) return 1;
            std::string val = argv[i];
            if (val == "144") disk_size_kb = 1440;
            else if (val == "200") disk_size_kb = 2048;
            else {
                std::cerr << "Invalid disk size\n";
                return 1;
            }
        } else if (arg == "-o") {
            if (++i >= argc) return 1;
            out_path = argv[i];
        } else {
            files.push_back(arg);
        }
    }

    if (disk_size_kb == 0 || out_path.empty() || files.empty()) {
        std::cerr << "Missing arguments.\n";
        return 1;
    }

    size_t disk_size_bytes = disk_size_kb * 1024;
    bool to_block = is_block_device(out_path);
    std::ofstream img_out;
    int fd_out = -1;

    if (to_block) {
        fd_out = open(out_path.c_str(), O_WRONLY);
        if (fd_out < 0) {
            std::cerr << "Cannot open device for writing: " << out_path << "\n";
            return 1;
        }
    } else {
        img_out.open(out_path, std::ios::binary);
        if (!img_out) {
            std::cerr << "Failed to create image file.\n";
            return 1;
        }
        write_zeroes(img_out, disk_size_bytes);
        img_out.seekp(0);
    }

    FileEntry table[MAX_FILES];
    size_t data_offset = SECTOR_SIZE * 4; // skip boot + dir sectors
    size_t file_index = 0;

    for (const auto& path : files) {
        if (file_index >= MAX_FILES) {
            std::cerr << "Too many files (max 64).\n";
            return 1;
        }

        std::ifstream in(path, std::ios::binary);
        if (!in) {
            std::cerr << "Failed to read file: " << path << "\n";
            return 1;
        }

        std::vector<char> contents((std::istreambuf_iterator<char>(in)), {});
        size_t len = contents.size();
        if (data_offset + len > disk_size_bytes) {
            std::cerr << "Disk full. File '" << path << "' won't fit.\n";
            return 1;
        }

        // Fill entry
        std::memset(&table[file_index], 0, sizeof(FileEntry));
        std::string name = std::filesystem::path(path).filename().string();
        std::strncpy(table[file_index].name, name.c_str(), sizeof(table[file_index].name) - 1);
        table[file_index].offset = data_offset;
        table[file_index].length = len;

        // Write file data
        if (to_block)
            pwrite(fd_out, contents.data(), len, data_offset);
        else {
            img_out.seekp(data_offset);
            img_out.write(contents.data(), len);
        }

        data_offset += len;
        file_index++;
    }

    // Write file table to sector 1–3
    size_t table_offset = SECTOR_SIZE;
    if (to_block)
        pwrite(fd_out, reinterpret_cast<char*>(table), file_index * sizeof(FileEntry), table_offset);
    else {
        img_out.seekp(table_offset);
        img_out.write(reinterpret_cast<char*>(table), file_index * sizeof(FileEntry));
    }

    if (to_block) close(fd_out);
    else img_out.close();

    std::cout << "Mighf floppy written to: " << out_path
              << (to_block ? " (device)" : " (image)") << "\n";

    return 0;
}
