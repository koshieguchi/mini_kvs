#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include "exp_constants.hpp"
#include "kv_store.hpp"
#include "utils.hpp"

// Function to benchmark put operations
void benchmark_kvstore_put(int memtable_size, int put_item_size, std::ofstream& file, bool display_output) {
    KVStore kv(memtable_size, ExpConstants::BUFFER_POOL_MIN_SIZE, ExpConstants::BUFFER_POOL_MAX_SIZE);
    kv.open(ExpConstants::EXP_DB_PATH + std::to_string(put_item_size / ExpConstants::ONE_MEGA_BYTE));

    const int num_items = put_item_size / Utils::ENTRY_SIZE;

    auto start_time = ExpConstants::Clock::now();
    for (int i = 0; i < num_items; i++) {
        kv.put(i, i);
    }
    auto stop_time = ExpConstants::Clock::now();

    auto operation_duration = std::chrono::duration_cast<ExpConstants::Microseconds>(stop_time - start_time);

    // Not display output if display_output is false
    if (!display_output) {
        return;
    }

    file << "put," << (put_item_size / ExpConstants::ONE_MEGA_BYTE) << ","
         << num_items / (operation_duration.count() / (float)ExpConstants::TIME_CONVERSION_MICROSECONDS) << std::endl;
    kv.close();
}

// Function to benchmark sequential get operations
void benchmark_kvstore_sequential_get(int memtable_size, int put_item_size, std::ofstream& file) {
    KVStore kv(memtable_size, ExpConstants::BUFFER_POOL_MIN_SIZE, ExpConstants::BUFFER_POOL_MAX_SIZE);
    kv.open(ExpConstants::EXP_DB_PATH + std::to_string(put_item_size / ExpConstants::ONE_MEGA_BYTE));

    const int num_get_operations = 1024 * 8;  // Number of get operations to perform

    auto start_time = ExpConstants::Clock::now();
    for (int i = 0; i < num_get_operations; i++) {
        kv.get(num_get_operations - 1 - i);
        // kv.get(i);
    }
    auto stop_time = ExpConstants::Clock::now();

    auto operation_duration = std::chrono::duration_cast<ExpConstants::Microseconds>(stop_time - start_time);

    file << "get(LSMTree-sequential)," << (put_item_size / ExpConstants::ONE_MEGA_BYTE) << ","
         << num_get_operations / (operation_duration.count() / (float)ExpConstants::TIME_CONVERSION_MICROSECONDS)
         << std::endl;
    kv.close();
}

// Function to benchmark random get operations
void benchmark_kvstore_random_get(int memtable_size, int put_item_size, std::ofstream& file) {
    KVStore kv(memtable_size, ExpConstants::BUFFER_POOL_MIN_SIZE, ExpConstants::BUFFER_POOL_MAX_SIZE);
    kv.open(ExpConstants::EXP_DB_PATH + std::to_string(put_item_size / ExpConstants::ONE_MEGA_BYTE));

    const int num_get_operations = 1024 * 8;  // Number of get operations to perform
    const int num_items = put_item_size / Utils::ENTRY_SIZE;

    // prepare random keys
    std::mt19937 rng(ExpConstants::Clock::now().time_since_epoch().count());
    std::uniform_int_distribution<unsigned int> dist(0, num_items - 1);
    std::vector<int> random_keys(num_get_operations);
    for (int& key : random_keys) {
        key = dist(rng);
    }

    // benchmark get operations
    auto start_time = ExpConstants::Clock::now();
    for (int i = 0; i < num_get_operations; i++) {
        kv.get(random_keys[i]);
    }
    auto stop_time = ExpConstants::Clock::now();

    auto operation_duration = std::chrono::duration_cast<ExpConstants::Microseconds>(stop_time - start_time);

    file << "get(LSMTree-random)," << (put_item_size / ExpConstants::ONE_MEGA_BYTE) << ","
         << num_get_operations / (operation_duration.count() / (float)ExpConstants::TIME_CONVERSION_MICROSECONDS)
         << std::endl;
    kv.close();
}

// Function to benchmark get operations
void benchmark_kvstore_scan(int memtable_size, int put_item_size, std::ofstream& file) {
    KVStore kv(memtable_size, ExpConstants::BUFFER_POOL_MIN_SIZE, ExpConstants::BUFFER_POOL_MAX_SIZE);
    kv.open(ExpConstants::EXP_DB_PATH + std::to_string(put_item_size / ExpConstants::ONE_MEGA_BYTE));

    const int num_scan_operations =
        Utils::PAGE_SIZE / Utils::ENTRY_SIZE;  // Number of scan operations to perform (approx one page)

    auto start_time = ExpConstants::Clock::now();
    kv.scan(0, num_scan_operations - 1);
    auto stop_time = ExpConstants::Clock::now();

    auto operation_duration = std::chrono::duration_cast<ExpConstants::Microseconds>(stop_time - start_time);

    // The reason we multiply "num_scan_operations" is because scan operation is performed on a range of keys
    file << "scan," << (put_item_size / ExpConstants::ONE_MEGA_BYTE) << ","
         << num_scan_operations / (operation_duration.count() / (float)ExpConstants::TIME_CONVERSION_MICROSECONDS)
         << std::endl;
    kv.close();
}

int main() {
    // Max number of entries in memtable (1MB)
    const int memtable_size = ExpConstants::ONE_MEGA_BYTE / Utils::ENTRY_SIZE;

    // Perform benchmarks and write to files

    Utils::clear_databases("exps", "experiment_db");

    // Experiment for put operations
    std::ofstream put_file("exps/results/step3_put_results.csv");
    put_file << "op_type,data_size(MB),throughput(op/s)" << std::endl;
    for (unsigned int i = ExpConstants::ONE_MEGA_BYTE; i <= ExpConstants::ONE_GIGA_BYTE; i *= 2) {
        benchmark_kvstore_put(memtable_size, i, put_file, true);
    }
    put_file.close();

    // Experiment for sequential get operations
    std::ofstream get_sequential_file("exps/results/step3_sequential_get_results.csv");
    get_sequential_file << "op_type,data_size(MB),throughput(op/s)" << std::endl;
    for (unsigned int i = ExpConstants::ONE_MEGA_BYTE; i <= ExpConstants::ONE_GIGA_BYTE; i *= 2) {
        benchmark_kvstore_sequential_get(memtable_size, i, get_sequential_file);
    }
    get_sequential_file.close();

    // Experiment for random get operations
    std::ofstream get_random_file("exps/results/step3_random_get_results.csv");
    get_random_file << "op_type,data_size(MB),throughput(op/s)" << std::endl;
    for (unsigned int i = ExpConstants::ONE_MEGA_BYTE; i <= ExpConstants::ONE_GIGA_BYTE; i *= 2) {
        benchmark_kvstore_random_get(memtable_size, i, get_random_file);
    }
    get_random_file.close();

    // Experiment for scan operations
    std::ofstream scan_file("exps/results/step3_scan_results.csv");
    scan_file << "op_type,data_size(MB),throughput(op/s)" << std::endl;
    for (unsigned int i = ExpConstants::ONE_MEGA_BYTE; i <= ExpConstants::ONE_GIGA_BYTE; i *= 2) {
        benchmark_kvstore_scan(memtable_size, i, scan_file);
    }
    scan_file.close();

    return 0;
}
