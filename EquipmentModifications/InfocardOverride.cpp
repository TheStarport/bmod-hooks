#include "Main.hpp"

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

std::wstring& GetShipInfo(Archetype::Ship* ship) {
	static std::wstring ret;
	ret += L"<RDL><PUSH/><PARA/> <JUST loc=\"c\"/><TRA bold=\"true\"/><TEXT>Stats</TEXT><TRA bold=\"false\"/><PARA/><JUST loc=\"l\"/><PARA/>";
	//ret += L"<TEXT>Hardpoints: 2x ME, 2x SE / B, 2x SM, 1x MM, 1xLB</TEXT><PARA/>";
	//ret += L"<TEXT>Additional Equipment: CD, CM, Mine Dropper</TEXT><PARA/>";
	ret += std::format(L"<TEXT>Mass: {:.0f}t</TEXT><PARA/>", ship->fMass);
	ret += std::format(L"<TEXT>Armor: {:.0f}</TEXT> <PARA/>", ship->fHitPoints);
	ret += std::format(L"<TEXT>Cargo Space: {:.0f}t</TEXT><PARA/>", ship->fHoldSize);
	ret += std::format(L"<TEXT>Pitch/Yaw/Roll Rate: {:.1f}°/{:.1f}°/{:.1f}°/s</TEXT><PARA/>", RadiansToDegrees(GetTurnRate(ship->fSteeringTorque[0], ship->fAngularDrag[0], ship->fRotationInertia[0])), RadiansToDegrees(GetTurnRate(ship->fSteeringTorque[1], ship->fAngularDrag[1], ship->fRotationInertia[1])), RadiansToDegrees(GetTurnRate(ship->fSteeringTorque[2], ship->fAngularDrag[2], ship->fRotationInertia[2])));
	ret += std::format(L"<TEXT>Angular Drag: {:.0f}N</TEXT><PARA/>", ship->fAngularDrag[0]);
	ret += std::format(L"<POP/></RDL>");
	return ret;
}

std::wstring& PrintCardTemplate(Archetype::Ship* ship) {
	static std::wstring ret;
	ret += std::format(L"<RDL><PUSH/><TRA bold=\"true\"/><TEXT>Stats</TEXT><TRA bold=\"false\"/><PARA/><PARA/><TEXT>Hardpoints:</TEXT><PARA/>");
	//ret += std::format(L"<TEXT>Additional Equipment:</TEXT><PARA/>");
	//ret += std::format(L"<TEXT>Mass:</TEXT><PARA/>");
	ret += std::format(L"<TEXT>Armor:</TEXT><PARA/>");
	ret += std::format(L"<TEXT>Cargo Space:</TEXT><PARA/>");
	ret += std::format(L"<TEXT>Pitch/Yaw/Roll Rate:</TEXT><PARA/>");
	ret += std::format(L"<TEXT>Angular Drag:</TEXT><PARA/>");
	ret += std::format(L"<POP/></RDL>");
	return ret;
}

std::wstring& PrintCardContents(Archetype::Ship* ship) {
	static std::wstring ret;
	ret += std::format(L"<RDL><PUSH/><PARA/><PARA/>");
	//ret += std::format(L"<TEXT>2x ME, 2x SE/B, 2x SM, 1x MM, 1xLB</TEXT><PARA/>");
	//ret += std::format(L"<TEXT>CD, CM, Mine Dropper</TEXT><PARA/>");
	ret += std::format(L"<TEXT>{:.0f}t</TEXT><PARA/>", ship->fMass);
	ret += std::format(L"<TEXT>{:.0f}</TEXT><PARA/>", ship->fHitPoints);
	ret += std::format(L"<TEXT>{:.0f}t</TEXT><PARA/>", ship->fHoldSize);
	ret += std::format(L"<TEXT>{:.1f}°/{:.1f}°/{:.1f}°/sec</TEXT><PARA/>", RadiansToDegrees(GetTurnRate(ship->fSteeringTorque[0], ship->fAngularDrag[0], ship->fRotationInertia[0])), RadiansToDegrees(GetTurnRate(ship->fSteeringTorque[1], ship->fAngularDrag[1], ship->fRotationInertia[1])), RadiansToDegrees(GetTurnRate(ship->fSteeringTorque[2], ship->fAngularDrag[2], ship->fRotationInertia[2])));
	ret += std::format(L"<TEXT>{:.0f}N</TEXT><PARA/>", ship->fAngularDrag[0]);
	ret += std::format(L"<POP/></RDL>");
	return ret;
}

/// Return the infocard text or 0 if custom infocard does not exist.
static const wchar_t* GetCustomIDS(uint ids_number)
{
	if (ids_number == 459591) {
		return GetShipInfo(Utils::GetShipArch()).c_str();
	}
	/*if (ids_number == 459592) {
		return PrintCardTemplate(Utils::GetShipArch()).c_str();
	}
	if (ids_number == 459593) {
		return PrintCardContents(Utils::GetShipArch()).c_str();
	}*/
	return 0;
}

/// Override ID strings.
static int GetIDSStringResult;
int __stdcall HkCb_GetIDSString(int rsrc, unsigned int ids_number, wchar_t* buf, unsigned int buf_size)
{
	const wchar_t* text = GetCustomIDS(ids_number);
	if (text)
	{
		//AddLog(L"Overriding infocard: %d [%s]\n", ids_number, text);    
		size_t num_chars = wcslen(text);
		wcsncpy(buf, text, buf_size);
		buf[buf_size - 1] = 0;
		return num_chars;
	}
	return 0;
}

/// Naked function hook for the infoname/infocard function.
__declspec(naked) void HkCb_GetIDSStringNaked()
{
	__asm {
		push[esp + 16]
		push[esp + 16]
		push[esp + 16]
		push[esp + 16]
		call HkCb_GetIDSString
		cmp eax, 0
		jnz done

		mov eax, [esp + 8]
		mov ecx, [esp + 4]
		push ebx
		push 0x4347E9
		done:
		ret
	}
}

typedef int(__cdecl* _GetString)(char* rsrc, uint ids, wchar_t* buf, int buf_chars);
extern _GetString GetString = (_GetString)0x4347E0;

typedef int(__cdecl* _FormatXML)(const wchar_t* buf, int buf_chars, RenderDisplayList& rdl, int iDunno);
extern _FormatXML FormatXML = (_FormatXML)0x57DB50;

/// Return true if we override the default infocard
bool __stdcall HkCb_GetInfocard(unsigned int ids_number, RenderDisplayList& rdl)
{
	const wchar_t* text = GetCustomIDS(ids_number);
	if (text)
	{
		// AddLog(L"Overriding infocard: %d [%s]\n", ids_number, text);    
		XMLReader reader;
		size_t num_chars = wcslen(text);
		if (!reader.read_buffer(rdl, (const char*)text, num_chars * 2))
		{
			FormatXML(text, num_chars, rdl, 1);
		}
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

	// Patch get ids string
	{
		BYTE patch2[] = { 0xB9, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE1, 0x90 }; // mov ecx HkCb_GetIDSStringNaked, jmp ecx
		DWORD* iAddr = (DWORD*)((char*)&patch2 + 1);
		*iAddr = reinterpret_cast<DWORD>((void*)HkCb_GetIDSStringNaked);
		Utils::Memory::WriteProcMem((char*)0x4347E0, &patch2, 8);
	}

	// Patch get infocard
	{
		BYTE patch[] = { 0xB9, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE1 }; // mov ecx HkCb_GetInfocardNaked, jmp ecx
		DWORD* iAddr = (DWORD*)((char*)&patch + 1);
		*iAddr = reinterpret_cast<DWORD>((void*)HkCb_GetInfocardNaked);
		Utils::Memory::WriteProcMem((char*)0x057DA40, &patch, 7);
	}
}