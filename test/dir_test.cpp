/* Test directories functionalities
 *
 */

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cerrno>
#include <cassert>
#include <unordered_map>


int main(int argc, char* argv[]) {

    /**
        /tmp/mountdir
        ├── top_plus
        └── top
            ├── dir_a
            |   └── subdir_a
            ├── dir_b
            └── file_a
    */
    const std::string mntdir = "/tmp/mountdir";
    const std::string nonexisting = mntdir + "/nonexisting";
    const std::string topdir = mntdir + "/top";
    const std::string longer = topdir + "_plus";
    const std::string dir_a  = topdir + "/dir_a";
    const std::string dir_b  = topdir + "/dir_b";
    const std::string file_a = topdir + "/file_a";
    const std::string subdir_a  = dir_a + "/subdir_a";


    int ret;
    int fd;
    DIR * dirstream = NULL;
    struct stat dirstat;

    // Open nonexistsing directory
    dirstream = opendir(nonexisting.c_str());
    if(dirstream != NULL || errno != ENOENT){
        std::cerr << "ERROR: succeeded on opening nonexisting dir: " << std::strerror(errno) << std::endl;
        return -1;
    }

    // Stat nonexisting directory
    ret = stat(nonexisting.c_str(), &dirstat);
    if(ret == 0 || errno != ENOENT){
        std::cerr << "Error stating nonexisitng directory: " << std::strerror(errno) << std::endl;
        return -1;
    }

    // Remove nonexisting directory
    ret = rmdir(nonexisting.c_str());
    if (ret == 0) {
        std::cerr << "Succeded on removing nonexisitng directory" << std::endl;
        return EXIT_FAILURE;
    }
    if (errno != ENOENT) {     
        std::cerr << "Wrong error number on removing nonexisitng directory: " << std::strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    // Close nonexisting directory
    ret = closedir(NULL);
    if(ret != -1 || errno != EINVAL){
        std::cerr << "Error closing nonexisting directory: " << std::strerror(errno) << std::endl;
        return -1;
    }

    //Create topdir
    ret = mkdir(topdir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if(ret != 0){
        std::cerr << "Error creating topdir: " << std::strerror(errno) << std::endl;
        return -1;
    }

    //Test stat on existing dir
    ret = stat(topdir.c_str(), &dirstat);
    if(ret != 0){
        std::cerr << "Error stating topdir: " << std::strerror(errno) << std::endl;
        return -1;
    }
    assert(S_ISDIR(dirstat.st_mode));

    // Open topdir
    fd = open(topdir.c_str(), O_DIRECTORY);
    if(ret != 0){
        std::cerr << "Error opening topdir: " << std::strerror(errno) << std::endl;
        return -1;
    }

    // Read and write should be impossible on directories
    char buff;
    ret = read(fd, &buff, 1);
    if (ret == 0) {
        std::cerr << "ERROR: succeded on reading directory" << std::endl;
        return -1;
    }
    if (errno != EISDIR) {
        std::cerr << "ERROR: wrong error number on directory read" << std::endl;
        return -1;
    }
    ret = write(fd, &buff, 1);
    if (ret == 0) {
        std::cerr << "ERROR: succeded on reading directory" << std::endl;
        return -1;
    }
    if (errno != EISDIR) {
        std::cerr << "ERROR: wrong error number on directory read" << std::endl;
        return -1;
    }

    /* Read top directory that is empty */
    //opening top directory
    dirstream = opendir(topdir.c_str());
    if(dirstream == NULL){
        std::cerr << "Error opening topdir: " << std::strerror(errno) << std::endl;
        return -1;
    }

    // Read empty directory
    errno = 0;
    struct dirent * d = readdir(dirstream);
    if(d == NULL && errno != 0){
        std::cerr << "Error reading topdir: " << std::strerror(errno) << std::endl;
        return -1;
    }
    if(closedir(dirstream) != 0){
        std::cerr << "Error closing topdir" << std::strerror(errno) << std::endl;
        return -1;
    }


    /* Populate top directory */

    std::unordered_map<std::string, bool> expected_dirents = {
        {"dir_a", true},
        {"dir_b", true},
        {"file_a", false}
    };

    for(auto f: expected_dirents){
        auto complete_name = topdir + "/" + f.first;
        if(f.second){
            // directory
            ret = mkdir(complete_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
            assert(ret == 0);
        }else{
            // regular file
            ret = creat(complete_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
            assert(ret != -1);
            ret = close(ret);
            assert(ret == 0);
        }
    }

    //create directory with the same prefix of topdir but with longer name
    ret = mkdir(longer.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    assert(ret == 0);
    //create sub directory at level 2 that must not be included in readdir
    ret = mkdir(subdir_a.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    assert(ret == 0);


    /* Read top directory that has been populated */

    //opening top directory
    dirstream = opendir(topdir.c_str());
    if(dirstream == NULL){
        std::cerr << "Error opening topdir: " << std::strerror(errno) << std::endl;
        return -1;
    }

    std::unordered_map<std::string, bool> found_dirents;

    while((d = readdir(dirstream)) != NULL){
        found_dirents.insert(std::make_pair(d->d_name, (d->d_type == DT_DIR)));
    }
    assert(found_dirents  == expected_dirents);

    // Remove file through rmdir should reise error
    ret = rmdir(file_a.c_str());
    if (ret == 0) {
        std::cerr << "ERROR: Succeded on removing file through rmdir function" << std::endl;
        return EXIT_FAILURE;
    }
    if (errno != ENOTDIR) {     
        std::cerr << "ERROR: Wrong error number on removing file through rmdir function: " << std::strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    if(closedir(dirstream) != 0){
        std::cerr << "Error closing topdir" << std::strerror(errno) << std::endl;
        return -1;
    }

    ret = rmdir(subdir_a.c_str());
    if(ret != 0){
        std::cerr << "Error removing subdirectory: " << std::strerror(errno) << std::endl;
        return -1;
    }

    dirstream = opendir(subdir_a.c_str());
    if(dirstream != NULL || errno != ENOENT){
        std::cerr << "Error: succede on opening removed directory: " << std::strerror(errno) << std::endl;
        return -1;
    }
}
