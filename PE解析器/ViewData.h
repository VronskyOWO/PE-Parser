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

struct ExportData
{
	std::string number{};
	std::string funcName{};
	std::string rva{};
};

struct DllInfo
{
	std::string dllName{};
	std::string originalFirstThunk{};
	std::string timeDateStamp{};
	std::string forwarderChain{};
	std::string firstThunk{};
	
};
struct FuncInfo
{
	std::string ordinal{};
	std::string funcName{};
};
struct ImportData
{
	DllInfo dllInfo{};
	std::vector<FuncInfo> funcsInfo{};
};
