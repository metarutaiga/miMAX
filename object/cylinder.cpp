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
    if (paramBlock.size() <= 4)
        return false;

    float radius = std::get<float>(paramBlock[0]);
    float height = std::get<float>(paramBlock[1]);
    int heightSegments = std::get<int>(paramBlock[2]);
    int capSegments = 1;
    int sides = std::get<int>(paramBlock[3]);
    int smooth = std::get<int>(paramBlock[4]);
    int sliceOn = false;
    float pieSliceFrom = 0.0f;
    float pieSliceTo = 0.0f;
    int getUVs = true;

    if (paramBlock.size() > 7) {
        sliceOn = std::get<int>(paramBlock[5]);
        pieSliceFrom = std::get<float>(paramBlock[6]);
        pieSliceTo = std::get<float>(paramBlock[7]);
    }

    if (paramBlock.size() > 8) {
        capSegments = std::get<int>(paramBlock[8]);
    }

    if (paramBlock.size() > 9) {
        getUVs = std::get<int>(paramBlock[9]);
    }

    node.text = node.text + format("%-16s : %s", "Primitive", "Cylinder") + '\n';
    node.text = node.text + format("%-16s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%-16s : %g", "Height", height) + '\n';
    node.text = node.text + format("%-16s : %d", "Height Segments", heightSegments) + '\n';
    node.text = node.text + format("%-16s : %d", "Cap Segments", capSegments) + '\n';
    node.text = node.text + format("%-16s : %d", "Sides", sides) + '\n';
    node.text = node.text + format("%-16s : %s", "Smooth", smooth ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %s", "Slice On", sliceOn ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %g", "Pie Slice From", pieSliceFrom) + '\n';
    node.text = node.text + format("%-16s : %g", "Pie Slice To", pieSliceTo) + '\n';
    node.text = node.text + format("%-16s : %s", "Get UVs", getUVs ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(CYLINDER_CLASS_ID, primitive);
