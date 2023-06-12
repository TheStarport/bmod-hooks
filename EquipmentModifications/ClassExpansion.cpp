#include "Main.hpp"

#include <string>
#include <list>
#include <vector>
#include <optional>
#include <map>
#include <array>

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

constexpr uint startingEquipmentCount = 67;
static uint totalEquipmentCount = startingEquipmentCount;

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
			int idsOrder = ini.get_value_int(3);
			int category = ini.get_value_int(4);
			if (type.empty() || newHp.empty() || !idsNumberRange)
			{
				return;
			}

			// No category was explicitly set, so we infer it
			if (category == 0)
			{
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
			}

			// Add new description index
			mountDesc.emplace_back(idsNumberRange++);

			// Add 10 new classes, total of 11 ids numbers
			for (uint i = 1; i <= TotalClasses; i++)
			{
				hpStrings[totalEquipmentCount] = newHp + "_" + std::to_string(i);
				hpCategoryMap[totalEquipmentCount] = category;
				levelIndex.emplace_back(i);
				equipIndex.emplace_back(idsOrder);
				idsBindings.emplace_back(totalEquipmentCount++, idsNumberRange++);
				mountList.emplace_back(2);
				mountDescriptionIndex.emplace_back(11);
			}
		}
	}
}

static std::unordered_map<uint, std::vector<uint>> customHpTypeMapping; // Map of item hash to specified HP Type

PBYTE gunHpTypeByIndexMemory = static_cast<PBYTE>(malloc(5));
PBYTE shieldHpTypeByIndexMemory = static_cast<PBYTE>(malloc(5));
PBYTE gunHpTypeCountMemory = static_cast<PBYTE>(malloc(5));
PBYTE shieldHpTypeCountMemory = static_cast<PBYTE>(malloc(5));
using GetHpTypeByIndexT = uint(*__fastcall)(Archetype::Equipment* equip, void* edx, uint index);
using GetHpTypeCountT = uint(*__fastcall)(Archetype::Equipment* equip);
GetHpTypeCountT getGunHpTypeCount = reinterpret_cast<GetHpTypeCountT>(GetProcAddress(GetModuleHandleA("common.dll"), "?get_number_of_hp_types@Gun@Archetype@@QBEHXZ"));
GetHpTypeCountT getShieldHpTypeCount = reinterpret_cast<GetHpTypeCountT>(GetProcAddress(GetModuleHandleA("common.dll"), "?get_number_of_hp_types@ShieldGenerator@Archetype@@QBEHXZ"));
GetHpTypeByIndexT getGunHpTypeByIndex = reinterpret_cast<GetHpTypeByIndexT>(GetProcAddress(GetModuleHandleA("common.dll"), "?get_hp_type_by_index@Gun@Archetype@@QBE?AW4HpAttachmentType@@H@Z"));
GetHpTypeByIndexT getShieldHpTypeByIndex = reinterpret_cast<GetHpTypeByIndexT>(GetProcAddress(GetModuleHandleA("common.dll"), "?get_hp_type_by_index@ShieldGenerator@Archetype@@QBE?AW4HpAttachmentType@@H@Z"));

uint GetCustomHpByIndex(const uint itemHash, const uint index)
{
	const auto mapping = customHpTypeMapping.find(itemHash);
	if (mapping == customHpTypeMapping.end())
	{
		return 0;
	}

	if (mapping->second.size() <= index)
	{
		return 0;
	}

	return mapping->second[index];
}

uint __fastcall GetHpTypeCountDetour(const Archetype::Gun* gun)
{
	if (const auto found = customHpTypeMapping.find(gun->iArchID); found == customHpTypeMapping.end())
	{
		Utils::Memory::UnDetour(reinterpret_cast<PBYTE>(getGunHpTypeCount), gunHpTypeCountMemory);
		const auto res = gun->get_number_of_hp_types();
		Utils::Memory::Detour(reinterpret_cast<PBYTE>(getGunHpTypeCount), GetHpTypeCountDetour, gunHpTypeCountMemory);
		return res;
	}

	const auto mapping = customHpTypeMapping.find(gun->iArchID);
	return mapping == customHpTypeMapping.end() ? 0 : mapping->second.size();
}

uint __fastcall GetHpTypeByIndexDetour(const Archetype::Gun* gun, void* edx, uint index)
{
	if (const auto found = customHpTypeMapping.find(gun->iArchID); found == customHpTypeMapping.end())
	{
		Utils::Memory::UnDetour(reinterpret_cast<PBYTE>(getGunHpTypeByIndex), gunHpTypeByIndexMemory);
		const auto res = gun->get_hp_type_by_index(index);
		Utils::Memory::Detour(reinterpret_cast<PBYTE>(getGunHpTypeByIndex), GetHpTypeByIndexDetour, gunHpTypeByIndexMemory);
		return res;
	}

	return GetCustomHpByIndex(gun->iArchID, index);
}

uint __fastcall GetShieldHpTypeCountDetour(const Archetype::Gun* shield)
{
	if (const auto found = customHpTypeMapping.find(shield->iArchID); found == customHpTypeMapping.end())
	{
		Utils::Memory::UnDetour(reinterpret_cast<PBYTE>(getShieldHpTypeCount), shieldHpTypeByIndexMemory);
		const auto res = shield->get_number_of_hp_types();
		Utils::Memory::Detour(reinterpret_cast<PBYTE>(getShieldHpTypeCount), GetShieldHpTypeCountDetour, shieldHpTypeByIndexMemory);
		return res;
	}

	const auto mapping = customHpTypeMapping.find(shield->iArchID);
	return mapping == customHpTypeMapping.end() ? 0 : mapping->second.size();
}

uint __fastcall GetShieldHpTypeByIndexDetour(const Archetype::ShieldGenerator* shield, void* edx, uint index)
{
	if (const auto found = customHpTypeMapping.find(shield->iArchID); found == customHpTypeMapping.end())
	{
		Utils::Memory::UnDetour(reinterpret_cast<PBYTE>(getShieldHpTypeByIndex), shieldHpTypeByIndexMemory);
		const auto res = shield->get_hp_type_by_index(index);
		Utils::Memory::Detour(reinterpret_cast<PBYTE>(getShieldHpTypeByIndex), GetShieldHpTypeByIndexDetour, shieldHpTypeByIndexMemory);
		return res;
	}

	return GetCustomHpByIndex(shield->iArchID, index);
}

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
bool __stdcall IsCustomHpType(const uint hash)
{
	const auto found = customHpTypeMapping.find(hash);
	return found != customHpTypeMapping.end();
}

void __stdcall AddNewHpMapping(const uint hash, const uint hpType)
{
	if (const auto mapping = customHpTypeMapping.find(hash); mapping == customHpTypeMapping.end())
	{
		customHpTypeMapping[hash] = { hpType };
	}
	else
	{
		mapping->second.emplace_back(hpType);
	}
}

constexpr static DWORD ProcessEquipmentNotFoundOffset = 0x586192;
constexpr static DWORD ProcessEquipmentFoundOffset = 0x5860C9;
static uint storedHash = 0;
__declspec(naked) void ProcessCustomHp()
{
	__asm
	{
		push eax
		mov eax, storedHash
		test eax, eax
		jz notFound
		push eax
		call IsCustomHpType // is it one of ours
		test al, al
		jz notFound // If we found a valid match send it through
		pop eax
		jmp ProcessEquipmentFoundOffset

		notFound :
		pop eax
			jmp ProcessEquipmentNotFoundOffset
	}
}

constexpr DWORD CurrentItemHashRetAddr = 0x585FAC;
__declspec(naked) void NakedCurrentItemHash()
{
	__asm
	{
		mov storedHash, 0 // Reset the stored hash if needed
		add esp, 0x0C
		push eax
		mov eax, [esp + 0x20]
		test eax, eax // if eax == 0 then not a gun
		jnz found
		mov eax, [esp + 0x24]
		test eax, eax // if eax == 0 then not a shield
		jz done

		found :
		mov eax, [eax + 8]
			mov storedHash, eax

			done :
		pop eax
			mov dword ptr[esp + 0x14], eax
			jmp CurrentItemHashRetAddr
	}
}

constexpr static DWORD WriteHardpointCountReturnAddress = 0x586026;
__declspec(naked) void NakedWriteHardpointCount()
{
	__asm
	{
		mov     ebp, totalEquipmentCount
		add     ebx, ecx
		jmp WriteHardpointCountReturnAddress
	}
}

//__declspec(naked) void NakedCleanUpItemHash()
//{
//	__asm
//	{
//		mov storedHash, 0
//		// Do the stuff we replaced
//		mov     al, byte ptr[esp + 0x5C]
//		test    al, al
//	}
//}

//constexpr static DWORD ItemCleanRetAddress = 0x5862F8;
//__declspec(naked) void NakedCleanUpItemHash()
//{
//	__asm
//	{
//		mov storedHash, 0
//		add esp, 4
//		mov eax, esi
//		jmp ItemCleanRetAddress
//	}
//}

static DWORD hpTableOffset = 0x18CA40;

static DWORD gunReadReplacementOffset = 0x9659E;
static DWORD gunReadSearchVanillaEntry = 0x965A5;
static DWORD gunReadFoundCustomEntry = 0x965CA;

__declspec(naked) void HpTypeGunRead()
{
	__asm
	{
		push ebp
		mov ebp, [esp + 12]
		mov esi, hpTableOffset
		push eax
		push edx
		push ecx
		mov ecx, eax
		call HpTypeDetour
		test eax, eax
		jz notFound
		push eax
		mov ecx, [ebp + 8]
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
__declspec(naked) void HpTypeShieldRead()
{
	__asm
	{
		push ebp
		mov ebp, esi
		mov esi, hpTableOffset
		push edx
		push eax
		push ecx
		mov ecx, eax
		call HpTypeDetour
		test eax, eax
		jz notFound

		push eax
		mov ebp, [ebp + 8]
		push ebp
		call AddNewHpMapping
		pop ecx
		mov esi, eax
		pop eax
		pop edx
		pop ebp
		jmp shieldReadFoundCustomEntry

		notFound:
			pop ecx
			pop eax
			pop edx
			pop ebp
			xor edi, edi
			jmp shieldReadSearchVanillaEntry
	}
}

#define HP_TYPE 67
__declspec(naked) void InvMntHook()
{
	__asm {
		xor al, al
		test	edx, edx
		jz	okay
		cmp	edx, 3
		jne	done
		test	ebp, ebp
		jz	done
		push	ecx
		push	dword ptr[ebp + 0x48]
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		mov	ecx, eax
		mov	eax, [eax]
		add	esp, 4
		call	dword ptr[eax + 0x18]	// get_hp_type
		cmp	eax, HP_TYPE
		pop	ecx
		mov	al, 0
		mov	edx, 3
		jb	done
		okay :
		mov	edx, 2
			done :
			ret
	}
}

__declspec(naked) void InvSellHook()
{
	__asm {
		mov	eax, [esi + 0x32c]
		cmp	eax, 3
		jne	done
		push	edx
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		add	esp, 4
		test	eax, eax
		jz	internal
		mov	ecx, eax
		mov	eax, [eax]
		call	dword ptr[eax + 0x18]	// get_hp_type
		cmp	eax, HP_TYPE
		mov	eax, 0
		jae	done
internal:
	mov	eax, 3
		done :
		ret
	}
}


__declspec(naked) void InvIconHook()
{
	__asm {
		mov	eax, [esi + 0x32c]
		cmp	eax, 3
		jne	done
		push	dword ptr[esp + 0x28 + 4]
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		add	esp, 4
		test	eax, eax
		jz	internal
		mov	ecx, eax
		mov	eax, [eax]
		call	dword ptr[eax + 0x18]	// get_hp_type
		cmp	eax, HP_TYPE
		mov	eax, 0
		jae	done
internal:
	mov	eax, 3
		done :
		ret
	}
}


__declspec(naked) void InvHook()
{
	__asm {
		mov	eax, [edi + 0x32c]
		cmp	eax, 3
		jne	done
		push	dword ptr[ebp + 0x48]
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		add	esp, 4
		test	eax, eax
		jz	internal
		mov	ecx, eax
		mov	eax, [eax]
		call	dword ptr[eax + 0x18]	// get_hp_type
		cmp	eax, HP_TYPE
		mov	eax, 0
		jae	done
internal:
	mov	eax, 3
		done :
		ret
	}
}


// Handle the Internal Equipment dealer list.
__declspec(naked) void DealerHook()
{
	__asm {
		push	eax
		mov	ecx, [eax + 0x32c]
		cmp	ecx, 3
		jne	done
		push	ebx
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		mov	ecx, eax
		add	esp, 4
		jcxz	internal
		mov	eax, [eax]
		call	dword ptr[eax + 0x18]	// get_hp_type
		cmp	eax, HP_TYPE
		mov	ecx, 0
		jae	done
internal:
	mov	ecx, 3
		done :
		pop	eax
		ret
	}
}


// Handle selected Internal Equipment in the dealer list.
__declspec(naked) void DealerSelectedHook()
{
	__asm {
		push	eax
		mov	ecx, [eax + 0x32c]
		cmp	ecx, 3
		jne	done
		push	ebp
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		mov	ecx, eax
		add	esp, 4
		jcxz	internal
		mov	eax, [eax]
		call	dword ptr[eax + 0x18]	// get_hp_type
		cmp	eax, HP_TYPE
		mov	ecx, 0
		jae	done
internal:
	mov	ecx, 3
		done :
		pop	eax
		ret
	}
}

// Handle mounting of Internal Equipment.
__declspec(naked) void MountHook()
{
	__asm {
		mov	edi, eax
		cmp	eax, 3
		jne	done
		push	dword ptr[esi + 4]
		call	dword ptr ds : [0x5c603c] // Archetype::GetEquipment
		add	esp, 4
		test	eax, eax
		jz	internal
		mov	ecx, eax
		mov	eax, [eax]
		call	dword ptr[eax + 0x18]	// get_hp_type
		cmp	eax, HP_TYPE
		mov	edi, 0
		jae	done
internal:
	mov	edi, 3
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

const PBYTE ADDR_RDSHIP1 = reinterpret_cast<PBYTE>(0x62f2f1d + 1);

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
			auto e = 67 + std::distance(hpStrings.begin(), i);
			return e;
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

// Dynamically get our category out depending on the equipment type.
// Guns / Turrets = category 0
// Shields = category 2
// Engines/Powerplants = category 3
int __stdcall DetermineHpCategory(const uint hpType)
{
	const auto found = hpCategoryMap.find(hpType);
	return found != hpCategoryMap.end() ? found->second : -1;
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
		cmp eax, -1
		je done
		mov ebp, eax

		done:
		pop ecx
		pop edx
		pop eax
		jmp HpCategoryReturnAddress
	}
}

//const static DWORD* HpCategoryJumpTableAddress = reinterpret_cast<PDWORD>(0x585B64);
//__declspec(naked) void SetHpCategoryItemNaked()
//{
//	__asm
//	{
//		push edx
//		push ecx
//		push eax
//		call DetermineHpCategory 
//		cmp eax, -1
//		pop ecx
//		je defaultBehavior
//		pop edx
//		retn
//
//		defaultBehavior:
//		mov	eax, [HpCategoryJumpTableAddress]
//		mov edx, 4
//		imul edx, ecx
//		add eax, edx
//		pop edx
//		mov eax, [eax]
//		jmp eax
//	}
//}

#define RELOFS( from, to ) \
  *(DWORD*)(from) = (PBYTE)(to) - (from) - 4

void CommonPatches()
{
	const auto AddrPower = reinterpret_cast<PBYTE>(0x62f80d0);
	const auto AddrEngine = reinterpret_cast<PBYTE>(0x62f817e);
	const auto AddrRead = reinterpret_cast<PBYTE>(0x62f2598);
	const auto AddrRdPower = reinterpret_cast<PBYTE>(0x62f4878);
	const auto AddrRdEngine = reinterpret_cast<PBYTE>(0x62f51b8);
	const auto AddrHpPower = ((PDWORD)0x639990c);
	const auto AddrHpEngine = ((PDWORD)0x6399ac4);
	const auto AddrWarning = reinterpret_cast<PBYTE>(0x62a9774 + 1);

	ProtectExecuteReadWrite(AddrWarning, 1);
	ProtectExecuteReadWrite(AddrRead, 5);
	ProtectExecuteReadWrite(AddrRdPower, 5);
	ProtectExecuteReadWrite(AddrRdEngine, 5);
	ProtectExecuteReadWrite(AddrPower, 6);
	ProtectReadWrite(AddrHpPower, 4);

	// *** COMMON.DLL ***

	AddrPower[0] = AddrEngine[0] = 0xe8;
	RELOFS(AddrPower + 1, InitHook);
	RELOFS(AddrEngine + 1, InitHook);
	AddrPower[5] = AddrEngine[5] = 0x90;

	if (*AddrRdEngine == 0xBE)
		multicruise = *(DWORD*)(AddrRdEngine + 1);
	AddrRead[0] = AddrRdPower[0] = AddrRdEngine[0] = 0xe9;
	RELOFS(AddrRead + 1, ShipHook);
	RELOFS(AddrRdPower + 1, ReadHook);
	RELOFS(AddrRdEngine + 1, (multicruise) ? ReadEngHook : ReadHook);

	// Point to our get_hp_type function.
	//*ADDR_HPPOWER = *ADDR_HPENGINE = (DWORD)get_hp_type;

	// Remove warning regarding connecting internal type to hardpoint.
	*AddrWarning = 0x4e;

	Utils::Memory::Detour(reinterpret_cast<PBYTE>(getGunHpTypeByIndex), GetHpTypeByIndexDetour, gunHpTypeByIndexMemory);
	Utils::Memory::Detour(reinterpret_cast<PBYTE>(getGunHpTypeCount), GetHpTypeCountDetour, gunHpTypeCountMemory);
	Utils::Memory::Detour(reinterpret_cast<PBYTE>(getShieldHpTypeCount), GetShieldHpTypeCountDetour, shieldHpTypeCountMemory);
	Utils::Memory::Detour(reinterpret_cast<PBYTE>(getShieldHpTypeByIndex), GetShieldHpTypeByIndexDetour, shieldHpTypeByIndexMemory);

	Utils::Memory::Patch((PBYTE)gunReadReplacementOffset, HpTypeGunRead);
	Utils::Memory::Patch((PBYTE)shieldReadReplacementOffset, HpTypeShieldRead);
}

void FreelancerExePatches()
{
	const auto addrEquip1 = reinterpret_cast<PBYTE>(0x47c8de);	// show class number
	const auto addrEquip2 = reinterpret_cast<PBYTE>(0x47d9bb);	// inventory list
	const auto addrEquip3 = reinterpret_cast<PBYTE>(0x4804e8);	// selected item
	const auto addrEquip4 = reinterpret_cast<PBYTE>(0x482ea7);	// dealer list
	const auto addrEquip5 = reinterpret_cast<PBYTE>(0x47ce7d);	// mounted icon
	const auto addrEquip6 = reinterpret_cast<PBYTE>(0x47f73a);
	const auto addrEquip7 = reinterpret_cast<PBYTE>(0x47d974);
	const auto addrEquip8 = reinterpret_cast<PBYTE>(0x47e8d7);
	const auto addrEquip9 = reinterpret_cast<PBYTE>(0x483869);
	const auto addrLevel = reinterpret_cast<PBYTE>(0x47c940);
	const auto addrEquip = reinterpret_cast<PBYTE>(0x47e4de);
	const auto addrEngXfer = reinterpret_cast<PBYTE>(0x48089d);
	const auto addrEngSell = reinterpret_cast<PBYTE>(0x480936);
	const auto addrMntDesc = ((PDWORD)0x47e041);
	const auto addrMntName1 = reinterpret_cast<PBYTE>(0x584728);
	const auto addrMntName2 = reinterpret_cast<PBYTE>(0x586c46);
	const auto addrMntName3 = ((PDWORD)0x586ccc);
	const auto addrMntName4 = reinterpret_cast<PBYTE>(0x585b17);
	const auto addrMntName5 = ((PDWORD)0x585b90);

	const auto addrIntTransfer = reinterpret_cast<PBYTE>(0x47b020);
	const auto addrIntDesc = reinterpret_cast<PBYTE>(0x47d608);
	const auto addrIntMnt = reinterpret_cast<PBYTE>(0x47e820);

	ProtectExecuteReadWrite(addrEquip1, 1);
	ProtectExecuteReadWrite(addrEquip3, 6);
	ProtectExecuteReadWrite(addrEquip4, 6);
	ProtectExecuteReadWrite(addrEquip6, 5);
	ProtectExecuteReadWrite(addrEquip9, 1);

	ProtectExecuteReadWrite(addrMntName1, 11);
	ProtectExecuteReadWrite(addrMntName2, 97);
	ProtectExecuteReadWrite(addrMntName4, 12);

	ProtectExecuteReadWrite(addrIntTransfer, 0x1e);
	ProtectExecuteReadWrite(addrIntDesc, 7);
	ProtectExecuteReadWrite(addrIntMnt, 4);

	// Have Internal Equipment recognise hard points.
	*addrEquip1 = *addrEquip9 = 0xeb;

	addrEquip7[0] = addrEquip2[0] = addrEquip3[0] = addrEquip4[0] =
		addrEquip5[0] = addrEquip6[0] = addrEquip8[0] = 0xe8;
	RELOFS(addrEquip5 + 1, InvIconHook);
	RELOFS(addrEquip7 + 1, InvMntHook);
	RELOFS(addrEquip8 + 1, InvSellHook);
	RELOFS(addrEquip2 + 1, InvHook);
	RELOFS(addrEquip3 + 1, DealerSelectedHook);
	RELOFS(addrEquip4 + 1, DealerHook);
	RELOFS(addrEquip6 + 1, MountHook);
	addrEquip7[5] = addrEquip8[5] = addrEquip2[5] = addrEquip3[5] =
		addrEquip4[5] = addrEquip5[5] = 0x90;

	// Recognise the class of the new types.
	addrLevel[2] = levelIndex.size();
	*reinterpret_cast<DWORD*>((addrLevel + 12)) = reinterpret_cast<DWORD>(levelIndex.data());

	// Recognise the equipment list of the new types.
	addrEquip[2] = equipIndex.size();
	*reinterpret_cast<DWORD*>((addrEquip + 8)) = reinterpret_cast<DWORD>(equipIndex.data());

	constexpr auto HardpointCategoryReadAddress = 0x586Ca5;
	constexpr auto EquipmentCategoryReadAddress = 0x585B23;

#ifdef ALLOW_ENGINE_SALE
	// allow engines to be sold/transferred.
	addrEngXfer[0] = 0x90;
	addrEngXfer[1] = 0xe9;
	addrEngSell[0] = 0xeb;

	// Don't automatically transfer engines and powerplants.
	addrIntTransfer[2] = 0x14;
	addrIntTransfer[0x1d] = 0x16;
#endif

	// adjust for the new mount names.
	addrMntName1[2] = idsBindings.size();
	addrMntName2[0x33] = mountList.size();
	addrMntName2[0x43] = mountList.size();
	addrMntName4[2] = mountDescriptionIndex.size();
	*reinterpret_cast<DWORD*>((addrMntName1 + 8)) = *reinterpret_cast<DWORD*>((addrMntName2 + 3)) = reinterpret_cast<DWORD>(idsBindings.data());
	*addrMntName3 = reinterpret_cast<DWORD>(idsBindings.data()) + 4;
	*reinterpret_cast<DWORD*>((addrMntName2 + 0x49)) = reinterpret_cast<DWORD>(mountList.data());
	*reinterpret_cast<DWORD*>((addrMntName4 + 8)) = reinterpret_cast<DWORD>(mountDescriptionIndex.data());
	for (uint i = 0; i < mountDesc.size() - 10; ++i)
		addrMntName5[i] = reinterpret_cast<DWORD>(GetMountDesc);

	// actually recognise the new mount points.
	addrMntName2[0x59] = 2;
	//addrMntName2[0x60] = 3; // category
	addrIntDesc[1] = 0x58;
	addrIntDesc[6] = 0xd2;
	addrIntMnt[1] = 0x78;
	addrIntMnt[4] = 0xc9;

	// Point to the new mount descriptions.
	*addrMntDesc = reinterpret_cast<DWORD>(mountDesc.data());

	// Function Detours/Patches
	Utils::Memory::Patch(reinterpret_cast<PBYTE>(HardpointCategoryReadAddress), SetHpCategoryNaked);

	{
		std::array<byte, 2> patch = { 0xEB, 0x12 };
		Utils::Memory::WriteProcMem(0x583651, patch.data(), patch.size()); 
	}

	//Utils::Memory::Patch(reinterpret_cast<PBYTE>(EquipmentCategoryReadAddress), SetHpCategoryItemNaked);
	//Utils::Memory::Patch(PBYTE(0x5860C3), ProcessCustomHp);
	//Utils::Memory::Patch(PBYTE(0x585F9F), NakedCurrentItemHash);;
	//Utils::Memory::Patch(PBYTE(0x586021), NakedWriteHardpointCount);

	//Utils::Memory::Patch(PBYTE(0x586182), NakedCleanUpItemHash);
	//Utils::Memory::Patch(PBYTE(0x586216), NakedCleanUpItemHash);
}

void SetupClassExpansion()
{
	ReadEquipmentClassMods();

	auto common = (DWORD)GetModuleHandleA("common.dll");
	hpTableOffset += common;

	gunReadReplacementOffset += common;
	gunReadSearchVanillaEntry += common;
	gunReadFoundCustomEntry += common;
	shieldReadFoundCustomEntry += common;
	shieldReadReplacementOffset += common;
	shieldReadSearchVanillaEntry += common;

	CommonPatches();
	
	if (Utils::Memory::GetCurrentExe() == "Freelancer.exe")
	{
		FreelancerExePatches();
	}	
}