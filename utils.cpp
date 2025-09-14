#include "core.hpp"

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
