#pragma once

#include "stdafx.h"

class Keyboard
{
public:
	Keyboard();
	~Keyboard();

	void KeyDown(WPARAM Key);

	// Return Last Key
	const WPARAM& GetLastKey() const { return LastKey; }

private:
	// Last KeyPressed
	WPARAM LastKey;
};

