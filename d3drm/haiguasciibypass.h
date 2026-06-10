#pragma once
#include <Windows.h>

// HAIGU's glyph-takeover core (HAIGU.dll+0x1300, sub_10001300) replaces every
// character of the captured string - including pure-ASCII digits - whenever the
// engine's glyph source bitmap sits below 0x40000000. On Win9x the engine's
// LCD-style digit glyphs lived in high (>=0x40000000) DirectDraw memory, so the
// check made HAIGU leave them alone; on modern Windows every surface buffer is
// a low heap allocation, so HAIGU wrongly takes over digit-only strings (game
// clock, trade up/down counters) and redraws them color-keyed without clearing
// the cell -> pixel pile-up.
//
// Apply() detours the takeover core's first instruction: if the captured string
// (HAIGU.dll+0x1B228) contains no GBK byte (>0xA0), the per-character takeover
// flag (HAIGU.dll+0x1DAA9) is cleared, HAIGU's own "no takeover" path runs and
// the engine draws the original glyph - exactly as it did on Win98.
//
// This lives in d3drm (not in SubTitans' patch framework) so the fix also works
// when the game runs with HAIGU alone, without subtitans.dll.
namespace HaiguAsciiBypass
{
	// Returns false when the loaded HAIGU.dll does not match the expected build
	// (entry bytes differ); the caller may warn and continue - HAIGU still works,
	// only the digit rendering fix is missing.
	bool Apply(HMODULE haiguModule);
}
