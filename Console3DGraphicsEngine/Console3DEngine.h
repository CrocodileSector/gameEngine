#pragma once

#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>

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

	void SetGlyph(int x, int y, short c);

	void SetColour(int x, int y, short c);

    short GetGlyph(int x, int y);

	short GetColour(int x, int y);

	short SampleGlyph(float x, float y);

	short SampleColour(float x, float y);

	bool Save(std::wstring in_sFile);

	bool Load(std::wstring in_sFile);
};

class ConsoleGameEngine
{
public:

	ConsoleGameEngine();

	~ConsoleGameEngine();

	const int ScreenWidth() const { return m_nScreenWidth; }
	const int ScreenHeight() const { return m_nScreenHeight; }

	void Start();

	void EnableSound();

	int ConstructConsole();

	virtual void Draw(int x, int y, short c = 0x2588, short col = 0x000F); //these fuckin magic numbers man...

	void DrawLine(int x1, int y1, int x2, int y2, short c = 0x2588, short col = 0x000F);

	void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short c = 0x2588, short col = 0x000F);

	void DrawCircle(int xc, int yc, int r, short c = 0x2588, short col = 0x000F);

	void DrawString(int x, int y, std::wstring c, short col = 0x000F);

	void DrawStringAlpha(int x, int y, std::wstring c, short col = 0x000F);

	void DrawSprite(int x, int y, Sprite* sp);

	void DrawPartialSprite(int x, int y, Sprite* sp, int ox, int oy, int w, int h);

	void DrawWireFrameModel(const std::vector<std::pair<float, float>> &vModelCoordinates, float x, float y, float r = 0.0f, float s = 1.0f, short col = FG_WHITE, short c = PIXEL_SOLID);

	void Fill(int x1, int x2, int y1, int y2, short c = 0x2588, short col = 0x000F);

	void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short c = 0x2588, short col = 0x000F);

	void FillCircle(int xc, int yc, int r, short c = 0x2588, short col = 0x000F);

	void Clip(int &x, int &y);

	virtual bool OnUserCreate() = 0;
	virtual bool OnUserUpdate(float fElapsedTime) = 0;
	virtual bool OnUserDestroy() { return true; };

private:

	CONSOLE_SCREEN_BUFFER_INFO m_OriginalConsoleInfo;
	CHAR_INFO* m_screenBuffer;
	std::wstring m_sAppName;


	HANDLE m_hOriginalConsole;
	HANDLE m_hConsole;
	HANDLE m_hConsoleIn;

	SMALL_RECT m_windowRect;

	int m_nScreenWidth;
	int m_nScreenHeight;

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