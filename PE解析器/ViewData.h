#pragma once
#include <vector>
#include <string>
#include <Windows.h>

struct BaseData
{
	std::string field{};
	std::string value{};
	std::string description{};
};
struct MachineType
{
	WORD value;
	const char* description;
};
using DosHeaderData = BaseData;
using NtSignatureData = BaseData;
using NtFileHeaderData = BaseData;

struct DataDirectoryEntryData
{
	std::string virtualAddress{};
	std::string size{};
	std::string description{};
};

struct OptionalHeaderData
{
	std::vector<BaseData> baseField{};
	std::vector<DataDirectoryEntryData> DataDirectory{};
};

