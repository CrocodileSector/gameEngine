/*
This is how a main using the game engine should look

int main()
{
	// Use olcConsoleGameEngine derived app
	Console3DEngine game;

	// Create a console with resolution 160x100 characters
	// Each character occupies 8x8 pixels
	game.ConstructConsole(160, 100, 8, 8);

	// Start the engine!
	game.Start();

	return 0;
}


*/
#include <iostream>
#include "Console3DEngine.h"

struct vec3D
{
	float x, y, z;
};

struct triangle
{
	vec3D v[3]; //v for vertices... and vendetta
};

struct mesh
{
	std::vector<triangle> tris;
};

struct translationMatrix
{
	float m[4][4] = { 0 };
};


class GameEngine : public ConsoleGameEngine
{
	float fTheta = 0.0f;
	mesh m_mCube;
	translationMatrix m_projectionMatrix;
	translationMatrix m_rotationZ, m_rotationX;

	float DegreeToRadiants(float f)
	{
		return f * 0.5f / 180.0f * 3.14159f;
	}

	void TranslateVector(vec3D &in, vec3D &out, translationMatrix& m)
	{
		out.x = in.x * m.m[0][0] + in.y * m.m[1][0] + in.z * m.m[2][0] + m.m[3][0];
		out.y = in.x * m.m[0][1] + in.y * m.m[1][1] + in.z * m.m[2][1] + m.m[3][1];
		out.z = in.x * m.m[0][2] + in.y * m.m[1][2] + in.z * m.m[2][2] + m.m[3][2];
		float w = in.x * m.m[0][2] + in.y * m.m[1][2] + m.m[2][2] + in.z * m.m[3][3]; //get Z to divide by it

		if (w != 0.0f)
		{
			out.x /= w;
			out.y /= w;
			out.z /= w;
		}
	}

public :
	GameEngine() { m_sAppName = L"bao"; }

	bool OnUserCreate() override
	{
		m_mCube.tris =
		{
			{0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f}, //SOUTH

			{1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f},
			{1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f}, //EAST

			{0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f},
			{0.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f}, //NORTH

			{0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f}, //WEST

			{0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f},
			{0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f}, //TOP

			{0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 1.0f,    1.0f, 0.0f, 0.0f}, //BOT

		};

		float fov = 90.0f;
		float zFar = 1000.0f;
		float zNear = 0.1f;
		float aspectRatio = (float)ScreenHeight() / (float)ScreenWidth();
		float tanEq = 1.0f / (tanf(fov * 0.5f / 180.0f * 3.14159f));
		float zNormalization = zFar / zFar - zNear;
		float adjustedZnorm = -zNear * zNormalization;

		m_projectionMatrix.m[0][0] = aspectRatio * tanEq;
		m_projectionMatrix.m[1][1] = tanEq;
		m_projectionMatrix.m[2][2] = zNormalization;
		m_projectionMatrix.m[3][2] = adjustedZnorm;
		m_projectionMatrix.m[2][3] = 1;
		
		m_rotationZ.m[2][2] = 1.0f;
		m_rotationZ.m[3][3] = 1.0f;

		m_rotationX.m[0][0] = 1.0f;
		m_rotationX.m[3][3] = 1.0f;

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Fill(0, ScreenWidth(), 0, ScreenHeight(), PIXEL_SOLID, FG_BLACK);

		fTheta += 1.0f * fElapsedTime;

		m_rotationZ.m[0][0] = cosf(fTheta);
		m_rotationZ.m[0][1] = sinf(fTheta);
		m_rotationZ.m[1][0] = -sinf(fTheta);
		m_rotationZ.m[1][1] = cosf(fTheta);

		m_rotationX.m[1][1] = cosf(fTheta * 0.5f);
		m_rotationX.m[1][2] = sinf(fTheta * 0.5f);
		m_rotationX.m[2][1] = -sinf(fTheta * 0.5f);
		m_rotationX.m[2][2] = cosf(fTheta * 0.5f);

		for (auto tri : m_mCube.tris)
		{
			triangle projectedTri, translatedTri, rotatedZtri, rotatedXtri;

			TranslateVector(tri.v[0], rotatedZtri.v[0], m_rotationZ);
			TranslateVector(tri.v[1], rotatedZtri.v[1], m_rotationZ);
			TranslateVector(tri.v[2], rotatedZtri.v[2], m_rotationZ);

			TranslateVector(rotatedZtri.v[0], rotatedXtri.v[0], m_rotationX);
			TranslateVector(rotatedZtri.v[1], rotatedXtri.v[1], m_rotationX);
			TranslateVector(rotatedZtri.v[2], rotatedXtri.v[2], m_rotationX);

			translatedTri = rotatedXtri;
			translatedTri.v[0].z = rotatedXtri.v[0].z + 3.0f;
			translatedTri.v[1].z = rotatedXtri.v[1].z + 3.0f;
			translatedTri.v[2].z = rotatedXtri.v[2].z + 3.0f;

			TranslateVector(translatedTri.v[0], projectedTri.v[0], m_projectionMatrix);
			TranslateVector(translatedTri.v[1], projectedTri.v[1], m_projectionMatrix);
			TranslateVector(translatedTri.v[2], projectedTri.v[2], m_projectionMatrix);

			projectedTri.v[0].x += 1; projectedTri.v[0].y += 1;
			projectedTri.v[1].x += 1; projectedTri.v[1].y += 1;
			projectedTri.v[2].x += 1; projectedTri.v[2].y += 1;

			projectedTri.v[0].x *= 0.3f * (float)ScreenWidth();
			projectedTri.v[0].y *= 0.3f * (float)ScreenHeight();
			projectedTri.v[1].x *= 0.3f * (float)ScreenWidth();
			projectedTri.v[1].y *= 0.3f * (float)ScreenHeight();
			projectedTri.v[2].x *= 0.3f * (float)ScreenWidth();
			projectedTri.v[2].y *= 0.3f * (float)ScreenHeight();

			DrawTriangle(projectedTri.v[0].x, projectedTri.v[0].y,
				projectedTri.v[1].x, projectedTri.v[1].y,
				projectedTri.v[2].x, projectedTri.v[2].y,
				PIXEL_SOLID, FG_WHITE);
		}

		return true;
	}
};


int main()
{
	GameEngine game;

	if (game.ConstructConsole(480, 254, 4, 4))
		game.Start();

    return 0;
}

