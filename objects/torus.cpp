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

    float radius1 = std::get<float>(paramBlock[0]);
    float radius2 = std::get<float>(paramBlock[1]);
    float rotation = 0.5f;
    float twist = 0.5f;
    int segments = std::get<int>(paramBlock[2]);
    int sides = std::get<int>(paramBlock[3]);
    int smooth = std::get<int>(paramBlock[4]);
    int sliceOn = false;
    float pieSliceFrom = 0.0f;
    float pieSliceTo = 0.0f;
    int getUVs = true;

    if (paramBlock.size() > 8) {
        sliceOn = std::get<int>(paramBlock[5]);
        pieSliceFrom = std::get<float>(paramBlock[6]);
        pieSliceTo = std::get<float>(paramBlock[7]);
        rotation = std::get<float>(paramBlock[8]);
    }

    if (paramBlock.size() > 9) {
        twist = std::get<float>(paramBlock[9]);
    }

    if (paramBlock.size() > 10) {
        getUVs = std::get<float>(paramBlock[10]);
    }

    node.text = node.text + format("%-16s : %s", "Primitive", "Torus") + '\n';
    node.text = node.text + format("%-16s : %g", "Radius1", radius1) + '\n';
    node.text = node.text + format("%-16s : %g", "Radius2", radius2) + '\n';
    node.text = node.text + format("%-16s : %g", "Rotation", rotation) + '\n';
    node.text = node.text + format("%-16s : %g", "Twist", twist) + '\n';
    node.text = node.text + format("%-16s : %d", "Segments", segments) + '\n';
    node.text = node.text + format("%-16s : %d", "Sides", sides) + '\n';
    node.text = node.text + format("%-16s : %d", "Smooth", smooth) + '\n';
    node.text = node.text + format("%-16s : %s", "Slice On", sliceOn ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %g", "Pie Slice From", pieSliceFrom) + '\n';
    node.text = node.text + format("%-16s : %g", "Pie Slice To", pieSliceTo) + '\n';
    node.text = node.text + format("%-16s : %s", "Get UVs", getUVs ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(TORUS_CLASS_ID, primitive);
