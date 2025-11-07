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
    float rotation = std::get<float>(paramBlock[2]);
    float twist = std::get<float>(paramBlock[3]);
    int segments = std::get<int>(paramBlock[4]);
    int sides = std::get<int>(paramBlock[5]);
    int smooth = std::get<int>(paramBlock[6]);

    node.text += format("Primitive : %s", "Torus") + '\n';
    node.text += format("Radius1 : %f", radius1) + '\n';
    node.text += format("Radius2 : %f", radius2) + '\n';
    node.text += format("Rotation : %f", rotation) + '\n';
    node.text += format("Twist : %f", twist) + '\n';
    node.text += format("Segments : %d", segments) + '\n';
    node.text += format("Sides : %d", sides) + '\n';
    node.text += format("Smooth : %d", smooth) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(class64(TORUS_CLASS_ID), primitive);
