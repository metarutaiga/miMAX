#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static char const* getChopSquash(int chopSquash)
{
    switch (chopSquash) {
    default:
    case 0: return "Chop";
    case 1: return "Squash";
    }
    return nullptr;
}

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 6)
        return false;

    float radius = std::get<float>(paramBlock[0]);
    int segments = std::get<int>(paramBlock[1]);
    int smooth = std::get<int>(paramBlock[2]);
    float hemisphere = 1.0f;
    int chopSquash = 0;
    int sliceOn = false;
    float sliceFrom = 0.0f;
    float sliceTo = 0.0f;
    int basePivot = 0;
    int mapCoords = true;

    if (paramBlock.size() > 5) {
        hemisphere = std::get<float>(paramBlock[3]);
        chopSquash = std::get<int>(paramBlock[4]);
        basePivot = std::get<int>(paramBlock[5]);
    }

    if (paramBlock.size() > 6) {
        mapCoords = std::get<int>(paramBlock[6]);
    }

    if (paramBlock.size() > 9) {
        sliceOn = std::get<int>(paramBlock[7]);
        sliceFrom = std::get<float>(paramBlock[8]);
        sliceTo = std::get<float>(paramBlock[9]);
    }

    node.text = node.text + format("%-24s : %s", "Primitive", "Sphere") + '\n';
    node.text = node.text + format("%-24s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%-24s : %d", "Segments", segments) + '\n';
    node.text = node.text + format("%-24s : %s", "Smooth", getBoolean(smooth)) + '\n';
    node.text = node.text + format("%-24s : %g", "Hemisphere", hemisphere) + '\n';
    node.text = node.text + format("%-24s : %s", "Chop Squash", getChopSquash(chopSquash)) + '\n';
    node.text = node.text + format("%-24s : %s", "Slice On", getBoolean(sliceOn)) + '\n';
    node.text = node.text + format("%-24s : %g", "Slice From", sliceFrom) + '\n';
    node.text = node.text + format("%-24s : %g", "Slice To", sliceTo) + '\n';
    node.text = node.text + format("%-24s : %s", "Base To Pivot", getBoolean(basePivot)) + '\n';
    node.text = node.text + format("%-24s : %s", "Generate Mapping Coords", getBoolean(mapCoords)) + '\n';

    int latCount = segments;
    int lonCount = segments * 2;
    for (int lat = 0; lat <= latCount; ++lat) {
        float v = lat / float(latCount);
        float theta = v * M_PI;
        for (int lon = 0; lon <= lonCount; ++lon) {
            float u = lon / float(lonCount);
            float phi = u * 2.0f * M_PI;
            float sinTheta = std::sinf(theta);
            float cosTheta = std::cosf(theta);
            float sinPhi = std::sinf(phi);
            float cosPhi = std::cosf(phi);
            Point3 point = { sinTheta * cosPhi, sinTheta * sinPhi, cosTheta };
            node.vertex.push_back(point * radius);
            if (mapCoords) {
                node.texture.push_back({ u, v, 0 });
            }
            if (lat != latCount && lon != lonCount) {
                node.vertexArray.push_back({});
                node.vertexArray.back().push_back((lat + 0) * (lonCount + 1) + (lon + 0));
                node.vertexArray.back().push_back((lat + 1) * (lonCount + 1) + (lon + 0));
                node.vertexArray.back().push_back((lat + 1) * (lonCount + 1) + (lon + 1));
                node.vertexArray.back().push_back((lat + 0) * (lonCount + 1) + (lon + 1));
                if (mapCoords) {
                    node.textureArray.push_back({});
                    node.textureArray.back().push_back((lat + 0) * (lonCount + 1) + (lon + 0));
                    node.textureArray.back().push_back((lat + 1) * (lonCount + 1) + (lon + 0));
                    node.textureArray.back().push_back((lat + 1) * (lonCount + 1) + (lon + 1));
                    node.textureArray.back().push_back((lat + 0) * (lonCount + 1) + (lon + 1));
                }
            }
        }
    }
    return true;
}

static bool register_object = miMAXNode::RegisterObject(SPHERE_CLASS_ID, primitive);
