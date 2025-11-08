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

    float length = std::get<float>(paramBlock[0]);
    float width = std::get<float>(paramBlock[1]);
    int lengthSegments = std::get<int>(paramBlock[2]);
    int widthSegments = std::get<int>(paramBlock[3]);
    int getUVs = std::get<int>(paramBlock[4]);
    float density = 1.0f;
    float renderScale = 1.0f;

    if (paramBlock.size() > 5) {
        density = std::get<float>(paramBlock[5]);
    }

    if (paramBlock.size() > 6) {
        renderScale = std::get<float>(paramBlock[6]);
    }

    node.text = node.text + format("%-16s : %s", "Primitive", "Grid") + '\n';
    node.text = node.text + format("%-16s : %g", "Length", length) + '\n';
    node.text = node.text + format("%-16s : %g", "Width", width) + '\n';
    node.text = node.text + format("%-16s : %d", "Length Segments", lengthSegments) + '\n';
    node.text = node.text + format("%-16s : %d", "Width Segments", widthSegments) + '\n';
    node.text = node.text + format("%-16s : %s", "Get UVs", getUVs ? "true" : "false") + '\n';
    node.text = node.text + format("%-16s : %g", "Density", density) + '\n';
    node.text = node.text + format("%-16s : %g", "Render Scale", renderScale) + '\n';

    node.vertex = {
        { -length, -width, 0 },
        {  length, -width, 0 },
        { -length,  width, 0 },
        {  length,  width, 0 },
    };
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(GRID_CLASS_ID, primitive);
