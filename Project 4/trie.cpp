#include "trie.h"

using namespace std;

struct TrieNode *Trie::getNode(void)
{
    struct TrieNode *pNode = new TrieNode;

    pNode->data = -1;
    pNode->isEndOfWord = false;

    for (int i = 0; i < ALPHABET_SIZE; i++)
        pNode->children[i] = nullptr;

    return pNode;
}

// If not present, inserts key into trie
// If the key is prefix of trie node, just
// marks leaf node
void Trie::insert(string &key, int data)
{
    struct TrieNode *pCrawl = root;

    for (int i = 0; i < key.length(); i++)
    {
        int index = key[i] - 'a';
        if (index < 0 || index >= ALPHABET_SIZE)
        {
            cout << "Bad input." << endl;
            abort();
        }
        if (!pCrawl->children[index])
            pCrawl->children[index] = getNode();

        pCrawl = pCrawl->children[index];
    }

    // mark last node as leaf
    pCrawl->isEndOfWord = true;
    pCrawl->data = data;
}

// Returns true if key presents in trie, else
// false
bool Trie::search(string &key, int *data)
{
    struct TrieNode *pCrawl = root;

    for (int i = 0; i < key.length(); i++)
    {
        int index = key[i] - 'a';
        if (index < 0 || index >= ALPHABET_SIZE)
        {
            cout << "Bad input." << endl;
            abort();
        }
        if (!pCrawl->children[index])
        {
            *data = -1;
            return false;
        }

        pCrawl = pCrawl->children[index];
    }
    *data = pCrawl->data;
    return (pCrawl->isEndOfWord);
}

void Trie::prefix_search(string &prefix, vector<int> &result)
{
    struct TrieNode *pCrawl = root;

    for (int i = 0; i < prefix.length(); i++)
    {
        int index = prefix[i] - 'a';
        if (!pCrawl->children[index])
            return;
        pCrawl = pCrawl->children[index];
    }

    // No prefix match
    if (pCrawl == root)
    {
        return;
    }

    walk(pCrawl, result);
}

void Trie::walk(struct TrieNode *pCrawl, vector<int> &result)
{

    if (pCrawl->isEndOfWord)
    {
        result.push_back(pCrawl->data);
    }

    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (pCrawl->children[i])
        {
            walk(pCrawl->children[i], result);
        }
    }
}

Trie::Trie()
{
    root = getNode();
}

void recursiveFree(struct TrieNode *node)
{
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (node->children[i] != nullptr)
        {
            recursiveFree(node->children[i]);
        }
    }
    delete node;
}

void Trie::clear()
{
    recursiveFree(root);
    root = getNode();
}

Trie::~Trie()
{
    recursiveFree(root);
}