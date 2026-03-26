#include "PECore.h"
BOOLEAN PECore::OpenFile(LPSTR filePath,_Out_ std::wstring& logInfo)
{
	CloseFile();

	//file mapping
	hCurrentFileHandle=CreateFileA(filePath, GENERIC_READ| GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hCurrentFileHandle == INVALID_HANDLE_VALUE)
	{	
		DWORD errorCode = GetLastError();
		CloseFile();
		logInfo = { L"CreateFile failed" };
		return FALSE;
	}
	SECURITY_ATTRIBUTES sa = {0};
	hCurrentFileMappingObj =CreateFileMappingA(hCurrentFileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!hCurrentFileMappingObj)
	{
		DWORD errorCode = GetLastError();
		CloseFile();
		logInfo = { L"CreateFileMapping failed" };
		return FALSE;
	}
	pCurrentAddrOfFileView =MapViewOfFile(hCurrentFileMappingObj, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!pCurrentAddrOfFileView)
	{
		DWORD errorCode = GetLastError();
		CloseFile();
		logInfo = { L"MapViewOfFile failed" };
		return FALSE;
	}
	currentFile.filePath = { filePath };

	if (*(PWORD)pCurrentAddrOfFileView != IMAGE_DOS_SIGNATURE || ((PIMAGE_NT_HEADERS)((PCHAR)pCurrentAddrOfFileView + ((PIMAGE_DOS_HEADER)pCurrentAddrOfFileView)->e_lfanew))->Signature != IMAGE_NT_SIGNATURE)
	{
		DWORD errorCode = GetLastError();
		logInfo = { L"非标准PE文件" };
		CloseFile();
		return FALSE;
	}
	currentFile.pDosHeader = (PIMAGE_DOS_HEADER)pCurrentAddrOfFileView;
	PIMAGE_NT_HEADERS pNTHeader = PIMAGE_NT_HEADERS((PCHAR)currentFile.pDosHeader + currentFile.pDosHeader->e_lfanew);
	if (pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		//64位PE
		currentFile.is64 = true;
		currentFile.pNtHeader64 = PIMAGE_NT_HEADERS64((PCHAR)currentFile.pDosHeader + currentFile.pDosHeader->e_lfanew);
		currentFile.sectionHeaders = PIMAGE_SECTION_HEADER((PCHAR)&currentFile.pNtHeader64->OptionalHeader + currentFile.pNtHeader64->FileHeader.SizeOfOptionalHeader);
		currentFile.sectionCount = currentFile.pNtHeader64->FileHeader.NumberOfSections;
		currentFile.exportDir = &currentFile.pNtHeader64->OptionalHeader.DataDirectory[0];
		currentFile.importDir = &currentFile.pNtHeader64->OptionalHeader.DataDirectory[1];
		currentFile.resourceDir = &currentFile.pNtHeader64->OptionalHeader.DataDirectory[2];
		currentFile.relocaleDir = &currentFile.pNtHeader64->OptionalHeader.DataDirectory[5];
	}
	else if (pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		currentFile.is64 = false;
		currentFile.pNtHeader32 = PIMAGE_NT_HEADERS32((PCHAR)currentFile.pDosHeader + currentFile.pDosHeader->e_lfanew);
		currentFile.sectionHeaders = PIMAGE_SECTION_HEADER((PCHAR)&currentFile.pNtHeader32->OptionalHeader + currentFile.pNtHeader32->FileHeader.SizeOfOptionalHeader);
		currentFile.sectionCount = currentFile.pNtHeader32->FileHeader.NumberOfSections;
		currentFile.exportDir = &currentFile.pNtHeader32->OptionalHeader.DataDirectory[0];
		currentFile.importDir = &currentFile.pNtHeader32->OptionalHeader.DataDirectory[1];
		currentFile.resourceDir = &currentFile.pNtHeader32->OptionalHeader.DataDirectory[2];
		currentFile.relocaleDir = &currentFile.pNtHeader32->OptionalHeader.DataDirectory[5];
	}
	else if (pNTHeader->OptionalHeader.Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC)
	{
		//ROM映像
		DWORD errorCode = GetLastError();
		logInfo = { L"暂不支持ROM映像" };
		CloseFile();
		return FALSE;
	}
	
	return TRUE;
}


std::vector<DosHeaderData> PECore::GetDosHeaderData()
{
	std::vector<DosHeaderData> data{};

#define ADD_WORD(field, desc) \
    data.push_back({#field, ToHex(currentFile.pDosHeader->field, 4), desc});


#define ADD_DWORD(field, desc) \
    data.push_back({#field, ToHex(currentFile.pDosHeader->field, 8), desc});

#define ADD_ARRAY(field, count, width, desc)           \
{                                                     \
    std::string val;                                  \
    for (int i = 0; i < count; ++i) {                 \
        val += ToHex(currentFile.pDosHeader->field[i], width); \
        if (i != count - 1) val += " ";               \
    }                                                 \
    data.push_back({#field, val, desc});              \
}

	ADD_WORD(e_magic, u8"[Magic number,就是一个标记]")
	ADD_WORD(e_cblp, u8"[Bytes on last page of file]")
	ADD_WORD(e_cp, u8"[Pages in file]")
	ADD_WORD(e_crlc, u8"[Relocations]")
	ADD_WORD(e_cparhdr, u8"[Size of header in paragraphs]")
	ADD_WORD(e_minalloc, u8"[Minimum extra paragraphs needed]")
	ADD_WORD(e_maxalloc, u8"[Maximum extra paragraphs needed]")
	ADD_WORD(e_ss, u8"[Initial (relative) SS value]")
	ADD_WORD(e_sp, u8"[Initial SP value]")
	ADD_WORD(e_csum, u8"[Checksum]")
	ADD_WORD(e_ip, u8"[Initial IP value]")
	ADD_WORD(e_cs, u8"[Initial (relative) CS value]")
	ADD_WORD(e_lfarlc, u8"[File address of relocation table]")
	ADD_WORD(e_ovno, u8"[Overlay number]")
	ADD_ARRAY(e_res,4,4,u8"[Reserved words]")
	ADD_WORD(e_oemid, u8"[OEM identifier (for e_oeminfo)]")
	ADD_WORD(e_oeminfo, u8"[OEM information; e_oemid specific")
	ADD_ARRAY(e_res2,10,4,u8"[Reserved words]")
	ADD_DWORD(e_lfanew, u8"[File address of new exe header。NT Header的文件地址=文件头+e_lfanew]")

#undef ADD_ARRAY
#undef ADD_DWORD
#undef ADD_WORD

	return data;
}
void PECore::CloseFile()
{
	currentFile.filePath = {};
	currentFile.pDosHeader = NULL;
	currentFile.pNtHeader32 = NULL;
	currentFile.pNtHeader64 = NULL;
	currentFile.is64 = false;
	currentFile.sectionHeaders = NULL;
	currentFile.sectionCount = 0;
	currentFile.importDir = NULL;
	currentFile.exportDir = NULL;
	currentFile.resourceDir = NULL;
	currentFile.relocaleDir = NULL;

	if (pCurrentAddrOfFileView)
	{
		UnmapViewOfFile(pCurrentAddrOfFileView);
		pCurrentAddrOfFileView = NULL;

		CloseHandle(hCurrentFileMappingObj);
		hCurrentFileMappingObj = NULL;

		CloseHandle(hCurrentFileHandle);
		hCurrentFileHandle = NULL;
	}

}

BOOLEAN PECore::GetOpenStatus()
{
	return pCurrentAddrOfFileView!=NULL && hCurrentFileHandle!=NULL && hCurrentFileMappingObj!=NULL;
}

PECore::~PECore()
{
}

PECore::PECore()
{
}
