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

//Define our macro to write patches. Parameters are Offset, then bytes to write, size is dynamically calculated from the variadic arguments.
#define PatchM(offset, ...)\
{\
	constexpr DWORD o = DWORD(offset);\
	std::array<byte, std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value > patch = { __VA_ARGS__ };\
	Utils::Memory::WriteProcMem(mod + o, patch.data(), patch.size());\
}

//Define our macro to write arbitrary values into memory.
#define PatchV(offset, val)\
{\
	const auto v = val;\
	constexpr DWORD o = DWORD(offset);\
	Utils::Memory::WriteProcMem(mod + o, (void*)&v, sizeof(v));\
}

//Patch Freelancer to regenerate restart.fl on every load.
bool __stdcall RegenRestartDetour(const char* currentFile)
{

	return _strcmpi(currentFile, "restart.fl") == 0;
}

DWORD restartReplacementOffset = 0x69011;
DWORD restartReturnAddressFoundRestart = 0x6903F;
DWORD restartReturnAddressNormalSave = 0x69016;
PVOID restartProcessSave = PVOID(0x68D50);

__declspec(naked) void RegenRestartNaked()
{
	__asm {
		pushad
		push eax
		call RegenRestartDetour
		test al, al
		jnz found
		popad
		call[restartProcessSave]
		jmp restartReturnAddressNormalSave

		found :
		popad
			add esp, 8
			jmp restartReturnAddressFoundRestart
	}
}

static DWORD navMapCleanRetAddress = 0x8E5AF;
__declspec(naked) void PatchOutNoNavMapEntries()
{
	__asm {
		mov[esi + 0xF28], 0
		mov[esi + 0xF2C], 0
		mov[esi + 0xF30], 0
		mov[esi + 0xF34], 0
		mov ecx, [esp + 0xC0 - 0xC]
		jmp navMapCleanRetAddress
	}
}

bool freelancerOffsetsChanged = false;

//Hacks for Freelancer.exe
void FreelancerHacks()
{
	DWORD mod = reinterpret_cast<DWORD>(GetModuleHandle(nullptr));

	if (!freelancerOffsetsChanged)
	{
		freelancerOffsetsChanged = true;
		navMapCleanRetAddress += mod;
	}

	//Filter out incompatible builds on server by default. Too early on it's own.
	//PatchM(0x1628F4, 0x50);

	//Prevent IPv6 addresses from appearing in the server list.
	PatchM(0x1ACF6A, 0x40, 0x74, 0x63, 0x48, 0x51, 0x8D, 0x54, 0xE4, 0x20, 0x52, 0x83, 0xE9, 0x08);

	//Unlock visual cap on cruise speed.
	PatchM(0x0D5936, 0x90, 0xE9);

	//Disable shield batteries.
	PatchM(0x0DAE0B, 0x00, 0x00, 0x00, 0x00);
	PatchM(0x0DAE87, 0x00, 0x00, 0x00, 0x00);
	PatchM(0x1D8690, 0x00);

	//Disable nanobots.
	PatchM(0x0DAD6D, 0x00, 0x00, 0x00, 0x00);
	PatchM(0x0DADB4, 0x00, 0x00, 0x00, 0x00);
	PatchM(0x1D86E0, 0x00);

	//Update contact list refresh rate.
	PatchV(0x1D7964, 1.0f);

	//Update weapon panel refresh rate.
	PatchV(0x1D8484, 0.1f);

	Utils::CommandLineParser cmd;
	if (!cmd.CmdOptionExists(L"-windowborders"))
	{
		//Removes window borders when Freelancer is running in a window.
		PatchM(0x02477A, 0x00, 0x00);
		PatchM(0x02490D, 0x00, 0x00);
	}

	//Cap framerate.
	PatchV(0x0210A0C, 120.0f);

	//Repair based on ship cost, not just hitpoints.
	PatchM(0x0B3A0E, 0x50, 0xFF, 0x15, 0xFC, 0x61, 0x5C, 0x00, 0x90, 0x89, 0x44, 0xE4, 0x14, 0xFF, 0x15, 0x58, 0x61, 0x5C, 0x00, 0x89, 0xC1, 0xFF, 0x15, 0x18, 0x64, 0x5C, 0x00, 0xD9, 0xE8);
	PatchM(0x0B3A30, 0x58);

	//Consider volume when buying external equipment.
	//PatchM(0x0838AF, 0xEB);

	//Active [gun] light_anim entries.
	PatchM(0x12D132, 0xEB, 0x6D);
	PatchM(0x12D052, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90);
	PatchM(0x12D0F6, 0x53);
	PatchM(0x12D0F9, 0x02);
	PatchM(0x12D100, 0xCA);

	//Keyboard moves ship in turret view.
	PatchM(0x0C7903, 0x00);
	PatchM(0x0DBB12, 0xEB);
	PatchM(0x0DBB58, 0xEB);
	PatchM(0x0DBB9E, 0xEB);
	PatchM(0x0DBBE4, 0xEB);

	//Disable camera level during turret view.
	PatchM(0x14A65B, 0x9C, 0xA1, 0x44, 0x97, 0x67, 0x00, 0x83, 0xC0, 0xF8, 0x50, 0xFF, 0x15, 0x70, 0x64, 0x5C, 0x00, 0x59, 0x91, 0x9D, 0x74, 0x0D, 0xA0, 0xCA, 0xEC, 0x67, 0x00, 0x88, 0x81, 0xF9, 0x00, 0x00, 0x00, 0xEB, 0x25, 0xB0, 0x00, 0x86, 0x81, 0xF9, 0x00, 0x00, 0x00, 0xA2, 0xCA, 0xEC, 0x67, 0x00);

	//Allow missiles to fire continuously on right-click.
	PatchM(0x11D281, 0x00);

	//String ID for the version text in the main menu.
	PatchV(0x16DDEC, 471580);
	PatchV(0x174890, 471580);

	//Distance out to which the 'tractor all' button functions.
	PatchV(0x1D848C, std::powf(1500.0f, 2.0f));

	//Remove cruise speed display limit
	PatchM(0x0D5936, 0x90, 0xE9);

	//Bypass ESRB notice.
	PatchM(0x166C2B, 0xEB);

	//Supress "Failed to get start location" messages in FLSpew.txt
	PatchM(0x03B348, 0xEB);

	//Show all group members on the Navmap correctly in MP
	PatchM(0x08D89B, 0x83, 0xC5, 0x18, 0xEB, 0x50);
	PatchM(0x08D997, 0x00);

	//Fix for Multiple HpFire Bug
	PatchM(0x134D3D, 0x90, 0xBF, 0x01, 0x00, 0x00, 0x00);

	//Enable targeting reticule for everything
	PatchM(0x0F045B, 0x00, 0x00);

	const auto noNavMapEntriesOffset = PBYTE(mod + 0x8E571);
	Utils::Memory::Patch(noNavMapEntriesOffset, PatchOutNoNavMapEntries);

}

bool serverOffsetsChanged = false;


struct SrvGun
{
	void* vtable;
	CELauncher* launcher;
};

DWORD server;
UINT projectilesPerFire;

typedef bool(__fastcall HandlePlayerLauncherFire)(SrvGun* srvGun, PVOID _edx, Vector* vector);

bool __fastcall HandlePlayerLauncherFire_Hook(SrvGun* srvGun, PVOID _edx, Vector* vector)
{
	projectilesPerFire = srvGun->launcher->GetProjectilesPerFire();
	return ((HandlePlayerLauncherFire*)(server + 0xD840))(srvGun, _edx, vector);
}

__declspec(naked) void SetProjectilesPerFire()
{
	__asm {
		push 0x3F800000
		push[projectilesPerFire]
		mov eax, [server]
		add eax, 0xD91A
		jmp eax
	}
}

//Hacks for server.dll
void ServerHacks()
{
	DWORD mod = server = reinterpret_cast<DWORD>(GetModuleHandle(L"server.dll"));

	//Increase NPC despawn distance.
	PatchV(0x086AEC, std::powf(20000.0f, 2.0f));

	//Energy weapons don't damage power.
	PatchM(0x00AFC0, 0xC2, 0x08, 0x00);

	//Respawn time for any solar object to regain full health once destroyed in MP.
	PatchV(0x085530, 600.0);

	//Square of player and group disappear distances in MP
	PatchV(0x086AF0, 196000000.0f);
	PatchV(0x086AF4, 625000000.0f);

	Utils::CommandLineParser cmd;
	if (!cmd.CmdOptionExists(L"-decrypt"))
	{
		//Disable encryption of file on creating MP character, saving, or creating the restart.fl file.
		PatchM(0x06BFA6, 0x14, 0xB3);
		PatchM(0x06E10D, 0x14, 0xB3);
		PatchM(0x07399D, 0x14, 0xB3);
	}

	//Patch for restart.fl. This only occurs once when the game is started.
	if (!serverOffsetsChanged)
	{
		serverOffsetsChanged = true;
		restartProcessSave = PVOID(DWORD(restartProcessSave) + mod);
		restartReplacementOffset += mod;
		restartReturnAddressFoundRestart += mod;
		restartReturnAddressNormalSave += mod;
	}
	Utils::Memory::Patch(PBYTE(restartReplacementOffset), RegenRestartNaked);

	//Patch for bugs associated with multiple HpFire hardpoints:
	Utils::Memory::PatchCallAddr(PCHAR(mod), 0xD9A9, PCHAR(HandlePlayerLauncherFire_Hook));
	Utils::Memory::PatchCallAddr(PCHAR(mod), 0xDC09, PCHAR(HandlePlayerLauncherFire_Hook));
	Utils::Memory::PatchCallAddr(PCHAR(mod), 0xE009, PCHAR(HandlePlayerLauncherFire_Hook));
	Utils::Memory::Patch(PBYTE(mod + 0xD913), SetProjectilesPerFire);
}

//Hacks for common.dll
void CommonHacks()
{
	DWORD mod = reinterpret_cast<DWORD>(GetModuleHandle(L"common.dll"));

	//Disable Act_PlayerEnemyClamp.
	PatchM(0x08E86A, 0xEB, 0x39);

	//Fixes CEGun::CanSeeTargetObject, allowing NPCs to 'see' everything.
	PatchM(0x038590, 0xB0, 0x01, 0xC2, 0x04, 0x00);

	//Enable NPC countermeasure and scanner usage.
	PatchM(0x13E52C, 0x00);

	//Enable mine droppers during cruise.
	PatchM(0x03A2B3, 0x09);

	//Radians value of NPC muzzle cone angle.
	PatchV(0x08A185, 0.349065f);
	PatchV(0x08AE95, 0.349065f);

	//Makes suns honour the visit flag.
	PatchM(0x0D670F, 0xE9, 0x47, 0xFF, 0xFF, 0xFF);

	//Make drag_modifier independent of interference/damage and apply it in cruise.
	PatchM(0x0DAD24, 0x41, 0x74);
	PatchM(0x053796, 0xEB);

	//Cap framerate.
	PatchV(0x01A74C, 120.0f);
	PatchM(0x01A892, 0x4C, 0xA7, 0x27);

	//disable ArchDB::Get random mission spew warning.
	PatchM(0x0995B6, 0x90, 0x90);
	PatchM(0x0995FC, 0x90, 0x90);

	//Repair price multiplier ratio
	//PatchV(0x004A28, 0.33f);
	//PatchV(0x0057FA, 0.33f);

	//Change MISSION_SATELLITE check flag
	PatchM(0x18C87C, 0x10, 0x90)

}

//Hacks for content.dll
void ContentHacks()
{
	DWORD mod = reinterpret_cast<DWORD>(GetModuleHandle(L"content.dll"));

	Utils::CommandLineParser cmd;
	if (!cmd.CmdOptionExists(L"-campaign"))
	{
		//Force newplayer.fl to m13.ini (Open Singleplayer)
		//PatchM(0x04EE3A, 0xA2, 0x6A);
	}

	//Increase NPC despawn distance (SP and MP)
	PatchV(0x0D3C36, 15000.0f);
	PatchV(0x0D3D6E, 15000.0f);

	//Increase maximum NPC spawning distance
	PatchV(0x11BC68, 10000.0);

	//NPC 'heartbeat' timer.
	PatchV(0x0BA57A, 31.0f);

	//Reputation needed for lawful factions to show rumors.
	PatchV(0x12E354, 0.0f);

	// Distance over which NPC spawns ignore density cap for multiple players
	PatchV(0x117A68, 14000.0f);
}

//Run all our patching functions.
extern "C" EXPORT LPVOID CreateInstance(LPVOID a1, LPCSTR callsign, LPVOID a3)
{
	//LoadLibrary(L"../DLLS/BIN/content.dll");
	ServerHacks();
	ContentHacks();
	return nullptr;
}

extern "C" EXPORT void DestroyInstance(LPVOID a1)
{
	return;
}

std::map <std::string, uint> hashMap;
using CreateIdType = uint(*)(const char*);
CreateIdType createId = (CreateIdType)CreateID;
PBYTE createIdData = PBYTE(malloc(5));

FILE* hashFile = nullptr;

uint CreateIdDetour(const char* string)
{
	if (!string) return 0;

	if (!hashFile)
	{
		fopen_s(&hashFile, "hashmap.txt", "wb");
	}

	Utils::Memory::UnDetour(PBYTE(createId), createIdData);
	uint hash = CreateID(string);
	Utils::Memory::Detour(PBYTE(createId), CreateIdDetour, createIdData);

	std::string str = string;
	if (hashMap.find(str) == hashMap.end())
	{
		hashMap[str] = hash;
		fprintf_s(hashFile, "%s,%u,0x%X\n", string, hash, hash);
		fflush(hashFile);
	}
	return hash;
}

//Run these patches for the client only.
void SetupHack()
{
	std::array<wchar_t, MAX_PATH> fileNameBuffer;
	GetModuleFileName(nullptr, fileNameBuffer.data(), MAX_PATH);
	std::wstring filename = fileNameBuffer.data();

	if (EndsWith(filename, L"Freelancer.exe"))
	{
		FreelancerHacks();
		Utils::Memory::DetourInit(PBYTE(createId), CreateIdDetour, createIdData);
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