#include "miMAX.h"
#include "chunk.h"
#include "format.h"

#if HAVE_3DSMAX_SDK
#include "object/sdk/prim/pyramid.h"
#endif

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 6)
        return false;

    float width = std::get<float>(paramBlock[0]);
    float depth = std::get<float>(paramBlock[1]);
    float height = std::get<float>(paramBlock[2]);
    int widthSegments = std::get<int>(paramBlock[3]);
    int depthSegments = std::get<int>(paramBlock[4]);
    int heightSegments = std::get<int>(paramBlock[5]);
    int mapCoords = std::get<int>(paramBlock[6]);

    node.text = node.text + format("%-24s : %s", "Primitive", "Pyramid") + '\n';
    node.text = node.text + format("%-24s : %g", "Width", width) + '\n';
    node.text = node.text + format("%-24s : %g", "Depth", depth) + '\n';
    node.text = node.text + format("%-24s : %g", "Height", height) + '\n';
    node.text = node.text + format("%-24s : %d", "Width Segments", widthSegments) + '\n';
    node.text = node.text + format("%-24s : %d", "Depth Segments", depthSegments) + '\n';
    node.text = node.text + format("%-24s : %d", "Height Segments", heightSegments) + '\n';
    node.text = node.text + format("%-24s : %s", "Generate Mapping Coords", getBoolean(mapCoords)) + '\n';
#if HAVE_3DSMAX_SDK
    void* param[] = {
        &width,
        &depth,
        &height,
        &widthSegments,
        &depthSegments,
        &heightSegments,
        &mapCoords,
    };

    Mesh mesh{node.vertex, node.vertexArray, node.texture, node.textureArray};
    IParamBlock block{param};

    BuildMesh(mesh, &block);
#endif
    return true;
}

static bool register_object = miMAXNode::RegisterObject(PYRAMID_CLASS_ID, primitive);
