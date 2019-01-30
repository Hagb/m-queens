#include "result_file.h"
#include "serialize_util.h"

#include <string>
#include <iostream>
#include <fstream>

namespace {
    const std::string filename = "result.uint64_t";
}

uint64_t result_file::load()
{
    std::ifstream file;
    file.open(filename, std::ifstream::in | std::ifstream::binary);

    if(!file || !file.is_open()) {
        std::cout << "Error reading file" << std::endl;
        return{};
    }

    constexpr size_t record_size = sizeof(uint64_t);
    uint8_t data[record_size] = {0};
    char* data_p = reinterpret_cast<char*> (data);

    file.read(data_p, record_size);
    if(!file) {
        std::cout << "Incomplete record in file" << std::endl;
        // TODO(sudden6): return an error code here
        return 0;
    }

    return serialize_util::unpack_u64(data);
}

bool result_file::save(uint64_t res)
{
    std::ofstream file;
    file.open(filename, std::ofstream::out | std::ofstream::binary);

    if(!file || !file.is_open()) {
        std::cout << "Error writing file" << std::endl;
        return false;
    }

    constexpr size_t record_size = sizeof(uint64_t);
    uint8_t record[record_size] = {0};
    char* data_p = reinterpret_cast<char*> (record);
    serialize_util::pack_u64(res, record);

    file.write(data_p, record_size);

    if(!file) {
        return false;
    }

    return true;
}