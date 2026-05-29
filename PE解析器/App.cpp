#include "App.h"
#include <vector>
#include <d3d11.h>
#include <functional>
#include <algorithm>

extern PECore peCore;
// forward declarations for helper parsing functions
static bool ParseIconRaw(const std::vector<BYTE>& iconRaw, int& outW, int& outH, std::vector<BYTE>& outRGBA);
static bool TryParseFirstIconInGroup(const ResourceNode& root, const ResourceData& groupData, int& outW, int& outH, std::vector<BYTE>& outRGBA);
void DrawHexDump(const BYTE* data, size_t size, size_t bytesPerRow = 16);
// forward declaration so UploadTexture_D3D11 can be used above its definition
static ID3D11ShaderResourceView* UploadTexture_D3D11(const unsigned char* rgba, int w, int h, ID3D11Device* dev, ID3D11DeviceContext* ctx);
static int GetTopLevelTypeId(const ResourceNode& root, const ResourceNode* target);
static bool BuildIcoFromGroup(const ResourceNode& root, const ResourceData& groupData, std::vector<BYTE>& outIco);
// 返回 HICON（调用者负责 DestroyIcon）; data 指向 ICO/ICONDIR 所在的内存，size 为长度
static HICON CreateIconFromMemory(const BYTE* data, size_t size)
{
    // CreateIconFromResourceEx 需要 POINT to an image resource (icon image), not the group directory.
    // For full .ico files the function can parse the ICO file header.
    // Try CreateIconFromResourceEx first (expects resource-format), fall back if fails。
    BOOL fIcon = TRUE;
    HICON hIcon = CreateIconFromResourceEx(
        (PBYTE)data,
        (DWORD)size,
        TRUE,
        0x00030000, // version
        0, 0,
        LR_DEFAULTCOLOR
    );
    return hIcon;
}

// 把 HICON -> width,height 和 RGBA(8-bit) 像素（RGBA）
static bool HICON_To_RGBA(HICON hIcon, int& outW, int& outH, std::vector<BYTE>& outRGBA)
{
    if (!hIcon) return false;

    ICONINFO ii;
    if (!GetIconInfo(hIcon, &ii)) return false;

    BITMAP bmp;
    HBITMAP hbmp = (HBITMAP)ii.hbmColor ? ii.hbmColor : ii.hbmMask;
    if (!GetObject(hbmp, sizeof(bmp), &bmp)) {
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return false;
    }

    int w = bmp.bmWidth;
    int h = bmp.bmHeight;
    // Some icon bitmaps have height == icon_height*2 (color + mask). Normalize if needed.
    if (h % 2 == 0 && (h / 2) == w) {
        // unlikely, but keep original if not matching; don't blindly halve.
    }
    outW = w; outH = h;
    outRGBA.assign(w * h * 4, 0);

    // create compatible DC and DIB section
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(NULL);
    void* pvBits = nullptr;
    HBITMAP hDib = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    if (!hDib) { ReleaseDC(NULL, hdc); if (ii.hbmColor) DeleteObject(ii.hbmColor); if (ii.hbmMask) DeleteObject(ii.hbmMask); return false; }

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP old = (HBITMAP)SelectObject(mem, hDib);

    // draw icon to DIB
    DrawIconEx(mem, 0, 0, hIcon, w, h, 0, NULL, DI_NORMAL);

    // copy pixels (BGRA in memory)
    const BYTE* src = (const BYTE*)pvBits;
    // convert BGRA -> RGBA
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int si = (y * w + x) * 4;
            BYTE b = src[si + 0];
            BYTE g = src[si + 1];
            BYTE r = src[si + 2];
            BYTE a = src[si + 3];
            int di = (y * w + x) * 4;
            outRGBA[di + 0] = r;
            outRGBA[di + 1] = g;
            outRGBA[di + 2] = b;
            outRGBA[di + 3] = a;
        }
    }

    // cleanup
    SelectObject(mem, old);
    DeleteObject(hDib);
    DeleteDC(mem);
    ReleaseDC(NULL, hdc);
    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);

    return true;
}

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
    case View_BoundImport:
        DrawBoundImport();
        break;
    case View_DelayImport:
        DrawDelayLoadImportView();
        break;
    case View_TLS:
        DrawTlsView();
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

    // 成员缓存
    std::vector<BYTE> icoBuffer;

    if (pSelectedNode && pSelectedNode->level == 3 && pSelectedNode->data.has_value())
    {
        auto& data = pSelectedNode->data.value();

        // If selected node changed since last frame, clear current icon texture so it will be re-decoded
        if (prevSelectedNode != pSelectedNode)
        {
            if (iconTexture)
            {
                auto old = reinterpret_cast<ID3D11ShaderResourceView*>(iconTexture);
                if (old) old->Release();
                iconTexture = (ImTextureID)0;
                iconTexW = 0; iconTexH = 0;
            }
            prevSelectedNode = pSelectedNode;
            selectedResData.dataRva = data.dataRva;
            selectedResData.dataSize = data.dataSize;
            selectedResData.typeId = GetTopLevelTypeId(resourceData, pSelectedNode);
        }

        ImGui::Text("DataRVA: 0x%X", data.dataRva);
        ImGui::Text("Size:    0x%X", data.dataSize);
        ImGui::Text("CodePage: 0x%X", data.codePage);

        // 右侧显示区
        ImGui::SameLine();
        ImGui::BeginGroup(); // 把右侧作为一组

        int topType = GetTopLevelTypeId(resourceData, pSelectedNode);

        if (topType == 3) // RT_ICON
        {
            if (iconTexture)
            {
                // 已有纹理，直接显示 using persistent size members
                ImGui::Image(iconTexture, ImVec2((float)iconTexW, (float)iconTexH));
            }
            else
            {
                // First try Windows API to create HICON from resource-format data
                HICON hIcon = CreateIconFromMemory(data.rawData.data(), data.rawData.size());
                int w = 0, h = 0;
                std::vector<BYTE> rgba;
                bool uploaded = false;

                if (hIcon && HICON_To_RGBA(hIcon, w, h, rgba))
                {
                    // release old texture if any
                    if (iconTexture)
                    {
                        auto old = reinterpret_cast<ID3D11ShaderResourceView*>(iconTexture);
                        if (old) old->Release();
                        iconTexture = (ImTextureID)0;
                    }

                    ID3D11ShaderResourceView* srv = UploadTexture_D3D11(rgba.data(), w, h, g_pd3dDevice, g_pd3dDeviceContext);
                    if (srv)
                    {
                        iconTexture = (ImTextureID)srv;
                        iconTexW = w; iconTexH = h; // store into members
                        ImGui::Image(iconTexture, ImVec2((float)iconTexW, (float)iconTexH));
                        uploaded = true;
                    }
                }
                if (hIcon) { DestroyIcon(hIcon); hIcon = NULL; }

                // Fallback: try parse RT_ICON raw data directly (BITMAPINFOHEADER + pixels)
                if (!uploaded)
                {
                    int pw=0, ph=0;
                    std::vector<BYTE> prgba;
                    if (ParseIconRaw(data.rawData, pw, ph, prgba))
                    {
                        if (iconTexture)
                        {
                            auto old = reinterpret_cast<ID3D11ShaderResourceView*>(iconTexture);
                            if (old) old->Release();
                            iconTexture = (ImTextureID)0;
                        }
                        ID3D11ShaderResourceView* srv = UploadTexture_D3D11(prgba.data(), pw, ph, g_pd3dDevice, g_pd3dDeviceContext);
                        if (srv)
                        {
                            iconTexture = (ImTextureID)srv;
                            iconTexW = pw; iconTexH = ph;
                            ImGui::Image(iconTexture, ImVec2((float)iconTexW, (float)iconTexH));
                            uploaded = true;
                        }
                    }
                }

                if (!uploaded)
                {
                    ImGui::Text(u8"无法解码 ICON 资源");
                }
            }
        }
        else if (topType == 14) // RT_GROUP_ICON
        {
            bool uploaded = false;
            // build ICO from group resource + RT_ICON entries in resourceData
            if (BuildIcoFromGroup(resourceData, data, icoBuffer))
            {
                // same workflow: create HICON from ICO memory, convert, upload
                HICON hIcon = CreateIconFromMemory(icoBuffer.data(), icoBuffer.size());
                int w = 0, h = 0;
                std::vector<BYTE> rgba;
                if (hIcon && HICON_To_RGBA(hIcon, w, h, rgba))
                {
                    if (iconTexture)
                    {
                        auto old = reinterpret_cast<ID3D11ShaderResourceView*>(iconTexture);
                        if (old) old->Release();
                        iconTexture = (ImTextureID)0;
                    }

                    ID3D11ShaderResourceView* srv = UploadTexture_D3D11(rgba.data(), w, h, g_pd3dDevice, g_pd3dDeviceContext);
                    if (srv)
                    {
                        iconTexture = (ImTextureID)srv;
                        iconTexW = w; iconTexH = h; // store into members
                        ImGui::Image(iconTexture, ImVec2((float)iconTexW, (float)iconTexH));
                        uploaded = true;
                    }
                }
                if (hIcon) { DestroyIcon(hIcon); hIcon = NULL; }

                // If HICON path failed, try parsing first referenced RT_ICON raw and upload
                if (!uploaded)
                {
                    int pw=0, ph=0;
                    std::vector<BYTE> prgba;
                    if (TryParseFirstIconInGroup(resourceData, data, pw, ph, prgba))
                    {
                        if (iconTexture)
                        {
                            auto old = reinterpret_cast<ID3D11ShaderResourceView*>(iconTexture);
                            if (old) old->Release();
                            iconTexture = (ImTextureID)0;
                        }
                        ID3D11ShaderResourceView* srv = UploadTexture_D3D11(prgba.data(), pw, ph, g_pd3dDevice, g_pd3dDeviceContext);
                        if (srv)
                        {
                            iconTexture = (ImTextureID)srv;
                            iconTexW = pw; iconTexH = ph;
                            ImGui::Image(iconTexture, ImVec2((float)iconTexW, (float)iconTexH));
                            uploaded = true;
                        }
                    }
                }
            }
            else
            {
                // BuildIcoFromGroup failed; as fallback, try to parse first RT_ICON referenced by groupData
                int pw=0, ph=0;
                std::vector<BYTE> prgba;
                if (TryParseFirstIconInGroup(resourceData, data, pw, ph, prgba))
                {
                    if (iconTexture)
                    {
                        auto old = reinterpret_cast<ID3D11ShaderResourceView*>(iconTexture);
                        if (old) old->Release();
                        iconTexture = (ImTextureID)0;
                    }
                    ID3D11ShaderResourceView* srv = UploadTexture_D3D11(prgba.data(), pw, ph, g_pd3dDevice, g_pd3dDeviceContext);
                    if (srv)
                    {
                        iconTexture = (ImTextureID)srv;
                        iconTexW = pw; iconTexH = ph;
                        ImGui::Image(iconTexture, ImVec2((float)iconTexW, (float)iconTexH));
                        uploaded = true;
                    }
                }
                else
                {
                    ImGui::Text(u8"无法从 GROUP_ICON 构建 ICO 或解析底层 RT_ICON");
                }
            }

            if (!uploaded && iconTexture==0)
            {
                // show placeholder
                ImGui::Text(u8"无法显示图标");
            }
        }

        ImGui::EndGroup();

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

void App::DrawBoundImport()
{

    if (boundImportData.empty())
    {
        ImGui::Text("No Bound Import");
        return;
    }
    ImGui::BeginChild("Bound Import View");
    ImGui::Text("Bound Import Imformation");
    ImGui::Separator();
   
    ImGuiTableFlags Flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("Bound Import Table",5, Flags))
    {
        ImGui::TableSetupScrollFreeze(0, 1); // 冻结表头
        ImGui::TableSetupColumn(u8"", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn(u8"DllName", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"TimeDateStamp", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"OffsetModuleName", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"NumberOfModuleForwarderRefs", ImGuiTableColumnFlags_WidthStretch);

        
        for (size_t i = 0; i < boundImportData.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("IMAGE_BOUND_IMPORT_DESCRIPTOR[%d]", i);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text(boundImportData[i].biDescriptor.dllName.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%08x", boundImportData[i].biDescriptor.biDescriptor.TimeDateStamp);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("0x%04x", boundImportData[i].biDescriptor.biDescriptor.OffsetModuleName);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("0x%04x", boundImportData[i].biDescriptor.biDescriptor.NumberOfModuleForwarderRefs);
            for (size_t i = 0; i < boundImportData[i].refs.size(); i++)
            {

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("IMAGE_BOUND_FORWARDER_REF[%d]", i);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(boundImportData[i].refs[i].dllName.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("0x%08x", boundImportData[i].refs[i].ref.TimeDateStamp);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("0x%04x", boundImportData[i].refs[i].ref.OffsetModuleName);
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("0x%04x(Reserved)", boundImportData[i].refs[i].ref.Reserved);
            }
        }

        ImGui::EndTable();
    }
   

    ImGui::EndChild();
}

void App::DrawTlsView()
{
    ImGui::BeginChild("TLS View");

    ImGui::Text("TLS Directory");
    ImGui::Separator();

    // =========================================================
    // 无 TLS
    // =========================================================
    if (tlsData.StartAddressOfRawData.empty())
    {
        ImGui::Text(u8"当前PE不存在TLS目录");
        ImGui::EndChild();
        return;
    }

    // =========================================================
    // 高度划分
    // =========================================================

    float availY = ImGui::GetContentRegionAvail().y;

    // 上：基本信息
    float topH = availY * 0.35f;

    // 中：Callbacks
    float midH = availY * 0.25f;

    // 下：RawData
    float bottomH = availY - topH - midH;

    // =========================================================
    // 1. TLS 基本信息
    // =========================================================

    ImGui::BeginChild(
        "TLS Basic Info",
        ImVec2(0, topH),
        true,
        ImGuiWindowFlags_HorizontalScrollbar
    );

    ImGui::Text("TLS Information");
    ImGui::Separator();

    ImGuiTableFlags infoFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("TLSInfoTable", 2, infoFlags))
    {
        ImGui::TableSetupColumn(
            "Field",
            ImGuiTableColumnFlags_WidthFixed,
            240.0f
        );

        ImGui::TableSetupColumn(
            "Value",
            ImGuiTableColumnFlags_WidthStretch
        );

        ImGui::TableHeadersRow();

        auto DrawRow =
            [](const char* field, const std::string& value)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", field);

                ImGui::TableSetColumnIndex(1);

                ImGui::PushFont(g_HexFont);
                ImGui::Text("%s", value.c_str());
                ImGui::PopFont();
            };

        DrawRow(
            "StartAddressOfRawData",
            tlsData.StartAddressOfRawData
        );

        DrawRow(
            "EndAddressOfRawData",
            tlsData.EndAddressOfRawData
        );

        DrawRow(
            "AddressOfIndex",
            tlsData.AddressOfIndex
        );

        DrawRow(
            "AddressOfCallBacks",
            tlsData.AddressOfCallBacks
        );

        DrawRow(
            "SizeOfZeroFill",
            tlsData.SizeOfZeroFill
        );

        DrawRow(
            "Characteristics",
            tlsData.Characteristics
        );

        DrawRow(
            "RawData RVA",
            tlsData.rawDataRva
        );

        DrawRow(
            "RawData Size",
            tlsData.rawDataSize
        );

        ImGui::EndTable();
    }

    ImGui::EndChild();

    ImGui::Separator();

    // =========================================================
    // 2. TLS Callback
    // =========================================================

    ImGui::BeginChild(
        "TLS Callback Window",
        ImVec2(0, midH),
        true,
        ImGuiWindowFlags_HorizontalScrollbar
    );

    ImGui::Text("TLS Callbacks");
    ImGui::Separator();

    if (tlsData.callBackRvaArray.empty())
    {
        ImGui::Text(u8"无TLS回调");
    }
    else
    {
        ImGuiTableFlags callbackFlags =
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable(
            "TLSCallbackTable",
            2,
            callbackFlags))
        {
            ImGui::TableSetupScrollFreeze(0, 1);

            ImGui::TableSetupColumn(
                "Index",
                ImGuiTableColumnFlags_WidthFixed,
                100.0f
            );

            ImGui::TableSetupColumn(
                "Callback RVA",
                ImGuiTableColumnFlags_WidthStretch
            );

            ImGui::TableHeadersRow();

            for (size_t i = 0;
                i < tlsData.callBackRvaArray.size();
                ++i)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%llu",
                    (unsigned long long)i);

                ImGui::TableSetColumnIndex(1);

                ImGui::PushFont(g_HexFont);

                ImGui::Text(
                    "%s",
                    tlsData.callBackRvaArray[i].c_str()
                );

                ImGui::PopFont();
            }

            ImGui::EndTable();
        }
    }

    ImGui::EndChild();

    ImGui::Separator();

    // =========================================================
    // 3. TLS RawData
    // =========================================================

    ImGui::BeginChild(
        "TLS RawData Window",
        ImVec2(0, 0),
        true,
        ImGuiWindowFlags_HorizontalScrollbar
    );

    ImGui::Text("TLS RawData");
    ImGui::Separator();

    if (tlsData.rawData.empty())
    {
        ImGui::Text(u8"无TLS RawData");
    }
    else
    {
        DrawHexDump(
            tlsData.rawData.data(),
            tlsData.rawData.size(),
            16
        );
    }

    ImGui::EndChild();

    ImGui::EndChild();
}

void App::DrawImportView()
{
    if (importDatas.empty())
    {
        ImGui::Text("No Import data");
        return;
    }

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
void App::DrawDelayLoadImportView()
{
    if (delayImportDatas.empty())
    {
        ImGui::Text("No delay load Import data");
        return;
    }

    ImGui::BeginChild("Delay Load Import View");
    ImGui::Text("Delay Load Import Information");
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

    if (ImGui::BeginTable("Import Table", 9, topFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);//冻结表头
        ImGui::TableSetupColumn(u8"DLL Name", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn(u8"Attributes", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"DllNameRVA", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"ModuleHandleRVA", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"ImportAddressTableRVA", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"ImportNameTableRVA", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"BoundImportAddressTableRVA", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"UnloadInformationTableRVA", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(u8"TimeDateStamp", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < delayImportDatas.size(); i++)
        {
            ImGui::TableNextRow();
            // 第一列 selectable
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(delayImportDatas[i].diDllInfo.dllName.c_str(), selectedDelayLoadImportIndex == i,
                ImGuiSelectableFlags_SpanAllColumns))
            {
                selectedDelayLoadImportIndex = i;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0x%08x", delayImportDatas[i].diDllInfo.delayLoadDesc.Attributes);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%08x", delayImportDatas[i].diDllInfo.delayLoadDesc.DllNameRVA);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("0x%08x", delayImportDatas[i].diDllInfo.delayLoadDesc.ModuleHandleRVA);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("0x%08x", delayImportDatas[i].diDllInfo.delayLoadDesc.ImportAddressTableRVA);
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("0x%08x", delayImportDatas[i].diDllInfo.delayLoadDesc.ImportNameTableRVA);
            ImGui::TableSetColumnIndex(6);
            ImGui::Text("0x%08x", delayImportDatas[i].diDllInfo.delayLoadDesc.BoundImportAddressTableRVA);
            ImGui::TableSetColumnIndex(7);
            ImGui::Text("0x%08x", delayImportDatas[i].diDllInfo.delayLoadDesc.UnloadInformationTableRVA);
            ImGui::TableSetColumnIndex(8);
            ImGui::Text("0x%08x", delayImportDatas[i].diDllInfo.delayLoadDesc.TimeDateStamp);
        }

        ImGui::EndTable();
    }
    ImGui::EndChild();// ========== 上表结束  ========== 




    ImGui::Separator();
    ImGui::Text("Delay Load Imported Functions");

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

    if (selectedDelayLoadImportIndex != -1 &&
        ImGui::BeginTable("DelayLoadFunctions", 3, bottomFlags))
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
            delayImportDatas[selectedDelayLoadImportIndex].funcsInfo;

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

    if (ImGui::TreeNodeEx("BoundImport", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::IsItemClicked())
        {
            currentView = View_BoundImport;
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("DelayImport", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::IsItemClicked())
        {
            currentView = View_DelayImport;
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("TLS", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::IsItemClicked())
        {
            currentView = View_TLS;
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
    boundImportData = peCore.GetBoundImportData();
    delayImportDatas = peCore.GetDelayImportData();
    tlsData = peCore.GetTlsDirectoryData();
}

void App::CloseFile()
{
    selectedResData.dataRva = 0;
    selectedResData.typeId = -1;
    selectedResData.resDataEntryRva = 0;
    selectedResData.dataSize = 0;
    selectedImportIndex = -1;
    selectedRelocationIndex = -1;

    // 释放可能上传到 GPU 的图标纹理（D3D11 的 SRV）
    if (iconTexture)
    {
        ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)(iconTexture);
        if (srv) srv->Release();
        iconTexture = 0;
    }

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

static ID3D11ShaderResourceView* UploadTexture_D3D11(const unsigned char* rgba, int w, int h, ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = rgba;
    sd.SysMemPitch = w * 4;

    ID3D11Texture2D* tex = nullptr;
    if (FAILED(dev->CreateTexture2D(&desc, &sd, &tex))) return nullptr;

    ID3D11ShaderResourceView* srv = nullptr;
    dev->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    return srv; // return as ImTextureID
}

// helper: 找到选中叶子节点对应的顶层资源类型 id（例如 3 = ICON, 14 = GROUP_ICON）
// DFS（深度优先搜索）：查找目标节点对应的顶层资源类型 ID
// @root: 资源树根节点
// @target: 目标节点指针
static int GetTopLevelTypeId(const ResourceNode& root, const ResourceNode* target)
{
    // DFS：当找到 target 的子树时，返回最上层的 type id（level==1）
    int foundType = -1;
    std::function<bool(const ResourceNode&, int)> dfs = [&](const ResourceNode& node, int currentType)->bool {
        int nextType = currentType;
        if (node.level == 1) nextType = node.id; // record type
        if (&node == target) { foundType = nextType; return true; }
        for (auto& c : node.children) {
            if (dfs(c, nextType)) return true;
        }
        return false;
        };
    dfs(root, -1);
    return foundType;
}

// helper: 在资源树中查找 RT_ICON（type==3）且 nameId==iconId 的 rawData（返回 true 且填充 outData）
// 遍历资源树，查找指定 ID 的图标资源
// @root: 资源树根节点
// @iconId: 图标 ID
// @outData: 输出的图标原始数据
static bool FindIconRawById(const ResourceNode& root, WORD iconId, std::vector<BYTE>& outData)
{
    // root.children[level1] are types; find type==3
    for (const auto& typeNode : root.children) {
        if (typeNode.level == 1 && typeNode.id == 3) {
            // typeNode.children are name nodes (level2)
            for (const auto& nameNode : typeNode.children) {
                if (!nameNode.isNamed && nameNode.id == iconId) {
                    // pick first language child (level3) that has data
                    for (const auto& langNode : nameNode.children) {
                        if (langNode.data.has_value()) {
                            outData = langNode.data.value().rawData;
                            return true;
                        }
                    }
                }
            }
        }
    }
    // fallback: search whole tree for a leaf whose id==iconId under type==3
    std::function<bool(const ResourceNode&)> dfs = [&](const ResourceNode& node)->bool {
        if (node.level == 2 && !node.isNamed && node.id == iconId) {
            for (const auto& langNode : node.children) {
                if (langNode.data.has_value()) {
                    outData = langNode.data.value().rawData;
                    return true;
                }
            }
        }
        for (auto& c : node.children) if (dfs(c)) return true;
        return false;
        };
    return dfs(root);
}

// Build ICO file from GROUP_ICON raw bytes (groupData) and underlying RT_ICON entries in resource tree.
// outIco will contain full ICO file bytes on success.
// 从 GROUP_ICON 和相关的 RT_ICON 资源构建 ICO 文件
// @root: 资源树根节点
// @groupData: GROUP_ICON 原始数据
// @outIco: 输出的 ICO 文件字节数据
static bool BuildIcoFromGroup(const ResourceNode& root, const ResourceData& groupData, std::vector<BYTE>& outIco)
{
    const BYTE* p = groupData.rawData.data();
    size_t sz = groupData.rawData.size();
    if (sz < 6) return false;
    // ICONDIR / GRPICONDIR header: WORD reserved, WORD type, WORD count
    WORD reserved = *(const WORD*)(p + 0);
    WORD type = *(const WORD*)(p + 2);
    WORD count = *(const WORD*)(p + 4);
    if (reserved != 0 || (type != 1 && type != 2) || count == 0) return false;
    size_t expectedHeader = 6 + (size_t)count * 14; // GRPICONDIRENTRY is 14 bytes
    if (sz < expectedHeader) return false;

    // parse entries
    struct GrpEntry { BYTE w; BYTE h; BYTE colorCount; BYTE reserved; WORD planes; WORD bitCount; DWORD bytesInRes; WORD nID; };
    std::vector<GrpEntry> entries;
    entries.reserve(count);
    const BYTE* pe = p + 6;
    for (int i = 0; i < count; ++i) {
        if ((size_t)(pe - p) + 14 > sz) return false;
        GrpEntry e;
        e.w = pe[0];
        e.h = pe[1];
        e.colorCount = pe[2];
        e.reserved = pe[3];
        e.planes = *(const WORD*)(pe + 4);
        e.bitCount = *(const WORD*)(pe + 6);
        e.bytesInRes = *(const DWORD*)(pe + 8);
        e.nID = *(const WORD*)(pe + 12);
        entries.push_back(e);
        pe += 14;
    }

    // Build ICO: ICONDIR + ICONDIRENTRY array + image data
    outIco.clear();
    // ICONDIR header
    outIco.resize(6);
    outIco[0] = 0; outIco[1] = 0;
    outIco[2] = 1; outIco[3] = 0; // type = 1 for icons
    outIco[4] = (BYTE)(count & 0xFF); outIco[5] = (BYTE)((count >> 8) & 0xFF);

    // reserve space for ICONDIRENTRY (16 bytes each)
    size_t entriesOffset = outIco.size();
    outIco.resize(outIco.size() + count * 16);

    // append image data, keep offsets
    size_t imageDataOffset = outIco.size();
    for (int i = 0; i < count; ++i) {
        auto& ge = entries[i];
        // find corresponding RT_ICON raw data by ge.nID
        std::vector<BYTE> iconRaw;
        if (!FindIconRawById(root, ge.nID, iconRaw)) {
            return false; // missing underlying icon resource
        }
        // Some RT_ICON raw data sizes may not match bytesInRes; use actual size
        DWORD bytesInRes = (DWORD)iconRaw.size();
        // fill ICONDIRENTRY at outIco[entriesOffset + i*16]
        BYTE* entryPtr = outIco.data() + entriesOffset + i * 16;
        entryPtr[0] = ge.w;
        entryPtr[1] = ge.h;
        entryPtr[2] = ge.colorCount;
        entryPtr[3] = ge.reserved;
        *(WORD*)(entryPtr + 4) = ge.planes;
        *(WORD*)(entryPtr + 6) = ge.bitCount;
        *(DWORD*)(entryPtr + 8) = bytesInRes;
        *(DWORD*)(entryPtr + 12) = (DWORD)imageDataOffset;

        // append iconRaw bytes
        outIco.insert(outIco.end(), iconRaw.begin(), iconRaw.end());
        imageDataOffset += bytesInRes;
    }

    return true;
}

// Definitions for parsing helpers
static bool ParseIconRaw(const std::vector<BYTE>& iconRaw, int& outW, int& outH, std::vector<BYTE>& outRGBA)
{
    if (iconRaw.size() < sizeof(BITMAPINFOHEADER)) return false;
    const BITMAPINFOHEADER* bih = reinterpret_cast<const BITMAPINFOHEADER*>(iconRaw.data());
    if (bih->biSize != sizeof(BITMAPINFOHEADER)) return false;
    int w = bih->biWidth;
    int h = bih->biHeight; // may be doubled (color+mask)
    int bpp = bih->biBitCount;
    if (w <= 0 || h == 0) return false;

    int colorH = h;
    if (h == w * 2 || (h % 2 == 0 && (h / 2) <= 1024))
        colorH = h / 2;

    const BYTE* p = iconRaw.data() + bih->biSize;
    size_t remaining = iconRaw.size() - bih->biSize;

    if (bpp == 32) {
        size_t needed = (size_t)w * colorH * 4;
        if (remaining < needed) return false;
        outW = w; outH = colorH;
        outRGBA.resize((size_t)w * colorH * 4);
        const BYTE* src = p;
        for (int y = 0; y < colorH; ++y) {
            const BYTE* srcLine = src + (size_t)(colorH - 1 - y) * (w * 4);
            BYTE* dstLine = outRGBA.data() + (size_t)y * (w * 4);
            for (int x = 0; x < w; ++x) {
                dstLine[x * 4 + 0] = srcLine[x * 4 + 2];
                dstLine[x * 4 + 1] = srcLine[x * 4 + 1];
                dstLine[x * 4 + 2] = srcLine[x * 4 + 0];
                dstLine[x * 4 + 3] = srcLine[x * 4 + 3];
            }
        }
        return true;
    }
    else if (bpp == 24) {
        size_t stride = ((w * 3 + 3) / 4) * 4;
        size_t needed = (size_t)stride * colorH;
        if (remaining < needed) return false;
        outW = w; outH = colorH;
        outRGBA.resize((size_t)w * colorH * 4);
        const BYTE* src = p;
        for (int y = 0; y < colorH; ++y) {
            const BYTE* srcLine = src + (size_t)(colorH - 1 - y) * stride;
            BYTE* dstLine = outRGBA.data() + (size_t)y * (w * 4);
            for (int x = 0; x < w; ++x) {
                const BYTE* s = srcLine + x * 3;
                dstLine[x * 4 + 0] = s[2];
                dstLine[x * 4 + 1] = s[1];
                dstLine[x * 4 + 2] = s[0];
                dstLine[x * 4 + 3] = 255;
            }
        }
        return true;
    }

    return false;
}

static bool TryParseFirstIconInGroup(const ResourceNode& root, const ResourceData& groupData, int& outW, int& outH, std::vector<BYTE>& outRGBA)
{
    const BYTE* p = groupData.rawData.data();
    size_t sz = groupData.rawData.size();
    if (sz < 6) return false;
    WORD count = *(const WORD*)(p + 4);
    const BYTE* pe = p + 6;
    for (int i = 0; i < count; ++i) {
        if ((size_t)(pe - p) + 14 > sz) break;
        WORD nID = *(const WORD*)(pe + 12);
        std::vector<BYTE> iconRaw;
        if (FindIconRawById(root, nID, iconRaw)) {
            if (ParseIconRaw(iconRaw, outW, outH, outRGBA)) return true;
        }
        pe += 14;
    }
    return false;
}

