#include "miMAX.h"
#include "chunk.h"
#include "format.h"

#if HAVE_3DSMAX_SDK
#include "object/sdk/prim/gridobj.h"
#endif

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 4)
        return false;

    float length = std::get<float>(paramBlock[0]);
    float width = std::get<float>(paramBlock[1]);
    int widthSegments = std::get<int>(paramBlock[2]);
    int lengthSegments = std::get<int>(paramBlock[3]);
    int mapCoords = std::get<int>(paramBlock[4]);
    float density = 1.0f;
    float scale = 1.0f;

    if (paramBlock.size() > 5) {
        density = std::get<float>(paramBlock[4]);
        mapCoords = std::get<int>(paramBlock[5]);
    }

    if (paramBlock.size() > 6) {
        scale = std::get<float>(paramBlock[5]);
        mapCoords = std::get<int>(paramBlock[6]);
    }

    node.text = node.text + format("%-24s : %s", "Primitive", "Grid") + '\n';
    node.text = node.text + format("%-24s : %g", "Length", length) + '\n';
    node.text = node.text + format("%-24s : %g", "Width", width) + '\n';
    node.text = node.text + format("%-24s : %d", "Length Segments", lengthSegments) + '\n';
    node.text = node.text + format("%-24s : %d", "Width Segments", widthSegments) + '\n';
    node.text = node.text + format("%-24s : %s", "Generate Mapping Coords", getBoolean(mapCoords)) + '\n';
    node.text = node.text + format("%-24s : %g", "Density", density) + '\n';
    node.text = node.text + format("%-24s : %g", "Scale", scale) + '\n';
#if HAVE_3DSMAX_SDK
    void* param[] = {
        &length,
        &width,
        &widthSegments,
        &lengthSegments,
        &density,
        &scale,
        &mapCoords,
    };

    Mesh mesh{node.vertex, node.vertexArray, node.texture, node.textureArray};
    IParamBlock block{param};

    BuildMesh(mesh, &block);
#else
    Point3 diffX = { length / lengthSegments, 0, 0 };
    Point3 diffY = { 0, width / widthSegments, 0 };
    Point3 min = { -length / 2, -width / 2, 0 };

    for (int a = 0; a <= lengthSegments; ++a) {
        float v = a / float(lengthSegments);
        Point3 point = min + diffX * a;
        for (int b = 0; b <= widthSegments; ++b) {
            node.vertex.push_back(point + diffY * b);
            if (mapCoords) {
                float u = b / float(widthSegments);
                node.texture.push_back({ u, v, 0 });
            }
            if (a != lengthSegments && b != widthSegments) {
                node.vertexArray.push_back({});
                node.vertexArray.back().push_back((a + 0) * (widthSegments + 1) + (b + 0));
                node.vertexArray.back().push_back((a + 1) * (widthSegments + 1) + (b + 0));
                node.vertexArray.back().push_back((a + 1) * (widthSegments + 1) + (b + 1));
                node.vertexArray.back().push_back((a + 0) * (widthSegments + 1) + (b + 1));
                if (mapCoords) {
                    node.textureArray.push_back({});
                    node.textureArray.back().push_back((a + 0) * (widthSegments + 1) + (b + 0));
                    node.textureArray.back().push_back((a + 1) * (widthSegments + 1) + (b + 0));
                    node.textureArray.back().push_back((a + 1) * (widthSegments + 1) + (b + 1));
                    node.textureArray.back().push_back((a + 0) * (widthSegments + 1) + (b + 1));
                }
            }
        }
    }
#endif
    return true;
}

static bool register_object = miMAXNode::RegisterObject(GRID_CLASS_ID, primitive);
