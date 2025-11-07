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

    float radius = std::get<float>(paramBlock[0]);
    float height = std::get<float>(paramBlock[1]);
    int heightSegments = std::get<int>(paramBlock[2]);
    int capSegments = std::get<int>(paramBlock[3]);
    int sides = std::get<int>(paramBlock[4]);
    bool smooth = std::get<int>(paramBlock[5]);

    node.text += format("Primitive : %s", "Cylinder") + '\n';
    node.text += format("Radius : %f", radius) + '\n';
    node.text += format("Height : %f", height) + '\n';
    node.text += format("Height Segments : %d", heightSegments) + '\n';
    node.text += format("Cap Segments : %d", capSegments) + '\n';
    node.text += format("Sides : %d", sides) + '\n';
    node.text += format("Smooth : %s", smooth ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(class64(CYLINDER_CLASS_ID), primitive);
