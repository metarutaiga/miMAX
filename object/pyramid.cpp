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
    if (paramBlock.size() <= 5)
        return false;

    float width = std::get<float>(paramBlock[0]);
    float depth = std::get<float>(paramBlock[1]);
    float height = std::get<float>(paramBlock[2]);
    int widthSegments = std::get<int>(paramBlock[3]);
    int depthSegments = std::get<int>(paramBlock[4]);
    int heightSegments = std::get<int>(paramBlock[5]);

    node.text += format("Primitive : %s", "Pyramid") + '\n';
    node.text += format("Width : %f", width) + '\n';
    node.text += format("Depth : %f", depth) + '\n';
    node.text += format("Height : %f", height) + '\n';
    node.text += format("Width Segments : %d", widthSegments) + '\n';
    node.text += format("Depth Segments : %d", depthSegments) + '\n';
    node.text += format("Height Segments : %d", heightSegments) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(class64(PYRAMID_CLASS_ID), primitive);
