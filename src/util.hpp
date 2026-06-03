#pragma once

namespace based_renderer
{
	// TODO: How does printing to the debug output work on other systems?

	// Works just like std::print, except it prints to the debug console.
	template<class... Args> 
	static void dprint(std::format_string<Args...> fmt, Args&&... args) noexcept
	{
		std::string s = std::format(fmt, std::forward<Args>(args)...);
		OutputDebugStringA(s.c_str());
	}

	// Same, but the format string is a wide string.
	template<class... Args>
	static void dprint(std::wformat_string<Args...> fmt, Args&&... args) noexcept
	{
		std::wstring s = std::format(fmt, std::forward<Args>(args)...);
		OutputDebugStringW(s.c_str());
	}

	// A clever way I found to remove an element from an std::vector.
	// Assumes that i is within the bounds of v.
	template <class T>
	static void unordered_remove(std::vector<T> &v, size_t const i) noexcept
	{
		v[i] = v.back();
		v.pop_back();
	}

	static std::string to_string(std::vector<std::string> const &v) noexcept
	{
		std::string res;
		if (v.size() > 0)
		{
			for (size_t i = 0; i < v.size() - 1; ++i)
			{
				res += v[i] + ", ";
			}
			res += v.back();
		}
		return res;
	}

	// static std::string read_entire_file(std::string const &path)
	// {
	// 	std::string res;

	// 	std::ifstream file(path);
	// 	if (!file)
	// 	{
	// 		std::string error = "Failed to load " + path + ".";
	// 		throw std::runtime_error{error};
	// 	}

	// 	std::ostringstream buffer;
	// 	buffer << file.rdbuf();
	// 	res = buffer.str();

	// 	return res;
	// }
}