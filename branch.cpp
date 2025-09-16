#include "core.hpp"
#include <filesystem>
#include <ostream>

// TODO: implement this
bool is_safe_to_switch_branch() {
    return false;
}

void GitRepo::create_branch(std::string branch_name, std::string commit_hash) {
    if (commit_hash.empty()) {
        // read hash from head
        // this hash will be empty if there are no commits made yet
        // and empty refs/heads/branch_name file means there are no commits in that branch
        commit_hash = read_hash_from_head();
    }
    std::cout << "creating branch: " << commit_hash << std::endl;

    write_to_file_in_git("refs/heads/" + branch_name, commit_hash);
}

void GitRepo::switch_branch(std::string branch_name) {
    if (!fs::exists(git_path / "refs" / "heads" / branch_name)) {
        std::cerr << (branch_name + " does not exists") << std::endl;
    }

    write_to_file_in_git("HEAD", "ref: refs/heads/" + branch_name);
}

std::string GitRepo::get_current_branch_name() {
    std::ifstream head_file(git_path / "HEAD");
    if (!head_file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return "";
    }

    std::string head_content;
    std::getline(head_file, head_content);
    if (head_content.size() == 0) {
        return head_content;
    }

    std::string prefix = "ref:";
    if (head_content.compare(0, prefix.size(), prefix) != 0) {
        assert("getting branch name in detached state NOT VALID");
    }

    // resolve branch_name in attached state
    std::vector<std::string> splits = split_by_delimitor(head_content, '/');
    std::string branch_name = splits.back();
    return branch_name;
}
