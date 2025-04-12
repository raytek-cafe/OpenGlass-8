#pragma once
#include "framework.hpp"
#include "cpprt.hpp"

namespace OpenGlass
{
	DWORD WINAPI InitializationThreadEntryPoint(PVOID);
	DWORD WINAPI UnInitializationThreadEntryPoint(PVOID);
}