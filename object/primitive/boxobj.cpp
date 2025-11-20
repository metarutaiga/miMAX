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

    float diffX = width / widthSegments;
    float diffY = length / lengthSegments;
    float diffZ = height / heightSegments;

    Point3 max = { length / 2, width / 2, height };
    Point3 min = { -length / 2, -width / 2, 0 };

    for (int i = 0; i < 6; ++i) {
        Point3 start = {};
        Point3 diffA = {};
        Point3 diffB = {};
        int segmentA = 0;
        int segmentB = 0;
        int baseIndex = (int)node.vertex.size();

        switch (i) {
        case 0:
        case 1:
            start = { min[0], min[1], (i == 0) ? min[2] : max[2] };
            diffA = { diffX, 0, 0 };
            diffB = { 0, diffY, 0 };
            segmentA = lengthSegments;
            segmentB = widthSegments;
            break;
        case 2:
        case 3:
            start = { min[0], (i == 2) ? min[1] : max[1], min[2] };
            diffA = { diffX, 0, 0 };
            diffB = { 0, 0, diffZ };
            segmentA = widthSegments;
            segmentB = heightSegments;
            break;
        case 4:
        case 5:
            start = { (i == 4) ? min[0] : max[0], min[1], min[2] };
            diffA = { 0, diffY, 0 };
            diffB = { 0, 0, diffZ };
            segmentA = lengthSegments;
            segmentB = heightSegments;
            break;
        }

        for (int a = 0; a <= segmentA; ++a) {
            float v = a / float(segmentA);
            Point3 point = start + diffA * a;
            for (int b = 0; b <= segmentB; ++b) {
                node.vertex.push_back(point + diffB * b);
                if (mapCoords) {
                    float u = b / float(segmentB);
                    node.texture.push_back({ u, v, 0 });
                }
                if (a != segmentA && b != segmentB) {
                    node.vertexArray.push_back({});
                    node.vertexArray.back().push_back(baseIndex + (a + 0) * (segmentB + 1) + (b + 0));
                    node.vertexArray.back().push_back(baseIndex + (a + 1) * (segmentB + 1) + (b + 0));
                    node.vertexArray.back().push_back(baseIndex + (a + 0) * (segmentB + 1) + (b + 1));
                    node.vertexArray.back().push_back(baseIndex + (a + 1) * (segmentB + 1) + (b + 1));
                    if (mapCoords) {
                        node.textureArray.push_back({});
                        node.textureArray.back().push_back(baseIndex + (a + 0) * (segmentB + 1) + (b + 0));
                        node.textureArray.back().push_back(baseIndex + (a + 1) * (segmentB + 1) + (b + 0));
                        node.textureArray.back().push_back(baseIndex + (a + 0) * (segmentB + 1) + (b + 1));
                        node.textureArray.back().push_back(baseIndex + (a + 1) * (segmentB + 1) + (b + 1));
                    }
                }
            }
        }
    }
    return true;
}

static bool register_object = miMAXNode::RegisterObject(BOXOBJ_CLASS_ID, primitive);
