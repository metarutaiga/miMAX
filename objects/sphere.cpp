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
    if (paramBlock.size() <= 6)
        return false;

    float radius = std::get<float>(paramBlock[0]);
    int segments = std::get<int>(paramBlock[1]);
    int smooth = std::get<int>(paramBlock[2]);
    float hemisphere = 1.0f;
    int chopSquash = 0;
    int hemiRecenter = 0;
    int getUVs = true;
    int sliceOn = false;
    float pieSliceFrom = 0.0f;
    float pieSliceTo = 0.0f;

    if (paramBlock.size() > 5) {
        hemisphere = std::get<float>(paramBlock[3]);
        chopSquash = std::get<int>(paramBlock[4]);
        hemiRecenter = std::get<int>(paramBlock[5]);
    }

    if (paramBlock.size() > 6) {
        getUVs = std::get<int>(paramBlock[6]);
    }

    if (paramBlock.size() > 9) {
        sliceOn = std::get<int>(paramBlock[7]);
        pieSliceFrom = std::get<float>(paramBlock[8]);
        pieSliceTo = std::get<float>(paramBlock[9]);
    }

    node.text = node.text + format("%-16s : %s", "Primitive", "Sphere") + '\n';
    node.text = node.text + format("%-16s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%-16s : %d", "Segments", segments) + '\n';
    node.text = node.text + format("%-16s : %s", "Smooth", smooth ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %g", "Hemisphere", hemisphere) + '\n';
    node.text = node.text + format("%-16s : %s", "ChopSquash", chopSquash == 0 ? "Chop" : "Squash") + '\n';
    node.text = node.text + format("%-16s : %s", "Hemi Recenter", hemiRecenter ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %s", "Get UVs", getUVs ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %s", "Slice On", sliceOn ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %g", "Pie Slice From", pieSliceFrom) + '\n';
    node.text = node.text + format("%-16s : %g", "Pie Slice To", pieSliceTo) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(SPHERE_CLASS_ID, primitive);
