#pragma once

// Windows stuff
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if defined max
#undef max
#endif
#if defined min
#undef min
#endif

// Com stuff
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// DirectX stuff
#include <d3d12.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include <DirectXMath.h>
#include <directxcollision.h>
#include <d3dcompiler.h>
#include <wincodec.h>


// Standard stuff
#include <exception>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <chrono>
#include <functional>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <thread>
#include <memory>
#include <cstring>
#include <type_traits>
#include <queue>
#include <array>
#include <iostream>
#include <filesystem>
#include <random>


// My stuff
#include "Exceptions.h"
#include "ISingletone.h"
#include "Events.h"
#include "CommonMath.h"
#include "Hashers.h"
#include "TypeMatching.h"
#include "TypeOutput.h"
#include "TypeInput.h"
#include "TypeUtils.h"
#include "BoundingBox.h"

#define _KiB(x) (x * 1024)
#define _MiB(x) (x * 1024 * 1024)

#define _64KiB _KiB(64)
#define _1MiB _MiB(1)
#define _2MiB _MiB(2)
#define _4MiB _MiB(4)
#define _8MiB _MiB(8)
#define _16MiB _MiB(16)
#define _32MiB _MiB(32)
#define _64MiB _MiB(64)
#define _128MiB _MiB(128)
#define _256MiB _MiB(256)

#define OBLIVION_ALIGN(x) __declspec(align(x))

#define STR1(x) #x
#define STR(x) STR1(x)
#define WSTR1(x) L##x
#define WSTR(x) WSTR1(x)
#define NAME_D3D12_OBJECT(x) x->SetName( WSTR(__FILE__ "(" STR(__LINE__) "): " L#x) )

#define ENABLE_LOGS 0
#define LOG_TO_FILE 1

extern std::ofstream gLogsFile;

constexpr auto APP_VERSION = "0.0.0";

enum class BindType {
    Graphics, Compute
};

namespace Oblivion {
    constexpr const char* APPLICATION_NAME = "Game";
    constexpr const char* ENGINE_NAME = "Oblivion";
    constexpr const wchar_t* wAPPLICATION_NAME = L"Game";
    constexpr const wchar_t* wENGINE_NAME = L"Oblivion";
    
    constexpr const UINT MIN_WINDOW_SIZE = 200;

#pragma region appendToString()

    template <typename ... Args>
    constexpr auto appendToString() -> std::string {
        return "";
    }

    template <typename type>
    constexpr auto appendToString(const type& arg) -> std::string {
        if constexpr (IsIterable<type>::value && !std::is_same_v<type, std::string>) {
            std::ostringstream stream;
            stream << arg.size() << " [ ";
            unsigned int currentElement = 0;
            for (const auto& it : arg) {
                stream << it;
                currentElement++;
                if (currentElement == arg.size()) {
                    stream << " ";
                } else {
                    stream << ", ";
                }
            }
            stream << "]";
            return stream.str();
        } else {
            std::ostringstream stream;
            stream << arg;
            return stream.str();
        }
    }

    template <typename type, typename... Args>
    constexpr auto appendToString(const type& A, Args... args) -> std::string {
        std::string res1 = appendToString(A);
        std::string res2 = appendToString(args...);
        return res1 + res2;
    }

#pragma endregion

    template <typename type, typename... Args>
    constexpr auto DebugPrint(const type& arg, Args... args) {
#if DEBUG || _DEBUG || ENABLE_LOGS
        auto outputString = appendToString(arg, args...);
#if defined _USE_OUPUT_DEBUG_STRING_
        OutputDebugStringA(outputString.c_str());
#endif
        std::cout << outputString;
        std::cout.flush();
#if defined LOG_TO_FILE
        gLogsFile << outputString;
        gLogsFile.flush();
#endif // LOG_TO_FILE
#endif // DEBUG || _DEBUG || ENABLE_LOGS
    }
    template <typename type, typename... Args>
    constexpr auto DebugPrintLine(const type& arg, Args... args) {
#if DEBUG || _DEBUG || ENABLE_LOGS
        auto outputString = appendToString(arg, args..., '\n');
#if defined _USE_OUPUT_DEBUG_STRING_
        OutputDebugStringA(outputString.c_str());
#endif
        std::cout << outputString;
        std::cout.flush();
#if defined LOG_TO_FILE
        gLogsFile << outputString;
        gLogsFile.flush();
#endif // LOG_TO_FILE
#endif // DEBUG || _DEBUG || ENABLE_LOGS
    }
    constexpr auto DebugPrintLine() {
        DebugPrintLine("");
    }
}




