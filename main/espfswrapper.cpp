#include "espfswrapper.h"

#include <sys/stat.h>

int file_copy(const char *to, const char *from) {
    FILE *fd_to;
    FILE *fd_from;
    char buf[512];
    ssize_t nread;
    int saved_errno;

    fd_from = fopen(from, "rb");
    //fd_from = open(from, O_RDONLY);
    if (fd_from == NULL) return -1;

    fd_to = fopen(to, "wb");
    if (fd_to == NULL) goto out_error;

    while (nread = fread(buf, 1, sizeof(buf), fd_from), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = fwrite(out_ptr, 1, nread, fd_to);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR) goto out_error;
        } while (nread > 0);
    }

    if (nread == 0) {
        if (fclose(fd_to) < 0) {
            fd_to = NULL;
            goto out_error;
        }
        fclose(fd_from);

        // Success!
        return 0;
    }

  out_error:
    saved_errno = errno;

    fclose(fd_from);
    if (fd_to) fclose(fd_to);

    errno = saved_errno;
    return -1;
}

bool file_exist(std::string filename) {
    struct stat st;
    return (stat(filename.c_str(), &st) == 0);
}

size_t file_size(std::string filename) {
    struct stat st;
    if (stat(filename.c_str(), &st) == 0)
        return st.st_size;

    return 0;
}
