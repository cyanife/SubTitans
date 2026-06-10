#include "subtitans.h"
#include "haiguclearpatch.h"

namespace HaiguClearDetour
{
	constexpr unsigned long DetourSize = 6;
	static unsigned long JmpFromAddress = 0;
	static unsigned long JmpBackAddress = 0;

	__declspec(naked) void Implementation()
	{
		__asm pushfd;
		__asm pushad;
		__asm add dword ptr[esp + 0x48], 64;
		__asm popad;
		__asm popfd;
		__asm push ebp;
		__asm mov ebp, esp;
		__asm sub esp, 0x38;
		__asm jmp[JmpBackAddress];
	}
}

HaiguClearPatch::HaiguClearPatch()
{
	GetLogger()->Informational("Constructing %s\n", __func__);

	DetourAddress = 0;
}

HaiguClearPatch::~HaiguClearPatch()
{
	GetLogger()->Informational("Destructing %s\n", __func__);
}

bool HaiguClearPatch::Validate()
{
	return DetourAddress != 0;
}

bool HaiguClearPatch::Apply()
{
	GetLogger()->Informational("%s\n", __FUNCTION__);

	HaiguClearDetour::JmpFromAddress = DetourAddress;
	HaiguClearDetour::JmpBackAddress = HaiguClearDetour::JmpFromAddress + HaiguClearDetour::DetourSize;
	if (!Detour::Create(HaiguClearDetour::JmpFromAddress, HaiguClearDetour::DetourSize, (unsigned long)HaiguClearDetour::Implementation))
		return false;

	return true;
}
