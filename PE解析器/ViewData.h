#pragma once
#include <vector>
#include <string>
#include <Windows.h>
#include <optional>  

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

struct ResourceData
{
	DWORD dataRva{};
	DWORD dataSize{};
	DWORD codePage{};
	DWORD reserved{};

	std::vector<BYTE> rawData; // ⭐ 关键：直接带出原始数据
};

struct ResourceNode
{
	std::string name;   // "MENU" / "资源名: xxx" / "语言ID: 1033"
	WORD id = 0;        // 如果是ID类型
	bool isNamed = false;
	bool isRoot = false;

	int level = 0;      // 1/2/3
	int typeId;
	IMAGE_RESOURCE_DIRECTORY rootInfo;//root才有值
	//子节点
	std::vector<ResourceNode> children;

	//如果是叶子节点，这里才有值
	std::optional<ResourceData> data;
};

