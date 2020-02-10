/* Simple Write/Read Test
 *
 * - open a file
 * - write some content
 * - close
 * - open the same file in read mode
 * - read the content
 * - check if the content match
 * - close
 */

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>

using namespace std;

int main(int argc, char* argv[]) {

    string mountdir = "/tmp/mountdir";
    string p = mountdir + "/file";
    char buffIn[] = "oops.";
    char *buffOut = new char[strlen(buffIn) + 1 + 20 ];
    int fd;
    int ret;
    struct stat st;

    fd = open((mountdir + "/nonexisting").c_str(), O_RDONLY);
    if(fd >= 0 ){
        cerr << "ERROR: Succeeded on opening non-existing file" << endl;
        return -1;
    }
    if(errno != ENOENT){
        cerr << "ERROR: wrong error number while opening non-existing file: " << errno << endl;
        return -1;
    }

    /* Access nonexisting file */
    ret = access(p.c_str(), F_OK);
    if(ret == 0){
        cerr << "ERROR: succeeded on accessing non-existing file" << endl;
        return -1;
    };

    if(errno != ENOENT){
        cerr << "ERROR: wrong error number while accessing non-existing file: " << errno << endl;
        return -1;
    }

    /* Stat nonexisting file */
    ret = stat(p.c_str(), &st);
    if(ret == 0){
        cerr << "ERROR: succeeded on stating non-existing file" << endl;
        return -1;
    };

    if(errno != ENOENT){
        cerr << "ERROR: wrong error number while staing non-existing file: " << errno << endl;
        return -1;
    }

    /* Write the file */

    fd = open(p.c_str(), O_WRONLY | O_CREAT, 0777);
    if(fd < 0){
        cerr << "Error opening file (write)" << endl;
        return -1;
    }
    auto nw = write(fd, buffIn, strlen(buffIn));
    if(nw != strlen(buffIn)){
        cerr << "Error writing file" << endl;
        return -1;
    }

    if(close(fd) != 0){
        cerr << "Error closing file" << endl;
        return -1;
    }

    /* Access existing file */
    ret = access(p.c_str(), F_OK);
    if(ret != 0){
        cerr << "ERROR: Failed to access file: "  << strerror(errno) << endl;
        return -1;
    };

    /* Check file size */
    ret = stat(p.c_str(), &st);
    if(ret != 0){
        cerr << "Error stating file: " << strerror(errno) << endl;
        return -1;
    };

    if(st.st_size != strlen(buffIn)){
        cerr << "Wrong file size after creation: " << st.st_size << endl;
        return -1;
    }


    /* Read the file back */

    fd = open(p.c_str(), O_RDONLY);
    if(fd < 0){
        cerr << "Error opening file (read)" << endl;
        return -1;
    }

    auto nr = read(fd, buffOut, strlen(buffIn));
    if(nr != strlen(buffIn)){
        cerr << "Error reading file" << endl;
        return -1;
    }

    nr = read(fd, buffOut, 1);
    if(nr != 0){
        cerr << "Error reading at end of file" << endl;
        return -1;
    }

    if(strncmp( buffIn, buffOut, strlen(buffIn)) != 0){
        cerr << "File content mismatch" << endl;
        return -1;
    }

    ret = close(fd);
    if(ret != 0){
        cerr << "Error closing file: " << strerror(errno) << endl;
        return -1;
    };

    /* Read beyond end of file */

    fd = open(p.c_str(), O_RDONLY);
    if(fd < 0){
        cerr << "Error opening file (read)" << endl;
        return -1;
    }

    nr = read(fd, buffOut, strlen(buffIn) + 20 );
    if(nr != strlen(buffIn)){
        cerr << "Error reading file" << endl;
        return -1;
    }

    nr = read(fd, buffOut, 1 );
    if(nr != 0){
        cerr << "Error reading at end of file" << endl;
        return -1;
    }

    /* Remove test file */
    ret = remove(p.c_str());
    if(ret != 0){
        cerr << "Error removing file: " << strerror(errno) << endl;
        return -1;
    };
}
