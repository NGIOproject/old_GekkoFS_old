#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>

using namespace std;

using ns = chrono::nanoseconds;
using get_time = chrono::steady_clock;

int main(int argc, char* argv[]) {

    auto filen = atoi(argv[1]);

//    cout << mkdir("/tmp/mountdir/bla", 0775) << endl;
//    auto buf = "BUFFERINO2";
//    struct stat attr;
//    cout << creat("/tmp/mountdir/creat.txt", 0666) << endl;
//    cout << creat("/tmp/mountdir/#test-dir.0/mdtest_tree.0/file.mdtest.0000000.0000000005", 0666) << endl;
//    cout << stat("/tmp/mountdir/creat.txt", &attr) << endl;
//    cout << unlink("/tmp/mountdir/creat.txt") << endl;


    auto start_t = get_time::now();
    int fd;
    for (int i = 0; i < filen; ++i) {
        string p = "/tmp/mountdir/file" + to_string(i);
        fd = creat(p.c_str(), 0666);
        if (i % 25000 == 0)
            cout << i << " files processed." << endl;
        close(fd);
    }

    auto end_t = get_time::now();
    auto diff = end_t - start_t;

    auto diff_count = chrono::duration_cast<ns>(diff).count();

    cout << diff_count << "ns\t" << (diff_count) / 1000000. << "ms" << endl;
    cout << filen / ((diff_count) / 1000000000.) << " files per second" << endl;

    return 0;

}