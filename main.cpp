#include <assert.h>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <ostream>
#include <string>

#include "core.hpp"
GitRepo repo;

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
    repo = GitRepo(get_base_path_from_config());
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
    // repo.commit("apoorvapendse", "rajeevtapadia", "two commit");
    print_git_log();
    return 0;
}

