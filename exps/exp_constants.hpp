#ifndef EXP_CONSTANTS_H
#define EXP_CONSTANTS_H

#include <chrono>
#include <cmath>
#include <iostream>

namespace ExpConstants {
using Clock = std::chrono::high_resolution_clock;
using Microseconds = std::chrono::microseconds;

const std::string EXP_DB_PATH = "exps/experiment_db_";
const unsigned int TIME_CONVERSION_MICROSECONDS = 1000000;  // Conversion factor to microseconds
const unsigned int ONE_MEGA_BYTE = 1048576;                 // 1024 * 1024, 1 MB
const unsigned int ONE_GIGA_BYTE = 1073741824;              // 1024 * 1024 * 1024, 1 GB

// === Buffer Pool Size ===
// Maximum number of page in each bucket: 1
// Maximum number of buckets: 2^(buffer_pool_max_size)
// Page size: 4096 bytes
// Buffer Pool size: 2^(buffer_pool_max_size) * 4096 bytes
// ex) buffer_pool_max_size = 14, buffer_pool_size = 2^14 * 4096 bytes = 64 MB
inline unsigned int BUFFER_POOL_MIN_SIZE = pow(2, 1);
// buffer_pool_size = 2^11 * 4096 bytes + 2^9 * 4096 bytes = 8 MB + 2MB = 10 MB
inline unsigned int BUFFER_POOL_MAX_SIZE = pow(2, 11) + pow(2, 9);

}  // namespace ExpConstants

#endif  // EXP_CONSTANTS_H
