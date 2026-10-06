/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace std::literals;

// CommonLib callbacks take the object by reference in CharmedBaryon's build and by pointer in alandtse's: a generic lambda plus this works with both
namespace compat
{
	template <class T>
	inline T* Ptr(T& a_ref) { return &a_ref; }
	template <class T>
	inline T* Ptr(T* a_ptr) { return a_ptr; }
}
