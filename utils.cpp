#include "core.hpp"
#include <algorithm>
#include <filesystem>

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

std::string get_file_content(std::string absolute_file_path) {
    std::ifstream inputFile(absolute_file_path, std::ios::binary);
    if (!inputFile.is_open()) {
        throw std::runtime_error("Error opening file: " + absolute_file_path);
    }

    // seek to end to get size
    inputFile.seekg(0, std::ios::end);
    std::streamsize size = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    std::string file_content(size, '\0');

    if (!inputFile.read(&file_content[0], size)) {
        throw std::runtime_error("Error reading file: " + absolute_file_path);
    }

    return file_content;
}

std::string GitRepo::read_object_content(std::string hash) {
    std::string dir = git_path / "objects" / hash.substr(0, 2);
    std::string filename = hash.substr(2);

    std::string fullPath = dir + "/" + filename;
    if (!fs::exists(fullPath)) {
        throw std::runtime_error("Object doesn't exist: " + fullPath + "\n");
    }

    std::string object_content = get_file_content(fullPath);
    return object_content;
}

void show_in_pager(const std::string &text) {
    FILE *pager = popen("less", "w");
    if (!pager) {
        std::cerr << "Failed to open pager" << std::endl;
        return;
    }

    // Write string to pager
    fwrite(text.c_str(), 1, text.size(), pager);

    // Close pager
    pclose(pager);
}

void create_file_in_git(std::string file_path) {
    fs::path full_path = repo.git_path / file_path;
    fs::path parent_dir = full_path.parent_path();
    assert(fs::exists(parent_dir));

    std::ofstream file(full_path);
    if (!file.is_open()) {
        std::cerr << "Failed to create file: " << full_path << std::endl;
        return;
    }

    file << "";
    file.close();
}

void write_to_file_in_git(std::string file_path, std::string content) {
    fs::path full_path = repo.git_path / file_path;
    fs::path parent_dir = full_path.parent_path();
    assert(fs::exists(parent_dir));

    std::ofstream file(full_path);
    if (!file.is_open()) {
        std::cerr << "Failed to create file: " << full_path << std::endl;
        return;
    }

    file << content;
    file.close();
}

std::string get_file_content_in_git(std::string file_path) {
    fs::path full_path = repo.git_path / file_path;
    return get_file_content(full_path);
}
