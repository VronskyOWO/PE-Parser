#pragma once
#include "PEFile2.h"
#include "ViewData.h"
#include "Tool.h"
class PECore
{
	PEFile2 currentFile;
	HANDLE hCurrentFileHandle;
	HANDLE hCurrentFileMappingObj;
	LPVOID pCurrentAddrOfFileView;
	const MachineType* GetMachineType(WORD machine);
public:
	BOOLEAN OpenFile(LPSTR filePath,_Out_ std::wstring& logInfo);
	std::vector<DosHeaderData> GetDosHeaderData();
	NtSignatureData GetNtSignatureData();
	std::vector <NtFileHeaderData> GetNtFileHeaderData();
	void CloseFile();
	BOOLEAN GetOpenStatus();
	PECore();
	~PECore();
};

