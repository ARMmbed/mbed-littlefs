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
        res = file[0].open(&fs, "hello", O_WRONLY | O_CREAT);
        TEST_ASSERT_EQUAL(0, res);
        size = strlen("Hello World!\n");
        memcpy(wbuffer, "Hello World!\n", size);
        res = file[0].write(wbuffer, size);
        TEST_ASSERT_EQUAL(size, res);
        res = file[0].close();
        TEST_ASSERT_EQUAL(0, res);
        res = file[0].open(&fs, "hello", O_RDONLY);
        TEST_ASSERT_EQUAL(0, res);
        size = strlen("Hello World!\n");
        res = file[0].read(rbuffer, size);
        TEST_ASSERT_EQUAL(size, res);
        res = memcmp(rbuffer, wbuffer, size);
        TEST_ASSERT_EQUAL(0, res);
        res = file[0].close();
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

    res = bd.deinit();
    TEST_ASSERT_EQUAL(0, res);
}

template <int file_size, int write_size, int read_size, const char * filename>
void test_write_file_test() {
    int res = bd.init();
    TEST_ASSERT_EQUAL(0, res);

    {
        size_t size = file_size;
        size_t chunk = write_size;
        srand(0);
        res = fs.mount(&bd);
        TEST_ASSERT_EQUAL(0, res);
        res = file[0].open(&fs, filename, O_WRONLY | O_CREAT);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            for (size_t b = 0; b < chunk; b++) {
                buffer[b] = rand() & 0xff;
            }
            res = file[0].write(buffer, chunk);
            TEST_ASSERT_EQUAL(chunk, res);
        }
        res = file[0].close();
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
        res = file[0].open(&fs, filename, O_RDONLY);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            res = file[0].read(buffer, chunk);
            TEST_ASSERT_EQUAL(chunk, res);
            for (size_t b = 0; b < chunk && i+b < size; b++) {
                res = buffer[b];
                TEST_ASSERT_EQUAL(rand() & 0xff, res);
            }
        }
        res = file[0].close();
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

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
        res = file[0].open(&fs, "smallavacado", O_RDONLY);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            res = file[0].read(buffer, chunk);
            TEST_ASSERT_EQUAL(chunk, res);
            for (size_t b = 0; b < chunk && i+b < size; b++) {
                res = buffer[b];
                TEST_ASSERT_EQUAL(rand() & 0xff, res);
            }
        }
        res = file[0].close();
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
        res = file[0].open(&fs, "mediumavacado", O_RDONLY);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            res = file[0].read(buffer, chunk);
            TEST_ASSERT_EQUAL(chunk, res);
            for (size_t b = 0; b < chunk && i+b < size; b++) {
                res = buffer[b];
                TEST_ASSERT_EQUAL(rand() & 0xff, res);
            }
        }
        res = file[0].close();
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
        res = file[0].open(&fs, "largeavacado", O_RDONLY);
        TEST_ASSERT_EQUAL(0, res);
        for (size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            res = file[0].read(buffer, chunk);
            TEST_ASSERT_EQUAL(chunk, res);
            for (size_t b = 0; b < chunk && i+b < size; b++) {
                res = buffer[b];
                TEST_ASSERT_EQUAL(rand() & 0xff, res);
            }
        }
        res = file[0].close();
        TEST_ASSERT_EQUAL(0, res);
        res = fs.unmount();
        TEST_ASSERT_EQUAL(0, res);
    }

    res = bd.deinit();
    TEST_ASSERT_EQUAL(0, res);
}

void test_dir_check() {

    char *filenames[] = {"blockfile", "bigblockfile", "hello", "smallavacado",
                         "mediumavacado", "largeavacado", ".", ".."};
    int res = bd.init();
    TEST_ASSERT_EQUAL(0, res);

    {
        res = fs.mount(&bd);
        TEST_ASSERT_EQUAL(0, res);
        res = dir[0].open(&fs, "/");
        TEST_ASSERT_EQUAL(0, res);
        int numFiles = sizeof(filenames)/sizeof(filenames[0]);
        // Check the filenames in directory
        while(1) {
            res = dir[0].read(&ent);
            if (0 == res) {
                break;
            }
            for (int i=0; i < numFiles ; i++) {
                res = strcmp(ent.d_name, filenames[i]);
                if (0 == res) {
                    res = ent.d_type;
                    // Check if files (intial) in filenames declaration are
                    // regular files or not.
                    if ( i < (numFiles-3)) {
                        TEST_ASSERT_EQUAL(DT_REG, res);
                    }else {
                        TEST_ASSERT_EQUAL(DT_DIR, res);
                    }
                    break;
                }
                else if( i == numFiles) {
                    TEST_ASSERT_EQUAL(0, res);
                }
            }
        }
        res = dir[0].close();
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
    Case("Small file test", test_write_file_test<32, 31, 29, "smallavacado">),
    Case("Medium file test", test_write_file_test<8192, 31, 29, "mediumavacado">),
    Case("Large file test", test_write_file_test<262144, 31, 29, "largeavacado">),
    Case("Block size file test", test_write_file_test<9000, 512, 512, "blockfile">),
    Case("Multiple block size file test", test_write_file_test<26215, MBED_TEST_BUFFER, MBED_TEST_BUFFER, "bigblockfile">),
    Case("Non-overlap check", test_non_overlap_check),
    Case("Dir check", test_dir_check),
};

Specification specification(test_setup, cases);

int main() {
    return !Harness::run(specification);
}
