#define __MIMAX_INTERNAL__
#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 5)
        return false;

    float length = std::get<float>(paramBlock[0]);
    float width = std::get<float>(paramBlock[1]);
    float height = std::get<float>(paramBlock[2]);
    int lengthSegments = std::get<int>(paramBlock[3]);
    int widthSegments = std::get<int>(paramBlock[4]);
    int heightSegments = std::get<int>(paramBlock[5]);
    int mapCoords = true;

    if (paramBlock.size() > 6) {
        mapCoords = std::get<int>(paramBlock[6]);
    }

    node.text = node.text + format("%-24s : %s", "Primitive", "Box") + '\n';
    node.text = node.text + format("%-24s : %g", "Length", length) + '\n';
    node.text = node.text + format("%-24s : %g", "Width", width) + '\n';
    node.text = node.text + format("%-24s : %g", "Height", height) + '\n';
    node.text = node.text + format("%-24s : %d", "Length Segments", lengthSegments) + '\n';
    node.text = node.text + format("%-24s : %d", "Width Segments", widthSegments) + '\n';
    node.text = node.text + format("%-24s : %d", "Height Segments", heightSegments) + '\n';
    node.text = node.text + format("%-24s : %s", "Generate Mapping Coords", getBoolean(mapCoords)) + '\n';

    node.vertex = {
        { -length, -width, -height },
        {  length, -width, -height },
        { -length,  width, -height },
        {  length,  width, -height },
        { -length, -width,  height },
        {  length, -width,  height },
        { -length,  width,  height },
        {  length,  width,  height },
    };
    return true;
}

static bool register_object = miMAXNode::RegisterObject(BOXOBJ_CLASS_ID, primitive);
