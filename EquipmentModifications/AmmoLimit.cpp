#include "Main.hpp"

std::unordered_map<uint, int> ammoLauncherMap;
int max_ammo;

int GetAmmoCount(uint ammoId)
{
    static EquipDescList* playerEquipDesc = (EquipDescList*)((char*)GetModuleHandleA(nullptr) + 0x272960);

    int ammoCount = 0;
    for (auto eq = playerEquipDesc->equip.begin(); eq != playerEquipDesc->equip.end(); eq++)
    {
        if (!eq->bMounted || eq->is_internal())
        {
            continue;
        }
        Archetype::Launcher* launchArch = dynamic_cast<Archetype::Launcher*>(Archetype::GetEquipment(eq->iArchID));
        if (!launchArch || launchArch->iProjectileArchID != ammoId)
        {
            continue;
        }

        auto ammoIter = ammoLauncherMap.find(eq->iArchID);
        if (ammoIter != ammoLauncherMap.end())
        {
            ammoCount += ammoIter->second;
        }
    }
    return ammoCount;
}

bool __fastcall ReadMunitionProjArchetype(INI_Reader* ini, Archetype::Launcher* launcher, char* searchedValue)
{
    if (ini->is_value(searchedValue))
    {
        return true;
    }

    if (ini->is_value("ammo_limit"))
    {
        ammoLauncherMap[launcher->iArchID] = ini->get_value_int(0);
    }

    return false;
}

__declspec(naked) void ReadMunitionProjArchetypeNaked()
{
    __asm {
        mov edx, [esp+0xC]
        jmp ReadMunitionProjArchetype
    }
}

UINT __stdcall ReadMunition(INI_Reader& ini, UINT* ammo)
{
    if (ini.is_value("self_detonate") && ammo[2])
    {
        if (ini.get_value_bool(0))
        {
            self_detonating_mines.insert(ammo[2]);
        }
        return 0x62f6f2c;
    }
    else if (ini.is_value("arming_time") && ammo[2])
    {
        missile_arming_time[ammo[2]] = ini.get_value_float(0);
        return 0x62f6f2c;
    }
    else if (ini.is_value("mine_arming_time") && ammo[2])
    {
        mine_arming_time[ammo[2]] = ini.get_value_float(0);
        return 0x62f6f2c;
    }
    return 0x62f6f36;
}

__declspec(naked) void Ammolimit_Init(void)
{
    __asm push	edi
    __asm push	esi
    __asm call	ReadMunition
    __asm jmp	eax
}

void __stdcall Get_Ammo_Limit(UINT ammo)
{
    max_ammo = GetAmmoCount(ammo);
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
#define ADDR_INI    ((PDWORD)(0x62f6e97+2))
#define ADDR_LIMIT1 ((PBYTE)0x47ee02) 	// checks "free" ammo
#define ADDR_LIMIT2 ((PBYTE)0x47f1e1) 	// equipment cost
#define ADDR_LIMIT3 ((PBYTE)0x483a72) 	// buy amount
#define ADDR_LIMIT4 ((PBYTE)0x62b3224)	// jettison/tractor

    ProtectX(ADDR_INI, 4);
    ProtectX(ADDR_LIMIT4, 6);

    RELOFS(ADDR_INI, Ammolimit_Init);

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
        Utils::Memory::PatchCallAddr((char*)GetModuleHandleA("common"), 0x9619D, (char*)ReadMunitionProjArchetypeNaked);
    }
}