#include "core.hpp"

std::string sha1_file(const std::string &abs_file_path) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA_CTX shaCtx;
    SHA1_Init(&shaCtx);

    std::ifstream file(abs_file_path, std::ios::binary);
    std::vector<char> buffer(8192);
    while (file.good()) {
        file.read(buffer.data(), buffer.size());
        SHA1_Update(&shaCtx, buffer.data(), file.gcount());
    }

    SHA1_Final(hash, &shaCtx);

    std::ostringstream oss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

std::string get_hash_from_content(std::string content) {
    unsigned char hash[SHA_DIGEST_LENGTH]; // SHA1 produces 20 bytes
    SHA1(reinterpret_cast<const unsigned char *>(content.c_str()), content.size(), hash);

    std::ostringstream oss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return oss.str();
}
