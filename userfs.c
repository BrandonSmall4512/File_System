#include "userfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h> 

#define BLOCK_SIZE 256
#define MAX_FILE_DESCRIPTORS 256

struct file* file_system = NULL;
static struct file_descriptor fds[MAX_FILE_DESCRIPTORS];

static struct file* find_file(const char* name) {
    struct file* current = file_system;
    while (current) {
        if (strcmp(current->name, name) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}

static struct block* allocate_block_chain(size_t num_blocks) {
    struct block* head = NULL;
    struct block** current = &head;

    for (size_t i = 0; i < num_blocks; i++) {
        *current = malloc(sizeof(struct block));
        if (!*current) {
            while (head) {
                struct block* temp = head;
                head = head->next;
                free(temp);
            }
            return NULL;
        }

        memset((*current)->data, 0, BLOCK_SIZE);
        (*current)->next = NULL;

        current = &(*current)->next;
    }

    return head;
}

int ufs_open(const char* name, int flags) {
    if (!name || !(flags & (UFS_READ | UFS_WRITE | UFS_CREATE))) {
        errno = EINVAL;
        return -1;
    }

    struct file* file = find_file(name);
    if (!file && (flags & UFS_CREATE)) {
        file = malloc(sizeof(struct file));
        if (!file) {
            errno = ENOMEM;
            return -1;
        }
        file->name = strdup(name);
        file->size = 0;
        file->blocks = NULL;
        file->next = file_system;
        file_system = file;
        printf("File '%s' created successfully.\n", name);
    }
    else if (!file) {
        errno = ENOENT;
        return -1;
    }

    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (!fds[i].used) {
            fds[i].file = file;
            fds[i].offset = 0;
            fds[i].mode = flags & (UFS_READ | UFS_WRITE);
            fds[i].used = true;
            printf("File '%s' opened successfully with descriptor %d.\n", name, i);
            return i;
        }
    }

    errno = EMFILE;
    return -1;
}

ptrdiff_t ufs_write(int fd, const void* buf, size_t count) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fds[fd].used || !(fds[fd].mode & UFS_WRITE)) {
        errno = EBADF;
        return -1;
    }

    struct file* file = fds[fd].file;
    size_t written = 0;

    while (written < count) {
        size_t block_index = fds[fd].offset / BLOCK_SIZE;
        size_t block_offset = fds[fd].offset % BLOCK_SIZE;

        struct block* current_block = file->blocks;
        struct block** last_block = &file->blocks;

        for (size_t i = 0; i < block_index; i++) {
            if (current_block != NULL) {
                current_block = current_block->next;
            }
            else {
                current_block = allocate_block_chain(1);
                *last_block = current_block;
            }
        }

        if (current_block == NULL) {
            current_block = allocate_block_chain(1);
            *last_block = current_block;
        }

        if (current_block) {
            size_t to_write = BLOCK_SIZE - block_offset;
            if (to_write > count - written)
                to_write = count - written;

            memcpy(current_block->data + block_offset, (const char*)buf + written, to_write);
            written += to_write;
            fds[fd].offset += to_write;
        }
        else {
            errno = ENOMEM;
            return -1;
        }
    }

    if (fds[fd].offset > file->size)
        file->size = fds[fd].offset;

    printf("Successfully wrote %zu bytes to the file.\n", written);
    return written;
}

ptrdiff_t ufs_read(int fd, void* buf, size_t count) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fds[fd].used || !(fds[fd].mode & UFS_READ)) {
        errno = EBADF;
        return -1;
    }

    struct file* file = fds[fd].file;
    size_t read = 0;

    while (read < count) {
        size_t block_index = fds[fd].offset / BLOCK_SIZE;
        size_t block_offset = fds[fd].offset % BLOCK_SIZE;

        struct block* current_block = file->blocks;

        for (size_t i = 0; i < block_index; i++) {
            if (!current_block)
                return read;
            current_block = current_block->next;
        }

        if (!current_block)
            return read;

        size_t to_read = BLOCK_SIZE - block_offset;
        if (to_read > count - read)
            to_read = count - read;

        memcpy((char*)buf + read, current_block->data + block_offset, to_read);
        read += to_read;
        fds[fd].offset += to_read;
    }

    printf("Successfully read %zu bytes from the file.\n", read);
    return read;
}


int ufs_close(int fd) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fds[fd].used) {
        errno = EBADF;
        return -1;
    }

    fds[fd].used = false;
    printf("File descriptor %d closed successfully.\n", fd);
    return 0;
}

int ufs_delete(const char* name) {
    struct file** current = &file_system;

    while (*current) {
        if (strcmp((*current)->name, name) == 0) {
            struct file* to_delete = *current;
            *current = (*current)->next;

            free(to_delete->name);
            struct block* block = to_delete->blocks;
            while (block) {
                struct block* next = block->next;
                free(block);
                block = next;
            }
            free(to_delete);
            printf("File '%s' deleted successfully.\n", name);
            return 0;
        }
        current = &(*current)->next;
    }

    errno = ENOENT;
    return -1;
}

int ufs_resize(const char* name, size_t new_size) {
    struct file* file = find_file(name);
    if (!file) {
        errno = ENOENT;
        return -1;
    }

    if (new_size == file->size) {
        return 0; 
    }

    if (new_size > file->size) {
        char* new_data = realloc(file->data, new_size);
        if (!new_data) {
            errno = ENOMEM;
            return -1;
        }
        
        memset(new_data + file->size, 0, new_size - file->size);
        file->data = new_data;
    } else {
        char* new_data = realloc(file->data, new_size);
        if (!new_data && new_size > 0) {
            errno = ENOMEM;
            return -1;
        }
        file->data = new_data;
    }

    file->size = new_size;
    return 0;
}


int ufs_stat(const char* name, struct stat* statbuf) {
    if (!name || !statbuf) {
        errno = EINVAL;
        return -1;
    }

    struct file* file = find_file(name);
    if (!file) {
        errno = ENOENT;
        return -1;
    }

    memset(statbuf, 0, sizeof(struct stat));
    statbuf->st_size = file->size; 
    statbuf->st_mode = S_IFREG;   

    return 0;
}



#ifndef TEST_MODE
int main() {
    int fd = ufs_open("testfile.txt", UFS_CREATE | UFS_WRITE);
    if (fd == -1) {
        perror("Failed to open file for writing");
        return 1;
    }

    const char* data = "Hello!)";
    if (ufs_write(fd, data, strlen(data)) == -1) {
        perror("Failed to write to file");
        ufs_close(fd);
        return 1;
    }

    ufs_close(fd);
    fd = ufs_open("testfile.txt", UFS_READ);
    if (fd == -1) {
        perror("Failed to open file for reading");
        return 1;
    }

    char buffer[256];
    ptrdiff_t bytes_read = ufs_read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        perror("Failed to read from file");
        ufs_close(fd);
        return 1;
    }
    buffer[bytes_read] = '\0';

    printf("Read from file: %s\n", buffer);

    if (ufs_close(fd) == -1) {
        perror("Failed to close file");
        return 1;
    }

    if (ufs_delete("testfile.txt") == -1) {
        perror("Failed to delete file");
        return 1;
    }

    printf("File operations completed successfully.\n");

    return 0;
}
#endif

