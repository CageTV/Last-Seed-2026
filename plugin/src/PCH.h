/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
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
