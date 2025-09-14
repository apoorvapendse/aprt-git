#include <core.hpp>

IndexEntry::IndexEntry(const std::string &hash, const std::string &relative_path, size_t size, uintmax_t time_stamp)
    : hash(hash), relative_path(relative_path), size(size), time_stamp(time_stamp) {}

