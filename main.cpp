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

class GitObject {
    public:
    std::string hash;
    
    GitObject(std::string content) {
        this->hash = hash_object(content);
    }
    
    // this class method will interact with fs to store any type of object in object store
    void save_object() {}
    
    private:
    std::string hash_object(std::string content) {
        return "avc";
    }
};

class GitBlob : public GitObject {
public:
    GitBlob(const std::string &filePath)
        : GitObject(readFile(filePath))
    {}

private:
    static std::string readFile(const std::string &filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filePath);
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
};

class GitTree: public GitObject {
    public:
    // GitTree(fs::path dir_path): GitObject("asf") {
    //     // construct a buffer which has the content of the tree and send to parent constructor
    // }
    
    GitTree(std::string content): GitObject(content) {
        save_object();
    }
};

class Entry {
public:
    int perms;
    std::string hash;
    std::string type;
    std::string name;

    Entry(int perms, const std::string &hash, const std::string &type, const std::string &name) 
        : perms(perms), hash(hash), type(type), name(name)
    {}

    std::string toString() const {
        std::ostringstream ss;
        ss << std::oct << perms << " ";   // permissions in octal
        ss << type << " ";
        ss << hash << "\t";
        ss << name;
        return ss.str();
    }
};

Entry generate_tree_entry(const fs::directory_entry &child) {
    int perms;
    std::string type;
    std::string hash;

    if (child.is_regular_file()) {
        perms = 0100644;      // typical blob file permissions
        type = "blob";

        // compute hash of file contents (placeholder here)
        std::ifstream file(child.path(), std::ios::binary);
        std::ostringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();

        // For now, use content as "hash"; in real Git, you would SHA-1 it
        hash = std::to_string(std::hash<std::string>{}(content));
    } else if (child.is_directory()) {
        perms = 040000;       // tree directory permissions in octal
        type = "tree";

        // For directories, compute tree hash recursively or placeholder
        hash = "dummy_tree_hash";  // replace with actual tree hash logic
    } else {
        throw std::runtime_error("Unsupported file type: " + child.path().string());
    }

    std::string name = child.path().filename().string();
    return Entry(perms, hash, type, name);
}


// class GitObjectStore {
    
// }

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
