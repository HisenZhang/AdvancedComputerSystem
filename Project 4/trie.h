#ifndef _TRIE_
#define _TRIE_

#include <string>
#include <vector>
#include <iostream>

// modified from implementation:
// https://www.geeksforgeeks.org/trie-insert-and-search/

#define ALPHABET_SIZE 26

struct TrieNode 
{ 
    int data;
    struct TrieNode *children[ALPHABET_SIZE];
    // isEndOfWord is true if the node 
    // represents end of a word 
    bool isEndOfWord; 
}; 

class Trie
{
private:
    struct TrieNode *root;
    void walk(struct TrieNode *pCrawl,std::vector<int> *result);
public:
    Trie();
    ~Trie();
    struct TrieNode *getNode(void);
    void insert(std::string key, int data);
    bool search(std::string key, int* data);
    void prefix_search(std::string prefix,std::vector<int> *result);
};


#endif