#include "pch.hpp"

// Works just like std::print, except it prints to the debug console.
template<class... Args> void 
dprint(std::format_string<Args...> fmt, Args&&... args) 
{
	std::string s = std::format(fmt, std::forward<Args>(args)...);
	OutputDebugStringA(s.c_str());
}

// TODO: Remove global.
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
	vk::InstanceCreateInfo instance_create_info 
	{
		{},
		&app_info,
	};
	vk::Instance instance = vk::createInstance(instance_create_info);

	// Choose the first discrete GPU.
	// If there is no discrete GPU, default to the last GPU.
	std::vector<vk::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();
	vk::PhysicalDevice physical_device = *std::find_if(physical_devices.begin(), physical_devices.end(), 
		[](vk::PhysicalDevice p) {
			vk::PhysicalDeviceProperties props = p.getProperties();
			return props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
		}
	);

	std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

	// TODO: This is stupid. Find out how queue priorities should be done.
	std::array<float, 64> queue_priorities {
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};

	std::vector<vk::DeviceQueueCreateInfo> device_queue_create_infos{queue_family_properties.size()};
	for (size_t i = 0; i < device_queue_create_infos.size(); ++i)
	{
		device_queue_create_infos[i] = vk::DeviceQueueCreateInfo {
			{},
			static_cast<uint32_t>(i),
			queue_family_properties[i].queueCount,
			queue_priorities.data(),
		};
	}

	vk::Device device = physical_device.createDevice(vk::DeviceCreateInfo({}, device_queue_create_infos));


	// std::vector<std::vector<vk::Queue>> queues{queue_family_properties.count()};
	// for (
	// 	size_t i = 0; i < queue_family_properties.count(); ++i)
	// {
	// 	queues[i].reserve(static_cast<size_t>(queue_family_properties[i].queueCount));
	// 	for (size_t j = 0; j < static_cast<size_t>(queue_family_properties[i].queueCount); ++j)
	// 	{
			
	// 	}
	// }

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
	device.destroy();
	instance.destroy();
	
	return 0;
}