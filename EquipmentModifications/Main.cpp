#include <Windows.h>
#include "Main.hpp"
#include "../Utils.hpp"

typedef void(*GlobalTimeFunc)(double delta);
unsigned char* data;
GlobalTimeFunc GlobalTimingFunction;

constexpr double SixtyFramesPerSecond = 1.0 / 60.0;
double timeCounter;

// Called every frame
void __cdecl Update(double delta)
{
	timeCounter += delta;
	while (timeCounter > SixtyFramesPerSecond)
	{
		UpdateQueuedShots(delta);
		timeCounter -= SixtyFramesPerSecond;
	}
	

	Utils::Memory::UnDetour((unsigned char*)GlobalTimingFunction, data);
	GlobalTimingFunction(delta);
	Utils::Memory::Detour((unsigned char*)GlobalTimingFunction, Update, data);
}

void SetupHack()
{
	// Setup hooks
	HMODULE common = GetModuleHandleA("common");
	GlobalTimingFunction = (GlobalTimeFunc)GetProcAddress(common, "?UpdateGlobalTime@Timing@@YAXN@Z");
	data = (unsigned char*)malloc(5);
	Utils::Memory::Detour((unsigned char*)GlobalTimingFunction, Update, data);


	SetupClassExpansion();
	SetupCustomClassName();
	InitBurstMod();
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
