#include "miMAX.h"
#include "chunk.h"
#include "format.h"

#if HAVE_3DSMAX_SDK
#include "object/sdk/prim/torus.h"
#endif

static char const* getSmooth(int smooth)
{
    switch (smooth) {
    default:
    case 0: return "None";
    case 1: return "All";
    case 2: return "Sides";
    case 3: return "Segments";
    }
    return nullptr;
}

static bool primitive(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 4)
        return false;

    float radius1 = std::get<float>(paramBlock[0]);
    float radius2 = std::get<float>(paramBlock[1]);
    float rotation = 0.5f;
    float twist = 0.5f;
    int segments = std::get<int>(paramBlock[2]);
    int sides = std::get<int>(paramBlock[3]);
    int smooth = std::get<int>(paramBlock[4]);
    int sliceOn = false;
    float sliceFrom = 0.0f;
    float sliceTo = 0.0f;
    int mapCoords = true;

    if (paramBlock.size() > 8) {
        rotation = std::get<float>(paramBlock[2]);
        segments = std::get<int>(paramBlock[3]);
        sides = std::get<int>(paramBlock[4]);
        smooth = std::get<int>(paramBlock[5]);
        sliceOn = std::get<int>(paramBlock[6]);
        sliceFrom = std::get<float>(paramBlock[7]);
        sliceTo = std::get<float>(paramBlock[8]);
    }

    if (paramBlock.size() > 9) {
        twist = std::get<float>(paramBlock[3]);
        segments = std::get<int>(paramBlock[4]);
        sides = std::get<int>(paramBlock[5]);
        smooth = std::get<int>(paramBlock[6]);
        sliceOn = std::get<int>(paramBlock[7]);
        sliceFrom = std::get<float>(paramBlock[8]);
        sliceTo = std::get<float>(paramBlock[9]);
    }

    if (paramBlock.size() > 10) {
        mapCoords = std::get<int>(paramBlock[10]);
    }

    node.text = node.text + format("%-24s : %s", "Primitive", "Torus") + '\n';
    node.text = node.text + format("%-24s : %g", "Radius1", radius1) + '\n';
    node.text = node.text + format("%-24s : %g", "Radius2", radius2) + '\n';
    node.text = node.text + format("%-24s : %g", "Rotation", rotation) + '\n';
    node.text = node.text + format("%-24s : %g", "Twist", twist) + '\n';
    node.text = node.text + format("%-24s : %d", "Segments", segments) + '\n';
    node.text = node.text + format("%-24s : %d", "Sides", sides) + '\n';
    node.text = node.text + format("%-24s : %s", "Smooth", getSmooth(smooth)) + '\n';
    node.text = node.text + format("%-24s : %s", "Slice On", getBoolean(sliceOn)) + '\n';
    node.text = node.text + format("%-24s : %g", "Slice From", sliceFrom) + '\n';
    node.text = node.text + format("%-24s : %g", "Slice To", sliceTo) + '\n';
    node.text = node.text + format("%-24s : %s", "Generate Mapping Coords", getBoolean(mapCoords)) + '\n';
#if HAVE_3DSMAX_SDK
    void* param[] = {
        &radius1,
        &radius2,
        &rotation,
        &twist,
        &segments,
        &sides,
        &smooth,
        &sliceOn,
        &sliceFrom,
        &sliceTo,
        &mapCoords,
    };

    Mesh mesh{node.vertex, node.vertexArray, node.texture, node.textureArray};
    IParamBlock block{param};

    BuildMesh(mesh, &block);
#endif
    return true;
}

static bool register_object = miMAXNode::RegisterObject(TORUS_CLASS_ID, primitive);
