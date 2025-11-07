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
    if (paramBlock.size() <= 3)
        return false;

    float length = std::get<float>(paramBlock[0]);
    float width = std::get<float>(paramBlock[1]);
    int lengthSegments = std::get<int>(paramBlock[2]);
    int widthSegments = std::get<int>(paramBlock[3]);

    node.vertex = {
        { -length, -width, 0 },
        {  length, -width, 0 },
        { -length,  width, 0 },
        {  length,  width, 0 },
    };

    node.text += format("Primitive : %s", "Plane") + '\n';
    node.text += format("Length : %f", length) + '\n';
    node.text += format("Width : %f", width) + '\n';
    node.text += format("Length Segments : %d", lengthSegments) + '\n';
    node.text += format("Width Segments : %d", widthSegments) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(class64(PLANE_CLASS_ID), primitive);
