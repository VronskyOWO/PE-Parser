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
public:
	BOOLEAN OpenFile(LPSTR filePath,_Out_ std::wstring& logInfo);
	std::vector<DosHeaderData> GetDosHeaderData();
	NtSignatureData GetNtSignatureData();
	void CloseFile();
	BOOLEAN GetOpenStatus();
	PECore();
	~PECore();
};

