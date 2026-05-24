#pragma once
#include "imgui.h"
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include <functional>   // 为 std::function
#include <algorithm>    // 为 std::min
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

// Forward declarations for D3D11 types to avoid requiring d3d11.h in this header
struct ID3D11Device;
struct ID3D11DeviceContext;

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
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
	ResourceNode* prevSelectedNode{}; // track previous selection to refresh icon
	SelectedResData selectedResData{};
	ImTextureID iconTexture{};
	int iconTexW = 0; // persistent icon width
	int iconTexH = 0; // persistent icon height
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
	void OpenFile();     // 声明
	void CloseFile();    // 声明
	void DrawPETree();
	void DrawPEView();
	DWORD RvaToFoa(DWORD rva);
};

