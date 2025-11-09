#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static char const* getBaseType(int type)
{
    switch (type) {
    default:
    case 0: return "Tetra";
    case 1: return "Octa";
    case 2: return "Icosa";
    }
    return nullptr;
}

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
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
    int basePivot = false;
    int mapCoords = true;

    if (paramBlock.size() > 4) {
        hemisphere = std::get<int>(paramBlock[3]);
        smooth = std::get<int>(paramBlock[4]);
    }

    if (paramBlock.size() > 6) {
        basePivot = std::get<int>(paramBlock[5]);
        mapCoords = std::get<int>(paramBlock[6]);
    }

    node.text = node.text + format("%-24s : %s", "Primitive", "GeoSphere") + '\n';
    node.text = node.text + format("%-24s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%-24s : %d", "Segments", segments) + '\n';
    node.text = node.text + format("%-24s : %s", "Geodesic Base Type", getBaseType(baseType)) + '\n';
    node.text = node.text + format("%-24s : %s", "Smooth", getBoolean(smooth)) + '\n';
    node.text = node.text + format("%-24s : %s", "Hemisphere", getBoolean(hemisphere)) + '\n';
    node.text = node.text + format("%-24s : %s", "Base To Pivot", getBoolean(basePivot)) + '\n';
    node.text = node.text + format("%-24s : %s", "Generate Mapping Coords", getBoolean(mapCoords)) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(GSPHERE_CLASS_ID, primitive);
