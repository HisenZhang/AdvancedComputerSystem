#pragma once

#include <algorithm>
#include <array>
#include <immintrin.h>
#include <numeric>
#include <vector>
#include <memory>

// SIMD FIR Filter reference: https://thewolfsound.com/fir-filter-with-simd/

// The number of floats that fit into an AVX register.
constexpr uint32_t AVX_FLOAT_COUNT = 8u;

struct alignas(AVX_FLOAT_COUNT * alignof(float)) avx_alignment_t {};

template <typename SampleType, size_t alignment = alignof(SampleType)>
struct FilterInput {
    static constexpr auto VECTOR_SIZE = alignment / alignof(SampleType);

    FilterInput(const std::vector<SampleType>& inputSignal, const std::vector<SampleType>& filter) {
        const auto minimalPaddedSize = inputSignal.size() + 2 * filter.size() - 2u;
        const auto alignedPaddedSize = VECTOR_SIZE * (highestMultipleOfNIn(minimalPaddedSize - 1u, VECTOR_SIZE) + 1u);
        inputLength = alignedPaddedSize;

        inputStorage.reset(new avx_alignment_t[inputLength / AVX_FLOAT_COUNT]);
        std::copy(inputSignal.begin(), inputSignal.end(), reinterpret_cast<float*>(inputStorage.get()) + filter.size() - 1u);
        x = reinterpret_cast<float*>(inputStorage.get());

        outputLength = inputSignal.size() + filter.size() - 1u;
        outputStorage.resize(outputLength);
        
        filterLength = VECTOR_SIZE * (highestMultipleOfNIn(filter.size() - 1u, VECTOR_SIZE) + 1u);
        reversedFilterCoefficientsStorage.resize(filterLength);
        
        std::reverse_copy(filter.begin(), filter.end(), reversedFilterCoefficientsStorage.begin());
        for (auto i = filter.size(); i < reversedFilterCoefficientsStorage.size(); ++i) {
            reversedFilterCoefficientsStorage[i] = 0.f;
        }
        
        c = reversedFilterCoefficientsStorage.data();
        filterLength = reversedFilterCoefficientsStorage.size();
        y = outputStorage.data();

        alignedStorageSize = reversedFilterCoefficientsStorage.size() + AVX_FLOAT_COUNT;
        for (auto k = 0u; k < AVX_FLOAT_COUNT; ++k) {
            alignedReversedFilterCoefficientsStorage[k].reset(new avx_alignment_t[alignedStorageSize / AVX_FLOAT_COUNT]);
            cAligned[k] = reinterpret_cast<SampleType*>(alignedReversedFilterCoefficientsStorage[k].get());
            
            for (auto i = 0u; i < k; ++i) {
                cAligned[k][i] = 0.f;
            }
            std::copy(reversedFilterCoefficientsStorage.begin(),
                      reversedFilterCoefficientsStorage.end(),
                      cAligned[k] + k);
            for (auto i = reversedFilterCoefficientsStorage.size() + k; i < alignedStorageSize; ++i) {
                cAligned[k][i] = 0.f;
            }
        }
    }

    std::vector<SampleType> output() {
        auto result = outputStorage;
        result.resize(outputLength);
        return result;
    }
    
    const SampleType* x;  // input signal
    size_t inputLength;
    const SampleType* c;  // reversed filter coefficients
    size_t filterLength;
    SampleType* y;  // output (filtered) signal
    size_t outputLength;
    size_t alignedStorageSize;
    SampleType* cAligned[AVX_FLOAT_COUNT];
 private:
    std::vector<float> reversedFilterCoefficientsStorage;
    std::vector<float> outputStorage;
    std::array<std::unique_ptr<avx_alignment_t[]>, AVX_FLOAT_COUNT> alignedReversedFilterCoefficientsStorage;
    std::unique_ptr<avx_alignment_t[]> inputStorage;
};


inline float* applyFirFilterSingle(const FilterInput<float>& input) {
    const auto* x = input.x;
    const auto* c = input.c;
    auto* y = input.y;
    
    for (auto i = 0u; i < input.outputLength; ++i) {
        y[i] = 0.f;
        for (auto j = 0u; j < input.filterLength; ++j) {
            y[i] += x[i + j] * c[j];
        }
    }

    return y;
}

float* applyFirFilterAVX_innerLoopVectorization(FilterInput<float>& input) {
    const auto* x = input.x;
    const auto* c = input.c;
    auto* y = input.y;
    
    // A fixed-size array to move the data from registers into
    std::array<float, AVX_FLOAT_COUNT> outStore;
    
    for (auto i = 0u; i < input.outputLength; ++i) {
        // Set a SIMD register to all zeros;
        // we will use it as an accumulator
        auto outChunk = _mm256_setzero_ps();
        
        // Note the increment
        for (auto j = 0u; j < input.filterLength; j += AVX_FLOAT_COUNT) {
            // Load the unaligned input signal data into a SIMD register
            auto xChunk = _mm256_loadu_ps(x + i + j);
            // Load the unaligned reversed filter coefficients 
            // into a SIMD register
            auto cChunk = _mm256_loadu_ps(c + j);
            
            // Multiply the both registers element-wise
            auto temp = _mm256_mul_ps(xChunk, cChunk);
            
            // Element-wise add to the accumulator
            outChunk = _mm256_add_ps(outChunk, temp);
        }
        
        // Transfer the contents of the accumulator 
        // to the output array
        _mm256_storeu_ps(outStore.data(), outChunk);
        
        // Sum the partial sums in the accumulator and assign to the output
        y[i] = std::accumulate(outStore.begin(), outStore.end(), 0.f);
    }
    
    return y;
}

float* applyFirFilterAVX_outerLoopVectorization(FilterInput<float>& input) {
    const auto* x = input.x;
    const auto* c = input.c;
    auto* y = input.y;
    
    // Note the increment
    for (auto i = 0u; i < input.outputLength; i += AVX_FLOAT_COUNT) {
        // Set 8 computed outputs initially to 0
        auto yChunk = _mm256_setzero_ps();
        
        for (auto j = 0u; j < input.filterLength; ++j) {
            // Load an 8-element vector from x into an AVX register
            auto xChunk = _mm256_loadu_ps(x + i + j);
            
            // Load c[j] filter coefficient into every variable
            // of an 8-element AVX register
            auto cChunk = _mm256_set1_ps(c[j]);
            
            // Element-wise multiplication
            auto temp = _mm256_mul_ps(xChunk, cChunk);
            
            // Add to the accumulators
            yChunk = _mm256_add_ps(yChunk, temp);
        }
        
        // Store 8 computed values in the result vector
        _mm256_storeu_ps(y + i, yChunk);
    }
    
    return y;
}

float* applyFirFilterAVX_outerInnerLoopVectorization(FilterInput<float>& input) {
  const auto* x = input.x;
  const auto* c = input.c;
  auto* y = input.y;

    // An 8-element array for transferring data from AVX registers
    std::array<float, AVX_FLOAT_COUNT> outStore;
    
    // 8 separate accumulators for each of the 8 computed outputs
    std::array<__m256, AVX_FLOAT_COUNT> outChunk;
    
    // Note the increment
    for (auto i = 0u; i < input.outputLength; i += AVX_FLOAT_COUNT) {
        // Initialize the accumulators to 0
        for (auto k = 0u; k < AVX_FLOAT_COUNT; ++k) {
            outChunk[k] = _mm256_setzero_ps();
        }
        
        // Note the increment
        for (auto j = 0u; j < input.filterLength; j += AVX_FLOAT_COUNT) {
            // Load filter coefficients into an AVX register
            auto cChunk = _mm256_loadu_ps(c + j);
            
            for (auto k = 0u; k < AVX_FLOAT_COUNT; ++k) {
                // Load the input samples into an AVX register
                auto xChunk = _mm256_loadu_ps(x + i + j + k);
                
                // Element-wise multiplication
                auto temp = _mm256_mul_ps(xChunk, cChunk);
                
                // Add to the dedicated accumulator
                outChunk[k] = _mm256_add_ps(outChunk[k], temp);
            }
        }
        
        // Summarize the accumulators
        for (auto k = 0u; k < AVX_FLOAT_COUNT; ++k) {
            // Transfer the data into the helper array
            _mm256_storeu_ps(outStore.data(), outChunk[k]);
            
            if (i + k < input.outputLength)
                // Final sum for each of the 8 outputs 
                // computed in 1 outer loop iteration
                y[i + k] = std::accumulate(outStore.begin(), outStore.end(), 0.f);
        }
    }
    
    return y;
}
