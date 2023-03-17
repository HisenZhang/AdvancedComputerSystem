#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include <immintrin.h>
#include "trie.h"

using namespace std;

unordered_map<string, int> dict;
unordered_map<int, string> dict_decode;
vector<int> encoded;
int entries = 0;
Trie tree;

template <typename T>
int vectorWrite(vector<T> &vec, char fn[])
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

long getFileSize(char *fn)
{
    struct stat stat_buf;
    int rc = stat(fn, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

template <typename T>
int vectorRead(vector<T> &vec, char *fn)
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
        fread(&buf, sizeof(T), 1, indata);
        vec.emplace_back(buf);
    }

    fclose(indata);
    return 0;
}

void encode(const char *fn)
{
    encoded.clear();
    dict.clear();
    dict_decode.clear();
    tree.clear();
    entries = 0;

    ifstream infile(fn);
    string buf;
    while (infile >> buf)
    {
        // cout << buf << endl;
        if (dict.find(buf) == dict.end())
        {
            dict.insert({buf, entries});
            dict_decode.insert({entries, buf});
            tree.insert(buf, entries);
            entries++;
        }
        encoded.push_back(dict[buf]);
    }

    infile.close();
    cout << "Encoding completed. Input: " << encoded.size() << " entries. Encoded: " << dict.size() << " entries." << endl;
}

void load()
{
    encoded.clear();
    dict.clear();
    dict_decode.clear();
    tree.clear();
    entries = 0;

    vectorRead(encoded, "../data/encoded.bin");

    ifstream indict("../data/dict.txt");
    string key;
    int value;
    while (indict >> key >> value)
    {
        // cout << key << value<<endl;
        dict.insert({key, value});
        dict_decode.insert({value, key});
        tree.insert(key, value);
    }
    indict.close();
}

void save()
{
    // output using fwrite to reduce overhead
    vectorWrite(encoded, "../data/encoded.bin");

    cout << "Encoded data saved." << endl;

    ofstream outdict("../data/dict.txt");
    for (const auto &[key, value] : dict)
    {
        outdict << key << ' ' << value << '\n';
    }
    outdict.close();

    cout << "Encoded dictionary saved." << endl;
}

#ifndef SIMD

bool find(string key)
{
    if (dict.find(key) == dict.end())
    {
        cout << "Key not found." << endl;
        return false;
    }
    int code = dict[key];
    cout << "@ ";
    for (size_t i = 0; i < encoded.size(); i++)
    {
        if (code == encoded[i])
        {
            cout << i << " ";
        }
    }
    cout << endl;
    return true;
}

#else

bool find(string key)
{
    if (dict.find(key) == dict.end())
    {
        cout << "Key not found." << endl;
        return false;
    }
    int code = dict[key];
    cout << code << ":" << key << " @ ";

    vector<int> loc;
    int mask;
    int total;
    char *base = (char *)(&encoded[0]);
    char *end = base + (encoded.size() * sizeof(int));
    char *dataptr = base;
    // assert(size % 16 == 0);

    __m256i tocmp = _mm256_set1_epi32(code);
    while (dataptr < end)
    {
        __m256i chunk = _mm256_loadu_si256((__m256i const *)dataptr);
        __m256i results = _mm256_cmpeq_epi32(chunk, tocmp);
        mask = _mm256_movemask_epi8(results);

        if (mask)
        {
            for (size_t i = 0; i < 8; i++)
            {
                if (mask >> (4 * i) & 0xf == 0xf)
                    loc.push_back(((dataptr + sizeof(int) * i) - base) / sizeof(int));
            }
        }
        dataptr += 8 * sizeof(int);
    }

    for (size_t i = 0; i < loc.size(); i++)
    {
        cout << loc[i] << ' ';
    }
    cout << endl;

    return true;
}

#endif

bool query()
{
    string key;
    cin >> key;
    return find(key);
    // return find(key);
}

void prefixQuery()
{
    string data;
    cin >> data;
    vector<int> result;
    tree.prefix_search(data, result);
    for (const auto r : result)
    {
        cout << r << ":" << dict_decode[r] << " ";
        find(dict_decode[r]);
    }
}

int main(int argc, char const *argv[])
{
    char *buf;
    char opt = 0;

    puts("===MENU===\n0. Exit\n1. Encode\n2. Save\n3. Load\n4. Query\n5. Prefix Query");
    puts("> ");
    while (cin >> opt)
    {
        switch (opt)
        {
        case '0':
            exit(EXIT_SUCCESS);
            break;
        case '1':
            encode(argv[1]);
            break;
        case '2':
            save();
            break;
        case '3':
            load();
            break;
        case '4':
            query();
            break;
        case '5':
            prefixQuery();
            break;
        default:
            puts("Invalid option.");
            break;
        }
        puts("> ");
    }
    return 0;
}
