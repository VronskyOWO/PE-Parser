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

    ImGui::Text(u8"Resource Viewer");
    ImGui::Separator();

    // ⭐ 用当前剩余高度
    float availY = ImGui::GetContentRegionAvail().y;

    // 上半部分高度（比如 50%）
    float topH = availY * 0.5f;

    // =========================
    // 上：资源树
    // =========================
    ImGui::BeginChild("ResourceTree", ImVec2(0, topH), true);

    DrawResourceNode(resourceData);

    ImGui::EndChild();

    ImGui::Separator();

    // =========================
    // 下：直接吃剩余空间（关键！）
    // =========================
    ImGui::BeginChild("ResourceData", ImVec2(0, 0), true);

    if (pSelectedNode && pSelectedNode->level == 3 && pSelectedNode->data.has_value())
    {
        auto& data = pSelectedNode->data.value();

        ImGui::Text("DataRVA: 0x%X", data.dataRva);
        ImGui::Text("Size:    0x%X", data.dataSize);
        ImGui::Text("CodePage: 0x%X", data.codePage);

        ImGui::Separator();

        ImGui::Text("Raw Data:");
        ImGui::Separator();

        DrawHexDump(data.rawData.data(), data.rawData.size(), 16);
    }
    else
    {
        ImGui::Text(u8"请选择一个 Level 3 资源节点");
    }

    ImGui::EndChild();

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


void App::DrawResourceNode(ResourceNode& node)
{
    char label[256] = {};

    // ===== 构造显示文本 =====
    if (node.level == 0)
    {
        sprintf_s(label, u8"root");
    }
    else
    {
        switch (node.level)
        {
        case 1:
            if (node.isNamed)
                sprintf_s(label, u8"资源类型: %s", node.name.c_str());
            else
                sprintf_s(label, u8"资源类型ID: %d (%s)", node.id, GetResTypeName(node.id));
            break;

        case 2:
            if (node.isNamed)
                sprintf_s(label, u8"资源名: %s", node.name.c_str());
            else
                sprintf_s(label, u8"资源名ID: %d", node.id);
            break;

        case 3:
            if (node.isNamed)
                sprintf_s(label, u8"语言: %s", node.name.c_str());
            else
                sprintf_s(label, u8"语言ID: %d", node.id);
            break;
        }
    }

    ImGui::PushID(&node);

    // ===== Level 3：叶子节点 =====
    if (node.level == 3)
    {
        bool selected = (pSelectedNode == &node);

        if (ImGui::Selectable(label, selected))
        {
            pSelectedNode = &node;
        }
    }
    else
    {
        // ===== 非叶子节点 =====
        if (ImGui::TreeNode(label))
        {
            for (auto& c : node.children)
            {
                DrawResourceNode(c);
            }
            ImGui::TreePop();
        }
    }

    ImGui::PopID();
}



void App::DrawBaseRelocaleView()
{
    if (baseRelocaleData.empty())
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

        
        for (int i = 0; i < baseRelocaleData.size(); i++)
        {
            CHAR arr[128] = { 0 };
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            sprintf_s(arr, sizeof(arr), "IMAGE_BASE_RELOCATION[%d]", i);
            if (ImGui::Selectable(arr, selectedRelocationIndex == i,
                ImGuiSelectableFlags_SpanAllColumns))
            {
                selectedRelocationIndex = i;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0x%08x", baseRelocaleData[i].blockInfo.VirtualAddress);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%08x", baseRelocaleData[i].blockInfo.SizeOfBlock);

        }

        ImGui::EndTable();
    }
    ImGui::EndChild();//上表结束

    // ========== 下表：选中块的 entries（带滚动条） ==========
    ImGui::BeginChild("RelocEntriesChild", ImVec2(0, bottomH), true, ImGuiWindowFlags_HorizontalScrollbar);

    
    ImGuiTableFlags bottomFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_RowBg;

    if (selectedRelocationIndex != -1 && ImGui::BeginTable("RelocationBlockEntrys", 3, bottomFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1); // 冻结表头
        ImGui::TableSetupColumn(u8"index", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn(u8"高4位(重定位类型)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"低12位(重定位偏移量)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        for (size_t i = 0; i < baseRelocaleData[selectedRelocationIndex].blockEntrys.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%u", i);
            //高4位
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0x%01x", ((baseRelocaleData[selectedRelocationIndex].blockEntrys[i]) & 0xf000) >> 12);
            //低12位
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%04x", (baseRelocaleData[selectedRelocationIndex].blockEntrys[i]) & 0x0fff);
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

        for (size_t i = 0; i < importDatas.size(); i++)
        {
            ImGui::TableNextRow();
            // 第一列 selectable
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(importDatas[i].dllInfo.dllName.c_str(), selectedImportIndex == i,
                ImGuiSelectableFlags_SpanAllColumns))
            {
                selectedImportIndex = i;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", importDatas[i].dllInfo.originalFirstThunk.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", importDatas[i].dllInfo.timeDateStamp.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", importDatas[i].dllInfo.forwarderChain.c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", importDatas[i].dllInfo.firstThunk.c_str());

        }
        
        ImGui::EndTable();
    }
    ImGui::EndChild();// ========== 上表结束  ========== 



    
    ImGui::Separator();
    ImGui::Text("Imported Functions");

    //// ========== 下表  ========== 
    //ImGui::BeginChild("Import bottom window", ImVec2(0, bottomH), true, ImGuiWindowFlags_HorizontalScrollbar);

    //ImGuiTableFlags bottomFlags =
    //    ImGuiTableFlags_Borders |
    //    ImGuiTableFlags_Resizable |
    //    ImGuiTableFlags_ScrollY |
    //    ImGuiTableFlags_RowBg;

    //if (selectedImportIndex!=-1 && ImGui::BeginTable("ImportFunctions", 2,
    //    bottomFlags))
    //{
    //    ImGui::TableSetupScrollFreeze(0, 1);//冻结表头
    //    ImGui::TableSetupColumn(u8"Ordinal(序号)", ImGuiTableColumnFlags_WidthFixed, 150);
    //    ImGui::TableSetupColumn(u8"Function Name", ImGuiTableColumnFlags_WidthStretch);
    //    ImGui::TableHeadersRow();

    //    for (size_t i = 0; i < importDatas[selectedImportIndex].funcsInfo.size(); i++)
    //    {
    //        ImGui::TableNextRow();
    //        // 第一列 selectable
    //        ImGui::TableSetColumnIndex(0);
    //        ImGui::Text("%s", importDatas[selectedImportIndex].funcsInfo[i].ordinal.c_str());
    //        ImGui::TableSetColumnIndex(1);
    //        ImGui::Text("%s", importDatas[selectedImportIndex].funcsInfo[i].funcName.c_str());

    //    }

    //    ImGui::EndTable();
    //}
    //ImGui::EndChild();//====== 下表结束 ======


    // ========== 下表  ==========
    ImGui::BeginChild(
        "Import bottom window",
        ImVec2(0, bottomH),
        true,
        ImGuiWindowFlags_HorizontalScrollbar
    );

    ImGuiTableFlags bottomFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_RowBg;

    if (selectedImportIndex != -1 &&
        ImGui::BeginTable("ImportFunctions", 3, bottomFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);

        ImGui::TableSetupColumn(
            u8"Hint(给loader在导出表查找时的建议性索引)",
            ImGuiTableColumnFlags_WidthFixed,
            120
        );

        ImGui::TableSetupColumn(
            u8"Ordinal(对应导出编号)",
            ImGuiTableColumnFlags_WidthFixed,
            120
        );

        ImGui::TableSetupColumn(
            u8"Name",
            ImGuiTableColumnFlags_WidthStretch
        );

        ImGui::TableHeadersRow();

        auto& funcs =
            importDatas[selectedImportIndex].funcsInfo;

        for (size_t i = 0; i < funcs.size(); i++)
        {
            auto& func = funcs[i];

            ImGui::TableNextRow();

            //
            // Hint
            //
            ImGui::TableSetColumnIndex(0);

            if (!func.importByOrdinal)
                ImGui::Text("%s", func.hint.c_str());
            else
                ImGui::TextUnformatted("");

            //
            // Ordinal
            //
            ImGui::TableSetColumnIndex(1);

            if (func.importByOrdinal)
                ImGui::Text("%s", func.ordinal.c_str());
            else
                ImGui::TextUnformatted("");

            //
            // Name
            //
            ImGui::TableSetColumnIndex(2);

            ImGui::Text("%s", func.funcName.c_str());
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
    //====== 下表结束 ======
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
        
        
        for (size_t i = 0; i < exportData.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", exportData[i].number.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", exportData[i].funcName.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", exportData[i].rva.c_str());
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
    if (ImGui::BeginTable("Section Headers Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
    {
        ImGui::TableHeadersRow();
        for (size_t i = 0; i < sectionHeadersData.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(140, 140, 140, 255));
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Section Header[%d] field", i);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("value");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("description");

            
            for (size_t j = 0; j < sectionHeadersData[i].size(); j++)
            {

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", sectionHeadersData[i][j].field.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", sectionHeadersData[i][j].value.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", sectionHeadersData[i][j].description.c_str());
            }

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
        for (size_t i = 0; i < optionalHeaderData.baseField.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", optionalHeaderData.baseField[i].field.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", optionalHeaderData.baseField[i].value.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", optionalHeaderData.baseField[i].description.c_str());
        }
        ImGui::TableNextRow();
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(100, 100, 180, 255)); 
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(u8"DataDirectory");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text(u8"VirtualAddress / Size");
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("description");
        for (size_t i = 0; i < optionalHeaderData.DataDirectory.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("DataDirectory[%d]", i);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s / %s", optionalHeaderData.DataDirectory[i].virtualAddress.c_str(), optionalHeaderData.DataDirectory[i].size.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", optionalHeaderData.DataDirectory[i].description.c_str());
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
}


void App::DrawNtFileHeaderView()
{
    
    ImGui::BeginChild("NT File Header View");
    ImGui::Text("NT_Header.FileHeader Information");
    ImGui::Separator();

    if (ImGui::BeginTable("NT_Header.FileHeader Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();
        for (size_t i = 0; i < ntFileHeaderData.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", ntFileHeaderData[i].field.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", ntFileHeaderData[i].value.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", ntFileHeaderData[i].description.c_str());
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
}
void App::DrawNtSignatureView()
{
 
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
        ImGui::Text("%s", ntSignatureData.field.c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", ntSignatureData.value.c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", ntSignatureData.description.c_str());

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
    importDatas = peCore.GetImportData();
    exportData = peCore.GetExportData();
    sectionHeadersData = peCore.GetSectionsTableData();
    optionalHeaderData = peCore.GetNtOptionalHeaderData();
    dosHeaderData = peCore.GetDosHeaderData();
    ntSignatureData = peCore.GetNtSignatureData();
    resourceData = peCore.GetResourcesData();
    ntFileHeaderData = peCore.GetNtFileHeaderData();
    pSelectedNode = nullptr;
    baseRelocaleData = peCore.GetBaseRelocaleData();
    
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

        for (size_t i = 0; i < dosHeaderData.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text(dosHeaderData[i].field.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text(dosHeaderData[i].value.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text(dosHeaderData[i].description.c_str());
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



 //static void DrawHexDump(const BYTE* data, size_t size, size_t bytesPerRow)
 //{
 //    if (!data || size == 0)
 //    {
 //        ImGui::Text("Empty");
 //        return;
 //    }

 //    ImGui::BeginChild("HexDump", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

 //    // 预估每行字符数：偏移(8) + 空格 + hex(3*16) + 空格 + ascii(16) + 结束
 //    // 用 ImGuiListClipper 避免大数据卡顿
 //    const size_t rowCount = (size + bytesPerRow - 1) / bytesPerRow;

 //    ImGuiListClipper clipper;
 //    clipper.Begin((int)rowCount);
 //    while (clipper.Step())
 //    {
 //        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
 //        {
 //            size_t offset = (size_t)row * bytesPerRow;
 //            size_t count = bytesPerRow;
 //            if (offset + count > size) count = size - offset;

 //            char line[256]{};
 //            char* p = line;

 //            // 偏移
 //            p += sprintf_s(p, sizeof(line) - (p - line), "%08llX  ", (unsigned long long)offset);

 //            // Hex 区
 //            for (size_t i = 0; i < bytesPerRow; ++i)
 //            {
 //                if (i < count)
 //                    p += sprintf_s(p, sizeof(line) - (p - line), "%02X ", data[offset + i]);
 //                else
 //                    p += sprintf_s(p, sizeof(line) - (p - line), "   ");
 //            }

 //            // 分隔
 //            p += sprintf_s(p, sizeof(line) - (p - line), " ");

 //            // ASCII 区
 //            for (size_t i = 0; i < count; ++i)
 //            {
 //                BYTE c = data[offset + i];
 //                *p++ = (c >= 32 && c <= 126) ? (char)c : '.';
 //            }
 //            *p = '\0';

 //            ImGui::TextUnformatted(line);
 //        }
 //    }

 //    ImGui::EndChild();
 //}

static void DrawHexDump(const BYTE* data, size_t size, size_t bytesPerRow)
{
    if (!data || size == 0)
    {
        ImGui::Text("Empty");
        return;
    }

    std::string dump;
    dump.reserve(size * 4);

    const size_t rowCount = (size + bytesPerRow - 1) / bytesPerRow;

    for (size_t row = 0; row < rowCount; ++row)
    {
        size_t offset = row * bytesPerRow;
        size_t count = std::min(bytesPerRow, size - offset);

        char line[256]{};
        char* p = line;

        p += sprintf_s(p, sizeof(line), "%08llX  ",
            (unsigned long long)offset);

        for (size_t i = 0; i < bytesPerRow; ++i)
        {
            if (i < count)
                p += sprintf_s(
                    p,
                    sizeof(line) - (p - line),
                    "%02X ",
                    data[offset + i]
                );
            else
                p += sprintf_s(
                    p,
                    sizeof(line) - (p - line),
                    "   "
                );
        }

        *p++ = ' ';

        for (size_t i = 0; i < count; ++i)
        {
            BYTE c = data[offset + i];
            *p++ = (c >= 32 && c <= 126) ? c : '.';
        }

        *p++ = '\n';
        *p = '\0';

        dump += line;
    }

    ImGui::BeginChild(
        "HexDump",
        ImVec2(0, 0),
        false,
        ImGuiWindowFlags_HorizontalScrollbar
    );

    // ⭐ 切换到等宽字体
    ImGui::PushFont(g_HexFont);

    ImGui::InputTextMultiline(
        "##HexDumpText",
        dump.data(),
        dump.size() + 1,
        ImVec2(-FLT_MIN, -FLT_MIN),
        ImGuiInputTextFlags_ReadOnly
    );

    ImGui::PopFont();

    ImGui::EndChild();
}