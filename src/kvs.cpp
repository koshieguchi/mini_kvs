
#include "kvs.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <set>

namespace fs = std::filesystem;

/* Public definitions */
KVS::KVS(int memtableSize, SearchType searchType, BufferPool *bufferPool, LSMTree *lsmTree) {
    this->memtable = new Memtable(memtableSize);
    this->allSSTs = {};
    this->bufferPool = bufferPool;
    this->searchType = searchType;
    this->isLSMTree = lsmTree != nullptr;
    this->lsmTree = lsmTree;
}

KVS::~KVS() {
    delete this->memtable;
    delete this->bufferPool;
    delete this->lsmTree;
    for (auto sst : this->allSSTs) {
        delete sst;
    }
    this->allSSTs.clear();
}

bool KVS::Open(const std::string &path) {
    this->kvsPath = path;
    if (!fs::exists(this->kvsPath)) {
        bool createDirRes = fs::create_directories(this->kvsPath);
        if (!createDirRes) {
            return false;
        }
    } else {
        this->memtable->Reset();
    }

    std::multimap<std::string, SST *> levelsMap;
    for (const auto &entry : fs::directory_iterator(this->kvsPath)) {
        if (fs::is_regular_file(entry) && entry.path().extension() == Utils::SST_FILE_EXTENSION) {
            std::string fileNameStem = entry.path().stem().string();
            std::string filePath = Utils::EnsureDirSlash(this->kvsPath) + Utils::GetFilenameWithExt(fileNameStem);
            if (!this->isLSMTree) {
                SST *sstFile = new SST(filePath, fs::file_size(entry));
                this->allSSTs.push_back(sstFile);
            }
        }
    }
    return true;
}

void KVS::Close() {
    auto data = this->memtable->GetAllData();
    if (!data.empty()) {
        if (this->isLSMTree) {
            return this->lsmTree->WriteMemtableData(data, this->searchType, this->kvsPath);
        }
        std::string fileName = Utils::GetFilenameWithExt(std::to_string(this->allSSTs.size()));
        std::string filePath = Utils::EnsureDirSlash(this->kvsPath) + fileName;
        SST *sstFile = new SST(filePath, data.size() * SST::KV_PAIR_BYTE_SIZE);
        if (this->searchType == SearchType::B_TREE_SEARCH) {
            sstFile->SetupBTreeFile();
        }
        std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
        sstFile->WriteFile(file, data, this->searchType, true);
        this->allSSTs.push_back(sstFile);
    }

    // Clear out all SST file objects
    for (auto sstFile : this->allSSTs) {
        delete sstFile;
    }
    this->allSSTs.clear();
}

void KVS::Put(uint64_t key, uint64_t value) {
    // Insert this new node into the binary search tree
    if (this->memtable->Put(key, value)) {
        return;
    }

    auto data = this->memtable->GetAllData();
    if (this->isLSMTree) {
        this->lsmTree->WriteMemtableData(data, this->searchType, this->kvsPath);
    } else {
        auto fileName = this->kvsPath + "/" + Utils::GetFilenameWithExt(std::to_string(this->allSSTs.size()));
        int sstFileDataSize = data.size() * SST::KV_PAIR_BYTE_SIZE;
        SST *sstFile = new SST(fileName, sstFileDataSize);
        if (this->searchType == SearchType::B_TREE_SEARCH) {
            sstFile->SetupBTreeFile();
        }
        std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
        sstFile->WriteFile(file, data, this->searchType, true);
        this->allSSTs.push_back(sstFile);
    }
    // Reset and create a new memtable in the memory
    this->memtable->Reset();
    this->memtable->Put(key, value);
}

uint64_t KVS::Get(uint64_t key) {
    auto value = this->memtable->Get(key);
    if (value != Utils::INVALID_VALUE) {
        return value;
    }

    if (this->isLSMTree) {
        return this->lsmTree->Get(key, this->bufferPool);
    }

    // Look for the key in the sst files from the youngest one to the oldest one based on their creation time.
    auto it = this->allSSTs.rbegin();
    while (it != this->allSSTs.rend() && value == Utils::INVALID_VALUE) {
        SST *sstFile = *it;
        if (searchType == SearchType::BINARY_SEARCH) {
            value = sstFile->PerformBinarySearch(key, this->bufferPool);
        } else {
            value = sstFile->PerformBTreeSearch(key, this->bufferPool, this->isLSMTree);
        }
        ++it;
    }
    return value;
}

void KVS::Update(uint64_t key, uint64_t newValue) {
    if (this->isLSMTree) {
        KVS::Put(key, newValue);
    } else {
        std::cerr << "Update is not supported in a non-LSMTree kvs." << std::endl;
    }
}

void KVS::Delete(uint64_t key) {
    if (this->isLSMTree) {
        KVS::Put(key, Utils::DELETED_KEY_VALUE);
    } else {
        std::cerr << "Delete is not supported in a non-LSMTree kvs." << std::endl;
    }
}

void KVS::Scan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scanResult) {
    scanResult = this->memtable->Scan(key1, key2);
    if (this->isLSMTree) {
        return this->lsmTree->Scan(key1, key2, scanResult);
    }

    // Look for the key in the sst files from the youngest one to the oldest one based on their creation time.
    auto it = this->allSSTs.rbegin();
    while (it != this->allSSTs.rend()) {
        SST *sstFile = *it;
        if (searchType == SearchType::BINARY_SEARCH) {
            sstFile->PerformBinaryScan(key1, key2, scanResult);
        } else {
            sstFile->PerformBTreeScan(key1, key2, scanResult);
        }
        ++it;
    }
}

// Used in experiments.
void KVS::ResetBufferPool(int bufferPoolMinSize, int bufferPoolMaxSize, EvictionPolicyType evictionPolicyType) {
    delete this->bufferPool;
    this->bufferPool = new BufferPool(bufferPoolMinSize, bufferPoolMaxSize, evictionPolicyType);
}
