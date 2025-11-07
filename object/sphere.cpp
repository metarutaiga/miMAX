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
    if (paramBlock.size() <= 4)
        return false;

    float radius = std::get<float>(paramBlock[0]);
    int segments = std::get<int>(paramBlock[1]);
    bool smooth = std::get<int>(paramBlock[2]);
    float hemisphere = std::get<float>(paramBlock[3]);
    int chopSquash = std::get<int>(paramBlock[4]);

    node.text += format("Primitive : %s", "Sphere") + '\n';
    node.text += format("Radius : %f", radius) + '\n';
    node.text += format("Segments : %d", segments) + '\n';
    node.text += format("Smooth : %s", smooth ? "true" : "false") + '\n';
    node.text += format("Hemisphere : %f", hemisphere) + '\n';
    node.text += format("ChopSquash : %s", chopSquash == 0 ? "Chop" : "Squash") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(class64(SPHERE_CLASS_ID), primitive);
