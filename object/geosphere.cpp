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
    int geodesicBaseType = std::get<int>(paramBlock[2]);
    bool smooth = std::get<int>(paramBlock[3]);
    bool hemisphere = std::get<int>(paramBlock[4]);

    node.text += format("Primitive : %s", "GeoSphere") + '\n';
    node.text += format("Radius : %f", radius) + '\n';
    node.text += format("Segments : %d", segments) + '\n';
    node.text += format("Geodesic Base Type : %d", geodesicBaseType) + '\n';
    node.text += format("Smooth : %f", smooth ? "true" : "false") + '\n';
    node.text += format("Hemisphere : %f", hemisphere ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(class64(GSPHERE_CLASS_ID), primitive);
