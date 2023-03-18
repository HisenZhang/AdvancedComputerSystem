#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>
#include <vector>
#include <iostream>
#include <sys/stat.h>

using namespace std;

template <typename T>
int vectorWrite(vector<T> &vec, const char fn[])
{
    FILE *outdata = fopen(fn, "wb");
    if (outdata == NULL)
    {
        perror("Error opening the file.\n");
        return -1;
    }
    fwrite(&vec[0], sizeof(T), vec.size(), outdata);
    fclose(outdata);
    return 0;
}

long getFileSize(const char *fn)
{
    struct stat stat_buf;
    int rc = stat(fn, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

template <typename T>
int vectorRead(vector<T> &vec, const char fn[])
{
    FILE *indata = fopen(fn, "rb");
    if (indata == NULL)
    {
        perror("Error opening the file.\n");
        return -1;
    }
    int count = getFileSize(fn) / sizeof(T);
    vec.reserve(count);
    int buf = 0;

    for (size_t i = 0; i < count; i++)
    {
        size_t result = fread(&buf, sizeof(T), 1, indata);
        if (result)
        {
            vec.emplace_back(buf);
        }
    }

    fclose(indata);
    return 0;
}

#endif