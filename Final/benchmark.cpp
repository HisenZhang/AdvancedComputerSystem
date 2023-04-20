#include "DSP.h"

#include <chrono>
typedef std::chrono::high_resolution_clock Clock;

using namespace std;

const std::vector<float> derivativeFilter = {
        -0.03926588615294601,
        -0.05553547436862413,
        -0.07674634732792313,
        -0.10359512127942656,
        -0.13653763745381112,
        -0.17563154711765214,
        -0.22037129189056356,
        -0.26953979617440665,
        -0.32110731416014476,
        -0.37220987669580885,
        -0.41923575145442643,
        -0.45803722114653117,
        -0.48426719093924325,
        -0.493817805141109  ,
        -0.483315193354812  ,
        -0.4506055551298345 ,
        -0.39515794257351333,
        -0.31831200150042305,
        -0.22331589501120253,
        -0.11512890376975546,
        0.0                 ,
        0.11512890376975546 ,
        0.22331589501120253 ,
        0.31831200150042305 ,
        0.39515794257351333 ,
        0.4506055551298345  ,
        0.483315193354812   ,
        0.493817805141109   ,
        0.48426719093924325 ,
        0.45803722114653117 ,
        0.41923575145442643 ,
        0.37220987669580885 ,
        0.32110731416014476 ,
        0.26953979617440665 ,
        0.22037129189056356 ,
        0.17563154711765214 ,
        0.13653763745381112 ,
        0.10359512127942656 ,
        0.07674634732792313 ,
        0.05553547436862413 ,
    };

void vanillaFilter(const FilterInput& input, std::vector<float>& outSignal)
{
  size_t n;

  for (n = 0; n < input.outputLength; n++)
  {
    size_t kmin, kmax, k;

    outSignal[n] = 0;

    kmin = (n >= input.filter.size() - 1) ? n - (input.filter.size() - 1) : 0;
    kmax = (n < input.signal.size() - 1) ? n : input.signal.size() - 1;

    for (k = kmin; k <= kmax; k++)
    {
      outSignal[n] += input.signal[k] * input.filter[n - k];
    }
  }
}


int main(int argc, char const *argv[])
{
    struct Signal test_signal;
    test_signal.sampleRate = 44100;
    GenerateSignal(SineGenerator,440,test_signal.sampleRate,atoi(argv[1]),test_signal.data);

    struct Signal test_out;
    test_out.sampleRate = 44100;

    struct FilterInput test_filter(test_signal.data,derivativeFilter);

    cout << "Test setup done." << endl;

    // AVX

    auto start = Clock::now();
    
    applyFirFilterAVX(test_filter,test_out.data);

    auto end = Clock::now();
    auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "SIMD " << int_ms.count() << " ms" << endl;

    // parallel

    start = Clock::now();

    applyFirFilterAVXParallel(test_filter,test_out.data);

    end = Clock::now();
    int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "threadpool " << int_ms.count() << " ms" << endl;

    // vanilla

    start = Clock::now();

    vanillaFilter(test_filter,test_out.data);

    end = Clock::now();
    int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "vanilla " << int_ms.count() << " ms" << endl;
    
    return EXIT_SUCCESS;
}
