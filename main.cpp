#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

class GitRepo {
  public:
  fs::path repo_path;
  fs::path git_path;
  
    GitRepo(const std::string &basePath = ".") {
        this->repo_path = fs::path(basePath);
        this->git_path = repo_path / ".aprt-git";

        if (fs::exists(git_path)) {
            std::cout << git_path << " already exists.\n";
            return;
        }

        // Create main directory
        fs::create_directories(git_path);

        // Create subdirectories
        fs::create_directories(git_path / "objects");
        fs::create_directories(git_path / "refs" / "heads");
        fs::create_directories(git_path / "refs" / "tags");

        // Create HEAD file
        {
            std::ofstream headFile(git_path / "HEAD");
            headFile << "ref: refs/heads/master\n";
        }

        std::cout << ".aprt-git repository initialized in " << git_path << std::endl;
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

// create a tree object
// create recusively tree for each dir in repo
// save tree object

int main() {
    GitRepo repo;
    // std::cout<<sha1File("./README.md")<<std::endl;
    save_blob("./README.md");
    return 0;
}
