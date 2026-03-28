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