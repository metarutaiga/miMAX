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
    int segments = std::get<int>(paramBlock[1]);
    bool smooth = std::get<int>(paramBlock[2]);
    float hemisphere = std::get<float>(paramBlock[3]);
    int chopSquash = std::get<int>(paramBlock[4]);
    bool hemiRecenter = std::get<int>(paramBlock[5]);
    bool getUVs = std::get<int>(paramBlock[6]);
    bool sliceOn = std::get<int>(paramBlock[7]);
    float pieSliceFrom = std::get<float>(paramBlock[8]);
    float pieSliceTo = std::get<float>(paramBlock[9]);

    node.text = node.text + format("%s : %s", "Primitive", "Sphere") + '\n';
    node.text = node.text + format("%s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%s : %d", "Segments", segments) + '\n';
    node.text = node.text + format("%s : %s", "Smooth", smooth ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %g", "Hemisphere", hemisphere) + '\n';
    node.text = node.text + format("%s : %s", "ChopSquash", chopSquash == 0 ? "Chop" : "Squash") + '\n';
    node.text = node.text + format("%s : %s", "Hemi Recenter", hemiRecenter ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %s", "Get UVs", getUVs ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %s", "Slice On", sliceOn ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %g", "Pie Slice From", pieSliceFrom) + '\n';
    node.text = node.text + format("%s : %g", "Pie Slice To", pieSliceTo) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(SPHERE_CLASS_ID, primitive);
