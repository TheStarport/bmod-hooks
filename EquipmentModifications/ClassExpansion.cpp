#include "Main.hpp"

#include <string>
#include <list>
#include <vector>
#include <optional>
#include <map>

#include "../Utils.hpp"

std::map<uint, std::string> hpStrings;

// Default add indexes for existing classes
std::vector<byte> levelIndex = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 		// guns
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 	// turrets, disruptor, torpedo
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 		// fighter shields
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 		// elite shields
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 		// freighter shields
};

std::vector<byte> equipIndex = {
	0, 1, 2, 3, 3, 3, 3, 3, 2, 2, 2, 2, 4,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 		// guns
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,		// turrets, disruptor, torpedo
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 		// fighter shields
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 		// elite shields
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 		// freighter shields
};

struct IdsBinding
{
	uint hpType;
	uint idsNumber;
};

std::vector<IdsBinding> idsBindings = {
	{  15, 1525 }, {  16, 1526 }, {  17, 1527 }, {  18, 1528 }, {  19, 1529 },
  {  20, 1530 }, {  21, 1531 }, {  22, 1532 }, {  23, 1533 }, {  24, 1534 },
  {  13, 1524 }, {  25, 1731 }, {  26, 1732 }, {  27, 1733 }, {  28, 1734 },
  {  29, 1735 }, {  30, 1736 }, {  31, 1737 }, {  32, 1738 }, {  33, 1739 },
  {  34, 1740 }, {  10, 1521 }, {  36, 1741 }, {  35, 1742 }, {  11, 1522 },
  {  12, 1523 }, {  37, 1700 }, {  38, 1701 }, {  39, 1702 }, {  40, 1704 },
  {  41, 1705 }, {  42, 1706 }, {  43, 1707 }, {  44, 1708 }, {  45, 1709 },
  {  46, 1710 }, {   6, 1517 }, {  47, 1711 }, {  48, 1712 }, {  49, 1713 },
  {  50, 1714 }, {  51, 1715 }, {  52, 1716 }, {  53, 1717 }, {  54, 1718 },
  {  55, 1719 }, {  56, 1720 }, {   7, 1518 }, {  57, 1721 }, {  58, 1722 },
  {  59, 1723 }, {  60, 1724 }, {  61, 1725 }, {  62, 1726 }, {  63, 1727 },
  {  64, 1728 }, {  65, 1729 }, {  66, 1730 }, {   8, 1519 }, {   9, 1520 },
};

uint totalEquipmentCount = 67;

std::vector<byte> mountList = {
  0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

std::vector<byte> mountDescriptionIndex = {
   0,  1,  1,  2,  3,  4,  5,  6,  7,  8, 10,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  9,  5,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
   3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
};

__declspec(naked) void GetMountDesc(void)
{
	__asm {
		lea	eax, [ecx - 1]
		ret
	}
}

std::vector<uint> mountDesc = { 907, 908, 916, 911, 909, 910, 912, 913, 914, 915 };

std::map<uint, int> hpCategoryMap;
constexpr uint TotalClasses = 10;

void ReadEquipmentClassMods()
{
	INI_Reader ini;
	if (!ini.open("../DATA/CONFIG/equipment_class_mods.ini", false))
	{
		return;
	}

	if (!ini.find_header("Equipment"))
	{
		return;
	}


	while (ini.read_value())
	{
		if (ini.is_value("new_hp_type"))
		{
			const std::string type = ini.get_value_string(0);
			const std::string newHp = ini.get_value_string(1);
			uint idsNumberRange = ini.get_value_int(2);

			if (type.empty() || newHp.empty() || !idsNumberRange)
			{
				return;
			}

			int category;
			if (type == "Gun" || type == "Turret")
			{
				category = 0;
			}
			else if (type == "Shield")
			{
				category = 2;
			}
			else if (type == "Powerplant" || type == "Engine")
			{
				category = 3;
			}
			else
			{
				category = 4;
			}

			// Add new description index
			mountDesc.emplace_back(idsNumberRange++);

			// Add 10 new classes, total of 11 ids numbers
			for (uint i = 1; i <= TotalClasses; i++)
			{
				hpStrings[totalEquipmentCount] = newHp + "_" + std::to_string(i);
				hpCategoryMap[totalEquipmentCount] = category;
				levelIndex.emplace_back(i);
				equipIndex.emplace_back(2);
				idsBindings.emplace_back(totalEquipmentCount++, idsNumberRange++);
				mountList.emplace_back(2);
				mountDescriptionIndex.emplace_back(11);
			}
		}
	}
}

static std::unordered_map<uint, uint> customHpTypeMapping; // Map of item hash to specified HP Type
uint __fastcall HpTypeDetour(const char* hpString)
{
	if (hpString == nullptr)
		return 0;

	const std::string hpStr = hpString;
	const auto found = std::ranges::find_if(hpStrings, [&hpStr](const auto& n) { return hpStr == n.second; });
	return found == hpStrings.end() ? 0 : found->first;
}

// If found return the custom number we setup
// Otherwise return 0
uint __stdcall IsCustomHpType(const uint hash)
{
	const auto found = customHpTypeMapping.find(hash);
	return found != customHpTypeMapping.end() ? found->second : 0;
}

void __stdcall AddNewHpMapping(const uint hash, const uint hpType)
{
	customHpTypeMapping[hash] = hpType;
}

static DWORD hpTableOffset = 0x18CA40;

static DWORD gunReadReplacementOffset = 0x9659E;
static DWORD gunReadSearchVanillaEntry = 0x965A5;
static DWORD gunReadFoundCustomEntry = 0x965CA;
Archetype::Gun* gun;
__declspec(naked) void HpTypeGunRead()
{
	__asm
	{
		push ebp
		mov ebp, [esp+12]
		mov esi, hpTableOffset
		push eax
		push edx
		push ecx
		mov ecx, eax
		call HpTypeDetour
		test eax, eax
		jz notFound
		push eax
		mov ecx, [ebp+8]
		push ecx
		call AddNewHpMapping
		pop ecx
		pop edx
		mov esi, eax
		pop eax
		pop ebp
		jmp gunReadFoundCustomEntry

		notFound :
		pop ecx
		pop edx
		pop eax
		pop ebp
		xor edi, edi
		jmp gunReadSearchVanillaEntry
	}
}

static DWORD shieldReadReplacementOffset = 0x959C0;
static DWORD shieldReadSearchVanillaEntry = 0x959C7;
static DWORD shieldReadFoundCustomEntry = 0x959E8;
/*__declspec(naked) void HpTypeShieldRead()
{
	__asm
	{
		mov esi, hpTableOffset
		push eax
		push edx
		push ecx
		mov ecx, eax
		call HpTypeDetour
		test eax, eax
		jz notFound
		pop ecx
		pop edx
		mov esi, eax
		pop eax
		jmp shieldReadFoundCustomEntry

		notFound :
		pop ecx
			pop edx
			pop eax
			xor edi, edi
			jmp shieldReadSearchVanillaEntry
	}
}*/

#define RELOFS( from, to ) \
  *(DWORD*)(from) = (PBYTE)(to) - (from) - 4


#define HP_TYPE 67

// Handle the Internal Equipment inventory list.
__declspec(naked) void InvMntHook(void)
{
	__asm {
		xor al, al
		test	edx, edx
		jz	okay
		cmp	edx, 3 // If page three
		//jne	done
		test	ebp, ebp
		jz	done
		push	ecx
		push	dword ptr[ebp + 0x48]
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		mov	ecx, eax
		add	esp, 4
		mov	eax, [eax + 8]
		push eax
		call IsCustomHpType // For guns this returns 4
		cmp	eax, HP_TYPE
		pop	ecx
		mov	al, 0
		mov	edx, 3
		jb	done
		okay :
		mov	edx, 2 // if two can buy?
			done :
			ret
	}
}

__declspec(naked) void InvSellHook(void)
{
	__asm {
		mov	eax, [esi + 0x32c]
		cmp	eax, 3 // If page three
		//jne	done
		push	edx
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		add	esp, 4
		test	eax, eax
		jz	internal
		mov	ecx, eax
		mov	eax, [eax + 8]
		push eax
		call IsCustomHpType // For guns this returns 4
		cmp	eax, HP_TYPE
		mov	eax, 0
		jae	done
internal:
	mov	eax, 3 // if three can buy?
		done :
		ret
	}
}

__declspec(naked) void InvIconHook(void)
{
	__asm {
		mov	eax, [esi + 0x32c]
		cmp	eax, 3 // If page three
		//jne	done
		push	dword ptr[esp + 0x28 + 4]
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		add	esp, 4
		test	eax, eax
		jz	internal
		mov	ecx, eax
		mov	eax, [eax + 8]
		push eax
		call IsCustomHpType // For guns this returns 4
		cmp	eax, HP_TYPE
		mov	eax, 0
		jae	done
internal:
	mov	eax, 3 // if three can buy?
		done :
		ret
	}
}

__declspec(naked) void InvHook(void)
{
	__asm {
		mov	eax, [edi + 0x32c]
		cmp	eax, 3 // If page three
		//jne	done
		push	dword ptr[ebp + 0x48]
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		add	esp, 4
		test	eax, eax
		jz	internal
		mov	ecx, eax
		mov	eax, [eax + 8]
		push eax
		call IsCustomHpType // For guns this returns 4
		cmp	eax, HP_TYPE
		mov	eax, 0
		jae	done
internal:
	mov	eax, 3 // if three can buy?
		done :
		ret
	}
}


// Handle the Internal Equipment dealer list.
__declspec(naked) void DealerHook(void)
{
	__asm {
		push	eax
		mov	ecx, [eax + 0x32c]
		cmp	ecx, 3 // If page three
		//jne	done
		push	ebx
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		mov	ecx, eax
		add	esp, 4
		jcxz	internal
		mov	eax, [eax + 8]
		push eax
		call IsCustomHpType // For guns this returns 4
		cmp	eax, HP_TYPE
		mov	ecx, 0
		jae	done
internal:
	mov	ecx, 3 // if three can buy?
		done :
		pop	eax
		ret
	}
}


// Handle selected Internal Equipment in the dealer list.
__declspec(naked) void DealerSelectedHook(void)
{
	__asm {
		push	eax
		mov	ecx, [eax + 0x32c]
		cmp	ecx, 3 // If page three
		//jne	done
		push	ebp
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		mov	ecx, eax
		add	esp, 4
		jcxz	internal
		call IsCustomHpType // For guns this returns 4
		cmp	eax, HP_TYPE
		mov	ecx, 0
		jae	done
internal:
	mov	ecx, 3 // if three can buy?
		done :
		pop	eax
		ret
	}
}


// Handle mounting of Internal Equipment.
__declspec(naked) void MountHook(void)
{
	__asm {
		mov	edi, eax
		cmp	eax, 3 // If page three
		//jne	done
		push	dword ptr[esi + 4]
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		add	esp, 4
		test	eax, eax
		jz	internal
		mov	ecx, eax
		mov	eax, [eax+8]
		push eax
		call IsCustomHpType // For guns this returns 4
		cmp	eax, HP_TYPE
		mov	edi, 0
		jae	done
internal:
	mov	edi, 3 // Can buy?
		done :
		ret	4
	}
}


// Set the default type to the original value.
__declspec(naked) void InitHook(void)
{
	__asm {
		mov	byte ptr[esi + 0x6f], 2
		mov[esi + 0x70], ebx
		mov[esi + 0x74], ebx
		ret
	}
}

#define ADDR_RDSHIP1 ((PBYTE)0x62f2f1d+1)

DWORD __stdcall HpRange(const LPCSTR type)
{
	for (auto i = hpStrings.begin(); i != hpStrings.end(); ++i)
	{
		if (_strnicmp(type, i->second.c_str(), i->second.length()) == 0)
		{
			std::string result;
			std::ranges::copy_if(i->second, std::back_inserter(result), [](const char ch) { return '0' <= ch && ch <= '9'; });

			char* end;
			const int c = strtol(result.c_str(), &end, 10);
			if (c < 1 || c > TotalClasses || *end)
				break;
			return HP_TYPE + std::distance(hpStrings.begin(), i) + c - 1;
		}
	}

	return 0;
}

// Recognize [Ship]'s new hp_type value.
__declspec(naked) void ShipHook(void)
{
	__asm {
		push	dword ptr[esp + 0x10]
		call	HpRange
		test	eax, eax
		jnz	fnd
		mov	al, 1
		push	0x62f27d2
		ret
		fnd :
		mov	edi, eax
			push	0x62f25a2
			ret
	}
}


// Process hp_type for [Power] and [Engine].
BOOL __stdcall HpSet(const LPCSTR type, const PBYTE equip)
{
	if (const int i = HpRange(type))
	{
		// Add our equipment to our HP maps
		AddNewHpMapping(*reinterpret_cast<uint*>(reinterpret_cast<char*>(equip) + 8), i);
		return TRUE;
	}
	return FALSE;
}


// Read the hp_type value for [Power] and [Engine].
__declspec(naked) void ReadHook(void)
{
	__asm {
		mov	eax, 0x6310410		// INI_Reader::is_value
		push	0x63a15ac		// "hp_type"
		mov	ecx, esi
		call	eax
		test	al, al
		jz	done
		mov	eax, 0x6310a30		// INI_Reader::get_value_string
		push	0
		mov	ecx, esi
		call	eax
		test	eax, eax
		jz	done
		push	edi
		push	eax
		call	HpSet
		done :
		pop	edi
			pop	esi
			ret	4
	}
}


// Special handling for MultiCruise.
DWORD multicruise;

__declspec(naked) void ReadEngHook(void)
{
	__asm {
		push	esi
		push	offset ReadHook
		push	esi
		push	edi
		jmp	multicruise
	}
}

// get_hp_type for Power and Engine Archetypes.
/*__declspec(naked) void get_hp_type(void)
{
	__asm {
		movzx	eax, byte ptr[ecx + 0x6f]
		ret
	}
}*/

// Dynamically get our category out depending on the equipment type.
// Guns / Turrets = category 0
// Shields = category 2
// Engines/Powerplants = category 3
int __stdcall DetermineHpCategory(const uint hpType)
{
	const auto found = hpCategoryMap.find(hpType);
	return found != hpCategoryMap.end() ? found->second : 3;
}

static constexpr DWORD HpCategoryReturnAddress = 0x586CAA;
__declspec(naked) void SetHpCategoryNaked()
{
	__asm {
		push eax
		push edx
		push ecx
		push esi
		call DetermineHpCategory
		mov ebp, eax
		pop ecx
		pop edx
		pop eax
		jmp HpCategoryReturnAddress
	}
}

void Patch(void)
{
#define ADDR_EQUIP1	((PBYTE)0x47c8de)	// show class number
#define ADDR_EQUIP2	((PBYTE)0x47d9bb)	// inventory list
#define ADDR_EQUIP3	((PBYTE)0x4804e8)	// selected item
#define ADDR_EQUIP4	((PBYTE)0x482ea7)	// dealer list
#define ADDR_EQUIP5	((PBYTE)0x47ce7d)	// mounted icon
#define ADDR_EQUIP6	((PBYTE)0x47f73a)
#define ADDR_EQUIP7	((PBYTE)0x47d974)
#define ADDR_EQUIP8	((PBYTE)0x47e8d7)
#define ADDR_EQUIP9	((PBYTE)0x483869)
#define ADDR_LEVEL	((PBYTE)0x47c940)
#define ADDR_EQUIP	((PBYTE)0x47e4de)
#define ADDR_ENGXFER	((PBYTE)0x48089d)
#define ADDR_ENGSELL	((PBYTE)0x480936)
#define ADDR_MNTDESC	((PDWORD)0x47e041)
#define ADDR_MNTNAME1 ((PBYTE)0x584728)
#define ADDR_MNTNAME2 ((PBYTE)0x586c46)
#define ADDR_MNTNAME3 ((PDWORD)0x586ccc)
#define ADDR_MNTNAME4 ((PBYTE)0x585b17)
#define ADDR_MNTNAME5 ((PDWORD)0x585b90)
	//#define ADDR_MNTNAME6 ((PBYTE)0x586835)	// doesn't seem to be used
#define ADDR_RESDLL	((PBYTE)0x5b0fad)
#define ADDR_INTXFER	((PBYTE)0x47b020)
#define ADDR_INTDESC	((PBYTE)0x47d608)
#define ADDR_INTMNT	((PBYTE)0x47e820)

#define ADDR_POWER	((PBYTE)0x62f80d0)
#define ADDR_ENGINE	((PBYTE)0x62f817e)
#define ADDR_RDSHIP	((PBYTE)0x62f2598)
#define ADDR_RDPOWER	((PBYTE)0x62f4878)
#define ADDR_RDENGINE ((PBYTE)0x62f51b8)
#define ADDR_HPPOWER	((PDWORD)0x639990c)
#define ADDR_HPENGINE ((PDWORD)0x6399ac4)
#define ADDR_WARNING	((PBYTE)0x62a9774+1)

	constexpr auto NewEquipmentCategory = 0x586CA5;

	ProtectExecuteReadWrite(ADDR_INTXFER, 0x1e);
	ProtectExecuteReadWrite(ADDR_EQUIP1, 1);
	//ProtectExecuteReadWrite( ADDR_LEVEL,    16 );
	//ProtectExecuteReadWrite( ADDR_EQUIP5,    6 );
	ProtectExecuteReadWrite(ADDR_INTDESC, 7);
	//ProtectExecuteReadWrite( ADDR_EQUIP7,    6 );
	//ProtectExecuteReadWrite( ADDR_EQUIP2,    6 );
	ProtectExecuteReadWrite(ADDR_MNTDESC, 4);
	//ProtectExecuteReadWrite( ADDR_EQUIP,    12 );
	//ProtectExecuteReadWrite( ADDR_INTMNT,    5 );
	//ProtectExecuteReadWrite( ADDR_EQUIP8,    6 );
	ProtectExecuteReadWrite(ADDR_EQUIP6, 5);
	ProtectExecuteReadWrite(ADDR_EQUIP3, 6);
	//ProtectExecuteReadWrite( ADDR_ENGXFER,   2 );
	//ProtectExecuteReadWrite( ADDR_ENGSELL,   1 );
	ProtectExecuteReadWrite(ADDR_EQUIP4, 6);
	ProtectExecuteReadWrite(ADDR_EQUIP9, 1);
	ProtectExecuteReadWrite(ADDR_MNTNAME1, 11);
	ProtectExecuteReadWrite(ADDR_MNTNAME4, 12);
	//ProtectExecuteReadWrite( ADDR_MNTNAME5,  8 );
	ProtectExecuteReadWrite(ADDR_MNTNAME2, 97);
	//ProtectExecuteReadWrite( ADDR_MNTNAME3,  4 );
	ProtectExecuteReadWrite(ADDR_RESDLL, 6);

	ProtectExecuteReadWrite(ADDR_WARNING, 1);
	ProtectExecuteReadWrite(ADDR_RDSHIP, 5);
	//ProtectExecuteReadWrite( ADDR_RDSHIP1,   1 );
	ProtectExecuteReadWrite(ADDR_RDPOWER, 5);
	ProtectExecuteReadWrite(ADDR_RDENGINE, 5);
	ProtectExecuteReadWrite(ADDR_POWER, 6);
	//ProtectExecuteReadWrite( ADDR_ENGINE,    6 );
	ProtectReadWrite(ADDR_HPPOWER, 4);
	//ProtectReadWrite( ADDR_HPENGINE,  4 );

	// *** FREELANCER.EXE ***

	// Have Internal Equipment recognise hard points.
	*ADDR_EQUIP1 = *ADDR_EQUIP9 = 0xeb;

	ADDR_EQUIP7[0] = ADDR_EQUIP2[0] = ADDR_EQUIP3[0] = ADDR_EQUIP4[0] =
		ADDR_EQUIP5[0] = ADDR_EQUIP6[0] = ADDR_EQUIP8[0] = 0xe8;
	RELOFS(ADDR_EQUIP5 + 1, InvIconHook);
	RELOFS(ADDR_EQUIP7 + 1, InvMntHook);
	RELOFS(ADDR_EQUIP8 + 1, InvSellHook);
	RELOFS(ADDR_EQUIP2 + 1, InvHook);
	RELOFS(ADDR_EQUIP3 + 1, DealerSelectedHook);
	RELOFS(ADDR_EQUIP4 + 1, DealerHook);
	RELOFS(ADDR_EQUIP6 + 1, MountHook);
	ADDR_EQUIP7[5] = ADDR_EQUIP8[5] = ADDR_EQUIP2[5] = ADDR_EQUIP3[5] =
		ADDR_EQUIP4[5] = ADDR_EQUIP5[5] = 0x90;

	// Recognise the class of the new types.
	ADDR_LEVEL[2] += 60;
	*(DWORD*)(ADDR_LEVEL + 12) = (DWORD)levelIndex.data();

	// Recognise the equipment list of the new types.
	ADDR_EQUIP[2] += 60;
	*(DWORD*)(ADDR_EQUIP + 8) = (DWORD)equipIndex.data();

	// Allow engines to be sold/transferred.

#ifdef ALLOW_ENGINE_SALE
	ADDR_ENGXFER[0] = 0x90;
	ADDR_ENGXFER[1] = 0xe9;
	ADDR_ENGSELL[0] = 0xeb;

	// Don't automatically transfer engines and powerplants.
	ADDR_INTXFER[2] = 0x14;
	ADDR_INTXFER[0x1d] = 0x16;
#endif

	// Adjust for the new mount names.
	ADDR_MNTNAME1[2] += 60;
	ADDR_MNTNAME2[0x33] += 60;
	ADDR_MNTNAME2[0x43] += 60;
	ADDR_MNTNAME4[2] += 60;
	*reinterpret_cast<DWORD*>((ADDR_MNTNAME1 + 8)) = *reinterpret_cast<DWORD*>((ADDR_MNTNAME2 + 3)) = reinterpret_cast<DWORD>(idsBindings.data());
	*ADDR_MNTNAME3 = reinterpret_cast<DWORD>(idsBindings.data()) + 4;
	*reinterpret_cast<DWORD*>((ADDR_MNTNAME2 + 0x49)) = reinterpret_cast<DWORD>(mountList.data());
	*reinterpret_cast<DWORD*>((ADDR_MNTNAME4 + 8)) = reinterpret_cast<DWORD>(mountDescriptionIndex.data());
	for (uint i = 0; i < mountDesc.size() - 10; ++i)
		ADDR_MNTNAME5[i] = reinterpret_cast<DWORD>(GetMountDesc);

	// Actually recognise the new mount points.
	ADDR_MNTNAME2[0x59] = 2;
	Utils::Memory::Patch((PBYTE)NewEquipmentCategory, SetHpCategoryNaked);
	ADDR_INTDESC[1] = 0x58;
	ADDR_INTDESC[6] = 0xd2;
	ADDR_INTMNT[1] = 0x78;
	ADDR_INTMNT[4] = 0xc9;

	// Point to the new mount descriptions.
	*ADDR_MNTDESC = reinterpret_cast<DWORD>(mountDesc.data());

	// *** COMMON.DLL ***

	ADDR_POWER[0] = ADDR_ENGINE[0] = 0xe8;
	RELOFS(ADDR_POWER + 1, InitHook);
	RELOFS(ADDR_ENGINE + 1, InitHook);
	ADDR_POWER[5] = ADDR_ENGINE[5] = 0x90;

	if (*ADDR_RDENGINE == 0xBE)
		multicruise = *(DWORD*)(ADDR_RDENGINE + 1);
	ADDR_RDSHIP[0] = ADDR_RDPOWER[0] = ADDR_RDENGINE[0] = 0xe9;
	RELOFS(ADDR_RDSHIP + 1, ShipHook);
	RELOFS(ADDR_RDPOWER + 1, ReadHook);
	RELOFS(ADDR_RDENGINE + 1, (multicruise) ? ReadEngHook : ReadHook);

	// Point to our get_hp_type function.
	//*ADDR_HPPOWER = *ADDR_HPENGINE = (DWORD)get_hp_type;

	// Remove warning regarding connecting internal type to hardpoint.
	*ADDR_WARNING = 0x4e;
}

void SetupClassExpansion()
{
	ReadEquipmentClassMods();

	const DWORD common = reinterpret_cast<DWORD>(GetModuleHandle(L"common.dll"));
	hpTableOffset += common;

	gunReadReplacementOffset += common;
	gunReadSearchVanillaEntry += common;
	gunReadFoundCustomEntry += common;
	shieldReadFoundCustomEntry += common;
	shieldReadReplacementOffset += common;
	shieldReadSearchVanillaEntry += common;

	Patch();

	Utils::Memory::Patch((PBYTE)gunReadReplacementOffset, HpTypeGunRead);
	//Utils::Memory::Patch((PBYTE)shieldReadReplacementOffset, HpTypeShieldRead);
}