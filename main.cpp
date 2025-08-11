#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

class GitRepo {
  public:
    GitRepo(const std::string &basePath = ".") {
        fs::path aprtPath = fs::path(basePath) / ".aprt-git";

        if (fs::exists(aprtPath)) {
            std::cout << aprtPath << " already exists.\n";
            return;
        }

        // Create main directory
        fs::create_directories(aprtPath);

        // Create subdirectories
        fs::create_directories(aprtPath / "objects");
        fs::create_directories(aprtPath / "refs" / "heads");
        fs::create_directories(aprtPath / "refs" / "tags");

        // Create HEAD file
        {
            std::ofstream headFile(aprtPath / "HEAD");
            headFile << "ref: refs/heads/master\n";
        }

        std::cout << ".aprt-git repository initialized in " << aprtPath << "\n";
    }
};

std::string sha1_file(const std::string& filePath) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA_CTX shaCtx;
    SHA1_Init(&shaCtx);

    std::ifstream file(filePath, std::ios::binary);
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

void save_blob(const std::string& file_path) {
    std::string hash = sha1_file(file_path);
    
    std::string dir = ".aprt-git/objects/" + hash.substr(0, 2);
        std::string filename = hash.substr(2);
    
        fs::create_directories(dir);
    
        std::string fullPath = dir + "/" + filename;
        if (fs::exists(fullPath)) {
            std::cout << "Object already exists: " << fullPath << "\n";
            return;
        }
    
        std::ifstream src(file_path, std::ios::binary);
        std::ofstream dst(fullPath, std::ios::binary);
        // TODO: add compression
        dst << src.rdbuf();
    
        std::cout << "Stored object: " << fullPath << "\n";   
    
}

int main() {
    GitRepo repo;
    // std::cout<<sha1File("./README.md")<<std::endl;
    save_blob("./README.md");
    return 0;
}
