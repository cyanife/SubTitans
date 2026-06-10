#include <Windows.h>
#include "../shared/detour.h"
#include "haiguasciibypass.h"

namespace HaiguAsciiBypass
{
	// HAIGU.dll module RVAs; identical in every known HAIGU build (the original
	// localization DLL and derivatives differ only in 20 bytes of st.exe hook
	// addresses, none of which touch these regions). Apply() verifies the entry
	// bytes before patching anyway.
	constexpr unsigned long TakeoverCoreRva = 0x1300;  // sub_10001300 entry
	constexpr unsigned long GbkStringRva = 0x1B228;	   // captured string buffer (byte_1001B228)
	constexpr unsigned long TakeoverFlagRva = 0x1DAA9; // per-char takeover flag (byte_1001DAA9)

	// Original entry instruction: mov al, [flag] == A0 xx xx xx xx (5 bytes)
	constexpr unsigned long DetourSize = 5;

	static unsigned long JmpBackAddress = 0;
	static unsigned long GbkStringAddress = 0;
	static unsigned long TakeoverFlagAddress = 0;

	__declspec(naked) static void Implementation()
	{
		__asm {
			pushfd
			push esi
			mov esi, GbkStringAddress
		scan_next:
			mov al, byte ptr [esi]
			test al, al
			jz pure_ascii
			cmp al, 0xA0
			ja replay ; GBK byte found -> let HAIGU take over as usual
			inc esi
			jmp scan_next
		pure_ascii:
			mov esi, TakeoverFlagAddress
			mov byte ptr [esi], 0 ; force HAIGU's own "no takeover" path
		replay:
			; replay the displaced original instruction: mov al, [takeover flag]
			mov esi, TakeoverFlagAddress
			mov al, byte ptr [esi]
			pop esi
			popfd
			jmp [JmpBackAddress]
		}
	}

	bool Apply(HMODULE haiguModule)
	{
		if (!haiguModule)
			return false;

		const unsigned long base = (unsigned long)haiguModule;
		const unsigned long detourAddress = base + TakeoverCoreRva;
		GbkStringAddress = base + GbkStringRva;
		TakeoverFlagAddress = base + TakeoverFlagRva;
		JmpBackAddress = detourAddress + DetourSize;

		const unsigned char *entry = (const unsigned char *)detourAddress;
		if (entry[0] == 0xE9)
			return true; // already detoured (e.g. by an older HAIGU-aware subtitans.dll build)
		if (entry[0] != 0xA0 || *(const unsigned long *)(entry + 1) != TakeoverFlagAddress)
			return false; // unexpected HAIGU build

		return Detour::Create(detourAddress, DetourSize, (unsigned long)Implementation);
	}
}
