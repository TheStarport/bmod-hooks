#include "Main.hpp"

int __fastcall GetIDS(Archetype::Equipment* object, CEquip* item)
{
	if (CEGun::cast(item))
	{
		CEGun* gun = (CEGun*)item;
		return gun->MunitionArch()->iIdsName;
	}

	else if (CEMineDropper::cast(item))
	{
		CEMineDropper* mine = (CEMineDropper*)item;
		return mine->MineArch()->iIdsName;
	}

	else if (CECounterMeasureDropper::cast(item))
	{
		CECounterMeasureDropper* cm = (CECounterMeasureDropper*)item;
		return cm->CounterMeasureArch()->iIdsName;
	}

	return (object->iIdsName); // original IDS
}

DWORD idsJump = 0x004DDB05;
__declspec(naked) void PatchIDS(void)
{
	__asm {
		pushad
		mov ecx, eax
		mov edx, esi
		call GetIDS
		mov edi, eax
		popad
		mov ecx, esi
		jmp[idsJump]
	}
}

constexpr uint ShipOverrideIDS = 459593;
#define ShipWeaponInfoIDS 459592
Archetype::Ship* curShip = nullptr;
int __fastcall GetShipIDS(Archetype::Ship* ship)
{
	curShip = ship;
	return ShipOverrideIDS; // Some arbitrary value I copied. Defaults to the patriot information.
}

DWORD idsShipJump = 0x47BD44;
static bool isInventory = false;
__declspec(naked) void PatchShipInfoInventoryIDS(void)
{
	__asm {
		mov eax, [ebp + 0x0EC]
		mov edx, [ebp + 0x014]
		mov ecx, [ebp + 0x018]
		push esi
		push eax
		push edi
		push ecx
		push edx
		push ebp
		mov  ecx, ebp
		mov  isInventory, 1
		call GetShipIDS
		pop ebp
		pop edx
		pop ecx
		pop edi
		mov ecx, eax
		pop eax
		pop esi
		jmp[idsShipJump]
	}
}

DWORD idsShipJump2 = 0x4B8C12;
__declspec(naked) void PatchShipInfoSellerIDS(void)
{
	__asm {
		mov ecx, [edi + 0x0F0]
		mov edx, [edi + 0x0F4]
		mov ebp, [edi + 0x0EC]
		push esi
		push eax
		push edi
		push ecx
		push edx
		push ebp
		mov  ecx, edi
		mov  isInventory, 0
		call GetShipIDS
		pop ebp
		pop edx
		pop ecx
		pop edi
		mov edx, eax
		pop eax
		pop esi
		jmp[idsShipJump2]
	}
}

void IdsPatch()
{
#define ADDR_IDS    ((PBYTE)0x4ddb00)
#define ADDR_IDS_SHIP_SELLER   ((PBYTE)0x4B8C00)
#define ADDR_IDS_SHIP_INVENTORY   ((PBYTE)0x47BD38)

	ProtectExecuteReadWrite(ADDR_IDS, 5);
	ADDR_IDS[0] = 0xe9;
	*(DWORD*)(ADDR_IDS + 1) = (PBYTE)PatchIDS - ADDR_IDS - 5;

	ProtectExecuteReadWrite(ADDR_IDS_SHIP_SELLER, 5);
	ADDR_IDS_SHIP_SELLER[0] = 0xe9;
	*(DWORD*)(ADDR_IDS_SHIP_SELLER + 1) = (PBYTE)PatchShipInfoSellerIDS - ADDR_IDS_SHIP_SELLER - 5;

	ProtectExecuteReadWrite(ADDR_IDS_SHIP_INVENTORY, 5);
	ADDR_IDS_SHIP_INVENTORY[0] = 0xe9;
	*(DWORD*)(ADDR_IDS_SHIP_INVENTORY + 1) = (PBYTE)PatchShipInfoInventoryIDS - ADDR_IDS_SHIP_INVENTORY - 5;
}

float RadiansToDegrees(float rad)
{
	constexpr float ratio = 180.0f / 3.1415926f;
	return rad * ratio;
}

float GetTurnRate(float torque, float angularDrag, float rotationalInertia)
{
	constexpr float time = 1.0f;
	constexpr float e = 2.718281f;
	constexpr float percent = 0.9f;
	float maxTurn = torque / angularDrag;
	float k = rotationalInertia / angularDrag;
	float percentOfMaxTurn = maxTurn / percent;
	float timeToTurn = -k * std::log(1.0f - percentOfMaxTurn);

	float angularVelocityWithRespectToTime = maxTurn * (1.0f - std::pow(e, -time / k));
	return angularVelocityWithRespectToTime;
}

bool HandleShipInfocard(uint ids, RenderDisplayList& rdl)
{
	if (!curShip)
	{
		curShip = Utils::GetShipArch();
	}

	if (ids == ShipOverrideIDS)
	{
		std::wostringstream ss;
		ss << std::fixed; // Don't use e notation

		ss << LR"(<?xml version="1.0" encoding="UTF-16"?><RDL><PUSH/><TRA data="1" mask="1" def="-2"/><RDL><PUSH/><PARA/><TRA bold="false"/>)";

		Vector turnRate =
		{
			RadiansToDegrees(GetTurnRate(curShip->fSteeringTorque[0], curShip->fAngularDrag[0], curShip->fRotationInertia[0])),
			RadiansToDegrees(GetTurnRate(curShip->fSteeringTorque[1], curShip->fAngularDrag[1], curShip->fRotationInertia[1])),
			RadiansToDegrees(GetTurnRate(curShip->fSteeringTorque[2], curShip->fAngularDrag[2], curShip->fRotationInertia[2])),
		};

		std::map<HpAttachmentType, std::vector<CacheString>> hardpoints;

		for (auto hpType : curShip->vHardpoints)
		{
			auto hps = curShip->get_legal_hps(hpType);
			if (!hps)
			{
				continue;
			}

			hardpoints[hpType] = *hps;
		}

		std::map<std::string, std::vector<HpAttachmentType>> hardpointToHpType;
		for (auto& type : hardpoints) 
		{
			for (auto& hardpoint : type.second)
			{
				std::string hp = hardpoint.value;
				auto ref = hardpointToHpType.find(hp);
				if (ref == hardpointToHpType.end())
				{
					(hardpointToHpType[hp] = {});
					ref = hardpointToHpType.find(hp);
				}

				ref->second.emplace_back(type.first);
			}
		}

		int smallEnergyCount = 0;
		int mediumEnergyCount = 0;
		int largeEnergyCount = 0;

		int smallMissileCount = 0;
		int mediumMissileCount = 0;
		int largeMissileCount = 0;

		int smallBallisticCount = 0;
		int mediumBallisticCount = 0;
		int largeBallisticCount = 0;

		int smallFleetEnergyCount = 0;
		int mediumFleetEnergyCount = 0;
		int largeFleetEnergyCount = 0;

		int smallFleetMissileCount = 0;
		int mediumFleetMissileCount = 0;
		int largeFleetMissileCount = 0;

		int smallFleetBallisticCount = 0;
		int mediumFleetBallisticCount = 0;
		int pointDefenseCount = 0;

#define FindHp(id) std::ranges::find(hp.second, (id)) != hp.second.end()

		std::function<bool(std::pair<const std::string, std::vector<HpAttachmentType>>&, std::vector<std::pair<HpAttachmentType, int&>>&, int index)> matchGroup;
		matchGroup = [&matchGroup](std::pair<const std::string, std::vector<HpAttachmentType>>& hp, std::vector<std::pair<HpAttachmentType, int&>>& pairs, int index = 0)
		{
			if (pairs.size() <= index)
			{
				return false;
			}

			auto& pair = pairs[index];
			if (FindHp(pair.first))
			{
				if (matchGroup(hp, pairs, index + 1))
				{	
					return false;
				}

				pair.second++;
				return true;
			}
			return false;
		};

		std::vector<std::pair<HpAttachmentType, int&>> energy =
		{
			{ hp_gun_special_1, smallEnergyCount },
			{ hp_gun_special_2, mediumEnergyCount },
			{ hp_gun_special_3, largeEnergyCount },
		};

		std::vector<std::pair<HpAttachmentType, int&>> missiles =
		{
			{ hp_gun_special_4, smallMissileCount },
			{ hp_gun_special_5, mediumMissileCount },
			{ hp_gun_special_6, largeMissileCount },
		};
		std::vector<std::pair<HpAttachmentType, int&>> ballistic =
		{
			{ hp_gun_special_7, smallBallisticCount },
			{ hp_gun_special_8, mediumBallisticCount },
			{ hp_gun_special_9, largeBallisticCount },
		};

		for (auto& hp : hardpointToHpType)
		{
			matchGroup(hp, energy, 0);
			matchGroup(hp, missiles, 0);
			matchGroup(hp, ballistic, 0);
			/*if (FindHp(hp_gun_special_1)) 
			{
				if (FindHp(hp_gun_special_2)) 
				{
					if (FindHp(hp_gun_special_3)) 
					{
						largeEnergyCount++;
						continue;
					}
					mediumEnergyCount++;
					continue;
				}
				smallEnergyCount++;
				continue;
			}
			else if (FindHp(hp_gun_special_4))
			{
				if (FindHp(hp_gun_special_5))
				{
					if (FindHp(hp_gun_special_6))
					{
						largeMissileCount++;
						continue;
					}
					mediumMissileCount++;
					continue;
				}
				smallMissileCount++;
				continue;				
			}
			else if (FindHp(hp_gun_special_7))
			{
				if (FindHp(hp_gun_special_8))
				{
					if (FindHp(hp_gun_special_9))
					{
						largeBallisticCount++;
						continue;
					}
					mediumBallisticCount++;
					continue;
				}
				smallBallisticCount++;
				continue;
			}
			else if (FindHp(hp_turret_special_1))
			{
				if (FindHp(hp_turret_special_2))
				{
					if (FindHp(hp_turret_special_3))
					{
						largeFleetEnergyCount++;
						continue;
					}
					mediumFleetEnergyCount++;
					continue;
				}
				smallFleetEnergyCount++;
				continue;
			}
			else if (FindHp(hp_turret_special_4))
			{
				if (FindHp(hp_turret_special_5))
				{
					if (FindHp(hp_turret_special_6))
					{
						largeFleetMissileCount++;
						continue;
					}
					mediumFleetMissileCount++;
					continue;
				}
				smallFleetMissileCount++;
				continue;
			}
			else if (FindHp(hp_turret_special_7))
			{
				if (FindHp(hp_turret_special_8))
				{
					mediumFleetBallisticCount++;
					continue;
				}
				smallFleetBallisticCount++;
				continue;
			}
			else if (FindHp(hp_turret_special_9))
			{
				pointDefenseCount++;
				continue;
			}*/
		}

#undef FindHp
		// x = class
		// y = hardpoint
		// for each item in list
		// if y found in other class
		// remove y from old list and store y in new list

		// list A = all hardpoints with multi-types removed
		// list B = all hardpoints that can mount multi-types

		std::wstring equipment = L"";
		// Weapons: 2x ME, 2x SM, 1x MM, 1xLB

		auto appendEquipment = [&equipment](int count, std::wstring prefix)
		{
			if (count) 
			{
				equipment += std::format(L"{}x{}, ", count, prefix);
			}
		};

		appendEquipment(smallEnergyCount, L"SE");
		appendEquipment(mediumEnergyCount, L"ME");
		appendEquipment(largeEnergyCount, L"LE");
		appendEquipment(smallMissileCount, L"SM");
		appendEquipment(mediumMissileCount, L"MM");
		appendEquipment(largeMissileCount, L"LM");
		appendEquipment(smallBallisticCount, L"SB");
		appendEquipment(mediumBallisticCount, L"MB");
		appendEquipment(largeBallisticCount, L"LB");

		appendEquipment(smallFleetEnergyCount, L"STE");
		appendEquipment(mediumFleetEnergyCount, L"MTE");
		appendEquipment(largeFleetEnergyCount, L"LTE");
		appendEquipment(smallFleetMissileCount, L"STM");
		appendEquipment(mediumFleetMissileCount, L"MTM");
		appendEquipment(largeFleetMissileCount, L"LTM");
		appendEquipment(smallFleetBallisticCount, L"STB");
		appendEquipment(mediumFleetBallisticCount, L"MTB");
		appendEquipment(pointDefenseCount, L"PD");

		if (!equipment.empty())
		{
			auto back = equipment.back();
			if (back == L' ') 
			{
				equipment = equipment.substr(0, equipment.size() - 2);
			}
		}


		if (isInventory)
		{

			ss << L"<JUST loc=\"c\"/><TRA bold=\"true\"/><TEXT>Stats</TEXT><TRA bold=\"false\"/><PARA/><JUST loc=\"l\"/><PARA/>"
				<< L"<TEXT>Mass: " << std::setprecision(0) << curShip->fMass << L"t</TEXT><PARA/>"
				<< L"<TEXT>Armor: " << curShip->fHitPoints << L"</TEXT><PARA/>"
				<< L"<TEXT>Cargo Space: " << curShip->fHoldSize << L"m³</TEXT><PARA/>"
				<< L"<TEXT>Turn Rate: " << std::setprecision(1) << turnRate.x << L"°/s</TEXT><PARA/>";
				if (!equipment.empty())
				{
					ss << L"<TEXT>Hardpoints: </TEXT><TRA data=\"0xE7C68490\" mask=\"-1\"/><TEXT>" << equipment << L"</TEXT><PARA/>";
				}
		}
		else
		{
			ss << L"<PARA/><TEXT>" << static_cast<int>(curShip->fMass) << L"t</TEXT><PARA/>"
				<< L"<TEXT>" << static_cast<int>(curShip->fHitPoints) << L"</TEXT><PARA/>"
				<< L"<TEXT>" << static_cast<int>(curShip->fHoldSize) << L"m³</TEXT><PARA/>"
				<< L"<TEXT>" << std::setprecision(1) << turnRate.x << L"°/s</TEXT><PARA/>"
				<< L"<TRA data=\"0xE7C68490\" mask=\"-1\"/><TEXT>" << equipment << L"</TEXT><PARA/>";
		}

		ss << L"<PARA/><POP/></RDL>";
		XMLReader reader;
		reader.read_buffer(rdl, reinterpret_cast<const char*>(ss.str().c_str()), wcslen(ss.str().c_str()) * 2);

		return true;
	}
	else if (ids == ShipWeaponInfoIDS)
	{
		static std::wstring ret;
		if (ret.empty())
		{
			ret += std::format(L"<RDL><PUSH/><TRA bold=\"true\"/><TEXT>Stats</TEXT><TRA bold=\"false\"/><PARA/><PARA/>");
			ret += std::format(L"<TEXT>Mass:</TEXT><PARA/>");
			ret += std::format(L"<TEXT>Armor:</TEXT><PARA/>");
			ret += std::format(L"<TEXT>Cargo Space:</TEXT><PARA/>");
			ret += std::format(L"<TEXT>Turn Rate:</TEXT><PARA/>");
			ret += std::format(L"<TEXT>Hardpoints:</TEXT><PARA/>");
			ret += L"<POP/></RDL>";
		}

		XMLReader reader;
		reader.read_buffer(rdl, reinterpret_cast<const char*>(ret.c_str()), wcslen(ret.c_str()) * 2);
		return true;

	}

	return false;
}

typedef int(__cdecl* _GetString)(char* rsrc, uint ids, wchar_t* buf, int buf_chars);
extern _GetString GetString = (_GetString)0x4347E0;

typedef int(__cdecl* _FormatXML)(const wchar_t* buf, int buf_chars, RenderDisplayList& rdl, int iDunno);
extern _FormatXML FormatXML = (_FormatXML)0x57DB50;

/// Return true if we override the default infocard
bool __stdcall HkCb_GetInfocard(unsigned int ids_number, RenderDisplayList& rdl)
{
	bool handled = HandleShipInfocard(ids_number, rdl);
	if (handled)
	{
		return true;
	}

	return false;
}

/// Naked function hook for the infocard function.
__declspec(naked) void HkCb_GetInfocardNaked()
{
	__asm {
		push[esp + 8]
		push[esp + 8]
		call HkCb_GetInfocard
		cmp al, 1
		jnz   infocard_not_found
		retn
		infocard_not_found :
		mov edx, ds : [0x67C400]
			push ebx
			push 0x057DA47
			ret
	}
}

void InitInfocardEdits()
{
	IdsPatch();

	// Patch get infocard
	{
		BYTE patch[] = { 0xB9, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE1 }; // mov ecx HkCb_GetInfocardNaked, jmp ecx
		DWORD* iAddr = (DWORD*)((char*)&patch + 1);
		*iAddr = reinterpret_cast<DWORD>((void*)HkCb_GetInfocardNaked);
		Utils::Memory::WriteProcMem((char*)0x057DA40, &patch, 7);
	}
}