//
#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>
#include <string.h>

static size_t read_file(FILE* fp, void** output) {
    size_t smart_size, count;
    size_t length = 0;
    //make it faster
    if (!fp) { //incase
        return 0;
    }
    else if (fp == stdin) {
        smart_size = stdin->_bufsiz;
    }
    else { //unreliable for stdin!
        struct stat filestats;
        int fd = fileno(fp);
        fstat(fd, &filestats);
        smart_size = filestats.st_size + 1; // +1 to get EOF, BIG file
    }
    //
    *output = calloc(1, 1); //just in case
    while (!feof(fp)) {
        *output = realloc(*output, length + smart_size + sizeof(wchar_t));
        count = fread(*output + length, 1, smart_size, fp);
        memset(*output + length + count, 0, sizeof(wchar_t)); // append 0, in case of wide char
        length += count;
    }
    *output = realloc(*output, length + sizeof(wchar_t));
    //
    return length;
}
