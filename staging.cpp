#include "core.hpp"

IndexObject GitRepo::parse_index_file() {
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

// Only meant to be called by maybe_stage_file
void GitRepo::stage_file(fs::path abs_path, fs::path relative_file_path) {
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

void GitRepo::serialize_index_to_file(IndexObject &index) {
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

void GitRepo::clear_index_file() {
    std::ofstream outfile(this->git_path / "index");
    if (!outfile) {
        std::cerr << "Error opening index file for writing." << std::endl;
        return;
    }
    // This will overwrite
    outfile << "";
    outfile.close();
}

void GitRepo::add_entry_to_index(IndexObject &index, fs::path abs_path, IndexEntry new_entry) {
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

void GitRepo::maybe_stage_file(fs::path relative_file_path) {
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
        return;
    };

    stage_file(abs_path, relative_file_path);
}

void GitRepo::remove_staged_file(fs::path file_path) {
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
void GitRepo::save_index() {
    std::ofstream index_file(git_path / "index");
    index_file << index_content;
    index_file.close();
}

