#include <Windows.h>
#include "../shared/gameversion.h"
#include "haiguasciibypass.h"
#include "injector.h"

namespace Injector
{
	static unsigned long GameVersion = 0;
	static bool UseSubTitans = false;

	static HMODULE HaiguLibrary = nullptr;
	static HMODULE SubTitansLibrary = nullptr;
	typedef void(__stdcall *SubTitansLibrary_InitializeLibrary)(unsigned long);
	typedef void(__stdcall *SubTitansLibrary_ReleaseLibrary)();

	void __stdcall LoadModule()
	{
		// (0) If SubTitans.dll is present, load it now so we can inspect its exports before
		//     deciding whether HAIGU is safe to load. This only maps the module and runs its
		//     DllMain (a no-op); all of SubTitans' patching happens later in InitializeLibrary,
		//     which we still call last, so HAIGU's hooks are installed before SubTitans' Apply()
		//     overwrites hook #4.
		//
		//     Our HAIGU-aware build exports STCN_HaiguIntegrated; a stock / third-party
		//     SubTitans.dll does not. A stock build's DirectDraw-replacement detour overwrites
		//     HAIGU hook #4 but never feeds HAIGU the palette pointer (dword_10017704) and leaves
		//     Device::EnumSurfaces / Surface::GetPallete unimplemented, so HAIGU + a stock
		//     SubTitans.dll crashes. When we detect that combination we skip HAIGU and let
		//     SubTitans run in English instead.
		bool subTitansIsHaiguAware = false;
		if (UseSubTitans)
		{
			SubTitansLibrary = LoadLibrary(L"subtitans.dll");
			if (!SubTitansLibrary)
			{
				MessageBox(NULL, L"Failed to load subtitans.dll!", L"D3DRM (Custom)", MB_ICONERROR);
				ExitProcess(-1);
			}

			subTitansIsHaiguAware = (GetProcAddress(SubTitansLibrary, "STCN_HaiguIntegrated") != nullptr);
		}

		// (1) Chinese localization core. HAIGU's MFC InitInstance installs its 4 inline hooks
		//     and loads the bitmap fonts. It must be loaded here (at the StartApplication
		//     detour), NOT in d3drm's DllMain: HAIGU is an MFC DLL and would deadlock if
		//     loaded under the DllMain loader lock.
		//
		//     HAIGU's hooks use hard-coded absolute addresses matching the retail/GOG 1.1 code
		//     layout (image base 0x400000). On any other build (Steam 1.0, the demos, or an
		//     ASLR/DEP-rebased 1.1) those addresses would land on the wrong code and crash, so
		//     we warn the user and skip localization instead of loading HAIGU. The localization
		//     patch bundles the v1.1 upgrade, so a correctly-patched install always passes this.
		if (GameVersion == Shared::ST_GAMEVERSION_1_1_0 || GameVersion == Shared::ST_GAMEVERSION_1_1_0_GOG)
		{
			if (UseSubTitans && !subTitansIsHaiguAware)
			{
				// A stock / third-party SubTitans.dll is present. It is incompatible with the
				// Chinese localization (HAIGU): loading both crashes. Skip HAIGU and let SubTitans
				// run in English. "A non-localization-aware SubTitans.dll was detected. It is
				//  incompatible with the Chinese localization (HAIGU), so the localization will not
				//  be loaded this time and the game runs in English. To use the localization,
				//  replace subtitans.dll with the one bundled in the patch, or delete subtitans.dll
				//  and restart the game."
				MessageBox(NULL,
						   L"检测到原版 SubTitans（subtitans.dll）。\n"
						   L"它与汉化版深海争霸不兼容，游戏无法运行。\n\n"
						   L"如需中文化，请将 subtitans.dll 替换为支持汉化的修改版，或删除 subtitans.dll 后重新启动游戏。",
						   L"深海争霸 中文化", MB_ICONWARNING);
				ExitProcess(-1);
			}
			else
			{
				HaiguLibrary = LoadLibrary(L"HAIGU.dll");
				if (!HaiguLibrary)
				{
					MessageBox(NULL, L"Failed to load HAIGU.dll!", L"D3DRM (Custom)", MB_ICONERROR);
					ExitProcess(-1);
				}

				// (1b) Win9x-era compatibility fix, applied directly to the loaded HAIGU
				//     module (see haiguasciibypass.h). Lives here rather than in SubTitans'
				//     patch framework so it also covers running HAIGU without subtitans.dll.
				//     Non-fatal on failure: HAIGU still works, only digits may glitch.
				if (!HaiguAsciiBypass::Apply(HaiguLibrary))
				{
					// "Failed to install the HAIGU digit-rendering compatibility fix
					//  (unexpected HAIGU.dll build). The game will run, but numbers such as
					//  the clock and trade quantities may render incorrectly."
					MessageBox(NULL,
							   L"HAIGU 数字显示兼容补丁安装失败（HAIGU.dll 版本不符）。\n"
							   L"游戏仍可运行，但时间、交易数量等数字可能显示异常。",
							   L"深海争霸 中文化", MB_ICONWARNING);
				}
			}
		}
		else
		{
			// "Submarine Titans Chinese localization requires v1.1. The current game is not v1.1,
			//  so the localization will not be loaded. Please install the v1.1 upgrade bundled with
			//  the localization patch first, then restart the game."
			MessageBox(NULL,
					   L"深海争霸 中文汉化需要 v1.1 版本。\n"
					   L"当前游戏不是 v1.1，中文化将不会加载。\n\n"
					   L"请重新安装汉化补丁以升级到 v1.1版本",
					   L"深海争霸 中文化", MB_ICONWARNING);
		}

		// (2) High-resolution rendering: drive SubTitans when it is present (loaded in step 0).
		//     For our HAIGU-aware build, Apply() installs the DirectDraw replacement detour over
		//     0x6B9981, intentionally overwriting HAIGU hook #4 (loaded first above) and disabling
		//     it; the palette pointer HAIGU needs (dword_10017704) is then supplied by SubTitans'
		//     fake DirectDrawCreate. For a stock build (HAIGU skipped above) this just runs
		//     SubTitans normally in English.
		if (!UseSubTitans)
			return;

		SubTitansLibrary_InitializeLibrary initializeLibrary = (SubTitansLibrary_InitializeLibrary)GetProcAddress(SubTitansLibrary, "InitializeLibrary");
		if (!initializeLibrary)
		{
			MessageBox(NULL, L"Failed to retrieve InitializeLibrary from subtitans.dll!", L"D3DRM (Custom)", MB_ICONERROR);
			ExitProcess(-1);
		}

		initializeLibrary(GameVersion);
	}

	constexpr unsigned long LoadModule_DetourSize = 5;
	static unsigned long LoadModule_JmpFrom = 0;
	static unsigned long LoadModule_JmpBack = 0;
	static unsigned long StartApplicationAddress = 0;
	__declspec(naked) void LoadModule_Detour()
	{
		__asm pushad;
		__asm pushfd;

		__asm call[LoadModule];

		__asm popfd;
		__asm popad;

		__asm call[StartApplicationAddress];

		__asm jmp[LoadModule_JmpBack];
	}

	void __stdcall UnloadModule()
	{
		if (!SubTitansLibrary)
		{
			MessageBox(NULL, L"Failed to unload subtitans.dll!", L"D3DRM (Custom)", MB_ICONERROR);
			ExitProcess(-1);
		}

		SubTitansLibrary_ReleaseLibrary releaseLibrary = (SubTitansLibrary_ReleaseLibrary)GetProcAddress(SubTitansLibrary, "ReleaseLibrary");
		if (!releaseLibrary)
		{
			MessageBox(NULL, L"Failed to retrieve ReleaseLibrary from subtitans.dll!", L"D3DRM (Custom)", MB_ICONERROR);
			ExitProcess(-1);
		}

		releaseLibrary();
		FreeLibrary(SubTitansLibrary);
	}

	constexpr unsigned long UnloadModule_DetourSize = 6;
	static unsigned long UnloadModule_JmpFrom = 0;
	static unsigned long UnloadModule_JmpBack = 0;
	__declspec(naked) void UnloadModule_Detour()
	{
		__asm pushad;
		__asm pushfd;

		__asm call[UnloadModule];

		__asm popfd;
		__asm popad;

		__asm mov dword ptr ss : [ebp - 0x60], eax;
		__asm mov eax, dword ptr ss : [ebp - 0x60];

		__asm jmp[UnloadModule_JmpBack];
	}

	// Only Kernel32 dependent
	bool ApplyDetour(unsigned long origin, unsigned long length, unsigned long destination)
	{
		HANDLE currentProcess = GetCurrentProcess();
		unsigned long *originPointer = (unsigned long *)origin;

		unsigned long previousProtection = 0;
		if (VirtualProtectEx(currentProcess, originPointer, length, PAGE_EXECUTE_READWRITE, &previousProtection) == FALSE)
			return false;

		// Hard length limit of 6...
		unsigned char bytesToCopy[] = {0xE9, 0x90, 0x90, 0x90, 0x90, 0x90};
		if (sizeof(bytesToCopy) < length)
			return false;

		unsigned long jumpDistance = destination - origin - 5;

		for (int i = 0; i < 4; ++i)
		{
			unsigned char *destByte = (unsigned char *)&jumpDistance;
			bytesToCopy[i + 1] = *(destByte + i);
		}

		SIZE_T writtenBytes = 0;
		if (WriteProcessMemory(currentProcess, originPointer, bytesToCopy, length, &writtenBytes) == FALSE)
			return false;

		FlushInstructionCache(currentProcess, originPointer, length);

		if (VirtualProtectEx(currentProcess, originPointer, length, previousProtection, &previousProtection) == FALSE)
			return false;

		return true;
	}

	bool Apply(unsigned long gameVersion, bool useSubTitans)
	{
		GameVersion = gameVersion;
		UseSubTitans = useSubTitans;
		switch (GameVersion)
		{
		case Shared::ST_GAMEVERSION_0_0_6:
		{
			LoadModule_JmpFrom = 0x005CE3CE;
			LoadModule_JmpBack = LoadModule_JmpFrom + LoadModule_DetourSize;

			UnloadModule_JmpFrom = 0x005CE3D3;
			UnloadModule_JmpBack = UnloadModule_JmpFrom + UnloadModule_DetourSize;

			StartApplicationAddress = 0x00401ACD;
		}
		break;
		case Shared::ST_GAMEVERSION_0_1_6:
		{
			LoadModule_JmpFrom = 0x0071A25E;
			LoadModule_JmpBack = LoadModule_JmpFrom + LoadModule_DetourSize;

			UnloadModule_JmpFrom = 0x0071A263;
			UnloadModule_JmpBack = UnloadModule_JmpFrom + UnloadModule_DetourSize;

			StartApplicationAddress = 0x00401FF0;
		}
		break;
		case Shared::ST_GAMEVERSION_1_0_0:
		{
			LoadModule_JmpFrom = 0x00734B6E;
			LoadModule_JmpBack = LoadModule_JmpFrom + LoadModule_DetourSize;

			UnloadModule_JmpFrom = 0x00734B73;
			UnloadModule_JmpBack = UnloadModule_JmpFrom + UnloadModule_DetourSize;

			StartApplicationAddress = 0x00401FEB;
		}
		break;
		case Shared::ST_GAMEVERSION_1_1_0:
		case Shared::ST_GAMEVERSION_1_1_0_GOG:
		{
			LoadModule_JmpFrom = 0x007337FE;
			LoadModule_JmpBack = LoadModule_JmpFrom + LoadModule_DetourSize;

			UnloadModule_JmpFrom = 0x00733803;
			UnloadModule_JmpBack = UnloadModule_JmpFrom + UnloadModule_DetourSize;

			StartApplicationAddress = 0x00401FF5;
		}
		break;
		case Shared::ST_GAMEVERSION_1_1_0_ASLR_DEP:
		case Shared::ST_GAMEVERSION_1_1_0_GOG_ASLR_DEP:
		{
			const unsigned int originalBaseAddress = Shared::IMAGE_BASE;
			const unsigned int baseAddress = (unsigned int)GetModuleHandle(NULL);

			LoadModule_JmpFrom = 0x007337FE - originalBaseAddress + baseAddress;
			LoadModule_JmpBack = LoadModule_JmpFrom + LoadModule_DetourSize;

			UnloadModule_JmpFrom = 0x00733803 - originalBaseAddress + baseAddress;
			UnloadModule_JmpBack = UnloadModule_JmpFrom + UnloadModule_DetourSize;

			StartApplicationAddress = 0x00401FF5 - originalBaseAddress + baseAddress;
		}
		break;
		default:
			return false;
		}

		if (!ApplyDetour(LoadModule_JmpFrom, LoadModule_DetourSize, (unsigned long)LoadModule_Detour))
			return false;

		// UnloadModule's detour only matters when SubTitans is loaded (it tears SubTitans
		// down); in localization-only mode there is nothing to unload, so gate it.
		if (UseSubTitans && !ApplyDetour(UnloadModule_JmpFrom, UnloadModule_DetourSize, (unsigned long)UnloadModule_Detour))
			return false;

		return true;
	}
}
