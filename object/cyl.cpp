#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
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
    float sliceFrom = 0.0f;
    float sliceTo = 0.0f;
    int mapCoords = true;

    if (paramBlock.size() > 7) {
        sliceOn = std::get<int>(paramBlock[5]);
        sliceFrom = std::get<float>(paramBlock[6]);
        sliceTo = std::get<float>(paramBlock[7]);
    }

    if (paramBlock.size() > 8) {
        capSegments = std::get<int>(paramBlock[3]);
        sides = std::get<int>(paramBlock[4]);
        smooth = std::get<int>(paramBlock[5]);
        sliceOn = std::get<int>(paramBlock[6]);
        sliceFrom = std::get<float>(paramBlock[7]);
        sliceTo = std::get<float>(paramBlock[8]);
    }

    if (paramBlock.size() > 9) {
        mapCoords = std::get<int>(paramBlock[9]);
    }

    node.text = node.text + format("%-24s : %s", "Primitive", "Cylinder") + '\n';
    node.text = node.text + format("%-24s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%-24s : %g", "Height", height) + '\n';
    node.text = node.text + format("%-24s : %d", "Height Segments", heightSegments) + '\n';
    node.text = node.text + format("%-24s : %d", "Cap Segments", capSegments) + '\n';
    node.text = node.text + format("%-24s : %d", "Sides", sides) + '\n';
    node.text = node.text + format("%-24s : %s", "Smooth", getBoolean(smooth)) + '\n';
    node.text = node.text + format("%-24s : %s", "Slice On", getBoolean(sliceOn)) + '\n';
    node.text = node.text + format("%-24s : %g", "Slice From", sliceFrom) + '\n';
    node.text = node.text + format("%-24s : %g", "Slice To", sliceTo) + '\n';
    node.text = node.text + format("%-24s : %s", "Generate Mapping Coords", getBoolean(mapCoords)) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(CYLINDER_CLASS_ID, primitive);
