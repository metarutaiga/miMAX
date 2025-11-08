#define __MIMAX_INTERNAL__
#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 3)
        return false;

    float radius = std::get<float>(paramBlock[0]);
    int segments = std::get<int>(paramBlock[1]);
    int baseType = std::get<int>(paramBlock[2]);
    int hemisphere = false;
    int smooth = std::get<int>(paramBlock[3]);
    int baseToPivot = false;
    int mapCoords = true;

    if (paramBlock.size() > 4) {
        hemisphere = std::get<int>(paramBlock[4]);
    }

    if (paramBlock.size() > 6) {
        baseToPivot = std::get<int>(paramBlock[5]);
        mapCoords = std::get<int>(paramBlock[6]);
    }

    node.text = node.text + format("%-16s : %s", "Primitive", "GeoSphere") + '\n';
    node.text = node.text + format("%-16s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%-16s : %d", "Segments", segments) + '\n';
    node.text = node.text + format("%-16s : %d", "Base Type", baseType) + '\n';
    node.text = node.text + format("%-16s : %s", "Hemisphere", hemisphere ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %s", "Smooth", smooth ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %s", "Base To Pivot", baseToPivot ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %s", "Mapping Coords", mapCoords ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(GSPHERE_CLASS_ID, primitive);
