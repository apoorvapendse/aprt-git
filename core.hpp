#ifndef CORE_HEADER
#define CORE_HEADER
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


    Entry(const std::string &perms, const std::string &hash, const std::string &type, const std::string &name);
    std::string toString() const;
    static Entry generate_tree_entry(const fs::directory_entry &child, std::string entry_hash);
};

class CommitObject {
  public:
    std::string tree;
    std::string author;
    std::string committer;
    std::string parent;

    static CommitObject parse_commit_content(std::string content);
};

class TreeObject {
  public:
    std::vector<Entry> children;

    static TreeObject parse_tree_content(std::string content);
};

class IndexEntry {
  public:
    std::string hash;
    std::string relative_path;
    size_t size;
    uintmax_t time_stamp;

    IndexEntry(const std::string &hash, const std::string &relative_path, size_t size, uintmax_t time_stamp);
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

    GitRepo(const std::string &basePath = ".");
    void save_blob(const std::string &abs_file_path);
    void save_hash_from_content(std::string content);
    IndexObject parse_index_file();
    std::string read_object_content(std::string hash);
    std::string hash_from_root(fs::path path = {});
    bool check_hash_exists_already(std::string hash);
    void commit(std::string author, std::string committer, std::string commit_message);
    void write_commit_hash_to_head_file(std::string hash);
    std::string get_previous_commit_hash();
    std::string read_hash_from_head();
    std::string search_for_blob_hash_for_a_given_tree(TreeObject &root_tree_obj, const std::string &relative_path);
    std::string get_file_hash_for_commit(std::string commit_hash, std::string relative_file_path);
    void stage_file(fs::path abs_path, fs::path relative_file_path);
    void serialize_index_to_file(IndexObject &index);
    void clear_index_file();
    void add_entry_to_index(IndexObject &index, fs::path abs_path, IndexEntry new_entry);
    void maybe_stage_file(fs::path relative_file_path);
    void remove_staged_file(fs::path file_path);
    void save_index();
    std::vector<std::string> get_immediate_children_hashes(std::string tree_hash);
};

extern GitRepo repo;

std::string get_base_path_from_config();
std::string sha1_file(const std::string &abs_file_path);
std::string get_file_content(std::string absolute_file_path);
std::string get_hash_from_content(std::string content);
std::vector<std::string> split_by_delimitor(const std::string &str, char delim);
char get_path_seperator();

#endif
