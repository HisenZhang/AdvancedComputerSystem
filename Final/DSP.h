#pragma once

#ifdef _WINDOWS
#include <Windows.h>
#else
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot/implot.h"
#include "imspinner/imspinner.h"
#define GL_SILENCE_DEPRECATION
#include "GLFW/glfw3.h" // Will drag system OpenGL headers
#include "AudioFile/AudioFile.h"
#include "nfd.h"

#ifndef _WINDOWS
#include "pulse/pulseaudio.h"
#include "pulse/simple.h"
#include "pulse/error.h"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <future>
#include <immintrin.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>

#define TAU 6.2831855

std::atomic<bool> bFilterThreadRunning { false };

// The number of floats that fit into an AVX register.
constexpr uint32_t AVX_FLOAT_COUNT = 8u;

#ifndef _WINDOWS
std::atomic<bool> bPlaybackAudioAsync;
#endif

struct Signal
{
    std::vector<float> data;
    uint32_t sampleRate;
};

struct FilterInput // From https://thewolfsound.com/fir-filter-with-simd/
{
    FilterInput(const std::vector<float>& inSignal, const std::vector<float>& inFilter)
    {
        // Pad beginning of signal with filter.size() many zeros
        const auto inputLength = inSignal.size() + 2 * inFilter.size() - 2u;
        this->signal.resize(inputLength, 0.f);
        std::copy(inSignal.begin(), inSignal.end(), this->signal.begin() + inFilter.size() - 1u);

        outputLength = inSignal.size() + inFilter.size() - 1u;

        // Copy the filter in reverse for the convolution
        this->filter.resize(inFilter.size(), 0.0f);
        std::reverse_copy(inFilter.begin(), inFilter.end(), this->filter.begin());
    }

    std::vector<float> signal;
    std::vector<float> filter;
    uint32_t outputLength;
};

void applyFirFilter(const FilterInput& input, std::vector<float>& outSignal)
{
    outSignal.resize(input.outputLength);

    for (uint32_t i = 0; i < outSignal.size(); ++i)
    {
        outSignal[i] = 0.0f;
        for (uint32_t j = 0; j < input.filter.size(); ++j)
        {
            outSignal[i] += input.signal[i + j] * input.filter[j];
        }
    }
}

void applyFirFilterAVX(const FilterInput& input, std::vector<float>& outSignal)
{
    outSignal.resize(input.outputLength, 0.0f);

    std::array<float, AVX_FLOAT_COUNT> result;
    for (uint32_t i = 0; i < input.outputLength; ++i)
    {
        __m256 accumulator = _mm256_setzero_ps();
        result.fill(0.0f);
        
        for (uint32_t j = 0; j < input.filter.size(); j += AVX_FLOAT_COUNT)
        {
            __m256 signalLoad = _mm256_loadu_ps(input.signal.data() + i + j); // load signal and filter
            __m256 filterLoad = _mm256_loadu_ps(input.filter.data() + j);
            __m256 temp       = _mm256_mul_ps(signalLoad, filterLoad);        // element-wise multiply
            accumulator       = _mm256_add_ps(accumulator, temp);             // element-wise add
        }
        
        _mm256_storeu_ps(result.data(), accumulator); // move the accumulator into the result and sum
        outSignal[i] = std::accumulate(result.begin(), result.end(), 0.f);
    }
}

struct AudioEffect {
    virtual void Apply(const Signal& inSignal, Signal& outSignal, bool bUseAVX=true) const = 0;
    virtual void DrawGUI() = 0;
    virtual AudioEffect* Clone() = 0;
};

struct DelayEffect : public AudioEffect
{
    void Apply(const Signal& inSignal, Signal& outSignal, bool bUseAVX=true) const override
    {
        uint32_t filterLength = inSignal.sampleRate * delay;
	    std::vector<float> echoFilter(filterLength);
	    for (int i = 0; i < filterLength; i++) {
            echoFilter[i] = 0.0f;
	    } echoFilter[0] = 1.0f; echoFilter[filterLength-1] = 1.0f;

        FilterInput input(inSignal.data, echoFilter);
        if (bUseAVX) applyFirFilterAVX(input, outSignal.data);
        else applyFirFilter(input, outSignal.data);
        outSignal.sampleRate = inSignal.sampleRate;
    }

    void DrawGUI() override
    {
        ImGui::Text("Delay");
        ImGui::DragFloat("Delay", &delay, 0.1f, 0.0f, 5.0f);
    }

    AudioEffect* Clone() override
    {
        AudioEffect* clone = new DelayEffect();
        ((DelayEffect*)clone)->delay = delay;
        return clone;
    }

    float delay = 1.0f;
};

struct DerivativeEffect : public AudioEffect
{
    void Apply(const Signal& inSignal, Signal& outSignal, bool bUseAVX = true) const override
    {
        FilterInput input(inSignal.data, derivativeFilter);
        if (bUseAVX) applyFirFilterAVX(input, outSignal.data);
        else applyFirFilter(input, outSignal.data);
        outSignal.sampleRate = inSignal.sampleRate;
    }

    void DrawGUI() override
    {
        ImGui::Text("Derivative");
    }

    AudioEffect* Clone() override
    {
        return new DerivativeEffect();
    }

    std::vector<float> derivativeFilter = {
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
};

#ifdef _WINDOWS
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
#endif

#include <list>

struct AudioPlaybackCleanup
{
private:
    struct _AudioPlayback
    {
        void* data;
        float duration;
    };

    inline static std::list<_AudioPlayback> cleanupList;
public:
    static void PushAudio(void* data, float duration)
    {
        cleanupList.push_back({ data, duration });
    }

    static void CleanupFrame(float dt)
    {
        std::cout << "Frame\n";

        for (auto it = cleanupList.begin(); it != cleanupList.end();)
        {
            std::cout << it->duration << "\n";

            it->duration -= dt;
            if (it->duration < 0.0f)
            {
                std::cout << "freed!\n";
                free(it->data);
                auto prevIt = it;
                it++;
                cleanupList.erase(prevIt);
            }
            else it++;
        }
    }

    static void Clear()
    {
        for (_AudioPlayback& c : cleanupList)
        {
            free(c.data);
        } cleanupList.clear();
    }
};

#ifndef _WINDOWS
void PlayBufferAsync(int16_t* audioBuffer, uint32_t numSamples, uint32_t samplingFrequency)
{
    pa_simple *s;
    pa_sample_spec ss;
    
    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 1;
    ss.rate = samplingFrequency;
    
    s = pa_simple_new(NULL,           // Use the default server.
                  "Fooapp",           // Our application's name.
                  PA_STREAM_PLAYBACK,
                  NULL,               // Use the default device.
                  "Music",            // Description of our stream.
                  &ss,                // Our sample format.
                  NULL,               // Use default channel map
                  NULL,               // Use default buffering attributes.
                  NULL                // Ignore error code.
                  );

    for (int i = 0; i < numSamples; i += 512)
    {
        uint16_t buffer[512];
        ssize_t size = 512 * sizeof(int16_t);
        memcpy(buffer, audioBuffer + i, size);
        pa_simple_write(s, buffer, size, NULL);

        if (bPlaybackAudioAsync.load() == false) break;
    }

    bPlaybackAudioAsync.store(false);

    pa_simple_free(s);
    free(audioBuffer);
}
#endif

void PlaySignal(const Signal& signal)
{
    uint32_t numSamples = signal.data.size();

#ifdef _WINDOWS
    // WinApi PlaySound requires the buffer in memory to have the wav file header
    char *wavBuffer = new char[sizeof(WaveHeader) + sizeof(int16_t)*numSamples];
    WaveHeader header(signal.sampleRate, /*bitDepth=*/16, /*numChannels=*/1, numSamples);
    memcpy(wavBuffer, &header, sizeof(WaveHeader));
    int16_t *data = (int16_t*)(wavBuffer + sizeof(WaveHeader));
#else
    int16_t *data = new int16_t[sizeof(int16_t)*numSamples];
#endif

    // Repackage audioBuffer as 16 bit ints
    constexpr float max16BitValue = 32768.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        int pcm = int(signal.data.data()[i] * (max16BitValue));
        if (pcm >= max16BitValue) pcm = max16BitValue - 1;
        else if (pcm < -max16BitValue) pcm = -max16BitValue;
        data[i] = int16_t(pcm);
    }

#if _WINDOWS
    //PlaySound((LPCSTR)filepath.c_str(), NULL, SND_FILENAME);
    //PlaySound(MAKEINTRESOURCE((char*)wavBuffer), NULL, SND_FILENAME);
    PlaySound(wavBuffer, NULL, SND_MEMORY | SND_ASYNC);
    AudioPlaybackCleanup::PushAudio(wavBuffer, numSamples / signal.sampleRate + 1);
#else
    bPlaybackAudioAsync.store(true);
    std::thread soundThread(PlayBufferAsync, data, numSamples, signal.sampleRate);
    soundThread.detach();
    AudioPlaybackCleanup::PushAudio(data, numSamples / signal.sampleRate + 1);
#endif
}

void StopAudio()
{
#ifdef _WINDOWS
    PlaySound(NULL, NULL, 0);
#else
    bPlaybackAudioAsync.store(false);
#endif
}

void WriteSignal(const Signal& signal, const std::string& path) {
	std::vector<std::vector<float>> channels = { signal.data };

	AudioFile<float> af;
	af.setAudioBuffer(channels);
    af.setSampleRate(signal.sampleRate);
	af.save(path);
}

float SineGenerator(float pos)
{
  return sin(pos*TAU);
}

float SquareGenerator(float pos)
{
  return sin(pos*TAU) > 0.0f ? 1.0f : -1.0f;
}

float TriangleGenerator(float pos)
{
  return 1-fabs(pos-0.5)*4;
}

float SawGenerator(float pos)
{
  return pos*2-1;
}

float GenerateSignalAtSampleIndex(float(*generator)(float), uint32_t i, float frequency, float sampleRate)
{
    float pos = fmod(frequency * i / sampleRate, 1.0);
    return generator(pos);
}

uint32_t GenerateSignal(float(*generator)(float), float frequency, uint32_t sampleRate, float duration, std::vector<float>& outSignal)
{
    uint32_t numSamples = sampleRate * duration;
    outSignal.resize(numSamples);
    for (uint32_t i = 0; i < numSamples; i++)
    {
        outSignal[i] = GenerateSignalAtSampleIndex(generator, i, frequency, sampleRate);
    }

    return sampleRate;
}

static void ApplyEffectsInternal(Signal inSignal, std::promise<Signal>&& outSignal, std::vector<AudioEffect*> effects, bool bUseAVX)
{
    Signal temp;
    for (AudioEffect* effect : effects)
    {
        effect->Apply(inSignal, temp, bUseAVX);
        inSignal = temp;
    }

    for (auto& effect : effects) delete(effect);

    outSignal.set_value(temp);
    bFilterThreadRunning.store(false);
}

void ApplyEffectsAsync(const Signal& inSignal, std::promise<Signal>& outSignalPromise, const std::vector<std::unique_ptr<AudioEffect>>& effects, bool bUseAVX)
{
    std::vector<AudioEffect*> effectsCopy; // Must copy the unique pointers to avoid race conditions
    effectsCopy.reserve(effects.size());
    for (const auto& effect : effects) effectsCopy.push_back(effect->Clone());

    std::thread filterThread(ApplyEffectsInternal, inSignal, std::move(outSignalPromise), effectsCopy, bUseAVX);
    filterThread.detach();
}
