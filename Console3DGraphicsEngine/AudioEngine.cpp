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

AudioEngine::AudioEngine()
	: m_bEnableSound(true)
{
}

AudioEngine::AudioEngine(const std::vector<AudioSample>& in_audioSamples)
	: m_vAudioSamples(in_audioSamples.begin(), in_audioSamples.end()), m_bEnableSound(true)
{
}

// Load a 16-bit WAVE file @ 44100Hz ONLY into memory. A sample ID
// number is returned if successful, otherwise -1
unsigned int AudioEngine::LoadAudioSample(std::wstring in_sWavFile)
{
	if (!m_bEnableSound)
		return -1;

	AudioSample sample(in_sWavFile);
	if (sample.m_bSampleValid)
	{
		m_vAudioSamples.push_back(sample);
		return m_vAudioSamples.size();
	}

	return -1;
}

void AudioEngine::PlaySample(int id, bool bLoop)
{
	sCurrentPlayingSample currSample;
	currSample.nAudioSampleID = id;
	currSample.nSamplePosition = 0;
	currSample.bFinished = false;
	currSample.bLoop = bLoop;

	m_lActiveSamples.push_back(currSample);
}

void AudioEngine::StopSample(int id)
{
	//TODO
}

bool AudioEngine::CreateAudio(unsigned int in_sampleRate, unsigned int in_channels, unsigned int in_blocks, unsigned int in_blockSamples)
{
	m_bAudioThreadActive = false;
	m_nSampleRate = in_sampleRate;
	m_nChannels = in_channels;
	m_nBlockFree = m_nBlockCount = in_blocks;
	m_nBlockSamples = in_blockSamples;
	m_nCurrentBlock = 0;
	m_pMemBlock = nullptr;
	m_pWaveHeaders = nullptr;

	//Create a device to output to
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = m_nSampleRate;
	waveFormat.wBitsPerSample = sizeof(short) * 8;
	waveFormat.nChannels = m_nChannels;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	//If device is valid, open it
	if (waveOutOpen(&m_hwDevice, WAVE_MAPPER, &waveFormat, (DWORD_PTR)waveOutProcWrapper, (DWORD_PTR)this, CALLBACK_FUNCTION) != S_OK)
		DestroyAudio();

	m_pMemBlock = new short[m_nBlockCount * m_nBlockSamples];
	if (!m_pMemBlock)
		return DestroyAudio();
	ZeroMemory(m_pMemBlock, sizeof(short) * m_nBlockCount * m_nBlockSamples);

	m_pWaveHeaders = new WAVEHDR[m_nBlockCount];
	if (!m_pWaveHeaders)
		return DestroyAudio();
	ZeroMemory(m_pWaveHeaders, sizeof(WAVEHDR) * m_nBlockCount);

	for (unsigned int i = 0; i < m_nBlockCount; i++)
	{
		m_pWaveHeaders[i].dwBufferLength = m_nBlockSamples * sizeof(short);
		m_pWaveHeaders[i].lpData = (LPSTR)(m_pMemBlock + (i * m_nBlockSamples));
	}

	m_bAudioThreadActive = true;
	m_tAudioThread = std::thread(&AudioEngine::AudioThread, this);

	std::unique_lock<std::mutex> lock(m_muxBlockNotZero);
	m_cvBlockNotZero.notify_one();

	return true;
}

bool AudioEngine::DestroyAudio()
{
	m_bAudioThreadActive = false;
	return false;
}


// Handler for soundcard request for more data
void AudioEngine::waveOutProc(HWAVEOUT in_hWaveOut, UINT in_uMsg, DWORD in_dwParam1, DWORD in_dwParam2)
{
	if (in_uMsg != WOM_DONE) 
		return;
	m_nBlockFree++;
	std::unique_lock<std::mutex> lock(m_muxBlockNotZero);
	m_cvBlockNotZero.notify_one();
}

// Static wrapper for sound card handler
void AudioEngine::waveOutProcWrapper(HWAVEOUT in_hWaveOut, UINT in_uMsg, DWORD dwInstance, DWORD in_dwParam1, DWORD in_dwParam2)
{
	((AudioEngine*)dwInstance)->waveOutProc(in_hWaveOut, in_uMsg, in_dwParam1, in_dwParam2);
}

// Audio thread. This loop responds to requests from the soundcard to fill 'blocks'
// with audio data. If no requests are available it goes dormant until the sound
// card is ready for more data. The block is filled by the "user" in some manner
// and then issued to the soundcard.
void AudioEngine::AudioThread()
{
	m_fGlobalTime = 0.0f;
	float fTimeStep = 1.0f / (float)m_nSampleRate;

	//shitty hack to get the maximum integer
	short nMaxSample = (short)pow(2, (sizeof(short) * 8) - 1) - 1;
	short fMaxSample = (float)nMaxSample;
	short nPreviousSample = 0;

	while (m_bAudioThreadActive)
	{
		//wait while a block is made available
		if (m_nBlockFree == 0)
		{
			std::unique_lock<std::mutex> lock(m_muxBlockNotZero);
			while (m_nBlockFree == 0)
				m_cvBlockNotZero.wait(lock);
		}

		m_nBlockFree--;

		// Prepare block for processing
		if (m_pWaveHeaders[m_nCurrentBlock].dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(m_hwDevice, &m_pWaveHeaders[m_nCurrentBlock], sizeof(WAVEHDR));

		short nNewSample = 0;
		int nCurrentBlock = m_nCurrentBlock * m_nBlockSamples;

		auto clip = [](float fSample, float fMax)
		{
			if (fSample >= 0.0)
				return fmin(fSample, fMax);
			else
				return fmax(fSample, -fMax);
		};

		for (unsigned int i = 0; i < m_nBlockSamples; i += m_nChannels)
		{
			for (unsigned int j = 0; j < m_nChannels; j++)
			{
				nNewSample = (short)(clip(GetMixerOutput(j, m_fGlobalTime, fTimeStep), 1.0) * fMaxSample);
				m_pMemBlock[nCurrentBlock + i + j] = nNewSample;
				nPreviousSample = nNewSample;
			}
			m_fGlobalTime = m_fGlobalTime + fTimeStep;
		}

		// Send block to sound device
		waveOutPrepareHeader(m_hwDevice, &m_pWaveHeaders[m_nCurrentBlock], sizeof(WAVEHDR));
		waveOutWrite(m_hwDevice, &m_pWaveHeaders[m_nCurrentBlock], sizeof(WAVEHDR));
		m_nCurrentBlock++;
		m_nCurrentBlock %= m_nBlockCount;
	}
}


// The Sound Mixer - If the user wants to play many sounds simultaneously, and
// perhaps the same sound overlapping itself, then you need a mixer, which
// takes input from all sound sources for that audio frame. This mixer maintains
// a list of sound locations for all concurrently playing audio samples. Instead
// of duplicating audio data, we simply store the fact that a sound sample is in
// use and an offset into its sample data. As time progresses we update this offset
// until it is beyound the length of the sound sample it is attached to. At this
// point we remove the playing souind from the list.
//
// Additionally, the users application may want to generate sound instead of just
// playing audio clips (think a synthesizer for example) in whcih case we also
// provide an "onUser..." event to allow the user to return a sound for that point
// in time.
//
// Finally, before the sound is issued to the operating system for performing, the
// user gets one final chance to "filter" the sound, perhaps changing the volume
// or adding funky effects
float AudioEngine::GetMixerOutput(int in_channel, float in_fGlobalTime, float in_fTimeStep)
{
	// Accumulate sample for this channel
	float fMixerSample = 0.0f;

	for (auto &s : m_lActiveSamples)
	{
		// Calculate sample position
		s.nSamplePosition += (long)((float)m_vAudioSamples[s.nAudioSampleID - 1].m_wavHeader.nSamplesPerSec * in_fTimeStep);

		// If sample position is valid add to the mix
		if (s.nSamplePosition < m_vAudioSamples[s.nAudioSampleID - 1].m_nSamples)
			fMixerSample += m_vAudioSamples[s.nAudioSampleID - 1].m_fSample[(s.nSamplePosition * m_vAudioSamples[s.nAudioSampleID - 1].m_nChannels) + in_channel];
		else
			s.bFinished = true; // Else sound has completed

		// If sounds have completed then remove them
		m_lActiveSamples.remove_if([](const sCurrentPlayingSample &s) {return s.bFinished; });

		// The users application might be generating sound, so grab that if it exists
		fMixerSample += onUserSoundSample(in_channel, in_fGlobalTime, in_fTimeStep);

		// Return the sample via an optional user override to filter the sound
		return onUserSoundFilter(in_channel, in_fGlobalTime, in_fTimeStep);
	}
}

// Overridden by user if they want to generate sound in real-time
float AudioEngine::onUserSoundSample(int in_channel, float in_fGlobalTime, float in_fTimeStep)
{
	return 0.0f;
}

// Overriden by user if they want to manipulate the sound before it is played
float AudioEngine::onUserSoundFilter(int in_channel, float in_fGlobalTime, float fSample)
{
	return fSample;
}

