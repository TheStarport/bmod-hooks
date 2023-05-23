#include "Main.hpp"
#include <Windows.h>
#include <string>
#include <list>
#include <vector>
#include <optional>
#include <iostream>

#include "../Utils.hpp"

DWORD fl;

static DWORD originalCallSite = *(DWORD*)0x5C70EC;
DWORD returnAddress = 0x7c9B7;
DWORD returnAddressSkipConcat = 0x7c9C3;

std::map<int, std::wstring> classStringReplacements;

void ParseIni()
{
    INI_Reader ini;
    if (!ini.open("../DATA/CONFIG/equipment_class_mods.ini", false))
    {
        return;
    }

    while (ini.read_header())
    {
        if (!ini.is_header("Equipment"))
        {
            continue;
        }

        while (ini.read_value())
        {
            if (ini.is_value("name_binding"))
            {
                classStringReplacements[ini.get_value_int(0)] = Utils::String::stows(ini.get_value_string(1));
            }
        }
    }
}

void __fastcall ClassNumberStringDetour(HpAttachmentType type, wchar_t* destination)
{
    wcscat(destination, classStringReplacements[type].c_str());
}

bool CompareHpType(int type)
{
    return classStringReplacements.contains(type);
}

void __declspec(naked) ClassNumberStringDetourNaked()
{
    __asm {
        pushad
        mov ebx, [esp + 0x6c]
        push ebx
        call CompareHpType
        pop ebx
        test al, al
        jz orig
        mov ecx, ebx
        mov edx, [esp + 0x2C]
        call ClassNumberStringDetour
        popad
        sub esp, 8
        jmp returnAddressSkipConcat

        orig :
        popad
            call originalCallSite
            jmp returnAddress
    }
}

void SetupCustomClassName()
{
    fl = reinterpret_cast<DWORD>(GetModuleHandle(nullptr));
    returnAddress += fl;
    returnAddressSkipConcat += fl;

    const auto offset = PBYTE(fl + 0x7c9B1);
    Utils::Memory::Patch(offset, ClassNumberStringDetourNaked);

    ParseIni();
}