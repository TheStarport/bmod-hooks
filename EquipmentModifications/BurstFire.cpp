#include "Main.hpp"
#include "../Utils.hpp"

struct BurstWeapon
{
	int remainingShots;
	float timeBetweenShots;
	float timeUntilNextShot;
	bool readyToFire;
	Vector originalFirePoint{};

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

typedef FireResult(__fastcall* CEFireType)(CELauncher* launcher, void* edx, Vector const&);

static std::vector<std::shared_ptr<BurstWeapon>> queuedShots;
static std::map<uint, CustomBurstWeaponArch*> customMissileArchList;

PBYTE ceGunFireData;
CEFireType ceGunFirePtr;

typedef void* (__fastcall* UnkClassType)(void* ptr, void* edx, CELauncher* launcher, void* unk);
UnkClassType unkPtr;
PBYTE unkData;

std::map<CELauncher*, void*> weirdFreelancerClassMap;
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
		if (a == FR_Success)
		{
			auto burst = std::make_shared<BurstWeapon>();
			burst->gun = gun;
			burst->timeUntilNextShot = customArch->second->timeBetweenShots;
			burst->timeBetweenShots = customArch->second->timeBetweenShots;
			burst->remainingShots = customArch->second->totalShots - 1;
			queuedShots.emplace_back(burst);
			burst->originalFirePoint = vector;
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

		if (a != FR_Success)
		{
			queuedShots.erase(repeaterIter);
			*(reinterpret_cast<float*>(reinterpret_cast<unsigned>(gun) + 0x50)) = refire;
		}

		return a;
	}

	return FR_RefireDelayNotElapsed;
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

		shot->timeUntilNextShot -= delta;

		if (shot->timeUntilNextShot <= 0)
		{
			shot->readyToFire = true;

			void* randClass = weirdFreelancerClassMap[shot->gun];

			typedef DWORD(__fastcall* FireFunctionType)(void* thing, void* edx, Vector& vec);
			const FireFunctionType fireFunc = (FireFunctionType)((*(void***)randClass)[3]);
			fireFunc(randClass, nullptr, shot->originalFirePoint);

			shot->remainingShots--;
			shot->timeUntilNextShot = shot->timeBetweenShots;
		}
	}
}

void* __fastcall UnkDetour(void* ptr, void* edx, CELauncher* launcher, void* unk)
{
	Utils::Memory::UnDetour(PBYTE(unkPtr), unkData);
	const auto a = unkPtr(ptr, edx, launcher, unk);
	Utils::Memory::Detour(PBYTE(unkPtr), UnkDetour, unkData);

	weirdFreelancerClassMap[launcher] = a;
	return a;
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
		ini.open(file.c_str(), false);
		while (ini.read_header())
		{
			if (ini.is_header("Gun"))
			{
				auto* arch = new CustomBurstWeaponArch();
				std::string nickname;
				bool ready = false;
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
						nickname = ini.get_value_string();
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
				}

				if (nickname.empty() || !ready)
				{
					delete arch;
					continue;
				}

				if (arch->totalShots <= 0)
				{
					arch->totalShots = 1;
				}

				uint hash = CreateID(nickname.c_str());
				if (auto existing = customMissileArchList.find(hash); existing != customMissileArchList.end())
				{
					delete existing->second;
				}

				customMissileArchList[CreateID(nickname.c_str())] = arch;
			}
		}

		ini.close();
	}

	ceGunFireData = PBYTE(malloc(5));
	ceGunFirePtr = CEFireType(GetProcAddress(GetModuleHandleA("common.dll"), "?Fire@CEGun@@UAE?AW4FireResult@@ABVVector@@@Z"));
	Utils::Memory::Detour(PBYTE(ceGunFirePtr), CEGunFireDetour, ceGunFireData);

	unkData = PBYTE(malloc(5));
	unkPtr = UnkClassType(0x52D880);
	Utils::Memory::Detour(PBYTE(unkPtr), UnkDetour, unkData);
}
