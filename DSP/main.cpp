#include "DSP.h"

#include "AudioFile/AudioFile.h"

#include <iostream>

// TODO:
// - Implement different filter types (echo, low pass, high pass, band pass, 
// - Preset waveforms to use (test files and also saw/square/sine waves)
// - Some kind of interface? imgui?

#define TAU 6.2831855

std::vector<float> GenerateSineWave(float amplitude, float frequency, int numSamples) {
	std::vector<float> result(numSamples);

	for (int i = 0; i < numSamples; i++)
	{
		result[i] = amplitude * std::sin(frequency * i * TAU);
	}

	return result;
}

void WriteSignal(const std::vector<float>& signal, const std::string& path) {
	std::vector<std::vector<float>> channels = { signal };

	AudioFile<float> af;
	af.setAudioBuffer(channels);
	af.save(path);
}

int main()
{
	std::cout << "Loading signals...\n";

	AudioFile<float> testSignal;
	testSignal.load("data/input/test-audio.wav");
	//thomasSignal.load("data/input/thomas.wav");
	//std::vector<float> sine = GenerateSineWave(1.0f, 440.0f, 441000 * 5);

	std::cout << "Loading filters...\n";
	std::vector<float> impulseResponse = { -1.0f, 0.0f, 1.0f };
	std::vector<float> derivativeImpulseResponse = {
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
	std::vector<float> echoImpulseResponse;
	for (int i = 0; i < 44100; i++) {
		echoImpulseResponse.push_back(0.0f);
	} echoImpulseResponse[0] = 1.0f; echoImpulseResponse.push_back(1.0f);

	FilterInput testEchoFilter(testSignal.samples[0], echoImpulseResponse);

	std::cout << "Applying filter (non-SIMD)...\n";
	std::vector<float> result;
	//applyFirFilter(testEchoFilter, result);
	//WriteSignal(result, "data/out.wav");

	std::cout << "Applying filter (SIMD)...\n";
	result.clear();
	applyFirFilterAVX(testEchoFilter, result);
	WriteSignal(result, "data/outSIMD.wav");

	return 0;
}