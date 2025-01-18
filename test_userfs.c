#include "userfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define BLOCK_SIZE 256

void test_open_write_read_close() {
    int fd = ufs_open("testfile.txt", UFS_CREATE | UFS_WRITE);
    assert(fd >= 0 && "Failed to open file for writing");

    const char* data = "Hello, UserFS!";
    ptrdiff_t written = (ptrdiff_t)ufs_write(fd, data, strlen(data));
    assert(written == (ptrdiff_t)strlen(data) && "Failed to write data");

    assert(ufs_close(fd) == 0 && "Failed to close file");

    fd = ufs_open("testfile.txt", UFS_READ);
    assert(fd >= 0 && "Failed to open file for reading");

    char buffer[64];
    ptrdiff_t read = (ptrdiff_t)ufs_read(fd, buffer, written);
    assert(read == written && "Failed to read data");   
    buffer[read] = '\0';
    assert(strcmp(buffer, data) == 0 && "Data mismatch");

    assert(ufs_close(fd) == 0 && "Failed to close file");
}



void test_delete_file() {
    int fd = ufs_open("deletable.txt", UFS_CREATE | UFS_WRITE);
    assert(fd >= 0 && "Failed to open file for deletion");
    assert(ufs_close(fd) == 0 && "Failed to close file");

    assert(ufs_delete("deletable.txt") == 0 && "Failed to delete file");
    assert(ufs_open("deletable.txt", UFS_READ) == -1 && "File still exists after deletion");
}

void test_multiple_files() {
    const char *filenames[] = {"file1.txt", "file2.txt", "file3.txt"};
    const char *contents[] = {"Hello", "World", "Test"};

    for (int i = 0; i < 3; i++) {
        int fd = ufs_open(filenames[i], UFS_CREATE | UFS_WRITE);
        assert(fd >= 0 && "Failed to open file for writing");

        ptrdiff_t written = ufs_write(fd, contents[i], strlen(contents[i]));
        assert(written == (ptrdiff_t)strlen(contents[i]) && "Failed to write data");

        assert(ufs_close(fd) == 0 && "Failed to close file");
    }

    for (int i = 0; i < 3; i++) {
        int fd = ufs_open(filenames[i], UFS_READ);
        assert(fd >= 0 && "Failed to open file for reading");

        char buffer[64];
        ptrdiff_t written = strlen(contents[i]); 
        ptrdiff_t read = ufs_read(fd, buffer, written);
        assert(read == written && "Failed to read data");   

        buffer[read] = '\0';
        assert(strcmp(buffer, contents[i]) == 0 && "Data mismatch");

        assert(ufs_close(fd) == 0 && "Failed to close file");
    }
}


void test_resize_file() {
    const char *filename = "resizable.txt";
    const char *data = "Hello, world!"; 
    const size_t original_size = strlen(data);
    const size_t new_size = 10;  

    int fd = ufs_open(filename, UFS_CREATE | UFS_WRITE);
    assert(fd >= 0 && "Failed to open file for writing");
    ptrdiff_t written = ufs_write(fd, data, original_size);
    assert(written == (ptrdiff_t)original_size && "Failed to write data");
    assert(ufs_close(fd) == 0 && "Failed to close file");

    fd = ufs_open(filename, UFS_WRITE);
    assert(fd >= 0 && "Failed to open file for resizing");
    int resize_result = ufs_resize(filename, new_size);
    assert(resize_result == 0 && "Failed to resize file");
    assert(ufs_close(fd) == 0 && "Failed to close file after resizing");

    struct stat file_stat;
    int stat_result = ufs_stat(filename, &file_stat);
    assert(stat_result == 0 && "Failed to get file stats");
    assert((off_t)new_size == file_stat.st_size && "File size not correctly updated");


    fd = ufs_open(filename, UFS_READ);
    assert(fd >= 0 && "Failed to open file for reading");

    char buffer[64];  
    ptrdiff_t read = ufs_read(fd, buffer, new_size);  
    assert((size_t)read == new_size && "Incorrect size after resizing");  
    
    
    buffer[read] = '\0';  
    
    assert(memcmp(buffer, data, new_size) == 0 && "Data mismatch after resizing");

    assert(ufs_close(fd) == 0 && "Failed to close file");
}


void test_large_write() {
    int fd = ufs_open("largefile.txt", UFS_CREATE | UFS_WRITE);
    assert(fd >= 0 && "Failed to open file for large write");

    char* data = malloc(BLOCK_SIZE * 2);
    if (data != NULL) {
        memset(data, 'A', BLOCK_SIZE * 2);

        assert(ufs_write(fd, data, BLOCK_SIZE * 2) == (ptrdiff_t)(BLOCK_SIZE * 2) && "Failed to write large data");

        free(data);
    }
    assert(ufs_close(fd) == 0 && "Failed to close file");
}


void test_error_handling() {
    assert(ufs_open(NULL, UFS_CREATE) == -1 && "Opened file with invalid name");
    assert(ufs_open("invalid.txt", UFS_READ) == -1 && "Opened non-existing file");
    assert(ufs_close(999) == -1 && "Closed invalid file descriptor");
}


int main() {
    test_open_write_read_close();
    test_delete_file();
    test_multiple_files();
    test_resize_file();
    test_large_write();
    test_error_handling();

    printf("All tests passed successfully.\n");
    return 0;
}

