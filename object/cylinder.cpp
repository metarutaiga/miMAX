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
    if (paramBlock.size() <= 9)
        return false;

    float radius = std::get<float>(paramBlock[0]);
    float height = std::get<float>(paramBlock[1]);
    int heightSegments = std::get<int>(paramBlock[2]);
    int capSegments = std::get<int>(paramBlock[3]);
    int sides = std::get<int>(paramBlock[4]);
    bool smooth = std::get<int>(paramBlock[5]);
    bool sliceOn = std::get<int>(paramBlock[6]);
    float pieSliceFrom = std::get<float>(paramBlock[7]);
    float pieSliceTo = std::get<float>(paramBlock[8]);
    bool getUVs = std::get<int>(paramBlock[9]);

    node.text = node.text + format("%s : %s", "Primitive", "Cylinder") + '\n';
    node.text = node.text + format("%s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%s : %g", "Height", height) + '\n';
    node.text = node.text + format("%s : %d", "Height Segments", heightSegments) + '\n';
    node.text = node.text + format("%s : %d", "Cap Segments", capSegments) + '\n';
    node.text = node.text + format("%s : %d", "Sides", sides) + '\n';
    node.text = node.text + format("%s : %s", "Smooth", smooth ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %s", "Slice On", sliceOn ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %g", "Pie Slice From", pieSliceFrom) + '\n';
    node.text = node.text + format("%s : %g", "Pie Slice To", pieSliceTo) + '\n';
    node.text = node.text + format("%s : %s", "Get UVs", getUVs ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(CYLINDER_CLASS_ID, primitive);
