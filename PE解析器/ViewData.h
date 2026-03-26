#pragma once
#include <vector>
#include <string>
#include <Windows.h>

struct BaseData
{
	std::string field{};
	std::string value{};
	std::string descriptor{};
};
struct MachineType
{
	WORD value;
	const char* description;
};
using DosHeaderData = BaseData;
using NtSignatureData = BaseData;
using NtFileHeaderData = BaseData;
