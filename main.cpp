#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

class Entry {
    /*
        // Tree entry for a given tree
        $ git cat-file bfeea50af3c3c263cb48d8e6356cb737adc61a8e -p
        100644 blob e7592a092e69d55f97ffa05eff9114e66e351d77    README.txt
        100644 blob e7592a092e69d55f97ffa05eff9114e66e351d77    copy.txt
        040000 tree 9e377827cb1253aadc6efba776fc29dd329a704f    test
    */
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

// helper to generate entry content for a given child.
Entry generate_tree_entry(const fs::directory_entry &child, std::string entry_hash) {
    int perms;
    std::string type;
    std::string hash;

    if (child.is_regular_file()) {
        perms = 0100644;      // typical blob file permissions
        type = "blob";
        hash = entry_hash;
    } else if (child.is_directory()) {
        perms = 040000;       // tree directory permissions in octal
        type = "tree";
        // For directories, compute tree hash recursively or placeholder
        hash = entry_hash;  // replace with actual tree hash logic
    } else {
        throw std::runtime_error("Unsupported file type: " + child.path().string());
    }

    std::string name = child.path().filename().string();
    return Entry(perms, hash, type, name);
}
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
            headFile << "";
        }

        std::cout << ".aprt-git repository initialized in " << git_path << std::endl;
    }

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
    
        std::string dir = git_path / "objects" / hash.substr(0, 2);
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

    void save_hash_from_content(std::string content) {
        std::string hash = get_hash_from_content(content);
        std::string dir = git_path / "objects" / hash.substr(0, 2);
        std::string filename = hash.substr(2);
    
        fs::create_directories(dir);
    
        std::string fullPath = dir + "/" + filename;
        if (fs::exists(fullPath)) {
            std::cout << "Object already exists: " << fullPath << "\n";
            return;
        }
    
        std::ofstream dst(fullPath, std::ios::binary);
        // TODO: add compression
        dst << content;
    
        std::cout << "Stored object: " << fullPath << "\n";   
    }

    std::string hash_from_root(fs::path path = {}){
        if(path.empty()) {
            path = this->repo_path;
        }
        std::cout << path << std::endl;
        std::cout << repo_path/ ".aprt-git" << std::endl;
        if(path.is_absolute() && path  == repo_path / ".aprt-git"){
            return "nothing changed";
        }
        // TODO: 
        // 1. Don't hash if not staged, add check for this
        // 2. 
        std::vector<Entry> entries;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                std::string dir_hash = hash_from_root(entry.path());
                if(dir_hash == "nothing changed"){
                    continue;
                }
                Entry dir_entry = generate_tree_entry(entry, dir_hash);
                entries.push_back(dir_entry);
            } else if (entry.is_regular_file()) {
                std::ifstream file(entry.path(), std::ios::binary);
                std::ostringstream ss;
                ss << file.rdbuf();
                std::string content = ss.str();

                std::string file_hash = get_hash_from_content(content);
                if(check_hash_exists_already(file_hash)){
                    continue;
                }
                std::cout << "Saving blob:" << entry.path() << std::endl;
                save_blob(entry.path());
                Entry file_entry = generate_tree_entry(entry, file_hash);
                entries.push_back(file_entry);
            }
        }
        // TODO:
        // Hash this entry file and save it if not present
        // if hash_from_root(root) returns a hash that already exists, `nothing to commit, working tree clean`
        std::string final_tree_object_content = "";
        if(entries.size() == 0)return "nothing changed";
        for(const auto& e : entries){
            if(e.type == "blob"){
               std::string blob_entry = "";
               blob_entry += std::to_string(e.perms) + " ";  
               blob_entry += e.type + " ";
               blob_entry += e.hash + " ";
               blob_entry += e.name + "\n";
               final_tree_object_content += blob_entry;
            }else{
               std::string tree_entry = "";
               tree_entry += std::to_string(e.perms) + " ";  
               tree_entry += e.type + " ";
               tree_entry += e.hash + " ";
               tree_entry += e.name + "\n";
               final_tree_object_content += tree_entry;
            }
        }
        // if this hash already exists, return something that denotes nothing changed here.
        if(check_hash_exists_already(get_hash_from_content(final_tree_object_content))){
            return "nothing changed";
        }
        
        save_hash_from_content(final_tree_object_content);
        return get_hash_from_content(final_tree_object_content);
    }

    bool check_hash_exists_already(std::string hash){
        std::string object_dir = hash.substr(0,2);
        std::string object_file_name = hash.substr(2);
        fs::path object_path = git_path / "objects" / object_dir / object_file_name;
        return fs::exists(object_path);
    }

    std::string get_hash_from_content(std::string content){
        unsigned char hash[SHA_DIGEST_LENGTH]; // SHA1 produces 20 bytes
        SHA1(reinterpret_cast<const unsigned char*>(content.c_str()), content.size(), hash);

        std::ostringstream oss;
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }

        return oss.str();       
    }

    /*
        tree d906209d7ed2121a398d675ff840a05d12c5661f
        parent 85a48f686c2b9a841e01d5caf219bb726b423719
        author apoorvapendse <apoorvavpendse@gmail.com> 1756058996 +0530
        committer apoorvapendse <apoorvavpendse@gmail.com> 1756058996 +0530

        Store tree/blob objects on disk

        Signed-off-by: apoorvapendse <apoorvavpendse@gmail.com>
    */
    void commit(std::string author, std::string committer, std::string commit_message){
        std::string root_tree_hash = hash_from_root();
        if(root_tree_hash == "nothing changed"){
            std::cout << "nothing to commit, working tree clean.";
            return;
        }

        std::string commit_content = "";
        std::string parent_hash = read_hash_from_head();
        commit_content += "tree " + root_tree_hash + "\n";
        if(parent_hash.size() != 0) {
            commit_content += "parent " + parent_hash + "\n";
        }
        commit_content += "author " + author + "\n";
        commit_content += "committer " + committer + "\n\n";
        commit_content += commit_message;

        save_hash_from_content(commit_content);
        std::string commit_hash = get_hash_from_content(commit_content);
        // TODO: Save this commit_hash in HEAD. Think about interactive rebase 🤯
        write_commit_hash_to_head_file(commit_hash);
    }
    void write_commit_hash_to_head_file(std::string hash) {
        // TODO: Store ref when it is implemented instead of storing hash directly in HEAD
        std::ofstream head_file(git_path / "HEAD");
        if (!head_file.is_open()) {
            throw std::runtime_error("could not open HEAD file");
        }
        head_file << hash;
    }

    std::string read_hash_from_head(){
        std::ifstream head_file(git_path / "HEAD");
        if (!head_file.is_open()) {
            std::cerr << "Failed to open file!" << std::endl;
            return "";
        }
        std::string hash;
        std::getline(head_file, hash);

        return hash;
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


// create a tree object

// create recusively tree for each dir in repo
// save tree object

int main() {
    GitRepo repo("/home/apoorva/programming/aprt-git/test");
    std::cout << "---------------------------------------" << std::endl;
    repo.commit("apoorvapendse", "rajeevtapadia", "Is this the real life\nIs this just fantasy");

    // std::cout<<sha1File("./README.md")<<std::endl;
    // std::cout << repo.get_hash_from_content("Hello world") << std::endl;
    return 0;
}
