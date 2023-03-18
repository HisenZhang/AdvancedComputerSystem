#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstring>

#include <chrono>
typedef std::chrono::high_resolution_clock Clock;

using namespace std;

void find(ifstream &infile, string &key)
{
    auto start = Clock::now();
    
    string buf;
    int line = 0;
    cout << key << " @ ";
    while (infile >> buf)
    {
        if (buf == key)
        {
            cout << line << " ";
        }
        line++;
    }
    cout << endl;

    auto end = Clock::now();
    auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "find " << int_ms.count() << " ms" << endl;
}

bool hasPrefix(string &word, string &prefix)
{
    for (size_t i = 0; i < prefix.length(); i++)
    {
        if (word[i] != prefix[i])
            return false;
    }
    return true;
}

void prefixFind(ifstream &infile, string &key)
{
    auto start = Clock::now();
    
    string buf;
    int line = 0;
    while (infile >> buf)
    {
        if (hasPrefix(buf, key))
        {
            cout << buf << " @ " << line << endl;
        }
        line++;
    }
    cout << endl;
    
    auto end = Clock::now();
    auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "prefix " << int_ms.count() << " ms" << endl;
}

int main(int argc, char const *argv[])
{
    if (argc < 4)
    {
        cout << "Usage: " << argv[0] << " INFILE [ FIND | PREFIX ] KEY" << endl;
        return EXIT_SUCCESS;
    }

    ifstream infile(argv[1]);
    string cmd = string(argv[2]);
    string key = string(argv[3]);

    transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "find")
        find(infile, key);
    else if (cmd == "prefix")
        prefixFind(infile, key);
    else
        cout << "Invalid command." << endl;

    return 0;
}
