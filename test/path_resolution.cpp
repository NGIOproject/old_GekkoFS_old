
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <system_error>
#include <cassert>
#include <cstring>
#include <sys/stat.h>


static const std::string extdir = "/tmp/ext.tmp";
static const std::string ext_linkdir = "/tmp/link.tmp";
static const std::string nodir = "/tmp/notexistent";
static const std::string mountdir = "/tmp/mountdir";
static const std::string intdir = mountdir + "/int";


void setup() {
    int ret;

    // Clean external dir
    ret = rmdir(extdir.c_str());
    if (ret != 0) {
        if (errno != ENOENT) {
            std::cerr << "ERROR: cannot remove external dir: " <<
                strerror(errno) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    ret = mkdir(extdir.c_str(), 0770);
    if (ret != 0) {
        std::cerr << "ERROR: cannot create external dir: "
            << strerror(errno) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Clean internal dir
    ret = rmdir(intdir.c_str());
    if (ret != 0) {
        if (errno != ENOENT) {
            std::cerr << "ERROR: cannot remove internal dir: " <<
                strerror(errno) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    ret = mkdir(intdir.c_str(), 0770);
    if (ret != 0) {
        std::cerr << "ERROR: cannot create internal dir: "
            << strerror(errno) << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


void teardown() {
    // Clean external link
    if (unlink(ext_linkdir.c_str())) {
        if (errno != ENOENT) {
            std::cerr << "ERROR: cannot remove external dir: " <<
                strerror(errno) << std::endl;
        }
    }

    // Clean external dir
    if (rmdir(extdir.c_str())) {
        if (errno != ENOENT) {
            std::cerr << "ERROR: cannot remove external dir: " <<
                strerror(errno) << std::endl;
        }
    }

    // Clean internal dir
    if (rmdir(intdir.c_str())) {
        if (errno != ENOENT) {
            std::cerr << "ERROR: cannot remove internal dir: " <<
                strerror(errno) << std::endl;
        }
    }

}

void test_chdir(const std::string& dst, const std::string& expected) {
    char path[125];
    int ret = chdir(dst.c_str());
    if (ret != 0) {
        throw std::system_error(errno, std::system_category(),
                "ERROR: Failed to chdir into: " + dst);
    }

    char * cwd = getcwd(path, sizeof(path));
    if (cwd == NULL) {
        throw std::system_error(errno, std::system_category(),
                "ERROR: Failed to get current cwd");
    }

    if (std::string(path) != expected) {
        throw std::system_error(errno, std::system_category(),
                "ERROR: Expected path do not match");
    }
}


int main(int argc, char* argv[]) {

    if(std::atexit(teardown)) {
         std::cerr << "Teardown function registration failed" << std::endl;
         return EXIT_FAILURE;
    }

    setup();

    char buffIn[] = "oops.";
    char *buffOut = new char[strlen(buffIn) + 1 + 20 ];
    char path[125];

    struct stat st;
    int fd;
    int ret;

    // change to unexistent dir
    assert(stat(nodir.c_str(), &st) != 0);
    ret = chdir(nodir.c_str());
    if (ret == 0) {
        std::cerr << "ERROR: Succeeded on chdir to a non-existing dir" << std::endl;
        return EXIT_FAILURE;
    }
    if (errno != ENOENT) {
        std::cerr << "ERROR: wrong error number while opening non-existing file: " <<
            errno << std::endl;
        return EXIT_FAILURE;
    }

    // change to external dir
    ret = chdir(extdir.c_str());
    if (ret != 0) {
        std::cerr << "ERROR: Failed to chdir into external dir: " <<
            strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    // test path resolution: from outside to inside
    test_chdir("../mountdir/int", intdir);

    // test path resolution: from inside to outside
    test_chdir("../../ext.tmp", extdir);

    // test complex path resolution
    test_chdir(mountdir + "/int/..//./int//../../../tmp/mountdir/../mountdir/./int/.//", intdir);

    // Create external link
    ret = symlink(extdir.c_str(), ext_linkdir.c_str());
    if (ret != 0) {
        std::cerr << "ERROR: Failed to make symbolic link: " <<
            strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    test_chdir("../../link.tmp", extdir);

    test_chdir("../link.tmp/../link.tmp/../mountdir/int", intdir);
}
