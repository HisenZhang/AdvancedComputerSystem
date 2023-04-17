#pragma once

#ifdef _WINDOWS
#include <Windows.h>
#else
#endif

#include "AudioFile/AudioFile.h"

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

struct FilterInput // From https://thewolfsound.com/fir-filter-with-simd/
{
    FilterInput(const std::vector<float>& inSignal, const std::vector<float>& filter)
    {
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

struct WaveHeader // From https://stackoverflow.com/a/70991482/14865787
{
    /** waveFormatHeader: The first 4 bytes of a wav file should be the characters "RIFF" */
    char chunkID[4] = { 'R', 'I', 'F', 'F' };
    /** waveFormatHeader: This is the size of the entire file in bytes minus 8 bytes */
    uint32_t chunkSize;
    /** waveFormatHeader" The should be characters "WAVE" */
    char format[4] = { 'W', 'A', 'V', 'E' };
    /** waveFormatHeader" This should be the letters "fmt ", note the space character */
    char subChunk1ID[4] = { 'f', 'm', 't', ' ' };
    /** waveFormatHeader: For PCM == 16, since audioFormat == uint16_t */
    uint32_t subChunk1Size = 16;
    /** waveFormatHeader: For PCM this is 1, other values indicate compression */
    uint16_t audioFormat = 1;
    /** waveFormatHeader: Mono = 1, Stereo = 2, etc. */
    uint16_t numChannels = 1;
    /** waveFormatHeader: Sample Rate of file */
    uint32_t sampleRate = 44100;
    /** waveFormatHeader: SampleRate * NumChannels * BitsPerSample/8 */
    uint32_t byteRate = 44100 * 2;
    /** waveFormatHeader: The number of bytes for one sample including all channels */
    uint16_t blockAlign = 2;
    /** waveFormatHeader: 8 bits = 8, 16 bits = 16 */
    uint16_t bitsPerSample = 16;
    /** waveFormatHeader: Contains the letters "data" */
    char subChunk2ID[4] = { 'd', 'a', 't', 'a' };
    /** waveFormatHeader: == NumberOfFrames * NumChannels * BitsPerSample/8
     This is the number of bytes in the data.
     */
    uint32_t subChunk2Size;
    
    WaveHeader(uint32_t samplingFrequency, uint16_t bitDepth, uint16_t numberOfChannels, uint32_t numSamples)
    {
        numChannels = numberOfChannels;
        sampleRate = samplingFrequency;
        bitsPerSample = bitDepth;
        
        byteRate = sampleRate * numChannels * bitsPerSample / 8;
        blockAlign = numChannels * bitsPerSample / 8;

        subChunk2Size = numSamples * numChannels * bitsPerSample / 8;
        chunkSize = 36 + subChunk2Size;
    };
};

void PlayBufferAsAudio(float* audioBuffer, uint32_t numSamples, uint32_t samplingFrequency)
{
    // WinApi PlaySound requires the buffer in memory to have the wav file header
    char *wavBuffer = new char[sizeof(WaveHeader) + sizeof(int16_t)*numSamples];
    WaveHeader header(/*samplingFrequency=*/samplingFrequency, /*bitDepth=*/16, /*numChannels=*/1, numSamples);
    memcpy(wavBuffer, &header, sizeof(WaveHeader));
    
    int16_t *data = (int16_t*)(wavBuffer + sizeof(WaveHeader));
    constexpr float max16BitValue = 32768.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        int pcm = int(audioBuffer[i] * (max16BitValue));
        if (pcm >= max16BitValue) pcm = max16BitValue - 1;
        else if (pcm < -max16BitValue) pcm = -max16BitValue;
        data[i] = int16_t(pcm);
    }

#ifdef _WINDOWS
    //PlaySound((LPCSTR)filepath.c_str(), NULL, SND_FILENAME);
    //PlaySound(MAKEINTRESOURCE((char*)wavBuffer), NULL, SND_FILENAME);
    PlaySound(wavBuffer, NULL, SND_MEMORY | SND_ASYNC);
#else
    std::system((std::string("afplay ") + filepath).c_str());
#endif
    free(wavBuffer);
}

#define TAU 6.2831855

std::vector<float> GenerateSineWave(float frequency, float sampleRate, int numSamples)
{
    const float radsPerSamp = TAU * frequency / sampleRate;
	std::vector<float> result(numSamples);

    for (uint32_t i = 0; i < numSamples; i++)
    {
        result[i] = std::sin(radsPerSamp * (float) i);
    }

	return result;
}

void WriteSignal(const std::vector<float>& signal, const std::string& path) {
	std::vector<std::vector<float>> channels = { signal };

	AudioFile<float> af;
	af.setAudioBuffer(channels);
	af.save(path);
}
