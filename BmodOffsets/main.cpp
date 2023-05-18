#include <Windows.h>
#include <string>
#include <list>
#include <vector>
#include <optional>
#include <array>
#include "../Utils.hpp"

bool EndsWith(std::wstring const& value, std::wstring const& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

//Define our macro to write patches (Parameters are Offset, then bytes to write, size is dynamically calculated from the variadic arguments.
#define Patch(offset, ...)\
{\
	constexpr DWORD o = DWORD(offset);\
	std::array<byte, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value > patch = { __VA_ARGS__ };\
	Utils::Memory::WriteProcMem(mod + o, patch.data(), patch.size());\
}

//Define our macro to write arbitrary value into memory.
#define PatchV(offset, val)\
{\
	const auto v = val;\
	constexpr DWORD o = DWORD(offset);\
	Utils::Memory::WriteProcMem(mod + o, &v, sizeof(v));\
}

void FreelancerHacks()
{
	DWORD mod = reinterpret_cast<DWORD>(GetModuleHandle(nullptr));
	//Unlock visual cap on cruise speed.
	Patch(0x0D5936, 0x90, 0xE9);
	//Disable shield batteries
	Patch(0x0DAE0B, 0x00, 0x00, 0x00, 0x00);
	//Allow missiles to fire continuously on right-click.
	Patch(0x11D281, 0x00);
}

void ServerHacks()
{
	DWORD mod = reinterpret_cast<DWORD>(GetModuleHandle(L"server.dll"));

}

void CommonHacks()
{
	DWORD mod = reinterpret_cast<DWORD>(GetModuleHandle(L"common.dll"));

}

void SetupHack()
{
	std::array<wchar_t, MAX_PATH> fileNameBuffer;
	GetModuleFileName(nullptr, fileNameBuffer.data(), MAX_PATH);
	std::wstring filename = fileNameBuffer.data();

	if (EndsWith(filename, L"Freelancer.exe"))
	{
		FreelancerHacks();
	}
	else if (EndsWith(filename, L"flserver.exe"))
	{
		ServerHacks();
	}
	CommonHacks();
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hModule);
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		SetupHack();
	}
	return TRUE;
}
