#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <mpi.h>

using namespace std;

using ns = chrono::nanoseconds;
using get_time = chrono::steady_clock;

int main(int argc, char* argv[]) {

//    auto filen = atoi(argv[1]);
    auto total_files = atoi(argv[1]);

    MPI_Init(NULL, NULL);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

//    auto total_files = filen * world_size;
    auto filen = total_files / world_size;

//    int filen = 3;

//    printf("Hello from rank %d\n", rank);
    MPI_Barrier(MPI_COMM_WORLD);

    auto start_t = get_time::now();
    auto end_tmp = start_t;
    auto progress_ind = filen / 10;
    int fd;
    for (int i = 0; i < filen; ++i) {
        string p = "/tmp/mountdir/file" + to_string(rank) + "_" + to_string(i);
        fd = creat(p.c_str(), 0666);
        if (i % progress_ind == 0) {
            end_tmp = get_time::now();
            auto diff_tmp = end_tmp - start_t;
            cout << "Rank " << rank << ":\t" << i << " files processed.\t " << (i / (progress_ind)) * 10 << "%\t"
                 << (i / (chrono::duration_cast<ns>(diff_tmp).count() / 1000000000.)) << " ops/sec" << endl;
        }
        close(fd);
    }

    auto end_t = get_time::now();
    auto diff = end_t - start_t;

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        auto diff_count = chrono::duration_cast<ns>(diff).count();
        cout << "\nFiles created in total: " << total_files << " with " << filen << " files per process" << endl;
        cout << diff_count << "ns\t" << (diff_count) / 1000000. << "ms" << endl;
        cout << total_files / ((diff_count) / 1000000000.) << " files per second" << endl;
    }

    MPI_Finalize();

//    cout << "done" << endl;
    return 0;

}