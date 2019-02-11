#pragma once
#pragma comment(lib, "winmm.lib")

#include <mmreg.h>

class AudioSample
{
public :
	AudioSample();

	AudioSample(std::wstring in_sWavFile);

	WAVEFORMATEX m_wavHeader;
	float* m_fSample = nullptr;
	long m_nSamples = 0;
	int m_nChannels = 0;
	bool m_bSampleValid = false;
};


class AudioEngine
{

};