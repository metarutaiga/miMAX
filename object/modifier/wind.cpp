#include "miMAX.h"
#include "chunk.h"
#include "format.h"

#if HAVE_3DSMAX_SDK
#include "object/sdk/mods/wind.h"
#endif

static const char* getType(int type)
{
    switch (type) {
    default:
    case 0: return "Planar";
    case 1: return "Spherical";
    }
    return nullptr;
}

static bool modifier(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 6)
        return false;

    float strength = std::get<float>(paramBlock[0]);
    float decay = std::get<float>(paramBlock[1]);
    int type = std::get<int>(paramBlock[2]);
    float iconsize = std::get<float>(paramBlock[3]);
    float turbulence = std::get<float>(paramBlock[4]);
    float frequency = std::get<float>(paramBlock[5]);
    float scale = std::get<float>(paramBlock[6]);
    int hoopson = 0;

    if (paramBlock.size() > 7) {
        hoopson = std::get<float>(paramBlock[7]);
    }

    node.text = node.text + format("%-24s : %s", "Modifier", "Wind") + '\n';
    node.text = node.text + format("%-24s : %g", "Strength", strength) + '\n';
    node.text = node.text + format("%-24s : %g", "Decay", decay) + '\n';
    node.text = node.text + format("%-24s : %s", "Type", getType(type)) + '\n';
    node.text = node.text + format("%-24s : %g", "Display Length", iconsize) + '\n';
    node.text = node.text + format("%-24s : %g", "Turbulence", turbulence) + '\n';
    node.text = node.text + format("%-24s : %g", "Frequency", frequency) + '\n';
    node.text = node.text + format("%-24s : %g", "Scale", scale) + '\n';
    node.text = node.text + format("%-24s : %s", "Hoopson", getBoolean(hoopson)) + '\n';
#if HAVE_3DSMAX_SDK
    void* param[] = {
        &strength,
        &decay,
        &type,
        &iconsize,
        &turbulence,
        &frequency,
        &scale,
        &hoopson,
    };

    Mesh mesh{node.vertex, node.vertexArray, node.texture, node.textureArray};
    IParamBlock block{param};

    BuildMesh(mesh, &block);
#endif
    return true;
}

static bool register_object = miMAXNode::RegisterObject(WINDOBJECT_CLASS_ID, modifier);
