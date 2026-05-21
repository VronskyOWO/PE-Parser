#pragma once
#include "imgui.h"
#include "App.h"
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include "PEFile.h"
#include "PECore.h"
enum ViewType
{
	View_None,
	View_DOS,
	View_NT_Signature,
	View_NT_FileHeader,
	View_NT_OptionalHeader,
	View_Sections,
	View_Import,
	View_Export,
	View_Resource,
	View_BaseRelocale,
	View_BoundImport,
};

extern ImFont* g_HexFont;
struct SelectedResData
{
	DWORD resDataEntryRva = 0;
	DWORD dataRva = 0;
	DWORD dataSize = 0;
	int typeId = -1;
};

class App
{
public:
	App();
	~App();
	void update();
	void SetDarkTheme();
	const char* GetResTypeName(WORD id);
private:
	std::vector<ImportData> importDatas{};
	std::vector<ExportData> exportData{};
	std::vector<std::vector<BaseData>> sectionHeadersData;
	OptionalHeaderData optionalHeaderData{};
	std::vector<NtFileHeaderData> ntFileHeaderData{};
	NtSignatureData ntSignatureData{};
	std::vector<DosHeaderData> dosHeaderData{};
	std::vector<BaseRelocaleEntry> baseRelocaleData{};
	std::vector<BoundImportDataBlock> boundImportData{};
	ResourceNode resourceData{};
	ResourceNode* pSelectedNode{};
	SelectedResData selectedResData{};
	int currentResTypeId = -1;
	int selectedImportIndex = -1;
	int selectedRelocationIndex = -1;
	ViewType currentView= View_None;
	PEFile* currentPE;
	void DrawDOSHeaderView();
	void DrawNtSignatureView();
	void DrawNtFileHeaderView();
	void DrawNtOptionalHeaderView();
	void DrawSectionsView();
	void DrawExportView();
	void DrawImportView();
	void DrawResourceView();
	void DrawResourceNode(ResourceNode& node);
	//void DrawResourceNode(PIMAGE_RESOURCE_DIRECTORY dir,DWORD baseRva,DWORD level);
	void DrawBaseRelocaleView();
	void DrawBoundImport();
	void DrawMenuBar();
	void OpenFile();     // ÉùÃ÷
	void CloseFile();    // ÉùÃ÷
	void DrawPETree();
	void DrawPEView();
	DWORD RvaToFoa(DWORD rva);
};

