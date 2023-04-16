#pragma once

#include <algorithm>
#include <array>
#include <immintrin.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

// The number of floats that fit into an AVX register.
constexpr uint32_t AVX_FLOAT_COUNT = 8u;

float highestMultipleOfNIn(float in, float N) {
    return static_cast<long long>(in / N);
}

struct FilterInput {
    FilterInput(const std::vector<float>& inSignal, const std::vector<float>& filter) {
        size_t alignment = 1u;
        const auto minimalPaddedSize = inSignal.size() + 2 * filter.size() - 2u;
        const auto alignedPaddedSize = alignment * (highestMultipleOfNIn(minimalPaddedSize - 1u, alignment) + 1u);

        const auto inputLength = alignedPaddedSize;
        this->signal.resize(inputLength, 0.f); // Pad input signal with zeros
        std::copy(inSignal.begin(), inSignal.end(), this->signal.begin() + filter.size() - 1u);

        outputLength = inSignal.size() + filter.size() - 1u;

        const auto filterLength = alignment * (highestMultipleOfNIn(filter.size() - 1u, alignment) + 1);
        this->filter.resize(filterLength);

        // Reverse and pad with zeros
        std::reverse_copy(filter.begin(), filter.end(), this->filter.begin());
        for (auto i = filter.size(); i < this->filter.size(); ++i) {
            this->filter[i] = 0.f;
        }
    }

    std::vector<float> signal;
    std::vector<float> filter;
    uint32_t outputLength;
};

void applyFirFilter(const FilterInput& input, std::vector<float>& outSignal) {
    outSignal.resize(input.outputLength);

    for (uint32_t i = 0; i < input.outputLength; ++i) {
        outSignal[i] = 0.0f;
        for (uint32_t j = 0; j < input.filter.size(); ++j) {
            outSignal[i] += input.signal[i + j] * input.filter[j];
        }
    }
}

void applyFirFilterAVX(const FilterInput& input, std::vector<float>& outSignal) {
    outSignal.resize(input.outputLength);

    std::array<float, AVX_FLOAT_COUNT> result;
    for (uint32_t i = 0; i < input.outputLength; ++i) {
        __m256 accumulator = _mm256_setzero_ps();
        
        for (uint32_t j = 0; j < input.filter.size(); j += AVX_FLOAT_COUNT) {
            __m256 signalLoad = _mm256_loadu_ps(input.signal.data() + i + j); // load signal and filter
            __m256 filterLoad = _mm256_loadu_ps(input.filter.data() + j);
            __m256 temp       = _mm256_mul_ps(signalLoad, filterLoad);        // element-wise multiply
            accumulator       = _mm256_add_ps(accumulator, temp);             // element-wise add
        }
        
        _mm256_storeu_ps(result.data(), accumulator); // move the accumulator into the result and sum
        outSignal[i] = std::accumulate(result.begin(), result.end(), 0.f);
    }
}
