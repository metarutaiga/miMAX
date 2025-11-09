#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 6)
        return false;

    float length = std::get<float>(paramBlock[0]);
    float width = std::get<float>(paramBlock[1]);
    int lengthSegments = std::get<int>(paramBlock[3]);
    int widthSegments = std::get<int>(paramBlock[4]);
    float scale = 1.0f;
    float density = 1.0f;
    int mapCoords = std::get<int>(paramBlock[6]);

    if (paramBlock.size() > 7) {
        density = std::get<float>(paramBlock[7]);
    }

    if (paramBlock.size() > 8) {
        scale = std::get<float>(paramBlock[8]);
    }

    node.text = node.text + format("%-24s : %s", "Primitive", "Grid") + '\n';
    node.text = node.text + format("%-24s : %g", "Length", length) + '\n';
    node.text = node.text + format("%-24s : %g", "Width", width) + '\n';
    node.text = node.text + format("%-24s : %d", "Length Segments", lengthSegments) + '\n';
    node.text = node.text + format("%-24s : %d", "Width Segments", widthSegments) + '\n';
    node.text = node.text + format("%-24s : %g", "Scale", scale) + '\n';
    node.text = node.text + format("%-24s : %g", "Density", density) + '\n';
    node.text = node.text + format("%-24s : %s", "Generate Mapping Coords", getBoolean(mapCoords)) + '\n';

    node.vertex = {
        { -length, -width, 0 },
        {  length, -width, 0 },
        { -length,  width, 0 },
        {  length,  width, 0 },
    };
    return true;
}

static bool register_object = miMAXNode::RegisterObject(GRID_CLASS_ID, primitive);
