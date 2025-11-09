#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pPolyChunk = getChunk(chunk, 0x08FE);
    if (pPolyChunk == nullptr)
        return false;
    auto& polyChunk = (*pPolyChunk);

    auto vertex = getProperty<float>(polyChunk, 0x0100);
    for (size_t i = 1; i + 3 < vertex.size(); i += 4) {
        node.vertex.push_back({vertex[i + 1], vertex[i + 2], vertex[1 + 3]});
    }

    auto vertexArray = getProperty<uint16_t>(polyChunk, 0x011A);
    for (size_t i = 2; i + 1 < vertexArray.size(); i += 2) {
        uint32_t count = (vertexArray[i] | vertexArray[i + 1] << 16) * 2;
        if (i + 2 + count + 1 > vertexArray.size()) {
            log("%s is corrupted\n", "Editable Poly");
            break;
        }
        i += 2;
        node.vertexArray.push_back({});
        for (size_t j = i, list = i + count; j < list; j += 2) {
            node.vertexArray.back().push_back(vertexArray[j] | vertexArray[j + 1] << 16);
        }
        i += count;
        uint16_t flags = vertexArray[i];
        i += 1;
        if (flags & 0x01)   i += 2;
        if (flags & 0x08)   i += 1;
        if (flags & 0x10)   i += 2;
        if (flags & 0x20)   i += 2 * (count - 6);
        i -= 2;
    }

    auto texture = getProperty<float>(polyChunk, 0x0128);
    for (size_t i = 1; i + 2 < texture.size(); i += 3) {
        node.texture.push_back({texture[i], texture[i + 1], texture[1 + 2]});
    }

    auto textureArray = getProperty<uint32_t>(polyChunk, 0x012B);
    for (size_t i = 0; i < textureArray.size(); ++i) {
        uint32_t count = textureArray[i];
        if (i + 1 + count > textureArray.size()) {
            log("%s is corrupted\n", "Editable Poly");
            break;
        }
        i += 1;
        node.textureArray.push_back({});
        for (size_t j = i, list = i + count; j < list; ++j) {
            node.textureArray.back().push_back(textureArray[j]);
        }
        i += count;
        i -= 1;
    }

    if (node.vertexArray.size() && node.textureArray.size()) {
        bool corrupted = (node.vertexArray.size() != node.textureArray.size());
        if (corrupted == false) {
            for (size_t i = 0; i < node.vertexArray.size() && i < node.textureArray.size(); ++i) {
                if (node.vertexArray[i].size() != node.textureArray[i].size()) {
                    corrupted = true;
                    break;
                }
            }
        }
        if (corrupted) {
            log("%s is corrupted (%zd:%zd)\n", "Editable Poly", node.vertexArray.size(), node.textureArray.size());
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

    node.text = node.text + format("%-24s : %s", "Primitive", "Editable Poly") + '\n';
    node.text = node.text + format("%-24s : %zd", "Vertex", node.vertex.size()) + '\n';
    node.text = node.text + format("%-24s : %zd", "Texture", node.texture.size()) + '\n';
    node.text = node.text + format("%-24s : %zd (%zd)", "Vertex Array", node.vertexArray.size(), totalVertexArray) + '\n';
    node.text = node.text + format("%-24s : %zd (%zd)", "Texture Array", node.textureArray.size(), totalTextureArray) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(EPOLYOBJ_CLASS_ID, primitive);
