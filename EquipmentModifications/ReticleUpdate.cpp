#include "Main.hpp"

int __cdecl checkReticleRender(IObjInspect* a1, IObjInspectImpl* a2)
{
	if (a1 == 0 || a2 == 0)
	{
		return 0;
	}
	float attitude;
	if (a2->get_attitude_towards(attitude, a1))
	{
		return 0;
	}

	CShip* playerCShip = Utils::GetCShip();
	if (playerCShip->playerGroup)
	{
		const CShip* cship = dynamic_cast<const CShip*>(a2->cobject());
		if (cship && playerCShip->playerGroup == cship->playerGroup)
		{
			return 0;
		}
	}
	return -1;
}

void __stdcall ReticleCheck(const CShip* ship, const CSimple* target, float& perceivedWeaponRange)
{
	if (ship != Utils::GetCShip())
	{
		return;
	}

	if (target->objectClass == CObject::CGUIDED_OBJECT)
	{
		const CGuided* cguided = reinterpret_cast<const CGuided*>(target);
		const Archetype::Munition* munition = reinterpret_cast<const Archetype::Munition*>(cguided->projarch());
		if (munition->bCruiseDisruptor)
		{
			perceivedWeaponRange = 0;
		}
		return;
	}

	if ((target->objectClass & CObject::CEQOBJ_MASK) != CObject::CEQOBJ_MASK)
	{
		return;
	}

	IObjRW* playertarget = ship->get_target();
	if (!playertarget || playertarget->cobject() != target)
	{
		return;
	}

	Vector targetPosition = target->vPosition;

	if (ship->subTargetId)
	{
		const CEqObj* ceqobj = reinterpret_cast<const CEqObj*>(target);
		if (ship->subTargetId < 33)
		{
			const CArchGroup* colGrp = ceqobj->archGroupManager.FindByID(ship->subTargetId);
			if (colGrp)
			{
				colGrp->GetCenterOfMass(targetPosition);
			}
		}
		else
		{
			const CExternalEquip* equip = reinterpret_cast<const CExternalEquip*>(ceqobj->equip_manager.FindByID(ship->subTargetId));
			if (equip)
			{
				equip->GetCenterOfMass(targetPosition);
			}
		}
	}

	PhySys::RayHit rayArray[20];

	int hitCount = PhySys::FindRayCollisions(ship->system, ship->vPosition, targetPosition, rayArray, 20);

	for (int i = 0; i < hitCount; i++)
	{
		CSimple* simple = reinterpret_cast<CSimple*>(rayArray[i].cobj);
		if (simple != target)
		{
			if (simple->objectClass == CObject::CEQUIPMENT_OBJECT)
			{
				perceivedWeaponRange = std::max(ship->gunStats.maxActiveMissileRange, ship->gunStats.maxActiveGunRange) + simple->fRadius;
				perceivedWeaponRange *= perceivedWeaponRange;
			}
			continue;
		}

		float distanceHitRay = Utils::Distance3D(ship->vPosition, rayArray[i].position);
		float distanceOriginal = Utils::Distance3D(ship->vPosition, targetPosition);
		float distanceOverride = distanceOriginal - distanceHitRay + std::max(ship->gunStats.maxActiveMissileRange, ship->gunStats.maxActiveGunRange);
		distanceOverride *= distanceOverride;
		perceivedWeaponRange = distanceOverride;

		break;
	}
}

struct CliGun
{
	uint vftable;
	CELauncher* launcher;
	IObjRW* owner;
};

typedef char(__thiscall* PlayerFire)(void* unk, Vector& firePosition);
char __fastcall PlayerFireDetour(CliGun* launcher, void* edx, Vector& firePosition)
{
	static const PlayerFire playerFireFunc = (PlayerFire)(0x52DD00);

	if (abs(firePosition.x) > 300000.f
		|| abs(firePosition.y) > 300000.f
		|| abs(firePosition.z) > 300000.f)
	{
		float magnitude = sqrtf(firePosition.x * firePosition.x
			+ firePosition.y * firePosition.y
			+ firePosition.z * firePosition.z);

		firePosition.x /= magnitude;
		firePosition.y /= magnitude;
		firePosition.z /= magnitude;

		firePosition.x *= 4000.f;
		firePosition.y *= 4000.f;
		firePosition.z *= 4000.f;

		Vector& playerPos = Utils::GetCShip()->vPosition;

		firePosition.x += playerPos.x;
		firePosition.y += playerPos.y;
		firePosition.z += playerPos.z;
	}
	else
	{
		CShip* cship = Utils::GetCShip();
		IObjInspectImpl* target = reinterpret_cast<IObjInspectImpl*>(cship->get_target());
		if (target)
		{
			Vector& playerPos = cship->vPosition;

			Vector targetPos;

			bool success = cship->get_tgt_lead_fire_pos(reinterpret_cast<IObjInspect*>(target), targetPos);

			if (success)
			{
				float distance = Utils::Distance3D(playerPos, targetPos);
				distance = std::min(distance, cship->gunStats.maxActiveGunRange);

				if (distance != cship->gunStats.maxActiveGunRange)
				{
					Vector firePositionOffsetPos;
					firePositionOffsetPos.x = firePosition.x - playerPos.x;
					firePositionOffsetPos.y = firePosition.y - playerPos.y;
					firePositionOffsetPos.z = firePosition.z - playerPos.z;

					float magnitude = sqrtf(firePositionOffsetPos.x * firePositionOffsetPos.x
						+ firePositionOffsetPos.y * firePositionOffsetPos.y
						+ firePositionOffsetPos.z * firePositionOffsetPos.z);

					firePositionOffsetPos.x /= magnitude;
					firePositionOffsetPos.y /= magnitude;
					firePositionOffsetPos.z /= magnitude;

					firePositionOffsetPos.x *= distance;
					firePositionOffsetPos.y *= distance;
					firePositionOffsetPos.z *= distance;

					firePosition.x = playerPos.x + firePositionOffsetPos.x;
					firePosition.y = playerPos.y + firePositionOffsetPos.y;
					firePosition.z = playerPos.z + firePositionOffsetPos.z;
				}
			}
		}
	}
	return playerFireFunc(launcher, firePosition);
}

uint retAddress = 0x62B140F;
__declspec(naked) void ReticleRangeHitRay()
{
	__asm {
		mov eax, esp
		add eax, 0x10
		push eax
		mov eax, [ebp + 0x10]
		push eax
		push esi
		call ReticleCheck
		fld[ebx]
		pop edi
		fsub[esi + 0x2C]
		jmp[retAddress]
	}
}

void InitReticle()
{
	auto hFreelancer = GetModuleHandleA(nullptr);
	PBYTE dummy = PBYTE(malloc(5));
	Utils::Memory::PatchCallAddr((char*)hFreelancer, 0xEC10E, (char*)checkReticleRender);
	Utils::Memory::DetourInit((uchar*)GetModuleHandleA("common") + 0x51409, ReticleRangeHitRay, dummy);

	BYTE patch[] = { 0x00, 0x00, 0x00, 0x00 };
	DWORD* iAddr = (DWORD*)((char*)&patch);
	*iAddr = reinterpret_cast<DWORD>((void*)PlayerFireDetour);
	Utils::Memory::WriteProcMem((char*)hFreelancer + 0x1DD5E4, patch, 4);

	static BYTE snapPatchedMemory[] = { 0x90, 0xE9 };
	static char* snapPatchAddr = (char*)hFreelancer + 0xF2493;
	Utils::Memory::WriteProcMem(snapPatchAddr, snapPatchedMemory, sizeof(snapPatchedMemory));
}