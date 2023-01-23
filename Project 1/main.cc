#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <condition_variable>
#include "/home/michaelchen/zstd.h" 

const int BlockSize = 16 * 1024; //16KB
//const int NumThreads = ; // number of threads

std::mutex queue_mutex;
std::queue<std::pair<char*, int>> block_queue;
std::vector<std::thread> worker_threads;
std::ifstream in_file;
std::ofstream out_file;
std::condition_variable cv;
int currentBlock = 0; // index of the current block being processed
bool finished = false; // flag to indicate if all blocks have been processed

void CompressBlock(int thread_id) {
  std::unique_lock<std::mutex> lock(queue_mutex);
  while (true) {
    std::pair<char*, int> block;
    cv.wait(lock, []{return currentBlock < worker_threads.size() || finished;});
    {
      
      if (block_queue.empty()) {
        break;
      }
      block = block_queue.front();
      block_queue.pop();
    }

    size_t compressed_size = ZSTD_compressBound(block.second);
    char* compressed_block = new char[compressed_size];

    compressed_size = ZSTD_compress(compressed_block, compressed_size, block.first, block.second, 1);
    if (ZSTD_isError(compressed_size)) {
      std::cerr << "Error: " << ZSTD_getErrorName(compressed_size) << std::endl;
      exit(1);
    }

    std::unique_lock<std::mutex> lock(queue_mutex);
    out_file.write(compressed_block, compressed_size);
    //lock.lock();
    //lock.unlock();
    //cv.notify_all();

    delete[] compressed_block;
    delete[] block.first;
  }
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Error: Invalid number of arguments. Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
    return 1;
  }
//open input file
  in_file.open(argv[1], std::ios::binary);
  if (!in_file.is_open()) {
    std::cerr << "Error: Failed to open input file: " << argv[1] << std::endl;
    return 1;
  }
//open output file
  out_file.open(argv[2], std::ios::binary);
  if (!out_file.is_open()) {
    std::cerr << "Error: Failed to open output file: " << argv[2] << std::endl;
    return 1;
  }

//multi threads
/*
  for (int i = 0; i < NumThreads; ++i) {
    worker_threads.emplace_back(CompressBlock, i);
  }
*/

  char* buffer = new char[BlockSize];          //NOT SURE FOR THIS PART, PLZ DOUBLE CHECK------------------------------------------//
  while (in_file.read(buffer, BlockSize)) {
    std::unique_lock<std::mutex> lock(queue_mutex);
    int size = in_file.gcount();


    cv.wait(lock, []{return currentBlock == worker_threads.size();});
    lock.unlock();
    cv.notify_all();
    }
  delete[] buffer;      //-----------------------------------------------------------------------------------------------------------

//wait for work thread to finish
/*
*/


//close files
  in_file.close();
  out_file.close();

  return 0;
  
}