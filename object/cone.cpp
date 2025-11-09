#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 8)
        return false;

    float radius1 = std::get<float>(paramBlock[0]);
    float radius2 = std::get<float>(paramBlock[1]);
    float height = std::get<float>(paramBlock[2]);
    int heightSegments = std::get<int>(paramBlock[3]);
    int capSegments = 1;
    int sides = std::get<int>(paramBlock[4]);
    int smooth = std::get<int>(paramBlock[5]);
    int sliceOn = std::get<int>(paramBlock[6]);
    float sliceFrom = std::get<float>(paramBlock[7]);
    float sliceTo = std::get<float>(paramBlock[8]);
    int mapCoords = true;

    if (paramBlock.size() > 9) {
        capSegments = std::get<int>(paramBlock[4]);
        sides = std::get<int>(paramBlock[5]);
        smooth = std::get<int>(paramBlock[6]);
        sliceOn = std::get<int>(paramBlock[7]);
        sliceFrom = std::get<float>(paramBlock[8]);
        sliceTo = std::get<float>(paramBlock[9]);
    }

    if (paramBlock.size() > 10) {
        mapCoords = std::get<int>(paramBlock[10]);
    }

    node.text = node.text + format("%-24s : %s", "Primitive", "Cone") + '\n';
    node.text = node.text + format("%-24s : %g", "Radius1", radius1) + '\n';
    node.text = node.text + format("%-24s : %g", "Radius2", radius2) + '\n';
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

static bool register_object = miMAXNode::RegisterObject(CONE_CLASS_ID, primitive);
