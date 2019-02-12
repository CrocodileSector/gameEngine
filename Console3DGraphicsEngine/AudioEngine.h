#pragma once
#pragma comment(lib, "winmm.lib")

#include <Windows.h>
#include <mmreg.h>
#include <list>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

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


// This structure represents a sound that is currently playing. It only
	// holds the sound ID and where this instance of it is up to for its
	// current playback
struct sCurrentPlayingSample
{
	int nAudioSampleID = 0;
	long nSamplePosition = 0;
	bool bFinished = false;
	bool bLoop = false;
};

class AudioEngine
{
public:
	AudioEngine();
	AudioEngine(const std::vector<AudioSample>& in_audioSamples);
	
	unsigned int LoadAudioSample(std::wstring in_sWavFile);
	void PlaySample(int id, bool bLoop = false);
	void StopSample(int id);
	bool CreateAudio(unsigned int in_sampleRate = 44100, unsigned int in_channels = 1,
		unsigned int in_blocks = 8, unsigned int in_blockSamples = 512);
	bool DestroyAudio();
	float GetMixerOutput(int in_channel, float in_fGlobalTime, float in_fTimeStep);
	
	void waveOutProc(HWAVEOUT in_hWaveOut, UINT in_uMsg, DWORD in_dwParam1, DWORD in_dwParam2);
	static void CALLBACK waveOutProcWrapper(HWAVEOUT in_hWaveOut, UINT in_uMsg, DWORD dwInstance, DWORD in_dwParam1, DWORD in_dwParam2);

	virtual float onUserSoundSample(int in_channel, float in_fGlobalTime, float in_fTimeStep);
	virtual float onUserSoundFilter(int in_channel, float in_fGlobalTime, float fSample);

	void AudioThread();
private:

	bool m_bEnableSound = false;

	std::vector<AudioSample> m_vAudioSamples;
	std::list<sCurrentPlayingSample> m_lActiveSamples;

	unsigned int m_nSampleRate;
	unsigned int m_nChannels;
	unsigned int m_nBlockCount;
	unsigned int m_nBlockSamples;
	unsigned int m_nCurrentBlock;

	short* m_pMemBlock = nullptr;
	WAVEHDR* m_pWaveHeaders = nullptr;
	HWAVEOUT m_hwDevice = nullptr;

	std::thread m_tAudioThread;
	std::atomic<bool> m_bAudioThreadActive = false;
	std::atomic<unsigned int> m_nBlockFree = 0;
	std::atomic<float> m_fGlobalTime = 0.0f;
	std::condition_variable m_cvBlockNotZero;
	std::mutex m_muxBlockNotZero;
};