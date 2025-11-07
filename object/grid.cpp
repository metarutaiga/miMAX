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
    if (paramBlock.size() <= 8)
        return false;

    float length = std::get<float>(paramBlock[0]);
    float width = std::get<float>(paramBlock[1]);
    int lengthSegments = std::get<int>(paramBlock[3]);
    int widthSegments = std::get<int>(paramBlock[4]);
    bool getUVs = std::get<int>(paramBlock[6]);
    float density = std::get<float>(paramBlock[7]);
    float renderScale = std::get<float>(paramBlock[8]);

    node.vertex = {
        { -length, -width, 0 },
        {  length, -width, 0 },
        { -length,  width, 0 },
        {  length,  width, 0 },
    };

    node.text = node.text + format("%s : %s", "Primitive", "Grid") + '\n';
    node.text = node.text + format("%s : %g", "Length", length) + '\n';
    node.text = node.text + format("%s : %g", "Width", width) + '\n';
    node.text = node.text + format("%s : %d", "Length Segments", lengthSegments) + '\n';
    node.text = node.text + format("%s : %d", "Width Segments", widthSegments) + '\n';
    node.text = node.text + format("%s : %s", "Get UVs", getUVs ? "true" : "false") + '\n';
    node.text = node.text + format("%s : %g", "Density", density) + '\n';
    node.text = node.text + format("%s : %g", "Render Scale", renderScale) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterPrimitive(GRID_CLASS_ID, primitive);
