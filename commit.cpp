#include <core.hpp>
void GitRepo::commit(std::string author, std::string committer, std::string commit_message) {
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

std::string GitRepo::hash_from_root(fs::path path = {}) {
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
            Entry dir_entry = Entry::generate_tree_entry(entry, dir_hash);
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
            Entry file_entry = Entry::generate_tree_entry(entry, file_hash);
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

CommitObject GitRepo::parse_commit_content(std::string content) {
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

void GitRepo::write_commit_hash_to_head_file(std::string hash) {
    // TODO: Store ref when it is implemented instead of storing hash directly in HEAD
    std::ofstream head_file(git_path / "HEAD");
    if (!head_file.is_open()) {
        throw std::runtime_error("could not open HEAD file");
    }
    head_file << hash;
}

std::string GitRepo::get_previous_commit_hash() {
    // TODO: If HEAD contains ref, read from refs/heads/<branch-name> for commit hash
    // Current assumption is HEAD will always contain the prrevious commit hash.

    return read_hash_from_head();
}

std::string GitRepo::read_hash_from_head() {
    std::ifstream head_file(git_path / "HEAD");
    if (!head_file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return "";
    }
    std::string hash;
    std::getline(head_file, hash);

    return hash;
}

// Returns the hash of the file during previous commit.
// Will be used to decide whether a file was modifed since last commit.
std::string GitRepo::get_file_hash_for_commit(std::string commit_hash, std::string relative_file_path) {
    std::string commit_content = read_object_content(commit_hash);
    CommitObject commit_object = parse_commit_content(commit_content);

    std::string root_tree_content = read_object_content(commit_object.tree);
    TreeObject root_tree_object = parse_tree_content(root_tree_content);
    return search_for_blob_hash_for_a_given_tree(root_tree_object, relative_file_path);
}
