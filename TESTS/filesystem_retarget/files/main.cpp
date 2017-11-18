#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include <stdlib.h>
#include <errno.h>

using namespace utest::v1;

// test configuration
#ifndef MBED_TEST_FILESYSTEM
#define MBED_TEST_FILESYSTEM LittleFileSystem
#endif

#ifndef MBED_TEST_FILESYSTEM_DECL
#define MBED_TEST_FILESYSTEM_DECL MBED_TEST_FILESYSTEM fs("fs")
#endif

#ifndef MBED_TEST_BLOCKDEVICE
#define MBED_TEST_BLOCKDEVICE SPIFBlockDevice
#define MBED_TEST_BLOCKDEVICE_DECL SPIFBlockDevice bd(PTE2, PTE4, PTE1, PTE5)
#endif

#ifndef MBED_TEST_BLOCKDEVICE_DECL
#define MBED_TEST_BLOCKDEVICE_DECL MBED_TEST_BLOCKDEVICE bd
#endif

#ifndef MBED_TEST_FILES
#define MBED_TEST_FILES 4
#endif

#ifndef MBED_TEST_DIRS
#define MBED_TEST_DIRS 4
#endif

#ifndef MBED_TEST_BUFFER
#define MBED_TEST_BUFFER 8192
#endif

#ifndef MBED_TEST_TIMEOUT
#define MBED_TEST_TIMEOUT 120
#endif


// declarations
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define INCLUDE(x) STRINGIZE(x.h)

#include INCLUDE(MBED_TEST_FILESYSTEM)
#include INCLUDE(MBED_TEST_BLOCKDEVICE)

MBED_TEST_FILESYSTEM_DECL;
MBED_TEST_BLOCKDEVICE_DECL;

Dir dir[MBED_TEST_DIRS];
File file[MBED_TEST_FILES];
DIR *dd[MBED_TEST_DIRS];
FILE *fd[MBED_TEST_FILES];
struct dirent ent;
struct dirent *ed;
size_t size;
uint8_t buffer[MBED_TEST_BUFFER];
uint8_t rbuffer[MBED_TEST_BUFFER];
uint8_t wbuffer[MBED_TEST_BUFFER];

static char file_counter = 0;
const char *filenames[] = {"smallavacado", "mediumavacado", "largeavacado",
                          "blockfile", "bigblockfile", "hello", ".", ".."};

// tests

void test_file_tests() {
    int res = bd.init();
    TEST_ASSERT_EQUAL(0, res);

    {
        res = MBED_TEST_FILESYSTEM::format(&bd);
        TEST_ASSERT_EQUAL(0, res);
    }

    res = bd.deinit();
    TEST_ASSERT_EQUAL(0, res);
}

void test_simple_file_test() {
    int res = bd.init();
    TEST_ASSERT_EQUAL(0, res);

    {
        res = fs.mount(&bd);
        TEST_ASSERT_EQUAL(0, res);
        res = !((fd[0] = fopen("/fs/" "hello", "wb")) != NULL);
        TEST_ASSERT_EQUAL(0, res);
        size = strlen("Hello World!\n");
        memcpy(wbuffer, "Hello World!\n", size);
        res = fwrite(wbuffer, 1, size, fd[0]);
        TEST_ASSERT_EQUAL(size, res);
        res = fclose(fd[0]);
        TEST_ASSERT_EQUAL(0, res);
        res = !((fd[0] = fopen("/fs/" "hello", "rb")) != NULL);
        TEST_ASSERT_EQUAL(0, res);
        size = strlen("Hello World!\n");
        res = fread(rbuffer, 1, size, fd[0]);
        TEST_ASSERT_EQUAL(size, res);
        res = memcmp(rbuffer, wbuffer, size);
        TEST_ASSERT_EQUAL(0, res);
        res = fclose(fd[0]);
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

    res = bd.deinit();
    TEST_ASSERT_EQUAL(0, res);
}

template <int file_size, int write_size, int read_size>
void test_write_file_test() {
    int res = bd.init();
    TEST_ASSERT_EQUAL(0, res);

    {
        size_t size = file_size;
        size_t chunk = write_size;
        srand(0);
        res = fs.mount(&bd);
        TEST_ASSERT_EQUAL(0, res);
        res = !((fd[0] = fopen("/fs/" "smallavacado", "wb")) != NULL);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            for (size_t b = 0; b < chunk; b++) {
                buffer[b] = rand() & 0xff;
            }
            res = fwrite(buffer, 1, chunk, fd[0]);
            TEST_ASSERT_EQUAL(chunk, res);
        }
        res = fclose(fd[0]);
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

    {
        size_t size = file_size;
        size_t chunk = read_size;
        srand(0);
        res = fs.mount(&bd);
        TEST_ASSERT_EQUAL(0, res);
        res = !((fd[0] = fopen("/fs/" "smallavacado", "rb")) != NULL);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            res = fread(buffer, 1, chunk, fd[0]);
            TEST_ASSERT_EQUAL(chunk, res);
            for (size_t b = 0; b < chunk && i+b < size; b++) {
                res = buffer[b];
                TEST_ASSERT_EQUAL(rand() & 0xff, res);
            }
        }
        res = fclose(fd[0]);
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

    file_counter++;
    res = bd.deinit();
    TEST_ASSERT_EQUAL(0, res);
}

void test_non_overlap_check() {
    int res = bd.init();
    TEST_ASSERT_EQUAL(0, res);

    {
        size_t size = 32;
        size_t chunk = 29;
        srand(0);
        res = fs.mount(&bd);
        TEST_ASSERT_EQUAL(0, res);
        res = !((fd[0] = fopen("/fs/" "smallavacado", "rb")) != NULL);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            res = fread(buffer, 1, chunk, fd[0]);
            TEST_ASSERT_EQUAL(chunk, res);
            for (size_t b = 0; b < chunk && i+b < size; b++) {
                res = buffer[b];
                TEST_ASSERT_EQUAL(rand() & 0xff, res);
            }
        }
        res = fclose(fd[0]);
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

    {
        size_t size = 8192;
        size_t chunk = 29;
        srand(0);
        res = fs.mount(&bd);
        TEST_ASSERT_EQUAL(0, res);
        res = !((fd[0] = fopen("/fs/" "mediumavacado", "rb")) != NULL);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            res = fread(buffer, 1, chunk, fd[0]);
            TEST_ASSERT_EQUAL(chunk, res);
            for (size_t b = 0; b < chunk && i+b < size; b++) {
                res = buffer[b];
                TEST_ASSERT_EQUAL(rand() & 0xff, res);
            }
        }
        res = fclose(fd[0]);
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

    {
        size_t size = 262144;
        size_t chunk = 29;
        srand(0);
        res = fs.mount(&bd);
        TEST_ASSERT_EQUAL(0, res);
        res = !((fd[0] = fopen("/fs/" "largeavacado", "rb")) != NULL);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            res = fread(buffer, 1, chunk, fd[0]);
            TEST_ASSERT_EQUAL(chunk, res);
            for (size_t b = 0; b < chunk && i+b < size; b++) {
                res = buffer[b];
                TEST_ASSERT_EQUAL(rand() & 0xff, res);
            }
        }
        res = fclose(fd[0]);
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

    res = bd.deinit();
    TEST_ASSERT_EQUAL(0, res);
}

void test_dir_check() {

    int res = bd.init();
    TEST_ASSERT_EQUAL(0, res);

    {
        res = fs.mount(&bd);
        TEST_ASSERT_EQUAL(0, res);
        res = !((dd[0] = opendir("/fs/" "/")) != NULL);
        TEST_ASSERT_EQUAL(0, res);
        int numFiles = sizeof(filenames)/sizeof(filenames[0]);
        // Check the filenames in directory
        while(1) {
            ed = readdir(dd[0]));
            res = dir[0].read(&ent);
            if (NULL == ed) {
                break;
            }
            for (int i=0; i < numFiles ; i++) {
                res = strcmp(ed->d_name, filenames[i]);
                if (0 == res) {
                    res = ed->d_type;
                    if ((DT_REG != res) && (DT_DIR != res)) {
                        TEST_ASSERT(1);
                    }
                    break;
                }
                else if( i == numFiles) {
                    TEST_ASSERT_EQUAL(0, res);
                }
            }
        }
        res = closedir(dd[0]);
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

    res = bd.deinit();
    TEST_ASSERT_EQUAL(0, res);
}


// test setup
utest::v1::status_t test_setup(const size_t number_of_cases) {
    GREENTEA_SETUP(MBED_TEST_TIMEOUT, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("File tests", test_file_tests),
    Case("Simple file test", test_simple_file_test),
    Case("Small file test", test_write_file_test<32, 31, 29>),
    Case("Medium file test", test_write_file_test<8192, 31, 29>),
    Case("Large file test", test_write_file_test<262144, 31, 29>),
    Case("Block size file test", test_write_file_test<9000, 512, 512>),
    Case("Multiple block size file test", test_write_file_test<26215, MBED_TEST_BUFFER, MBED_TEST_BUFFER>),
    Case("Non-overlap check", test_non_overlap_check),
    Case("Dir check", test_dir_check),
};

Specification specification(test_setup, cases);

int main() {
    return !Harness::run(specification);
}
