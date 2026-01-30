#include "pch.hpp"

template<class... Args> void 
dprint(std::format_string<Args...> fmt, Args&&... args) 
{
	std::string s = std::format(fmt, std::forward<Args>(args)...);
	OutputDebugStringA(s.c_str());
}

static bool running;

LRESULT __stdcall Wndproc(
	HWND   hWnd,
	UINT   Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	LRESULT res = 0;

	switch (Msg)
	{
		case WM_DESTROY:
		case WM_CLOSE: {
			running = false;
		} break;
		default: {
			res = DefWindowProcW(hWnd, Msg, wParam, lParam);
		} break;
	}

	return res;
}

int __stdcall WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd) 
{
	UNUSED(hPrevInstance);
	UNUSED(lpCmdLine);
	UNUSED(nShowCmd);

	vk::ApplicationInfo app_info{};
	vk::InstanceCreateInfo instance_create_info {
		{},
		&app_info,
	};
	vk::Instance instance = vk::createInstance(instance_create_info);

	WNDCLASSEXW window_class 
	{
		.cbSize = sizeof(WNDCLASSEXW),
		.style = 0,
		.lpfnWndProc = Wndproc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = hInstance,
		.hIcon = nullptr,
		.hCursor = nullptr,
		.hbrBackground = nullptr,
		.lpszMenuName = nullptr,
		.lpszClassName = L"based_renderer",
		.hIconSm = nullptr,
	};
	if (!RegisterClassExW(&window_class))
	{
		return -1;
	}

	HWND window 
	{ 
		CreateWindowExW(
			0,
			L"based_renderer",
			L"based_renderer",
			WS_OVERLAPPEDWINDOW,
			1920/4,
			1080/4,
			1920/2,
			1080/2,
			nullptr,
			nullptr,
			hInstance,
			nullptr
		) 
	};
	if (!window)
	{
		return -2;
	}

	if (!DestroyWindow(window))
	{
		return -3;
	}
	if (!UnregisterClassW(window_class.lpszClassName, window_class.hInstance))
	{
		return -4;
	}
	instance.destroy();
	
	return 0;
}