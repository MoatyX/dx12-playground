#include "dx_app_base.h"
#include "exceptions.h"
#include "win32_app.h"

int WINAPI WinMain(const HINSTANCE inst, HINSTANCE, LPSTR, const int cmd_show)
{
	int return_code = 0;
	dx_app_base app(1280, 720, L"Window App");
	try
	{
		return_code = win32_app::run(&app, inst, cmd_show, WIN32_ALLOC_CONSOLE);
	}
	catch (dx12_exception& e)
	{
		MessageBox(win32_app::get_window(), e.to_string().c_str(), L"Fatal Runtime Error", MB_OK);
		return -1;
	}
	
	return return_code;
}
