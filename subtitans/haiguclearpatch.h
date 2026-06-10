#pragma once
#include "patch.h"

class HaiguClearPatch : public Patch
{
public:
	HaiguClearPatch();
	virtual ~HaiguClearPatch();

	bool Validate() override;
	bool Apply() override;
	const wchar_t *ErrorMessage() override { return L"Failed to apply the HAIGU clear patch"; }

	unsigned long DetourAddress;
};
