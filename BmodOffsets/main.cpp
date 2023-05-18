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
	Utils::Memory::WriteProcMem(mod + o, (void*)&v, sizeof(v));\
}

void FreelancerHacks()
{
	DWORD mod = reinterpret_cast<DWORD>(GetModuleHandle(nullptr));

	//Unlock visual cap on cruise speed.
	Patch(0x0D5936, 0x90, 0xE9);

	//Disable shield batteries.
	Patch(0x0DAE0B, 0x00, 0x00, 0x00, 0x00);
	Patch(0x0DAE87, 0x00, 0x00, 0x00, 0x00);
	Patch(0x1D86E0, 0x00);

	//Disable nanobots.
	Patch(0x0DAE0B, 0x00, 0x00, 0x00, 0x00)
	Patch(0x0DAE87, 0x00, 0x00, 0x00, 0x00)
	Patch(0x1D86E0, 0x00);

	//Update contact list refresh rate.
	PatchV(0x1D7964, 0.25f);

	//Update weapon panel refresh rate.
	PatchV(0x1D8484, 0.25f);

	//Removes window borders when Freelancer is running in a window.
	Patch(0x02477A, 0x00, 0x00);
	Patch(0x02490D, 0x00, 0x00);

	//Cap framerate.
	PatchV(0x0210A0C, 120.0f);

	//Repair based on ship cost, not just hitpoints.
	Patch(0x0B3A0E, 0x50, 0xFF, 0x15, 0xFC, 0x61, 0x5C, 0x00, 0x90, 0x89, 0x44, 0xE4, 0x14, 0xFF, 0x15, 0x58, 0x61, 0x5C, 0x00, 0x89, 0xC1, 0xFF, 0x15, 0x18, 0x64, 0x5C, 0x00, 0xD9, 0xE8);
	Patch(0x0B3A30, 0x58);

	//Consider volume when buying external equipment.
	//Patch(0x0838AF, 0xEB);

	//Active [gun] light_anim entries.
	Patch(0x12D132, 0xEB, 0x6D);
	Patch(0x12D052, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90);
	Patch(0x12D0F6, 0x53);
	Patch(0x12D0F9, 0x02);
	Patch(0x12D100, 0xCA);

	//Keyboard moves ship in turret view.
	Patch(0x0C7903, 0x00);
	Patch(0x0DBB12, 0xEB);
	Patch(0x0DBB58, 0xEB);
	Patch(0x0DBB9E, 0xEB);
	Patch(0x0DBBE4, 0xEB);

	//Disable camera level during turret view
	Patch(0x14A65B, 0x9C, 0xA1, 0x44, 0x97, 0x67, 0x00, 0x83, 0xC0, 0xF8, 0x50, 0xFF, 0x15, 0x70, 0x64, 0x5C, 0x00, 0x59, 0x91, 0x9D, 0x74, 0x0D, 0xA0, 0xCA, 0xEC, 0x67, 0x00, 0x88, 0x81, 0xF9, 0x00, 0x00, 0x00, 0xEB, 0x25, 0xB0, 0x00, 0x86, 0x81, 0xF9, 0x00, 0x00, 0x00, 0xA2, 0xCA, 0xEC, 0x67, 0x00);

	//Allow missiles to fire continuously on right-click.
	Patch(0x11D281, 0x00);
}

void ServerHacks()
{
	DWORD mod = reinterpret_cast<DWORD>(GetModuleHandle(L"server.dll"));

	//Increase NPC despawn distance.
	PatchV(0x086AEC, std::powf(20000.0f, 2.0f));

	//Energy weapons don't damage power.
	Patch(0x00AFC0, 0xC2, 0x08, 0x00);

	//Regenerate restart.fl on every game load.
	//Patch(0x6903E, 0x00);
}

void CommonHacks()
{
	DWORD mod = reinterpret_cast<DWORD>(GetModuleHandle(L"common.dll"));

	//Disable Act_PlayerEnemyClamp.
	Patch(0x08E86A, 0xEB, 0x39);

	//Enable NPC countermeasure usage.
	Patch(0x13E52C, 0x00);

	//Enable mine droppers during cruise.
	Patch(0x03A2B3, 0x09);

	//CEGun::CanSeeTargetObject fix for NPCs.
	Patch(0x038590, 0xB0, 0x01, 0xC2, 0x04, 0x00);

	//Radians value of NPC muzzle cone angle
	PatchV(0x08A185, 0.349065f);
	PatchV(0x08AE95, 0.349065f);

	//Makes suns honour the visit flag.
	Patch(0x0D670F, 0xE9, 0x47, 0xFF, 0xFF, 0xFF);

	//Make drag_modifier independent of interference/damage and apply it in cruise
	Patch(0x0DAD24, 0x41, 0x74);
	Patch(0x053796, 0xEB);

	//Cap maximum FPS at 120
	PatchV(0x01A74C, 120.0f);
	Patch(0x01A892, 0x4C, 0xA7, 0x27);

	//Include external equipment in cargo size
	Patch(0x053048, 0xE3, 0x1F);
	Patch(0x05330E, 0xE3, 0x1F);
	Patch(0x0A9BA3, 0x00);
	Patch(0x0AA904, 0x00);

	//disable ArchDB::Get random mission spew warning.
	Patch(0x0995B6, 0x90, 0x90);
	Patch(0x0995FC, 0x90, 0x90);
}

//Run our hack process and check if the server or the client are running accordingly. Always run hacks for common.dll.
void SetupHack()
{
	std::array<wchar_t, MAX_PATH> fileNameBuffer;
	GetModuleFileName(nullptr, fileNameBuffer.data(), MAX_PATH);
	std::wstring filename = fileNameBuffer.data();

	if (EndsWith(filename, L"Freelancer.exe"))
	{
		FreelancerHacks();
		ServerHacks();
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
