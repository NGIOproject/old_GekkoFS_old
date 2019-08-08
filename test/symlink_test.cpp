/* Test fs functionality involving links */

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <sys/stat.h>

int main(int argc, char* argv[]) {

    const std::string mountdir = "/tmp/mountdir";
    const std::string dir_int = mountdir + "/dir";
    const std::string dir_ext = "/tmp/dir";
    const std::string target_int = dir_int + "/target";
    const std::string target_ext = dir_ext + "/target";
    const std::string link_int = dir_int + "/link";
    const std::string link_ext = dir_ext + "/tmp/link";

    char buffIn[] = "oops.";
    char *buffOut = new char[strlen(buffIn) + 1];
    
    struct stat st;
    int ret;
    int fd;

    // Clean external dir
    ret = rmdir(dir_ext.c_str());
    if (ret != 0) {
        if (errno != ENOENT) {
            std::cerr << "ERROR: cannot remove internal dir: " << strerror(errno) << std::endl;
            return -1;
        }
    }
    ret = mkdir(dir_ext.c_str(), 0770);
    if (ret != 0) {
        std::cerr << "ERROR: cannot create external dir: " << strerror(errno) << std::endl;
        return -1;
    }
    
    
    // Clean internal dir
    ret = rmdir(dir_int.c_str());
    if (ret != 0) {
        if (errno != ENOENT) {
            std::cerr << "ERROR: cannot remove internal dir: " << strerror(errno) << std::endl;
            return -1;
        }
    }
    ret = mkdir(dir_int.c_str(), 0770);
    if (ret != 0) {
        std::cerr << "ERROR: cannot create internal dir: " << strerror(errno) << std::endl;
        return -1;
    }


    // Create link to directory: NOT SUPPORTED
    ret = symlink(dir_int.c_str(), link_int.c_str());
    if (ret != -1) {
        std::cerr << "ERROR: Succeeded on creating link to directory" << std::endl;
        return -1;
    }
    if(errno != ENOTSUP){
        std::cerr << "ERROR: wrong error number on link to directory: " << errno << std::endl;
        return -1;
    }
    assert(lstat(link_int.c_str(), &st) != 0 && errno == ENOENT);


    // Create link from inside to outside: NOT SUPPORTED
    ret = symlink(target_ext.c_str(), link_int.c_str());
    if (ret != -1) {
        std::cerr << "ERROR: Succeeded on creating link to outside" << std::endl;
        return -1;
    }
    if(errno != ENOTSUP){
        std::cerr << "ERROR: wrong error number on link to outside: " << errno << std::endl;
        return -1;
    }
    assert(lstat(link_int.c_str(), &st) != 0 && errno == ENOENT);
    

    // Create link from outside to inside: NOT SUPPORTED
    ret = symlink(target_int.c_str(), link_ext.c_str());
    if (ret != -1) {
        std::cerr << "ERROR: Succeeded on creating link from outside" << std::endl;
        return -1;
    }
    if(errno != ENOTSUP){
        std::cerr << "ERROR: wrong error number on link from outside: " << errno << std::endl;
        return -1;
    }
    assert(lstat(link_ext.c_str(), &st) != 0 && errno == ENOENT);

    
    // Create regular link
    ret = symlink(target_int.c_str(), link_int.c_str());
    if (ret < 0) {
        std::cerr << "ERROR: Failed to create link: " << strerror(errno) << std::endl;
        return -1;
    }
    
    // Check link stat
    ret = lstat(link_int.c_str(), &st);
    if (ret != 0) {
        std::cerr << "ERROR: Failed to stat link:" << strerror(errno) << std::endl;
        return -1;
    }
    // Check link mode
    if (!S_ISLNK(st.st_mode)) {
        std::cerr << "ERROR: Link has wrong file type" << std::endl;
        return -1;
    }
    // Check link size 
    if (st.st_size != target_int.size()) {
        std::cerr << "ERROR: Link has wrong size" << std::endl;
        return -1;
    }


    // Check readlink
    char* target_path = new char[target_int.size() + 1];
    ret = readlink(link_int.c_str(), target_path, target_int.size() + 1);
    if (ret <= 0) {
        std::cerr << "ERROR: Failed to retrieve link path: " << strerror(errno) << std::endl;
        return -1;
    }
    // Check return value, should be the length of target path
    if (ret != target_int.size()) {
        std::cerr << "ERROR: readlink returned unexpected value: " << ret << std::endl;
        return -1;
    }
    // Check returned string
    if (std::string(target_path) != target_int) {
        std::cerr << "ERROR: readlink returned unexpected target path: " << std::string(target_path) << std::endl;
        return -1;
    }

    // Overwrite link
    fd = symlink(target_int.c_str(), link_int.c_str());
    if (fd == 0) {
        std::cerr << "ERROR: Succeed on overwriting link" << std::endl;
        return -1;
    }
    if(errno != EEXIST){
        std::cerr << "ERROR: wrong error number on overwriting symlink" << errno << std::endl;
        return -1;
    }
   
    // Check target stat
    ret = stat(link_int.c_str(), &st);
    if (ret != -1) {
        std::cerr << "ERROR: Succeed on stating unexistent target through link" << std::endl;
        return -1;
    }
    if(errno != ENOENT){
        std::cerr << "ERROR: wrong error number on stating unexistent target through link" << std::endl;
        return -1;
    }


    /* Write on link */
    fd = open(link_int.c_str(), O_WRONLY | O_CREAT, 0770);
    if(fd < 0){
        std::cerr << "ERROR: opening target for write" << strerror(errno) << std::endl;
        return -1;
    }
    auto nw = write(fd, buffIn, strlen(buffIn));
    if(nw != strlen(buffIn)){
        std::cerr << "ERROR: writing target" << strerror(errno) << std::endl;
        return -1;
    }
    if(close(fd) != 0){
        std::cerr << "ERROR: closing target" << strerror(errno) << std::endl;
        return -1;
    }

    
    // Check target stat through link
    ret = stat(link_int.c_str(), &st);
    if (ret != 0) {
        std::cerr << "ERROR: Failed to stat target through link: " << strerror(errno) << std::endl;
        return -1;
    }
    // Check link mode
    if (!S_ISREG(st.st_mode)) {
        std::cerr << "ERROR: Target has wrong file type" << std::endl;
        return -1;
    }
    // Check link size 
    if (st.st_size != strlen(buffIn)) {
        std::cerr << "ERROR: Link has wrong size" << std::endl;
        return -1;
    }


    /* Read the link back */
    fd = open(link_int.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "ERROR: opening link (read): " << strerror(errno) << std::endl;
        return -1;
    }
    auto nr = read(fd, buffOut, strlen(buffIn) + 1);
    if (nr != strlen(buffIn)) {
        std::cerr << "ERROR: reading link" << strerror(errno) << std::endl;
        return -1;
    }
    if (strncmp(buffIn, buffOut, strlen(buffIn)) != 0) {
        std::cerr << "ERROR: File content mismatch" << std::endl;
        return -1;
    }
    ret = close(fd);
    if (ret != 0) {
        std::cerr << "ERROR: Error closing link: " << strerror(errno) << std::endl;
        return -1;
    };


    /* Remove link */
    ret = unlink(link_int.c_str());
    if (ret != 0) {
        std::cerr << "Error removing link: " << strerror(errno) << std::endl;
        return -1;
    };

    assert((lstat(link_int.c_str(), &st) == -1) && (errno == ENOENT)); 
    assert((stat(link_int.c_str(), &st) == -1) && (errno == ENOENT)); 
    
    /* Remove target */
    ret = unlink(target_int.c_str());
    if (ret != 0) {
        std::cerr << "Error removing link: " << strerror(errno) << std::endl;
        return -1;
    };


    // Clean test working directories
    ret = rmdir(dir_int.c_str());
    if (ret != 0) {
        std::cerr << "ERROR: cannot remove internal dir: " << strerror(errno) << std::endl;
        return -1;
    }
    ret = rmdir(dir_ext.c_str());
    if (ret != 0) {
        std::cerr << "ERROR: cannot remove internal dir: " << strerror(errno) << std::endl;
        return -1;
    }
}
