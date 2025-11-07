#define __MIMAX_INTERNAL__
#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool primitive(int(*log)(char const*, ...), Chunk const& scene, Chunk const& chunk, miMAXNode& node)
{
    auto* pPolyChunk = getChunk(chunk, 0x08FE);
    if (pPolyChunk == nullptr)
        return false;
    auto& polyChunk = (*pPolyChunk);

    auto vertexArray = getProperty<int>(polyChunk, 0x0912);
    for (size_t i = 1; i + 4 < vertexArray.size(); i += 5) {
        node.vertexArray.push_back({});
        node.vertexArray.back().push_back(vertexArray[i]);
        node.vertexArray.back().push_back(vertexArray[i + 1]);
        node.vertexArray.back().push_back(vertexArray[i + 2]);
    }

    auto vertex = getProperty<float>(polyChunk, 0x0914);
    for (size_t i = 1; i + 2 < vertex.size(); i += 3) {
        node.vertex.push_back({vertex[i], vertex[i + 1], vertex[i + 2]});
    }

    auto texture = getProperty<float>(polyChunk, 0x0916);
    if (texture.empty())
        texture = getProperty<float>(polyChunk, 0x2394);
    for (size_t i = 1; i + 2 < texture.size(); i += 3) {
        node.texture.push_back({texture[i], texture[i + 1], texture[i + 2]});
    }

    auto textureArray = getProperty<int>(polyChunk, 0x0918);
    if (textureArray.empty())
        textureArray = getProperty<int>(polyChunk, 0x2396);
    for (size_t i = 1; i + 2 < textureArray.size(); i += 3) {
        node.textureArray.push_back({});
        node.textureArray.back().push_back(textureArray[i]);
        node.textureArray.back().push_back(textureArray[i + 1]);
        node.textureArray.back().push_back(textureArray[i + 2]);
    }

    if (node.vertexArray.size() && node.textureArray.size()) {
        if (node.vertexArray.size() != node.textureArray.size()) {
            log("%s is corrupted (%zd:%zd)\n", "Editable Mesh", node.vertexArray.size(), node.textureArray.size());
        }
    }

    size_t totalVertexArray = 0;
    size_t totalTextureArray = 0;
    for (auto& array : node.vertexArray) {
        totalVertexArray += array.size();
    }
    for (auto& array : node.textureArray) {
        totalTextureArray += array.size();
    }

    node.text += format("Primitive : %s", "Editable Mesh") + '\n';
    node.text += format("Vertex : %zd", node.vertex.size()) + '\n';
    node.text += format("Texture : %zd", node.texture.size()) + '\n';
    node.text += format("Vertex Array : %zd (%zd)", node.vertexArray.size(), totalVertexArray) + '\n';
    node.text += format("Texture Array : %zd (%zd)", node.textureArray.size(), totalTextureArray) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(class64(EDITTRIOBJ_CLASS_ID), primitive);
