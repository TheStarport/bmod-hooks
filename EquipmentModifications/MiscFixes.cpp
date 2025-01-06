#include "Main.hpp"

bool __fastcall AddScanListDetour(CEqObj* scanning, CSimple* scanned, ScanList* scanList)
{
	if (scanList->currSize > 240 && scanned->objectClass != CObject::CGUIDED_OBJECT)
	{
		// reserve 16 last slots for CGuided objects
		return false;
	}
	if (scanning->ownerPlayer && scanning == Utils::GetCShip())
	{
		// never cull dockables
		if (scanned->objectClass == CObject::CSOLAR_OBJECT
			&& scanned->type & (OBJ_JUMP_HOLE | OBJ_JUMP_GATE | OBJ_TRADELANE_RING | OBJ_DOCKING_RING | OBJ_STATION))
		{
			return true;
		}
		// only allow asteroids and solars below 2k range (adjusting for radius) to be added
		if ((scanned->objectClass == CObject::CASTEROID_OBJECT
			|| scanned->objectClass == CObject::CSOLAR_OBJECT)
			&& (Utils::Distance3D(scanning->vPosition, scanned->vPosition) - scanned->fRadius) > 2000)
		{
			return false;
		}
	}
	return true;
}

FARPROC callScannerInterference = FARPROC(0x62B5E00);
__declspec(naked) void AddScanListDetourNaked()
{
	__asm
	{
		pushad
		mov edx, [edi + 0x10]
		push esi
		call AddScanListDetour
		test al, al
		jz skipAsteroid
		popad
		jmp[callScannerInterference]

		skipAsteroid:
		popad
			pop edi
			pop edi
			pop esi
			mov al, 0
			pop ebx
			retn 4
	}
}

void InitMiscFixes()
{
	auto hCommon = GetModuleHandleA("common");
	{
		Utils::Memory::PatchCallAddr((char*)hCommon, 0x3F807, (char*)AddScanListDetourNaked);
	}
}