#include "PECore.h"

static MachineType g_MachineTypes[] =
{
	{0x0,    u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前假定此字段的内容适用于任何计算机类型"},
	{0x184,  u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Alpha AXP，32 位地址空间"},
	{0x284,  u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Alpha 64/AXP 64，64 位地址空间"},
	{0x1d3,  u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Matsushita AM33"},
	{0x8664, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:x64"},
	{0x1c0,  u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:ARM little endian"},
	{0xaa64, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:ARM64 little endian"},
	{0x1c4,  u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:ARM Thumb-2 little endian"},
	{0xebc,  u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:EFI Byte Code"},
	{0x14c,  u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Intel 386 或更高版本的处理器和兼容的处理器"},
	{0x200,  u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Intel Itanium 处理器系列"},
	{0x6232, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:LoongArch 32 位处理器系列"},
	{0x6264, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:LoongArch 64 位处理器系列"},
	{0x9041, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:三菱 M32R 小 endian"},
	{0x266, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:MIPS16"},
	{0x366, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:将 MIPS 与 FPU 结合使用"},
	{0x466, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:将 MIPS16 与 FPU 结合使用"},
	{0x1f0, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Power PC 小 endian"},
	{0x1f1, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:支持浮点的 Power PC"},
	{0x160, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:MIPS I 兼容 32 位大尾号"},
	{0x162, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:MIPS I 兼容 32 位小 endian"},
	{0x166, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:MIPS III 兼容的 64 位小 endian"},
	{0x168, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:MIPS IV 兼容 64 位小 endian"},
	{0x168, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:MIPS IV 兼容 64 位小 endian"},
	{0x5032, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:RISC-V 32 位地址空间"},
	{0x5064, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:RISC-V 64 位地址空间"},
	{0x5128, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:RISC-V 128 位地址空间"},
	{0x1a2, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Hitachi SH3"},
	{0x1a3, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Hitachi SH3 DSP"},
	{0x1a6, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Hitachi SH4"},
	{0x1a8, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:Hitachi SH5"},
	{0x1c2, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:ARM Thumb/Thumb-2 Little-Endian"},
	{0x169, u8"字段表示:这个 PE 文件是为哪种CPU架构编译的,指示PE运行平台。当前:MIPS little-endian WCE v2"},
};
BOOLEAN PECore::OpenFile(LPSTR filePath,_Out_ std::wstring& logInfo)
{
	CloseFile();

	//file mapping
	hCurrentFileHandle=CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hCurrentFileHandle == INVALID_HANDLE_VALUE)
	{	
		DWORD errorCode = GetLastError();
		CloseFile();
		logInfo = { L"CreateFile failed" };
		return FALSE;
	}
	SECURITY_ATTRIBUTES sa = {0};
	hCurrentFileMappingObj =CreateFileMappingA(hCurrentFileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hCurrentFileMappingObj)
	{
		DWORD errorCode = GetLastError();
		CloseFile();
		logInfo = { L"CreateFileMapping failed" };
		return FALSE;
	}
	pCurrentAddrOfFileView =MapViewOfFile(hCurrentFileMappingObj, FILE_MAP_READ, 0, 0, 0);
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
		//x64
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
		//x32
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

NtSignatureData PECore::GetNtSignatureData()
{
	if (currentFile.is64) return { u8"Signature", ToHex(currentFile.pNtHeader64->Signature,8), u8"一个签名,标识该文件为 PE 格式映像文件" };
	else return { u8"Signature", ToHex(currentFile.pNtHeader32->Signature,8), u8"一个签名,标识该文件为 PE 格式映像文件" };
}
std::vector<NtFileHeaderData> PECore::GetNtFileHeaderData()
{
	std::vector<DosHeaderData> data{};

	if (currentFile.is64)
	{
#define ADD_WORD(field, desc) \
    data.push_back({#field, ToHex(currentFile.pNtHeader64->FileHeader.field, 4), desc});

#define ADD_DWORD(field, desc) \
    data.push_back({#field, ToHex(currentFile.pNtHeader64->FileHeader.field, 8), desc});

		ADD_WORD(Machine, GetMachineType(currentFile.pNtHeader64->FileHeader.Machine)->description)
		ADD_WORD(NumberOfSections, u8"Section(节)的数量")
		ADD_DWORD(TimeDateStamp, u8"此PE的创建时间")
		ADD_DWORD(PointerToSymbolTable, u8"COFF 符号表的文件偏移量")
		ADD_DWORD(NumberOfSymbols, u8"符号表中的项数")
		ADD_WORD(SizeOfOptionalHeader, u8"OptionalHeader的大小。但对象文件不需要它。 对于对象文件，此值应为零。")
		ADD_WORD(Characteristics, u8"指示文件属性的标志")

#undef ADD_DWORD
#undef ADD_WORD
	}
	else
	{
#define ADD_WORD(field, desc) \
    data.push_back({#field, ToHex(currentFile.pNtHeader32->FileHeader.field, 4), desc});

#define ADD_DWORD(field, desc) \
    data.push_back({#field, ToHex(currentFile.pNtHeader32->FileHeader.field, 8), desc});

		ADD_WORD(Machine, GetMachineType(currentFile.pNtHeader32->FileHeader.Machine)->description)
			ADD_WORD(NumberOfSections, u8"Section(节)的数量")
			ADD_DWORD(TimeDateStamp, u8"此PE的创建时间")
			ADD_DWORD(PointerToSymbolTable, u8"COFF 符号表的文件偏移量")
			ADD_DWORD(NumberOfSymbols, u8"符号表中的项数")
			ADD_WORD(SizeOfOptionalHeader, u8"OptionalHeader的大小。但对象文件不需要它。 对于对象文件，此值应为零。")
			ADD_WORD(Characteristics, u8"指示文件属性的标志")

#undef ADD_DWORD
#undef ADD_WORD
	}

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


const MachineType* PECore::GetMachineType(WORD machine)
{
	for (auto& m : g_MachineTypes)
	{
		if (m.value == machine)
			return &m;
	}

	return nullptr;
}