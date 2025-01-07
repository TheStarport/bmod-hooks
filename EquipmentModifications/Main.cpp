#include <Windows.h>
#include "Main.hpp"
#include "../Utils.hpp"

typedef void(*GlobalTimeFunc)(double delta);
unsigned char* data;
GlobalTimeFunc globalTimingFunction;

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
}

void SetupHack()
{
	// Setup hooks
	HMODULE common = GetModuleHandleA("common");
	globalTimingFunction = (GlobalTimeFunc)GetProcAddress(common, "?UpdateGlobalTime@Timing@@YAXN@Z");
	data = (unsigned char*)malloc(5);
	Utils::Memory::DetourInit((unsigned char*)globalTimingFunction, Update, data);

	SetupClassExpansion();
	InitInfocardEdits();
	FreelancerOnlyHacks();

	InitBurstMod();
	InitMissileFixes();
	InitMiscFixes();
	InitAmmoLimit();
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
