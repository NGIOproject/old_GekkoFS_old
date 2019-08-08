#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <memory>

using namespace std;

int main(int argc, char* argv[]) {

    string p = "/tmp/mountdir/file"s;
    /*
     * consider the following cases:
     * 1. Very first chunk has offset or not and is serviced by this node
     * 2. If offset, will still be only 1 chunk written (small IO): (offset + bulk_size <= CHUNKSIZE) ? bulk_size
     * 3. If no offset, will only be 1 chunk written (small IO): (bulk_size <= CHUNKSIZE) ? bulk_size
     * 4. Chunks between start and end chunk have size of the CHUNKSIZE
     * 5. Last chunk (if multiple chunks are written): Don't write CHUNKSIZE but chnk_size_left for this destination
     *    Last chunk can also happen if only one chunk is written. This is covered by 2 and 3.
     */
    // base chunks
    // set chunksize to 40
    // Single chunk size 40
    char buf_single[] = "1222222222222222222222222222222222222221";
    // Single chunk size 5
    char buf_single_short[] = "12221";
    // multiple chunks size 120
    char buf_multiple[] = "122222222222222222222222222222222222222221133333333333333333333333333333333333114444444444444444444444444444444444444444441";
    // multiple chunks end chunk half (100)
    char buf_multiple_not_aligned[] = "1222222222222222222222222222222222222221133333333333333333333333333333333333333114444444444444444441";

    // overwrite
    // single chunk size 40
    char buf_ow_single[] = "abbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbba";
    // sing chunk size short 5
    char buf_ow_single_short[] = "abbba";
    //multiple chunks size 80
    char buf_ow_multiple[] = "abbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbba";

    // do tests
    auto fd = open(p.c_str(), O_CREAT | O_WRONLY, 0777);
    auto nw = write(fd, &buf_single, strlen(buf_single));
    close(fd);
    remove(p.c_str());

    fd = open(p.c_str(), O_CREAT | O_WRONLY, 0777);
    nw = write(fd, &buf_multiple, strlen(buf_multiple));
    close(fd);

    char read_buf[] = "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999";
    fd = open(p.c_str(), O_RDONLY, 0777);
    auto rs = read(fd, &read_buf, strlen(buf_multiple));
    printf("buffer read: %s\n size: %lu", read_buf, rs);
    close(fd);

    return 0;

}