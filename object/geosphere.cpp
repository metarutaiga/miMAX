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

    float radius = std::get<float>(paramBlock[0]);
    int segments = std::get<int>(paramBlock[1]);
    int baseType = std::get<int>(paramBlock[2]);
    bool hemisphere = std::get<int>(paramBlock[3]);
    bool smooth = std::get<int>(paramBlock[4]);
    bool baseToPivot = std::get<int>(paramBlock[5]);
    bool mapCoords = std::get<int>(paramBlock[6]);

    node.text = node.text + format("%s : %s", "Primitive", "GeoSphere") + '\n';
    node.text = node.text + format("%s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%s : %d", "Segments", segments) + '\n';
    node.text = node.text + format("%s : %d", "Base Type", baseType) + '\n';
    node.text = node.text + format("%s : %s", "Hemisphere", hemisphere ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %s", "Smooth", smooth ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %s", "Base To Pivot", baseToPivot ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %s", "Mapping Coords", mapCoords ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(GSPHERE_CLASS_ID, primitive);
