#include "win32_app.h"
#include "constBuffDemo.h"

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int show)
{
	constBuffDemo buff(800, 600, L"Constant Buffer Demo");
	win32_app::run(&buff, hinstance, show, false);
}