#include <mach-o/dyld.h>
#include <sys/dir.h>
#include <functional>
#include <string>
#include "ImGuiHelper.h"
#include "miMAX.h"

static miMAXNode* root;
static std::vector<std::pair<std::string, std::string>> finders;
static int finderIndex;
static std::vector<std::string> messages;
static int messagesFocus;
static int messagesIndex;

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
    ImGui::DockBuilderDockWindow("Finder", left);
    ImGui::DockBuilderDockWindow("Node", middle);
    ImGui::DockBuilderDockWindow("Text", right);
    ImGui::DockBuilderDockWindow("Message", bottom);
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

static bool TreeNode(miMAXNode& node, std::function<void(std::string& text)> select, size_t depth = 0)
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
        int flags = (selected == &child) ? ImGuiTreeNodeFlags_Selected : 0;
        if (depth == 0)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
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
            updated |= TreeNode(child, select, depth + 1);
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

        if (ImGui::Begin("Finder")) {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::SetNextWindowSize(region);
            if (ImGui::ListBox("##100", &finderIndex, [](void* user_data, int index) {
                auto* finders = (std::pair<std::string, std::string>*)user_data;
                return finders[index].first.c_str();
            }, finders.data(), (int)finders.size())) {
                if (finders.size() > finderIndex) {
                    auto& pair = finders[finderIndex];
                    messages.clear();
                    delete root;
                    root = miMAXNode::OpenFile(pair.second.c_str(), Message);
                    messagesFocus = messagesIndex = (int)messages.size() - 1;
                }
            }
        }
        ImGui::End();

        static std::string nodeText;
        if (ImGui::Begin("Node") && root) {
            TreeNode(*root, [](std::string const& text) {
                nodeText = text;
            });
        }
        ImGui::End();

        if (ImGui::Begin("Text")) {
            ImGui::TextUnformatted(nodeText.c_str());
        }
        ImGui::End();

        if (ImGui::Begin("Message")) {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::SetNextWindowSize(region);
            ImGui::ListBox("##400", &messagesIndex, &messagesFocus, [](void* user_data, int index) {
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
