#include "App.h"

extern PECore peCore;
void DrawHexDump(const BYTE* data, size_t size, size_t bytesPerRow = 16);

void App::update()
{
    DrawMenuBar();

    //让主窗口 自动填充屏幕
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("Main",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove);

    if (ImGui::BeginTable("layout", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthFixed, 250.0f);
        ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        DrawPETree();

        ImGui::TableNextColumn();
        DrawPEView();

        ImGui::EndTable();
    }
    ImGui::End();

}
void App::DrawPEView()
{
    ImGui::BeginChild("PE View");

    ImGui::Text("PE Information");

    ImGui::Separator();
    // 检查是否有PE文件加载
    if (!peCore.GetOpenStatus())
    {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), u8"请先打开PE文件");
        ImGui::EndChild();
        return;
    }
    // 打印调试信息，确保 currentView 更新为 View_DOS
    switch (currentView)
    {
    case View_DOS:
        DrawDOSHeaderView(); 
        break;
    case View_NT_Signature:
        DrawNtSignatureView(); 
        break;
    case View_NT_FileHeader:
        DrawNtFileHeaderView(); 
        break;
    case View_NT_OptionalHeader:
        DrawNtOptionalHeaderView();
        break;
    case View_Sections:
        DrawSectionsView(); 
        break;
    case View_Export:
        DrawExportView();
        break;
    case View_Import:
        DrawImportView();
        break;
    case View_Resource:
        DrawResourceView();
        break;
    case View_BaseRelocale:
        DrawBaseRelocaleView();
        break;
    default:
        ImGui::Text(u8"等待加载文件...");
        break;
    }

    ImGui::EndChild();
}




void App::DrawResourceView()
{
    ImGui::BeginChild("Resource View");

    ImGui::Text(u8"Resource Tree(一级目录、二级目录、三级目录)");
    ImGui::Separator();

    if (!currentPE || currentPE->importDir == NULL)//??
    {
        ImGui::Text("No Resource");
        ImGui::EndChild();
        return;
    }

    auto dir = currentPE->is64 ?
        currentPE->pNtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] :
        currentPE->pNtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];

    if (dir.VirtualAddress == 0)
    {
        ImGui::Text("No Resource Directory");
        ImGui::EndChild();
        return;
    }

    DWORD baseRva = dir.VirtualAddress;
    DWORD foa = RvaToFoa(baseRva);

    auto root = (PIMAGE_RESOURCE_DIRECTORY)
        (currentPE->fileReadBuffer + foa);

    // --- 树 ---
    DrawResourceNode(root, baseRva,1);
    // --- 详情 ---
    ImGui::Separator();

    // Start two columns layout
    ImGui::Columns(2, "ResourceViewColumns", false);
    ImGui::Text("Resource Data Entry");

    // Left column - Resource Data Entry Details
    ImGui::BeginChild("Resource Details", ImVec2(0, 300), true);
    if (selectedResData.resDataEntryRva)
    {
        DWORD resDataEntryFoa = RvaToFoa(selectedResData.resDataEntryRva);
        PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)(currentPE->fileReadBuffer + resDataEntryFoa);

        // 资源数据详细信息
        ImGui::Text("OffsetToData 0x%X --------->", pResDataEntry->OffsetToData);
        ImGui::Text("Size: 0x%X", pResDataEntry->Size);
        ImGui::Text("CodePage: 0x%X", pResDataEntry->CodePage);
        ImGui::Text("Reserved: 0x%X", pResDataEntry->Reserved);
    }
    else
    {
        ImGui::Text("No item selected");
    }
    ImGui::EndChild();

    ImGui::NextColumn();  // Move to the next column (right column)
    ImGui::Text("Resource Data");
    // Right column - Resource Data
    ImGui::BeginChild("Resource Data", ImVec2(0, 300), true);
    if (selectedResData.dataRva)
    {
        DWORD dataFoa = RvaToFoa(selectedResData.dataRva);
        BYTE* data = (BYTE*)(currentPE->fileReadBuffer + dataFoa);

        ImGui::Text("raw bytes (size=0x%X)", selectedResData.dataSize);
        ImGui::Separator();
        DrawHexDump(data, (size_t)selectedResData.dataSize, 16);
    }
    ImGui::EndChild();

   
    // End columns
    ImGui::Columns(1);


    ImGui::EndChild();
}

const char* App::GetResTypeName(WORD id)
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

void App::DrawResourceNode(
    PIMAGE_RESOURCE_DIRECTORY dir,
    DWORD baseRva,
    DWORD level)
{
    auto entry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(dir + 1);
    int count = dir->NumberOfNamedEntries + dir->NumberOfIdEntries;

    for (int i = 0; i < count; i++, entry++)
    {
        char label[MAX_PATH] = { 0 };

        // --- 解析名字 ---
        if (entry->NameIsString)
        {
            DWORD nameRva = baseRva + entry->NameOffset;
            DWORD nameFoa = RvaToFoa(nameRva);

            auto str = (PIMAGE_RESOURCE_DIR_STRING_U)
                (currentPE->fileReadBuffer + nameFoa);

            char utf8[MAX_PATH] = {0};
            WideCharToMultiByte(CP_UTF8, 0,
                str->NameString,
                str->Length,
                utf8, sizeof(utf8),
                NULL, NULL);

            sprintf_s(label, u8"资源名: %s", utf8);


            switch (level)
            {
            case 1:
                sprintf_s(label, u8"资源类型: %s", utf8);
                break;
            case 2:
                sprintf_s(label, u8"资源名: %s", utf8);
                break;
            case 3:
                sprintf_s(label, u8"资源语言: %s", utf8);
                break;
            default:
                break;
            }
        }
        else
        {
            switch (level)
            {
            case 1:
                sprintf_s(label, u8"资源类型ID: %u--%s", entry->Id, GetResTypeName(entry->Id));
                currentResTypeId = entry->Id;
                break;
            case 2:
                sprintf_s(label, u8"资源名ID: %u", entry->Id);
                break;
            case 3:
                sprintf_s(label, u8"资源语言ID: %u", entry->Id);
                break;
            default:
                break;
            }
            
        }

        
        // --- 子目录 ---
        if (entry->DataIsDirectory)
        {
            DWORD subRva = baseRva + (entry->OffsetToDirectory & 0x7FFFFFFF);
            DWORD subFoa = RvaToFoa(subRva);

            auto subDir = (PIMAGE_RESOURCE_DIRECTORY)
                (currentPE->fileReadBuffer + subFoa);

            ImGui::PushID(entry);

            if (ImGui::TreeNode(label))
            {
                DrawResourceNode(subDir, baseRva, level + 1);
                ImGui::TreePop();
            }

            ImGui::PopID();

        }
        else
        {
            DWORD dataEntryRva = baseRva + (entry->OffsetToData & 0x7FFFFFFF);
            auto pResDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)(currentPE->fileReadBuffer + RvaToFoa(dataEntryRva));
            DWORD dataRva = pResDataEntry->OffsetToData;
            // 叶子节点 → selectable
            ImGui::PushID(entry);

            if (ImGui::Selectable(label, selectedResData.dataRva == dataRva))
            {
                selectedResData.dataRva = dataRva;
                selectedResData.typeId = currentResTypeId;
                selectedResData.resDataEntryRva = dataEntryRva;
                selectedResData.dataSize = pResDataEntry->Size;
            }

            ImGui::PopID();

        }
    }
}

void App::DrawBaseRelocaleView()
{
    if (currentPE->relocaleDir->VirtualAddress == NULL)
    {
        ImGui::Text(u8"no relocale table");
        return;
    }
    ImGui::BeginChild("BaseRelocale View");
    ImGui::Text("BaseRelocale Imformation");
    ImGui::Separator();
    // 给上下两个表格分配高度（你也可以改成你想要的比例）
    float availY = ImGui::GetContentRegionAvail().y;
    float topH = availY * 0.45f;
    float gap = ImGui::GetStyle().ItemSpacing.y;
    float bottomH = availY - topH - gap;
    if (bottomH < 100.0f) bottomH = 100.0f;

    // ========== 上表：重定位块列表（带滚动条） ==========
    ImGui::BeginChild("RelocBlocksChild", ImVec2(0, topH), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGuiTableFlags topFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("BaseRelocale Table", 3, topFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);//冻结表头
        ImGui::TableSetupColumn(u8"", ImGuiTableColumnFlags_WidthFixed, 250.0f);
        ImGui::TableSetupColumn(u8"VirtualAddress", ImGuiTableColumnFlags_WidthStretch, 170.0f);
        ImGui::TableSetupColumn(u8"SizeOfBlock", ImGuiTableColumnFlags_WidthStretch, 170.0f);
        ImGui::TableHeadersRow();

        //显示所有 IMAGE_BASE_RELOCATION 信息
        PIMAGE_BASE_RELOCATION pBaseRelocation= (PIMAGE_BASE_RELOCATION)(currentPE->fileReadBuffer + RvaToFoa(currentPE->relocaleDir->VirtualAddress));

        int index = 0;
        while (pBaseRelocation->VirtualAddress!=0)
        {
            CHAR arr[128] = { 0 };
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            sprintf_s(arr, sizeof(arr), "IMAGE_BASE_RELOCATION[%d]", index);
            if (ImGui::Selectable(arr, selectedRelocationIndex == index,
                ImGuiSelectableFlags_SpanAllColumns))
            {
                selectedRelocationIndex = index;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0x%08x", pBaseRelocation->VirtualAddress);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%08x", pBaseRelocation->SizeOfBlock);

            pBaseRelocation = (PIMAGE_BASE_RELOCATION)((PCHAR)pBaseRelocation + pBaseRelocation->SizeOfBlock);
            index++;
        }

        ImGui::EndTable();
    }
    ImGui::EndChild();//上表结束

    // ========== 下表：选中块的 entries（带滚动条） ==========
    ImGui::BeginChild("RelocEntriesChild", ImVec2(0, bottomH), true, ImGuiWindowFlags_HorizontalScrollbar);

    //找到所选中的 IMAGE_BASE_RELOCATION
    PIMAGE_BASE_RELOCATION pBaseRelocation = (PIMAGE_BASE_RELOCATION)(currentPE->fileReadBuffer + RvaToFoa(currentPE->relocaleDir->VirtualAddress));
    PIMAGE_BASE_RELOCATION selectedBaseRelocation = NULL;
    for (int i = 0; i <= selectedRelocationIndex; i++)
    {
        selectedBaseRelocation = pBaseRelocation;
        pBaseRelocation = (PIMAGE_BASE_RELOCATION)((PCHAR)pBaseRelocation + pBaseRelocation->SizeOfBlock);
    }
    ImGuiTableFlags bottomFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_RowBg;

    if (selectedBaseRelocation && ImGui::BeginTable("RelocationBlockEntrys", 3, bottomFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1); // 冻结表头
        ImGui::TableSetupColumn(u8"index", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn(u8"高4位(重定位类型)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"低12位(重定位偏移量)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        DWORD entryCount=(selectedBaseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION))/sizeof(WORD);
        PWORD pEntry = PWORD((PCHAR)selectedBaseRelocation + sizeof(IMAGE_BASE_RELOCATION));
        for (size_t i = 0; i < entryCount; i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%u", i);
            //高4位
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0x%01x", ((*pEntry) & 0xf000) >> 12);
            //低12位
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%04x", (*pEntry) & 0x0fff);
            pEntry++;
        }

        
        ImGui::EndTable();
    }
    ImGui::EndChild();


    ImGui::EndChild();
}

void App::DrawImportView()
{
    ImGui::BeginChild("Import View");
    ImGui::Text("Import Information");
    ImGui::Separator();


    // 计算可用高度
    float availY = ImGui::GetContentRegionAvail().y;
    float gap = ImGui::GetStyle().ItemSpacing.y;

    // 预留中间标题区域高度（Separator + Text + Spacing）
    float midH = 0.0f;
    midH += ImGui::GetFrameHeightWithSpacing(); // 大致按一行文字+spacing算
    midH += ImGui::GetStyle().SeparatorTextBorderSize; // 可忽略，但保守一点
    midH += gap; // 额外间距

    // 把中间区域扣掉，再分配给上下表格
    float remainY = availY - midH;
    if (remainY < 200.0f) remainY = availY; // 太小就别扣了，避免负数

    float topH = remainY * 0.45f;
    float bottomH = remainY - topH;
    if (bottomH < 100.0f) bottomH = 100.0f;


    // ========== 上表  ========== 
    ImGui::BeginChild("Import top window", ImVec2(0, topH), true, ImGuiWindowFlags_HorizontalScrollbar);

    ImGuiTableFlags topFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("Import Table", 5, topFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);//冻结表头
        ImGui::TableSetupColumn(u8"DLL Name", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn(u8"OriginalFirstThunk", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn(u8"TimeDateStamp", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"ForwarderChain", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"FirstThunk", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        int index = 0;
        PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor =(PIMAGE_IMPORT_DESCRIPTOR)(currentPE->fileReadBuffer + RvaToFoa(currentPE->importDir->VirtualAddress));
        while (!RtlIsZeroMemory(pImportDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR)))
        {
            PCHAR pName =currentPE->fileReadBuffer + RvaToFoa(pImportDescriptor->Name);
            ImGui::TableNextRow();
            // 第一列 selectable
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(pName, selectedImportIndex == index,
                ImGuiSelectableFlags_SpanAllColumns))
            {
                selectedImportIndex = index;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0x%08x",pImportDescriptor->OriginalFirstThunk);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%08x",pImportDescriptor->TimeDateStamp);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("0x%08x",pImportDescriptor->ForwarderChain);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("0x%08x",pImportDescriptor->FirstThunk);

            pImportDescriptor++;
            index++;
        }
        
        ImGui::EndTable();
    }
    ImGui::EndChild();// ========== 上表结束  ========== 



    PIMAGE_IMPORT_DESCRIPTOR selectedDescriptor = NULL;

    PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor =
        (PIMAGE_IMPORT_DESCRIPTOR)(currentPE->fileReadBuffer +
            RvaToFoa(currentPE->importDir->VirtualAddress));

    for (int i = 0; i <= selectedImportIndex; i++)
    {
        selectedDescriptor = pImportDescriptor;
        pImportDescriptor++;
    }

    
    ImGui::Separator();
    ImGui::Text("Imported Functions");

    // ========== 下表  ========== 
    ImGui::BeginChild("Import bottom window", ImVec2(0, bottomH), true, ImGuiWindowFlags_HorizontalScrollbar);

    ImGuiTableFlags bottomFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_RowBg;

    if (selectedDescriptor && ImGui::BeginTable("ImportFunctions", 2,
        bottomFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);//冻结表头
        ImGui::TableSetupColumn(u8"Ordinal(序号)", ImGuiTableColumnFlags_WidthFixed, 150);
        ImGui::TableSetupColumn(u8"Function Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        PCHAR temp;
        if (selectedDescriptor->OriginalFirstThunk != NULL)
        {
            temp = currentPE->fileReadBuffer +
                RvaToFoa(selectedDescriptor->OriginalFirstThunk);
        }
        else
        {
            temp = currentPE->fileReadBuffer +
                RvaToFoa(selectedDescriptor->FirstThunk);
        }

        if (currentPE->is64)
        {
            PIMAGE_THUNK_DATA64 pThunk = (PIMAGE_THUNK_DATA64)temp;

            while (!RtlIsZeroMemory(pThunk, sizeof(IMAGE_THUNK_DATA64)))
            {
                ImGui::TableNextRow();

                if (IMAGE_SNAP_BY_ORDINAL64(pThunk->u1.Ordinal))
                {
                    //仅序号导出
                    WORD ordinal = IMAGE_ORDINAL64(pThunk->u1.Ordinal);

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("0x%04x", ordinal);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text(u8"仅序号导出");
                }
                else
                {
                    //有名称导出
                    PIMAGE_IMPORT_BY_NAME pImportByName =
                        (PIMAGE_IMPORT_BY_NAME)(currentPE->fileReadBuffer +
                            RvaToFoa(pThunk->u1.AddressOfData));

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("0x%04x", pImportByName->Hint);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", pImportByName->Name);
                }

                pThunk++;
            }
        }
        else
        {
            PIMAGE_THUNK_DATA32 pThunk = (PIMAGE_THUNK_DATA32)temp;

            while (!RtlIsZeroMemory(pThunk, sizeof(IMAGE_THUNK_DATA32)))
            {
                ImGui::TableNextRow();

                if (IMAGE_SNAP_BY_ORDINAL32(pThunk->u1.Ordinal))
                {
                    WORD ordinal = IMAGE_ORDINAL32(pThunk->u1.Ordinal);

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("0x%04x", ordinal);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text(u8"仅序号导出");
                }
                else
                {
                    PIMAGE_IMPORT_BY_NAME pImportByName =
                        (PIMAGE_IMPORT_BY_NAME)(currentPE->fileReadBuffer +
                            RvaToFoa(pThunk->u1.AddressOfData));

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("0x%04x", pImportByName->Hint);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", pImportByName->Name);
                }

                pThunk++;
            }
        }


        ImGui::EndTable();
    }
    ImGui::EndChild();//====== 下表结束 ======


    ImGui::EndChild();
}
DWORD App::RvaToFoa(DWORD rva)
{
    DWORD sizeOfHeaders;

    // 在 headers 中
    if (rva < currentPE->sectionHeaders[0].VirtualAddress)
    {
        return rva;
    }

    // 遍历 section
    for (DWORD i = 0; i < currentPE->sectionCount; i++)
    {
        PIMAGE_SECTION_HEADER section = &currentPE->sectionHeaders[i];
        DWORD start = section->VirtualAddress;

        if (i == currentPE->sectionCount - 1)
        {
            return section->PointerToRawData + (rva - start);
        }

        DWORD nextSectionStart = currentPE->sectionHeaders[i + 1].VirtualAddress;
     
        if (rva >= start && rva < nextSectionStart)
        {
            return section->PointerToRawData + (rva - start);
        }
    }

    return 0;
}

void App::DrawExportView()
{
    ImGui::BeginChild("Export View");
    ImGui::Text("Export Information");
    ImGui::Separator();

    if (ImGui::BeginTable("Export Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn(u8"导出编号", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn(u8"函数导出名", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn(u8"RVA", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        //定位IMAGE_EXPORT_DIRECTORY
        DWORD foa = RvaToFoa(currentPE->exportDir->VirtualAddress);
        PIMAGE_EXPORT_DIRECTORY pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(currentPE->fileReadBuffer + foa);
        DWORD AddressOfFunctionsFOA = RvaToFoa(pExportDirectory->AddressOfFunctions);
        DWORD AddressOfNameOrdinalsFOA = RvaToFoa(pExportDirectory->AddressOfNameOrdinals);
        DWORD AddressOfNamesFOA = RvaToFoa(pExportDirectory->AddressOfNames);
        PDWORD AddressOfFunctions = (PDWORD)(currentPE->fileReadBuffer+ AddressOfFunctionsFOA);
        PWORD AddressOfNameOrdinals = (PWORD)(currentPE->fileReadBuffer+ AddressOfNameOrdinalsFOA);
        PDWORD AddressOfNames = (PDWORD)(currentPE->fileReadBuffer+ AddressOfNamesFOA);
        BOOL flag = false;
        for (size_t i = 0; i < pExportDirectory->NumberOfFunctions; i++)
        {
            flag = false;
            size_t j = 0;
            //判断AddressOfFunctions[i]是不是名称导出
            for (j; j < pExportDirectory->NumberOfNames; j++)
            {
                if (AddressOfNameOrdinals[j] == i)
                {
                    flag = true;
                    break;
                }
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", i + pExportDirectory->Base);
            if (flag)
            {
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", currentPE->fileReadBuffer + RvaToFoa(AddressOfNames[j]));
            }
           
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%08x", AddressOfFunctions[i]);
        }


        ImGui::EndTable();
    }

    ImGui::EndChild();
}
void App::DrawSectionsView()
{
    ImGui::BeginChild("Section Headers View");
    ImGui::Text("Section Headers Information");
    ImGui::Separator();
    CHAR tempSectionNameStr[9] = { 0 };
    if (ImGui::BeginTable("Section Headers Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
    {
        ImGui::TableHeadersRow();
        for (size_t i = 0; i < currentPE->sectionCount; i++)
        {
            RtlZeroMemory(tempSectionNameStr, 9);
            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(140, 140, 140, 255));
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Section Header[%d] field", i);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("value");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("description");

#define ADD_ROW(field, desc,format) \
            ImGui::TableNextRow(); \
            ImGui::TableSetColumnIndex(0); \
            ImGui::Text(#field); \
            ImGui::TableSetColumnIndex(1); \
            ImGui::Text(format, currentPE->sectionHeaders[i].field); \
            ImGui::TableSetColumnIndex(2); \
            ImGui::Text("%s", desc);


            RtlCopyMemory(tempSectionNameStr, currentPE->sectionHeaders[i].Name, IMAGE_SIZEOF_SHORT_NAME);
            ImGui::TableNextRow(); 
            ImGui::TableSetColumnIndex(0); 
            ImGui::Text("Name"); 
            ImGui::TableSetColumnIndex(1); 
            ImGui::Text("%s [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",tempSectionNameStr, currentPE->sectionHeaders[i].Name[0],
                currentPE->sectionHeaders[i].Name[1],
                currentPE->sectionHeaders[i].Name[2],
                currentPE->sectionHeaders[i].Name[3],
                currentPE->sectionHeaders[i].Name[4],
                currentPE->sectionHeaders[i].Name[5],
                currentPE->sectionHeaders[i].Name[6],
                currentPE->sectionHeaders[i].Name[7]);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text(u8"Section Name,不一定以NULL结尾");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Misc[union]");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0x%08x", currentPE->sectionHeaders[i].Misc.VirtualSize);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text(u8"VirtualSize/PhysicalAddress(PhysicalAddress是历史遗留，现代几乎都是解释成VirtualSize，即内存中对齐前的大小，但也不推荐使用，因为有的Section这个值可能不准确)");
            
            ADD_ROW(VirtualAddress, u8"Section在内存中的RVA", "0x%08x");
            ADD_ROW(SizeOfRawData, u8"Section在文件中按FileAlignment对齐后的大小", "0x%08x");
            ADD_ROW(PointerToRawData, u8"Section在文件中的偏移地址(FOA)", "0x%08x");
            ADD_ROW(PointerToRelocations, u8".OBJ文件中使用，指向重定位表的指针", "0x%08x");
            ADD_ROW(PointerToLinenumbers, u8"调试行号信息(现代基本不用)", "0x%08x");
            ADD_ROW(NumberOfRelocations, u8".OBJ文件中使用，重定位项数目.", "0x%04x");
            ADD_ROW(NumberOfLinenumbers, u8"行号表中行号的数量(现代基本不用)", "0x%04x");
            ADD_ROW(Characteristics, u8"section 属性(读/写/执行 等等)", "0x%08x");
#undef ADD_ROW
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
}
void App::DrawNtOptionalHeaderView()
{
    ImGui::BeginChild("NT Optional Header View");
    ImGui::Text("NT_Header.OptionalHeader Information");
    ImGui::Separator();

    if (ImGui::BeginTable("NT_Header.OptionalHeader Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
    {

        ImGui::TableHeadersRow();

        PVOID _pOptionalHeader = NULL;
        if (currentPE->is64)_pOptionalHeader = &currentPE->pNtHeader64->OptionalHeader;
        else _pOptionalHeader = &currentPE->pNtHeader32->OptionalHeader;

        if (currentPE->is64)
        {
            //64
#define ADD_ROW(field, desc,format) \
            ImGui::TableNextRow(); \
            ImGui::TableSetColumnIndex(0); \
            ImGui::Text(#field); \
            ImGui::TableSetColumnIndex(1); \
            ImGui::Text(format, PIMAGE_OPTIONAL_HEADER64(_pOptionalHeader)->field); \
            ImGui::TableSetColumnIndex(2); \
            ImGui::Text("%s", desc);

            ADD_ROW(Magic,"","0x%04x");
            ADD_ROW(MajorLinkerVersion, "","0x%02x");
            ADD_ROW(MinorLinkerVersion, "","0x%02x");
            ADD_ROW(SizeOfCode, "","0x%08x");
            ADD_ROW(SizeOfInitializedData, "","0x%08x");
            ADD_ROW(SizeOfUninitializedData, "","0x%08x");
            ADD_ROW(AddressOfEntryPoint, "","0x%08x");
            ADD_ROW(BaseOfCode, "","0x%08x");
            ADD_ROW(ImageBase, "","0x%016llx");
            ADD_ROW(SectionAlignment, "","0x%08x");
            ADD_ROW(FileAlignment, "","0x%08x");
            ADD_ROW(MajorOperatingSystemVersion, "","0x%04x");
            ADD_ROW(MinorOperatingSystemVersion, "","0x%04x");
            ADD_ROW(MajorImageVersion, "","0x%04x");
            ADD_ROW(MinorImageVersion, "","0x%04x");
            ADD_ROW(MajorSubsystemVersion, "","0x%04x");
            ADD_ROW(MinorSubsystemVersion, "","0x%04x");
            ADD_ROW(Win32VersionValue, "","0x%08x");
            ADD_ROW(SizeOfImage, "","0x%08x");
            ADD_ROW(SizeOfHeaders, "","0x%08x");
            ADD_ROW(CheckSum, "","0x%08x");
            ADD_ROW(Subsystem, "","0x%04x");
            ADD_ROW(DllCharacteristics, "","0x%04x");
            ADD_ROW(SizeOfStackReserve, "","0x%016llx");
            ADD_ROW(SizeOfStackCommit, "","0x%016llx");
            ADD_ROW(SizeOfHeapReserve, "","0x%016llx");
            ADD_ROW(SizeOfHeapCommit, "","0x%016llx");
            ADD_ROW(LoaderFlags, "", "0x%08x");
            ADD_ROW(NumberOfRvaAndSizes, "", "0x%08x");
            
            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(140, 140, 140, 255));
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("DataDirectory");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Description");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("RVA/Size");

#define ADD_DATA_DIRECTORY_ROW(index,name) \
            ImGui::TableNextRow(); \
            ImGui::TableSetColumnIndex(0); \
            ImGui::Text("DataDirectory[%d]",index); \
            ImGui::TableSetColumnIndex(1); \
            ImGui::Text("%s", name); \
            ImGui::TableSetColumnIndex(2); \
            ImGui::Text("0x%08x / 0x%08x", PIMAGE_OPTIONAL_HEADER64(_pOptionalHeader)->DataDirectory[index].VirtualAddress,PIMAGE_OPTIONAL_HEADER64(_pOptionalHeader)->DataDirectory[index].Size);

            ADD_DATA_DIRECTORY_ROW(0, "Export table address and size");
            ADD_DATA_DIRECTORY_ROW(1, "Import table address and size");
            ADD_DATA_DIRECTORY_ROW(2, "Resource table address and size");
            ADD_DATA_DIRECTORY_ROW(3, "Exception table address and size");
            ADD_DATA_DIRECTORY_ROW(4, "Certificate table address and size");
            ADD_DATA_DIRECTORY_ROW(5, "Base relocation table address and size");
            ADD_DATA_DIRECTORY_ROW(6, "Debugging information starting address and size");
            ADD_DATA_DIRECTORY_ROW(7, "Architecture-specific data address and size");
            ADD_DATA_DIRECTORY_ROW(8, "Global pointer register relative virtual address");
            ADD_DATA_DIRECTORY_ROW(9, "Thread local storage (TLS) table address and size");
            ADD_DATA_DIRECTORY_ROW(10, "Load configuration table address and size");
            ADD_DATA_DIRECTORY_ROW(11, "Bound import table address and size");
            ADD_DATA_DIRECTORY_ROW(12, "Import address table(IAT) address and size");
            ADD_DATA_DIRECTORY_ROW(13, "Delay import descriptor address and size");
            ADD_DATA_DIRECTORY_ROW(14, "The CLR header address and size");
            ADD_DATA_DIRECTORY_ROW(15, "Reserved");
#undef ADD_DATA_DIRECTORY_ROW

#undef ADD_ROW
           
        }
        else
        {
            //32
#define ADD_ROW(field, desc,format) \
            ImGui::TableNextRow(); \
            ImGui::TableSetColumnIndex(0); \
            ImGui::Text(#field); \
            ImGui::TableSetColumnIndex(1); \
            ImGui::Text(format, PIMAGE_OPTIONAL_HEADER32(_pOptionalHeader)->field); \
            ImGui::TableSetColumnIndex(2); \
            ImGui::Text("%s", desc);

            ADD_ROW(Magic, "", "0x%04x");
            ADD_ROW(MajorLinkerVersion, "", "0x%02x");
            ADD_ROW(MinorLinkerVersion, "", "0x%02x");
            ADD_ROW(SizeOfCode, "", "0x%08x");
            ADD_ROW(SizeOfInitializedData, "", "0x%08x");
            ADD_ROW(SizeOfUninitializedData, "", "0x%08x");
            ADD_ROW(AddressOfEntryPoint, "", "0x%08x");
            ADD_ROW(BaseOfCode, "", "0x%08x");
            ADD_ROW(ImageBase, "", "0x%08x");
            ADD_ROW(SectionAlignment, "", "0x%08x");
            ADD_ROW(FileAlignment, "", "0x%08x");
            ADD_ROW(MajorOperatingSystemVersion, "", "0x%04x");
            ADD_ROW(MinorOperatingSystemVersion, "", "0x%04x");
            ADD_ROW(MajorImageVersion, "", "0x%04x");
            ADD_ROW(MinorImageVersion, "", "0x%04x");
            ADD_ROW(MajorSubsystemVersion, "", "0x%04x");
            ADD_ROW(MinorSubsystemVersion, "", "0x%04x");
            ADD_ROW(Win32VersionValue, "", "0x%08x");
            ADD_ROW(SizeOfImage, "", "0x%08x");
            ADD_ROW(SizeOfHeaders, "", "0x%08x");
            ADD_ROW(CheckSum, "", "0x%08x");
            ADD_ROW(Subsystem, "", "0x%04x");
            ADD_ROW(DllCharacteristics, "", "0x%04x");
            ADD_ROW(SizeOfStackReserve, "", "0x%08x");
            ADD_ROW(SizeOfStackCommit, "", "0x%08x");
            ADD_ROW(SizeOfHeapReserve, "", "0x%08x");
            ADD_ROW(SizeOfHeapCommit, "", "0x%08x");
            ADD_ROW(LoaderFlags, "", "0x%08x");
            ADD_ROW(NumberOfRvaAndSizes, "", "0x%08x");


            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(140, 140, 140, 255));
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("DataDirectory");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Description");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("RVA/Size");

#define ADD_DATA_DIRECTORY_ROW(index,name) \
            ImGui::TableNextRow(); \
            ImGui::TableSetColumnIndex(0); \
            ImGui::Text("DataDirectory[%d]",index); \
            ImGui::TableSetColumnIndex(1); \
            ImGui::Text("%s", name); \
            ImGui::TableSetColumnIndex(2); \
            ImGui::Text("0x%08x / 0x%08x", PIMAGE_OPTIONAL_HEADER32(_pOptionalHeader)->DataDirectory[index].VirtualAddress,PIMAGE_OPTIONAL_HEADER32(_pOptionalHeader)->DataDirectory[index].Size);

            ADD_DATA_DIRECTORY_ROW(0, "Export table address and size");
            ADD_DATA_DIRECTORY_ROW(1, "Import table address and size");
            ADD_DATA_DIRECTORY_ROW(2, "Resource table address and size");
            ADD_DATA_DIRECTORY_ROW(3, "Exception table address and size");
            ADD_DATA_DIRECTORY_ROW(4, "Certificate table address and size");
            ADD_DATA_DIRECTORY_ROW(5, "Base relocation table address and size");
            ADD_DATA_DIRECTORY_ROW(6, "Debugging information starting address and size");
            ADD_DATA_DIRECTORY_ROW(7, "Architecture-specific data address and size");
            ADD_DATA_DIRECTORY_ROW(8, "Global pointer register relative virtual address");
            ADD_DATA_DIRECTORY_ROW(9, "Thread local storage (TLS) table address and size");
            ADD_DATA_DIRECTORY_ROW(10, "Load configuration table address and size");
            ADD_DATA_DIRECTORY_ROW(11, "Bound import table address and size");
            ADD_DATA_DIRECTORY_ROW(12, "Import address table(IAT) address and size");
            ADD_DATA_DIRECTORY_ROW(13, "Delay import descriptor address and size");
            ADD_DATA_DIRECTORY_ROW(14, "The CLR header address and size");
            ADD_DATA_DIRECTORY_ROW(15, "Reserved");
#undef ADD_DATA_DIRECTORY_ROW

#undef ADD_ROW
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
}


void App::DrawNtFileHeaderView()
{
    
    std::vector<NtFileHeaderData> viewData=peCore.GetNtFileHeaderData();
    ImGui::BeginChild("NT File Header View");
    ImGui::Text("NT_Header.FileHeader Information");
    ImGui::Separator();

    if (ImGui::BeginTable("NT_Header.FileHeader Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();
        for (size_t i = 0; i < viewData.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", viewData[i].field.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", viewData[i].value.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", viewData[i].descriptor.c_str());
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
}
void App::DrawNtSignatureView()
{
    NtSignatureData sign= peCore.GetNtSignatureData();
    ImGui::BeginChild("NT Signature  View");

    ImGui::Text("NT_Header.Signature Information");
    ImGui::Separator();

    if (ImGui::BeginTable("NTSignature", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s",sign.field.c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", sign.value.c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s",sign.descriptor.c_str());

        ImGui::EndTable();
    }

    ImGui::EndChild();
}
void App::DrawPETree()
{
    ImGui::BeginChild("PE Tree");

    // DOS Header 作为一个 Leaf 节点
    if (ImGui::TreeNodeEx("DOS Header", ImGuiTreeNodeFlags_Leaf))
    {
        // 使用 ImGui::IsItemClicked 来检测是否点击了此节点
        if (ImGui::IsItemClicked())
        {
            currentView = View_DOS;
        }
        ImGui::TreePop();
    }

    // NT HEADER
    if (ImGui::TreeNodeEx("NT Header", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 子节点：Signature
        if (ImGui::TreeNodeEx("Signature", ImGuiTreeNodeFlags_Leaf))
        {
            if (ImGui::IsItemClicked())
            {
                currentView = View_NT_Signature;
            }
            ImGui::TreePop();
        }

        // 子节点：FileHeader
        if (ImGui::TreeNodeEx("FileHeader", ImGuiTreeNodeFlags_Leaf))
        {
            if (ImGui::IsItemClicked())
            {
                currentView = View_NT_FileHeader;
            }
            ImGui::TreePop();
        }

        // 子节点：OptionalHeader
        if (ImGui::TreeNodeEx("OptionalHeader", ImGuiTreeNodeFlags_Leaf))
        {
            if (ImGui::IsItemClicked())
            {
                currentView = View_NT_OptionalHeader;
            }
            ImGui::TreePop();
        }

        ImGui::TreePop(); // 关闭 NT Header
    }

    // Sections 作为 Leaf 节点
    if (ImGui::TreeNodeEx("Section Headers", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::IsItemClicked())
        {
            currentView = View_Sections;
        }
        ImGui::TreePop();
    }

    // Import Table 作为 Leaf 节点
    if (ImGui::TreeNodeEx("Import", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::IsItemClicked())
        {
            currentView = View_Import;
        }
        ImGui::TreePop();
    }

    // Export Table 作为 Leaf 节点
    if (ImGui::TreeNodeEx("Export", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::IsItemClicked())
        {
            currentView = View_Export;
        }
        ImGui::TreePop();
    }

    // Resource Table 作为 Leaf 节点
    if (ImGui::TreeNodeEx("Resource", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::IsItemClicked())
        {
            currentView = View_Resource;
        }
        ImGui::TreePop(); 
    }

    if (ImGui::TreeNodeEx("BaseRelocale", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::IsItemClicked())
        {
            currentView = View_BaseRelocale;
        }
        ImGui::TreePop();
    }

    ImGui::EndChild();
}




void App::DrawMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu(u8"文件"))
        {
            if (ImGui::MenuItem(u8"打开"))
            {
                OpenFile();
            }

            if (ImGui::MenuItem(u8"关闭"))
            {
                CloseFile();
            }

            ImGui::Separator();

            if (ImGui::MenuItem(u8"退出"))
            {
                PostQuitMessage(0);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(u8"视图"))
        {
            if (ImGui::MenuItem(u8"Sections"))
            {
                
            }
            if (ImGui::MenuItem(u8"Imports"))
            {

            }
            if (ImGui::MenuItem(u8"Exports"))
            {

            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(u8"工具"))
        {
            ImGui::Text(u8"PE Parser");
            ImGui::Text(u8"Author: Vronsky");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(u8"帮助"))
        {
            if (ImGui::MenuItem(u8"关于"))
            {

            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void App::OpenFile()
{
    CloseFile();
    if (!currentPE)
    {
        MessageBoxA(0, "currentPE == NULL", 0, 0);
        return;
    }
    char file[MAX_PATH] = { 0 };
    OPENFILENAMEA ofn={0};
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        "PE Files (*.exe;*.dll;*.sys;*.efi)\0*.exe;*.dll;*.sys;*.efi\0"
        "EXE Files (*.exe)\0*.exe\0"
        "DLL Files (*.dll)\0*.dll\0"
        "SYS Files (*.sys)\0*.sys\0"
        "UEFI Files (*.efi)\0*.efi\0"
        "All Files (*.*)\0*.*\0";

    std::wstring log{};


    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn))
    {
        BOOLEAN res2=peCore.OpenFile(ofn.lpstrFile, log);
        if (!res2)
        {
            MessageBoxW(0, log.c_str(), 0, 0);
        }
        currentView = View_DOS;
    }
    
}

void App::CloseFile()
{
    selectedResData.dataRva = 0;
    selectedResData.typeId = -1;
    selectedResData.resDataEntryRva = 0;
    selectedResData.dataSize = 0;
    selectedImportIndex = -1;
    selectedRelocationIndex = -1;

    
    peCore.CloseFile();         

    currentView = View_None;
}


void App::SetDarkTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(10, 5);
    style.ItemSpacing = ImVec2(8, 6);
    style.ScrollbarSize = 14;

    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.TabRounding = 3.0f;

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);

    colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);

    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.26f, 1.00f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);

    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.45f, 0.48f, 1.00f);

    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.48f, 1.00f);

    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);

    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
}

void App::DrawDOSHeaderView()
{
    std::vector<DosHeaderData> data = peCore.GetDosHeaderData();
    ImGui::BeginChild("DOS Header View");

    ImGui::Text("DOS Header Information");
    ImGui::Separator();

    if (ImGui::BeginTable("DOSHeaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable| ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        auto* dos = currentPE->pDosHeader;

        for (size_t i = 0; i < data.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text(data[i].field.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text(data[i].value.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text(data[i].descriptor.c_str());
        }


        ImGui::EndTable();
    }

    ImGui::EndChild();
}


App::App()
{
    currentPE = (PEFile*)VirtualAlloc(NULL, sizeof(PEFile), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!currentPE)
    {
        MessageBoxA(0, "PEFile: VirtualAlloc failed", 0, 0);
        return;
    }
    currentView = View_None; // 确保初始化为 View_None
}

App::~App()
{
    if (currentPE->pDosHeader)
    {
        VirtualFree(currentPE->pDosHeader, 0, MEM_RELEASE);
        currentPE->pDosHeader = nullptr;
    }
    if (currentPE->pNtHeader32)
    {
        VirtualFree(currentPE->pNtHeader32, 0, MEM_RELEASE);
        currentPE->pNtHeader32 = nullptr;
    }
    if (currentPE->pNtHeader64)
    {
        VirtualFree(currentPE->pNtHeader64, 0, MEM_RELEASE);
        currentPE->pNtHeader64 = nullptr;
    }
    if (currentPE->sectionHeaders)
    {
        VirtualFree(currentPE->sectionHeaders, 0, MEM_RELEASE);
        currentPE->sectionHeaders = nullptr;
    }
    if (currentPE->fileReadBuffer)
    {
        VirtualFree(currentPE->fileReadBuffer, 0, MEM_RELEASE);
        currentPE->fileReadBuffer = nullptr;
    }
    if (currentPE)
    {
        VirtualFree(currentPE,0, MEM_RELEASE);
        currentPE = nullptr;
    }
}



 static void DrawHexDump(const BYTE* data, size_t size, size_t bytesPerRow)
 {
     if (!data || size == 0)
     {
         ImGui::Text("Empty");
         return;
     }

     ImGui::BeginChild("HexDump", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

     // 预估每行字符数：偏移(8) + 空格 + hex(3*16) + 空格 + ascii(16) + 结束
     // 用 ImGuiListClipper 避免大数据卡顿
     const size_t rowCount = (size + bytesPerRow - 1) / bytesPerRow;

     ImGuiListClipper clipper;
     clipper.Begin((int)rowCount);
     while (clipper.Step())
     {
         for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
         {
             size_t offset = (size_t)row * bytesPerRow;
             size_t count = bytesPerRow;
             if (offset + count > size) count = size - offset;

             char line[256]{};
             char* p = line;

             // 偏移
             p += sprintf_s(p, sizeof(line) - (p - line), "%08llX  ", (unsigned long long)offset);

             // Hex 区
             for (size_t i = 0; i < bytesPerRow; ++i)
             {
                 if (i < count)
                     p += sprintf_s(p, sizeof(line) - (p - line), "%02X ", data[offset + i]);
                 else
                     p += sprintf_s(p, sizeof(line) - (p - line), "   ");
             }

             // 分隔
             p += sprintf_s(p, sizeof(line) - (p - line), " ");

             // ASCII 区
             for (size_t i = 0; i < count; ++i)
             {
                 BYTE c = data[offset + i];
                 *p++ = (c >= 32 && c <= 126) ? (char)c : '.';
             }
             *p = '\0';

             ImGui::TextUnformatted(line);
         }
     }

     ImGui::EndChild();
 }