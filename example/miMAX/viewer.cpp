#include <mach-o/dyld.h>
#include <sys/dir.h>
#include <functional>
#include <string>
#include "ImGuiHelper.h"
#include "miMAX.h"

static std::vector<std::pair<std::string, std::string>> finders;
static int finderIndex;
static miMAXNode* root;
static std::string nodeText;
static std::vector<char> dump;
static int dumpIndex;
static std::vector<std::string> messages;
static int messagesFocus;
static int messagesIndex;

static const char* eascii[0x80] = {
    "\xC3\x87",     "\xC3\xBC",     "\xC3\xA9",     "\xC3\xA2",     "\xC3\xA4",     "\xC3\xA0",     "\xC3\xA5",     "\xC3\xA7",
    "\xC3\xAA",     "\xC3\xAB",     "\xC3\xA8",     "\xC3\xAF",     "\xC3\xAE",     "\xC3\xAC",     "\xC3\x84",     "\xC3\x85",
    "\xC3\x89",     "\xC3\xA6",     "\xC3\x86",     "\xC3\xB4",     "\xC3\xB6",     "\xC3\xB2",     "\xC3\xBB",     "\xC3\xB9",
    "\xC3\xBF",     "\xC3\x96",     "\xC3\x9C",     "\xC2\xA2",     "\xC2\xA3",     "\xC2\xA5",     "\xE2\x82\xA7", "\xC6\x92",
    "\xC3\xA1",     "\xC3\xAD",     "\xC3\xB3",     "\xC3\xBA",     "\xC3\xB1",     "\xC3\x91",     "\xC2\xAA",     "\xC2\xBA",
    "\xC2\xBF",     "\xE2\x8C\x90", "\xC2\xAC",     "\xC2\xBD",     "\xC2\xBC",     "\xC2\xA1",     "\xC2\xAB",     "\xC2\xBB",
    "\xE2\x96\x91", "\xE2\x96\x92", "\xE2\x96\x93", "\xE2\x94\x82", "\xE2\x94\xA4", "\xC3\x81",     "\xC3\x82",     "\xC3\x80",
    "\xC2\xA9",     "\xE2\x95\xA3", "\xE2\x95\x91", "\xE2\x95\x97", "\xE2\x95\x9D", "\xE2\x95\x9C", "\xE2\x95\x9B", "\xE2\x94\x90",
    "\xE2\x94\x94", "\xE2\x94\xB4", "\xE2\x94\xAC", "\xE2\x94\x9C", "\xE2\x94\x80", "\xE2\x94\xBC", "\xE2\x95\x9E", "\xE2\x95\x9F",
    "\xE2\x95\x9A", "\xE2\x95\x94", "\xE2\x95\xA9", "\xE2\x95\xA6", "\xE2\x95\xA0", "\xE2\x95\x90", "\xE2\x95\xAC", "\xE2\x95\xA7",
    "\xE2\x95\xA8", "\xE2\x95\xA4", "\xE2\x95\xA5", "\xE2\x95\x99", "\xE2\x95\x98", "\xE2\x95\x92", "\xE2\x95\x93", "\xE2\x95\xAB",
    "\xE2\x95\xAA", "\xE2\x94\x98", "\xE2\x94\x8C", "\xE2\x96\x88", "\xE2\x96\x84", "\xE2\x96\x8C", "\xE2\x96\x90", "\xE2\x96\x80",
    "\xCE\xB1",     "\xC3\x9F",     "\xCE\x93",     "\xCF\x80",     "\xCE\xA3",     "\xCF\x83",     "\xC2\xB5",     "\xCF\x84",
    "\xCE\xA6",     "\xCE\x98",     "\xCE\xA9",     "\xCE\xB4",     "\xE2\x88\x9E", "\xCF\x86",     "\xCE\xB5",     "\xE2\x88\xA9",
    "\xE2\x89\xA1", "\xC2\xB1",     "\xE2\x89\xA5", "\xE2\x89\xA4", "\xE2\x8C\xA0", "\xE2\x8C\xA1", "\xC3\xB7",     "\xE2\x89\x88",
    "\xC2\xB0",     "\xC2\xB7",     "\xE2\x88\x99", "\xE2\x88\x9A", "\xE2\x81\xBF", "\xC2\xB2",     "\xE2\x96\xA0", "\xE2\x8C\x82",
};


static int Message(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int length = vsnprintf(nullptr, 0, format, args) + 1;
    std::string output;
    output.resize(length);
    vsnprintf(output.data(), length, format, args);
    output.pop_back();
    va_end(args);
    messages.emplace_back(output);
    return length;
}

static void Hex(const char* label, std::vector<char>& data, int& index)
{
    ImGui::ListBox(label, &index, [](void* user_data, int index) -> const char* {
        auto& binary = *(std::vector<char>*)user_data;
        auto code = (uint8_t*)binary.data();
        auto size = binary.size();
        auto i = index * 16;

        static char line[256];
        int width = (6 + (2 + 1) * 16);
        int offset = snprintf(line, 256, "%04X: ", i);
        for (int j = 0; j < 16; ++j) {
            if ((i + j) >= size)
                break;
            offset += snprintf(line + offset, 256 - offset, "%02X ", code[(i + j)]);
        }
        if (width > offset) {
            memset(line + offset, ' ', width - offset);
        }

        char* ascii = line + width;
        for (int j = 0; j < 16; ++j) {
            if ((i + j) >= size)
                break;
            uint8_t c = binary[i + j];
            if (c == 0x00 || c == '\n') {
                (*ascii++) = ' ';
            }
            else if (c >= 0x01 && c <= 0x7F) {
                (*ascii++) = c;
            }
            else {
                for (int i = 0; i < 4; ++i) {
                    char u = eascii[c - 0x80][i];
                    if (u == 0)
                        break;
                    (*ascii++) = u;
                }
            }
        }
        (*ascii++) = 0;

        return line;
    }, &data, (int)(data.size() + 15) / 16);
}

void Init()
{
    ImGuiID id = ImGui::GetID("miMAX");

    static bool initialize = false;
    if (initialize) {
        ImGui::DockSpace(id);
        return;
    }
    initialize = true;

    ImGuiID dockid = ImGui::DockBuilderAddNode(id);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(dockid, ImGuiDir_Down, 1.0f / 4.0f, nullptr, &dockid);
    ImGuiID left = ImGui::DockBuilderSplitNode(dockid, ImGuiDir_Left, 1.0f / 5.0f, nullptr, &dockid);
    ImGuiID middle = ImGui::DockBuilderSplitNode(dockid, ImGuiDir_Left, 1.0f / 2.0f, nullptr, &dockid);
    ImGuiID right = dockid;
    ImGui::DockBuilderDockWindow("Finder##100", left);
    ImGui::DockBuilderDockWindow("Node##200", middle);
    ImGui::DockBuilderDockWindow("ClassData", middle);
    ImGui::DockBuilderDockWindow("ClassDirectory", middle);
    ImGui::DockBuilderDockWindow("Config", middle);
    ImGui::DockBuilderDockWindow("DllDirectory", middle);
    ImGui::DockBuilderDockWindow("Scene", middle);
    ImGui::DockBuilderDockWindow("VideoPostQueue", middle);
    ImGui::DockBuilderDockWindow("Text##300", right);
    ImGui::DockBuilderDockWindow("Dump##310", right);
    ImGui::DockBuilderDockWindow("Message##400", bottom);
    ImGui::DockBuilderFinish(dockid);

    static char appPath[4096];
    unsigned int appPathSize = sizeof(appPath);
    _NSGetExecutablePath(appPath, &appPathSize);
    if (char* slash = strrchr(appPath, '/'))
        (*slash) = '\0';

    std::string path = appPath;
    path += "/../../../../../";

    DIR* dir = opendir(path.c_str());
    if (dir) {
        while (struct dirent* dirent = readdir(dir)) {
            if (dirent->d_name[0] == '.')
                continue;
            if (strcasestr(dirent->d_name, ".max") == nullptr)
                continue;
            finders.emplace_back(dirent->d_name, path + dirent->d_name);
        }
        closedir(dir);
    }
    std::stable_sort(finders.begin(), finders.end());
}

static bool TreeNode(miMAXNode& node, std::function<void(std::string& text)> select)
{
    static void* selected;
    bool updated = false;

    if (selected) {
        intptr_t delta = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) delta = -1;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) delta = 1;
        if (delta != 0) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                if (&(*it) != selected)
                    continue;
                auto previous = it;
                if (previous != node.begin())
                    previous--;
                if (delta < 0) {
                    select(previous->text);
                    selected = &(*previous);
                    updated = true;
                    break;
                }
                auto next = it;
                if (next != node.end())
                    next++;
                if (next != node.end()) {
                    select(next->text);
                    selected = &(*next);
                    updated = true;
                    break;
                }
            }
        }
    }

    for (auto& child : node) {
        int flags = 0;
        if (selected == &child)
            flags |= ImGuiTreeNodeFlags_Selected;
        if (child.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        ImGui::PushID(&child);
        bool open = ImGui::TreeNodeEx(child.name.c_str(), flags);
        ImGui::PopID();
        if (open == false)
            continue;

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Name:%s", child.name.c_str());
            ImGui::Text("Position:%g, %g, %g", child.position[0], child.position[1], child.position[2]);
            ImGui::Text("Rotation:%g, %g, %g, %g", child.rotation[0], child.rotation[1], child.rotation[2], child.rotation[3]);
            ImGui::Text("Scale:%g, %g, %g", child.scale[0], child.scale[1], child.scale[2]);
            if (child.keyPosition[0].empty() == false)  ImGui::Text("X Position Key:%zd", child.keyPosition[0].size());
            if (child.keyPosition[1].empty() == false)  ImGui::Text("Y Position Key:%zd", child.keyPosition[1].size());
            if (child.keyPosition[2].empty() == false)  ImGui::Text("Z Position Key:%zd", child.keyPosition[2].size());
            if (child.keyRotation[0].empty() == false)  ImGui::Text("X Rotation Key:%zd", child.keyRotation[0].size());
            if (child.keyRotation[1].empty() == false)  ImGui::Text("Y Rotation Key:%zd", child.keyRotation[1].size());
            if (child.keyRotation[2].empty() == false)  ImGui::Text("Z Rotation Key:%zd", child.keyRotation[2].size());
            if (child.keyScale.empty() == false)        ImGui::Text("Scale Key:%zd", child.keyScale.size());
            if (child.vertex.empty() == false) {
                ImGui::Separator();
                ImGui::Text("Vertex : %zd", child.vertex.size());
                ImGui::Text("Texture : %zd", child.texture.size());
                ImGui::Text("Normal : %zd", child.normal.size());
                ImGui::Text("Vertex Color : %zd", child.vertexColor.size());
//              ImGui::Text("Vertex Illum : %zd", child.vertexIllum.size());
                ImGui::Text("Vertex Alpha : %zd", child.vertexAlpha.size());
                ImGui::Text("Vertex Array : %zd", child.vertexArray.size());
                ImGui::Text("Texture Array : %zd", child.textureArray.size());
//              ImGui::Text("Polygon Array : %zd", child.polygonArray.size());
            }
            ImGui::EndTooltip();

            if (ImGui::IsItemClicked() && selected != &child) {
                select(child.text);
                selected = &child;
            }
        }
        if (child.empty() == false) {
            updated |= TreeNode(child, select);
        }

        ImGui::TreePop();
    }

    return updated;
}

static bool TreeChunk(miMAXNode::Chunk& chunk, std::function<void(uint16_t type, std::vector<char> const& property)> select)
{
    static void* selected;
    bool updated = false;

    if (selected) {
        intptr_t delta = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) delta = -1;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) delta = 1;
        if (delta != 0) {
            size_t index = std::distance(chunk.data(), (miMAXNode::Chunk::value_type*)selected) + delta;
            if (index < chunk.size()) {
                auto& child = chunk[index];
                select(child.type, child.property);
                selected = &child;
                updated = true;
            }
        }
    }

    for (size_t i = 0; i < chunk.size(); ++i) {
        auto& child = chunk[i];

        int flags = 0;
        if (selected == &child)
            flags |= ImGuiTreeNodeFlags_Selected;
        if (child.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        char text[128];
        snprintf(text, 128, "%zX:%s", i, child.name.c_str());

        ImGui::PushID(&child);
        bool open = ImGui::TreeNodeEx(text, flags);
        ImGui::PopID();
        if (open == false)
            continue;

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            if (child.classDllName.empty() == false) {
                ImGui::Text("Index:%zX", i);
                ImGui::Text("Type:%04X", child.type);
                ImGui::Text("Class:%08X-%08X-%08X-%08X", child.classData.dllIndex, child.classData.classID.first, child.classData.classID.second, child.classData.superClassID);
                ImGui::Text("DllFile:%s", child.classDllFile.c_str());
                ImGui::Text("DllName:%s", child.classDllName.c_str());
                ImGui::Text("Name:%s", child.name.c_str());
                if (child.empty()) {
                    ImGui::Text("Size:%zd", child.property.size());
                }
                else {
                    size_t size = 0;
                    std::function<void(miMAXNode::Chunk const&)> traversal = [&](miMAXNode::Chunk const& chunk) {
                        size += chunk.property.size();
                        for (auto const& child : chunk)
                            traversal(child);
                    };
                    traversal(child);
                    ImGui::Text("Size:%zd", size);
                }
            }
            else {
                if (child.empty()) {
                    ImGui::Text("Size:%zd", child.property.size());
                }
                else {
                    size_t size = 0;
                    std::function<void(miMAXNode::Chunk const&)> traversal = [&](miMAXNode::Chunk const& chunk) {
                        size += chunk.property.size();
                        for (auto const& child : chunk)
                            traversal(child);
                    };
                    traversal(child);
                    ImGui::Text("Size:%zd", size);
                }
            }
            ImGui::EndTooltip();

            if (ImGui::IsItemClicked() && selected != &child) {
                select(child.type, child.property);
                selected = &child;
            }
        }
        if (child.empty() == false) {
            updated |= TreeChunk(child, select);
        }

        ImGui::TreePop();
    }

    return updated;
}

bool GUI(ImVec2 screen)
{
    static int resize = 1;
    static ImVec2 window_size = ImVec2(1536.0f, 864.0f);

    bool show = true;
    const char* title = "miMAX";
    ImGui::SetNextWindowSize(window_size, resize > 0 ? ImGuiCond_Always : ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2((screen.x - window_size.x) / 2.0f, (screen.y - window_size.y) / 2.0f), resize > 0 ? ImGuiCond_Always : ImGuiCond_Once);
    resize = -abs(resize);

    if (ImGui::Begin(title, &show, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse)) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));
        ImGui::BeginChild(title, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight()));
        if (ImGui::Button("X")) {
            show = false;
        }
        ImGui::SameLine();
        ImVec2 region = ImGui::GetContentRegionAvail();
        float offset = (region.x - ImGui::CalcTextSize(title).x) / 2.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
        ImGui::TextUnformatted(title);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            switch (abs(resize)) {
            case 1:
                resize = 2;
                window_size.x = screen.x;
                window_size.y = screen.y - 128.0f;
                break;
            case 2:
                resize = 1;
                window_size.x = 1536.0f;
                window_size.y = 864.0f;
                break;
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        Init();

        if (ImGui::Begin("Finder##100")) {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::SetNextWindowSize(region);
            if (ImGui::ListBox("##101", &finderIndex, [](void* user_data, int index) {
                auto* finders = (std::pair<std::string, std::string>*)user_data;
                return finders[index].first.c_str();
            }, finders.data(), (int)finders.size())) {
                if (finders.size() > finderIndex) {
                    auto& pair = finders[finderIndex];
                    nodeText.clear();
                    messages.clear();
                    delete root;
                    root = miMAXNode::OpenFile(pair.second.c_str(), Message);
                    messagesFocus = messagesIndex = (int)messages.size() - 1;
                }
            }
        }
        ImGui::End();

        if (ImGui::Begin("Node##200") && root) {
            miMAXNode& node = (root->size() == 1) ? root->front() : (*root);
            TreeNode(node, [](std::string const& text) {
                nodeText = text;
            });
        }
        ImGui::End();

        for (int i = 0; i < 6; ++i) {
            char const* name = nullptr;
            miMAXNode::Chunk* chunk = nullptr;
            switch (i) {
            case 0: name = "ClassData";         chunk = root ? root->classData : nullptr;       break;
            case 1: name = "ClassDirectory";    chunk = root ? root->classDirectory : nullptr;  break;
            case 2: name = "Config";            chunk = root ? root->config : nullptr;          break;
            case 3: name = "DllDirectory";      chunk = root ? root->dllDirectory : nullptr;    break;
            case 4: name = "Scene";             chunk = root ? root->scene : nullptr;           break;
            case 5: name = "VideoPostQueue";    chunk = root ? root->videoPostQueue : nullptr;  break;
            }
            if (ImGui::Begin(name) && chunk) {
                TreeChunk(*chunk, [](uint16_t type, std::vector<char> const& data) {
                    dump = data;
                    dumpIndex = 0;
                });
            }
            ImGui::End();
        }

        if (ImGui::Begin("Text##300")) {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::InputTextMultiline("##301", nodeText, region, ImGuiInputTextFlags_ReadOnly);
        }
        ImGui::End();

        if (ImGui::Begin("Dump##310")) {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::SetNextWindowSize(region);
            Hex("##311", dump, dumpIndex);
        }
        ImGui::End();

        if (ImGui::Begin("Message##400")) {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::SetNextWindowSize(region);
            ImGui::ListBox("##401", &messagesIndex, &messagesFocus, [](void* user_data, int index) {
                auto* messages = (std::string*)user_data;
                return messages[index].c_str();
            }, messages.data(), (int)messages.size());
            messagesFocus = -1;
        }
        ImGui::End();
    }
    ImGui::End();

    return show;
}
