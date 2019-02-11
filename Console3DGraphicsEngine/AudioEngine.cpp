#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif 

#include <iostream>

#include "AudioEngine.h"

AudioSample::AudioSample()
{
}

AudioSample::AudioSample(std::wstring in_sWavFile)
{
	FILE *f = nullptr;
	_wfopen_s(&f, in_sWavFile.c_str(), L"rb");

	if (!f)
		return;

	char dump[4];

	//should read "RIFF" field and skip it
	fread(&dump, sizeof(char), 4, f); 
	if (strncmp(dump, "RIFF", 4) != 0)
		return;
	fread(&dump, sizeof(char), 4, f); 

	//should read "WAVE" field and skip it
	fread(&dump, sizeof(char), 4, f);
	if (strncmp(dump, "WAVE", 4) != 0)
		return;
	fread(&dump, sizeof(char), 4, f);


	fread(&m_wavHeader, sizeof(WAVEFORMATEX) - 2, 1, f); // -2 because WAVEFORMATEX holds its own size on the first 2 bytes

	if (m_wavHeader.wBitsPerSample != 16 || m_wavHeader.nSamplesPerSec != 44100)
	{
		fclose(f);
		return;
	}

	long nChunkSize = 0;
	fread(&dump, sizeof(char), 4, f);
	fread(&nChunkSize, sizeof(long), 1, f);

	//iterate through the chunks until we find our audio data
	while (strncmp(dump, "data", 4) != 0)
	{
		fseek(f, nChunkSize, SEEK_CUR);
		fread(&dump, sizeof(char), 4, f);
		fread(&nChunkSize, sizeof(long), 1, f);
	}

	m_nSamples = nChunkSize / (m_wavHeader.nChannels * (m_wavHeader.wBitsPerSample >> 3));
	m_nChannels = m_wavHeader.nChannels;

	m_fSample = new float[m_nSamples * m_nChannels];

	float* pSample = m_fSample;

	for (long i = 0; i < m_nSamples; ++i)
	{
		for (int j = 0; j < m_nChannels; ++j)
		{
			short s = 0;
			fread(&s, sizeof(short), 1, f);
			*pSample = (float)s / (float)(SHRT_MAX);
			pSample++;
		}
	}

	fclose(f);
	m_bSampleValid = true;
}
