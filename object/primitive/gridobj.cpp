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
    int mapCoords = std::get<int>(paramBlock[6]);
    float density = 1.0f;
    float scale = 1.0f;

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
    node.text = node.text + format("%-24s : %s", "Generate Mapping Coords", getBoolean(mapCoords)) + '\n';
    node.text = node.text + format("%-24s : %g", "Density", density) + '\n';
    node.text = node.text + format("%-24s : %g", "Scale", scale) + '\n';

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
    return true;
}

static bool register_object = miMAXNode::RegisterObject(GRID_CLASS_ID, primitive);
