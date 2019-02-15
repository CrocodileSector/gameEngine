#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif 

#ifndef DEBUG
#define DEBUG 1
#endif // !DEBUG


#pragma comment(lib, "winmm.lib")

#include "Console3DEngine.h"

#include <Windows.h>
#include <iostream>

// Define our static variables
std::atomic<bool> ConsoleGameEngine::m_bAtomicSwitch(false);
std::condition_variable ConsoleGameEngine::m_cvQuitGame;
std::mutex ConsoleGameEngine::m_gameMutex;

Sprite::Sprite() { }

Sprite::Sprite(int w, int h)
{
	Create(w, h);
}

Sprite::Sprite(std::wstring in_sFile)
{
	if (!Load(in_sFile))
		Create(8, 8);
}

void Sprite::Create(int w, int h)
{
	m_nWidth = w;
	m_nHeight = h;

	m_glyphs = new short[m_nHeight * m_nWidth];
	m_colours = new short[m_nHeight * m_nWidth];

	for (size_t i = 0; i < m_nHeight * m_nWidth; ++i)
	{
		m_glyphs[i] = L' ';
		m_colours[i] = FG_BLACK;
	}
}

void Sprite::SetGlyph(int x, int y, short c)
{
	if (x < 0 || x > m_nWidth || y < 0 || y > m_nHeight)
		return;
	else
		m_glyphs[y * m_nWidth + x] = c;
}

void Sprite::SetColour(int x, int y, short c)
{
	if (x < 0 || x > m_nWidth || y < 0 || y > m_nHeight)
		return;
	else
		m_colours[y * m_nWidth + x] = c;
}

short Sprite::GetGlyph(int x, int y)
{
	if (x < 0 || x > m_nWidth || y < 0 || y > m_nHeight)
		return -1;
	else
		return m_glyphs[y * m_nWidth + x];
}

short Sprite::GetColour(int x, int y)
{
	if (x < 0 || x > m_nWidth || y < 0 || y > m_nHeight)
		return -1;
	else
		return m_colours[y * m_nWidth + x];
}

short Sprite::SampleGlyph(float x, float y)
{
	int sample_X = (int)(x * (float)m_nWidth);
	int sample_Y = (int)(y * (float)m_nHeight - 1.0f); //wtf is with this magic number?


	if (sample_X < 0 || sample_X > m_nWidth || sample_Y < 0 || sample_Y > m_nHeight)
		return L' ';
	else
		return m_glyphs[sample_Y * m_nWidth + sample_X];
}

short Sprite::SampleColour(float x, float y)
{
	int sample_X = (int)(x * (float)m_nWidth);
	int sample_Y = (int)(y * (float)m_nHeight - 1.0f); //wtf is with this magic number?


	if (sample_X < 0 || sample_X >= m_nWidth || sample_Y < 0 || sample_Y >= m_nHeight)
		return L' ';
	else
		return m_colours[sample_Y * m_nWidth + sample_X];
}

bool Sprite::Save(std::wstring in_sFile)
{
	FILE *f = nullptr;

	_wfopen_s(&f, in_sFile.c_str(), L"wb");

	if (!f)
		return false;

	fwrite(&m_nWidth, sizeof(int), 1, f);
	fwrite(&m_nHeight, sizeof(int), 1, f);
	fwrite(m_glyphs, sizeof(short), m_nWidth * m_nHeight, f);
	fwrite(m_colours, sizeof(short), m_nWidth * m_nHeight, f);

	fclose(f);

	return true;
}

bool Sprite::Load(std::wstring in_sFile)
{
	delete[] m_glyphs;
	delete[] m_colours;

	m_nHeight = 0;
	m_nWidth = 0;

	FILE *f = nullptr;

	_wfopen_s(&f, in_sFile.c_str(), L"rb");

	if (!f)
		return false;

	fread(&m_nWidth, sizeof(int), 1, f);
	fread(&m_nHeight, sizeof(int), 1, f);

	Create(m_nWidth, m_nHeight);

	fread(m_glyphs, sizeof(short), m_nWidth * m_nHeight, f);
	fread(m_colours, sizeof(short), m_nWidth * m_nHeight, f);

	fclose(f);

	return true;
}

ConsoleGameEngine::ConsoleGameEngine()
{
	m_nScreenHeight = 30;
	m_nScreenWidth = 80;

	m_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	m_hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);

	std::memset(m_keyNewState, 0, 256 * sizeof(short));
	std::memset(m_keyOldState, 0, 256 * sizeof(short));

	m_mousePosX = 0;
	m_mousePosY = 0;

	m_bEnableSound = false;

	m_sAppName = L"Default";
}

ConsoleGameEngine::~ConsoleGameEngine()
{
	SetConsoleActiveScreenBuffer(m_hOriginalConsole);
	delete[] m_screenBuffer;
}

void ConsoleGameEngine::Start()
{
	// Start the thread
	m_bAtomicSwitch = true;
	std::thread t = std::thread(&ConsoleGameEngine::gameThread, this);

	// Wait for thread to be exited
	t.join();
}

void ConsoleGameEngine::EnableSound()
{
	m_bEnableSound = true;
}

int ConsoleGameEngine::ConstructConsole(int width, int height, int fontw, int fonth)
{
	if (m_hConsoleOut == INVALID_HANDLE_VALUE)
		return Error(L"ConstructConsole:: Bad output handle");

	m_nScreenHeight = height;
	m_nScreenWidth = width;

	// Update 13/09/2017 - It seems that the console behaves differently on some systems
	// and I'm unsure why this is. It could be to do with windows default settings, or
	// screen resolutions, or system languages. Unfortunately, MSDN does not offer much
	// by way of useful information, and so the resulting sequence is the reult of experiment
	// that seems to work in multiple cases.
	//
	// The problem seems to be that the SetConsoleXXX functions are somewhat circular and 
	// fail depending on the state of the current console properties, i.e. you can't set
	// the buffer size until you set the screen size, but you can't change the screen size
	// until the buffer size is correct. This coupled with a precise ordering of calls
	// makes this procedure seem a little mystical :-P. Thanks to wowLinh for helping - Jx9


	// Change console visual size to a minimum so ScreenBuffer can shrink
	// below the actual visual size
	m_windowRect = { 0, 0, 1, 1 };
	SetConsoleWindowInfo(m_hConsoleOut, TRUE, &m_windowRect);

	// Set the size of the screen buffer
	COORD coords = { (short)m_nScreenWidth, (short)m_nScreenHeight };
	if (!SetConsoleScreenBufferSize(m_hConsoleOut, coords))
		return Error(L"ConstructConsole::SetConsoleScreenBufferSize");

	// Assign screen buffer to the console
	if (!SetConsoleActiveScreenBuffer(m_hConsoleOut))
		return Error(L"ConstructConsole::SetConsoleActiveScreenBuffer");

	// Set the font size now that the screen buffer has been assigned to the console
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = fontw;
	cfi.dwFontSize.Y = fonth;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;

	wcscpy_s(cfi.FaceName, L"Consolas");
	if (!SetCurrentConsoleFontEx(m_hConsoleOut, false, &cfi))
		return Error(L"ConstructConsole::SetCurrentConsoleFontEx");

	// Get screen buffer info and check the maximum allowed window size. Return
	// error if exceeded, so user knows their dimensions/fontsize are too large
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(m_hConsoleOut, &csbi))
		return Error(L"ConstructConsole::GetConsoleScreenBufferInfo");
	else if (m_nScreenHeight < csbi.dwMaximumWindowSize.Y)
		return Error(L"Screen Height/Font Height too big...");
	else if (m_nScreenWidth < csbi.dwMaximumWindowSize.X)
		return Error(L"Screen Width/Font Width too big...");

	m_windowRect = { 0, 0, (short)m_nScreenWidth - 1, (short)m_nScreenHeight - 1 };
	if (!SetConsoleWindowInfo(m_hConsoleOut, TRUE, &m_windowRect))
		return Error(L"ConstructConsole::SetConsoleWindowInfo");

	// Set flags to allow mouse input
	if (!SetConsoleMode(m_hConsoleIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT))
		return Error(L"ConstructConsole::SetConsoleMode");

	m_screenBuffer = new CHAR_INFO[m_nScreenHeight * m_nScreenWidth];
	std::memset(m_screenBuffer, 0, sizeof(CHAR_INFO) * m_nScreenHeight * m_nScreenWidth);

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CloseHandler, TRUE);

	return true;
}

void ConsoleGameEngine::Draw(int x, int y, short c, short col)
{
	if (x >= 0 && x <= m_nScreenWidth && y >= 0 && y <= m_nScreenHeight)
	{
		m_screenBuffer[y * m_nScreenWidth + x].Char.UnicodeChar = c;
		m_screenBuffer[y * m_nScreenWidth + x].Attributes = col;
	}
}

void approxCoord(float &coord_i, int &coord_o, float offset)
{
	float cOff = (coord_i + offset);
	float cDecPart = cOff - cOff;
	if (cDecPart < 0.5)
		coord_o = std::floor<float, int>(cOff);
	else
		coord_o = std::floor<float, int>(cOff);
}
	
void ConsoleGameEngine::DrawLineA(int x1, int y1, int x2, int y2, short c, short col)
{
	int dx = x2 - x1;
	int dy = y2 - y1;

	float xOffset = 0.0f;
	float yOffset = 0.0f;

	//float m = dy / dx;
	int dxAbs = std::abs(dx);
	int dyAbs = std::abs(dy);
	int steps = (dxAbs > dyAbs) ? dxAbs : dyAbs;

	if (dx != 0)
		xOffset = dx / steps;
	if (dy != 0)
		yOffset = dy / steps;
	
	int y = y1;
	int x = x1;

	for (short i = 0; i < steps; ++i)
	{
		Draw(x, y, c, col);

		float xOff = (x + xOffset);
		float xDecPart = xOff - xOff;

		if (xDecPart != 0.0f && xDecPart < 0.5)
			x = std::floor<float, int>(xOff);
		else
			x = std::floor<float, int>(xOff);

		float yOff = (y + yOffset);
		float yDecPart = yOff - yOff;

		if (xDecPart != 0.0f && yDecPart < 0.5)
			y = std::floor<float, int>(yOff);
		else
			y = std::floor<float, int>(yOff);
	}
}

void ConsoleGameEngine::DrawLine(int x1, int y1, int x2, int y2, short c, short col)
{
	int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;

	dx = x2 - x1;
	dy = y2 - y1;

	dx1 = abs(dx);
	dy1 = abs(dy);

	px = 2 * dy1 - dx1;
	py = 2 * dx1 - dy1;

	if (dy1 <= dx1)
	{
		if (dx >= 0)
		{
			x = x1;
			y = y1;
			xe = x2;
		}
		else
		{
			x = x2;
			y = y2;
			xe = x1;
		}

		Draw(x, y, c, col);

		for (i = 0; x < xe; i++)
		{
			x = x + 1;
			if (px < 0)
				px = px + 2 * dy1;
			else
			{
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0	))
					y = y + 1;
				else
					y = y - 1;

				px = px + 2 * (dy1 - dx1);
			}
			Draw(x, y, c, col);
		}
	}
	else
	{
		if (dy >= 0)
		{
			x = x1;
			y = y1; 
			ye = y2;
		}
		else
		{
			x = x2;
			y = y2;
			ye = y1;
		}

		Draw(x, y, c, col);
		for (i = 0; y < ye; i++)
		{
			y = y + 1;
			if (py <= 0)
				py = py + 2 * dx1;
			else
			{
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) 
					x = x + 1; 
				else 
					x = x - 1;
				py = py + 2 * (dx1 - dy1);
			}
			Draw(x, y, c, col);
		}
	}
}

void ConsoleGameEngine::DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short c, short col)
{
	//Clip(x1, y1);
	//Clip(x2, y2);
	//Clip(x3, y3);
	DrawLineA(x1, y1, x2, y2, c, col);
	DrawLineA(x2, y2, x3, y3, c, col);
	DrawLineA(x3, y3, x1, y1, c, col);
}

void ConsoleGameEngine::DrawCircle(int xc, int yc, int r, short c, short col)
{
	int x = 0;
	int y = r;
	int p = 3 - 2 * r;
	if (!r)
		return;

	while (y >= x) // only formulate 1/8 of circle
	{
		Draw(xc - x, yc - y, c, col);//upper left left
		Draw(xc - y, yc - x, c, col);//upper upper left
		Draw(xc + y, yc - x, c, col);//upper upper right
		Draw(xc + x, yc - y, c, col);//upper right right
		Draw(xc - x, yc + y, c, col);//lower left left
		Draw(xc - y, yc + x, c, col);//lower lower left
		Draw(xc + y, yc + x, c, col);//lower lower right
		Draw(xc + x, yc + y, c, col);//lower right right

		if (p < 0) 
			p += 4 * x++ + 6;
		else
			p += 4 * (x++ - y--) + 10;
	}
}

void ConsoleGameEngine::DrawString(int x, int y, std::wstring c, short col)
{
	for (size_t i = 0; i < c.size(); i++)
	{
		m_screenBuffer[y * m_nScreenWidth + x + i].Char.UnicodeChar = c[i];
		m_screenBuffer[y * m_nScreenWidth + x + i].Attributes = col;
	}
}

void ConsoleGameEngine::DrawStringAlpha(int x, int y, std::wstring c, short col)
{
	for (size_t i = 0; i < c.size(); i++)
	{
		if (c[i] != L' ')
		{
			m_screenBuffer[y * m_nScreenWidth + x + i].Char.UnicodeChar = c[i];
			m_screenBuffer[y * m_nScreenWidth + x + i].Attributes = col;
		}
	}
}

void ConsoleGameEngine::DrawSprite(int x, int y, Sprite * sp)
{
	if (sp == nullptr)
		return;

	for (int i = 0; i < sp->GetWidth(); i++)
	{
		for (int j = 0; j < sp->GetHeight(); j++)
		{
			if (sp->GetGlyph(i, j) != L' ')
				Draw(x + i, y + j, sp->GetGlyph(i, j), sp->GetColour(i, j));
		}
	}
}

void ConsoleGameEngine::DrawPartialSprite(int x, int y, Sprite * sp, int ox, int oy, int w, int h)
{
	if (sp == nullptr)
		return;

	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			if (sp->GetGlyph(i + ox, j + oy) != L' ')
				Draw(x + i, y + j, sp->GetGlyph(i + ox, j + oy), sp->GetColour(i + ox, j + oy));
		}
	}
}

void ConsoleGameEngine::DrawWireFrameModel(const std::vector<std::pair<float, float>>& vModelCoordinates, float x, float y, float r, float s, short col, short c)
{
	// Create translated model vector of coordinate pairs
	std::vector<std::pair<float, float>> vecTransformedCoordinates;
	int verts = vModelCoordinates.size();
	vecTransformedCoordinates.resize(verts);

	// Rotate
	for (int i = 0; i < verts; i++)
	{
		vecTransformedCoordinates[i].first = vModelCoordinates[i].first * cosf(r) - vModelCoordinates[i].second * sinf(r);
		vecTransformedCoordinates[i].second = vModelCoordinates[i].first * sinf(r) + vModelCoordinates[i].second * cosf(r);
	}

	// Scale
	for (int i = 0; i < verts; i++)
	{
		vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first * s;
		vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second * s;
	}

	// Translate
	for (int i = 0; i < verts; i++)
	{
		vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first + x;
		vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second + y;
	}

	// Draw Closed Polygon
	for (int i = 0; i < verts + 1; i++)
	{
		int j = (i + 1);
		DrawLine((int)vecTransformedCoordinates[i % verts].first, (int)vecTransformedCoordinates[i % verts].second,
			(int)vecTransformedCoordinates[j % verts].first, (int)vecTransformedCoordinates[j % verts].second, c, col);
	}
}

void ConsoleGameEngine::Fill(int x1, int x2, int y1, int y2, short c, short col)
{
	Clip(x1, y1);
	Clip(x2, y2);
	for (int x = x1; x < x2; x++)
		for (int y = y1; y < y2; y++)
			Draw(x, y, c, col);
}

void ConsoleGameEngine::FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short c, short col)
{
	auto SWAP = [](int &x, int &y) 
	{
		int t = x; 
		x = y; 
		y = t;
	};

	auto drawline = [&](int sx, int ex, int ny)
	{ 
		for (int i = sx; i <= ex; i++) 
			Draw(i, ny, c, col); 
	};

	int t1x, t2x, y, minx, maxx, t1xp, t2xp;
	bool changed1 = false;
	bool changed2 = false;
	int signx1, signx2, dx1, dy1, dx2, dy2;
	int e1, e2;

	// Sort vertices
	if (y1>y2) 
	{ 
		SWAP(y1, y2); 
		SWAP(x1, x2);
	}
	if (y1>y3)
	{
		SWAP(y1, y3); 
		SWAP(x1, x3);
	}
	if (y2>y3)
	{ 
		SWAP(y2, y3); 
		SWAP(x2, x3); 
	}

	// Starting points
	t1x = t2x = x1; 
	y = y1;  

	dx1 = (int)(x2 - x1); 

	if (dx1<0) 
	{ 
		dx1 = -dx1; 
		signx1 = -1; 
	}
	else 
		signx1 = 1;

	dy1 = (int)(y2 - y1);

	dx2 = (int)(x3 - x1);

	if (dx2<0)
	{ 
		dx2 = -dx2; 
		signx2 = -1;
	}
	else 
		signx2 = 1;

	dy2 = (int)(y3 - y1);

	if (dy1 > dx1)
	{   // swap values
		SWAP(dx1, dy1);
		changed1 = true;
	}
	if (dy2 > dx2) 
	{   // swap values
		SWAP(dy2, dx2);
		changed2 = true;
	}

	e2 = (int)(dx2 >> 1);

	// Flat top, just process the second half
	if (y1 == y2) 
		goto next;

	e1 = (int)(dx1 >> 1);

	for (int i = 0; i < dx1;)
	{
		t1xp = 0; t2xp = 0;
		if (t1x<t2x)
		{ 
			minx = t1x; 
			maxx = t2x; 
		}
		else
		{ 
			minx = t2x;
			maxx = t1x; 
		}
		// process first line until y value is about to change
		while (i<dx1) 
		{
			i++;
			e1 += dy1;
			while (e1 >= dx1) 
			{
				e1 -= dx1;
				if (changed1) 
					t1xp = signx1;//t1x += signx1;
				else          
					goto next1;
			}
			if (changed1)
				break;
			else 
				t1x += signx1;
		}
		// Move line
	next1:
		// process second line until y value is about to change
		while (1) 
		{
			e2 += dy2;
			while (e2 >= dx2)
			{
				e2 -= dx2;

				if (changed2)
					t2xp = signx2;//t2x += signx2;
				else          
					goto next2;
			}
			if (changed2) 
				break;
			else            
				t2x += signx2;
		}
	next2:
		if (minx>t1x)
			minx = t1x;
		if (minx>t2x)
			minx = t2x;
		if (maxx<t1x) 
			maxx = t1x;
		if (maxx<t2x) 
			maxx = t2x;

		drawline(minx, maxx, y);    // Draw line from min to max points found on the y
									// Now increase y
		if (!changed1)
			t1x += signx1;

		t1x += t1xp;

		if (!changed2) 
			t2x += signx2;

		t2x += t2xp;
		y += 1;
		if (y == y2) 
			break;

	}
next:
	// Second half
	dx1 = (int)(x3 - x2); 
	if (dx1<0) 
	{ 
		dx1 = -dx1; 
		signx1 = -1; 
	}
	else
		signx1 = 1;

	dy1 = (int)(y3 - y2);
	t1x = x2;

	if (dy1 > dx1)
	{   // swap values
		SWAP(dy1, dx1);
		changed1 = true;
	}
	else changed1 = false;

	e1 = (int)(dx1 >> 1);

	for (int i = 0; i <= dx1; i++) 
	{
		t1xp = 0; 
		t2xp = 0;
		if (t1x<t2x) 
		{ 
			minx = t1x;
			maxx = t2x; 
		}
		else
		{ 
			minx = t2x;
			maxx = t1x;
		}
		// process first line until y value is about to change
		while (i<dx1) 
		{
			e1 += dy1;
			while (e1 >= dx1)
			{
				e1 -= dx1;
				if (changed1) 
				{ 
					t1xp = signx1; 
					break; 
				}//t1x += signx1;
				else       
					goto next3;
			}
			if (changed1) 
				break;
			else   	   	 
				t1x += signx1;
			if (i<dx1) 
				i++;
		}
	next3:
		// process second line until y value is about to change
		while (t2x != x3)
		{
			e2 += dy2;
			while (e2 >= dx2) 
			{
				e2 -= dx2;
				if (changed2)
					t2xp = signx2;
				else         
					goto next4;
			}

			if (changed2) 
				break;
			else 
				t2x += signx2;
		}
	next4:

		if (minx>t1x) 
			minx = t1x; 

		if (minx>t2x) 
			minx = t2x;

		if (maxx<t1x) 
			maxx = t1x; 

		if (maxx<t2x)
			maxx = t2x;

		drawline(minx, maxx, y);

		if (!changed1) 
			t1x += signx1;

		t1x += t1xp;

		if (!changed2) 
			t2x += signx2;

		t2x += t2xp;
		y += 1;

		if (y>y3) 
			return;
	}
}

void ConsoleGameEngine::FillCircle(int xc, int yc, int r, short c, short col)
{
	// Taken from wikipedia
	int x = 0;
	int y = r;
	int p = 3 - 2 * r;
	if (!r) 
		return;

	auto drawline = [&](int sx, int ex, int ny)
	{
		for (int i = sx; i <= ex; i++)
			Draw(i, ny, c, col);
	};

	while (y >= x)
	{
		// Modified to draw scan-lines instead of edges
		drawline(xc - x, xc + x, yc - y);
		drawline(xc - y, xc + y, yc - x);
		drawline(xc - x, xc + x, yc + y);
		drawline(xc - y, xc + y, yc + x);

		if (p < 0)
			p += 4 * x++ + 6;
		else
			p += 4 * (x++ - y--) + 10;
	}
}

void ConsoleGameEngine::Clip(int & x, int & y)
{
	if (x < 0)
		x = 0;

	if (x >= m_nScreenWidth)
		x = m_nScreenWidth;

	if (y < 0) 
		y = 0;

	if (y >= m_nScreenHeight) 
		y = m_nScreenHeight;
}

int ConsoleGameEngine::Error(const wchar_t * msg)
{
	wchar_t buf[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
	SetConsoleActiveScreenBuffer(m_hOriginalConsole);
	wprintf(L"ERROR: %s\n\t%s\n", msg, buf);

	return 0;
}


void ConsoleGameEngine::gameThread()
{
	// Create user resources as part of this thread
	if (!OnUserCreate())
		m_bAtomicSwitch = false;

	// Check if sound system should be enabled
	if (m_bEnableSound)
	{
		m_pAudioEngine = new AudioEngine;
		if (!m_pAudioEngine || !m_pAudioEngine->CreateAudio())
		{
			m_bAtomicSwitch = false; // Failed to create audio system			
			m_bEnableSound = false;
		}
	}

	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();

	while (m_bAtomicSwitch)
	{
		// Run as fast as possible
		while (m_bAtomicSwitch)
		{
			// Handle Timing
			tp2 = std::chrono::system_clock::now();
			std::chrono::duration<float> elapsedTime = tp2 - tp1;
			tp1 = tp2;
			float fElapsedTime = elapsedTime.count();

			// Handle Keyboard Input
			for (int i = 0; i < 256; i++)
			{
				m_keyNewState[i] = GetAsyncKeyState(i);

				m_keys[i].bPressed = false;
				m_keys[i].bReleased = false;

				if (m_keyNewState[i] != m_keyOldState[i])
				{
					if (m_keyNewState[i] & 0x8000)
					{
						m_keys[i].bPressed = !m_keys[i].bHeld;
						m_keys[i].bHeld = true;
					}
					else
					{
						m_keys[i].bReleased = true;
						m_keys[i].bHeld = false;
					}
				}

				m_keyOldState[i] = m_keyNewState[i];
			}

			// Handle Mouse Input - Check for window events
			INPUT_RECORD inBuf[32];
			DWORD events = 0;
			GetNumberOfConsoleInputEvents(m_hConsoleIn, &events);
			if (events > 0)
				ReadConsoleInput(m_hConsoleIn, inBuf, events, &events);

			// Handle events - we only care about mouse clicks and movement
			// for now
			for (DWORD i = 0; i < events; i++)
			{
				switch (inBuf[i].EventType)
				{
				case FOCUS_EVENT:
				{
					m_isConsoleInFocus = inBuf[i].Event.FocusEvent.bSetFocus;
				}
				break;

				case MOUSE_EVENT:
				{
					switch (inBuf[i].Event.MouseEvent.dwEventFlags)
					{
					case MOUSE_MOVED:
					{
						m_mousePosX = inBuf[i].Event.MouseEvent.dwMousePosition.X;
						m_mousePosY = inBuf[i].Event.MouseEvent.dwMousePosition.Y;
					}
					break;

					case 0:
					{
						for (int m = 0; m < 5; m++)
							m_mouseNewState[m] = (inBuf[i].Event.MouseEvent.dwButtonState & (1 << m)) > 0;

					}
					break;

					default:
						break;
					}
				}
				break;

				default:
					break;
					// We don't care just at the moment
				}
			}

			for (int m = 0; m < 5; m++)
			{
				m_mouse[m].bPressed = false;
				m_mouse[m].bReleased = false;

				if (m_mouseNewState[m] != m_mouseOldState[m])
				{
					if (m_mouseNewState[m])
					{
						m_mouse[m].bPressed = true;
						m_mouse[m].bHeld = true;
					}
					else
					{
						m_mouse[m].bReleased = true;
						m_mouse[m].bHeld = false;
					}
				}

				m_mouseOldState[m] = m_mouseNewState[m];
			}


			// Handle Frame Update
			if (!OnUserUpdate(fElapsedTime))
				m_bAtomicSwitch = false;

			// Update Title & Write Screen Buffer
			bool bRenderStatus = false;
			wchar_t s[256];
			
			swprintf_s(s, 256, L"Vlad - Console Game engine - %s - FPS: %3.2f", m_sAppName.c_str(), 1.0f / fElapsedTime);
			SetConsoleTitle(s);

			bRenderStatus = WriteConsoleOutput(m_hConsoleOut, m_screenBuffer, { (short)m_nScreenWidth, (short)m_nScreenHeight }, { 0,0 }, &m_windowRect);

			if (!bRenderStatus)
				Error(L"Failed to write screen buffer...");
		}

		if (m_bEnableSound)
		{
			// Close and Clean up audio system
			m_bEnableSound = false;
			delete m_pAudioEngine;
		}

		// Allow the user to free resources if they have overrided the destroy function
		if (OnUserDestroy())
		{
			// User has permitted destroy, so exit and clean up
			for (size_t i = 0; i < m_nScreenHeight * m_nScreenWidth; ++i)
			{
				m_screenBuffer[i].Char.UnicodeChar = 0;
				m_screenBuffer[i].Attributes = 0;
			}

			delete[] m_screenBuffer;
			SetConsoleActiveScreenBuffer(m_hOriginalConsole);
			m_cvQuitGame.notify_one();
		}
		else
		{
			// User denied destroy for some reason, so continue running
			m_bAtomicSwitch = true;
		}
	}
}
