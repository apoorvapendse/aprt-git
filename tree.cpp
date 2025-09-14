#include "core.hpp"


std::string GitRepo::search_for_blob_hash_for_a_given_tree(TreeObject &root_tree_obj, const std::string &relative_path) {
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


TreeObject GitRepo::parse_tree_content(std::string content) {
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

// helper to generate entry content for a given child.
Entry Entry::generate_tree_entry(const fs::directory_entry &child, std::string entry_hash) {
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

// returns hashes of immediate children for a given tree
std::vector<std::string> GitRepo::get_immediate_children_hashes(std::string tree_hash) {
    std::string tree_object_content = read_object_content(tree_hash);
    TreeObject tree_obj = parse_tree_content(tree_object_content);
    std::vector<std::string> children_hashes;
    for (auto &child : tree_obj.children) {
        children_hashes.push_back(child.hash);
    }
}



