#include "win32_app.h"
#include <fcntl.h>
#include <iostream>

void update_keys_state(simple_input& input, WPARAM new_key, simple_input::key_state new_state)
{
	input.last_key_ = new_key;
	input.last_key_state_ = new_state;
}

HWND win32_app::m_window_ = nullptr;

int win32_app::run(dx_app_base* dx_app, HINSTANCE hinstance, int cmd_show, bool alloc_cmd = false)
{
	//parse cmd parameters if any.
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	dx_app->parse_cmd(argv, argc);
	LocalFree(argv);

	//init the window class
	WNDCLASSEX window_class = { 0 };
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
	window_class.hInstance = hinstance;
	window_class.lpfnWndProc = window_events;
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = L"dx12App";
	RegisterClassEx(&window_class);

	//set window dimensions
	RECT window_rect = { 0, 0, static_cast<LONG>(dx_app->get_width()), static_cast<LONG>(dx_app->get_height()) };
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, false);

	//create the window and store a handle to it
	m_window_ = CreateWindow(window_class.lpszClassName, dx_app->get_title(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		window_rect.right - window_rect.left,
		window_rect.bottom - window_rect.top, nullptr, nullptr, hinstance, dx_app);

	if (alloc_cmd)
	{		
		AllocConsole();
		freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);	//redirect stdio to the new console

		std::wstring title(dx_app->get_title());
		title += L" Console";
		SetConsoleTitle(title.c_str());
	}

	//after creating and initialing the window, now init the directX app
	dx_app->init_core();

	ShowWindow(m_window_, cmd_show);

	//main loop
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		//process win32 messages
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	//when the win32 application quits, do clean up
	dx_app->on_destroy();

	
	if (alloc_cmd) {
		FreeConsole();
	}

	return static_cast<char>(msg.wParam);
}

HWND win32_app::get_window()
{
	return m_window_;
}

float win32_app::m_width_ = 0;
float win32_app::m_height_ = 0;
float win32_app::get_aspect_ratio()
{
	return (static_cast<float>(m_width_) / static_cast<float>(m_height_));
}

LRESULT win32_app::window_events(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	static simple_input* input = simple_input::instance();
	dx_app_base* dx_app = reinterpret_cast<dx_app_base*>(GetWindowLongPtr(window, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE: //cache a pointer to the app, by resetting it as a window parameter
		{
			LPCREATESTRUCT create_window_struct = reinterpret_cast<LPCREATESTRUCT>(lparam);			//get the struct we filled out using CreateWindow()
			SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create_window_struct->lpCreateParams));	//reset the CreateParam	
		}
		return 0;
	case WM_PAINT:
		dx_app->on_update();			//update game state
		dx_app->on_pre_render();		//render new game state
		dx_app->on_render();
		dx_app->on_post_render();
		return 0;
	case WM_CLOSE:
		FreeConsole();
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		if (input)	
		{
			update_keys_state(*input, wparam, simple_input::key_state::down);
		}
		return 0;
	case WM_KEYUP:
		if (input)
		{
			update_keys_state(*input, wparam, simple_input::key_state::up);
		}
		return 0;
	case WM_SIZE:
		m_width_ = LOWORD(lparam);
		m_height_ = HIWORD(lparam);
		return 0;
	default:
		break;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(window, message, wparam, lparam);
}
