#define __MIMAX_INTERNAL__
#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool primitive(int(*log)(char const*, ...), Chunk const& scene, Chunk const& chunk, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 6)
        return false;

    float width = std::get<float>(paramBlock[0]);
    float depth = std::get<float>(paramBlock[1]);
    float height = std::get<float>(paramBlock[2]);
    int widthSegments = std::get<int>(paramBlock[3]);
    int depthSegments = std::get<int>(paramBlock[4]);
    int heightSegments = std::get<int>(paramBlock[5]);
    bool getUVs = std::get<int>(paramBlock[6]);

    node.text = node.text + format("%s : %s", "Primitive", "Pyramid") + '\n';
    node.text = node.text + format("%s : %g", "Width", width) + '\n';
    node.text = node.text + format("%s : %g", "Depth", depth) + '\n';
    node.text = node.text + format("%s : %g", "Height", height) + '\n';
    node.text = node.text + format("%s : %d", "Width Segments", widthSegments) + '\n';
    node.text = node.text + format("%s : %d", "Depth Segments", depthSegments) + '\n';
    node.text = node.text + format("%s : %d", "Height Segments", heightSegments) + '\n';
    node.text = node.text + format("%s : %s", "Get UVs", getUVs ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(PYRAMID_CLASS_ID, primitive);
