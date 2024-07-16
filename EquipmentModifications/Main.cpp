#include <Windows.h>
#include "Main.hpp"
#include "../Utils.hpp"

typedef void(*GlobalTimeFunc)(double delta);
unsigned char* data;
GlobalTimeFunc globalTimingFunction;

constexpr double SixtyFramesPerSecond = 1.0 / 60.0;
double timeCounter;



typedef int(__cdecl* ThornDoBuffer)(char* str, int length, char* desc);
typedef int(__cdecl* ThornDoMain)(void* state, int bin);
static ThornDoBuffer LuaDoBuffer = ThornDoBuffer(0x06F4CCE0);
static ThornDoMain LuaDoMain = ThornDoMain(0x6F4CAE0);
unsigned char* thornDoMainData;

int LuaDoString(char* str)
{
	return LuaDoBuffer(str, strlen(str), str);
}

static int LuaDoMainHook(void* state, int bin)
{
	Utils::Memory::UnDetour((unsigned char*)LuaDoMain, thornDoMainData);

	Archetype::Ship* ship = Utils::GetShipArch();
	std::stringstream ss;
	// Add any information to the ship object for use in Lua code that you want
	if (ship)
	{
		ss << "SHIP = { " <<
			"class = " << ship->iShipClass << ", " <<
			"cargo = " << ship->fHoldSize << ", " <<
			"model = \"" << ship->szName << "\", " <<
			"arch_type = " << ship->iArchType << ", " <<
			"id = " << ship->iArchID << ", " <<
			"mass = " << ship->fMass << ", " <<
			"hitpoints = " << ship->fHitPoints <<
			"} ";
	}

	LuaDoString((char*)ss.str().c_str());
	auto res = LuaDoMain(state, bin);
	Utils::Memory::Detour((unsigned char*)LuaDoMain, LuaDoMainHook, thornDoMainData);
	return res;
}

using ScriptLoadPtr = void* (*)(const char*);
ScriptLoadPtr thornLoad;
unsigned char* thornLoadData;

void* ScriptLoadHook(const char* script)
{
	thornDoMainData = PBYTE(malloc(5));
	Utils::Memory::UnDetour(PBYTE(thornLoad), thornLoadData);
	Utils::Memory::Detour((unsigned char*)LuaDoMain, LuaDoMainHook, thornDoMainData);
	return thornLoad(script);
}


// Called every frame
void __cdecl Update(double delta)
{
	timeCounter += delta;
	while (timeCounter > SixtyFramesPerSecond)
	{
		UpdateQueuedShots(delta);
		timeCounter -= SixtyFramesPerSecond;
	}


	Utils::Memory::UnDetour((unsigned char*)globalTimingFunction, data);
	globalTimingFunction(delta);
	Utils::Memory::Detour((unsigned char*)globalTimingFunction, Update, data);
}

void FreelancerOnlyHacks()
{
	if (Utils::Memory::GetCurrentExe() != "Freelancer.exe")
	{
		return;
	}

	SetupCustomClassName();
	InitBurstMod();
}

void SetupHack()
{
	// Setup hooks
	HMODULE common = GetModuleHandleA("common");
	globalTimingFunction = (GlobalTimeFunc)GetProcAddress(common, "?UpdateGlobalTime@Timing@@YAXN@Z");
	data = (unsigned char*)malloc(5);
	Utils::Memory::Detour((unsigned char*)globalTimingFunction, Update, data);

	SetupClassExpansion();
	InitInfocardEdits();
	FreelancerOnlyHacks();

	thornLoadData = (unsigned char*)malloc(5);
	thornLoad = reinterpret_cast<ScriptLoadPtr>(GetProcAddress(GetModuleHandleA("common.dll"), "?ThornScriptLoad@@YAPAUIScriptEngine@@PBD@Z")); // NOLINT
	Utils::Memory::Detour(PBYTE(thornLoad), ScriptLoadHook, thornLoadData);

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
