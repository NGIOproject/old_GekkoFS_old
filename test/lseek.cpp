#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <limits>

using namespace std;

int main(int argc, char* argv[]) {

    string mountdir = "/tmp/mountdir";
    string f = mountdir + "/file";
    int fd;

    fd = open(f.c_str(), O_WRONLY | O_CREAT, 0777);
    if(fd < 0){
        cerr << "Error opening file (write): " << strerror(errno) << endl;
        return -1;
    }
    off_t pos = static_cast<off_t>(numeric_limits<int>::max()) + 1;

    off_t ret = lseek(fd, pos, SEEK_SET);
    if(ret == -1) {
        cerr << "Error seeking file: " << strerror(errno) << endl;
        return -1;
    }

    if(ret != pos) {
        cerr << "Error seeking file: unexpected returned position " << ret << endl;
        return -1;
    }

    if(close(fd) != 0){
        cerr << "Error closing file" << endl;
        return -1;
    }

    /* Remove test file */
    ret = remove(f.c_str());
    if(ret != 0){
        cerr << "Error removing file: " << strerror(errno) << endl;
        return -1;
    };
}
