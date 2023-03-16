#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <stdio.h>

using namespace std;

int main(int argc, char const *argv[])
{
    unordered_map<string, int> index;
    vector<int> encoded;
    int entries = 0;

    ifstream infile("../data/Column.txt");

    string buf;
    while (infile >> buf)
    {
        // cout << buf << endl;
        if (index.find(buf) == index.end())
        {
            index.insert({buf, entries++});
        }
        encoded.push_back(index[buf]);
    }

    cout << "Encoding completed: " << encoded.size() << " entries." << endl;

    // output using fwrite to reduce overhead

    FILE *outdata = fopen("../data/encoded.bin", "wb");
    if (outdata == NULL)
    {
        perror("Error opening the file.\n");
        return -1;
    }
    fwrite(&encoded[0], sizeof(int), encoded.size(), outdata);
    fclose(outdata);

    cout << "Encoded data saved." << endl;

    ofstream outdict("../data/dict.txt");
    for (const auto &[key, value] : index)
    {
        outdict << value << ":" << key << '\n';
    }
    outdict.close();

    return 0;
}
