#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <stdio.h>
#include "trie.h"

using namespace std;

unordered_map<string, int> dict;
unordered_map<int, string> dict_decode;
vector<int> encoded;
int entries = 0;
Trie tree;

template <typename T>
int vectorWrite(vector<T> vec, char fn[])
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

void encode(char fn[])
{
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

    cout << "Encoding completed. Total: " << encoded.size() << " entries. Encoded: " << dict.size() << " entries." << endl;
}

void save()
{
    // output using fwrite to reduce overhead
    vectorWrite(encoded, "../data/encoded.bin");

    cout << "Encoded data saved." << endl;

    ofstream outdict("../data/dict.txt");
    for (const auto &[key, value] : dict)
    {
        outdict << value << ":" << key << '\n';
    }
    outdict.close();

    cout << "Encoded dictionary saved." << endl;
}

bool query()
{
    string data;
    cin >> data;
    if (dict.find(data) == dict.end())
    {
        cout << "Data not found." << endl;
        return false;
    }
    int code = dict[data];
    cout << "Data at column location(s): ";
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

void prefixQuery()
{
    string data;
    cin >> data;
    vector<int> result;
    tree.prefix_search(data,&result);
    for(const auto r: result){
        cout << r << ":" << dict_decode[r]<< endl;
    }
}

int main(int argc, char const *argv[])
{
    tree = Trie();

    char *buf;
    char opt = 0;

    puts("===MENU===\n0. Exit\n1. Encode\n2. Save\n3. Query\n4. Prefix Query");
    puts("> ");
    while (cin >> opt)
    {
        switch (opt)
        {
        case '0':
            exit(EXIT_SUCCESS);
            break;
        case '1':
            encode("../data/100k.txt");
            break;
        case '2':
            save();
            break;
        case '3':
            query();
            break;
        case '4':
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
