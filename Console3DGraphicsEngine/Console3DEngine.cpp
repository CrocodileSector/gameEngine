#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif 

#pragma comment(lib, "winmm.lib")

#include "Console3DEngine.h"
#include "EngineDefines.h"

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
		return;
	else
		return m_glyphs[y * m_nWidth + x];
}

short Sprite::GetColour(int x, int y)
{
	if (x < 0 || x > m_nWidth || y < 0 || y > m_nHeight)
		return;
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
}

ConsoleGameEngine::~ConsoleGameEngine()
{
}

void ConsoleGameEngine::Start()
{
}

void ConsoleGameEngine::EnableSound()
{
}

int ConsoleGameEngine::ConstructConsole()
{
	return 0;
}

void ConsoleGameEngine::Draw(int x, int y, short c, short col)
{
}

void ConsoleGameEngine::DrawLine(int x1, int y1, int x2, int y2, short c, short col)
{
}

void ConsoleGameEngine::DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short c, short col)
{
}

void ConsoleGameEngine::DrawCircle(int xc, int yc, int r, short c, short col)
{
}

void ConsoleGameEngine::DrawString(int x, int y, std::wstring c, short col)
{
}

void ConsoleGameEngine::DrawStringAlpha(int x, int y, std::wstring c, short col)
{
}

void ConsoleGameEngine::DrawSprite(int x, int y, Sprite * sp)
{
}

void ConsoleGameEngine::DrawPartialSprite(int x, int y, Sprite * sp, int ox, int oy, int w, int h)
{
}

void ConsoleGameEngine::DrawWireFrameModel(const std::vector<std::pair<float, float>>& vModelCoordinates, float x, float y, float r, float s, short col, short c)
{
}

void ConsoleGameEngine::Fill(int x1, int x2, int y1, int y2, short c, short col)
{
}

void ConsoleGameEngine::FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short c, short col)
{
}

void ConsoleGameEngine::FillCircle(int xc, int yc, int r, short c, short col)
{
}

void ConsoleGameEngine::Clip(int & x, int & y)
{
}

void ConsoleGameEngine::gameThread()
{
}
