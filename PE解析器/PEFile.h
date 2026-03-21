#pragma once
#include <Windows.h>
#include <vector>
#include <string>

struct PEFile
{
	//文件信息
	std::string filePath;
	PCHAR fileReadBuffer;

	//Header
	PIMAGE_DOS_HEADER pDosHeader = nullptr;
	PIMAGE_NT_HEADERS32 pNtHeader32 = nullptr;
	PIMAGE_NT_HEADERS64 pNtHeader64 = nullptr;

	//Section
	PIMAGE_SECTION_HEADER sectionHeaders = nullptr;
	int sectionCount = 0;

	//DATA DIRECTORY
	PIMAGE_DATA_DIRECTORY importDir{};
	PIMAGE_DATA_DIRECTORY exportDir{};
	PIMAGE_DATA_DIRECTORY resourceDir{};
	PIMAGE_DATA_DIRECTORY relocateDir{};

	bool is64 = false;
};