#pragma once

#include <Windows.h>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "AudioEngine.h"

enum COLOUR
{
	FG_BLACK = 0x0000,
	FG_DARK_BLUE = 0x0001,
	FG_DARK_GREEN = 0x0002,
	FG_DARK_CYAN = 0x0003,
	FG_DARK_RED = 0x0004,
	FG_DARK_MAGENTA = 0x0005,
	FG_DARK_YELLOW = 0x0006,
	FG_GREY = 0x0007, // Thanks MS :-/
	FG_DARK_GREY = 0x0008,
	FG_BLUE = 0x0009,
	FG_GREEN = 0x000A,
	FG_CYAN = 0x000B,
	FG_RED = 0x000C,
	FG_MAGENTA = 0x000D,
	FG_YELLOW = 0x000E,
	FG_WHITE = 0x000F,
	BG_BLACK = 0x0000,
	BG_DARK_BLUE = 0x0010,
	BG_DARK_GREEN = 0x0020,
	BG_DARK_CYAN = 0x0030,
	BG_DARK_RED = 0x0040,
	BG_DARK_MAGENTA = 0x0050,
	BG_DARK_YELLOW = 0x0060,
	BG_GREY = 0x0070,
	BG_DARK_GREY = 0x0080,
	BG_BLUE = 0x0090,
	BG_GREEN = 0x00A0,
	BG_CYAN = 0x00B0,
	BG_RED = 0x00C0,
	BG_MAGENTA = 0x00D0,
	BG_YELLOW = 0x00E0,
	BG_WHITE = 0x00F0,
};

enum PIXEL_TYPE
{
	PIXEL_SOLID = 0x2588,
	PIXEL_THREEQUARTERS = 0x2593,
	PIXEL_HALF = 0x2592,
	PIXEL_QUARTER = 0x2591,
};

class Sprite
{
public:
	Sprite();

	Sprite(int w, int h);

	Sprite(std::wstring in_sFile);

private:
	int m_nWidth = 0;
	int m_nHeight = 0;
	short* m_glyphs = nullptr;
	short* m_colours = nullptr;

	void Create(int w, int h);

public:

	const int GetWidth() const { return m_nWidth; }
	const int GetHeight() const { return m_nHeight; }

	void SetGlyph(int x, int y, short c);

	void SetColour(int x, int y, short c);

    short GetGlyph(int x, int y);

	short GetColour(int x, int y);

	short SampleGlyph(float x, float y);

	short SampleColour(float x, float y);

	bool Save(std::wstring in_sFile);

	bool Load(std::wstring in_sFile);
};

struct sKeyState
{
	bool bPressed;
	bool bReleased;
	bool bHeld;
};// ;

class ConsoleGameEngine
{
public:

	ConsoleGameEngine();

	~ConsoleGameEngine();

	const int ScreenWidth() const { return m_nScreenWidth; }
	const int ScreenHeight() const { return m_nScreenHeight; }

	void Start();

	void EnableSound();

	int ConstructConsole(int width, int height, int fontw, int fonth);

	virtual void Draw(int x, int y, short c = 0x2588, short col = 0x000F); //these fuckin magic numbers man...

	void DrawLine(int x1, int y1, int x2, int y2, short c = 0x2588, short col = 0x000F);

	void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short c = 0x2588, short col = 0x000F);

	void DrawCircle(int xc, int yc, int r, short c = 0x2588, short col = 0x000F);

	void DrawString(int x, int y, std::wstring c, short col = 0x000F);

	void DrawStringAlpha(int x, int y, std::wstring c, short col = 0x000F);

	void DrawSprite(int x, int y, Sprite* sp);

	void DrawPartialSprite(int x, int y, Sprite* sp, int ox, int oy, int w, int h);

	void DrawWireFrameModel(const std::vector<std::pair<float, float>> &vModelCoordinates, float x, float y, float r = 0.0f, float s = 1.0f, short col = COLOUR::FG_WHITE, short c = PIXEL_TYPE::PIXEL_SOLID);

	void Fill(int x1, int x2, int y1, int y2, short c = 0x2588, short col = 0x000F);

	void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short c = 0x2588, short col = 0x000F);

	void FillCircle(int xc, int yc, int r, short c = 0x2588, short col = 0x000F);

	void Clip(int &x, int &y);

	virtual bool OnUserCreate() = 0;
	virtual bool OnUserUpdate(float fElapsedTime) = 0;
	virtual bool OnUserDestroy() { return true; };
	
protected:
	std::wstring m_sAppName;
	int m_mousePosX;
	int m_mousePosY;

	sKeyState GetKey(int nKeyID) { return m_keys[nKeyID]; }
	sKeyState GetMouse(int nMouseButtonID) { return m_mouse[nMouseButtonID]; }
	int GetMouseX() { return m_mousePosX; }
	int GetMouseY() { return m_mousePosY; }
	bool IsFocused() { return m_isConsoleInFocus; }

	int Error(const wchar_t *msg);
	static BOOL CloseHandler(DWORD evt)
	{
		// Note this gets called in a seperate OS thread, so it must
		// only exit when the game has finished cleaning up, or else
		// the process will be killed before OnUserDestroy() has finished
		if (evt == CTRL_CLOSE_EVENT)
		{
			m_bAtomicSwitch = false;

			// Wait for thread to be exited
			std::unique_lock<std::mutex> lock(m_gameMutex);
			m_cvQuitGame.wait(lock);
		}
		return true;
	}
private:

	CONSOLE_SCREEN_BUFFER_INFO m_OriginalConsoleInfo;
	CHAR_INFO* m_screenBuffer;

	AudioEngine* m_pAudioEngine = nullptr;


	HANDLE m_hOriginalConsole;
	HANDLE m_hConsoleOut;
	HANDLE m_hConsoleIn;

	SMALL_RECT m_windowRect;

	int m_nScreenWidth;
	int m_nScreenHeight;

	sKeyState m_keys[256];
	sKeyState m_mouse[5];

	short m_keyOldState[256] = { 0 };
	short m_keyNewState[256] = { 0 };
	bool m_mouseOldState[5] = { 0 };
	bool m_mouseNewState[5] = { 0 };
	bool m_isConsoleInFocus = true;
	bool m_bEnableSound = false;

	static std::atomic<bool> m_bAtomicSwitch;
	static std::condition_variable m_cvQuitGame;
	static std::mutex m_gameMutex;

	void gameThread();
};