#include "miMAX.h"
#include "chunk.h"
#include "format.h"

#if HAVE_3DSMAX_SDK
#include "object/sdk/mods/gravity.h"
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
    if (paramBlock.size() <= 3)
        return false;

    float strength = std::get<float>(paramBlock[0]);
    float decay = std::get<float>(paramBlock[1]);
    int type = std::get<int>(paramBlock[2]);
    float iconsize = std::get<float>(paramBlock[3]);
    int hoopson = 0;

    if (paramBlock.size() > 4) {
        hoopson = std::get<float>(paramBlock[4]);
    }

    node.text = node.text + format("%-24s : %s", "Modifier", "Gravity") + '\n';
    node.text = node.text + format("%-24s : %g", "Strength", strength) + '\n';
    node.text = node.text + format("%-24s : %g", "Decay", decay) + '\n';
    node.text = node.text + format("%-24s : %s", "Type", getType(type)) + '\n';
    node.text = node.text + format("%-24s : %g", "Display Length", iconsize) + '\n';
    node.text = node.text + format("%-24s : %s", "Hoopson", getBoolean(hoopson)) + '\n';
#if HAVE_3DSMAX_SDK
    void* param[] = {
        &strength,
        &decay,
        &type,
        &iconsize,
        &hoopson,
    };

    Mesh mesh{node.vertex, node.vertexArray, node.texture, node.textureArray};
    IParamBlock block{param};

    BuildMesh(mesh, &block);
#endif
    return true;
}

static bool register_object = miMAXNode::RegisterObject(GRAVITYOBJECT_CLASS_ID, modifier);
