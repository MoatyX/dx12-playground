#pragma once

#include "dx_app_base.h"
#include "simple_input.h"

#if defined(_DEBUG) || defined(DEBUG)
#define WIN32_ALLOC_CONSOLE true
#else
#define WIN32_ALLOC_CONSOLE false
#endif

/**
 * \brief simple static class that creates a window to run dx12 apps inside
 */
class win32_app
{
public:
	/**
	 * @brief creates a Win32 Window and runs the dx_app's main loop
	 * @param alloc_console whether or not to make a console with the window
	 */
	static int run(dx_app_base* dx_app, HINSTANCE hinstance, int cmd_show, bool alloc_console);
	static HWND get_window();
	static float get_aspect_ratio();

protected:
	/// <summary>
	/// process win32 Window events and drive the dx app
	/// </summary>
	/// <param name="window"></param>
	/// <param name="message"></param>
	/// <param name="wparam"></param>
	/// <param name="lparam"></param>
	/// <returns></returns>
	static LRESULT CALLBACK window_events(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

private:
	static HWND m_window_;
	static float m_width_;
	static float m_height_;

	static bool cmd_;
};

