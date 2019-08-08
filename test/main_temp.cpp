//
// Created by evie on 1/16/18.
//

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <cstring>
#include <sys/statfs.h>

using namespace std;

int main(int argc, char* argv[]) {

//    auto path = "/tmp/mountdir/test";
    auto path = "/tmp/testing/test";

    auto fd = creat(path, 0667);
    struct stat mystat{};
    fstat(fd, &mystat);
    struct statfs sfs{};
    auto ret = statfs(path, &sfs);
    cout << ret << " errno:" << errno << endl;












//    char buf[] = "lefthyblubber";
//    char buf1[] = "rebbulbyhtfellefthyblubber";
//
//    auto fd = creat(path, 0677);
//    auto fd_dup = dup2(fd,33);
//    struct stat mystat{};
//    fstat(fd, &mystat);
//    auto nw = write(fd, &buf, strlen(buf));
//    fstat(fd_dup, &mystat);
//    close(fd);
//    auto nw_dup = pwrite(fd_dup, &buf1, strlen(buf1), 0);
//    fstat(fd_dup, &mystat);
//    close(fd_dup);
//    nw_dup = pwrite(fd_dup, &buf1, strlen(buf1), 0);

    return 0;
}