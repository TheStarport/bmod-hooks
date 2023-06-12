#include <Windows.h>
#include <string>
#include <list>
#include <vector>
#include <optional>
#include <array>
#include "../Utils.hpp"

const wchar_t* ifmt = L" %d";
const wchar_t* f0fmt = L" %.0f";
const wchar_t* f2fmt = L" %.2f";
const wchar_t* f0fmtPercent = L" %.0f%%";
const wchar_t* f2fmtS = L" %.2fs";
const wchar_t* f0fmtSpace = L" %.0f";
const wchar_t* ufmt = L" %u";
const wchar_t* f0fmtv = L" %.0f (%.0f/s)";

LPWSTR buf = (LPWSTR)0x66fc60; // internal WCHAR buf[4096]
typedef uint(*GetStringT)(void* rsrc, uint res, LPWSTR buf, uint len);
GetStringT GetString = (GetStringT)0x4347e0;

void GetStatLine(byte flags, uint* len, uint name_id, const wchar_t* value_fmt,
	uint suffix_id, ...)
{
	if (flags & 1)
		*len += GetString(NULL, name_id, buf + *len, 128);

	if (flags & 2)
	{
		if (flags & 4)
		{
			wcscpy(buf + *len, L"n/a");
			*len += 3;
		}
		else
		{
			va_list args;
			va_start(args, suffix_id);
			*len += _vswprintf(buf + *len, value_fmt, args);
			va_end(args);
			if (suffix_id)
				*len += GetString(NULL, suffix_id, buf + *len, 128);
		}
	}
	buf[(*len)++] = '\n';
}

uint SetPowerCoreInfocard(Archetype::Power* powercore, byte flags)
{
	// Energy Capacity:   1000
	// Regeneration Rate: 95/s
	uint len = 0;
	GetStatLine(flags, &len, 459503, ifmt, 0, (int)powercore->fCapacity);
	GetStatLine(flags, &len, 459504, ifmt, 459534, (int)powercore->fChargeRate);
	return len;
}

uint SetEngineInfocard(Archetype::Engine* engine, byte flags)
{
	// Top Speed:			80 m/s
	// Reverse Speed:       16 m/s
	// Cruise Charge Time:	5.00s
	// Cruise Energy Drain: 20/s
	uint len = 0;
	GetStatLine(flags, &len, 459505, ifmt, 1760, (int)(engine->fMaxForce / engine->fLinearDrag));
	GetStatLine(flags, &len, 459506, ifmt, 1760, (int)((engine->fMaxForce / engine->fLinearDrag) * engine->fReverseFraction));
	GetStatLine(flags, &len, 459507, f2fmtS, 0, engine->fCruiseChargeTime);
	GetStatLine(flags, &len, 459508, ifmt, 459534, (int)engine->fCruisePowerUsage);
	return len;
}

uint SetScannerInfocard(Archetype::Scanner* scanner, byte flags)
{
	// Range: 			 2500m
	// Cargo Scan Range: 2000m
	uint len = 0;
	GetStatLine(flags, &len, 459509, ifmt, 1759, (int)scanner->fRange);
	GetStatLine(flags, &len, 459510, ifmt, 1759, (int)scanner->fCargoScanRange);
	return len;
}

uint SetTractorInfocard(Archetype::Tractor* tractor, byte flags)
{
	// Range:        1500m
	// Extend Speed: 2000 m/s
	uint len = 0;
	GetStatLine(flags, &len, 459509, ifmt, 1759, (int)tractor->fMaxLength);
	GetStatLine(flags, &len, 459511, ifmt, 1760, (int)tractor->fReachSpeed);
	return len;
}

uint SetArmorInfocard(Archetype::Armor* armor, byte flags)
{
	// Armor Improvement: 17%
	uint len = 0;
	GetStatLine(flags, &len, 459512, f0fmtPercent, 0, (armor->fHitPointsScale - 1) * 100);
	return len;
}

uint SetShieldGeneratorInfocard(Archetype::ShieldGenerator* shieldgenerator, byte flags)
{
	// Shield Capacity:		  3570 u
	// Regeneration Rate:	  320 u/s
	// Rebuild Time:		  9.45s
	// Rebuild Energy Drain:  41
	// Constant Energy Drain: 271
	uint len = 0;
	GetStatLine(flags, &len, 459513, f0fmt, 459532, shieldgenerator->fMaxCapacity * (1.0f - shieldgenerator->fOfflineThreshold));
	GetStatLine(flags, &len, 459514, f0fmt, 459533, shieldgenerator->fRegenerationRate);
	GetStatLine(flags, &len, 459515, f2fmtS, 0, shieldgenerator->fOfflineRebuildTime);
	GetStatLine(flags, &len, 459516, f0fmtSpace, 459534, shieldgenerator->fRebuildPowerDraw);
	GetStatLine(flags, &len, 459517, f0fmtSpace, 459534, shieldgenerator->fConstantPowerDraw);
	return len;
}

uint SetGunInfocard(Archetype::Gun* gun, byte flags)
{
	// Hull Damage: 250 (1000/s)
	// Shield Damage: 251 (500/s)
	// Energy Usage: 49 (492/s)
	// Refire Rate: 3.03/s
	// Projectile Speed: 1776 m/s
	// Top Speed: 500 m/s
	// Range: 742m
	// Spread: 2°
	// Projectile Turn Rate: 15°/s 
	uint len = 0;
	uint munitionId = gun->iProjectileArchID;
	auto munition = reinterpret_cast<Archetype::Munition*>(Archetype::GetEquipment(munitionId));
	uint motorId = munition->iMotorID;
	auto motor = Archetype::GetMotor(motorId);
	uint explosionId = munition->iExplosionArchID;
	ID_String ExplosionIDS;
	ExplosionIDS.id = explosionId;
	auto explosion = Archetype::GetExplosion(ExplosionIDS);

	if (munition->fHullDamage > 0)
	{
		GetStatLine(flags, &len, 459518, f0fmtv, 0, munition->fHullDamage, (munition->fHullDamage * (1.0f / gun->fRefireDelay)));
	}
	if (munition->fHullDamage < 0)
	{
		//If fHullDamage damage is negative, change the infocard line to the repair gun one and convert to a positive number.
		GetStatLine(flags, &len, 459519, f0fmtv, 0, munition->fHullDamage * -1, ((munition->fHullDamage * -1) * (1.0f / gun->fRefireDelay)));
	}
	if (munition->fEnergyDamage != 0)
	{
		GetStatLine(flags, &len, 459520, f0fmtv, 0, munition->fEnergyDamage, (munition->fEnergyDamage * (1.0f / gun->fRefireDelay)));
	}
	if (explosionId != 0)
	{
		if (explosion->fHullDamage > 0)
		{
			GetStatLine(flags, &len, 459518, f0fmtv, 0, explosion->fHullDamage, (explosion->fHullDamage * (1.0f / gun->fRefireDelay)));
		}
		if (explosion->fHullDamage < 0)
		{
			//If fHullDamage damage is negative, change the infocard line to the repair gun one and convert to a positive number.
			GetStatLine(flags, &len, 459518, f0fmtv, 0, explosion->fHullDamage * -1, ((explosion->fHullDamage * -1) * (1.0f / gun->fRefireDelay)));
		}
		if (explosion->fEnergyDamage != 0)
		{
			GetStatLine(flags, &len, 459520, f0fmtv, 0, explosion->fEnergyDamage, (explosion->fEnergyDamage * (1.0f / gun->fRefireDelay)));
		}
	}
	if (gun->fPowerUsage > 0.0f)
	{
		GetStatLine(flags, &len, 459522, f0fmtv, 0, gun->fPowerUsage, (gun->fPowerUsage * (1.0f / gun->fRefireDelay)));
	}
	GetStatLine(flags, &len, 459523, f2fmt, 459534, (1.0f / gun->fRefireDelay));
	if (motorId == 0)
	{
		GetStatLine(flags, &len, 459524, f0fmt, 1760, gun->fMuzzleVelocity);
	}
	if (motorId != 0)
	{
		//If it's a missile, display top speed instead of velocity.
		GetStatLine(flags, &len, 459525, f0fmt, 1760, motor->fLifetime * motor->fAccel + gun->fMuzzleVelocity);
	}
	if (motorId == 0)
	{
		GetStatLine(flags, &len, 459526, f0fmt, 1759, (munition->fLifeTime * gun->fMuzzleVelocity));
	}
	if (motorId != 0)
	{
		//If the munition has a motor, then use the missile range formula instead (This one is currently wrong). 
		auto topSpeed = motor->fLifetime * motor->fAccel;
		auto accelDuration = munition->fLifeTime - motor->fLifetime;
		auto totalVelocity = gun->fMuzzleVelocity * motor->fLifetime + 1 / 2 * motor->fAccel * std::pow(motor->fLifetime, 2);
		GetStatLine(flags, &len, 459526, f0fmt, 1759, totalVelocity + (accelDuration * topSpeed));
	}
	if (gun->fDispersionAngle > 0.0f and munition->fMaxAngularVelocity == 100.0f or gun->iProjectileArchID == 2356034817 or gun->iProjectileArchID == 2782560389)
	{
		//If the gun has dispersion angle and isn't a missile, display it. Specific exceptions for bmod_ugb_01 and bmod_rocket_01
		GetStatLine(flags, &len, 459527, f2fmt, 459535, (gun->fDispersionAngle * 180.0f / 3.141592653f));
	}
	if (munition->iExplosionArchID != 0 and munition->fMaxAngularVelocity > 0.1f)
	{
		//If it's a missile and it can turn, display it's turn rate.
		GetStatLine(flags, &len, 459528, f0fmt, 459536, (munition->fMaxAngularVelocity * 180.0f / 3.141592653f));
	}
	if (munition->fDetonationDist > 0)
	{
		GetStatLine(flags, &len, 459537, f0fmt, 1759, munition->fDetonationDist);
	}
	return len;
}

uint SetMineDropperInfocard(Archetype::MineDropper* minedropper, byte flags)
{
	// Hull Damage: 250
	// Shield Damage: 251
	// Explosion Radius: 30m
	// Refire Rate: 3.03/s
	// Top Speed: 30 m/s
	// Lifetime: 10.50s
	// Seeker Distance: 100m
	// Energy Usage: 25 (If present)
	uint len = 0;
	uint munitionId = minedropper->iProjectileArchID;
	auto munition = reinterpret_cast<Archetype::Mine*>(Archetype::GetEquipment(munitionId));
	uint explosionId = munition->iExplosionArchID;
	ID_String ExplosionIDS;
	ExplosionIDS.id = explosionId;
	auto explosion = Archetype::GetExplosion(ExplosionIDS);

	if (explosion->fHullDamage > 0)
	{
		GetStatLine(flags, &len, 459518, f0fmt, 0, explosion->fHullDamage);
	}
	if (explosion->fHullDamage < 0)
	{
		GetStatLine(flags, &len, 459519, f0fmt, 0, explosion->fHullDamage * -1);
	}
	if (explosion->fEnergyDamage > 0)
	{
		GetStatLine(flags, &len, 459520, f0fmt, 0, explosion->fEnergyDamage);
	}
	GetStatLine(flags, &len, 459529, f0fmt, 1759, explosion->fRadius);
	GetStatLine(flags, &len, 459523, f2fmt, 459534, (1.0f / minedropper->fRefireDelay));
	GetStatLine(flags, &len, 459525, f0fmt, 1760, munition->fTopSpeed);
	GetStatLine(flags, &len, 459530, f2fmtS, 0, munition->fLifeTime);
	GetStatLine(flags, &len, 459531, f0fmt, 1759, munition->fSeekerDist);
	if (minedropper->fPowerUsage > 0.0f)
	{
		GetStatLine(flags, &len, 459522, f0fmt, 0, minedropper->fPowerUsage);
	}
	return len;
}

uint SetCounterMeasureDropperInfocard(Archetype::CounterMeasureDropper* countermeasuredropper, byte flags)
{
	// Decoy Effectiveness: 25%
	// Decoy Range: 1000m
	// Projectile Speed: 1500 m/s
	// Refire Rate: 8.33/s
	// Energy Usage: 5
	uint len = 0;
	uint munitionId = countermeasuredropper->iProjectileArchID;
	auto munition = reinterpret_cast<Archetype::CounterMeasure*>(Archetype::GetEquipment(munitionId));
	GetStatLine(flags, &len, 1751, ifmt, 0, munition->fDiversionPctg);
	GetStatLine(flags, &len, 1750, f0fmt, 1759, munition->fRange);
	GetStatLine(flags, &len, 1746, f0fmt, 1760, countermeasuredropper->fMuzzleVelocity);
	GetStatLine(flags, &len, 1747, f2fmt, 459534, (1.0f / countermeasuredropper->fRefireDelay));
	GetStatLine(flags, &len, 1748, f0fmt, 0, countermeasuredropper->fPowerUsage);
	return len;
}

uint SetMunitionInfocard(Archetype::Munition* munition, byte flags)
{
	// Decoy Effectiveness: 25%
	// Decoy Range: 1000m
	// Projectile Speed: 1500 m/s
	// Refire Rate: 8.33/s
	// Energy Usage: 5
	uint len = 0;
	uint explosionId = munition->iExplosionArchID;
	ID_String ExplosionIDS;
	ExplosionIDS.id = explosionId;
	auto explosion = Archetype::GetExplosion(ExplosionIDS);
	if (munition->iExplosionArchID == 0)
	{
		if (munition->fHullDamage > 0)
		{
			GetStatLine(flags, &len, 459518, f0fmt, 0, munition->fHullDamage);

		}
		if (munition->fHullDamage < 0)
		{
			GetStatLine(flags, &len, 459519, f0fmt, 0, munition->fHullDamage * -1);

		}
		if (munition->fEnergyDamage > 0)
		{
			GetStatLine(flags, &len, 459520, f0fmt, 0, munition->fEnergyDamage);
		}
	}
	if (munition->iExplosionArchID != 0)
	{
		if (explosion->fHullDamage > 0)
		{
			GetStatLine(flags, &len, 459518, f0fmt, 0, explosion->fHullDamage);

		}
		if (explosion->fHullDamage < 0)
		{
			GetStatLine(flags, &len, 459519, f0fmt, 0, explosion->fHullDamage * -1);

		}
		if (explosion->fEnergyDamage > 0)
		{
			GetStatLine(flags, &len, 459520, f0fmt, 0, explosion->fEnergyDamage);
		}
		GetStatLine(flags, &len, 459529, f0fmt, 1759, explosion->fRadius);
		GetStatLine(flags, &len, 459530, f2fmtS, 0, munition->fLifeTime);

		if (munition->fDetonationDist > 0)
		{
			GetStatLine(flags, &len, 459537, f0fmt, 1759, munition->fDetonationDist);
		}
	}
	return len;
}

uint __stdcall StatsHook(int idx, LPBYTE equip, BYTE name, BYTE value)
{
	BYTE flags = name + (value << 1);
	uint len = 0;

	// If the name contains a non-breaking space, display the stats as "n/a".
	if (value)
	{
		GetString(NULL, *(uint*)(equip + 0x14), buf, 128);
		if (wcschr(buf, L'�') != NULL)
			flags |= 4;
	}

	switch (idx)
	{
	case 0:
		return SetPowerCoreInfocard(reinterpret_cast<Archetype::Power*>(equip), flags);
	case 1: // Engine
		return SetEngineInfocard(reinterpret_cast<Archetype::Engine*>(equip), flags);
	case 2: // Scanner
		return SetScannerInfocard(reinterpret_cast<Archetype::Scanner*>(equip), flags);
	case 3: // Tractor
		return SetTractorInfocard(reinterpret_cast<Archetype::Tractor*>(equip), flags);
	case 4: // Armor
		return SetArmorInfocard(reinterpret_cast<Archetype::Armor*>(equip), flags);
	case 5: // ShieldGenerator
		return SetShieldGeneratorInfocard(reinterpret_cast<Archetype::ShieldGenerator*>(equip), flags);
	case 7: // Gun
		return SetGunInfocard(reinterpret_cast<Archetype::Gun*>(equip), flags);
	case 8: // MineDropper
		return SetMineDropperInfocard(reinterpret_cast<Archetype::MineDropper*>(equip), flags);
	case 9: // CounterMeasureDropper
		return SetCounterMeasureDropperInfocard(reinterpret_cast<Archetype::CounterMeasureDropper*>(equip), flags);
	case 12:
		return SetMunitionInfocard(reinterpret_cast<Archetype::Munition*>(equip), flags);
		break;
		return len;
	}
}

__declspec(naked) void StatsHookNaked()
{
	__asm {
		push	dword ptr[esp + 0x22c]
		push	dword ptr[esp + 0x22c]
		push	edi
		push	eax
		call	StatsHook
		push	0
		push	dword ptr[esp + 0x224]
		push	0x4854c0
		ret
	}
}

std::array<DWORD, 15> statsJumpTable = {
	(DWORD)StatsHookNaked,	// Power
	(DWORD)StatsHookNaked,	// Engine
	(DWORD)StatsHookNaked,	// Scanner
	(DWORD)StatsHookNaked,	// Tractor
	(DWORD)StatsHookNaked,	// Armor
	(DWORD)StatsHookNaked,	// ShieldGenerator
	0x484DA9,				// Thruster
	(DWORD)StatsHookNaked,	// Gun 
	(DWORD)StatsHookNaked,	// MineDropper 
	(DWORD)StatsHookNaked,  // CounterMeasureDropper
	0x484C9C,				// RepairKit
	0x484D23,				// ShieldBattery
	(DWORD)StatsHookNaked,	// Munition & Mine 0x484A6B
	0x4851E5,				// CounterMeasure
	0x485495,				// default
};

std::array<byte, 24> statsJumpIndex = {
0,	// Power
1,	// Engine
14, // Shield
5,	// ShieldGenerator
6,	// Thruster
14, // Launcher
7,	// Gun
8,	// MineDropper
9,	// CounterMeasureDropper
2,	// Scanner
14, // Light
3,	// Tractor
14, // AttachedFXEquip
14, // InternalFXEquip
14, // RepairDroid
10, // RepairKit
11, // ShieldBattery
14, // CloakingDevice
14, // TradeLaneEquip
14, // Projectile
12, // Munition
12, // Mine
13, // CounterMeasure
4	// Armor
};

std::array<byte, 24> statsBoolIndex = {
	0, // Power
	0, // Engine
	1, // Shield
	0, // ShieldGenerator
	0, // Thruster
	1, // Launcher
	0, // Gun
	0, // MineDropper
	0, // CounterMeasureDropper
	0, // Scanner
	1, // Light
	0, // Tractor
	1, // AttachedFXEquip
	1, // InternalFXEquip
	1, // RepairDroid
	0, // RepairKit
	0, // ShieldBattery
	1, // CloakingDevice
	1, // TradeLaneEquip
	1, // Projectile
	0, // Munition
	0, // Mine
	0, // CounterMeasure
	0  // Armor
};


bool EndsWith(std::wstring const& value, std::wstring const& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void FreelancerHacks()
{
#define ADDR_STATS1 ((PBYTE)0x47c21f)
#define ADDR_STATS2 ((PBYTE)0x4840c9)
#define ADDR_STATSNL1 ((PBYTE)0x47c355)
#define ADDR_STATSNL2 ((PBYTE)0x47c501)
	// Replace the equipment stats jump table.
	ProtectExecuteReadWrite(ADDR_STATS1, 26);
	ProtectExecuteReadWrite(ADDR_STATS2, 19);
	ADDR_STATS1[2] = ADDR_STATS2[2] = -11;
	ADDR_STATS1[5] = ADDR_STATS2[5] = 23;
	*(DWORD*)(ADDR_STATS2 + 15) = (DWORD)statsJumpIndex.data();
	*(DWORD*)(ADDR_STATS2 + 22) = (DWORD)statsJumpTable.data();
	*(DWORD*)(ADDR_STATS1 + 15) = (DWORD)statsBoolIndex.data();

	// Add a blank line after the equipment's "Stats".
	ProtectExecuteReadWrite(ADDR_STATSNL1, 20);
	ProtectExecuteReadWrite(ADDR_STATSNL2, 20);
	*(DWORD*)(ADDR_STATSNL1 + 1) =
		*(DWORD*)(ADDR_STATSNL1 + 16) =
		*(DWORD*)(ADDR_STATSNL2 + 1) =
		*(DWORD*)(ADDR_STATSNL2 + 16) = 0x5d0a08;
}



//Run these patches for the client only.
void SetupHack()
{
	std::array<wchar_t, MAX_PATH> fileNameBuffer;
	GetModuleFileName(nullptr, fileNameBuffer.data(), MAX_PATH);
	std::wstring filename = fileNameBuffer.data();

	if (EndsWith(filename, L"Freelancer.exe"))
	{
		FreelancerHacks();
	}
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, void* lpReserved)
{
	DisableThreadLibraryCalls(hModule);
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		SetupHack();
	}
	return TRUE;
}