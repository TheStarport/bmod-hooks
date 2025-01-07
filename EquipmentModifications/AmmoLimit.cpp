#include "Main.hpp"

std::unordered_map<uint, ammoStruct> ammoMap;
int max_ammo;

int GetLauncherCount(uint ammoId)
{
    static EquipDescList* playerEquipDesc = (EquipDescList*)((char*)GetModuleHandleA(nullptr) + 0x272960);

    int launcherCount = 0;
    for (auto eq = playerEquipDesc->equip.begin(); eq != playerEquipDesc->equip.end(); eq++)
    {
        if (!eq->bMounted || eq->is_internal())
        {
            continue;
        }
        Archetype::Launcher* launchArch = dynamic_cast<Archetype::Launcher*>(Archetype::GetEquipment(eq->iArchID));
        if (launchArch && launchArch->iProjectileArchID == ammoId)
        {
            launcherCount++;
        }
    }
    return std::max(1, launcherCount);
}

void __stdcall Get_Ammo_Limit(UINT ammo)
{
    auto iter = ammoMap.find(ammo);
    if (iter == ammoMap.end())
    {
        max_ammo = MAX_PLAYER_AMMO;
        return;
    }
    int launcherCount = GetLauncherCount(iter->first);
    int maxLauncherStackAllowed = std::min(iter->second.launcherMaxStack, launcherCount);
    max_ammo = iter->second.maxAmmo * maxLauncherStackAllowed;
}

__declspec(naked) void Ammolimit1_Hook()
{
    __asm push	eax
    __asm push	eax
    __asm call	Get_Ammo_Limit
    __asm mov	edx, offset max_ammo
    __asm pop	eax
    __asm ret
}

__declspec(naked) void Ammolimit2_Hook()
{
    __asm pushf
    __asm push	dword ptr[ebx + 0xb8]
        __asm call	Get_Ammo_Limit
    __asm mov	edx, offset max_ammo
    __asm popf
    __asm ret
}

__declspec(naked) void Ammolimit3_Hook()
{
    __asm push	eax
    __asm push	dword ptr[esp + 0x1c + 8]
        __asm call	Get_Ammo_Limit
    __asm mov	ecx, offset max_ammo
    __asm pop	eax
    __asm ret
}

__declspec(naked) void Ammolimit4_Hook()
{
    __asm push	ecx
    __asm push	dword ptr[esp + 0x1c + 4]
        __asm call	Get_Ammo_Limit
    __asm mov	ebx, max_ammo
    __asm pop	ecx
    __asm ret
}

DWORD protectXDummy;
#define ProtectX( addr, size ) \
  VirtualProtect( addr, size, PAGE_EXECUTE_READWRITE, &protectXDummy )

#define RELOFS( from, to ) \
  *(DWORD*)(from) = (DWORD)(to) - (DWORD)(from) - 4

void InitAmmoLimit()
{
#define ADDR_LIMIT1 ((PBYTE)0x47ee02) 	// checks "free" ammo
#define ADDR_LIMIT2 ((PBYTE)0x47f1e1) 	// equipment cost
#define ADDR_LIMIT3 ((PBYTE)0x483a72) 	// buy amount
#define ADDR_LIMIT4 ((PBYTE)0x62b3224)	// jettison/tractor

    ProtectX(ADDR_LIMIT4, 6);

    ADDR_LIMIT4[0] = 0x90;
    ADDR_LIMIT4[1] = 0xe8;
    RELOFS(ADDR_LIMIT4 + 2, Ammolimit4_Hook);

    if (ProtectX(ADDR_LIMIT1, 6))
    {
        ProtectX(ADDR_LIMIT2, 6);
        ProtectX(ADDR_LIMIT3, 6);

        *ADDR_LIMIT1 = *ADDR_LIMIT2 = *ADDR_LIMIT3 = 0x90;
        ADDR_LIMIT1[1] = ADDR_LIMIT2[1] = ADDR_LIMIT3[1] = 0xe8;
        RELOFS(ADDR_LIMIT1 + 2, Ammolimit1_Hook);
        RELOFS(ADDR_LIMIT2 + 2, Ammolimit2_Hook);
        RELOFS(ADDR_LIMIT3 + 2, Ammolimit3_Hook);
    }
}