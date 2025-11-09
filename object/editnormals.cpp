#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool modifier(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pNormalChunk = getChunk(child, 0x2512, 0x0240);
    if (pNormalChunk == nullptr)
        pNormalChunk = getChunk(child, 0x2512, 0x0250);
    if (pNormalChunk == nullptr)
        return false;

    auto normals = getProperty<float>(*pNormalChunk, 0x0110);
    if (normals.empty())
        return false;

    for (size_t i = 1; i + 2 < normals.size(); i += 3) {
        node.normal.push_back({normals[i], normals[i + 1], normals[1 + 2]});
    }
    node.text = node.text + format("Normal : %zd", node.normal.size()) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(EDIT_NORMALS_CLASS_ID, modifier);
