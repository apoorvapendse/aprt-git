#include "core.hpp"

GitRepo::GitRepo(const std::string &basePath) {
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

void GitRepo::save_blob(const std::string &abs_file_path) {
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

void GitRepo::save_hash_from_content(std::string content) {
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

bool GitRepo::check_hash_exists_already(std::string hash) {
    std::string object_dir = hash.substr(0, 2);
    std::string object_file_name = hash.substr(2);
    fs::path object_path = git_path / "objects" / object_dir / object_file_name;
    return fs::exists(object_path);
}

