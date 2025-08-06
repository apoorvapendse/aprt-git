#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MB 1024 * 1000

typedef struct Hash{
    char hash[41];
} Hash;

// TODO: add more struct fields as per requirement
typedef struct ObjectStore {
    FILE* root_dir_path;
    FILE* objects_dir_path;
    Hash* HEAD;
} ObjectStore;

typedef struct Commit {

} Commit;

typedef struct Tree {

} Tree;

typedef struct Blob {
    Hash hash;
} Blob;

long int get_file_size(FILE* fp) {
    fseek(fp, 0L, SEEK_END);
    long int size = ftell(fp);
    rewind(fp);
    return size;
}

Hash* blob_hash(char* content) {
    Hash* hash = (Hash*) malloc(sizeof(Hash));
    strcpy(hash->hash, "abcd");
    return hash;
}

char* hash_to_path(Hash* hash) {
    char* hash_str = hash->hash;
    char* path = malloc(42 * sizeof(char));
    strncpy(path, hash_str, 2);
    path[2] = '/';
    strncpy(path+3, hash_str+2, 39);
    return path;
}

// create a blob on disk
Blob* blob_create(FILE* file){
    // read file
    long int file_size = get_file_size(file);
    char* buf = (char*) malloc(file_size* sizeof(char));
    fread(buf, file_size, file_size, file);
    printf("%s\n", buf);
    Hash* hash = blob_hash(buf);
    printf("%s\n", hash->hash);
    // store on disk at appropriate path
    // check if exist already
    // /ab/cd
    char* path = hash_to_path(hash);
    printf("path: %s\n", path);
    char str[99];
    str[0] = '.';
    str[1] = '/';
    mkdir("test-git", 755);
    printf("%d\n",mkdir(strcat(str,path), 755));
    return NULL;
}
// void blob_save(Blob*);

// create a dir structure
// child folder will be objects, HEAD, refs
ObjectStore *create_object_store();

int main() {
    printf("deez\n");
    FILE* fp = fopen("test", "r");
    assert(fp != NULL);
    blob_create(fp);
    return 0;
}
