#pragma once

#define NOMINMAX

// Window Library
#include <windows.h>

// D3D Library
#include <d3d11.h>
#include <d3dcompiler.h>
#include "DirectXTK/Inc/DDSTextureLoader.h"

// Standard Library
#include <cmath>
#include <cassert>
#include <string>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <functional>
#include <filesystem>
#include <iterator>
#include <sstream>

// Global Included
#include "Global/Types.h"
#include "Global/Memory.h"
#include "Global/Constant.h"
#include "Global/Enum.h"
#include "Global/Matrix.h"
#include "Global/Quat.h"
#include "Global/Vector.h"
#include "Global/CoreTypes.h"
#include "Global/Macro.h"
#include "Global/Function.h"
#include "Global/Name.h"
#include "Global/NameTable.h"
#include "Global/PipelineType.h"
#include "Math/AABB.h"
#include "Math/Math.h"
#include "Global/Traits.h"
#include "Manager/Time/Week05TimeClass.h"


using std::clamp;
using std::unordered_map;
using std::to_string;
using std::function;
using std::wstring;
using std::cout;
using std::cerr;
using std::min;
using std::max;
using std::exception;
using std::stoul;
using std::ofstream;
using std::ifstream;
using std::setw;
using std::sort;
using std::shared_ptr;
using std::unique_ptr;
using std::streamsize;

// File System
namespace filesystem = std::filesystem;
using filesystem::path;
using filesystem::exists;
using filesystem::create_directories;



// Library Linking
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#ifdef _DEBUG
#pragma comment(lib, "DirectXTK/Bin/Debug/DirectXTK.lib")
#else
#pragma comment(lib, "DirectXTK/Bin/Release/DirectXTK.lib")
#endif


#define IMGUI_DEFINE_MATH_OPERATORS
#include "Render/UI/Window/ConsoleWindow.h"

// DT Include Once
#ifndef TIME_MANAGER
#define TIME_MANAGER
#include "Manager/Time/TimeManager.h"
#endif // _TIME_MANAGER
