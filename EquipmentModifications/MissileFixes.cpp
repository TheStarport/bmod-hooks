#include "Main.hpp"

std::unordered_set<uint> self_detonating_mines;
std::unordered_map<uint, float> missile_arming_time;
std::unordered_map<uint, float> top_speed_map;
std::unordered_map<uint, float> mine_arming_time;
std::unordered_map<uint, uint> gun_barrel_counts;

float GetRangeAtSpeed(const Archetype::MotorData* motor, Archetype::Munition* munition, float muzzleVelocity, float shipVelocity)
{
	auto topSpeedIter = top_speed_map.find(munition->iArchID);

	float startSpeed = muzzleVelocity + shipVelocity;
	float distance = munition->fLifeTime * startSpeed;

	if (motor && motor->fAccel > 0.0)
	{
		float motorLifetime = std::min(motor->fLifetime, munition->fLifeTime - motor->fDelay);
		if (topSpeedIter != top_speed_map.end())
		{
			startSpeed = std::min(startSpeed, topSpeedIter->second);
			motorLifetime = std::min(motorLifetime, std::max(0.0f, topSpeedIter->second - startSpeed) / motor->fAccel);
		}

		float motorAcceleration = motor->fAccel * motorLifetime;

		distance += motorAcceleration * motorLifetime * 0.5f;

		float timeMaxSpeed = std::max(0.0f, munition->fLifeTime - motor->fDelay - motorLifetime);
		distance += motorAcceleration * timeMaxSpeed;
	}

	return distance;
}

uint DumbMissileJumpMissile = 0x62B1595;
uint DumbMissileJumpGun = 0x62B15B5;
__declspec(naked) void DumbMissileFix()
{
	__asm
	{
		mov ebx, 2
		cmp[eax + 0x90], ebx
		mov ecx, edi
		jz jumpMissile
		xor ebx, ebx
		jmp[DumbMissileJumpGun]
		jumpMissile:
		xor ebx, ebx
			jmp[DumbMissileJumpMissile]
	}
}

float __fastcall MunitionRangeDetour(Archetype::Gun* gun)
{
	if (gun->isRangeCalculated)
	{
		return gun->range;
	}
	Archetype::Munition* projArch = reinterpret_cast<Archetype::Munition*>(Archetype::GetEquipment(gun->iProjectileArchID));
	if (!projArch->iMotorID)
	{
		gun->isRangeCalculated = true;
		gun->range = gun->fMuzzleVelocity * projArch->fLifeTime;
		if (projArch->iSeeker == 1) // dumbfire
		{
			ID_String idString;
			idString.id = projArch->iExplosionArchID;
			Archetype::Explosion* explosion = Archetype::GetExplosion(idString);
			gun->range += explosion->fRadius;
		}
		return gun->range;
	}

	const Archetype::MotorData* motor = Archetype::GetMotor(projArch->iMotorID);
	gun->isRangeCalculated = true;
	gun->range = GetRangeAtSpeed(motor, projArch, gun->fMuzzleVelocity, 0.0f);
	return gun->range;
}

CELauncher::FireResult __fastcall CanFireDetour(CEGun* gun, void* edx, Vector& firePos)
{
	using CanFireOriginal = CELauncher::FireResult(__thiscall*)(CEGun*, Vector& firePos);
	CanFireOriginal CanFireOriginalFunc = CanFireOriginal(0x62976F0);

	CELauncher::FireResult fireresult = CanFireOriginalFunc(gun, firePos);
	if (!gun->owner->ownerPlayer
		|| fireresult != CELauncher::FireResult::Success)
	{
		return fireresult;
	}

	if (!gun->MunitionArch()->iSeeker)
	{
		return fireresult;
	}

	auto armingTimeIter = missile_arming_time.find(gun->projArch->iArchID);
	if (armingTimeIter == missile_arming_time.end())
	{
		return fireresult;
	}

	CShip* ownerShip = (CShip*)gun->owner;
	IObjRW* target = ownerShip->get_target();
	if (!target)
	{
		return fireresult;
	}

	const CObject* targetCObj = target->cobject();

	Vector positionDiff = { ownerShip->vPosition.x - targetCObj->vPosition.x,ownerShip->vPosition.y - targetCObj->vPosition.y, ownerShip->vPosition.z - targetCObj->vPosition.z };
	Vector momentumShip = ownerShip->get_velocity();
	Vector momentumTarget = targetCObj->get_velocity();
	Vector momentumDiff = { momentumShip.x - momentumTarget.x,momentumShip.y - momentumTarget.y, momentumShip.z - momentumTarget.z };

	float timeToTarget;
	float muzzleVelocity = gun->LauncherArch()->fMuzzleVelocity;

	if (!gun->ComputeTimeToTgt(positionDiff, momentumDiff, muzzleVelocity, timeToTarget))
	{
		return fireresult;
	}

	PhySys::RayHit rayArray[20];

	int hitCount = PhySys::FindRayCollisions(ownerShip->system, ownerShip->vPosition, targetCObj->vPosition, rayArray, 20);

	for (int i = 0; i < hitCount; i++)
	{
		CSimple* simple = reinterpret_cast<CSimple*>(rayArray[i].cobj);
		if (simple != targetCObj)
		{
			continue;
		}

		float distanceStandard = Utils::Distance3D(ownerShip->vPosition, targetCObj->vPosition);
		float distanceHitRay = Utils::Distance3D(ownerShip->vPosition, rayArray[i].position);

		timeToTarget -= (distanceStandard - distanceHitRay) / muzzleVelocity;
		break;
	}

	if (timeToTarget <= armingTimeIter->second)
	{
		return CELauncher::FireResult::FailureCruiseActive;
	}

	return fireresult;
}

void InitMissileFixes()
{
	auto hCommon = GetModuleHandleA("common");
	{
		PBYTE jumpDummy = PBYTE(malloc(5));
		Utils::Memory::DetourInit((uchar*)hCommon + 0x5158B, DumbMissileFix, jumpDummy);
	}

	{
		PBYTE jumpDummy = PBYTE(malloc(5));
		Utils::Memory::DetourInit((uchar*)hCommon + 0x96910, MunitionRangeDetour, jumpDummy);
	}

	{
		uint CanFireDetourAddr = (uint)CanFireDetour;
		Utils::Memory::WriteProcMem((char*)hCommon + 0x13D1E8, &CanFireDetourAddr, 4);
	}
}