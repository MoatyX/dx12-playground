#include "triangle.h"
#include "win32_app.h"

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int show)
{
	triangle tri_app(800, 600, L"Triangle");
	return win32_app::run(&tri_app, hinstance, show, true);
}