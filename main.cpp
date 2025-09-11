#include <assert.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
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
    std::string perms;
    std::string hash;
    std::string type;
    std::string name;

    Entry(const std::string &perms, const std::string &hash, const std::string &type, const std::string &name)
        : perms(perms), hash(hash), type(type), name(name) {}

    std::string toString() const {
        std::ostringstream ss;
        ss << perms << " ";
        ss << type << " ";
        ss << hash << "\t";
        ss << name;
        return ss.str();
    }
};

// helper to generate entry content for a given child.
Entry generate_tree_entry(const fs::directory_entry &child, std::string entry_hash) {
    std::string perms;
    std::string type;
    std::string hash;

    if (child.is_regular_file()) {
        perms = "100644"; // typical blob file permissions
        type = "blob";
        hash = entry_hash;
    } else if (child.is_directory()) {
        perms = "040000"; // tree directory permissions in octal
        type = "tree";
        // For directories, compute tree hash recursively or placeholder
        hash = entry_hash; // replace with actual tree hash logic
    } else {
        throw std::runtime_error("Unsupported file type: " + child.path().string());
    }

    std::string name = child.path().filename().string();
    return Entry(perms, hash, type, name);
}

class CommitObject {
  public:
    std::string tree;
    std::string author;
    std::string committer;
    std::string parent;
};

class TreeObject {
  public:
    std::vector<Entry> children;
};

class IndexEntry {
  public:
    std::string hash;
    std::string relative_path;
    size_t size;
    uintmax_t time_stamp;

    IndexEntry(const std::string &hash, const std::string &relative_path, size_t size, uintmax_t time_stamp)
        : hash(hash), relative_path(relative_path), size(size), time_stamp(time_stamp) {}
};

class IndexObject {
  public:
    std::vector<IndexEntry> entries;
};

class GitRepo {
  public:
    fs::path repo_path;
    fs::path git_path;
    std::string index_content;

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

        {
            std::ofstream indexFile(git_path / "index");
            indexFile << "";
        }

        // TODO: load index file into string index_content

        std::cout << ".aprt-git repository initialized in " << git_path << std::endl;
    }

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

    void save_blob(const std::string &abs_file_path) {
        std::string hash = sha1_file(abs_file_path);

        std::string dir = git_path / "objects" / hash.substr(0, 2);
        std::string filename = hash.substr(2);

        fs::create_directories(dir);

        std::string fullPath = dir + "/" + filename;
        if (fs::exists(fullPath)) {
            std::cout << "Object already exists: " << fullPath << "\n";
            return;
        }

        std::ifstream src(abs_file_path, std::ios::binary);
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

    std::string get_file_content(std::string absolute_file_path) {
        std::ifstream inputFile(absolute_file_path);
        if (!inputFile.is_open()) {
            throw std::runtime_error("Error opening file: " + absolute_file_path + "\n");
        }

        std::string file_content;
        std::string currline;
        while (std::getline(inputFile, currline)) {
            file_content += currline + "\n";
        }
        return file_content;
    }

    std::string read_object_content(std::string hash) {
        std::string dir = git_path / "objects" / hash.substr(0, 2);
        std::string filename = hash.substr(2);

        std::string fullPath = dir + "/" + filename;
        if (!fs::exists(fullPath)) {
            throw std::runtime_error("Object doesn't exist: " + fullPath + "\n");
        }

        std::string object_content = get_file_content(fullPath);
        return object_content;
    }

    CommitObject parse_commit_content(std::string content) {
        CommitObject commit;

        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.rfind("tree ", 0) == 0) {
                commit.tree = line.substr(5);
            } else if (line.rfind("author ", 0) == 0) {
                commit.author = line.substr(7);
            } else if (line.rfind("committer ", 0) == 0) {
                commit.committer = line.substr(10);
            } else if (line.rfind("parent ", 0) == 0) {
                commit.parent = line.substr(7);
            }
            // TODO: Add commit message if required
        }

        return commit;
    }

    TreeObject parse_tree_content(std::string content) {
        TreeObject tree_obj;
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            std::string perms = line.substr(0, 6);
            std::string type = line.substr(7, 4);
            std::string hash = line.substr(12, 40);
            std::string name = line.substr(53);
            Entry e(perms, hash, type, name);
            tree_obj.children.push_back(e);
        }
        return tree_obj;
    }

    IndexObject parse_index_file() {
        std::string content = get_file_content(git_path / "index");

        std::istringstream iss(content);
        std::string line;
        IndexObject index_obj;

        while (std::getline(iss, line)) {
            auto parts = split_by_delimitor(line, ' ');
            std::string hash = parts[0];
            std::string relative_path = parts[1];
            size_t size = stoi(parts[2]);
            uintmax_t time_stamp = stoull(parts[3]);
            IndexEntry entry = IndexEntry(hash, relative_path, size, time_stamp);
            index_obj.entries.push_back(entry);
        }
        return index_obj;
    }

    // returns hashes of immediate children for a given tree
    std::vector<std::string> get_immediate_children_hashes(std::string tree_hash) {
        std::string tree_object_content = read_object_content(tree_hash);
        TreeObject tree_obj = parse_tree_content(tree_object_content);
        std::vector<std::string> children_hashes;
        for (auto &child : tree_obj.children) {
            children_hashes.push_back(child.hash);
        }
    }

    std::string hash_from_root(fs::path path = {}) {
        if (path.empty()) {
            path = this->repo_path;
        }
        std::cout << path << std::endl;
        std::cout << repo_path / ".aprt-git" << std::endl;
        if (path.is_absolute() && path == repo_path / ".aprt-git") {
            return "nothing changed";
        }
        // TODO:
        // 1. Don't hash if not staged, add check for this
        // 2.
        std::vector<Entry> entries;
        for (const auto &entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                std::string dir_hash = hash_from_root(entry.path());
                if (dir_hash == "nothing changed") {
                    continue;
                }
                Entry dir_entry = generate_tree_entry(entry, dir_hash);
                entries.push_back(dir_entry);
            } else if (entry.is_regular_file()) {

                // Only commit staged blobs with their staged hashes from index
                // If a blob is not staged, use get_file_hash_for_commit to get its previous hash
                // and simply return this.

                // Relative path upto this file.
                fs::path rel = fs::relative(entry.path(), repo_path);
                std::string path_to_check = rel.string();
                // TODO: Ensure path_to_check is present in `index` before generating entry for latest hash
                IndexObject index_obj = parse_index_file();
                bool current_file_is_staged = false;
                std::string file_hash; 
                for(auto &entry: index_obj.entries){
                    if(entry.relative_path == path_to_check) {
                        current_file_is_staged = true;
                        file_hash = entry.hash;
                        break;
                    }
                }
                if(!current_file_is_staged) {
                    std::string prev_commit_hash = get_previous_commit_hash();
                    if(prev_commit_hash.empty()) {
                        // Meaning we are creating the first commit and this file wasn't staged
                        // Avoid adding this as a new entry in the current tree
                        continue;
                    }
                    file_hash = get_file_hash_for_commit(prev_commit_hash, path_to_check);
                    if(file_hash == "new_blob") {
                        // Meaning this file was added in this commit and is not staged
                        // Avoid adding this as a new entry in the current tree
                        continue;
                    }
                }

                // Either file_hash will come from prev commit in case it isn't staged, or it will come from index, 
                // where path_to_check is the relative path mapped with the staged version of the file.
                Entry file_entry = generate_tree_entry(entry, file_hash);
                entries.push_back(file_entry);
            }
        }
        // TODO:
        // Hash this entry file and save it if not present
        // if hash_from_root(root) returns a hash that already exists, `nothing to commit, working tree clean`
        std::string final_tree_object_content = "";
        if (entries.size() == 0)
            return "nothing changed";
        for (const auto &e : entries) {
            if (e.type == "blob") {
                std::string blob_entry = "";
                blob_entry += e.perms + " ";
                blob_entry += e.type + " ";
                blob_entry += e.hash + " ";
                blob_entry += e.name + "\n";
                final_tree_object_content += blob_entry;
            } else {
                std::string tree_entry = "";
                tree_entry += e.perms + " ";
                tree_entry += e.type + " ";
                tree_entry += e.hash + " ";
                tree_entry += e.name + "\n";
                final_tree_object_content += tree_entry;
            }
        }
        // if this hash already exists, return something that denotes nothing changed here.
        if (check_hash_exists_already(get_hash_from_content(final_tree_object_content))) {
            return "nothing changed";
        }

        save_hash_from_content(final_tree_object_content);
        return get_hash_from_content(final_tree_object_content);
    }

    bool check_hash_exists_already(std::string hash) {
        std::string object_dir = hash.substr(0, 2);
        std::string object_file_name = hash.substr(2);
        fs::path object_path = git_path / "objects" / object_dir / object_file_name;
        return fs::exists(object_path);
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

    /*
        tree d906209d7ed2121a398d675ff840a05d12c5661f
        parent 85a48f686c2b9a841e01d5caf219bb726b423719
        author apoorvapendse <apoorvavpendse@gmail.com> 1756058996 +0530
        committer apoorvapendse <apoorvavpendse@gmail.com> 1756058996 +0530

        Store tree/blob objects on disk

        Signed-off-by: apoorvapendse <apoorvavpendse@gmail.com>
    */
    void commit(std::string author, std::string committer, std::string commit_message) {

        // TODO: Current commit implementation runs an implicit `git add .`. Now the next task is only committing
        // those blobs that were in index
        std::string root_tree_hash = hash_from_root();
        if (root_tree_hash == "nothing changed") {
            std::cout << "nothing to commit, working tree clean.";
            return;
        }

        std::string commit_content = "";
        std::string parent_hash = read_hash_from_head();
        commit_content += "tree " + root_tree_hash + "\n";
        // if (parent_hash.size() !=) {
        //     commit_content += "parent " + parent_hash + "\n";
        // }
        commit_content += "author " + author + "\n";
        commit_content += "committer " + committer + "\n\n";
        commit_content += commit_message;

        save_hash_from_content(commit_content);
        std::string commit_hash = get_hash_from_content(commit_content);
        // TODO: Save this commit_hash in HEAD. Think about interactive rebase 🤯
        write_commit_hash_to_head_file(commit_hash);

        clear_index_file();
    }

    void write_commit_hash_to_head_file(std::string hash) {
        // TODO: Store ref when it is implemented instead of storing hash directly in HEAD
        std::ofstream head_file(git_path / "HEAD");
        if (!head_file.is_open()) {
            throw std::runtime_error("could not open HEAD file");
        }
        head_file << hash;
    }

    std::string get_previous_commit_hash() {
        // TODO: If HEAD contains ref, read from refs/heads/<branch-name> for commit hash
        // Current assumption is HEAD will always contain the prrevious commit hash.

        return read_hash_from_head();
    }

    std::string read_hash_from_head() {
        std::ifstream head_file(git_path / "HEAD");
        if (!head_file.is_open()) {
            std::cerr << "Failed to open file!" << std::endl;
            return "";
        }
        std::string hash;
        std::getline(head_file, hash);

        return hash;
    }

    std::vector<std::string> split_by_delimitor(const std::string &str, char delim) {
        std::vector<std::string> result;
        std::string token;

        // Traverse the string and split by '/'
        for (char ch : str) {
            if (ch == delim) {
                if (!token.empty()) {
                    result.push_back(token);
                    token.clear();
                }
            } else {
                token += ch;
            }
        }

        // Add the last token if it's not empty
        if (!token.empty()) {
            result.push_back(token);
        }

        return result;
    }

    char get_path_seperator() {
#ifdef _WIN32
        '\\';
#else
        return '/';
#endif
    }

    std::string search_for_blob_hash_for_a_given_tree(TreeObject &root_tree_obj, const std::string &relative_path) {
        std::vector<std::string> path_parts = split_by_delimitor(relative_path, get_path_seperator());
        int n = path_parts.size();
        for (int i = 0; i < n - 1; i++) {
            for (auto child : root_tree_obj.children) {
                if (child.name == path_parts[i]) {
                    root_tree_obj = parse_tree_content(read_object_content(child.hash));
                    break;
                }
            }
        }
        // We have to find the blob from current root_tree_obj now.

        for (auto &child : root_tree_obj.children) {
            if (child.name == path_parts[n - 1]) {
                assert(child.type == "blob");
                return child.hash;
            }
        }

        return "new_blob";
    }

    // Returns the hash of the file during previous commit.
    // Will be used to decide whether a file was modifed since last commit.
    std::string get_file_hash_for_commit(std::string commit_hash, std::string relative_file_path) {
        std::string commit_content = read_object_content(commit_hash);
        CommitObject commit_object = parse_commit_content(commit_content);

        std::string root_tree_content = read_object_content(commit_object.tree);
        TreeObject root_tree_object = parse_tree_content(root_tree_content);
        return search_for_blob_hash_for_a_given_tree(root_tree_object, relative_file_path);
    }

    // Only meant to be called by maybe_stage_file
    void stage_file(fs::path abs_path, fs::path relative_file_path) {
        auto stat = fs::status(abs_path);
        auto size = fs::file_size(abs_path);
        auto mtime = fs::last_write_time(abs_path).time_since_epoch().count();
        std::string current_hash_for_blob = sha1_file(abs_path);
        save_blob(abs_path);
        std::cout << "stagin file" << std::endl;

        IndexObject index = parse_index_file();

        IndexEntry new_entry = IndexEntry(current_hash_for_blob, relative_file_path, size, mtime);
        add_entry_to_index(index, abs_path, new_entry);
    }

    void serialize_index_to_file(IndexObject &index) {
        std::ofstream outfile(this->git_path / "index");
        if (!outfile) {
            std::cerr << "Error opening index file for writing." << std::endl;
            return;
        }

        for (const IndexEntry &entry : index.entries) {
            outfile << entry.hash << " " << entry.relative_path << " " << entry.size << " " << entry.time_stamp << "\n";
        }

        outfile.close();
    };
    
    void clear_index_file(){
        std::ofstream outfile(this->git_path / "index");
        if (!outfile) {
            std::cerr << "Error opening index file for writing." << std::endl;
            return;
        }
        // This will overwrite
        outfile << "";
        outfile.close();
    }

    void add_entry_to_index(IndexObject &index, fs::path abs_path, IndexEntry new_entry) {
        bool exists = false;
        for (IndexEntry &entry : index.entries) {
            // if same relative_path exists -> update
            if (entry.relative_path == new_entry.relative_path) {
                if (entry.hash == new_entry.hash) {
                    std::cout << "duplicate hash detected file already staged" << std::endl;
                    return;
                }
                exists = true;
                entry.hash = new_entry.hash;
                entry.time_stamp = new_entry.time_stamp;
                entry.size = new_entry.size;
                std::cout << "entry updated succesfully" << std::endl;
            }
        }
        if (!exists) {
            index.entries.push_back(new_entry);
        }
        serialize_index_to_file(index);
    }

    void maybe_stage_file(fs::path relative_file_path) {
        fs::path abs_path = repo_path / relative_file_path;
        std::string previous_commit_hash = get_previous_commit_hash();
        // TODO: check for duplicate entry in the index file by hash for a given blob

        //
        if (previous_commit_hash.empty()) {
            // no commits have been made so far, so stage this file.
            std ::cout << "No prev commits detected, staging file blindly " << std::endl;
            stage_file(abs_path, relative_file_path);
            return;
        }
        std::string current_hash_for_blob = sha1_file(abs_path);
        std::string previous_hash_for_blob = get_file_hash_for_commit(previous_commit_hash, relative_file_path);

        if (previous_hash_for_blob == current_hash_for_blob) {
            std::cout << "No changes detected in blob to stage:" << relative_file_path.string() << std::endl;
        };

        stage_file(abs_path, relative_file_path);
    }

    void remove_staged_file(fs::path file_path) {
        fs::path index_path = git_path / "index";
        fs::path temp_path = git_path / "index.tmp";

        std::ifstream in(index_path);
        std::ofstream out(temp_path);

        std::string line;
        while (std::getline(in, line)) {
            if (line.find(file_path.string()) == std::string::npos) {
                out << line << "\n";
            }
        }

        in.close();
        out.close();

        fs::rename(temp_path, index_path);
    }
    void save_index() {
        std::ofstream index_file(git_path / "index");
        index_file << index_content;
        index_file.close();
    }
};

/*
 * staging a file should
 *  hash it
 *  create blob obj
 *  save blob obj
 *  add the entry ie the file path and hash in index file
 *  only file can be staged not directories
 */

// create a tree object

// create recusively tree for each dir in repo
// save tree object

std::string get_base_path_from_config() {
    std::ifstream file("config.aprt");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config.aprt" << std::endl;
        exit(1);
    }

    std::string base_path;
    if (std::getline(file, base_path)) {
    } else {
        std::cerr << "ERROR: config file is empty." << std::endl;
        exit(1);
    }
    return base_path;
}

int main() {
    GitRepo repo(get_base_path_from_config());
    // std::cout << "---------------------------------------" << std::endl;

    // repo.stage_file("README.txt");

    // std::cout<<sha1File("./README.md")<<std::endl;
    // std::cout << repo.get_hash_from_content("Hello world") << std::endl;

    // repo.parse_tree_content("100644 tree 1d7e200148f3b648f4af053c06777184d5328357 subdir\n040000 blob "
    //                         "89d69a3b673d7d7d5ab7ebf2bbd88d994b1cc633 README.txt");
    // repo.maybe_stage_file("README.txt");
    // repo.maybe_stage_file("lavda");
    // repo.maybe_stage_file("laugh");

    // repo.maybe_stage_file("subdir/gg.txt");
    // repo.commit("apoorvapendse", "rajeevtapadia", "1st commit");
    // repo.maybe_stage_file("README.txt");
    repo.commit("apoorvapendse", "rajeevtapadia", "2nd commit");
    return 0;
}
