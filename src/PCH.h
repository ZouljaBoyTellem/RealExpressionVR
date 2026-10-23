#pragma once

// Windows headers must be included after CommonLib, but these must be defined before anything
// (e.g. xbyak) includes <windows.h>: otherwise it drags in the old winsock.h, which conflicts with
// WinSock2.h, and defines min/max macros, which break SimpleIni.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>

#include <WinSock2.h>
#include <WS2tcpip.h>

using namespace std::literals;

namespace logger = SKSE::log;
