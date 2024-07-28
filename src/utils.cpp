#include <fcntl.h>

#include "Utils.h"

namespace Utils {

int BinarySearch(std::vector<uint64_t> keys, uint64_t key, int startIndex) {
    if (key == keys[startIndex]) {
        return startIndex;
    } else if (key == keys[keys.size() - 1]) {
        return keys.size() - 1;
    }

    // Do a binary search
    int start = startIndex;
    int end = keys.size() - 1;
    while (start <= end) {
        int mid = start + (end - start) / 2;
        if (keys[mid] == key) {
            return mid;
        } else if (key < keys[mid]) {
            end = mid - 1;
        } else {
            start = mid + 1;
        }
    }

    // At this point end < start, and key not found, return index in which
    // keys[index - 1] <= key and keys[index + 1] >= key
    return start;
}

std::string GetBinaryFromInt(uint64_t integer, int numBits) {
    std::string result = std::bitset<64>(integer).to_string();
    return result.substr(result.length() - numBits);
}

std::string GetFilenameWithExt(const std::string &fileName) { return fileName + Utils::SST_FILE_EXTENSION; }

std::vector<uint64_t> GetKeys(std::vector<uint64_t> &data) {
    std::vector<uint64_t> keys;
    for (int i = 0; i < data.size(); i += 2) {
        keys.push_back(data[i]);
    }
    return keys;
}

int OpenFile(const std::string &fileName) {
    int fd = open(fileName.c_str(), O_RDONLY);  // Remove O_DIRECT flag when debugging
    // int fd = open(fileName.c_str(), O_RDONLY | O_DIRECT); // Remove O_DIRECT flag when debugging
    if (fd == -1) {
        perror("fd");
    }
    return fd;
}

std::string EnsureDirSlash(std::string directory) {
    if (directory.substr(directory.length() - 1) == "/") {
        return directory;
    }
    return directory + "/";
}
}  // namespace Utils
