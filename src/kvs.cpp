
#include "kvs.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <set>

namespace fs = std::filesystem;

/* Public definitions */
KVS::KVS(int memtable_size, SearchType search_type, BufferPool *buffer_pool, LSMTree *lsm_tree) {
    this->memtable = new Memtable(memtable_size);
    this->all_ssts = {};
    this->buffer_pool = buffer_pool;
    this->search_type = search_type;
    this->is_lsm_tree = lsm_tree != nullptr;
    this->lsm_tree = lsm_tree;
}

KVS::~KVS() {
    delete this->memtable;
    delete this->buffer_pool;
    delete this->lsm_tree;
    for (auto sst : this->all_ssts) {
        delete sst;
    }
    this->all_ssts.clear();
}

bool KVS::Open(const std::string &path) {
    this->kvs_path = path;
    if (!fs::exists(this->kvs_path)) {
        bool create_dir_res = fs::create_directories(this->kvs_path);
        if (!create_dir_res) {
            return false;
        }
    } else {
        this->memtable->Reset();
    }

    std::multimap<std::string, SST *> levelsMap;
    for (const auto &entry : fs::directory_iterator(this->kvs_path)) {
        if (fs::is_regular_file(entry) && entry.path().extension() == Utils::SST_FILE_EXTENSION) {
            std::string file_nameStem = entry.path().stem().string();
            std::string file_path = Utils::EnsureDirSlash(this->kvs_path) + Utils::GetFilenameWithExt(file_nameStem);
            if (!this->is_lsm_tree) {
                SST *sst_file = new SST(file_path, fs::file_size(entry));
                this->all_ssts.push_back(sst_file);
            }
        }
    }
    return true;
}

void KVS::Close() {
    auto data = this->memtable->GetAllData();
    if (!data.empty()) {
        if (this->is_lsm_tree) {
            return this->lsm_tree->WriteMemtableData(data, this->search_type, this->kvs_path);
        }
        std::string file_name = Utils::GetFilenameWithExt(std::to_string(this->all_ssts.size()));
        std::string file_path = Utils::EnsureDirSlash(this->kvs_path) + file_name;
        SST *sst_file = new SST(file_path, data.size() * SST::KV_PAIR_BYTE_SIZE);
        if (this->search_type == SearchType::B_TREE_SEARCH) {
            sst_file->SetupBTreeFile();
        }
        std::ofstream file(sst_file->GetFileName(), std::ios::out | std::ios::binary);
        sst_file->WriteFile(file, data, this->search_type, true);
        this->all_ssts.push_back(sst_file);
    }

    // Clear out all SST file objects
    for (auto sst_file : this->all_ssts) {
        delete sst_file;
    }
    this->all_ssts.clear();
}

void KVS::Put(uint64_t key, uint64_t value) {
    // Insert this new node into the binary search tree
    if (this->memtable->Put(key, value)) {
        return;
    }

    auto data = this->memtable->GetAllData();
    if (this->is_lsm_tree) {
        this->lsm_tree->WriteMemtableData(data, this->search_type, this->kvs_path);
    } else {
        auto file_name = this->kvs_path + "/" + Utils::GetFilenameWithExt(std::to_string(this->all_ssts.size()));
        int sst_file_data_size = data.size() * SST::KV_PAIR_BYTE_SIZE;
        SST *sst_file = new SST(file_name, sst_file_data_size);
        if (this->search_type == SearchType::B_TREE_SEARCH) {
            sst_file->SetupBTreeFile();
        }
        std::ofstream file(sst_file->GetFileName(), std::ios::out | std::ios::binary);
        sst_file->WriteFile(file, data, this->search_type, true);
        this->all_ssts.push_back(sst_file);
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

    if (this->is_lsm_tree) {
        return this->lsm_tree->Get(key, this->buffer_pool);
    }

    // Look for the key in the sst files from the youngest one to the oldest one based on their creation time.
    auto it = this->all_ssts.rbegin();
    while (it != this->all_ssts.rend() && value == Utils::INVALID_VALUE) {
        SST *sst_file = *it;
        if (search_type == SearchType::BINARY_SEARCH) {
            value = sst_file->PerformBinarySearch(key, this->buffer_pool);
        } else {
            value = sst_file->PerformBTreeSearch(key, this->buffer_pool, this->is_lsm_tree);
        }
        ++it;
    }
    return value;
}

void KVS::Update(uint64_t key, uint64_t new_value) {
    if (this->is_lsm_tree) {
        KVS::Put(key, new_value);
    } else {
        std::cerr << "Update is not supported in a non-LSMTree kvs." << std::endl;
    }
}

void KVS::Delete(uint64_t key) {
    if (this->is_lsm_tree) {
        KVS::Put(key, Utils::DELETED_KEY_VALUE);
    } else {
        std::cerr << "Delete is not supported in a non-LSMTree kvs." << std::endl;
    }
}

void KVS::Scan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scan_result) {
    scan_result = this->memtable->Scan(key1, key2);
    if (this->is_lsm_tree) {
        return this->lsm_tree->Scan(key1, key2, scan_result);
    }

    // Look for the key in the sst files from the youngest one to the oldest one based on their creation time.
    auto it = this->all_ssts.rbegin();
    while (it != this->all_ssts.rend()) {
        SST *sst_file = *it;
        if (search_type == SearchType::BINARY_SEARCH) {
            sst_file->PerformBinaryScan(key1, key2, scan_result);
        } else {
            sst_file->PerformBTreeScan(key1, key2, scan_result);
        }
        ++it;
    }
}

// Used in experiments.
void KVS::ResetBufferPool(int buffer_pool_min_size, int buffer_pool_max_size, EvictionPolicyType eviction_policy_type) {
    delete this->buffer_pool;
    this->buffer_pool = new BufferPool(buffer_pool_min_size, buffer_pool_max_size, eviction_policy_type);
}
