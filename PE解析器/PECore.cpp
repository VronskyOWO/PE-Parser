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
	ADD_ARRAY(e_res,4,4,u8"[保留]")
	ADD_WORD(e_oemid, u8"[OEM identifier (for e_oeminfo)]")
	ADD_WORD(e_oeminfo, u8"[OEM information; e_oemid specific")
	ADD_ARRAY(e_res2,10,4,u8"[保留]")
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
std::vector<ExportData> PECore::GetExportData()
{
	std::vector<ExportData> data{};
	PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)((PCHAR)pCurrentAddrOfFileView + RvaToFoa(currentFile.exportDir->VirtualAddress));
	PWORD AddressOfNameOrdinals = (PWORD)((PCHAR)pCurrentAddrOfFileView + RvaToFoa(pExportDir->AddressOfNameOrdinals));
	PDWORD AddressOfNames = (PDWORD)((PCHAR)pCurrentAddrOfFileView + RvaToFoa(pExportDir->AddressOfNames));
	PDWORD AddressOfFunctions = (PDWORD)((PCHAR)pCurrentAddrOfFileView + RvaToFoa(pExportDir->AddressOfFunctions));
	bool flag=false;
	for (size_t i = 0; i < pExportDir->NumberOfFunctions; i++)
	{
		ExportData exportData{};
		exportData.number = std::to_string(pExportDir->Base + i);
		
		flag = false;
		size_t j = 0;
		//判断AddressOfFunctions[i]是不是名称导出
		for (j; j < pExportDir->NumberOfNames; j++)
		{
			if (AddressOfNameOrdinals[j] == i)
			{
				flag = true;
				break;
			}
		}
		if (flag)
		{
			exportData.funcName = std::string((PCHAR)pCurrentAddrOfFileView + RvaToFoa(AddressOfNames[j]));
		}
		else
		{
			exportData.funcName = std::string("");
		}

		exportData.rva = ToHex(AddressOfFunctions[i], 8);

		data.push_back(exportData);
	}
	
	return data;
}
std::vector<ImportData> PECore::GetImportData()
{
	std::vector<ImportData> importDatas{};
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor= (PIMAGE_IMPORT_DESCRIPTOR)((PCHAR)pCurrentAddrOfFileView + RvaToFoa(currentFile.importDir->VirtualAddress));

	while (!RtlIsZeroMemory(pImportDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR)))
	{
		ImportData importData{};
		importData.dllInfo.dllName = std::string((PCHAR)pCurrentAddrOfFileView + RvaToFoa(pImportDescriptor->Name));
		importData.dllInfo.firstThunk=ToHex(pImportDescriptor->FirstThunk,8);
		importData.dllInfo.forwarderChain=ToHex(pImportDescriptor->ForwarderChain,8);
		importData.dllInfo.originalFirstThunk=ToHex(pImportDescriptor->OriginalFirstThunk,8);
		importData.dllInfo.timeDateStamp=ToHex(pImportDescriptor->TimeDateStamp,8);

		PCHAR temp;
		if (pImportDescriptor->OriginalFirstThunk != NULL)
		{
			temp = (PCHAR)pCurrentAddrOfFileView +
				RvaToFoa(pImportDescriptor->OriginalFirstThunk);
		}
		else
		{
			temp = (PCHAR)pCurrentAddrOfFileView +
				RvaToFoa(pImportDescriptor->FirstThunk);
		}
		std::vector<FuncInfo> funcsInfo{};
		if (currentFile.is64)
		{
			PIMAGE_THUNK_DATA64 pThunk = (PIMAGE_THUNK_DATA64)temp;

			while (!RtlIsZeroMemory(pThunk, sizeof(IMAGE_THUNK_DATA64)))
			{
				FuncInfo funcInfo{};
				
				if (IMAGE_SNAP_BY_ORDINAL64(pThunk->u1.Ordinal))
				{
					//仅序号导出
					WORD ordinal = IMAGE_ORDINAL64(pThunk->u1.Ordinal);

					funcInfo.funcName = std::string(u8"仅序号导出");
					funcInfo.ordinal = ToHex(ordinal, 4);
				}
				else
				{
					//有名称导出
					PIMAGE_IMPORT_BY_NAME pImportByName =
						(PIMAGE_IMPORT_BY_NAME)((PCHAR)pCurrentAddrOfFileView +
							RvaToFoa(pThunk->u1.AddressOfData));

					funcInfo.funcName = std::string(pImportByName->Name);
					funcInfo.ordinal = ToHex(pImportByName->Hint, 4);
				}
				funcsInfo.push_back(funcInfo);
				pThunk++;
			}
		}
		else
		{
			PIMAGE_THUNK_DATA32 pThunk = (PIMAGE_THUNK_DATA32)temp;

			while (!RtlIsZeroMemory(pThunk, sizeof(IMAGE_THUNK_DATA32)))
			{
				FuncInfo funcInfo{};
				if (IMAGE_SNAP_BY_ORDINAL32(pThunk->u1.Ordinal))
				{
					WORD ordinal = IMAGE_ORDINAL32(pThunk->u1.Ordinal);

					funcInfo.funcName = std::string(u8"仅序号导出");
					funcInfo.ordinal = ToHex(ordinal, 4);
				}
				else
				{
					PIMAGE_IMPORT_BY_NAME pImportByName =
						(PIMAGE_IMPORT_BY_NAME)((PCHAR)pCurrentAddrOfFileView +
							RvaToFoa(pThunk->u1.AddressOfData));

					funcInfo.funcName = std::string(pImportByName->Name);
					funcInfo.ordinal = ToHex(pImportByName->Hint, 4);
				}
				funcsInfo.push_back(funcInfo);
				pThunk++;
			}
		}

		importData.funcsInfo = funcsInfo;
		importDatas.push_back(importData);
		pImportDescriptor++;
	}

	return importDatas;

}
OptionalHeaderData PECore::GetNtOptionalHeaderData()
{
	std::vector<std::string> dataDirectoryEntryDesc{
	u8"Export table address and size",
	u8"Import table address and size",
	u8"Resource table address and size",
	u8"Exception table address and size",
	u8"Certificate table address and size",
	u8"Base relocation table address and size",
	u8"Debugging information starting address and size",
	u8"Architecture-specific data address and size",
	u8"Global pointer register relative virtual address",
	u8"Thread local storage (TLS) table address and size",
	u8"Load configuration table address and size",
	u8"Bound import table address and size",
	u8"Import address table(IAT) address and size",
	u8"Delay import descriptor address and size",
	u8"The CLR header address and size",
	u8"Reserved"
	};
	OptionalHeaderData data{};
	if (currentFile.is64)
	{
#define ADD_BYTE(field, desc) \
    data.baseField.push_back({#field, ToHex(currentFile.pNtHeader64->OptionalHeader.field, 2), desc});

#define ADD_WORD(field, desc) \
    data.baseField.push_back({#field, ToHex(currentFile.pNtHeader64->OptionalHeader.field, 4), desc});

#define ADD_DWORD(field, desc) \
    data.baseField.push_back({#field, ToHex(currentFile.pNtHeader64->OptionalHeader.field, 8), desc});

#define ADD_QWORD(field, desc) \
    data.baseField.push_back({#field, ToHex(currentFile.pNtHeader64->OptionalHeader.field, 16), desc});

#define ADD_DATA_DIRECTORY_ENTRY(i, desc) \
    data.DataDirectory.push_back({ToHex(currentFile.pNtHeader64->OptionalHeader.DataDirectory[i].VirtualAddress, 8), ToHex(currentFile.pNtHeader64->OptionalHeader.DataDirectory[i].Size, 8), desc});
			
			ADD_WORD(Magic, u8"决定了镜像文件是PE32还是PE32+可执行文件。最常见的数值是 0x10B，它将其标识为普通可执行文件(PE32)。0x107 将其标识为 ROM 镜像，0x20B 将其标识为 PE32+ 可执行文件(x64)。")
			ADD_BYTE(MajorLinkerVersion, u8"链接器主版本号")
			ADD_BYTE(MinorLinkerVersion, u8"链接器次版本号")
			ADD_DWORD(SizeOfCode, u8"代码(.text)节的大小，若存在多个代码段则为所有代码段的总和")
			ADD_DWORD(SizeOfInitializedData, u8"已初始化数据节的大小，若存在多个数据节，则为所有此类节的总和。")
			ADD_DWORD(SizeOfUninitializedData, u8"未初始化数据节（BSS）的大小，若存在多个BSS节，则为所有此类节的总和。")
			ADD_DWORD(AddressOfEntryPoint, u8"可执行文件加载到内存时，入口点相对于ImageBase的偏移(RVA)。对于程序映像，这是起始地址；对于设备驱动，这是初始化函数的地址。动态链接库（DLL）的入口点为可选字段。若不存在入口点，此字段必须设为零。")
			ADD_DWORD(BaseOfCode, u8"代码节起始处相对于ImageBase的偏移量")
			ADD_QWORD(ImageBase, u8"映像加载到内存时首字节的首选地址")
			ADD_DWORD(SectionAlignment, u8"节加载到内存时的对齐方式（以字节为单位）。该值必须大于或等于FileAlignment。默认值为该架构的页面大小。")
			ADD_DWORD(FileAlignment, u8"The alignment factor (in bytes) that is used to align the raw data of sections in the image file.该值必须是介于 512 至 65536（含）之间的 2 的幂次方，默认值为 512。如果SectionAlignment小于系统架构的页面大小，则FileAlignment必须与SectionAlignment保持一致。")
			ADD_WORD(MajorOperatingSystemVersion, u8"所需操作系统的主版本号")
			ADD_WORD(MinorOperatingSystemVersion, u8"所需操作系统的次版本号")
			ADD_WORD(MajorImageVersion, u8"映像的主版本号")
			ADD_WORD(MinorImageVersion, u8"映像的次版本号")
			ADD_WORD(MajorSubsystemVersion, u8"子系统的主版本号")
			ADD_WORD(MinorSubsystemVersion, u8"子系统的次版本号")
			ADD_DWORD(Win32VersionValue, u8"保留，必须为零")
			ADD_DWORD(SizeOfImage, u8"映像加载到内存中的大小。该大小必须是SectionAlignment值的整数倍。")
			ADD_DWORD(SizeOfHeaders, u8"MS-DOS stub、PE 头和节头的总大小。向上取整为 FileAlignment 的倍数")
			ADD_DWORD(CheckSum, u8"映像文件校验和。用于计算校验和的算法已集成到 IMAGHELP.DLL 中。加载时会对以下内容进行验证：所有驱动程序、启动时加载的所有动态链接库（DLL），以及加载到关键 Windows 进程中的所有动态链接库（DLL）。")
			ADD_WORD(Subsystem, u8"运行此映像所需的子系统")
			ADD_WORD(DllCharacteristics, u8"DLL 特征")
			ADD_QWORD(SizeOfStackReserve, u8"要保留的堆栈大小。仅提交 SizeOfStackCommit 部分；其余部分会按需逐个页面分配，直至达到预留的总大小。")
			ADD_QWORD(SizeOfStackCommit, u8"要提交的堆栈大小")
			ADD_QWORD(SizeOfHeapReserve, u8"要保留的本地堆空间大小。仅提交 SizeOfHeapCommit 部分；其余部分会逐页分配，直至达到预留大小")
			ADD_QWORD(SizeOfHeapCommit, u8"要提交的本地堆空间大小")
			ADD_DWORD(LoaderFlags, u8"保留，必须为零")
			ADD_DWORD(NumberOfRvaAndSizes, u8"数据目录项数量")
				for (size_t i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++)
				{
					ADD_DATA_DIRECTORY_ENTRY(i, dataDirectoryEntryDesc[i])
				}
			
#undef ADD_DATA_DIRECTORY_ENTRY
#undef ADD_QWORD
#undef ADD_DWORD
#undef ADD_WORD
#undef ADD_BYTE
	}
	else
	{
#define ADD_BYTE(field, desc) \
    data.baseField.push_back({#field, ToHex(currentFile.pNtHeader32->OptionalHeader.field, 2), desc});

#define ADD_WORD(field, desc) \
    data.baseField.push_back({#field, ToHex(currentFile.pNtHeader32->OptionalHeader.field, 4), desc});

#define ADD_DWORD(field, desc) \
    data.baseField.push_back({#field, ToHex(currentFile.pNtHeader32->OptionalHeader.field, 8), desc});

#define ADD_QWORD(field, desc) \
    data.baseField.push_back({#field, ToHex(currentFile.pNtHeader32->OptionalHeader.field, 16), desc});

#define ADD_DATA_DIRECTORY_ENTRY(i, desc) \
    data.DataDirectory.push_back({ToHex(currentFile.pNtHeader32->OptionalHeader.DataDirectory[i].VirtualAddress, 8), ToHex(currentFile.pNtHeader32->OptionalHeader.DataDirectory[i].Size, 8), desc});

		ADD_WORD(Magic, u8"决定了镜像文件是PE32还是PE32+可执行文件。最常见的数值是 0x10B，它将其标识为普通可执行文件(PE32)。0x107 将其标识为 ROM 镜像，0x20B 将其标识为 PE32+ 可执行文件(x64)。")
			ADD_BYTE(MajorLinkerVersion, u8"链接器主版本号")
			ADD_BYTE(MinorLinkerVersion, u8"链接器次版本号")
			ADD_DWORD(SizeOfCode, u8"代码(.text)节的大小，若存在多个代码段则为所有代码段的总和")
			ADD_DWORD(SizeOfInitializedData, u8"已初始化数据节的大小，若存在多个数据节，则为所有此类节的总和。")
			ADD_DWORD(SizeOfUninitializedData, u8"未初始化数据节（BSS）的大小，若存在多个BSS节，则为所有此类节的总和。")
			ADD_DWORD(AddressOfEntryPoint, u8"可执行文件加载到内存时，入口点相对于ImageBase的偏移(RVA)。对于程序映像，这是起始地址；对于设备驱动，这是初始化函数的地址。动态链接库（DLL）的入口点为可选字段。若不存在入口点，此字段必须设为零。")
			ADD_DWORD(BaseOfCode, u8"代码节起始处相对于ImageBase的偏移量")
			ADD_DWORD(BaseOfData, u8"数据节开头加载到内存后，相对于其映像基址的偏移量.(PE32特意有)。")
			ADD_DWORD(ImageBase, u8"映像加载到内存时首字节的首选地址")
			ADD_DWORD(SectionAlignment, u8"节加载到内存时的对齐方式（以字节为单位）。该值必须大于或等于FileAlignment。默认值为该架构的页面大小。")
			ADD_DWORD(FileAlignment, u8"The alignment factor (in bytes) that is used to align the raw data of sections in the image file.该值必须是介于 512 至 65536（含）之间的 2 的幂次方，默认值为 512。如果SectionAlignment小于系统架构的页面大小，则FileAlignment必须与SectionAlignment保持一致。")
			ADD_WORD(MajorOperatingSystemVersion, u8"所需操作系统的主版本号")
			ADD_WORD(MinorOperatingSystemVersion, u8"所需操作系统的次版本号")
			ADD_WORD(MajorImageVersion, u8"映像的主版本号")
			ADD_WORD(MinorImageVersion, u8"映像的次版本号")
			ADD_WORD(MajorSubsystemVersion, u8"子系统的主版本号")
			ADD_WORD(MinorSubsystemVersion, u8"子系统的次版本号")
			ADD_DWORD(Win32VersionValue, u8"保留，必须为零")
			ADD_DWORD(SizeOfImage, u8"映像加载到内存中的大小。该大小必须是SectionAlignment值的整数倍。")
			ADD_DWORD(SizeOfHeaders, u8"所有“头部区域”在文件中的总大小（按 FileAlignment 对齐,dos header + dos stub + NT header + Section Table + padding）")
			ADD_DWORD(CheckSum, u8"映像文件校验和。用于计算校验和的算法已集成到 IMAGHELP.DLL 中。加载时会对以下内容进行验证：所有驱动程序、启动时加载的所有动态链接库（DLL），以及加载到关键 Windows 进程中的所有动态链接库（DLL）。")
			ADD_WORD(Subsystem, u8"运行此映像所需的子系统")
			ADD_WORD(DllCharacteristics, u8"DLL 特征")
			ADD_DWORD(SizeOfStackReserve, u8"要保留的堆栈大小。仅提交 SizeOfStackCommit 部分；其余部分会按需逐个页面分配，直至达到预留的总大小。")
			ADD_DWORD(SizeOfStackCommit, u8"要提交的堆栈大小")
			ADD_DWORD(SizeOfHeapReserve, u8"要保留的本地堆空间大小。仅提交 SizeOfHeapCommit 部分；其余部分会逐页分配，直至达到预留大小")
			ADD_DWORD(SizeOfHeapCommit, u8"要提交的本地堆空间大小")
			ADD_DWORD(LoaderFlags, u8"保留，必须为零")
			ADD_DWORD(NumberOfRvaAndSizes, u8"数据目录项数量")
			for (size_t i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++)
			{
				ADD_DATA_DIRECTORY_ENTRY(i, dataDirectoryEntryDesc[i])
			}

#undef ADD_DATA_DIRECTORY_ENTRY
#undef ADD_QWORD
#undef ADD_DWORD
#undef ADD_WORD
#undef ADD_BYTE
	}
	return data;
}

std::vector<std::vector<BaseData>> PECore::GetSectionsTableData()
{
	std::vector<std::vector<BaseData>> data{};

	for (size_t i = 0; i < currentFile.sectionCount; i++)
	{
		std::vector<BaseData> section{};
#define ADD_WORD(field, desc) \
    section.push_back({#field, ToHex(currentFile.sectionHeaders[i].field, 4), desc});

#define ADD_DWORD(field, desc) \
    section.push_back({#field, ToHex(currentFile.sectionHeaders[i].field, 8), desc});
#define ADD_ARRAY(field, count , width, desc)           \
{                                                     \
    std::string val;                                  \
    for (int j = 0; j < count; ++j) {                 \
        val += ToHex(currentFile.sectionHeaders[i].field[j], width); \
        if (j != count - 1) val += " ";               \
    }                                                 \
    val += " [";                                      \
    std::string name((char*)currentFile.sectionHeaders[i].field, count); \
    name.erase(name.find_last_not_of('\0') + 1);       \
    val += name;                                      \
    val += "]";                                       \
    section.push_back({#field, val, desc});           \
}


		ADD_ARRAY(Name, 8, 2, u8"section name,,不一定以NULL结尾");
		ADD_DWORD(Misc.VirtualSize, u8"加载到内存中时节的总大小(对齐前)。 如果此值大于 SizeOfRawData，则节中会用零填充。 此字段仅对可执行映像有效，应针对对象文件设置为零")
			ADD_DWORD(VirtualAddress, u8"对于可执行映像，是指当节加载到内存中时，该节相对于映像基址的RVA。 对于对象文件，此字段是应用重定位前第一个字节的地址；为简单起见，编译器应将此字段设置为零。 否则，它是重定位期间从偏移量中减去的任意值")
			ADD_DWORD(SizeOfRawData, u8"Section在文件中按FileAlignment对齐后的大小")
			ADD_DWORD(PointerToRawData, u8"指向COFF文件中该节第一页的文件指针。对于可执行映像，该值必须是可选头中FileAlignment的整数倍。对于目标文件，为获得最佳性能，该值应按4字节边界对齐。若某节仅包含未初始化数据，则此字段应设为零")
			ADD_DWORD(PointerToRelocations, u8"指向该节重定位条目起始位置的文件指针。对于可执行镜像或无重定位的情况，该指针会被设为零")
			ADD_DWORD(PointerToLinenumbers, u8"指向该节行号条目起始位置的文件指针。如果没有 COFF 行号，则该值设为零。对于可执行文件而言，此值应为零，因为 COFF 调试信息已被弃用")
			ADD_WORD(NumberOfRelocations, u8"该节的重定位条目数量。对于可执行镜像，此值设为零")
			ADD_WORD(NumberOfLinenumbers, u8"该节的行号条目数。对于image，此值应为零，因为 COFF 调试信息已被弃用。")
			ADD_DWORD(Characteristics, u8"描述该节特性的标志")

			data.push_back(section);
#undef ADD_ARRAY
#undef ADD_DWORD
#undef ADD_WORD
	}

	return data;
}
ResourceNode PECore::GetResourcesData()
{

	ResourceNode result;

	if (!currentFile.resourceDir->VirtualAddress)
		return result;

	DWORD baseRva = currentFile.resourceDir->VirtualAddress;
	DWORD foa = RvaToFoa(baseRva);

	auto root = (PIMAGE_RESOURCE_DIRECTORY)
		((PBYTE)pCurrentAddrOfFileView + foa);

	ParseResourceNode(root, baseRva, 0, result);

	return result;

}

void PECore::ParseResourceNode(
	PIMAGE_RESOURCE_DIRECTORY dir,
	DWORD baseRva,
	int level,
	ResourceNode& node
)
{
	auto entry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(dir + 1);
	int count = dir->NumberOfNamedEntries + dir->NumberOfIdEntries;

	node.level = level;
	if (level == 0)
	{
		node.isRoot = true;
	}

	for (int i = 0; i < count; i++, entry++)
	{
		ResourceNode subNode{};
		subNode.level = level + 1;
		// --- 名字 ---
		if (entry->NameIsString)
		{
			DWORD nameRva = baseRva + entry->NameOffset;
			DWORD nameFoa = RvaToFoa(nameRva);

			auto str = (PIMAGE_RESOURCE_DIR_STRING_U)
				((PBYTE)pCurrentAddrOfFileView + nameFoa);

			char utf8[MAX_PATH] = {};
			WideCharToMultiByte(CP_UTF8, 0,
				str->NameString,
				str->Length,
				utf8, sizeof(utf8),
				NULL, NULL);

			subNode.name = utf8;
			subNode.isNamed = true;
		}
		else
		{
			subNode.id = entry->Id;
		}

		// --- 子目录 ---
		if (entry->DataIsDirectory)
		{
			DWORD subRva = baseRva + (entry->OffsetToDirectory & 0x7FFFFFFF);
			DWORD subFoa = RvaToFoa(subRva);

			auto subDir = (PIMAGE_RESOURCE_DIRECTORY)
				((PBYTE)pCurrentAddrOfFileView + subFoa);

			ParseResourceNode(subDir, baseRva, level + 1, subNode);
		}
		else
		{
			DWORD dataEntryRva = baseRva + (entry->OffsetToData & 0x7FFFFFFF);
			auto pResDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)
				((PBYTE)pCurrentAddrOfFileView + RvaToFoa(dataEntryRva));

			ResourceData data{};
			data.dataRva = pResDataEntry->OffsetToData;
			data.dataSize = pResDataEntry->Size;
			data.codePage = pResDataEntry->CodePage;
			data.reserved = pResDataEntry->Reserved;

			//拷贝 raw data
			DWORD dataFoa = RvaToFoa(data.dataRva);
			BYTE* src = (BYTE*)pCurrentAddrOfFileView + dataFoa;

			data.rawData.assign(src, src + data.dataSize);

			subNode.data = data;
		}

		node.children.push_back(subNode);
	}
}


//void PECore::DrawResourceNode(
//	PIMAGE_RESOURCE_DIRECTORY dir,
//	DWORD baseRva,
//	DWORD level)
//{
//	auto entry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(dir + 1);
//	int count = dir->NumberOfNamedEntries + dir->NumberOfIdEntries;
//
//	for (int i = 0; i < count; i++, entry++)
//	{
//		char label[MAX_PATH] = { 0 };
//
//		// --- 解析名字 ---
//		if (entry->NameIsString)
//		{
//			DWORD nameRva = baseRva + entry->NameOffset;
//			DWORD nameFoa = RvaToFoa(nameRva);
//
//			auto str = (PIMAGE_RESOURCE_DIR_STRING_U)
//				((PCHAR)pCurrentAddrOfFileView+ nameFoa);
//
//			char utf8[MAX_PATH] = { 0 };
//			WideCharToMultiByte(CP_UTF8, 0,
//				str->NameString,
//				str->Length,
//				utf8, sizeof(utf8),
//				NULL, NULL);
//
//			sprintf_s(label, u8"资源名: %s", utf8);
//
//
//			switch (level)
//			{
//			case 1:
//				sprintf_s(label, u8"资源类型: %s", utf8);
//				break;
//			case 2:
//				sprintf_s(label, u8"资源名: %s", utf8);
//				break;
//			case 3:
//				sprintf_s(label, u8"资源语言: %s", utf8);
//				break;
//			default:
//				break;
//			}
//		}
//		else
//		{
//			switch (level)
//			{
//			case 1:
//				sprintf_s(label, u8"资源类型ID: %u--%s", entry->Id, GetResTypeName(entry->Id));
//				currentResTypeId = entry->Id;
//				break;
//			case 2:
//				sprintf_s(label, u8"资源名ID: %u", entry->Id);
//				break;
//			case 3:
//				sprintf_s(label, u8"资源语言ID: %u", entry->Id);
//				break;
//			default:
//				break;
//			}
//
//		}
//
//
//		// --- 子目录 ---
//		if (entry->DataIsDirectory)
//		{
//			DWORD subRva = baseRva + (entry->OffsetToDirectory & 0x7FFFFFFF);
//			DWORD subFoa = RvaToFoa(subRva);
//
//			auto subDir = (PIMAGE_RESOURCE_DIRECTORY)
//				(currentPE->fileReadBuffer + subFoa);
//
//			ImGui::PushID(entry);
//
//			if (ImGui::TreeNode(label))
//			{
//				DrawResourceNode(subDir, baseRva, level + 1);
//				ImGui::TreePop();
//			}
//
//			ImGui::PopID();
//
//		}
//		else
//		{
//			DWORD dataEntryRva = baseRva + (entry->OffsetToData & 0x7FFFFFFF);
//			auto pResDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)(currentPE->fileReadBuffer + RvaToFoa(dataEntryRva));
//			DWORD dataRva = pResDataEntry->OffsetToData;
//			// 叶子节点 → selectable
//
//			if (ImGui::Selectable(label, selectedResData.dataRva == dataRva))
//			{
//				selectedResData.dataRva = dataRva;
//				selectedResData.typeId = currentResTypeId;
//				selectedResData.resDataEntryRva = dataEntryRva;
//				selectedResData.dataSize = pResDataEntry->Size;
//			}
//
//
//		}
//	}
//}


const char* PECore::GetResTypeName(WORD id)
{
	switch (id)
	{
	case 1:  return "CURSOR";
	case 2:  return "BITMAP";
	case 3:  return "ICON";
	case 4:  return "MENU";
	case 5:  return "DIALOG";
	case 6:  return "STRING";
	case 7:  return "FONTDIR";
	case 8:  return "FONT";
	case 9:  return "ACCELERATOR";
	case 10: return "RCDATA";
	case 11: return "MESSAGETABLE";
	case 12: return "GROUP_CURSOR";
	case 14: return "GROUP_ICON";
	case 16: return "VERSION";
	case 24: return "MANIFEST";
	default: return "UNKNOWN";
	}
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

DWORD PECore::RvaToFoa(DWORD rva)
{
	DWORD sizeOfHeaders;

	// 在 headers 中
	if (rva < currentFile.sectionHeaders[0].VirtualAddress)
	{
		return rva;
	}

	// 遍历 section
	for (DWORD i = 0; i < currentFile.sectionCount; i++)
	{
		PIMAGE_SECTION_HEADER section = &currentFile.sectionHeaders[i];
		DWORD start = section->VirtualAddress;

		if (i == currentFile.sectionCount - 1)
		{
			return section->PointerToRawData + (rva - start);
		}

		DWORD nextSectionStart = currentFile.sectionHeaders[i + 1].VirtualAddress;

		if (rva >= start && rva < nextSectionStart)
		{
			return section->PointerToRawData + (rva - start);
		}
	}

	return 0;
}
