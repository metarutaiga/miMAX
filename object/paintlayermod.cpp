#define __MIMAX_INTERNAL__
#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool modifier(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pColorChunk = getChunk(child, 0x2512);
    if (pColorChunk == nullptr)
        return false;

    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 1)
        return false;

    switch (std::get<int>(paramBlock[1])) {
    default:
        node.vertexColor = getProperty<Point3>(*pColorChunk, 0x0110);
        node.text = node.text + format("Vertex Color : %zd", node.vertexColor.size()) + '\n';
        break;
    case -1:
        node.vertexIllum = getProperty<Point3>(*pColorChunk, 0x0110);
        node.text = node.text + format("Vertex Illum : %zd", node.vertexIllum.size()) + '\n';
        break;
    case -2:
        node.vertexAlpha = getProperty<Point3>(*pColorChunk, 0x0110);
        node.text = node.text + format("Vertex Alpha : %zd", node.vertexAlpha.size()) + '\n';
        break;
    }
    return true;
}

static bool register_object = miMAXNode::RegisterObject(PAINTLAYERMOD_CLASS_ID, modifier);
