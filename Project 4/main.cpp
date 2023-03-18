#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <thread>
#include <mutex>
#include <stdio.h>
#include <immintrin.h>
#include "trie.h"
#include "utils.h"
#include "ctpl_stl.h"

using namespace std;

#define CHUNK_SIZE 1000 // read chunk size in lines

unordered_map<string, int> dict;
unordered_map<int, string> dict_decode;
vector<int> encoded;
int entries = 0;
Trie tree;

mutex mtx;

void encoder_worker(int id, vector<string> *chunk)
{
    // cout << "Thread " << id << " running." <<endl;

    for (size_t i = 0; i < chunk->size(); i++)
    {
        bool isFresh = (dict.find((*chunk)[i]) == dict.end());

        mtx.lock();

        if (isFresh)
        {
            dict.insert({(*chunk)[i], entries});
            dict_decode.insert({entries, (*chunk)[i]});
            tree.insert((*chunk)[i], entries);
            entries++;
        }
        encoded.push_back(dict[(*chunk)[i]]);

        mtx.unlock();
    }

    delete chunk;
}

void encode(const char *fn, int nWorker)
{
    encoded.clear();
    dict.clear();
    dict_decode.clear();
    tree.clear();
    entries = 0;

    ctpl::thread_pool pool(nWorker <= 0 ? thread::hardware_concurrency() : nWorker);

    ifstream infile(fn);
    string line;
    vector<string> *chunk = new vector<string>;
    (*chunk).reserve(CHUNK_SIZE);
    while (getline(infile, line))
    {
        (*chunk).push_back(line);
        if ((*chunk).size() == CHUNK_SIZE)
        {
            pool.push(encoder_worker, chunk);
            chunk = new vector<string>;
            (*chunk).reserve(CHUNK_SIZE);
        }
    }
    infile.close();

    if (!(*chunk).empty())
    {
        pool.push(encoder_worker, chunk);
    }

    pool.stop(true);

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

    cout << "Loading completed. Encoded: " << encoded.size() << " entries. Total: " << dict.size() << " entries." << endl;
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
    cout << code << ":" << key << " @ ";
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
}

void prefixQuery()
{
    string data;
    cin >> data;
    vector<int> result;
    tree.prefix_search(data, result);
    for (const auto r : result)
    {
        find(dict_decode[r]);
    }
}

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " INFILE N_WORKER" << endl;
        return EXIT_SUCCESS;
    }

    char *buf;
    char opt = 0;

    cout << "===MENU===\n"
         << "0. Exit\n"
         << "1. Encode\n"
         << "2. Save\n"
         << "3. Load\n"
         << "4. Query\n"
         << "5. Prefix Query" << endl;
    cout << "> ";
    while (cin >> opt)
    {
        switch (opt)
        {
        case '0':
            exit(EXIT_SUCCESS);
            break;
        case '1':
            encode(argv[1], atoi(argv[2]));
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
        cout << "> ";
    }
    return EXIT_SUCCESS;
}
