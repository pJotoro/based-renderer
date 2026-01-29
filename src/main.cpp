#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <print>
#include <system_error>

#include <vulkan/vulkan.h>

struct WindowClass 
{
	WNDCLASSEXW desc;
	
	WindowClass(WNDCLASSEXW _desc) : desc(_desc) 
	{
		if (!RegisterClassExW(&desc)) 
		{
			throw std::system_error{0};
		}
	}

	~WindowClass() 
	{
		if (!UnregisterClassW(desc.lpszClassName, desc.hInstance)) 
		{

		}
	}
};

struct Window {
	HWND handle;

	Window(
			DWORD     dwExStyle,
  			LPCWSTR   lpClassName,
  			LPCWSTR   lpWindowName,
			DWORD     dwStyle,
			int       X,
			int       Y,
			int       nWidth,
			int       nHeight,
  			HWND      hWndParent,
  			HMENU     hMenu,
  			HINSTANCE hInstance,
  			LPVOID    lpParam) 
	{
		handle = CreateWindowExW(
			dwExStyle,
			lpClassName,
			lpWindowName,
			dwStyle,
			X,
			Y,
			nWidth,
			nHeight,
			hWndParent,
			hMenu,
			hInstance,
			lpParam);
		if (!handle)
		{

		}
	}

	~Window()
	{

	}
};

int main() {
	WNDCLASSEXW window_class_desc {

	};
	WindowClass window_class {window_class_desc};

	std::println("Hello, world!");
	return 0;
}