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
    if (paramBlock.size() <= 10)
        return false;

    float radius1 = std::get<float>(paramBlock[0]);
    float radius2 = std::get<float>(paramBlock[1]);
    float rotation = std::get<float>(paramBlock[2]);
    float twist = std::get<float>(paramBlock[3]);
    int segments = std::get<int>(paramBlock[4]);
    int sides = std::get<int>(paramBlock[5]);
    int smooth = std::get<int>(paramBlock[6]);
    bool sliceOn = std::get<int>(paramBlock[7]);
    float pieSliceFrom = std::get<float>(paramBlock[8]);
    float pieSliceTo = std::get<float>(paramBlock[9]);
    bool getUVs = std::get<int>(paramBlock[10]);

    node.text = node.text + format("%s : %s", "Primitive", "Torus") + '\n';
    node.text = node.text + format("%s : %g", "Radius1", radius1) + '\n';
    node.text = node.text + format("%s : %g", "Radius2", radius2) + '\n';
    node.text = node.text + format("%s : %g", "Rotation", rotation) + '\n';
    node.text = node.text + format("%s : %g", "Twist", twist) + '\n';
    node.text = node.text + format("%s : %d", "Segments", segments) + '\n';
    node.text = node.text + format("%s : %d", "Sides", sides) + '\n';
    node.text = node.text + format("%s : %d", "Smooth", smooth) + '\n';
    node.text = node.text + format("%s : %s", "Slice On", sliceOn ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %g", "Pie Slice From", pieSliceFrom) + '\n';
    node.text = node.text + format("%s : %g", "Pie Slice To", pieSliceTo) + '\n';
    node.text = node.text + format("%s : %s", "Get UVs", getUVs ? "true" : "false") + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(TORUS_CLASS_ID, primitive);
