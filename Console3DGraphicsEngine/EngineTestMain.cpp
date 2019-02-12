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

class GameEngine : public ConsoleGameEngine
{
public :
	GameEngine()
	{

	}

public :
	bool OnUserCreate() override
	{
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		return true;
	}
};


int main()
{
	GameEngine game;

	if (game.ConstructConsole(256, 240, 4, 4))
		game.Start();

    return 0;
}

