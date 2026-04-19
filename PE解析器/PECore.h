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
	std::vector <ExportData> GetExportData();
	std::vector <ImportData> GetImportData();
	OptionalHeaderData GetNtOptionalHeaderData();
	std::vector<std::vector<BaseData>> GetSectionsTableData();
	std::vector<BaseRelocaleEntry> GetBaseRelocaleData();
	ResourceNode GetResourcesData();
	void ParseResourceNode(
		PIMAGE_RESOURCE_DIRECTORY dir,
		DWORD baseRva,
		int level,
		ResourceNode& node);

	const char* GetResTypeName(WORD id);
	void CloseFile();
	BOOLEAN GetOpenStatus();
	DWORD RvaToFoa(DWORD rva);
	PECore();
	~PECore();
};

