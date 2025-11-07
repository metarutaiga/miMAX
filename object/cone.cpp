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

    float radius1 = std::get<float>(paramBlock[0]);
    float radius2 = std::get<float>(paramBlock[1]);
    float height = std::get<float>(paramBlock[2]);
    int heightSegments = std::get<int>(paramBlock[3]);
    int capSegments = std::get<int>(paramBlock[4]);
    int sides = std::get<int>(paramBlock[5]);
    bool smooth = std::get<int>(paramBlock[6]);

    node.text += format("Primitive : %s", "Cone") + '\n';
    node.text += format("Radius1 : %f", radius1) + '\n';
    node.text += format("Radius2 : %f", radius2) + '\n';
    node.text += format("Height : %f", height) + '\n';
    node.text += format("Height Segments : %d", heightSegments) + '\n';
    node.text += format("Cap Segments : %d", capSegments) + '\n';
    node.text += format("Sides : %d", sides) + '\n';
    node.text += format("Smooth : %s", smooth ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(class64(CONE_CLASS_ID), primitive);
