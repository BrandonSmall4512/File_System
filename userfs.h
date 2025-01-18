#ifndef USERFS_H
#define USERFS_H

#include <stddef.h>
#include <stdbool.h>
#include <sys/stat.h>

#define UFS_READ 1
#define UFS_WRITE 2
#define UFS_CREATE 4

#define BLOCK_SIZE 256
#define MAX_FILE_DESCRIPTORS 256

struct block {
    char data[BLOCK_SIZE];
    struct block* next;
};

struct file {
    char* name;
    size_t size;
    char* data;
    struct block* blocks;
    struct file* next;
};


extern struct file* file_system;

struct file_descriptor {
    struct file* file;
    size_t offset;
    int mode;
    bool used;  
};

int ufs_open(const char* name, int flags);
ptrdiff_t ufs_write(int fd, const void* buf, size_t count);
ptrdiff_t ufs_read(int fd, void* buf, size_t count);
int ufs_close(int fd);
int ufs_delete(const char* name);
int ufs_resize(const char* name, size_t new_size);

int ufs_stat(const char* name, struct stat* statbuf);

#endif // USERFS_H

