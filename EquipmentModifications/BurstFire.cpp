#include "Main.hpp"
#include "../Utils.hpp"

struct BurstWeapon
{
	int remainingShots;
	float timeBetweenShots;
	float timeUntilNextShot;
	bool readyToFire;
	Vector originalFirePoint{};
	Vector updatedFirePoint{};

	CEGun* gun;

	BurstWeapon()
	{
		remainingShots = 3;
		timeBetweenShots = 1.0f;
		timeUntilNextShot = 1.0f;
		readyToFire = false;
		originalFirePoint = Vector();

		gun = nullptr;
	}
};

struct CustomBurstWeaponArch
{
	float timeBetweenShots;
	int totalShots;

	CustomBurstWeaponArch()
	{
		timeBetweenShots = 1.0f;
		totalShots = 1;
	}
};

struct BurstFireData
{
	int magCapacity;
	float reloadTime;
};
struct BurstFireInfo
{
	ushort magCapacity;
	ushort magStatus;
	float reloadDuration;
	bool isReloading;
};
std::unordered_map<ushort, BurstFireInfo> playerMagClips;
static std::unordered_map<uint, BurstFireData> burstFireData;

void InitBurstFireEquipment()
{
	CShip* cship = Utils::GetCShip();

	if (!cship)
	{
		return;
	}

	playerMagClips.clear();
	CEquipTraverser tr(EquipmentClass::Gun | EquipmentClass::CM | EquipmentClass::MineLauncher);
	CELauncher* launcher = 0;
	while (launcher = (CELauncher*)cship->equip_manager.Traverse(tr))
	{
		launcher->refireDelayElapsed = 60.f;
		auto burstData = burstFireData.find(launcher->archetype->iArchID);
		if (burstData == burstFireData.end())
		{
			continue;
		}

		BurstFireInfo& magData = playerMagClips[launcher->iSubObjId];
		magData.magCapacity = burstData->second.magCapacity;
		magData.magStatus = burstData->second.magCapacity;
		magData.reloadDuration = launcher->LauncherArch()->fRefireDelay - burstData->second.reloadTime;
		magData.isReloading = false;
	}
}

typedef FireResult(__fastcall* CEFireType)(CELauncher* launcher, void* edx, Vector const&);

static std::vector<std::shared_ptr<BurstWeapon>> queuedShots;
static std::unordered_map<uint, CustomBurstWeaponArch*> customMissileArchList;

PBYTE ceGunFireData;
CEFireType ceGunFirePtr;

typedef void* (__fastcall* UnkClassType)(void* ptr, void* edx, CELauncher* launcher, void* unk);
UnkClassType unkPtr;
PBYTE unkData;

// Map of launcher to pair of unk class pointer and ship owner id
std::unordered_map<CELauncher*, std::pair<void*, uint>> weirdFreelancerClassMap;

void __fastcall PlayerConsumeFireResourcesDetour(CELauncher* launcher)
{
	if (Utils::GetClientId() != launcher->owner->ownerPlayer)
	{
		return;
	}

	auto burstIter = playerMagClips.find(launcher->iSubObjId);
	if (burstIter == playerMagClips.end())
	{
		return;
	}
	BurstFireInfo& burstInfo = burstIter->second;

	uint remainingAmmo = --burstInfo.magStatus;

	if (remainingAmmo == 0)
	{
		burstInfo.isReloading = true;
		burstInfo.magStatus = burstInfo.magCapacity;
		launcher->refireDelayElapsed = burstInfo.reloadDuration;
	}
}

FireResult __fastcall CEGunFireDetour(CEGun* gun, void* edx, const Vector& vector)
{
	// If we are not a custom burst 
	const auto customArch = customMissileArchList.find(gun->archetype->iArchID);
	if (customArch == customMissileArchList.end())
	{
		Utils::Memory::UnDetour(PBYTE(ceGunFirePtr), ceGunFireData);
		const auto a = ceGunFirePtr(gun, edx, vector);
		Utils::Memory::Detour(PBYTE(ceGunFirePtr), CEGunFireDetour, ceGunFireData);
		return a;
	}

	const auto repeaterIter = std::find_if(queuedShots.begin(), queuedShots.end(), [&g = gun](std::shared_ptr<BurstWeapon> burst) -> bool { return burst->gun == g; });
	if (repeaterIter == queuedShots.end())
	{
		Utils::Memory::UnDetour(PBYTE(ceGunFirePtr), ceGunFireData);
		const auto a = ceGunFirePtr(gun, edx, vector);
		Utils::Memory::Detour(PBYTE(ceGunFirePtr), CEGunFireDetour, ceGunFireData);

		// Only queue up successful shots
		if (a == FireResult::Success)
		{
			auto burst = std::make_shared<BurstWeapon>();
			burst->gun = gun;
			burst->timeUntilNextShot = customArch->second->timeBetweenShots;
			burst->timeBetweenShots = customArch->second->timeBetweenShots;
			burst->remainingShots = customArch->second->totalShots - 1;
			burst->originalFirePoint = vector;
			queuedShots.emplace_back(burst);
		}

		return a;
	}
	else if (repeaterIter->get()->readyToFire)
	{
		// Reset the refire right before we fire. Power requirements are a thing still though.
		// We want to store the original refire, for the event of failure.
		const float refire = *(reinterpret_cast<float*>(reinterpret_cast<unsigned>(gun) + 0x50));
		*(reinterpret_cast<float*>(reinterpret_cast<unsigned>(gun) + 0x50)) = *reinterpret_cast<float*>(*(reinterpret_cast<unsigned*>(gun) + 3) + 144);

		repeaterIter->get()->readyToFire = false;

		Utils::Memory::UnDetour(PBYTE(ceGunFirePtr), ceGunFireData);
		const auto a = ceGunFirePtr(gun, edx, vector);
		Utils::Memory::Detour(PBYTE(ceGunFirePtr), CEGunFireDetour, ceGunFireData);

		if (a != FireResult::Success)
		{
			queuedShots.erase(repeaterIter);
			*(reinterpret_cast<float*>(reinterpret_cast<unsigned>(gun) + 0x50)) = refire;
		}

		return a;
	}
	return  FireResult::RefireDelayNotElapsed;
}

void UpdateQueuedShots(double& delta)
{
	for (size_t i = 0; i < queuedShots.size(); i++)
	{
		const auto& shot = queuedShots[i];

		if (shot->remainingShots <= 0)
		{
			queuedShots.erase(queuedShots.begin() + i--);
			continue;
		}

		shot->timeUntilNextShot -= static_cast<float>(delta);

		if (shot->timeUntilNextShot <= 0)
		{
			shot->readyToFire = true;
			const auto Gun = shot->gun;

			auto randClass = weirdFreelancerClassMap[shot->gun];
			const auto ship = Utils::GetCShip();

			shot->updatedFirePoint = Gun->GetAvgBarrelDirWS();

			shot->updatedFirePoint.x = shot->updatedFirePoint.x * 10000000000;
			shot->updatedFirePoint.y = shot->updatedFirePoint.y * 10000000000;
			shot->updatedFirePoint.z = shot->updatedFirePoint.z * 10000000000;

			if (Gun->CanFire(shot->updatedFirePoint) == CELauncher::FireResult::Success|| ship->get_hit_pts() <= 0 || !CObject::Find(randClass.second, CObject::Class::CSHIP_OBJECT) || Gun->IsDestroyed())
			{
				shot->remainingShots = 0;
				return;
			}
			else {

				typedef DWORD(__fastcall* FireFunctionType)(void* thing, void* edx, Vector& vec);
				const FireFunctionType fireFunc = (FireFunctionType)((*(void***)randClass.first)[3]);
				fireFunc(randClass.first, nullptr, shot->updatedFirePoint);
			}
			shot->remainingShots--;
			shot->timeUntilNextShot = shot->timeBetweenShots;
		}

	}
}

void __cdecl DelayDisplayFunc(ushort& sId, char* buffer, int unknown, int remainingAmmo)
{
	CShip* cship = Utils::GetCShip();
	CELauncher* launcher = (CELauncher*)cship->equip_manager.FindByID(sId);

	float refireRate = launcher->LauncherArch()->fRefireDelay;

	float remainingCooldown = refireRate - launcher->refireDelayElapsed;
	auto burstInfo = playerMagClips.find(sId);
	if (burstInfo != playerMagClips.end())
	{
		if (burstInfo->second.isReloading)
		{
			if (remainingCooldown > 0.1f)
			{
				std::string reloadingTimer;
				mstime timestampmod = Utils::timeInMS() % 600;
				if (timestampmod < 150)
				{
					reloadingTimer = ".";
				}
				else if (timestampmod < 300)
				{
					reloadingTimer = "..";
				}
				else if (timestampmod < 450)
				{
					reloadingTimer = "...";
				}
				else
				{
					reloadingTimer = "";
				}
				if (remainingAmmo == -1)
				{
					sprintf(buffer, "%.1f%s", remainingCooldown, reloadingTimer.c_str());
				}
				else
				{
					sprintf(buffer, "%d-%.1f%s", remainingAmmo, remainingCooldown, reloadingTimer.c_str());
				}
				return;
			}

			burstInfo->second.isReloading = false;
		}

		uint length = 0;
		if (remainingAmmo != -1)
		{
			length = sprintf(buffer, "%d-", remainingAmmo);
		}

		if (refireRate < 1.0f)
		{
			sprintf(buffer + length, "%d", burstInfo->second.magStatus);
		}
		else if (remainingCooldown <= 0)
		{
			sprintf(buffer + length, "%d-0.0", burstInfo->second.magStatus);
		}
		else
		{
			sprintf(buffer + length, "%d-%.1f", burstInfo->second.magStatus, remainingCooldown);
		}
		return;
	}

	if (remainingAmmo == -1)
	{
		if (refireRate < 1.0)
		{
			sprintf(buffer, "\0");
		}
		else
		{
			if (remainingCooldown <= 0)
			{
				sprintf(buffer, "0.0");
			}
			else
			{
				sprintf(buffer, "%.1f", remainingCooldown);
			}
		}
		return;
	}

	if (refireRate < 1.0f)
	{
		sprintf(buffer, "%d", remainingAmmo);
		return;
	}

	if (remainingCooldown <= 0)
	{
		sprintf(buffer, "%d--0.0", remainingAmmo);
	}
	else
	{
		sprintf(buffer, "%d--%.1f", remainingAmmo, remainingCooldown);
	}
}

void __declspec(naked) DelayDisplay()
{
	__asm
	{
		push ecx
		push esi
		call DelayDisplayFunc
		pop esi
		add esp, 0x4
		pop eax
		add esp, 0x4
		add eax, 0xB
		mov ecx, [edi + 0x50]
		jmp eax
	}
}

void* __fastcall UnkDetour(void* ptr, void* edx, CELauncher* launcher, void* unk)
{
	Utils::Memory::UnDetour(PBYTE(unkPtr), unkData);
	const auto a = unkPtr(ptr, edx, launcher, unk);
	Utils::Memory::Detour(PBYTE(unkPtr), UnkDetour, unkData);

	weirdFreelancerClassMap[launcher] = { a, launcher->owner->id };
	return a;
}

typedef void(__fastcall* PlayerLaunchType)(void* unk);
void __fastcall PlayerLaunchDetour(void* unk, void* edx)
{
	const static PlayerLaunchType playerLaunchFunc = PlayerLaunchType(DWORD(GetModuleHandleA(nullptr)) + 0x14B520);

	playerLaunchFunc(unk);

	InitBurstFireEquipment();
}

void InitBurstMod()
{
	// Vector to store our ini files
	std::vector<std::string> files;

	INI_Reader ini;
	if (ini.open("freelancer.ini", false))
	{
		ini.find_header("Data");
		while (ini.read_value())
		{
			if (ini.is_value("equipment"))
				files.emplace_back(std::string("..\\DATA\\") + ini.get_value_string());
		}
	}

	ini.close();

	for (auto& file : files)
	{
		if (!ini.open(file.c_str(), false))
		{
			continue;
		}
		while (ini.read_header())
		{
			uint nickname = 0;
			if (ini.is_header("CounterMeasureDropper"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						nickname = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("burst_fire"))
					{
						float baseRefire = ((Archetype::MineDropper*)Archetype::GetEquipment(nickname))->fRefireDelay;
						burstFireData[nickname] = { ini.get_value_int(0), baseRefire - ini.get_value_float(1) };
					}
				}
			}
			else if (ini.is_header("MineDropper"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						nickname = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("burst_fire"))
					{
						float baseRefire = ((Archetype::MineDropper*)Archetype::GetEquipment(nickname))->fRefireDelay;
						burstFireData[nickname] = { ini.get_value_int(0), baseRefire - ini.get_value_float(1) };
					}
				}
			}
			else if (ini.is_header("Gun"))
			{
				auto* arch = new CustomBurstWeaponArch();
				bool ready = false;
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						nickname = CreateID(ini.get_value_string());
					}
					else if (ini.is_value("total_projectiles_per_fire"))
					{
						arch->totalShots = ini.get_value_int(0);
						ready = true;
					}
					else if (ini.is_value("time_between_multiple_projectiles"))
					{
						arch->timeBetweenShots = ini.get_value_float(0);
						ready = true;
					}
					else if (ini.is_value("burst_fire"))
					{
						burstFireData[nickname] = { ini.get_value_int(0), ini.get_value_float(1) };
					}
				}

				if (!nickname || !ready)
				{
					delete arch;
					continue;
				}

				if (arch->totalShots <= 0)
				{
					arch->totalShots = 1;
				}

				if (auto existing = customMissileArchList.find(nickname); existing != customMissileArchList.end())
				{
					delete existing->second;
				}

				customMissileArchList[nickname] = arch;
			}
			else if (ini.is_header("Mine"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						nickname = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("ammo_limit"))
					{
						ammoMap[nickname] = { ini.get_value_int(0), std::max(1, ini.get_value_int(1)) };
					}
				}
			}
			else if (ini.is_header("CounterMeasure"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						nickname = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("ammo_limit"))
					{
						ammoMap[nickname] = { ini.get_value_int(0), std::max(1, ini.get_value_int(1)) };
					}
				}
			}
			else if (ini.is_header("Munition"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						nickname = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("ammo_limit"))
					{
						ammoMap[nickname] = { ini.get_value_int(0), std::max(1, ini.get_value_int(1)) };
					}
				}
			}
		}

		ini.close();
	}

	ceGunFireData = PBYTE(malloc(5));
	ceGunFirePtr = CEFireType(GetProcAddress(GetModuleHandleA("common.dll"), "?Fire@CEGun@@UAE?AW4FireResult@@ABVVector@@@Z"));
	Utils::Memory::DetourInit(PBYTE(ceGunFirePtr), CEGunFireDetour, ceGunFireData);


	auto hFreelancer = GetModuleHandleA(nullptr);
	auto hCommon = GetModuleHandleA("common");

	uint PlayerFireDetourSuccessFunc = (uint)PlayerConsumeFireResourcesDetour;
	uint* PlayerFireDetourSuccessFuncPtr = &PlayerFireDetourSuccessFunc;
	Utils::Memory::WriteProcMem((char*)hCommon + 0x13D108, PlayerFireDetourSuccessFuncPtr, 4);
	Utils::Memory::WriteProcMem((char*)hCommon + 0x13D040, PlayerFireDetourSuccessFuncPtr, 4);
	Utils::Memory::WriteProcMem((char*)hCommon + 0x13CF78, PlayerFireDetourSuccessFuncPtr, 4);
	Utils::Memory::PatchCallAddr((char*)hCommon, 0x37853, (char*)PlayerConsumeFireResourcesDetour);

	//Reimplement DelayDisplay by M0tah, with added Burst Fire features
	{
		BYTE patch[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xD0 };
		DWORD* iAddr = (DWORD*)((char*)&patch + 1);
		*iAddr = reinterpret_cast<DWORD>((void*)DelayDisplay);
		Utils::Memory::WriteProcMem((char*)hFreelancer + 0xDDFD5, patch, sizeof(patch));
		BYTE patch2[] = { 0x00 };
		Utils::Memory::WriteProcMem((char*)hFreelancer + 0xDDFC0, patch2, sizeof(patch2));
	}

	Utils::Memory::PatchCallAddr((char*)hFreelancer, 0x14ACB3, (char*)PlayerLaunchDetour);

	unkData = PBYTE(malloc(5));
	unkPtr = UnkClassType(0x52D880);
	Utils::Memory::DetourInit(PBYTE(unkPtr), UnkDetour, unkData);
}
